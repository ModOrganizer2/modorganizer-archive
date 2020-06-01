#ifndef INPUTSTREAM_H
#define INPUTSTREAM_H


#include "7zip/IStream.h"

#include <filesystem>

#include "fileio.h"
#include "unknown_impl.h"

/** This class implements an input stream for opening archive files
 *
 * Note that the handling on errors could be better.
 */
class InputStream :
    public IInStream
{

  UNKNOWN_1_INTERFACE(IInStream);

public:
  InputStream();

  virtual ~InputStream();

  bool Open(std::filesystem::path const &filename);

  STDMETHOD(Read)(void *data, UInt32 size, UInt32 *processedSize);
  STDMETHOD(Seek)(Int64 offset, UInt32 seekOrigin, UInt64 *newPosition);

private:
  IO::FileIn m_File;
};

#endif // INPUTSTREAM_H
