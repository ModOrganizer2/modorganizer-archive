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

#include <shlobj_core.h>

#include <QDebug>
#include <QDir>

#include <filesystem>
#include <string>
#include <stdexcept>

QString operationResultToString(Int32 operationResult)
{
  namespace R = NArchive::NExtract::NOperationResult;

  switch(operationResult)
  {
    case R::kOK:
      return {};

    case R::kUnsupportedMethod:
      return "Encoding method unsupported";

    case R::kDataError:
      return "Data error";

    case R::kCRCError:
      return "CRC error";

    case R::kUnavailable:
      return "Unavailable";

    case R::kUnexpectedEnd:
      return "Unexpected end of archive";

    case R::kDataAfterEnd:
      return "Data after end of archive";

    case R::kIsNotArc:
      return "Not an ARC";

    case R::kHeadersError:
      return "Bad headers";

    case R::kWrongPassword:
      return "Wrong password";

    default:
      return QString("Unknown error %1").arg(operationResult);
  }
}

CArchiveExtractCallback::CArchiveExtractCallback(ProgressCallback *progressCallback,
    FileChangeCallback *fileChangeCallback,
    ErrorCallback *errorCallback,
    PasswordCallback *passwordCallback,
    IInArchive *archiveHandler,
    const QString &directoryPath,
    FileData* const *fileData,
    std::size_t nbFiles,
    UInt64 totalFileSize,
    QString *password)
  : m_ArchiveHandler(archiveHandler)
  , m_Total(0)
  , m_DirectoryPath(QDir::toNativeSeparators(directoryPath).toStdWString())
  , m_Extracting(false)
  , m_Canceled(false)
  , m_Timers{}
  //m_ProcessedFileInfo //not sure how to initialise this!
  , m_OutputFileStream{}
  , m_OutFileStreamCom{}
  , m_FileData(fileData)
  , m_NbFiles(nbFiles)
  , m_TotalFileSize(totalFileSize)
  , m_ExtractedFileSize(0)
  , m_LastCallbackFileSize(0)
  , m_Password(password)
  , m_ProgressCallback(progressCallback)
  , m_FileChangeCallback(fileChangeCallback)
  , m_ErrorCallback(errorCallback)
  , m_PasswordCallback(passwordCallback)
{
}

CArchiveExtractCallback::~CArchiveExtractCallback()
{
  qDebug().nospace().noquote() << QString::fromStdString(m_Timers.GetStream.toString("GetStream"));
  qDebug().nospace().noquote() << QString::fromStdString(m_Timers.SetOperationResult.SetMTime.toString("SetOperationResult.SetMTime"));
  qDebug().nospace().noquote() << QString::fromStdString(m_Timers.SetOperationResult.Close.toString("SetOperationResult.Close"));
  qDebug().nospace().noquote() << QString::fromStdString(m_Timers.SetOperationResult.Release.toString("SetOperationResult.Release"));
  qDebug().nospace().noquote() << QString::fromStdString(m_Timers.SetOperationResult.SetFileAttributesW.toString("SetOperationResult.SetFileAttributesW"));

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
  auto guard = m_Timers.GetStream.instrument();
  namespace fs = std::filesystem;

  *outStream = nullptr;
  m_OutFileStreamCom.Release();

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
        auto fullpath = m_DirectoryPath / filename.toStdWString();
        std::error_code ec;
        std::filesystem::create_directories(fullpath, ec);
        if (ec) {
          reportError(L"cannot created directory '{}': {}", fullpath, ec);
          return E_ABORT;
        }
        m_FullProcessedPaths.push_back(fullpath);
      }
    } else {
      for (const QString &filename : filenames) {
        auto fullProcessedPath = m_DirectoryPath / filename.toStdWString();
        //If the filename contains a '/' we want to make the directory
        auto directoryPath = fullProcessedPath.parent_path();
        if (!fs::exists(directoryPath)) {
          //Make the containing directory
          std::error_code ec;
          std::filesystem::create_directories(directoryPath, ec);
          if (ec) {
            reportError(L"cannot created directory '{}': {}", directoryPath, ec);
            return E_ABORT;
          }
          //m_DirectoryPath.mkpath(filename.left(slashPos));
        }
        //If the file already exists, delete it
        if (fs::exists(fullProcessedPath)) {
          std::error_code ec;
          if (!fs::remove(fullProcessedPath, ec)) {
            reportError(L"cannot delete output file '{}': {}", fullProcessedPath, ec);
            return E_ABORT;
          }
        }
        m_FullProcessedPaths.push_back(fullProcessedPath);
      }

      m_OutputFileStream = new MultiOutputStream();
      
      /* [this](UInt32 size, UInt64 totalSize) {
        m_ExtractedFileSize += size;
        if (m_ProgressCallback != nullptr) {
          float percentage = static_cast<float>(m_ExtractedFileSize) / static_cast<float>(m_TotalFileSize);
          (*m_ProgressCallback)(percentage);
        }
      }); */
      CComPtr<MultiOutputStream> outStreamCom(m_OutputFileStream);

      if (!m_OutputFileStream->Open(m_FullProcessedPaths)) {
        reportError(L"cannot open output file '{}': {}", m_FullProcessedPaths[0].native(), GetLastError());
        return E_ABORT;
      }

      UInt64 fileSize = getProperty<UInt64>(index, kpidSize);
      if (m_OutputFileStream->SetSize(fileSize) != S_OK) {
        qDebug().nospace().noquote() << "SetSize() failed.";
      }

      //This is messy but I can't find another way of doing it. A simple
      //assignment of m_outFileStream to *outStream doesn't increase the
      //reference count.
      m_OutFileStreamCom = outStreamCom;
      *outStream = outStreamCom.Detach();
    }

    /* if (m_FileChangeCallback != nullptr) {
      (*m_FileChangeCallback)(filenames[0]);
    } */

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
    reportError(operationResultToString(operationResult));
  }

  if (m_OutFileStreamCom) {
    if (m_ProcessedFileInfo.MTimeDefined) {
      auto guard = m_Timers.SetOperationResult.SetMTime.instrument();
      m_OutputFileStream->SetMTime(&m_ProcessedFileInfo.MTime);
    }
    auto guard = m_Timers.SetOperationResult.Close.instrument();
    RINOK(m_OutputFileStream->Close())
  }

  {
    auto guard = m_Timers.SetOperationResult.Release.instrument();
    m_OutFileStreamCom.Release();
  }

  auto guard = m_Timers.SetOperationResult.SetFileAttributesW.instrument();
  if (m_Extracting && m_ProcessedFileInfo.AttribDefined) {
    //this is moderately annoying. I can't do this on the file handle because if
    //the file in question is a directory there isn't a file handle.
    //Also I'd like to convert the attributes to QT attributes but I'm not sure
    //if that's possible. Hence the conversions and strange string.
    for (auto &path : m_FullProcessedPaths) {
      std::wstring const fn = L"\\\\?\\" + path.native();
      //If the attributes are POSIX-based, fix that
      if (m_ProcessedFileInfo.Attrib & 0xF0000000)
        m_ProcessedFileInfo.Attrib &= 0x7FFF;

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
