#ifndef INPUTSTREAM_H
#define INPUTSTREAM_H

#include "unknown_impl.h"

#include "7zip/IStream.h"

#include <QFile>
class QString;

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

  bool Open(QString const &filename);

  STDMETHOD(Read)(void *data, UInt32 size, UInt32 *processedSize);
  STDMETHOD(Seek)(Int64 offset, UInt32 seekOrigin, UInt64 *newPosition);

private:
  QFile m_File;
};

#endif // INPUTSTREAM_H
