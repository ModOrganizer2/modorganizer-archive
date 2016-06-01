#include <Unknwn.h>
#include "inputstream.h"

#include <QDir>
#include <QString>

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

InputStream::~InputStream()
{}


bool InputStream::Open(const QString &filename)
{
  m_File.setFileName(filename);
  return m_File.open(QIODevice::ReadOnly);
}

STDMETHODIMP InputStream::Read(void *data, UInt32 size, UInt32 *processedSize)
{
  qint64 realProcessedSize = m_File.read(static_cast<char *>(data), size);
  if (processedSize != nullptr) {
    *processedSize = realProcessedSize == -1 ? 0 : static_cast<UInt32>(realProcessedSize);
  }

  return ConvertBoolToHRESULT(realProcessedSize != -1);
}

STDMETHODIMP InputStream::Seek(Int64 offset, UInt32 seekOrigin, UInt64 *newPosition)
{
  qint64 realNewPosition = offset;
  switch (seekOrigin)
  {
    default:
      return STG_E_INVALIDFUNCTION;

    case FILE_BEGIN:
      break;

    case FILE_CURRENT:
      realNewPosition += m_File.pos();
      break;

    case FILE_END:
      realNewPosition += m_File.size();
  }

  bool result = m_File.seek(realNewPosition);
  if (result && newPosition != nullptr) {
    *newPosition = static_cast<UInt64>(realNewPosition);
  }
  return ConvertBoolToHRESULT(result);

}
