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
#include "callback.h"


class CArchiveOpenCallback: public IArchiveOpenCallback,
                            public ICryptoGetTextPassword,
                            public CMyUnknownImp
{

public:

  CArchiveOpenCallback(PasswordCallback* passwordCallback)
    : m_PasswordCallback(passwordCallback) {}

  ~CArchiveOpenCallback() { delete m_PasswordCallback; }

  MY_UNKNOWN_IMP1(ICryptoGetTextPassword)

  STDMETHOD(SetTotal)(const UInt64 *files, const UInt64 *bytes);
  STDMETHOD(SetCompleted)(const UInt64 *files, const UInt64 *bytes);

  STDMETHOD(CryptoGetTextPassword)(BSTR *password);

  UString GetPassword() const { return m_Password; }

private:

  PasswordCallback* m_PasswordCallback;
  UString m_Password;

};




#endif // OPENCALLBACK_H
