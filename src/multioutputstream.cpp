#include "multioutputstream.h"

#include <QDir>

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

MultiOutputStream::MultiOutputStream()
{}

MultiOutputStream::~MultiOutputStream()
{}

HRESULT MultiOutputStream::Close()
{
  for (OutFile &f : m_Handles) {
    f->close();
  }
  return S_OK;
}

bool MultiOutputStream::Open(std::vector<QString> const &fileNames)
{
  m_ProcessedSize = 0;
  bool ok = true;
  m_Handles.clear();
  for (std::size_t f = 0; f != fileNames.size(); ++f) {
    m_Handles.push_back(OutFile(new QFile(fileNames[f])));
    if (!m_Handles[f]->open(QIODevice::WriteOnly)) {
      ok = false;
    }
  }
  return ok;
}

STDMETHODIMP MultiOutputStream::Write(const void *data, UInt32 size, UInt32 *processedSize)
{
  bool update_processed(true);
  for (OutFile &f : m_Handles) {
    qint64 realProcessedSize = f->write(static_cast<char const *>(data), size);
    if (realProcessedSize == -1) {
      return ConvertBoolToHRESULT(false);
    }
    if (update_processed) {
      m_ProcessedSize += realProcessedSize;
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
  for (OutFile &f : m_Handles) {
    //Get the underlying windows handle from the QFile object.
    HANDLE h = reinterpret_cast<HANDLE>(::_get_osfhandle(f->handle()));
    if (! ::SetFileTime(h, nullptr, nullptr, mTime)) {
      return false;
    }
  }
  return true;
}
