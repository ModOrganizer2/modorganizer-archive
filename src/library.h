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

#ifndef ARCHIVE_LIBRARY_H
#define ARCHIVE_LIBRARY_H

#include <Windows.h>

/**
 * Very small wrapper around Windows DLLs functions.
 */
class ALibrary
{
public:
  ALibrary(const char* path) : m_Module{nullptr}, m_LastError{ERROR_SUCCESS}
  {
    m_Module = LoadLibraryA(path);
    if (m_Module == nullptr) {
      updateLastError();
    }
  }

  ~ALibrary()
  {
    if (m_Module) {
      FreeLibrary(m_Module);
    }
  }

  /**
   * @param Name of the function or variable.
   *
   * @return a pointer to the given function or variable, or nullptr if an error
   *     occurred.
   *
   * @note T must be a pointer type, and no type checking is performed.
   */
  template <class T>
  T resolve(const char* procName)
  {
    if (!m_Module) {
      return nullptr;
    }
    auto proc = GetProcAddress(m_Module, procName);
    if (!proc) {
      updateLastError();
      return nullptr;
    }
    return reinterpret_cast<T>(proc);
  }

  /**
   * @return the last error that occurs with this object, or ERROR_SUCCESS if no
   *     error occurred.
   */
  DWORD getLastError() const { return m_LastError; }

  /**
   * @return true if this library is open, false otherwize.
   */
  bool isOpen() const
  {
    // Note: The library is always open, unless it fails during construction.
    return m_Module != nullptr;
  }

  operator bool() const { return isOpen(); }

private:
  void updateLastError() { m_LastError = ::GetLastError(); }

  HMODULE m_Module;
  DWORD m_LastError;
};

#endif
