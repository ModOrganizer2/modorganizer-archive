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


#define WIN32_LEAN_AND_MEAN
#include <windows.h>


template <typename RET, typename PAR>
class Callback
{

public:

  virtual RET operator()(PAR parameter) = 0;

};


template <typename RET, typename PAR>
class FunctionCallback : public Callback<RET, PAR>
{
public:

  typedef RET (*Function)(PAR);

public:

  FunctionCallback(Function function)
    : m_Function(function)
  {
  }

  virtual RET operator()(PAR parameter) {
    return m_Function(parameter);
  }

private:

  Function m_Function;
};


template <class CLASS, typename RET, typename PAR>
class MethodCallback : public Callback<RET, PAR>
{
public:

 typedef RET (CLASS::*Method)(PAR);

 MethodCallback(CLASS* object, Method method)
   : m_Object(object), m_Method(method)
 {}

 virtual RET operator()(PAR parameter) {
    return (m_Object->*m_Method)(parameter);
 }

private:

 CLASS* m_Object;
 Method m_Method;

};

static const int MAX_PASSWORD_LENGTH = 256;

typedef Callback<void, float> ProgressCallback;
typedef Callback<void, LPSTR> PasswordCallback;
typedef Callback<void, LPCWSTR> FileChangeCallback;
typedef Callback<void, LPCWSTR> ErrorCallback;


#endif // CALLBACK_H
