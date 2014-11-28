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


#include "opencallback.h"
#include "Common/StringConvert.h"
#include "Windows/FileDir.h"
#include "Windows/PropVariant.h"
#include "Windows/PropVariantConversions.h"
#include <string>


#define UNUSED(x)


CArchiveOpenCallback::CArchiveOpenCallback(PasswordCallback *passwordCallback)
  : m_PasswordCallback(passwordCallback)
  , m_SubArchiveMode(false)
{

}

void CArchiveOpenCallback::LoadFileInfo(const UString &path, const UString &fileName)
{
  m_Path = path;
  if (!m_FileInfo.Find(path + fileName)) {
    throw std::runtime_error("invalid archive path");
  }
}


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
  if (m_PasswordCallback != nullptr) {
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

STDMETHODIMP CArchiveOpenCallback::SetSubArchiveName(const wchar_t *name)
{
  m_SubArchiveMode = true;
  m_SubArchiveName = name;
  return  S_OK;
}

STDMETHODIMP CArchiveOpenCallback::GetProperty(PROPID propID, PROPVARIANT *value)
{
  NWindows::NCOM::CPropVariant prop;
  if (propID == kpidIsAnti) {
    prop = false;
    prop.Detach(value);
    return S_OK;
  }

  switch(propID) {
    case kpidPath: prop = m_FileInfo.Name; break;
    case kpidName: {
      if (m_SubArchiveMode) {
        prop = m_SubArchiveName.c_str();
      } else {
        prop = m_FileInfo.Name;
      }
    } break;
    case kpidIsDir: prop = m_FileInfo.IsDir(); break;
    case kpidSize: prop = m_FileInfo.Size; break;
    case kpidAttrib: prop = (UINT32)m_FileInfo.Attrib; break;
    case kpidCTime: prop = m_FileInfo.CTime; break;
    case kpidATime: prop = m_FileInfo.ATime; break;
    case kpidMTime: prop = m_FileInfo.MTime; break;
  }

  prop.Detach(value);
  return S_OK;
}

STDMETHODIMP CArchiveOpenCallback::GetStream(const wchar_t *name, IInStream **inStream)
{
  *inStream = nullptr;

  UString fullPath = m_Path + name;
  if (!m_FileInfo.Find(fullPath)) {
    return S_FALSE;
  }

  if (m_FileInfo.IsDir()) {
    return S_FALSE;
  }

  CInFileStream *inFile = new CInFileStream;
  CMyComPtr<IInStream> inStreamTemp = inFile;
  if (!inFile->Open(fullPath)) {
    return ::GetLastError();
  }
  *inStream = inStreamTemp.Detach();
  return S_OK;
}

