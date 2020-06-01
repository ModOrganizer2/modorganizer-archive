#ifndef ARCHIVE_LIBRARY_H
#define ARCHIVE_LIBRARY_H

#include <Windows.h>

/**
 * Very small wrapper around Windows DLLs functions.
 */
class ALibrary {
public:

  ALibrary(const char* path) : 
    m_Module{ nullptr }, m_LastError{ ERROR_SUCCESS } {
    m_Module = LoadLibraryA(path);
    if (m_Module == nullptr) {
      updateLastError();
    }
  }

  ~ALibrary() {
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
  T resolve(const char* procName) {
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
  DWORD getLastError() const {
    return m_LastError;
  }

  /**
   * @return true if this library is open, false otherwize.
   */
  bool isOpen() const {
    // Note: The library is always open, unless it fails during construction.
    return m_Module != nullptr;
  }

  operator bool() const { return isOpen(); }

private:

  void updateLastError() {
    m_LastError = ::GetLastError();
  }

  HMODULE m_Module;
  DWORD m_LastError;

};

#endif