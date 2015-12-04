#ifndef MULTIOUTPUTSTREAM_H
#define MULTIOUTPUTSTREAM_H

#include "unknown_impl.h"

#include "7zip/IStream.h"

class QFile;
class QString;

#include <memory>
#include <vector>

/** This class allows you to open and output to multiple file handles at a time.
 * It implements the ISequentalOutputStream interface and has some extra functions
 * which are used by the CArchiveExtractCallback class to basically open and
 * set the timestamp on all the files.
 *
 * Note that the handling on errors could be better.
 */
class MultiOutputStream :
    public ISequentialOutStream
{

  UNKNOWN_1_INTERFACE(ISequentialOutStream);

public:
  MultiOutputStream();

  virtual ~MultiOutputStream();

  /** Opens the supplied files.
   *
   * @returns true if all went OK, false if any file failed to open
   */
  bool Open(std::vector<QString> const &fileNames);

  /** Closes all the files opened by the last open
   *
   * Note if there are any errors, the code will merely report the last one.
   */
  HRESULT Close();

  /** Sets the modification time on the open files
   *
   * @returns true if all files had the time set succesfully, false otherwise
   * note that this will give up as soon as it gets an error
   */
  bool SetMTime(FILETIME const *mTime);

  // ISequentialOutStream interface

  /** Write data to all the streams
   *
   * The processedSize returned will be the same as size, unless
   * there was an error, in which case it might or might not be different.
   * @warn If an error happens, the code will not attempt any further writing,
   * so some files might not get written to at all
   */
  STDMETHOD(Write)(const void *data, UInt32 size, UInt32 *processedSize);

private:
  /** This is the amount of data written to *any one* file.
   *
   * If there are errors writing to one of the files, this might or might
   * not match what was actually written to another of the files
   */
  UInt64 m_ProcessedSize;

  typedef std::unique_ptr<QFile> OutFile;

  /** All the files opened for this 'stream'
   *
   */
  std::vector<OutFile> m_Handles;

};

#endif // MULTIOUTPUTSTREAM_H
