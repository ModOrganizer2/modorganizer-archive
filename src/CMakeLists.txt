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
    callback.cpp
    extractcallback.cpp
    opencallback.cpp
	multioutputstream.cpp
    ${SEVENZIP_ROOT_DIR}/CPP/7zip/Common/FileStreams.cpp
    ${SEVENZIP_ROOT_DIR}/CPP/Common/MyVector.cpp
    ${SEVENZIP_ROOT_DIR}/CPP/Common/MyString.cpp
    ${SEVENZIP_ROOT_DIR}/CPP/Windows/FileIO.cpp
    ${SEVENZIP_ROOT_DIR}/CPP/Windows/FileFind.cpp
    ${SEVENZIP_ROOT_DIR}/CPP/Windows/DLL.cpp
    ${SEVENZIP_ROOT_DIR}/CPP/Common/StringConvert.cpp
    ${SEVENZIP_ROOT_DIR}/CPP/Windows/PropVariant.cpp
    ${SEVENZIP_ROOT_DIR}/CPP/Windows/PropVariantConversions.cpp
    ${SEVENZIP_ROOT_DIR}/CPP/Common/IntToString.cpp
    ${SEVENZIP_ROOT_DIR}/CPP/Windows/FileDir.cpp
    ${SEVENZIP_ROOT_DIR}/CPP/Windows/FileName.cpp
  )

SET(archive_HDRS
    archive.h
    callback.h
    extractcallback.h
    opencallback.h
	multioutputstream.h
    ${SEVENZIP_ROOT_DIR}/CPP/7zip/Common/FileStreams.h
    ${SEVENZIP_ROOT_DIR}/CPP/Common/MyVector.h
    ${SEVENZIP_ROOT_DIR}/CPP/Common/MyString.h
    ${SEVENZIP_ROOT_DIR}/CPP/Windows/FileIO.h
    ${SEVENZIP_ROOT_DIR}/CPP/Windows/FileFind.h
    ${SEVENZIP_ROOT_DIR}/CPP/Windows/DLL.h
    ${SEVENZIP_ROOT_DIR}/CPP/Common/StringConvert.h
    ${SEVENZIP_ROOT_DIR}/CPP/Windows/PropVariant.h
    ${SEVENZIP_ROOT_DIR}/CPP/Windows/PropVariantConversions.h
    ${SEVENZIP_ROOT_DIR}/CPP/Common/IntToString.h
    ${SEVENZIP_ROOT_DIR}/CPP/Common/MyCom.h
    ${SEVENZIP_ROOT_DIR}/CPP/Windows/FileDir.h
    ${SEVENZIP_ROOT_DIR}/CPP/Windows/FileName.h
  )

INCLUDE_DIRECTORIES(${SEVENZIP_INCLUDE_DIR})
INCLUDE_DIRECTORIES(${SEVENZIP_INCLUDE_DIR}/CPP)

INCLUDE_DIRECTORIES(${WINDOWSSDK_PREFERRED_DIR}/Include)


INCLUDE(msvc_pch_support)

ADD_MSVC_PRECOMPILED_HEADER("stdafx.h" "stdafx.cpp" archive_SRCS)


ADD_DEFINITIONS(-D_UNICODE -DUNICODE -D_WINDLL -DNOMINMAX)

ADD_LIBRARY(archive SHARED ${archive_HDRS} ${archive_SRCS})

#TARGET_PRECOMPILED_HEADER(archive StdAfx.h)
#ADD_DEFINITIONS(-Fp${CMAKE_BINARY_DIR}/CMakeFiles/archive.pch.dir/StdAfx.h.pch)

# not sure how to get precompiled headers to work in cmake. Instead use /FI
# to force-include stdafx.h in every compilation unit
SET_PROPERTY(TARGET archive APPEND_STRING PROPERTY COMPILE_FLAGS "/GL /EHsc")

# TODO: shouldn't /DEBUG be set automatically in RELWITHDEBINFO?
SET_PROPERTY(TARGET archive APPEND_STRING PROPERTY LINK_FLAGS_RELWITHDEBINFO "/DEBUG /LTCG /INCREMENTAL:NO /LARGEADDRESSAWARE /OPT:REF /OPT:ICF")

###############
## Installation

INSTALL(TARGETS archive
        RUNTIME DESTINATION bin/dlls
        LIBRARY DESTINATION lib)
INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/archive.pdb DESTINATION pdb)
