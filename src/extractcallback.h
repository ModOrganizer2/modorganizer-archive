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

#include "callback.h"
#include "multioutputstream.h"
#include "unknown_impl.h"

#include "7zip/Archive/IArchive.h"
#include "7zip/IPassword.h"

#include <QDir>
class QString;

#include <atlbase.h>

class FileData;

class CArchiveExtractCallback: public IArchiveExtractCallback,
                               public ICryptoGetTextPassword
{

  //A note: It appears that the IArchiveExtractCallback interface includes the
  //IProgress interface, swo we need to respond to it
  UNKNOWN_3_INTERFACE(IArchiveExtractCallback,
                      ICryptoGetTextPassword,
                      IProgress);

public:

  CArchiveExtractCallback(ProgressCallback *progressCallback,
                          FileChangeCallback *fileChangeCallback,
                          ErrorCallback *errorCallback,
                          PasswordCallback *passwordCallback,
                          IInArchive *archiveHandler,
                          const QString &directoryPath,
                          FileData * const *fileData,
                          QString *password);

  virtual ~CArchiveExtractCallback();

  void SetCanceled(bool aCanceled);

  INTERFACE_IArchiveExtractCallback(;)

  // ICryptoGetTextPassword
  STDMETHOD(CryptoGetTextPassword)(BSTR *aPassword);

private:

  void reportError(const QString &message);

  template <typename T> bool getOptionalProperty(UInt32 index, int property, T *result) const;
  template <typename T> T getProperty(UInt32 index, int property) const;

private:

  CComPtr<IInArchive> m_ArchiveHandler;

  UInt64 m_Total;

  QDir m_DirectoryPath;
  bool m_Extracting;
  bool m_Canceled;

  struct CProcessedFileInfo {
    FILETIME MTime;
    UInt32 Attrib;
    bool isDir;
    bool AttribDefined;
    bool MTimeDefined;
  } m_ProcessedFileInfo;

  CComPtr<MultiOutputStream> m_OutFileStream;

  std::vector<QString> m_FullProcessedPaths;

  FileData* const *m_FileData;
  QString *m_Password;

  ProgressCallback *m_ProgressCallback;
  FileChangeCallback *m_FileChangeCallback;
  ErrorCallback *m_ErrorCallback;
  PasswordCallback *m_PasswordCallback;

};


#endif // EXTRACTCALLBACK_H
