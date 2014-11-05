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


#ifndef OPENCALLBACK_H
#define OPENCALLBACK_H


#include "Common/StringConvert.h"
#include "7zip/Archive/IArchive.h"
#include "7zip/IPassword.h"
#include "7zip/Common/FileStreams.h"
#include "Windows/FileFind.h"
#include "callback.h"
#include <stdexcept>


class CArchiveOpenCallback: public IArchiveOpenCallback,
                            public IArchiveOpenVolumeCallback,
                            public ICryptoGetTextPassword,
                            public IArchiveOpenSetSubArchiveName,
                            public CMyUnknownImp
{

public:

  MY_UNKNOWN_IMP3(
    IArchiveOpenVolumeCallback,
    ICryptoGetTextPassword,
    IArchiveOpenSetSubArchiveName
  )

  CArchiveOpenCallback(PasswordCallback* passwordCallback);

  ~CArchiveOpenCallback() { }

  INTERFACE_IArchiveOpenCallback(;)
  INTERFACE_IArchiveOpenVolumeCallback(;)

  STDMETHOD(CryptoGetTextPassword)(BSTR *password);
  STDMETHOD(SetSubArchiveName)(const wchar_t *name);

  void LoadFileInfo(const UString &path, const UString &fileName);

  UString GetPassword() const { return m_Password; }

private:

  PasswordCallback *m_PasswordCallback;
  UString m_Password;

  UString m_Path;
  UString m_FileName;

  bool m_SubArchiveMode;
  std::wstring m_SubArchiveName;

  NWindows::NFile::NFind::CFileInfoW m_FileInfo;

};




#endif // OPENCALLBACK_H
