#-------------------------------------------------
#
# Project created by QtCreator 2011-08-15T12:13:56
#
#-------------------------------------------------

QT       -= gui

#TARGET = archive
TEMPLATE = lib

!include(../LocalPaths.pri) {
  message("paths to required libraries need to be set up in LocalPaths.pri")
}

SOURCES += archive.cpp \
    extractcallback.cpp \
    callback.cpp \
    opencallback.cpp \
    multioutputstream.cpp \
    interfaceguids.cpp \
    propertyvariant.cpp \
    inputstream.cpp

HEADERS += archive.h\
    "$${SEVENZIPPATH}/C/Types.h" \
    "$${SEVENZIPPATH}/CPP/7zip/IDecl.h" \
    "$${SEVENZIPPATH}/CPP/7zip/IPassword.h" \
    "$${SEVENZIPPATH}/CPP/7zip/IStream.h" \
    "$${SEVENZIPPATH}/CPP/7zip/IProgress.h" \
    "$${SEVENZIPPATH}/CPP/7zip/PropID.h" \
    "$${SEVENZIPPATH}/CPP/7zip/Archive/IArchive.h" \
    "$${SEVENZIPPATH}/CPP/Common/MyUnknown.h" \
    "$${SEVENZIPPATH}/CPP/Common/Types.h" \
    extractcallback.h \
    callback.h \
    opencallback.h \
    multioutputstream.h \
    propertyvariant.h \
    inputstream.h \
    unknown_impl.h


OTHER_FILES += \
    version.rc


RC_FILE += \
    version.rc


INCLUDEPATH += "$${SEVENZIPPATH}/CPP"

DEFINES += _UNICODE _WINDLL WIN32 NOMINMAX
LIBS += -lkernel32 -luser32 -loleaut32 -lole32


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
