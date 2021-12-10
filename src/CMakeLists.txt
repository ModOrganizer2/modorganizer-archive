cmake_minimum_required(VERSION 3.16)
# appveyor does not build modorganizer in its standard location, so use
# DEPENDENCIES_DIR to find cmake_common
if(EXISTS ${LOCAL_DEPENDENCIES_DIR}/modorganizer_super/cmake_common)
	include(${LOCAL_DEPENDENCIES_DIR}/modorganizer_super/cmake_common/src.cmake)
	add_definitions(-DMODORGANIZER_ARCHIVE_BUILDING)
else()
	file(GLOB_RECURSE source_files CONFIGURE_DEPENDS *.cpp)
	file(GLOB_RECURSE header_files CONFIGURE_DEPENDS *.h)

	set(input_files ${source_files} ${header_files})
	source_group(src REGULAR_EXPRESSION ".*\\.(h|cpp|ui)")

	add_definitions("-DDLLEXPORT=_declspec(dllexport)")
	set(COMPILE_FLAGS "/std:c++latest /MP")
	set(OPTIMIZE_COMPILE_FLAGS "/O2")
	set(OPTIMIZE_LINK_FLAGS "/LTCG /INCREMENTAL:NO /OPT:REF /OPT:ICF")
	set(COMPILE_FLAGS "${COMPILE_FLAGS} /W2 /wd4464")

	add_library(${PROJECT_NAME} SHARED ${input_files})

	target_include_directories(${PROJECT_NAME} PRIVATE ${SEVENZ_ROOT}/CPP)
	target_link_libraries(${PROJECT_NAME} PRIVATE fmt::fmt)

	set_property(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" PROPERTY
		VS_STARTUP_PROJECT ${PROJECT_NAME})

	set_target_properties(${PROJECT_NAME} PROPERTIES
		COMPILE_FLAGS "${COMPILE_FLAGS}"
		COMPILE_FLAGS_RELWITHDEBINFO "${OPTIMIZE_COMPILE_FLAGS}"
		LINK_FLAGS_RELWITHDEBINFO "${OPTIMIZE_LINK_FLAGS}")
endif()
