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


#ifndef EXTRACTCALLBACK_H
#define EXTRACTCALLBACK_H

#include "callback.h"
#include "multioutputstream.h"
#include "unknown_impl.h"

#include "7zip/Archive/IArchive.h"
#include "7zip/IPassword.h"

#include <fmt/format.h>
#include <chrono>

template<>
struct fmt::formatter<std::error_code, wchar_t>: formatter<std::wstring, wchar_t>
{
  template<typename FormatContext>
  auto format(std::error_code const& ec, FormatContext& ctx) {
    // Need to move error_code to a wide string...
    auto s = ec.message();
    return formatter<std::wstring, wchar_t>::format(std::wstring(s.begin(), s.end()), ctx);
  }
};

template<>
struct fmt::formatter<std::filesystem::path, wchar_t> : formatter<std::wstring, wchar_t>
{
  template<typename FormatContext>
  auto format(std::filesystem::path const& path, FormatContext& ctx) {
    return formatter<std::wstring, wchar_t>::format(path.native(), ctx);
  }
};


#include <QDir>
#include <filesystem>
class QString;

#include <atlbase.h>

class FileData;

class CArchiveExtractCallback: public IArchiveExtractCallback,
                               public ICryptoGetTextPassword
{

  //A note: It appears that the IArchiveExtractCallback interface includes the
  //IProgress interface, swo we need to respond to it
  UNKNOWN_3_INTERFACE(IArchiveExtractCallback,
                      ICryptoGetTextPassword,
                      IProgress);

public:

  CArchiveExtractCallback(ProgressCallback *progressCallback,
                          FileChangeCallback *fileChangeCallback,
                          ErrorCallback *errorCallback,
                          PasswordCallback *passwordCallback,
                          IInArchive *archiveHandler,
                          const QString &directoryPath,
                          FileData * const *fileData,
                          std::size_t nbFiles,
                          UInt64 totalFileSize,
                          QString *password);

  virtual ~CArchiveExtractCallback();

  void SetCanceled(bool aCanceled);

  INTERFACE_IArchiveExtractCallback(;)

  // ICryptoGetTextPassword
  STDMETHOD(CryptoGetTextPassword)(BSTR *aPassword);

private:

  void reportError(const QString &message);

  template <class... Args>
  void reportError(const wchar_t* format, Args&& ...args)
  {
    auto message = fmt::format(format, std::forward<Args>(args)...);
    reportError(QString::fromStdWString(message));
  }

  template <typename T> bool getOptionalProperty(UInt32 index, int property, T *result) const;
  template <typename T> T getProperty(UInt32 index, int property) const;

private:

  CComPtr<IInArchive> m_ArchiveHandler;

  UInt64 m_Total;

  std::filesystem::path m_DirectoryPath;
  bool m_Extracting;
  bool m_Canceled;

  struct Timer {

    using clock_t = std::chrono::system_clock;

    struct TimerGuard {

      TimerGuard(TimerGuard const&) = delete;
      TimerGuard(TimerGuard &&) = delete;
      TimerGuard& operator=(TimerGuard const&) = delete;
      TimerGuard& operator=(TimerGuard &&) = delete;

      ~TimerGuard() {
        m_Timer.ncalls++;
        m_Timer.time += clock_t::now() - m_Start;
      }

    private:
      TimerGuard(Timer& timer) : m_Timer{ timer }, m_Start{ clock_t::now() } { }

      Timer& m_Timer;
      clock_t::time_point m_Start;

      friend class Timer;
    };

    Timer() = default;

    TimerGuard instrument() {
      return { *this };
    }

    std::string toString(std::string const& name) const {
      auto ms = [](auto&& t) { return std::chrono::duration<double, std::milli>(t); };
      return fmt::format("Instrument '{}': {} calls, total of {}ms, {:.3f}ms per call on average.",
        name, ncalls, ms(time).count(), ms(time).count() / ncalls);
    }

  private:

    std::size_t ncalls{ 0 };
    clock_t::duration time{ 0 };

  };

  struct Timers {
    Timer GetStream;
    struct {
      Timer SetMTime;
      Timer Close;
      Timer Release;
      Timer SetFileAttributesW;
    } SetOperationResult;
  } m_Timers;

  struct CProcessedFileInfo {
    FILETIME MTime;
    UInt32 Attrib;
    bool isDir;
    bool AttribDefined;
    bool MTimeDefined;
  } m_ProcessedFileInfo;

  MultiOutputStream *m_OutputFileStream;
  CComPtr<MultiOutputStream> m_OutFileStreamCom;

  std::vector<std::filesystem::path> m_FullProcessedPaths;

  FileData* const *m_FileData;
  std::size_t m_NbFiles;
  UInt64 m_TotalFileSize;
  UInt64 m_LastCallbackFileSize;
  UInt64 m_ExtractedFileSize;
  QString *m_Password;

  ProgressCallback *m_ProgressCallback;
  FileChangeCallback *m_FileChangeCallback;
  ErrorCallback *m_ErrorCallback;
  PasswordCallback *m_PasswordCallback;

};


#endif // EXTRACTCALLBACK_H
