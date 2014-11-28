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


#include "callback.h"



#ifdef _WINDLL
#define DLLEXPORT _declspec(dllexport)
#else
#define DLLEXPORT _declspec(dllimport)
#endif


class FileData {
public:
  virtual LPCWSTR getFileName() const = 0;
  virtual void setSkip(bool skip) = 0;
  virtual bool getSkip() const = 0;
  virtual void setOutputFileName(LPCWSTR fileName) = 0;
  virtual LPCWSTR getOutputFileName() const = 0;
  virtual UINT64 getCRC() const = 0;
  virtual bool isDirectory() const = 0;
};


class Archive {

public:

  enum Error {
    ERROR_NONE,
    ERROR_EXTRACT_CANCELLED,
    ERROR_LIBRARY_NOT_FOUND,
    ERROR_LIBRARY_INVALID,
    ERROR_ARCHIVE_NOT_FOUND,
    ERROR_FAILED_TO_OPEN_ARCHIVE,
    ERROR_INVALID_ARCHIVE_FORMAT,
    ERROR_LIBRARY_ERROR,
    ERROR_ARCHIVE_INVALID
  };

public:

  virtual ~Archive() {}

  virtual bool isValid() const = 0;

  virtual Error getLastError() const = 0;

  virtual bool open(LPCTSTR archiveName, PasswordCallback *passwordCallback) = 0;

  virtual bool close() = 0;

  virtual bool getFileList(FileData* const *&data, size_t &size) = 0;

  virtual bool extract(LPCTSTR outputDirectory, ProgressCallback *progressCallback,
                       FileChangeCallback* fileChangeCallback, ErrorCallback* errorCallback) = 0;

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
