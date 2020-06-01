#ifndef ARCHIVE_FILEIO_H
#define ARCHIVE_FILEIO_H

// This code is adapted from 7z client code.

#include <Windows.h>

#include "7zip//Archive/IArchive.h"

#include <filesystem>
#include <string>

namespace IO {

  /**
   * Small class that wraps windows BY_HANDLE_FILE_INFORMATION and returns
   * type matching 7z types.
   */
  class FileInfo {
  public:

    FileInfo() : m_Valid{ false } {};
    FileInfo(std::filesystem::path const& path, BY_HANDLE_FILE_INFORMATION fileInfo) : 
      m_Valid{ true }, m_Path(path), m_FileInfo{ fileInfo } { }

    bool isValid() const { return m_Valid; }

    const std::filesystem::path& path() const { return m_Path; }

    UInt32 fileAttributes() const { return m_FileInfo.dwFileAttributes; }
    FILETIME creationTime() const { return m_FileInfo.ftCreationTime; }
    FILETIME lastAccessTime() const { return m_FileInfo.ftLastAccessTime; }
    FILETIME lastWriteTime() const { return m_FileInfo.ftLastWriteTime; }
    UInt32 volumeSerialNumber() const { return m_FileInfo.dwVolumeSerialNumber; }
    UInt64 fileSize() const { return ((UInt64)m_FileInfo.nFileSizeHigh) << 32 | m_FileInfo.nFileSizeLow; }
    UInt32 numberOfLinks() const { return m_FileInfo.nNumberOfLinks; }
    UInt64 fileInfex() const { return ((UInt64)m_FileInfo.nFileIndexHigh) << 32 | m_FileInfo.nFileIndexLow; }

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

    bool MatchesMask(UINT32 mask) const { return ((m_FileInfo.dwFileAttributes & mask) != 0); }

    bool m_Valid;
    std::filesystem::path m_Path;
    BY_HANDLE_FILE_INFORMATION m_FileInfo;
  };

  class FileBase {
  public: // Constructors, destructor, assignment.

    FileBase() noexcept : m_Handle{ INVALID_HANDLE_VALUE } { }

    FileBase(FileBase&& other) noexcept : m_Handle{ other.m_Handle } {
      other.m_Handle = INVALID_HANDLE_VALUE;
    }

    ~FileBase() noexcept {
      Close();
    }

    FileBase(FileBase const&) = delete;
    FileBase& operator=(FileBase const&) = delete;
    FileBase& operator=(FileBase&&) = delete;

  public: // Operations

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
    static bool GetFileInformation(std::filesystem::path const& path, FileInfo* info) noexcept;

  protected:

    bool Create(std::filesystem::path const& path, DWORD desiredAccess, DWORD shareMode, DWORD creationDisposition, DWORD flagsAndAttributes) noexcept;

  protected:

    static constexpr UInt32 kChunkSizeMax = (1 << 22);

    HANDLE m_Handle;

  };

  class FileIn : public FileBase {
  public:
    using FileBase::FileBase;

  public: // Operations
    bool Open(std::filesystem::path const& filepath, DWORD shareMode, DWORD creationDisposition, DWORD flagsAndAttributes) noexcept;
    bool OpenShared(std::filesystem::path const& filepath, bool shareForWrite) noexcept;
    bool Open(std::filesystem::path const& filepath) noexcept;

    bool Read(void* data, UInt32 size, UInt32& processedSize) noexcept;

  protected:
    bool Read1(void* data, UInt32 size, UInt32& processedSize) noexcept;
    bool ReadPart(void* data, UInt32 size, UInt32& processedSize) noexcept;
  };

  class FileOut: public FileBase {
  public:
    using FileBase::FileBase;

  public: // Operations:

    bool Open(std::filesystem::path const& fileName, DWORD shareMode, DWORD creationDisposition, DWORD flagsAndAttributes) noexcept;
    bool Open(std::filesystem::path const& fileName) noexcept;

    bool SetTime(const FILETIME* cTime, const FILETIME* aTime, const FILETIME* mTime) noexcept;
    bool SetMTime(const FILETIME* mTime) noexcept;
    bool Write(const void* data, UInt32 size, UInt32& processedSize) noexcept;


    bool SetLength(UInt64 length) noexcept;
    bool SetEndOfFile() noexcept;

  protected: // Protected Operations:

    bool WritePart(const void* data, UInt32 size, UInt32& processedSize) noexcept;
  };

}

#endif
