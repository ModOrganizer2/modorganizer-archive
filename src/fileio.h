/*
Mod Organizer archive handling

Copyright (C) 2020 MO2 Team. All rights reserved.

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

#ifndef ARCHIVE_FILEIO_H
#define ARCHIVE_FILEIO_H

// This code is adapted from 7z client code.

#include <Windows.h>

#include "7zip//Archive/IArchive.h"

#include <filesystem>
#include <string>

namespace IO
{

/**
 * Small class that wraps windows BY_HANDLE_FILE_INFORMATION and returns
 * type matching 7z types.
 */
class FileInfo
{
public:
  FileInfo() : m_Valid{false} {};
  FileInfo(std::filesystem::path const& path, BY_HANDLE_FILE_INFORMATION fileInfo)
      : m_Valid{true}, m_Path(path), m_FileInfo{fileInfo}
  {}

  bool isValid() const { return m_Valid; }

  const std::filesystem::path& path() const { return m_Path; }

  UInt32 fileAttributes() const { return m_FileInfo.dwFileAttributes; }
  FILETIME creationTime() const { return m_FileInfo.ftCreationTime; }
  FILETIME lastAccessTime() const { return m_FileInfo.ftLastAccessTime; }
  FILETIME lastWriteTime() const { return m_FileInfo.ftLastWriteTime; }
  UInt32 volumeSerialNumber() const { return m_FileInfo.dwVolumeSerialNumber; }
  UInt64 fileSize() const
  {
    return ((UInt64)m_FileInfo.nFileSizeHigh) << 32 | m_FileInfo.nFileSizeLow;
  }
  UInt32 numberOfLinks() const { return m_FileInfo.nNumberOfLinks; }
  UInt64 fileInfex() const
  {
    return ((UInt64)m_FileInfo.nFileIndexHigh) << 32 | m_FileInfo.nFileIndexLow;
  }

  bool isArchived() const { return MatchesMask(FILE_ATTRIBUTE_ARCHIVE); }
  bool isCompressed() const { return MatchesMask(FILE_ATTRIBUTE_COMPRESSED); }
  bool isDir() const { return MatchesMask(FILE_ATTRIBUTE_DIRECTORY); }
  bool isEncrypted() const { return MatchesMask(FILE_ATTRIBUTE_ENCRYPTED); }
  bool isHidden() const { return MatchesMask(FILE_ATTRIBUTE_HIDDEN); }
  bool isNormal() const { return MatchesMask(FILE_ATTRIBUTE_NORMAL); }
  bool isOffline() const { return MatchesMask(FILE_ATTRIBUTE_OFFLINE); }
  bool isReadOnly() const { return MatchesMask(FILE_ATTRIBUTE_READONLY); }
  bool iasReparsePoint() const { return MatchesMask(FILE_ATTRIBUTE_REPARSE_POINT); }
  bool isSparse() const { return MatchesMask(FILE_ATTRIBUTE_SPARSE_FILE); }
  bool isSystem() const { return MatchesMask(FILE_ATTRIBUTE_SYSTEM); }
  bool isTemporary() const { return MatchesMask(FILE_ATTRIBUTE_TEMPORARY); }

private:
  bool MatchesMask(UINT32 mask) const
  {
    return ((m_FileInfo.dwFileAttributes & mask) != 0);
  }

  bool m_Valid;
  std::filesystem::path m_Path;
  BY_HANDLE_FILE_INFORMATION m_FileInfo;
};

class FileBase
{
public:  // Constructors, destructor, assignment.
  FileBase() noexcept : m_Handle{INVALID_HANDLE_VALUE} {}

  FileBase(FileBase&& other) noexcept : m_Handle{other.m_Handle}
  {
    other.m_Handle = INVALID_HANDLE_VALUE;
  }

  ~FileBase() noexcept { Close(); }

  FileBase(FileBase const&)            = delete;
  FileBase& operator=(FileBase const&) = delete;
  FileBase& operator=(FileBase&&)      = delete;

public:  // Operations
  bool Close() noexcept;

