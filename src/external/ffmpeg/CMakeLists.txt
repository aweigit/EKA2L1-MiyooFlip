if (NOT DEFINED FFMPEG_CORE_NAME)
	set(FFMPEG_CORE_NAME ffmpeg)
endif()

add_library(${FFMPEG_CORE_NAME} INTERFACE)

target_include_directories(${FFMPEG_CORE_NAME} INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")
if (WIN32)
	if (MINGW)
		target_link_libraries(${FFMPEG_CORE_NAME} INTERFACE
				"${CMAKE_CURRENT_SOURCE_DIR}/windows/x86_64/lib/mingw/libavformat.a"
				"${CMAKE_CURRENT_SOURCE_DIR}/windows/x86_64/lib/mingw/libavcodec.a"
				"${CMAKE_CURRENT_SOURCE_DIR}/windows/x86_64/lib/mingw/libswscale.a"
				"${CMAKE_CURRENT_SOURCE_DIR}/windows/x86_64/lib/mingw/libavutil.a"
				"${CMAKE_CURRENT_SOURCE_DIR}/windows/x86_64/lib/mingw/libswresample.a"
				"Bcrypt.lib"
				"z")
	else()
		target_link_libraries(${FFMPEG_CORE_NAME} INTERFACE
				"${CMAKE_CURRENT_SOURCE_DIR}/windows/x86_64/lib/msvc/avformat.lib"
				"${CMAKE_CURRENT_SOURCE_DIR}/windows/x86_64/lib/msvc/avcodec.lib"
				"${CMAKE_CURRENT_SOURCE_DIR}/windows/x86_64/lib/msvc/swscale.lib"
				"${CMAKE_CURRENT_SOURCE_DIR}/windows/x86_64/lib/msvc/avutil.lib"
				"${CMAKE_CURRENT_SOURCE_DIR}/windows/x86_64/lib/msvc/swresample.lib"
				"Bcrypt.lib")
	endif()
elseif (APPLE)
	target_link_libraries(${FFMPEG_CORE_NAME} INTERFACE
		"${CMAKE_CURRENT_SOURCE_DIR}/macos/x86_64/lib/libavformat.a"
		"${CMAKE_CURRENT_SOURCE_DIR}/macos/x86_64/lib/libavcodec.a"
		"${CMAKE_CURRENT_SOURCE_DIR}/macos/x86_64/lib/libswscale.a"
		"${CMAKE_CURRENT_SOURCE_DIR}/macos/x86_64/lib/libavutil.a"
		"${CMAKE_CURRENT_SOURCE_DIR}/macos/x86_64/lib/libswresample.a"
		"z")
elseif(LINUX_SDL2)
	if (TRIMUI_BRICK OR MIYOO_FLIP)
		target_link_libraries(${FFMPEG_CORE_NAME} INTERFACE
		"${CMAKE_CURRENT_SOURCE_DIR}/linux/aarch64/lib/libavformat.a"
		"${CMAKE_CURRENT_SOURCE_DIR}/linux/aarch64/lib/libavcodec.a"
		"${CMAKE_CURRENT_SOURCE_DIR}/linux/aarch64/lib/libswscale.a"
		"${CMAKE_CURRENT_SOURCE_DIR}/linux/aarch64/lib/libavutil.a"
		"${CMAKE_CURRENT_SOURCE_DIR}/linux/aarch64/lib/libswresample.a"
		"z")
	else()
		target_link_libraries(${FFMPEG_CORE_NAME} INTERFACE
		"${CMAKE_CURRENT_SOURCE_DIR}/linux/x86_64/lib/libavformat.a"
		"${CMAKE_CURRENT_SOURCE_DIR}/linux/x86_64/lib/libavcodec.a"
		"${CMAKE_CURRENT_SOURCE_DIR}/linux/x86_64/lib/libswscale.a"
		"${CMAKE_CURRENT_SOURCE_DIR}/linux/x86_64/lib/libavutil.a"
		"${CMAKE_CURRENT_SOURCE_DIR}/linux/x86_64/lib/libswresample.a"
		"z")
	endif()
elseif (UNIX)
	target_link_libraries(${FFMPEG_CORE_NAME} INTERFACE
		"${CMAKE_CURRENT_SOURCE_DIR}/linux/x86_64/lib/libavformat.a"
		"${CMAKE_CURRENT_SOURCE_DIR}/linux/x86_64/lib/libavcodec.a"
		"${CMAKE_CURRENT_SOURCE_DIR}/linux/x86_64/lib/libswscale.a"
		"${CMAKE_CURRENT_SOURCE_DIR}/linux/x86_64/lib/libavutil.a"
		"${CMAKE_CURRENT_SOURCE_DIR}/linux/x86_64/lib/libswresample.a"
		"z")
endif()