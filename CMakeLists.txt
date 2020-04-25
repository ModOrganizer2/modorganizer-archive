cmake_minimum_required(VERSION 3.16)

project(archive)
set(project_type dll)
set(enable_warnings OFF)

include(../cmake_common/project.cmake)
add_subdirectory(src)
