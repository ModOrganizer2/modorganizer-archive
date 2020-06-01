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

#ifndef CALLBACK_H
#define CALLBACK_H

#include <functional>
#include <string>

namespace ArchiveCallbacks {

  enum class LogLevel {
    Debug,
    Info,
    Warning,
    Error
  };

  static const int MAX_PASSWORD_LENGTH = 256;

  using LogCallback = std::function<void(LogLevel, std::wstring const& log)>;
  using ProgressCallback = std::function<void(float)>;
  using PasswordCallback = std::function<std::wstring()>;
  using FileChangeCallback = std::function<void(std::wstring const&)>;
  using ErrorCallback = std::function<void(std::wstring const&)>;

}

#endif // CALLBACK_H
