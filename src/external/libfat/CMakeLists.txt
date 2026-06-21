project(FAT)
cmake_minimum_required(VERSION 3.2)

option(BUILD_EXAMPLES "Build the examples project as well" OFF)

add_library(FAT
    include/fat/fat.h
    src/fat.cpp)

target_include_directories(FAT PUBLIC include)

if (BUILD_EXAMPLES)
add_executable(FAT_EXTRACT
    examples/extract.cpp)

target_link_libraries(FAT_EXTRACT PRIVATE FAT)
endif()