  bool GetPosition(UInt64& position) noexcept;
  bool GetLength(UInt64& length) const noexcept;

  bool Seek(Int64 distanceToMove, DWORD moveMethod, UInt64& newPosition) noexcept;
  bool Seek(UInt64 position, UInt64& newPosition) noexcept;
  bool SeekToBegin() noexcept;
  bool SeekToEnd(UInt64& newPosition) noexcept;

  // Note: Only the static version (unlike in 7z) because I want FileInfo to hold the
  // path to the file, and the non-static version is never used (except by the static
  // version).
  static bool GetFileInformation(std::filesystem::path const& path,
                                 FileInfo* info) noexcept;

protected:
  bool Create(std::filesystem::path const& path, DWORD desiredAccess, DWORD shareMode,
              DWORD creationDisposition, DWORD flagsAndAttributes) noexcept;

protected:
  static constexpr UInt32 kChunkSizeMax = (1 << 22);

  HANDLE m_Handle;
};

class FileIn : public FileBase
{
public:
  using FileBase::FileBase;

public:  // Operations
  bool Open(std::filesystem::path const& filepath, DWORD shareMode,
            DWORD creationDisposition, DWORD flagsAndAttributes) noexcept;
  bool OpenShared(std::filesystem::path const& filepath, bool shareForWrite) noexcept;
  bool Open(std::filesystem::path const& filepath) noexcept;

  bool Read(void* data, UInt32 size, UInt32& processedSize) noexcept;

protected:
  bool Read1(void* data, UInt32 size, UInt32& processedSize) noexcept;
  bool ReadPart(void* data, UInt32 size, UInt32& processedSize) noexcept;
};

class FileOut : public FileBase
{
public:
  using FileBase::FileBase;

public:  // Operations:
  bool Open(std::filesystem::path const& fileName, DWORD shareMode,
            DWORD creationDisposition, DWORD flagsAndAttributes) noexcept;
  bool Open(std::filesystem::path const& fileName) noexcept;

  bool SetTime(const FILETIME* cTime, const FILETIME* aTime,
               const FILETIME* mTime) noexcept;
  bool SetMTime(const FILETIME* mTime) noexcept;
  bool Write(const void* data, UInt32 size, UInt32& processedSize) noexcept;

  bool SetLength(UInt64 length) noexcept;
  bool SetEndOfFile() noexcept;

protected:  // Protected Operations:
  bool WritePart(const void* data, UInt32 size, UInt32& processedSize) noexcept;
};

/**
 * @brief Convert the given wide-string to a path object, after adding (if not already
 * present) the Windows long-path prefix.
 *
 * @param path The string containing the path.
 * @param exists true if the path should exists, false otherwize.
 *
 * @return the created path.
 */
inline std::filesystem::path make_path(std::wstring const& pathstr)
{
  namespace fs                         = std::filesystem;
  constexpr const wchar_t* lprefix     = L"\\\\?\\";
  constexpr const wchar_t* unc_prefix  = L"\\\\";
  constexpr const wchar_t* unc_lprefix = L"\\\\?\\UNC\\";

  // If path is already a long path, just return it:
  if (pathstr.starts_with(lprefix)) {
    return fs::path{pathstr}.make_preferred();
  }

  fs::path path{pathstr};

  // Convert to an absolute path:
  if (!path.is_absolute()) {
    path = fs::absolute(path);
  }

  // backslashes
  path = path.make_preferred();

  // Get rid of duplicate separators and relative moves
  path = path.lexically_normal();

  const std::wstring pathstr_fixed = path.native();

  // If this is a UNC, the prefix is different
  if (pathstr_fixed.starts_with(unc_prefix)) {
    return fs::path{unc_lprefix + pathstr_fixed.substr(2)};
  }

  // Add the long-path prefix (cannot concatenate string an path so need
  // to call .native() to concatenate):
  return fs::path{lprefix + pathstr_fixed};
}

}  // namespace IO

#endif
