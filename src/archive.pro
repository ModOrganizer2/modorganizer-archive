#-------------------------------------------------
#
# Project created by QtCreator 2011-08-15T12:13:56
#
#-------------------------------------------------

QT       -= core gui

#TARGET = archive
TEMPLATE = lib

SOURCES += archive.cpp \
    "$(7ZIPPATH)/CPP/Windows/DLL.cpp" \
    "$(7ZIPPATH)/CPP/Windows/FileIO.cpp" \
    "$(7ZIPPATH)/CPP/7zip/Common/FileStreams.cpp" \
    "$(7ZIPPATH)/CPP/Windows/FileFind.cpp" \
    "$(7ZIPPATH)/CPP/Common/MyVector.cpp" \
    "$(7ZIPPATH)/CPP/Common/MyString.cpp" \
    StdAfx.cpp \
    "$(7ZIPPATH)/CPP/Common/StringConvert.cpp" \
    "$(7ZIPPATH)/CPP/Windows/PropVariantConversions.cpp" \
    "$(7ZIPPATH)/CPP/Windows/PropVariant.cpp" \
    "$(7ZIPPATH)/CPP/Common/IntToString.cpp" \
    "$(7ZIPPATH)/CPP/Windows/FileDir.cpp" \
    "$(7ZIPPATH)/CPP/Windows/FileName.cpp" \
    extractcallback.cpp \
    callback.cpp \
    opencallback.cpp


HEADERS += archive.h\
    "$(7ZIPPATH)/CPP/7zip/Common/FileStreams.h" \
    StdAfx.h \
    "$(7ZIPPATH)/CPP/Common/MyVector.h" \
    "$(7ZIPPATH)/CPP/Common/MyString.h" \
    "$(7ZIPPATH)/CPP/Windows/FileIO.h" \
    "$(7ZIPPATH)/CPP/Windows/FileFind.h" \
    "$(7ZIPPATH)/CPP/Windows/DLL.h" \
    "$(7ZIPPATH)/CPP/Common/StringConvert.h" \
    "$(7ZIPPATH)/CPP/Windows/PropVariantConversions.h" \
    "$(7ZIPPATH)/CPP/Windows/PropVariant.h" \
    "$(7ZIPPATH)/CPP/Common/IntToString.h" \
    "$(7ZIPPATH)/CPP/Common/MyCom.h" \
    "$(7ZIPPATH)/CPP/Windows/FileDir.h" \
    "$(7ZIPPATH)/CPP/Windows/FileName.h" \
    extractcallback.h \
    callback.h \
    opencallback.h


INCLUDEPATH += ../../../7zip/CPP

PRECOMPILED_HEADER = stdafx.h

DEFINES += _UNICODE _WINDLL WIN32

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

OTHER_FILES += \
    version.rc

RC_FILE += \
    version.rc
