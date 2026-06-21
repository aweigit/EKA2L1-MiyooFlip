# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.8)

if(TRIMUI_BRICK)
        set(MAIN_SRC "main_trimuiBrick.cpp")
        set(MENU_SRC "menu_trimuiBrick.cpp")
        set(GLES2_LIB "EGL" "GLESv2")
elseif(MIYOO_FLIP)
        set(MAIN_SRC "main_mflip.cpp")
        set(MENU_SRC "menu_mflip.cpp")
        set(GLES2_LIB "mali" "drm")
else()
        set(MAIN_SRC "main_ubuntu.cpp")
        set(MENU_SRC "menu_ubuntu.cpp")
        set(GLES2_LIB "EGL" "GLESv2")
endif()

add_library(native-lib STATIC
        include/emu_window_sdl2.h
        include/input_dialog.h
        include/launcher.h
        include/state.h
        include/thread.h
        src/emu_window_sdl2.cpp
        src/input_dialog.cpp
        src/launcher.cpp
        src/state.cpp
        src/thread.cpp
        src/native-lib.cpp)

target_link_libraries(native-lib PRIVATE
        ${GLES2_LIB}
        common
        cpu
        drivers
        glad
        lunasvg
        epoc
        epockern
        epocpkg
        epocservs
        sqlite3
        yaml-cpp)

target_include_directories(native-lib PRIVATE include)
add_dependencies(native-lib
        audiooutputrouting
        avkonfep
        ecam
        scdv
        mediaclientaudio
        mediaclientaudiostream
        mediaclientvideo
        vibractrl
        backlightctrl
        postingsurfacefactory
        goommonitor)
target_link_options(native-lib PRIVATE "-Wl,-Bsymbolic-functions")

add_executable(native
        include/emu_window_sdl2.h
        include/input_dialog.h
        include/launcher.h
        include/state.h
        include/thread.h
        src/${MAIN_SRC}
        src/cJSON.c
)

add_executable(menu
        include/emu_window_sdl2.h
        include/input_dialog.h
        include/launcher.h
        include/state.h
        include/thread.h
        src/${MENU_SRC}
)

target_include_directories(native PRIVATE include)
#target_link_options(native PRIVATE -L${CUSTOM_LINK_PATH} -Wl,-rpath-link=${CUSTOM_LINK_PATH})
target_link_libraries(native PRIVATE
        ${GLES2_LIB}
        native-lib
        SDL2
        common
        cpu
        drivers
        glad
        lunasvg
        epoc
        epockern
        epocpkg
        epocservs
        sqlite3
        yaml-cpp)
add_dependencies(native #定义目标之间依赖关系的命令，确保在构建某个目标之前先构建其依赖的目标
        native-lib
        audiooutputrouting
        avkonfep
        ecam
        scdv
        mediaclientaudio
        mediaclientaudiostream
        mediaclientvideo
        vibractrl
        backlightctrl
        postingsurfacefactory
        goommonitor)
target_compile_options(native PRIVATE 
        -fpermissive
        )


set_target_properties(native PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/
)

target_include_directories(menu PRIVATE include)
#target_link_options(menu PRIVATE -L${CUSTOM_LINK_PATH} -Wl,-rpath-link=${CUSTOM_LINK_PATH})
target_link_libraries(menu PRIVATE
        ${GLES2_LIB}
        native-lib
        SDL2
        SDL2_ttf
        common
        cpu
        drivers
        glad
        lunasvg
        epoc
        epockern
        epocpkg
        epocservs
        sqlite3
        yaml-cpp)
add_dependencies(menu #定义目标之间依赖关系的命令，确保在构建某个目标之前先构建其依赖的目标
        native-lib
        audiooutputrouting
        avkonfep
        ecam
        scdv
        mediaclientaudio
        mediaclientaudiostream
        mediaclientvideo
        vibractrl
        backlightctrl
        postingsurfacefactory
        goommonitor)
target_compile_options(menu PRIVATE 
        -fpermissive
        )


set_target_properties(menu PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/
)

if (ENABLE_SCRIPTING)
    message("Enable scripting on Linux build")
    target_link_libraries(native-lib PRIVATE -Wl,--whole-archive $<TARGET_FILE:scripting> -Wl,--no-whole-archive)
endif()