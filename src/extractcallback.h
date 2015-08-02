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


#ifndef EXTRACTCALLBACK_H
#define EXTRACTCALLBACK_H

#include "multioutputstream.h"

#include "Common/StringConvert.h"

#include "7zip/Archive/IArchive.h"
#include "7zip/IPassword.h"

#include "7zip/Common/FileStreams.h"
#include "callback.h"


class FileData;

class CArchiveExtractCallback: public IArchiveExtractCallback,
                               public ICryptoGetTextPassword,
                               public CMyUnknownImp
{
public:

  CArchiveExtractCallback(ProgressCallback *progressCallback,
                          FileChangeCallback *fileChangeCallback,
                          ErrorCallback *errorCallback,
                          PasswordCallback *passwordCallback)
    : m_Canceled(false)
    , m_FileData(nullptr)
    , m_ProgressCallback(progressCallback)
    , m_FileChangeCallback(fileChangeCallback)
    , m_ErrorCallback(errorCallback)
    , m_PasswordCallback(passwordCallback)
  {}

  virtual ~CArchiveExtractCallback() { delete m_ProgressCallback; delete m_FileChangeCallback; delete m_ErrorCallback; }

  MY_UNKNOWN_IMP1(ICryptoGetTextPassword)

  // IProgress
  STDMETHOD(SetTotal)(UInt64 size);
  STDMETHOD(SetCompleted)(const UInt64 *completeValue);

  // IArchiveExtractCallback
  STDMETHOD(GetStream)(UInt32 index, ISequentialOutStream **outStream, Int32 askExtractMode);
  STDMETHOD(PrepareOperation)(Int32 askExtractMode);
  STDMETHOD(SetOperationResult)(Int32 resultEOperationResult);

  // ICryptoGetTextPassword
  STDMETHOD(CryptoGetTextPassword)(BSTR *aPassword);

  void SetCanceled(bool aCanceled);

public:

  void Init(IInArchive *archiveHandler, const UString &directoryPath, FileData* const *fileData, const UString &password);

private:

  void reportError(const UString &message);

private:

  CMyComPtr<IInArchive> m_ArchiveHandler;

  unsigned __int64 m_Completed;
  unsigned __int64 m_Total;

  UString m_DirectoryPath;
  UString m_DiskFilePath;
  bool m_ExtractMode;
  bool m_Canceled;

  struct CProcessedFileInfo {
    FILETIME MTime;
    UInt32 Attrib;
    bool isDir;
    bool AttribDefined;
    bool MTimeDefined;
  } m_ProcessedFileInfo;

  MultiOutputStream *m_OutFileStreamSpec;
  CMyComPtr<ISequentialOutStream> m_OutFileStream;

  FileData* const *m_FileData;
  UString m_Password;

  ProgressCallback *m_ProgressCallback;
  FileChangeCallback *m_FileChangeCallback;
  ErrorCallback *m_ErrorCallback;
  PasswordCallback *m_PasswordCallback;

};


#endif // EXTRACTCALLBACK_H
