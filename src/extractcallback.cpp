/*
Mod Organizer archive handling

Copyright (C) 2012 Sebastian Herbord. All rights reserved.

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 3 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include <Unknwn.h>
#include "extractcallback.h"

#include "archive.h"
#include "propertyvariant.h"

#include <QDebug>
#include <QDir>

#include <string>

CArchiveExtractCallback::CArchiveExtractCallback(ProgressCallback *progressCallback,
    FileChangeCallback *fileChangeCallback,
    ErrorCallback *errorCallback,
    PasswordCallback *passwordCallback,
    IInArchive *archiveHandler,
    const QString &directoryPath,
    FileData* const *fileData,
    QString *password)
  : m_ArchiveHandler(archiveHandler)
  , m_Total(0)
  , m_DirectoryPath(directoryPath)
  , m_Extracting(false)
  , m_Canceled(false)
  //m_ProcessedFileInfo //not sure how to initialise this!
  , m_OutFileStream()
  , m_FileData(fileData)
  , m_Password(password)
  , m_ProgressCallback(progressCallback)
  , m_FileChangeCallback(fileChangeCallback)
  , m_ErrorCallback(errorCallback)
  , m_PasswordCallback(passwordCallback)
{
}

CArchiveExtractCallback::~CArchiveExtractCallback()
{
  delete m_ProgressCallback;
  delete m_FileChangeCallback;
  delete m_ErrorCallback;
}

STDMETHODIMP CArchiveExtractCallback::SetTotal(UInt64 size)
{
  m_Total = size;
  return S_OK;
}

STDMETHODIMP CArchiveExtractCallback::SetCompleted(const UInt64 *completed)
{
  if (m_ProgressCallback != nullptr) {
    float percentage = static_cast<float>(*completed) / static_cast<float>(m_Total);
    (*m_ProgressCallback)(percentage);
  }

  return m_Canceled ? E_ABORT : S_OK;
}

template <typename T> bool CArchiveExtractCallback::getOptionalProperty(UInt32 index, int property, T *result) const
{
  PropertyVariant prop;
  if (m_ArchiveHandler->GetProperty(index, property, &prop) != S_OK) {
    qDebug() << "Error getting property" << property;
    return false;
  }
  if (prop.is_empty()) {
    return false;
  }
  *result = static_cast<T>(prop);
  return true;
}

template <typename T> T CArchiveExtractCallback::getProperty(UInt32 index, int property) const
{
  PropertyVariant prop;
  if (m_ArchiveHandler->GetProperty(index, property, &prop) != S_OK) {
    throw std::runtime_error("Error getting property");
  }
  return static_cast<T>(prop);
}

STDMETHODIMP CArchiveExtractCallback::GetStream(UInt32 index, ISequentialOutStream **outStream, Int32 askExtractMode)
{
  *outStream = nullptr;
  m_OutFileStream.Release();

  m_FullProcessedPaths.clear();
  m_Extracting = false;

  if (askExtractMode != NArchive::NExtract::NAskMode::kExtract) {
    return S_OK;
  }

  std::vector<QString> filenames = m_FileData[index]->getAndClearOutputFileNames();
  if (filenames.empty()) {
    return S_OK;
  }

  try
  {
    m_ProcessedFileInfo.AttribDefined = getOptionalProperty(index, kpidAttrib, &m_ProcessedFileInfo.Attrib);

    m_ProcessedFileInfo.isDir = getProperty<bool>(index, kpidIsDir);

    //Why do we do this? And if we are doing this, shouldn't we copy the created
    //and accessed times (kpidATime, kpidCTime) as well?
    m_ProcessedFileInfo.MTimeDefined = getOptionalProperty(index, kpidMTime, &m_ProcessedFileInfo.MTime);

    if (m_ProcessedFileInfo.isDir) {
      for (const QString &filename : filenames) {
        // TODO: error handling
        m_DirectoryPath.mkpath(filename);
        m_FullProcessedPaths.push_back(m_DirectoryPath.absoluteFilePath(filename));
      }
    } else {
      for (const QString &filename : filenames) {
        //If the filename contains a '/' we want to make the directory
        int slashPos = filename.lastIndexOf('\\');
        if (slashPos != -1) {
          //Make the containing directory
          m_DirectoryPath.mkpath(filename.left(slashPos));
        }
        QString fullProcessedPath = m_DirectoryPath.absoluteFilePath(filename);
        //If the file already exists, delete it
        if (m_DirectoryPath.exists(filename)) {
          if (! m_DirectoryPath.remove(filename)) {
            reportError("can't delete output file " + fullProcessedPath);
            return E_ABORT;
          }
        }
        m_FullProcessedPaths.push_back(fullProcessedPath);
      }

      m_OutFileStream = new MultiOutputStream;
      if (!m_OutFileStream->Open(m_FullProcessedPaths)) {
        reportError("can not open output file " + m_FullProcessedPaths[0]);
        return E_ABORT;
      }
      //This is messy but I can't find another way of doing it. A simple
      //assignment of m_outFileStream to *outStream doesn't increase the
      //reference count.
      CComPtr<MultiOutputStream> temp(m_OutFileStream);
      *outStream = temp.Detach();
    }

    if (m_FileChangeCallback != nullptr) {
      (*m_FileChangeCallback)(filenames[0]);
    }

    return S_OK;
  }
  catch (std::exception const &e)
  {
    qDebug() << "Caught exception " << e.what() << " in GetStream";
  }
  return E_FAIL;
}

STDMETHODIMP CArchiveExtractCallback::PrepareOperation(Int32 askExtractMode)
{
  if (m_Canceled) {
    return E_ABORT;
  }
  m_Extracting = askExtractMode == NArchive::NExtract::NAskMode::kExtract;
  return S_OK;
}

STDMETHODIMP CArchiveExtractCallback::SetOperationResult(Int32 operationResult)
{
  if (operationResult != NArchive::NExtract::NOperationResult::kOK) {
    switch(operationResult) {
      case NArchive::NExtract::NOperationResult::kUnsupportedMethod: {
        reportError("encoding method unsupported");
      } break;
      case NArchive::NExtract::NOperationResult::kCRCError: {
        reportError("CRC error");
      } break;
      case NArchive::NExtract::NOperationResult::kDataError: {
        reportError("Data error");
      } break;
      default: {
        reportError("Unknown error");
      } break;
    }
  }

  if (m_OutFileStream != nullptr) {
    if (m_ProcessedFileInfo.MTimeDefined) {
      m_OutFileStream->SetMTime(&m_ProcessedFileInfo.MTime);
    }
    RINOK(m_OutFileStream->Close());
  }

  m_OutFileStream.Release();

  if (m_Extracting && m_ProcessedFileInfo.AttribDefined) {
    //this is moderately annoying. I can't do this on the file handle because if
    //the file in question is a directory there isn't a file handle.
    //Also I'd like to convert the attributes to QT attributes but I'm not sure
    //if that's possible. Hence the conversions and strange string.
    for (QString const &filename : m_FullProcessedPaths) {
      std::wstring const fn = L"\\\\?\\" + QDir::toNativeSeparators(filename).toStdWString();
      //Should probably log any errors here somehow
      ::SetFileAttributesW(fn.c_str(), m_ProcessedFileInfo.Attrib);
    }
  }
  return S_OK;
}


STDMETHODIMP CArchiveExtractCallback::CryptoGetTextPassword(BSTR *passwordOut)
{
  // if we've already got a password, don't ask again (and again...)
  if (m_Password->isEmpty() && m_PasswordCallback != nullptr) {
    (*m_PasswordCallback)(m_Password);
  }

  *passwordOut = ::SysAllocString(m_Password->toStdWString().c_str());
  return *passwordOut != 0 ? S_OK : E_OUTOFMEMORY;
}


void CArchiveExtractCallback::SetCanceled(bool aCanceled)
{
  m_Canceled = aCanceled;
}


void CArchiveExtractCallback::reportError(const QString &message)
{
  if (m_ErrorCallback != nullptr) {
    (*m_ErrorCallback)(message);
  }
}
