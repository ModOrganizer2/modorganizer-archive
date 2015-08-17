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
#include <map>
#include <string>
#include <sstream>
#include <unordered_map>
#include <vector>

using namespace NWindows;


static const TCHAR DLLName[] = TEXT("dlls/7z.dll");

class FileDataImpl : public FileData {
  friend class Archive;
public:
  FileDataImpl(const std::wstring &fileName, UInt64 crc, bool isDirectory);

  virtual LPCWSTR getFileName() const;
  virtual void addOutputFileName(LPCWSTR fileName);
  virtual std::vector<std::wstring> getAndClearOutputFileNames();
  virtual bool isDirectory() const { return m_IsDirectory; }
  virtual UInt64 getCRC() const;

private:
  std::wstring m_FileName;
  UInt64 m_CRC;
  std::vector<std::wstring> m_OutputFileNames;
  bool m_IsDirectory;
};


LPCWSTR FileDataImpl::getFileName() const
{
  return m_FileName.c_str();
}

void FileDataImpl::addOutputFileName(LPCWSTR fileName)
{
  m_OutputFileNames.push_back(fileName);
}

std::vector<std::wstring> FileDataImpl::getAndClearOutputFileNames()
{
  std::vector<std::wstring> result = m_OutputFileNames;
  m_OutputFileNames.clear();
  return result;
}

UInt64 FileDataImpl::getCRC() const {
  return m_CRC;
}


FileDataImpl::FileDataImpl(const std::wstring &fileName, UInt64 crc, bool isDirectory)
  : m_FileName(fileName)
  , m_CRC(crc)
  , m_IsDirectory(isDirectory)
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

  HRESULT loadFormats();

private:

  CreateObjectType CreateObjectFunc;

  bool m_Valid;
  Error m_LastError;

  NWindows::NDLL::CLibrary *m_Library;
  UString m_ArchiveName;
  CMyComPtr<IInArchive> m_ArchivePtr;
  CArchiveExtractCallback *m_ExtractCallback;

  PasswordCallback *m_PasswordCallback;

  std::vector<FileData*> m_FileList;

  UString m_Password;

  struct ArchiveFormatInfo
  {
    CLSID m_ClassID;
    std::wstring m_Name;
    std::string m_StartSignature;
    std::wstring m_Extensions;
    std::wstring m_AdditionalExtensions;
  };

  typedef std::vector<ArchiveFormatInfo> Formats;
  Formats m_Formats;

  typedef std::unordered_map<std::wstring, Formats> FormatMap;
  FormatMap m_FormatMap;

  //I don't think one signature could possibly describe two formats.
  typedef std::map<std::string, ArchiveFormatInfo> SignatureMap;
  SignatureMap m_SignatureMap;

  std::size_t m_MaxSignatureLen = 0;
};

typedef UInt32 (WINAPI *GetNumberOfFormatsFunc)(UInt32 *numFormats);
typedef UInt32 (WINAPI *GetHandlerPropertyFunc2)(UInt32 index, PROPID propID, PROPVARIANT *value);

static HRESULT ReadProp(
    GetHandlerPropertyFunc2 getProp2,
    UInt32 index, PROPID propID, NCOM::CPropVariant &prop)
{
  return getProp2(index, propID, &prop);
}

static HRESULT ReadBoolProp(
    GetHandlerPropertyFunc2 getProp2,
    UInt32 index, PROPID propID, bool &res)
{
  NCOM::CPropVariant prop;
  RINOK(ReadProp(getProp2, index, propID, prop));
  if (prop.vt == VT_BOOL) {
    res = VARIANT_BOOLToBool(prop.boolVal);
  } else if (prop.vt != VT_EMPTY) {
    return E_FAIL;
  } else {
    res = false;
  }
  return S_OK;
}

static HRESULT ReadStringProp(
    GetHandlerPropertyFunc2 getProp2,
    UInt32 index, PROPID propID, UString &res)
{
  NCOM::CPropVariant prop;
  RINOK(ReadProp(getProp2, index, propID, prop));
  if (prop.vt == VT_BSTR) {
    res = prop.bstrVal;
  } else if (prop.vt != VT_EMPTY) {
    return E_FAIL;
  } else {
    res.Empty();
  }
  return S_OK;
}


