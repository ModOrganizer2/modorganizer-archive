#ifndef PROPERTYVARIANT_H
#define PROPERTYVARIANT_H

#include <PropIdl.h>

/** This class implements a wrapper round the PROPVARIANT structure which
 * makes it a little friendler to use and a little more C++ish
 *
 * Sadly the inheritance needs to be public. Once you have a pointer to the
 * base class you can do pretty much anything to it anyway.
 */
class PropertyVariant : public tagPROPVARIANT
{
public:
  PropertyVariant();
  ~PropertyVariant();

  //clears the property should you wish to overwrite it with another one.
  void clear();

  bool is_empty() {
    return vt == VT_EMPTY;
  }

  //It's too much like hard work listing all the valid ones. If it doesn't
  //link, then you need to implement it...
  template <typename T> explicit operator T() const;

  template <typename T> PropertyVariant& operator=(T const &);

};

#endif // PROPVARIANT_H
