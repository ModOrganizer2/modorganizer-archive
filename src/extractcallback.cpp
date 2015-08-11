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


#include "extractcallback.h"

#include "Windows/FileDir.h"
#include "Windows/FileFind.h"
#include "Windows/PropVariant.h"
#include "Windows/PropVariantConversions.h"
#include <string>
#include "archive.h"


using namespace NWindows;


void CArchiveExtractCallback::Init(IInArchive *archiveHandler, const UString &directoryPath,
                                   FileData* const *fileData, const UString &password)
{
  m_ArchiveHandler = archiveHandler;
  m_DirectoryPath = directoryPath;
  m_FileData = fileData;
  m_Password = password;
  m_Canceled = false;
  NFile::NName::NormalizeDirPathPrefix(m_DirectoryPath);
}

STDMETHODIMP CArchiveExtractCallback::SetTotal(UInt64 size)
{
  m_Total = size;
  return S_OK;
}

STDMETHODIMP CArchiveExtractCallback::SetCompleted(const UInt64 *value)
{
  m_Completed = *value;
  float percentage = static_cast<float>(m_Completed) / static_cast<float>(m_Total);
  (*m_ProgressCallback)(percentage);

  return m_Canceled ? E_ABORT : S_OK;
}


static HRESULT IsArchiveItemProp(IInArchive *archive, UInt32 index, PROPID propID, bool &result)
{
  NCOM::CPropVariant prop;
  RINOK(archive->GetProperty(index, propID, &prop));
  if (prop.vt == VT_BOOL)
    result = VARIANT_BOOLToBool(prop.boolVal);
  else if (prop.vt == VT_EMPTY)
    result = false;
  else
    return E_FAIL;
  return S_OK;
}

static HRESULT IsArchiveItemFolder(IInArchive *archive, UInt32 index, bool &result)
{
  return IsArchiveItemProp(archive, index, kpidIsDir, result);
}

static const wchar_t *kEmptyFileAlias = L"[Content]";


STDMETHODIMP CArchiveExtractCallback::GetStream(UInt32 index, ISequentialOutStream **outStream, Int32 askExtractMode)
{
  *outStream = 0;
  m_OutFileStream.Release();

  if (askExtractMode != NArchive::NExtract::NAskMode::kExtract) {
    return S_OK;
  }

  std::vector<std::wstring> filenames = m_FileData[index]->getAndClearOutputFileNames();
  if (filenames.empty()) {
    return S_OK;
  }

  // Get Attrib
  {
    NCOM::CPropVariant prop;
    RINOK(m_ArchiveHandler->GetProperty(index, kpidAttrib, &prop));
    if (prop.vt == VT_EMPTY) {
      m_ProcessedFileInfo.Attrib = 0;
      m_ProcessedFileInfo.AttribDefined = false;
    } else {
      if (prop.vt != VT_UI4) {
        return E_FAIL;
      }
      m_ProcessedFileInfo.Attrib = prop.ulVal;
      m_ProcessedFileInfo.AttribDefined = true;
    }
  }

  RINOK(IsArchiveItemFolder(m_ArchiveHandler, index, m_ProcessedFileInfo.isDir));

  // Get Modified Time
  {
    NCOM::CPropVariant prop;
    RINOK(m_ArchiveHandler->GetProperty(index, kpidMTime, &prop));
    m_ProcessedFileInfo.MTimeDefined = false;
    switch(prop.vt)
    {
      case VT_EMPTY:
        // _processedFileInfo.MTime = _utcMTimeDefault;
        break;
      case VT_FILETIME:
        m_ProcessedFileInfo.MTime = prop.filetime;
        m_ProcessedFileInfo.MTimeDefined = true;
        break;
      default:
        return E_FAIL;
    }
  }

  // Get Size - why? we don't do anything with it...
  {
    NCOM::CPropVariant prop;
    RINOK(m_ArchiveHandler->GetProperty(index, kpidSize, &prop));
    bool newFileSizeDefined = (prop.vt != VT_EMPTY);
    UInt64 newFileSize;
    if (newFileSizeDefined) {
      newFileSize = ConvertPropVariantToUInt64(prop);
    }
  }

  //Kludge for callbacks
  UString filePath = filenames[0].c_str();

  if (m_ProcessedFileInfo.isDir) {
    for (const std::wstring &filename : filenames) {
      // TODO: error handling
      NFile::NDirectory::CreateComplexDirectory(m_DirectoryPath + filename.c_str());
    }
  } else {
    // Create folders for file and delete old file
    std::vector<UString> fullProcessedPaths;
    for (const std::wstring &filename : filenames) {
      const UString &filenameU = filename.c_str();
      int slashPos = filenameU.ReverseFind(WCHAR_PATH_SEPARATOR);
      if (slashPos >= 0) {
        NFile::NDirectory::CreateComplexDirectory(m_DirectoryPath + filenameU.Left(slashPos));
      }
      UString fullProcessedPath = m_DirectoryPath + filenameU;
      NFile::NFind::CFileInfoW fileInfo;
      if (fileInfo.Find(fullProcessedPath)) {
        if (!NFile::NDirectory::DeleteFileAlways(fullProcessedPath)) {
          reportError(UString(L"can't delete output file ") + fullProcessedPath);
          return E_ABORT;
        }
      }
      fullProcessedPaths.push_back(fullProcessedPath);
    }

    //This is a kludge for the callbacks
    m_DiskFilePath =  m_DirectoryPath + filePath;

    m_OutFileStreamSpec = new MultiOutputStream;
    CMyComPtr<ISequentialOutStream> outStreamLoc(m_OutFileStreamSpec);
    if (!m_OutFileStreamSpec->Open(fullProcessedPaths, CREATE_ALWAYS)) {
      reportError(UString(L"can not open output file ") + m_DiskFilePath);
      return E_ABORT;
    }
    m_OutFileStream = outStreamLoc;
    *outStream = outStreamLoc.Detach();
  }

  if (m_FileChangeCallback != nullptr) {
    (*m_FileChangeCallback)(filePath);
  }

  return S_OK;
}

