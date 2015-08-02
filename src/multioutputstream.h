#ifndef MULTIOUTPUTSTREAM_H
#define MULTIOUTPUTSTREAM_H

#ifdef _WIN32
#define USE_WIN_FILE
#endif

#ifdef USE_WIN_FILE
#include "Windows/FileIO.h"
#else
#include "Common/C_FileIO.h"
#endif

#include "Common/MyCom.h"
#include "Common/MyString.h"
#include "7zip/IStream.h"

#include <string>
#include <vector>

/** This class is like a COutputStream but wraps multiple file handles.
 * Note that the handling on errors could be better. Basically, except for
 * Close, any processing will stop on the first error
 */
class MultiOutputStream :
    public IOutStream,
    public CMyUnknownImp
{
  #ifdef USE_WIN_FILE
  typedef NWindows::NFile::NIO::COutFile OutFile;
  #else
  typedef NC::NFile::NIO::COutFile OutFile;
  #endif
  std::vector<OutFile> m_Handles;

public:
  virtual ~MultiOutputStream() {}

  bool Create(std::vector<UString> const &fileNames, bool createAlways);

  bool Open(std::vector<UString> const &fileNames, DWORD creationDisposition);

  #ifdef USE_WIN_FILE
  #ifndef _UNICODE
  //Not implemented. Why bother?
  bool Create(LPCWSTR fileName, bool createAlways);
  bool Open(LPCWSTR fileName, DWORD creationDisposition)
  #endif
  #endif

  HRESULT Close();

  UInt64 ProcessedSize;

  #ifdef USE_WIN_FILE
  bool SetTime(const FILETIME *cTime, const FILETIME *aTime, const FILETIME *mTime);

  bool SetMTime(const FILETIME *mTime);
  #endif


  MY_UNKNOWN_IMP1(IOutStream)

  STDMETHOD(Write)(const void *data, UInt32 size, UInt32 *processedSize);
  STDMETHOD(Seek)(Int64 offset, UInt32 seekOrigin, UInt64 *newPosition);
  STDMETHOD(SetSize)(UInt64 newSize);
};



#endif // MULTIOUTPUTSTREAM_H
