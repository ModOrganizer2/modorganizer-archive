#include "multioutputstream.h"

#include "StdAfx.h"

static inline HRESULT ConvertBoolToHRESULT(bool result)
{
  if (result) {
    return S_OK;
  }
  DWORD lastError = ::GetLastError();
  if (lastError == 0) {
    return E_FAIL;
  }
  return HRESULT_FROM_WIN32(lastError);
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
  }
  return S_OK;
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