//Seriously, there is one format returned in the list that has no registered
//extension and no signature. WTF?
HRESULT ArchiveImpl::loadFormats()
{
  GetHandlerPropertyFunc2 getProp2 = (GetHandlerPropertyFunc2)m_Library->GetProc("GetHandlerProperty2");
  if (getProp2 == nullptr) {
    return S_OK;
  }

  UInt32 numFormats = 1;
  GetNumberOfFormatsFunc getNumberOfFormats = (GetNumberOfFormatsFunc)m_Library->GetProc("GetNumberOfFormats");
  if (getNumberOfFormats != nullptr) {
    RINOK(getNumberOfFormats(&numFormats));
  }

  for (UInt32 i = 0; i < numFormats; ++i)
  {
    ArchiveFormatInfo item;

    UString name;
    RINOK(ReadStringProp(getProp2, i, NArchive::kName, name));
    item.m_Name.assign(static_cast<wchar_t const *>(name), name.Length());

    //Should split up the extensions and map extension to type, and see what we get from that for preference
    //then try all extensions anyway...
    NCOM::CPropVariant prop;
    if (ReadProp(getProp2, i, NArchive::kClassID, prop) != S_OK) {
      return E_FAIL;
    }
    if (prop.vt != VT_BSTR) {
      continue; //only if empty? return E_FAIL;
    }
    item.m_ClassID = *(const GUID *)prop.bstrVal;

    UString ext;
    RINOK(ReadStringProp(getProp2, i, NArchive::kExtension, ext));
    item.m_Extensions.assign(static_cast<wchar_t const *>(ext), ext.Length());

    //This is unnecessary currently for our purposes. Basically, for each
    //extension, there's an 'addext' which, if set (to other than *) means that
    //theres a double encoding going on. For instance, the bzip format is like this
    //addext = "* * .tar .tar"
    //ext    = "bz2 bzip2 tbz2 tbz"
    //which means that tbz2 and tbz should uncompress to a tar file which can be
    //further processed as if it were a tar file. Having said which, we don't
    //need to support this at all, so I'm storing it but ignoring it.
    UString addext;
    RINOK(ReadStringProp(getProp2, i, NArchive::kAddExtension, addext));
    item.m_AdditionalExtensions.assign(static_cast<wchar_t const *>(addext), addext.Length());

    prop.Clear();
    if (ReadProp(getProp2, i, NArchive::kStartSignature, prop) == S_OK) {
      if (prop.vt == VT_BSTR) {
        //If he can do a memmove, i can do a reinterpret_cast.
        //This appears to be abusing the interface and storing a normal string in
        //something claiming to be a wide string.
        std::string signature(reinterpret_cast<char const *>(prop.bstrVal),
                              ::SysStringByteLen(prop.bstrVal));

        if (! signature.empty()) {
          item.m_StartSignature = signature;
          if (m_MaxSignatureLen < signature.size()) {
            m_MaxSignatureLen = signature.size();
          }
          m_SignatureMap[signature] = item;
        }
      }
      else {
        continue; //onlif empty? return E_FAIL;
      }

    }

    //Now split the extension up from the space separated string and create
    //a map from each extension to the possible formats
    //We could make these pointers but it's not a massive overhead and nobody
    //should be changing this
    std::wistringstream s(item.m_Extensions);
    std::wstring t;
    std::vector<std::wstring> exts;
    while (s >> t) {
      m_FormatMap[t].push_back(item);
    }
    m_Formats.push_back(item);
  }
  return S_OK;
}