STDMETHODIMP CArchiveExtractCallback::PrepareOperation(Int32 askExtractMode)
{
  if (m_Canceled) {
    return E_ABORT;
  }
  m_ExtractMode = askExtractMode == NArchive::NExtract::NAskMode::kExtract;

  return S_OK;
}

STDMETHODIMP CArchiveExtractCallback::SetOperationResult(Int32 operationResult)
{
  if (operationResult != NArchive::NExtract::NOperationResult::kOK) {
    switch(operationResult) {
      case NArchive::NExtract::NOperationResult::kUnSupportedMethod: {
        reportError(L"encoding method unsupported");
      } break;
      case NArchive::NExtract::NOperationResult::kCRCError: {
        reportError(L"CRC error");
      } break;
      case NArchive::NExtract::NOperationResult::kDataError: {
        reportError(L"Data error");
      } break;
      default: {
        reportError(L"Unknown error");
      } break;
    }
  }

  if (m_OutFileStream != nullptr) {
    if (m_ProcessedFileInfo.MTimeDefined)
      m_OutFileStreamSpec->SetMTime(&m_ProcessedFileInfo.MTime);
    RINOK(m_OutFileStreamSpec->Close());
  }

  m_OutFileStream.Release();
  if (m_ExtractMode && m_ProcessedFileInfo.AttribDefined) {
    NFile::NDirectory::MySetFileAttributes(m_DiskFilePath, m_ProcessedFileInfo.Attrib);
  }

  return S_OK;
}


STDMETHODIMP CArchiveExtractCallback::CryptoGetTextPassword(BSTR *passwordOut)
{
  if (m_Password.IsEmpty() && (m_PasswordCallback != nullptr)) {
    char* passwordBuffer = new char[MAX_PASSWORD_LENGTH + 1];
    memset(passwordBuffer, '\0', MAX_PASSWORD_LENGTH + 1);
    (*m_PasswordCallback)(passwordBuffer);
    m_Password = GetUnicodeString(passwordBuffer);
  }

  // reuse the password entered on opening, we don't ask the user twice
  return StringToBstr(m_Password, passwordOut);
}


void CArchiveExtractCallback::SetCanceled(bool aCanceled)
{
  m_Canceled = aCanceled;
}


void CArchiveExtractCallback::reportError(const UString &message)
{
  if (m_ErrorCallback != nullptr) {
    (*m_ErrorCallback)(message);
  }
}
