cmake_minimum_required(VERSION 3.5)

set(CMAKE_MODULE_PATH
	${CMAKE_CURRENT_SOURCE_DIR}/../../../../Dependencies/cmake-modules
	${CMAKE_MODULE_PATH}
)

include(Header)

project(GSH_OpenGLWin32)

if(NOT TARGET Framework_Win32)
	add_subdirectory(
		${CMAKE_CURRENT_SOURCE_DIR}/../../../../Framework/build_cmake/FrameworkWin32
		${CMAKE_CURRENT_BINARY_DIR}/Framework_Win32
	)
endif()
list(APPEND gsh_opengl_win32_PROJECT_LIBS Framework_Win32)

if(NOT TARGET gsh_opengl)
	add_subdirectory(
		${CMAKE_CURRENT_SOURCE_DIR}/../GSH_OpenGL
		${CMAKE_CURRENT_BINARY_DIR}/gsh_opengl
	)
endif()
list(APPEND gsh_opengl_win32_PROJECT_LIBS gsh_opengl)

add_library(gsh_opengl_win32 STATIC 
	GSH_OpenGLWin32.cpp
	GSH_OpenGLWin32.h
)

target_link_libraries(gsh_opengl_win32 ${gsh_opengl_win32_PROJECT_LIBS})