ArchiveImpl::ArchiveImpl()
  : m_Valid(false)
  , m_LastError(ERROR_NONE)
  , m_Library(new NWindows::NDLL::CLibrary)
  , m_PasswordCallback(nullptr)
{
  if (!m_Library->Load(DLLName)) {
    m_LastError = ERROR_LIBRARY_NOT_FOUND;
    return;
  }

  CreateObjectFunc = (CreateObjectType)m_Library->GetProc("CreateObject");
  if (CreateObjectFunc == nullptr) {
    m_LastError = ERROR_LIBRARY_INVALID;
    return;
  }

  if (loadFormats() != S_OK) {
    m_LastError = ERROR_LIBRARY_INVALID;
    return;
  }

  m_Valid = true;
}

ArchiveImpl::~ArchiveImpl()
{
  clearFileList();
  m_ArchivePtr.Release();
  delete m_PasswordCallback;
  delete m_Library;
}


std::wstring GetArchiveItemPath(IInArchive *archive, UInt32 index)
{
  NWindows::NCOM::CPropVariant prop;
  if (archive->GetProperty(index, kpidPath, &prop) != S_OK) {
    throw std::runtime_error("failed to retrieve kpidPath");
  }
  if(prop.vt == VT_BSTR) {
    return prop.bstrVal;
  } else if (prop.vt == VT_EMPTY) {
    return std::wstring();
  } else {
    throw std::runtime_error("failed to retrieve item path");
  }
}

bool ArchiveImpl::open(LPCTSTR archiveName, PasswordCallback *passwordCallback)
{
  m_LastError = ERROR_NONE;
  // in rars the password seems to be requested during extraction, not on open, so we need to hold on
  // to the callback for now
  m_PasswordCallback = passwordCallback;

  UString normalizedName = archiveName;
  normalizedName.Trim();
  int fileNamePartStartIndex;
  NFile::NDirectory::MyGetFullPathName(normalizedName, m_ArchiveName, fileNamePartStartIndex);

  NFile::NFind::CFileInfoW fileInfo;
  if (!fileInfo.Find(m_ArchiveName)) {
    m_ArchiveName = L"";
    m_LastError = ERROR_ARCHIVE_NOT_FOUND;
    return false;
  }

  if (fileInfo.IsDir()) {
    m_LastError = ERROR_ARCHIVE_NOT_FOUND;
    return false;
  }

  CInFileStream *fileSpec = new CInFileStream;
  CMyComPtr<IInStream> file = fileSpec;

  if (!fileSpec->Open(m_ArchiveName)) {
    m_LastError = ERROR_FAILED_TO_OPEN_ARCHIVE;
    return false;
  }

  // actually open the archive
  CArchiveOpenCallback *openCallback = new CArchiveOpenCallback(passwordCallback);

  openCallback->LoadFileInfo(m_ArchiveName.Left(fileNamePartStartIndex),
                             m_ArchiveName.Mid(fileNamePartStartIndex));

  CMyComPtr<IArchiveOpenCallback> openCallbackPtr(openCallback);

  //FIXME Would it be more sensible to try the signature first? Then iterate
  //over the formats

  // determine archive type based on extension
  Formats const *formats = nullptr;
  {
    int extensionPos = m_ArchiveName.ReverseFind(L'.');
    UString extension = m_ArchiveName.Mid(extensionPos + 1);
    extension.MakeLower();
    std::wstring ext(static_cast<wchar_t const *>(extension), extension.Length());
    FormatMap::const_iterator map_iter = m_FormatMap.find(ext);
    if (map_iter != m_FormatMap.end()) {
      formats = &map_iter->second;
    }
  }

  //OK, we have some potential formats. If there is only one, try it now. If
  //there are multiple formats, we'll try by signature lookup first.
  bool found = false;
  if (formats != nullptr && formats->size() == 1) {
    if (CreateObjectFunc(&formats->front().m_ClassID, &IID_IInArchive, (void**)&m_ArchivePtr) != S_OK) {
      m_LastError = ERROR_LIBRARY_ERROR;
      return false;
    }

    if (m_ArchivePtr->Open(file, 0, openCallbackPtr) == S_OK) {
      found = true;
    }
  }

  if (!found) {
    //Read the signature of the file and look that up.
    std::vector<char> buff;
    buff.reserve(m_MaxSignatureLen);
    UInt32 act;
    file->Seek(0, STREAM_SEEK_SET, nullptr);
    file->Read(buff.data(), m_MaxSignatureLen, &act);
    file->Seek(0, STREAM_SEEK_SET, nullptr);
    std::string signature = std::string(buff.data(), act);
    //Get the first iterator that is strictly > the signature we're looking for.
    //Note: This assumes there is at least one signature to look at!
    SignatureMap::const_iterator fmt = m_SignatureMap.upper_bound(signature);
    --fmt;
    //this must be <= to our key. Again, given we have unique signatures in here,
    //there shouldn't be any issue with spuriously matching.
    if (fmt->first == std::string(buff.data(), fmt->first.size())) {
      if (CreateObjectFunc(&fmt->second.m_ClassID, &IID_IInArchive, (void**)&m_ArchivePtr) != S_OK) {
        m_LastError = ERROR_LIBRARY_ERROR;
        return false;
      }

      if (m_ArchivePtr->Open(file, 0, openCallbackPtr) == S_OK) {
        //this is where I really want to output stuff saying what I've done.
        found = true;
      }
      //Arguably we should give up here if it's not OK if 7zip can't even start
      //to decode even though we've found the format from the signature.
      //Sadly, the 7zip API documentation is pretty well non-existant.
    }
    //If we get here, we have a file which doesn't have an identifiable
    //signature and doesn't have a unique extension. We *could* iterate over
    //all the formats and try them, but that seems excessive. In any case, the
    //7-zip documentation doesn't document informatio I used to get the formats.
  }

  if (!found) {
    m_LastError = ERROR_INVALID_ARCHIVE_FORMAT;
    return false;
  }

  m_Password = openCallback->GetPassword();
/*
  UInt32 subFile = ULONG_MAX;
  {
    NCOM::CPropVariant prop;
    if (m_ArchivePtr->GetArchiveProperty(kpidMainSubfile, &prop) != S_OK) {
      throw std::runtime_error("failed to get property kpidMainSubfile");
    }

    if (prop.vt == VT_UI4) {
      subFile = prop.ulVal;
    }
  }

  if (subFile != ULONG_MAX) {
    std::wstring subPath = GetArchiveItemPath(m_ArchivePtr, subFile);

    CMyComPtr<IArchiveOpenSetSubArchiveName> setSubArchiveName;
    openCallbackPtr.QueryInterface(IID_IArchiveOpenSetSubArchiveName, (void **)&setSubArchiveName);
    if (setSubArchiveName) {
      setSubArchiveName->SetSubArchiveName(subPath.c_str());
    }
  }*/

  resetFileList();
  return true;
}


