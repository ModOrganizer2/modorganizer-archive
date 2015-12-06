# modorganizer-archive

This module provides a wrapper round the 7zip 7z.dll allowing easy(ish) access to the contents of an archive.

AS this is a DLL you need to load it as an archive. 

    Archive* CreateArchive();

This creates an archive handler. The following methods are available:

    bool Archive::isValid() const;
    
Returns false if there was a problem initialising 7zip. This probably means the system can't find 7z.dll or it's corrupt, very old (or possibly too new). You should call this before calling open!

    bool Archive::open(QString const &archiveName, PasswordCallback *passwordCallback)
    
This attempts to open the specified archive file. It should manage to open pretty much anything 7zip recognises. It returns `true` on success.

* `passwordChangeCallback(QString *)` (if not null) is called if a password is needed. Get the password from the user and set the passed in pointer to point to the entered password. Note that this may be called during `extract` rather than during `open`. If you don't supply this callback, archives with passwords will be unreadable. _Note to self: This parameter should be optional_

Once you have opened, the archive, you will want to find out what files are in it, and to extract them.

To find out what files there are, you call

    bool Archive::getFileList(FileData* const *&data, size_t &size)

Supply a reference to a FileData pointer and a reference to a location to contain the number of entries. This will usually return `true` if you succesfully opened the archive. You then manipulate the FileList to control which files to extract and what names to give them.

    Error Archive::getLastError() const

Gets the error code associated with the last error.

    bool Archive::extract(QString const &outputDirectory, ProgressCallback *progressCallback,
                          FileChangeCallback* fileChangeCallback, ErrorCallback* errorCallback)

Extract files from the current archive (see getFileList) into outputDirectory. During the extraction:

* `progressCallback(float)` is called from time to time so you can update a progress meter. The value passed is the proportion of the file processed (i.e. 0 to 1).
* `fileChangeCallback(QString const &)` is called whenever the file being processed is changed. You can use this to update the progress meter.
* `errorCallback(QString const &)` is called if an error occured, with an error message. There's not much you can do here beyond displaying an error if you so wish. This will also result in a failure return from extract.

Note that if you do not wish to deal with a specific callback, simply pass `nullptr`.

After the extract returns, you may call getFileList again to get a clean copy of the file list so you can extract different files from the archive (this is what the fomod installer does).

    void Archive::cancel();
  
Call this from any of the callbacks to cancel a running extraction. This will cause `extract` to return `false` and `getLastError` to return `ERROR_EXTRACT_CANCELLED`.

    void Archive::close();

Closes the current archive and release any resources associated with it.

## The FileData class

As you have seen above, the `getFileList` method returns a pointer to an array of entries about all the files in the archive. Each `FileData` entry supports the following methods

    QString getFileName() const
    
This returns the name of the file in the archive.

    void addOutputFileName(QString const &fileName)

Adds a secondary (or tertiary...) output location for this file. This needs to be specified relative the the `outputDirectory` passed to `extract`.

    std::vector<QString> getAndClearOutputFileNames()

Returns the list of output filenames for this file, then clears it. This is normally used inside the `extract` function.

    uint64_t getCRC() const
    
Returns the CRC stored for the file, in case you wish to check it.

    bool isDirectory() const

Directory entries get stored in the archive as well as normal files.
