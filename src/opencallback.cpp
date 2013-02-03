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

#include "opencallback.h"
#include "Common/StringConvert.h"
#include "Windows/FileDir.h"
#include "Windows/FileFind.h"
#include "Windows/PropVariant.h"
#include "Windows/PropVariantConversions.h"
#include <string>


#define UNUSED(x)


STDMETHODIMP CArchiveOpenCallback::SetTotal(const UInt64 *UNUSED(files), const UInt64 *UNUSED(bytes))
{
  return S_OK;
}

STDMETHODIMP CArchiveOpenCallback::SetCompleted(const UInt64 *UNUSED(files), const UInt64 *UNUSED(bytes))
{
  return S_OK;
}

STDMETHODIMP CArchiveOpenCallback::CryptoGetTextPassword(BSTR* passwordOut)
{
  if (m_PasswordCallback != NULL) {
    char* passwordBuffer = new char[MAX_PASSWORD_LENGTH + 1];
    memset(passwordBuffer, '\0', MAX_PASSWORD_LENGTH + 1);
    (*m_PasswordCallback)(passwordBuffer);
    m_Password = GetUnicodeString(passwordBuffer);
    HRESULT res = StringToBstr(m_Password, passwordOut);
    delete [] passwordBuffer;
    return res;
  } else {
    // report error here!
    return E_ABORT;
  }
}
