/*
Copyright (C) 2012 Sebastian Herbord. All rights reserved.

This file is part of Mod Organizer.

Mod Organizer is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Mod Organizer is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with Mod Organizer.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "archive.h"

#include "Common/MyCom.h"
#include "Common/IntToString.h"
#include "Common/MyInitGuid.h"
#include "Common/StringConvert.h"

#include "Windows/DLL.h"
#include "Windows/FileDir.h"
#include "Windows/FileFind.h"
#include "Windows/FileName.h"
#include "Windows/NtCheck.h"
#include "Windows/PropVariant.h"
#include "Windows/PropVariantConversions.h"

#include "7zip/Common/FileStreams.h"
#include "7zip/MyVersion.h"

#include "7zip/Archive/IArchive.h"
#include "7zip/IPassword.h"

#include "extractcallback.h"
#include "opencallback.h"

#include <algorithm>
#include <string>
#include <vector>


using namespace NWindows;


static const TCHAR DLLName[] = TEXT("dlls/7z.dll");



DEFINE_GUID(CLSID_CFormatZip,
  0x23170F69, 0x40C1, 0x278A, 0x10, 0x00, 0x00, 0x01, 0x10, 0x01, 0x00, 0x00);
DEFINE_GUID(CLSID_CFormatRar,
  0x23170F69, 0x40C1, 0x278A, 0x10, 0x00, 0x00, 0x01, 0x10, 0x03, 0x00, 0x00);
DEFINE_GUID(CLSID_CFormat7z,
  0x23170F69, 0x40C1, 0x278A, 0x10, 0x00, 0x00, 0x01, 0x10, 0x07, 0x00, 0x00);



class FileDataImpl : public FileData {
  friend class Archive;
public:
  FileDataImpl(const std::wstring &fileName, bool isDirectory);

  virtual LPCWSTR getFileName() const;
  virtual void setSkip(bool skip) { m_Skip = skip; }
  virtual bool getSkip() const { return m_Skip; }
  virtual void setOutputFileName(LPCWSTR fileName);
  virtual LPCWSTR getOutputFileName() const;
  virtual bool isDirectory() const { return m_IsDirectory; }

private:
  std::wstring m_FileName;
  std::wstring m_OutputFileName;
  bool m_Skip;
  bool m_IsDirectory;
};


LPCWSTR FileDataImpl::getFileName() const
{
  return m_FileName.c_str();
}


void FileDataImpl::setOutputFileName(LPCWSTR fileName)
{
  m_OutputFileName = fileName;
}

LPCWSTR FileDataImpl::getOutputFileName() const
{
  return m_OutputFileName.c_str();
}


FileDataImpl::FileDataImpl(const std::wstring& fileName, bool isDirectory)
  : m_FileName(fileName), m_OutputFileName(fileName), m_Skip(false), m_IsDirectory(isDirectory)
{
}


/// represents the connection to one archive and provides common functionality
class ArchiveImpl : public Archive {

public:

  ArchiveImpl();
  virtual ~ArchiveImpl();

  virtual bool isValid() const { return m_Valid; }

  virtual Error getLastError() const { return m_LastError; }

  virtual bool open(LPCTSTR archiveName, PasswordCallback *passwordCallback);
  virtual bool close();
  virtual bool getFileList(FileData* const *&data, size_t &size);
  virtual bool extract(LPCTSTR outputDirectory, ProgressCallback *progressCallback,
                       FileChangeCallback* fileChangeCallback, ErrorCallback* errorCallback);

  virtual void cancel();

  void operator delete(void* ptr) {
    ::operator delete(ptr);
  }

private:

  typedef UINT32 (WINAPI *CreateObjectType)(
      const GUID *clsID,
      const GUID *interfaceID,
      void **outObject);

private:

  virtual void destroy() {
    delete this;
  }

  void clearFileList();
  void resetFileList();

private:

  CreateObjectType CreateObjectFunc;

  bool m_Valid;
  Error m_LastError;

  NWindows::NDLL::CLibrary *m_Library;
  UString m_ArchiveName;
  CMyComPtr<IInArchive> m_ArchivePtr;
//  CMyComPtr<IArchiveExtractCallback> m_ExtractCallback;
  CArchiveExtractCallback *m_ExtractCallback;

  std::vector<FileData*> m_FileList;

  UString m_Password;

};


ArchiveImpl::ArchiveImpl()
  : m_Valid(true), m_LastError(ERROR_NONE), m_Library(new NWindows::NDLL::CLibrary)
{
  if (!m_Library->Load(DLLName)) {
    m_LastError = ERROR_LIBRARY_NOT_FOUND;
    m_Valid = false;
  }

  CreateObjectFunc = (CreateObjectType)m_Library->GetProc("CreateObject");
  if (CreateObjectFunc == NULL) {
    m_LastError = ERROR_LIBRARY_INVALID;
    m_Valid = false;
  }
}


ArchiveImpl::~ArchiveImpl()
{
  clearFileList();
  m_ArchivePtr.Release();
  delete m_Library;
}


bool ArchiveImpl::open(LPCTSTR archiveName, PasswordCallback *passwordCallback)
{
  m_LastError = ERROR_NONE;
  m_ArchiveName = GetUnicodeString(archiveName);
  NFile::NFind::CFileInfoW fileInfo;
  if (!fileInfo.Find(m_ArchiveName)) {
    m_ArchiveName = L"";
    m_LastError = ERROR_ARCHIVE_NOT_FOUND;
    return false;
  }

  CInFileStream *fileSpec = new CInFileStream;
  CMyComPtr<IInStream> file = fileSpec;

  if (!fileSpec->Open(m_ArchiveName)) {
    m_LastError = ERROR_FAILED_TO_OPEN_ARCHIVE;
    return false;
  }
  UInt64 size;
  fileSpec->GetSize(&size);

  const GUID *formatIdentifier = NULL;

  // actually open the archive
  CArchiveOpenCallback *openCallback = new CArchiveOpenCallback(passwordCallback);

  CMyComPtr<IArchiveOpenCallback> openCallbackPtr(openCallback);


  // determine archive type based on extension
  int extensionPos = m_ArchiveName.ReverseFind(L'.');
  UString extension = m_ArchiveName.Mid(extensionPos + 1);
  extension.MakeLower();
  if (extension == L"7z") {
    formatIdentifier = &CLSID_CFormat7z;
  } else if (extension == L"zip") {
    formatIdentifier = &CLSID_CFormatZip;
  } else if (extension == L"rar") {
    formatIdentifier = &CLSID_CFormatRar;
  }

  if (formatIdentifier == NULL) {
    // need to try different format identifiers
    const GUID *identifiers[] = { &CLSID_CFormat7z, &CLSID_CFormatZip, &CLSID_CFormatRar, NULL };
    HRESULT res = S_FALSE;
    for (int i = 0; identifiers[i] != NULL && res != S_OK; ++i) {
      if (CreateObjectFunc(identifiers[i], &IID_IInArchive, (void**)&m_ArchivePtr) != S_OK) {
        m_LastError = ERROR_LIBRARY_ERROR;
      }
      res = m_ArchivePtr->Open(file, 0, openCallbackPtr);
      if (res != S_OK) {
        m_ArchivePtr.Release();
      }
    }
    if (res != S_OK) {
      m_LastError = ERROR_INVALID_ARCHIVE_FORMAT;
      return false;
    }
  } else {
    if (CreateObjectFunc(formatIdentifier, &IID_IInArchive, (void**)&m_ArchivePtr) != S_OK) {
      m_LastError = ERROR_LIBRARY_ERROR;
      return false;
    }

    if (m_ArchivePtr->Open(file, 0, openCallbackPtr) != S_OK) {
      m_LastError = ERROR_ARCHIVE_INVALID;
      return false;
    }
  }

  m_Password = openCallback->GetPassword();

  resetFileList();
  return true;
}


bool ArchiveImpl::close()
{
  return m_ArchivePtr->Close() == S_OK;
}


void ArchiveImpl::clearFileList()
{
  for (std::vector<FileData*>::iterator iter = m_FileList.begin(); iter != m_FileList.end(); ++iter) {
    delete *iter;
  }
  m_FileList.clear();
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


void ArchiveImpl::resetFileList()
{
  UInt32 numItems = 0;
  clearFileList();

  m_ArchivePtr->GetNumberOfItems(&numItems);

  for (UInt32 i = 0; i < numItems; ++i) {
    NWindows::NCOM::CPropVariant prop;
    m_ArchivePtr->GetProperty(i, kpidPath, &prop);
    UString fileName = ConvertPropVariantToString(prop);
    bool isDirectory = false;
    IsArchiveItemProp(m_ArchivePtr, i, kpidIsDir, isDirectory);
    m_FileList.push_back(new FileDataImpl(std::wstring(fileName), isDirectory));
  }
}


bool ArchiveImpl::getFileList(FileData* const *&data, size_t &size)
{
  data = &m_FileList[0];
  size = m_FileList.size();
  return true;
}



bool ArchiveImpl::extract(LPCTSTR outputDirectory, ProgressCallback* progressCallback,
                          FileChangeCallback* fileChangeCallback, ErrorCallback* errorCallback)
{
  m_ExtractCallback = new CArchiveExtractCallback(progressCallback,fileChangeCallback, errorCallback);
  CMyComPtr<IArchiveExtractCallback> extractCallback = m_ExtractCallback;
  m_ExtractCallback->Init(m_ArchivePtr, GetUnicodeString(outputDirectory), &m_FileList[0], m_Password);
  HRESULT result = m_ArchivePtr->Extract(NULL, (UInt32)(Int32)(-1), false, extractCallback);
  switch (result) {
    case S_OK: {
      //nop
    } break;
    case E_ABORT: {
      m_LastError = ERROR_EXTRACT_CANCELLED;
    } break;
    default: {
      m_LastError = ERROR_LIBRARY_ERROR;
    } break;
  }

  return result == S_OK;
}


void ArchiveImpl::cancel()
{
  m_ExtractCallback->SetCanceled(true);
}


extern "C" DLLEXPORT Archive *CreateArchive()
{
  return new ArchiveImpl;
}

