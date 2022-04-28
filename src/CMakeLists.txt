cmake_minimum_required(VERSION 3.16)

add_library(archive SHARED)
mo2_configure_library(archive
    WARNINGS OFF PRIVATE_DEPENDS 7z fmt)
target_compile_definitions(archive PRIVATE -DMODORGANIZER_ARCHIVE_BUILDING)
mo2_install_target(archive)
