CMAKE_MINIMUM_REQUIRED (VERSION 2.8)

#SET(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR};${CMAKE_CURRENT_SOURCE_DIR}/CMakePCHCompiler;${CMAKE_MODULE_PATH}")
SET(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR};${CMAKE_MODULE_PATH}")

SET(PROJ_AUTHOR Tannin)
SET(PROJ_ARCH x86)
IF(CMAKE_SIZEOF_VOID_P EQUAL 8)
  SET(PROJ_ARCH x64)
ENDIF()


FIND_PACKAGE(7zip REQUIRED)
FIND_PACKAGE(WindowsSDK REQUIRED)

SET(archive_SRCS
    archive.cpp
    extractcallback.cpp
    callback.cpp
    opencallback.cpp
    multioutputstream.cpp
    interfaceguids.cpp
    propertyvariant.cpp
    inputstream.cpp
  )

SET(archive_HDRS
    archive.h
    callback.h
    extractcallback.h
    opencallback.h
    multioutputstream.h
    propertyvariant.h
    inputstream.h
    unknown_impl.h
    ${SEVENZIP_ROOT_DIR}/C/Types.h
    ${SEVENZIP_ROOT_DIR}/CPP/7zip/IDecl.h
    ${SEVENZIP_ROOT_DIR}/CPP/7zip/IPassword.h
    ${SEVENZIP_ROOT_DIR}/CPP/7zip/IStream.h
    ${SEVENZIP_ROOT_DIR}/CPP/7zip/IProgress.h
    ${SEVENZIP_ROOT_DIR}/CPP/7zip/PropID.h
    ${SEVENZIP_ROOT_DIR}/CPP/7zip/Archive/IArchive.h
    ${SEVENZIP_ROOT_DIR}/CPP/Common/MyUnknown.h
    ${SEVENZIP_ROOT_DIR}/CPP/Common/Types.h
  )

INCLUDE_DIRECTORIES(${SEVENZIP_INCLUDE_DIR})
INCLUDE_DIRECTORIES(${SEVENZIP_INCLUDE_DIR}/CPP)

INCLUDE_DIRECTORIES(${WINDOWSSDK_PREFERRED_DIR}/Include)


ADD_DEFINITIONS(-D_UNICODE -DUNICODE -D_WINDLL -DNOMINMAX)

ADD_LIBRARY(archive SHARED ${archive_HDRS} ${archive_SRCS})

# TODO: shouldn't /DEBUG be set automatically in RELWITHDEBINFO?
SET_PROPERTY(TARGET archive APPEND_STRING PROPERTY LINK_FLAGS_RELWITHDEBINFO "/DEBUG /LTCG /INCREMENTAL:NO /LARGEADDRESSAWARE /OPT:REF /OPT:ICF")

###############
## Installation

INSTALL(TARGETS archive
        RUNTIME DESTINATION bin/dlls
        LIBRARY DESTINATION lib)
INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/archive.pdb DESTINATION pdb)
