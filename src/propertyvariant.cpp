#include "propertyvariant.h"

#include <QString>

#include <guiddef.h>

#include <stdint.h>
#include <string>
#include <stdexcept>

PropertyVariant::PropertyVariant()
{
  PropVariantInit(this);
}

PropertyVariant::~PropertyVariant()
{
  clear();
}

void PropertyVariant::clear()
{
  PropVariantClear(this);
}

//Arguably the behviours for empty here are wrong.
template <> PropertyVariant::operator bool() const
{
  switch (vt)
  {
    case VT_EMPTY:
      return false;

    case VT_BOOL:
      return boolVal != VARIANT_FALSE;

    default:
      throw std::runtime_error("Property is not a bool");
  }
}

template <> PropertyVariant::operator uint64_t() const
{
  switch (vt)
  {
    case VT_EMPTY:
      return 0;

    case VT_UI1:
      return bVal;

    case VT_UI2:
      return uiVal;

    case VT_UI4:
      return ulVal;

    case VT_UI8:
      return static_cast<uint64_t>(uhVal.QuadPart);

    default:
      throw std::runtime_error("Property is not an unsigned integer");
  }
}

template <> PropertyVariant::operator uint32_t() const
{
  switch (vt)
  {
    case VT_EMPTY:
      return 0;

    case VT_UI1:
      return bVal;

    case VT_UI2:
      return uiVal;

    case VT_UI4:
      return ulVal;

    default:
      throw std::runtime_error("Property is not an unsigned integer");
  }
}


template <> PropertyVariant::operator std::wstring() const
{
  switch (vt)
  {
    case VT_EMPTY:
      return L"";

    case VT_BSTR:
      return std::wstring(bstrVal, ::SysStringLen(bstrVal));

    default:
      throw std::runtime_error("Property is not a string");
  }
}

template <> PropertyVariant::operator QString() const
{
  switch (vt)
  {
    case VT_EMPTY:
      return "";

    case VT_BSTR:
      return QString::fromWCharArray(bstrVal, ::SysStringLen(bstrVal));

    default:
      throw std::runtime_error("Property is not a string");
  }
}


//This is what he does, though it looks rather a strange use of the property
template <> PropertyVariant::operator std::string() const
{
  switch (vt)
  {
    case VT_EMPTY:
      return "";

    case VT_BSTR:
      //If he can do a memcpy, I can do a reinterpret case
      return std::string(reinterpret_cast<char const *>(bstrVal), ::SysStringByteLen(bstrVal));

    default:
      throw std::runtime_error("Property is not a string");
  }
}

//This is what he does, though it looks rather a strange use of the property
template <> PropertyVariant::operator GUID() const
{
  switch (vt)
  {
    case VT_BSTR:
      //He did a cast too!
      return *reinterpret_cast<const GUID *>(bstrVal);

    default:
      throw std::runtime_error("Property is not a GUID (string)");
  }
}

template <> PropertyVariant::operator FILETIME() const
{
  switch (vt)
  {
    case VT_FILETIME:
      return filetime;

    default:
      throw std::runtime_error("Property is not a file time");
  }
}

//Assignments
template <> PropertyVariant& PropertyVariant::operator=(std::wstring const &str)
{
  clear();
  vt = VT_BSTR;
  bstrVal = ::SysAllocString(str.c_str());
  if (bstrVal == NULL) {
    throw std::bad_alloc();
  }
  return *this;
}

template <> PropertyVariant& PropertyVariant::operator=(uint64_t const &n)
{
  clear();
  vt = VT_UI8;
  uhVal.QuadPart = n;
  return *this;
}
