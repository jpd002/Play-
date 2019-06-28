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

if(NOT TARGET_PLATFORM_UNIX_AARCH64 AND (TARGET_PLATFORM_UNIX OR TARGET_PLATFORM_WIN32))
	if (NOT TARGET glew_s)
		add_subdirectory(
			${CMAKE_CURRENT_SOURCE_DIR}/../../../Dependencies/glew-2.0.0
			${CMAKE_CURRENT_BINARY_DIR}/glew-2.0.0
		)
	endif()
	list(APPEND PROJECT_LIBS glew_s)
endif()

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

target_link_libraries(play_libretro ${PROJECT_LIBS})
set_target_properties(play_libretro PROPERTIES PREFIX "")
