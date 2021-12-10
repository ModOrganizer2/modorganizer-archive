cmake_minimum_required(VERSION 3.16)

project(archive)

if(NOT DEFINED DEPENDENCIES_DIR)
	set(LOCAL_DEPENDENCIES_DIR ${CMAKE_SOURCE_DIR}}/../..)
else()
    set(LOCAL_DEPENDENCIES_DIR ${DEPENDENCIES_DIR})
endif()

if(EXISTS ${LOCAL_DEPENDENCIES_DIR}/modorganizer_super/cmake_common)
	set(project_type dll)
	set(enable_warnings OFF)
	include(${DEPENDENCIES_DIR}/modorganizer_super/cmake_common/project.cmake)
else()
	include(FetchContent)

	set(FETCHCONTENT_QUIET FALSE)

	FetchContent_Declare(
		sevenz
		URL https://www.7-zip.org/a/7z1900-src.7z
	)
	FetchContent_Declare(
		fmtlib
		GIT_REPOSITORY https://github.com/fmtlib/fmt.git
		GIT_TAG        6.2.1
	    GIT_PROGRESS   TRUE
	)
	FetchContent_MakeAvailable(fmtlib sevenz)
	FetchContent_GetProperties(sevenz
		SOURCE_DIR SEVENZ_ROOT
	)

endif()

add_subdirectory(src)