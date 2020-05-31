#include <Unknwn.h>
#include "multioutputstream.h"

#include <fcntl.h>
#include <io.h>

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

MultiOutputStream::MultiOutputStream(WriteCallback callback) :
  m_WriteCallback(callback) {}

MultiOutputStream::~MultiOutputStream() { }

HRESULT MultiOutputStream::Close()
{
  for (auto& handle : m_Handles) {
    handle.close();
  }
  return S_OK;
}

bool MultiOutputStream::Open(std::vector<std::filesystem::path> const & filepaths)
{
  m_ProcessedSize = 0;
  bool ok = true;
  m_Handles.clear();
  for (auto &path: filepaths) {
    int handle = _wopen(path.c_str(), _O_CREAT | _O_BINARY | _O_WRONLY, _S_IREAD | _S_IWRITE);
    if (handle != -1) {
      m_Handles.emplace_back(handle);
    }
    else {
      ok = false;
    }
  }
  return ok;
}

#include <QDebug>

STDMETHODIMP MultiOutputStream::Write(const void *data, UInt32 size, UInt32 *processedSize)
{
  bool update_processed(true);
  for (auto &wrapper : m_Handles) {
    qint64 realProcessedSize = write(wrapper.handle(), static_cast<char const *>(data), size);
    if (realProcessedSize == -1) {
      return ConvertBoolToHRESULT(false);
    }
    if (update_processed) {
      m_ProcessedSize += realProcessedSize;
      /* if (m_WriteCallback) {
        m_WriteCallback(realProcessedSize, m_ProcessedSize);
      } */
      update_processed = false;
    }
    if (processedSize != nullptr) {
      *processedSize = realProcessedSize;
    }
  }
  return S_OK;
}

bool MultiOutputStream::SetMTime(FILETIME const *mTime)
{
  for (auto &wrapper : m_Handles) {
    //Get the underlying windows handle from the QFile object.
    HANDLE h = reinterpret_cast<HANDLE>(::_get_osfhandle(wrapper.handle()));
    if (! ::SetFileTime(h, nullptr, nullptr, mTime)) {
      return false;
    }
  }
  return true;
}
