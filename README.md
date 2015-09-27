# modorganizer-archive

This module provides a wrapper round the 7zip 7z.dll allowing easy(ish) access to the contents of an archive.

AS this is a DLL you need to load it as an archive. 

    Archive* CreateArchive();

This creates an archive handler. The following methods are available:

    bool Archive::isValid() const;
    
Returns false if there was a problem initialising 7zip. This probably means the system can't find 7z.dll or it's corrupt, very old (or possibly too new). You should call this before calling open!

    bool Archive::open(QString const &archiveName, PasswordCallback *passwordCallback)
    
This attempts to open the specified archive file. It should manage to open pretty much anything 7zip recognises. It returns `true` on success.

If passwordCallback is (specified and) not null, it will be called whenever a password is needed when decoding the archive. Note that although some archive formats don't need the password till decoding, some need it straight away. See `extract` for what to do in the callback.  _Note to self: 2nd parameter should be optional_

Once you have opened, the archive, you will want to find out what files are in it, and to extract them.

To find out what files there are, you call

    bool Archive::getFileList(FileData* const *&data, size_t &size)

Supply a reference to a FileData pointer and a reference to a location to contain the number of entries. This will usually return `true` if you succesfully opened the archive. You then manipulate the FileList to control which files to extract and what names to give them.

    Error Archive::getLastError() const

Gets the error code associated with the last error.

    bool Archive::extract(QString const &outputDirectory, ProgressCallback *progressCallback,
                          FileChangeCallback* fileChangeCallback, ErrorCallback* errorCallback)

Extract files from the current archive (see getFileList) into outputDirectory. During the extraction,

* `passwordChangeCallback(QString &)` is called if a password is needed.
* `progressCallback` is called ---
* `fileChangeCallback` is called --- 
* `errorCallback` is called if an error occured. There's not much you can do here beyond displaying an error if you so wish.

Note that if you do not wish to deal with a specific callback, simply pass`nullptr`.

After the extract returns, you may call getFileList again to get a clean copy of the file list so you can extract different files from the archive (this is what the fomod installer does).

    void Archive::cancel();
  
Call this from any of the callbacks to cancel a running extraction. This will cause `extract` to return `false` and `getLastError` to return `ERROR_EXTRACT_CANCELLED`.

    void Archive::close();

Closes the current archive and release any resources associated with it.


class FileData {
public:
  virtual QString getFileName() const = 0;
  virtual void addOutputFileName(QString const &fileName) = 0;
  virtual std::vector<QString> getAndClearOutputFileNames() = 0;
  virtual uint64_t getCRC() const = 0;
  virtual bool isDirectory() const = 0;
};


#endif // ARCHIVE_H