bool ArchiveImpl::close()
{
  if (m_ArchivePtr != nullptr) {
    return m_ArchivePtr->Close() == S_OK;
  } else {
    return true;
  }
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
    m_ArchivePtr->GetProperty(i, kpidCRC, &prop);
    UInt64 crc = 0LL;
    if (prop.vt != VT_EMPTY) {
      crc = ConvertPropVariantToUInt64(prop);
    }
    bool isDirectory = false;
    IsArchiveItemProp(m_ArchivePtr, i, kpidIsDir, isDirectory);
    m_FileList.push_back(new FileDataImpl(std::wstring(fileName), crc, isDirectory));
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
  m_ExtractCallback = new CArchiveExtractCallback(progressCallback, fileChangeCallback, errorCallback, m_PasswordCallback);
  m_ExtractCallback->Init(m_ArchivePtr, GetUnicodeString(outputDirectory), &m_FileList[0], &m_Password);
  HRESULT result = m_ArchivePtr->Extract(nullptr, (UInt32)(Int32)(-1), false, m_ExtractCallback);
  switch (result) {
    case S_OK: {
      //nop
    } break;
    case E_ABORT: {
      m_LastError = ERROR_EXTRACT_CANCELLED;
    } break;
    case E_OUTOFMEMORY: {
      m_LastError = ERROR_OUT_OF_MEMORY;
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

