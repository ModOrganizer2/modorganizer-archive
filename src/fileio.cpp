#include "fileio.h"

inline bool BOOLToBool(BOOL v) { return (v != FALSE); }

namespace IO {

  bool FileOut::Close() noexcept {
    if (m_Handle == INVALID_HANDLE_VALUE)
      return true;
    if (!::CloseHandle(m_Handle))
      return false;
    m_Handle = INVALID_HANDLE_VALUE;
    return true;
  }

  bool FileOut::Open(std::wstring const& fileName, DWORD shareMode, DWORD creationDisposition, DWORD flagsAndAttributes) noexcept {
    return Create(fileName, GENERIC_WRITE, shareMode, creationDisposition, flagsAndAttributes);
  }

  bool FileOut::Open(std::wstring const& fileName) noexcept {
    return Open(fileName, FILE_SHARE_READ, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL);
  }

  bool FileOut::SetTime(const FILETIME* cTime, const FILETIME* aTime, const FILETIME* mTime) noexcept {
    return BOOLToBool(::SetFileTime(m_Handle, cTime, aTime, mTime));
  }
  bool FileOut::SetMTime(const FILETIME* mTime) noexcept {
    return SetTime(NULL, NULL, mTime);
  }
  bool FileOut::Write(const void* data, UInt32 size, UInt32& processedSize) noexcept {
    processedSize = 0;
    do
    {
      UInt32 processedLoc = 0;
      bool res = WritePart(data, size, processedLoc);
      processedSize += processedLoc;
      if (!res)
        return false;
      if (processedLoc == 0)
        return true;
      data = (const void*)((const unsigned char*)data + processedLoc);
      size -= processedLoc;
    } while (size > 0);
    return true;
  }

  bool FileOut::GetLength(UInt64& length) const noexcept {
    DWORD sizeHigh;
    DWORD sizeLow = ::GetFileSize(m_Handle, &sizeHigh);
    if (sizeLow == 0xFFFFFFFF)
      if (::GetLastError() != NO_ERROR)
        return false;
    length = (((UInt64)sizeHigh) << 32) + sizeLow;
    return true;
  }

  bool FileOut::Seek(Int64 distanceToMove, DWORD moveMethod, UInt64& newPosition) const  noexcept {
    LONG high = (LONG)(distanceToMove >> 32);
    DWORD low = ::SetFilePointer(m_Handle, (LONG)(distanceToMove & 0xFFFFFFFF), &high, moveMethod);
    if (low == 0xFFFFFFFF)
      if (::GetLastError() != NO_ERROR)
        return false;
    newPosition = (((UInt64)(UInt32)high) << 32) + low;
    return true;
  }
  bool FileOut::Seek(UInt64 position, UInt64& newPosition) const  noexcept {
    return Seek(position, FILE_BEGIN, newPosition);
  }

  bool FileOut::SeekToBegin() const noexcept
  {
    UInt64 newPosition;
    return Seek(0, newPosition);
  }

  bool FileOut::SeekToEnd(UInt64& newPosition) const noexcept
  {
    return Seek(0, FILE_END, newPosition);
  }

  bool FileOut::SetLength(UInt64 length) noexcept {
    UInt64 newPosition;
    if (!Seek(length, newPosition))
      return false;
    if (newPosition != length)
      return false;
    return SetEndOfFile();
  }
  bool FileOut::SetEndOfFile() noexcept {
    return BOOLToBool(::SetEndOfFile(m_Handle));
  }

  bool FileOut::WritePart(const void* data, UInt32 size, UInt32& processedSize) noexcept {
    if (size > kChunkSizeMax)
      size = kChunkSizeMax;
    DWORD processedLoc = 0;
    bool res = BOOLToBool(::WriteFile(m_Handle, data, size, &processedLoc, NULL));
    processedSize = (UInt32)processedLoc;
    return res;
  }

  bool FileOut::Create(std::wstring const& path, DWORD desiredAccess, DWORD shareMode, DWORD creationDisposition, DWORD flagsAndAttributes) noexcept {
    if (!Close()) {
      return false;
    }

    m_Handle = ::CreateFileW(path.c_str(), desiredAccess, shareMode,
      (LPSECURITY_ATTRIBUTES)NULL, creationDisposition, flagsAndAttributes, (HANDLE)NULL);

    return m_Handle != INVALID_HANDLE_VALUE;
  }

}