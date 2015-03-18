#-------------------------------------------------
#
# Project created by QtCreator 2011-08-15T12:13:56
#
#-------------------------------------------------

QT       -= core gui

#TARGET = archive
TEMPLATE = lib

!include(../LocalPaths.pri) {
  message("paths to required libraries need to be set up in LocalPaths.pri")
}


SOURCES += archive.cpp \
    "$${SEVENZIPPATH}/CPP/Windows/DLL.cpp" \
    "$${SEVENZIPPATH}/CPP/Windows/FileIO.cpp" \
    "$${SEVENZIPPATH}/CPP/7zip/Common/FileStreams.cpp" \
    "$${SEVENZIPPATH}/CPP/Windows/FileFind.cpp" \
    "$${SEVENZIPPATH}/CPP/Common/MyVector.cpp" \
    "$${SEVENZIPPATH}/CPP/Common/MyString.cpp" \
    StdAfx.cpp \
    "$${SEVENZIPPATH}/CPP/Common/StringConvert.cpp" \
    "$${SEVENZIPPATH}/CPP/Windows/PropVariantConversions.cpp" \
    "$${SEVENZIPPATH}/CPP/Windows/PropVariant.cpp" \
    "$${SEVENZIPPATH}/CPP/Common/IntToString.cpp" \
    "$${SEVENZIPPATH}/CPP/Windows/FileDir.cpp" \
    "$${SEVENZIPPATH}/CPP/Windows/FileName.cpp" \
    extractcallback.cpp \
    callback.cpp \
    opencallback.cpp


HEADERS += archive.h\
    "$${SEVENZIPPATH}/CPP/7zip/Common/FileStreams.h" \
    StdAfx.h \
    "$${SEVENZIPPATH}/CPP/Common/MyVector.h" \
    "$${SEVENZIPPATH}/CPP/Common/MyString.h" \
    "$${SEVENZIPPATH}/CPP/Windows/FileIO.h" \
    "$${SEVENZIPPATH}/CPP/Windows/FileFind.h" \
    "$${SEVENZIPPATH}/CPP/Windows/DLL.h" \
    "$${SEVENZIPPATH}/CPP/Common/StringConvert.h" \
    "$${SEVENZIPPATH}/CPP/Windows/PropVariantConversions.h" \
    "$${SEVENZIPPATH}/CPP/Windows/PropVariant.h" \
    "$${SEVENZIPPATH}/CPP/Common/IntToString.h" \
    "$${SEVENZIPPATH}/CPP/Common/MyCom.h" \
    "$${SEVENZIPPATH}/CPP/Windows/FileDir.h" \
    "$${SEVENZIPPATH}/CPP/Windows/FileName.h" \
    extractcallback.h \
    callback.h \
    opencallback.h


OTHER_FILES += \
    version.rc


RC_FILE += \
    version.rc


INCLUDEPATH += "$${SEVENZIPPATH}/CPP"

PRECOMPILED_HEADER = stdafx.h

DEFINES += _UNICODE _WINDLL WIN32 NOMINMAX

LIBS += -lkernel32 -luser32 -loleaut32


CONFIG(release, debug|release) {
  QMAKE_CXXFLAGS += /Zi
  QMAKE_LFLAGS += /DEBUG
}

CONFIG(debug, debug|release) {
  SRCDIR = $$OUT_PWD/debug
  DSTDIR = $$PWD/../../outputd
} else {
  SRCDIR = $$OUT_PWD/release
  DSTDIR = $$PWD/../../output
}

SRCDIR ~= s,/,$$QMAKE_DIR_SEP,g
DSTDIR ~= s,/,$$QMAKE_DIR_SEP,g

QMAKE_POST_LINK += xcopy /y /I $$quote($$SRCDIR\\archive*.dll) $$quote($$DSTDIR\\dlls) $$escape_expand(\\n)

OTHER_FILES +=\
    SConscript
