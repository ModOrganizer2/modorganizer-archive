#ifndef UNKNOWN_IMPL_H
#define UNKNOWN_IMPL_H

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

/* This implements a common way of creating classes which implement one or more
 * COM interfaces.
 *
 * Your implementaton should be something like this
 *
 * class MyImplementation :
 *  public interface1,
 *  public interface2 //etc to taste
 * {
 *     UNKNOWN_1_INTERFACE(interface1);
 * or  UNKNOWN_2_INTERFACE(interface1, interface2); etc
 *
 * It would probably be possible to do something very magic with macros
 * that allowed you to do
 * class My_Implmentation : UNKNOWN_IMPL_n(interface1, ...)
 *
 * that did all the above for you, but I can't see how to maintain the braces
 * in order to make the class declaration look nice.
 *
 * You might ask why I have e-implemented this from the 7-zip stuff (sort of).
 *
 * Well,
 *
 * Firstly, the 7zip macros are slightly buggy (they don't null the outpointer
 * if the interface isn't found), and they aren't thread safe.
 *
 * Secondly, although 7-zip has a requirement to run on platforms other than
 * windows, we don't. So I changed this to use the Com functionality supplied
 * by windows, which (potentially) has better debugging facilities
 *
 * Thirdly, this removes a number of dependencies on 7-zip source which aren't
 * really part of the API
 */

/* Do not use these macros, use the ones at the bottom */
#define UNKNOWN__INTERFACE_BEGIN\
 public:\
  STDMETHOD_(ULONG, AddRef)()\
  {\
    return ::InterlockedIncrement(&m_RefCount__);\
  }\
\
  STDMETHOD_(ULONG, Release)()\
  {\
    ULONG res = ::InterlockedDecrement(&m_RefCount__);\
    if (res == 0) {\
      delete this;\
    }\
    return res;\
  }\
\
  STDMETHOD(QueryInterface)(REFGUID iid, void **outObject)\
  {\
    if (iid == IID_IUnknown) {

#define UNKNOWN__INTERFACE_UNKNOWN(interface)\
      *outObject = static_cast<IUnknown *>(static_cast<interface *>(this));\
    }

#define UNKNOWN__INTERFACE_NAME(interface)\
    else if (iid == IID_##interface) {\
      *outObject = static_cast<interface *>(this);\
    }

#define UNKNOWN__INTERFACE_END\
    else {\
      *outObject = nullptr;\
      return E_NOINTERFACE;\
    }\
    AddRef();\
    return S_OK;\
  }\
 private:\
  ULONG m_RefCount__ = 0

/* These are the macros you should be using */
#define UNKNOWN_1_INTERFACE(interface)\
  UNKNOWN__INTERFACE_BEGIN\
  UNKNOWN__INTERFACE_UNKNOWN(interface)\
  UNKNOWN__INTERFACE_NAME(interface)\
  UNKNOWN__INTERFACE_END

#define UNKNOWN_2_INTERFACE(interface1, interface2)\
  UNKNOWN__INTERFACE_BEGIN\
  UNKNOWN__INTERFACE_UNKNOWN(interface1)\
  UNKNOWN__INTERFACE_NAME(interface1)\
  UNKNOWN__INTERFACE_NAME(interface2)\
  UNKNOWN__INTERFACE_END

#define UNKNOWN_3_INTERFACE(interface1, interface2, interface3)\
  UNKNOWN__INTERFACE_BEGIN\
  UNKNOWN__INTERFACE_UNKNOWN(interface1)\
  UNKNOWN__INTERFACE_NAME(interface1)\
  UNKNOWN__INTERFACE_NAME(interface2)\
  UNKNOWN__INTERFACE_NAME(interface3)\
  UNKNOWN__INTERFACE_END

#define UNKNOWN_4_INTERFACE(interface1, interface2, interface3, interface4)\
  UNKNOWN__INTERFACE_BEGIN\
  UNKNOWN__INTERFACE_UNKNOWN(interface1)\
  UNKNOWN__INTERFACE_NAME(interface1)\
  UNKNOWN__INTERFACE_NAME(interface2)\
  UNKNOWN__INTERFACE_NAME(interface3)\
  UNKNOWN__INTERFACE_NAME(interface4)\
  UNKNOWN__INTERFACE_END

#endif // UNKNOWN_IMPL_H
