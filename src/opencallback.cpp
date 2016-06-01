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
#include "opencallback.h"

#include "inputstream.h"
#include "propertyvariant.h"

#include <QDebug>

#include <atlbase.h>

#include <memory>
#include <string>
#include <vector>


#define UNUSED(x)

CArchiveOpenCallback::CArchiveOpenCallback(PasswordCallback *passwordCallback, const QFileInfo &fileinfo)
  : m_PasswordCallback(passwordCallback)
  , m_Path(fileinfo.absoluteDir())
  , m_FileInfo(fileinfo)
  , m_SubArchiveMode(false)

{
  if (!m_FileInfo.exists()) {
    throw std::runtime_error("invalid archive path");
  }
}

/* -------------------- IArchiveOpenCallback -------------------- */
STDMETHODIMP CArchiveOpenCallback::SetTotal(const UInt64 *UNUSED(files), const UInt64 *UNUSED(bytes))
{
  return S_OK;
}

STDMETHODIMP CArchiveOpenCallback::SetCompleted(const UInt64 *UNUSED(files), const UInt64 *UNUSED(bytes))
{
  return S_OK;
}

/* -------------------- ICryptoGetTextPassword -------------------- */
/* This apparently implements ICryptoGetTextPassword but even with a passworded
 * archive it doesn't seem to be called. There is also another API which isn't
 * implemented.
 */
STDMETHODIMP CArchiveOpenCallback::CryptoGetTextPassword(BSTR* passwordOut)
{
  if (m_PasswordCallback == nullptr) {
    return E_ABORT;
  }
  (*m_PasswordCallback)(&m_Password);
  *passwordOut = ::SysAllocString(m_Password.toStdWString().c_str());
  return *passwordOut != 0 ? S_OK : E_OUTOFMEMORY;
}

/* -------------------- IArchiveOpenSetSubArchiveName -------------------- */
/* I don't know what this does or how you call it. */
STDMETHODIMP CArchiveOpenCallback::SetSubArchiveName(const wchar_t *name)
{
  m_SubArchiveMode = true;
  m_SubArchiveName = name;
  return S_OK;
}

/* -------------------- IArchiveOpenVolumeCallback -------------------- */

STDMETHODIMP CArchiveOpenCallback::GetProperty(PROPID propID, PROPVARIANT *value)
{
  //A scan of the source code indicates that the only things that ever call the
  //IArchiveOpenVolumeCallback interface ask for the file name and size.
  PropertyVariant &prop = *static_cast<PropertyVariant*>(value);
  switch (propID) {
    case kpidName: {
      if (m_SubArchiveMode) {
        prop = m_SubArchiveName;
      } else {
        prop = m_FileInfo.fileName().toStdWString();
      }
    } break;

    case kpidSize: {
        prop = static_cast<UInt64>(m_FileInfo.size());
    } break;

    default: qDebug() << "Unexpected property " << propID;
  }
  return S_OK;
}

STDMETHODIMP CArchiveOpenCallback::GetStream(const wchar_t *name, IInStream **inStream)
{
  *inStream = nullptr;

  QString filename = QString::fromWCharArray(name);
  m_FileInfo.setFile(m_Path.filePath(filename));

  if (!m_FileInfo.exists() || m_FileInfo.isDir()) {
    return S_FALSE;
  }

  CComPtr<InputStream> inFile(new InputStream);

  if (!inFile->Open(m_FileInfo.absoluteFilePath())) {
    return ::GetLastError();
  }

  *inStream = inFile.Detach();
  return S_OK;
}
