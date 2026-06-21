cmake_minimum_required(VERSION 3.8)

project (EKA2L1)

message(STATUS "CMAKE_SYSTEM_NAME: ${CMAKE_SYSTEM_NAME}")

if(NOT DEFINED CUSTOM_LINK_PATH)
    message(FATAL_ERROR  "先设置链接库路径CUSTOM_LINK_PATH.例如:/usr/lib/x86_64-linux-gnu")
    return()
endif()

if(MIYOO_FLIP)
    add_compile_options(-fPIC -DSDL_DISABLE_IMMINTRIN_H -march=armv8.2-a+crc+fp16 -mcpu=cortex-a55 -mtune=cortex-a55 -Ofast)
elseif(TRIMUI_BRICK)
    add_compile_options(-fPIC -DSDL_DISABLE_IMMINTRIN_H -march=armv8-a+crc -mcpu=cortex-a53 -mtune=cortex-a53 -Ofast)
else()
    add_compile_options(-fPIC -DSDL_VIDEO_DRIVER_X11 -Ofast)
endif()

link_directories(${CUSTOM_LINK_PATH})
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-rpath-link=${CUSTOM_LINK_PATH}")

# Clang 14 is required to compile nicely with C++ 20
set (CMAKE_CXX_STANDARD 20)

option(EKA2L1_BUILD_TOOLS "Build EKA2L1's tool" OFF)
option(EKA2L1_BUILD_TESTS "Build EKA2L1's tests" OFF)
option(EKA2L1_ENABLE_SCRIPTING_ABILITY "Enable to script with Python or Lua" OFF)
option(EKA2L1_ENABLE_UNEXPECTED_EXCEPTION_HANDLER "Enable EKA2L1 to dump unexpected exception" OFF)
option(EKA2L1_BUILD_VULKAN_BACKEND "Build Vulkan backend" OFF)
option(EKA2L1_DEPLOY_DMG "Deploy EKA2L1 as .dmg" OFF)
option(EKA2L1_BUILD_PATCH "Enable building Symbian's DLL patches using Symbian SDK" OFF)
option(EKA2L1_ENABLE_DISCORD_RICH_PRESENCE "Enable support for Discord Rich Presence" OFF)

set (CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
set (ROOT ${CMAKE_CURRENT_SOURCE_DIR})

find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
    set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")

    message("Using CCache for building")
endif()

if (MSVC)#当CMake检测到当前使用的编译器是微软的Visual C++编译器（如cl.exe）时，会自动定义 MSVC 宏。
    add_definitions(-D_CRT_SECURE_NO_DEPRECATE)
    add_definitions(-D_CRT_NONSTDC_NO_DEPRECATE)
    add_definitions(-D_DISABLE_VECTOR_ANNOTATION)
endif(MSVC)

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options(-Wno-error)
endif()

if (EKA2L1_BUILD_VULKAN_BACKEND)
    set (BUILD_WITH_VULKAN 1)
else()
    set (BUILD_WITH_VULKAN 0)
endif()

if (EKA2L1_ENABLE_SCRIPTING_ABILITY)
    set (ENABLE_SCRIPTING 1)
endif(EKA2L1_ENABLE_SCRIPTING_ABILITY)

if (CI)
    set (BUILD_FOR_USER 1)
endif (CI)

set (ENABLE_SEH_HANDLER 0)

if (EKA2L1_ENABLE_UNEXPECTED_EXCEPTION_HANDLER)
    set(ENABLE_SEH_HANDLER 1)
endif (EKA2L1_ENABLE_UNEXPECTED_EXCEPTION_HANDLER)

if (EKA2L1_ENABLE_DISCORD_RICH_PRESENCE AND NOT LINUX_SDL2 AND NOT (CMAKE_CXX_COMPILER_ID STREQUAL "GNU"))
    set(ENABLE_DISCORD_RICH_PRESENCE 1)
else()
    set(ENABLE_DISCORD_RICH_PRESENCE 0)
endif()

add_subdirectory(src/patch)
add_subdirectory(src/external)
add_subdirectory(src/emu)

if (EKA2L1_BUILD_TOOLS)
    add_subdirectory(src/tools)
endif()

if (EKA2L1_BUILD_TESTS)
    enable_testing()
    add_subdirectory(src/tests)
endif()
