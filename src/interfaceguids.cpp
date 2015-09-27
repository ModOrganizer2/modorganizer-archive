//This file instantiates the GUIDs needed for linking with the 7zip code
#include <initguid.h>

//extractcallback, opencallback
#include "7zip/IPassword.h"

//archive, opencallback
//Note: In a sense, this is unnecessary as IArchive.h includes it
#include "7zip/IStream.h"

//archive, opencallback
#include "7zip/Archive/IArchive.h"
