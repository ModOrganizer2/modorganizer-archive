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

public:

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

public:

  virtual ~Archive() {}

  virtual bool isValid() const = 0;

  virtual Error getLastError() const = 0;

  virtual void setLogCallback(LogCallback logCallback) = 0;

  virtual bool open(std::wstring const &archiveName, PasswordCallback passwordCallback) = 0;

  virtual void close() = 0;

  virtual const std::vector<FileData*>& getFileList() const = 0;

  virtual bool extract(std::wstring const &outputDirectory, ProgressCallback progressCallback,
                       FileChangeCallback fileChangeCallback, ErrorCallback errorCallback) = 0;

  virtual void cancel() = 0;

  void operator delete(void *ptr) {
    if (ptr != nullptr) {
      Archive *object = static_cast<Archive*>(ptr);
      object->destroy();
    }
  }

private:

  virtual void destroy() = 0;

};


/// factory function for archive-objects
extern "C" DLLEXPORT Archive* CreateArchive();


#endif // ARCHIVE_H
