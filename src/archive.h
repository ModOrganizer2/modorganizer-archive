/*
Mod Organizer archive handling

Copyright (C) 2012 Sebastian Herbord. All rights reserved.

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


#ifndef ARCHIVE_H
#define ARCHIVE_H

#include <cstdint>
#include <memory>
#include <string>

#include "callback.h"

#ifndef DLLEXPORT
  #ifdef MODORGANIZER_ARCHIVE_BUILDING
  #define DLLEXPORT _declspec(dllexport)
  #else
  #define DLLEXPORT _declspec(dllimport)
  #endif
#endif

class FileData {
public:

  /**
   * @return the full filepath to this entry.
   */
  virtual std::wstring getFileName() const = 0;

  /**
   * @return the size of this entry in bytes (uncompressed).
   */
  virtual uint64_t getSize() const = 0;

  /**
   * @brief Add the given filename to the list of files to create from this
   *   entry when extracting.
   *
   * @param filepath The filename to add.
   */
  virtual void addOutputFileName(std::wstring const& filepath) = 0;
  
  /**
   * @brief Retrieve the list of filepaths to extract this entry to and clear the
   *   internal list.
   *
   * @return the list of paths this entry should be extracted to.
   */
  virtual std::vector<std::wstring> getAndClearOutputFileNames() = 0;
  
  /**
   * @return the CRC of this file.
   */
  virtual uint64_t getCRC() const = 0;

  /**
   * @return true if this entry is a directory, false otherwize.
   */
  virtual bool isDirectory() const = 0;
};


class Archive {
public: // Declarations

  enum class Error {
    ERROR_NONE,
    ERROR_EXTRACT_CANCELLED,
    ERROR_LIBRARY_NOT_FOUND,
    ERROR_LIBRARY_INVALID,
    ERROR_ARCHIVE_NOT_FOUND,
    ERROR_FAILED_TO_OPEN_ARCHIVE,
    ERROR_INVALID_ARCHIVE_FORMAT,
    ERROR_LIBRARY_ERROR,
    ERROR_ARCHIVE_INVALID,
    ERROR_OUT_OF_MEMORY
  };

public: // Special member functions:

  virtual ~Archive() {}

  void operator delete(void* ptr) {
    if (ptr != nullptr) {
      Archive* object = static_cast<Archive*>(ptr);
      object->destroy();
    }
  }

public:

  /**
   * @brief Check if this Archive wrapper is in a valid state.
   *
   * A non-valid Archive instance usually means that the 7z DLLs could not be loaded properly. Failures
   * to open or extract archives do not invalidate the Archive, so this should only be used to check
   * if the Archive object has been initialized properly.
   *
   * @return true if this instance is valid, false otherwise. 
   */
  virtual bool isValid() const = 0;

  /**
   * @return retrieve the error-code of the last error that occurred.
   */
  virtual Error getLastError() const = 0;

  /**
   * @brief Set the callback used to log messages.
   *
   * To remove the callback, you can pass a default-constructed LogCallback object.
   *
   * @param logCallback The new callback to use for logging message.
   */
  virtual void setLogCallback(ArchiveCallbacks::LogCallback logCallback) = 0;

  /**
   * @brief Open the given archive.
   *
   * @param archivePath Path to the archive to open.
   * @param passwordCallback Callback to use to ask user for password. This callback must remain
   *   valid until extraction is complete since some types of archives only requires password when
   *   extracting.
   *
   * @return true if the archive was open properly, false otherwise.
   */
  virtual bool open(std::wstring const &archivePath, ArchiveCallbacks::PasswordCallback passwordCallback) = 0;

  /**
   * @brief Close the currently opened archive.
   */
  virtual void close() = 0;

  /**
   * @return the list of files in the currently opened archive.
   */
  virtual const std::vector<FileData*>& getFileList() const = 0;

  /**
   * @brief Extract the content of the archive.
   *
   * This function uses the filenames from FileData to obtain the extraction paths of file.
   *
   * @param outputDirectory Path to the directory where the archive should be extracted. If not empty,
   *   conflicting files will be replaced by the extracted ones.
   * @param progressCallback Function called to notify extraction progress. This function is called
   *   when new data is written to the disk, not when the archive is read, so it may take a little
   *   time to start.
   * @param fileChangeCallback Function called when the file currently being extracted changes.
   * @param errorCallback Function called when an error occurs.
   *
   * @return true if the archive was extracted, false otherwise.
   */
  virtual bool extract(std::wstring const &outputDirectory, ArchiveCallbacks::ProgressCallback progressCallback,
    ArchiveCallbacks::FileChangeCallback fileChangeCallback, ArchiveCallbacks::ErrorCallback errorCallback) = 0;

  /**
   * @brief Cancel the current extraction process.
   */
  virtual void cancel() = 0;

private:

  virtual void destroy() = 0;

};

/**
 * @brief Factory function for archive-objects.
 *
 * @return a pointer to a new Archive object that can be used to manipulate archives.
 */
DLLEXPORT std::unique_ptr<Archive> CreateArchive();


#endif // ARCHIVE_H
