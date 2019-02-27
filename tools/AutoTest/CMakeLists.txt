cmake_minimum_required(VERSION 3.5)

set(CMAKE_MODULE_PATH
	${CMAKE_CURRENT_SOURCE_DIR}/../../../Dependencies/cmake-modules
	${CMAKE_MODULE_PATH}
)
include(Header)

project(AutoTest)
if (NOT TARGET PlayCore)
	add_subdirectory(
		${CMAKE_CURRENT_SOURCE_DIR}/../../Source/
		${CMAKE_CURRENT_BINARY_DIR}/Source
	)
endif()

if(TARGET_PLATFORM_WIN32)
	if(NOT TARGET gsh_opengl)
		add_subdirectory(
			${CMAKE_CURRENT_SOURCE_DIR}/../../Source/gs/GSH_OpenGL
			${CMAKE_CURRENT_BINARY_DIR}/gs/GSH_OpenGL
		)
	endif()
	list(APPEND PROJECT_LIBS gsh_opengl)

	if(NOT TARGET gsh_opengl_win32)
		add_library(gsh_opengl_win32 STATIC 
			../../Source/ui_win32/GSH_OpenGL_SettingsWnd.cpp
			../../Source/ui_win32/GSH_OpenGLWin32.cpp
		)
		target_link_libraries(gsh_opengl_win32 Boost::boost glew_s gsh_opengl)
	endif()
	list(APPEND PROJECT_LIBS gsh_opengl_win32)

	if(NOT TARGET gsh_d3d9)
		add_subdirectory(
			${CMAKE_CURRENT_SOURCE_DIR}/../../Source/ui_win32/GSH_Direct3D9
			${CMAKE_CURRENT_BINARY_DIR}/gs/GSH_Direct3D9
		)
	endif()
	list(APPEND PROJECT_LIBS gsh_d3d9)
endif()

add_executable(autotest
	JUnitTestReportWriter.cpp
	Main.cpp
)
target_link_libraries(autotest PlayCore ${PROJECT_LIBS})
