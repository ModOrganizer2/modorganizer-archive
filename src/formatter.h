#ifndef ARCHIVE_FORMAT_H
#define ARCHIVE_FORMAT_H

// This header specialize formatter() for some useful types. It should not be
// exposed outside of the library:

#include <fmt/format.h>

#include <filesystem>
#include <stdexcept>
#include <string>

// Specializing fmt::formatter works, but gives warning, for whatever reason... So putting
// everything in the namespace.
namespace fmt {

  // I don't know why fmt does not provide this...
  template<>
  struct formatter<std::string, wchar_t> : formatter<std::wstring, wchar_t>
  {
    template<typename FormatContext>
    auto format(std::string const& s, FormatContext& ctx) {
      return formatter<std::wstring, wchar_t>::format(std::wstring(s.begin(), s.end()), ctx);
    }
  };

  template<>
  struct formatter<std::exception, wchar_t> : formatter<std::string, wchar_t>
  {
    template<typename FormatContext>
    auto format(std::exception const& ex, FormatContext& ctx) {
      return formatter<std::string, wchar_t>::format(ex.what(), ctx);
    }
  };

  template<>
  struct formatter<std::error_code, wchar_t> : formatter<std::string, wchar_t>
  {
    template<typename FormatContext>
    auto format(std::error_code const& ec, FormatContext& ctx) {
      return formatter<std::string, wchar_t>::format(ec.message(), ctx);
    }
  };

  template<>
  struct formatter<std::filesystem::path, wchar_t> : formatter<std::wstring, wchar_t>
  {
    template<typename FormatContext>
    auto format(std::filesystem::path const& path, FormatContext& ctx) {
      return formatter<std::wstring, wchar_t>::format(path.native(), ctx);
    }
  };

}

#endif