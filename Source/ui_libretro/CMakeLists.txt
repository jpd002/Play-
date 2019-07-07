cmake_minimum_required(VERSION 3.5)

set(CMAKE_MODULE_PATH
	${CMAKE_CURRENT_SOURCE_DIR}/../../../Dependencies/cmake-modules
	${CMAKE_MODULE_PATH}
)
include(Header)

project(Play_Libretro_Core)

add_definitions(-DPLAY_VERSION="${PROJECT_Version}")

if(NOT TARGET PlayCore)
	add_subdirectory(
		${CMAKE_CURRENT_SOURCE_DIR}/../
		${CMAKE_CURRENT_BINARY_DIR}/Source
	)
endif()
list(APPEND PROJECT_LIBS PlayCore)

if(NOT TARGET gsh_opengl)
	add_subdirectory(
		${CMAKE_CURRENT_SOURCE_DIR}/../gs/GSH_OpenGL
		${CMAKE_CURRENT_BINARY_DIR}/gs/GSH_OpenGL
	)
endif()
list(INSERT PROJECT_LIBS 0 gsh_opengl)

set(SRC
	main_libretro.cpp
	GSH_OpenGL_Libretro.cpp
	GSH_OpenGL_Libretro.h
	PH_Libretro_Input.cpp
	PH_Libretro_Input.h
	SH_LibreAudio.cpp
	SH_LibreAudio.h
)

if(TARGET_PLATFORM_ANDROID)
	list(APPEND PROJECT_LIBS android log GLESv3 EGL)
elseif(TARGET_PLATFORM_IOS)
	list(APPEND PROJECT_LIBS "-ObjC -lsqlite3 -framework OpenGLES")
endif()

add_library(play_libretro SHARED ${SRC})
target_include_directories(play_libretro PRIVATE
	./
	../../
	${CMAKE_CURRENT_BINARY_DIR}
)

if(TARGET_PLATFORM_ANDROID)
	set_target_properties(play_libretro PROPERTIES SUFFIX "_android.so")
endif()
if(TARGET_PLATFORM_IOS)
	set_target_properties(play_libretro PROPERTIES SUFFIX "_ios.dylib")
endif()

target_link_libraries(play_libretro ${PROJECT_LIBS})
set_target_properties(play_libretro PROPERTIES PREFIX "")
