#include <Unknwn.h>
#include "inputstream.h"

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

InputStream::InputStream()
{}

InputStream::~InputStream() { }


bool InputStream::Open(std::filesystem::path const& filename)
{
  return m_File.Open(filename);
}

STDMETHODIMP InputStream::Read(void *data, UInt32 size, UInt32 *processedSize)
{
  UInt32 realProcessedSize;
  bool result =  m_File.Read(data, size, realProcessedSize);

  if (processedSize != nullptr) {
    *processedSize = realProcessedSize;
  }

  if (result) {
    return S_OK;
  }

  return HRESULT_FROM_WIN32(::GetLastError());
}

STDMETHODIMP InputStream::Seek(Int64 offset, UInt32 seekOrigin, UInt64 *newPosition)
{
  UInt64 realNewPosition = offset;
  bool result = m_File.Seek(offset, seekOrigin, realNewPosition);

  if (newPosition) {
    *newPosition = realNewPosition;
  }
  return ConvertBoolToHRESULT(result);
}
