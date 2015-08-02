#include "multioutputstream.h"


#include "StdAfx.h"

#ifndef _WIN32
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#endif

//Not supported so not including these headers because I'm not quite sure where
//they are
#ifdef SUPPORT_DEVICE_FILE
//#include "7zip/C/Alloc.h"
//#include "7zip/Common/Defs.h"
#endif


static inline HRESULT ConvertBoolToHRESULT(bool result)
{
  #ifdef _WIN32
  if (result)
    return S_OK;
  DWORD lastError = ::GetLastError();
  if (lastError == 0)
    return E_FAIL;
  return HRESULT_FROM_WIN32(lastError);
  #else
  return result ? S_OK: E_FAIL;
  #endif
}

//////////////////////////
// MultiOutputStream

HRESULT MultiOutputStream::Close()
{
  DWORD lastError;
  bool ok = true;
  for (OutFile &f : m_Handles) {
    if (!f.Close()) {
      lastError = ::GetLastError();
      ok = false;
    }
  }
  return ok ? S_OK :
         lastError == 0 ? E_FAIL : HRESULT_FROM_WIN32(lastError);
}

bool MultiOutputStream::Create(const std::vector<UString> &fileNames, bool createAlways)
{
  ProcessedSize = 0;
  bool ok = true;
  m_Handles.resize(fileNames.size());
  for (std::size_t f = 0; f != fileNames.size(); ++f) {
    if (!m_Handles[f].Create(fileNames[f], createAlways)) {
      ok = false;
    }
  }
  return ok;
}

bool MultiOutputStream::Open(std::vector<UString> const &fileNames, DWORD creationDisposition)
{
  ProcessedSize = 0;
  bool ok = true;
  m_Handles.resize(fileNames.size());
  for (std::size_t f = 0; f != fileNames.size(); ++f) {
    if (!m_Handles[f].Open(fileNames[f], creationDisposition)) {
      ok = false;
    }
  }
  return ok;
}

STDMETHODIMP MultiOutputStream::Write(const void *data, UInt32 size, UInt32 *processedSize)
{
  bool update_processed(true);
  for (OutFile &f : m_Handles) {

  #ifdef USE_WIN_FILE

    UInt32 realProcessedSize;
    bool result = f.WritePart(data, size, realProcessedSize);
    if (update_processed) {
      ProcessedSize += realProcessedSize;
      update_processed = false;
    }
    if (processedSize != NULL) {
      *processedSize = realProcessedSize;
    }
    if (!result) {
      return ConvertBoolToHRESULT(result);
    }
  #else
    if (processedSize != NULL) {
      *processedSize = 0;
    }
    ssize_t res = f.Write(data, (size_t)size);
    if (res == -1) {
      return E_FAIL;
    }
    if (processedSize != NULL) {
      *processedSize = (UInt32)res;
    }
    ProcessedSize += res;
  #endif
  }
  return S_OK;
}

STDMETHODIMP MultiOutputStream::Seek(Int64 offset, UInt32 seekOrigin, UInt64 *newPosition)
{
  if (seekOrigin >= 3)
    return STG_E_INVALIDFUNCTION;
  /*
  #ifdef USE_WIN_FILE

  UInt64 realNewPosition;
  bool result = File.Seek(offset, seekOrigin, realNewPosition);
  if (newPosition != NULL)
    *newPosition = realNewPosition;
  return ConvertBoolToHRESULT(result);

  #else

  off_t res = File.Seek(offset, seekOrigin);
  if (res == -1)
    return E_FAIL;
  if (newPosition != NULL)
    *newPosition = (UInt64)res;
  return S_OK;

  #endif
  */
}

STDMETHODIMP MultiOutputStream::SetSize(UInt64 newSize)
{
  int one = 2;
  /*
  #ifdef USE_WIN_FILE
  UInt64 currentPos;
  if (!File.Seek(0, FILE_CURRENT, currentPos))
    return E_FAIL;
  bool result = File.SetLength(newSize);
  UInt64 currentPos2;
  result = result && File.Seek(currentPos, currentPos2);
  return result ? S_OK : E_FAIL;
  #else
  return E_FAIL;
  #endif
  */
  return S_OK;
}

bool MultiOutputStream::SetTime(const FILETIME *cTime, const FILETIME *aTime, const FILETIME *mTime)
{
  for (OutFile &f : m_Handles) {
    if (!f.SetTime(cTime, aTime, mTime)) {
      return false;
    }
  }
  return true;
}

bool MultiOutputStream::SetMTime(const FILETIME *mTime)
{
  for (OutFile &f : m_Handles) {
    if (!f.SetMTime(mTime)) {
      return false;
    }
  }
  return true;
}
