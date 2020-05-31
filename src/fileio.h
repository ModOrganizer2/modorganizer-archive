#ifndef ARCHIVE_FILEIO_H
#define ARCHIVE_FILEIO_H

// This code is adapted from 7z client code.

#include <Windows.h>

#include "7zip//Archive/IArchive.h"

#include <string>

namespace IO {

  class FileOut {
  public: // Constructors, destructor, assignment.

    FileOut() noexcept : m_Handle{ INVALID_HANDLE_VALUE } { }

    FileOut(FileOut&& other) noexcept : m_Handle{ other.m_Handle } {
      other.m_Handle = INVALID_HANDLE_VALUE;
    }

    ~FileOut() noexcept {
      Close();
    }

    FileOut(FileOut const&) = delete;
    FileOut& operator=(FileOut const&) = delete;
    FileOut& operator=(FileOut&&) = delete;

  public: // Operations:

    bool Close() noexcept;
    bool Open(std::wstring const& fileName, DWORD shareMode, DWORD creationDisposition, DWORD flagsAndAttributes) noexcept;
    bool Open(std::wstring const& fileName) noexcept;

    bool SetTime(const FILETIME* cTime, const FILETIME* aTime, const FILETIME* mTime) noexcept;
    bool SetMTime(const FILETIME* mTime) noexcept;
    bool Write(const void* data, UInt32 size, UInt32& processedSize) noexcept;

    bool GetLength(UInt64& length) const noexcept;

    bool Seek(Int64 distanceToMove, DWORD moveMethod, UInt64& newPosition) const  noexcept;
    bool Seek(UInt64 position, UInt64& newPosition) const  noexcept;

    bool SeekToBegin() const noexcept;

    bool SeekToEnd(UInt64& newPosition) const noexcept;

    bool SetLength(UInt64 length) noexcept;
    bool SetEndOfFile() noexcept;

  protected: // Protected Operations:

    static constexpr UInt32 kChunkSizeMax = (1 << 22);

    bool WritePart(const void* data, UInt32 size, UInt32& processedSize) noexcept;

    bool Create(std::wstring const& path, DWORD desiredAccess, DWORD shareMode, DWORD creationDisposition, DWORD flagsAndAttributes) noexcept;

  protected:

    HANDLE m_Handle;
  };

}

#endif
