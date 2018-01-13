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
	add_definitions(-D_CRT_SECURE_NO_WARNINGS)
	add_definitions(-D_SCL_SECURE_NO_WARNINGS)
	add_definitions(-D_LIB)
	add_definitions(-D_UNICODE -DUNICODE)

	if(NOT TARGET gsh_opengl)
		add_subdirectory(
			../../Source/gs/GSH_OpenGL
			../../Source/gs/GSH_OpenGL
		)
	endif()
	list(APPEND PROJECT_LIBS gsh_opengl)

	if (NOT TARGET Framework_Win32)
		add_subdirectory(
			${CMAKE_CURRENT_SOURCE_DIR}/../../../Framework/build_cmake/FrameworkWin32
			${CMAKE_CURRENT_BINARY_DIR}/Framework_Win32
		)
	endif()
	list(APPEND PROJECT_LIBS Framework_Win32)

	#If building for Windows XP, we need to use the provided d3dcompiler since it's not available in the SDK
	if(CMAKE_GENERATOR_TOOLSET MATCHES "_xp$")
	include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../../Dependencies/d3dcompiler_winxp/Include)
		if(CMAKE_SIZEOF_VOID_P EQUAL 8)
			list(APPEND PROJECT_LIBS ${CMAKE_CURRENT_SOURCE_DIR}/../../../Dependencies/d3dcompiler_winxp/Lib/x64/d3dcompiler.lib)
		else()
			list(APPEND PROJECT_LIBS ${CMAKE_CURRENT_SOURCE_DIR}/../../../Dependencies/d3dcompiler_winxp/Lib/x86/d3dcompiler.lib)
		endif()
	else()
		list(APPEND PROJECT_LIBS d3dcompiler)
	endif()

list(APPEND PROJECT_LIBS uxtheme opengl32 glu32 dinput8 d3d9 vfw32 winmm gdiplus Boost::date_time)

	if(NOT TARGET gsh_opengl_win32)
		add_library(gsh_opengl_win32 STATIC 
			../../Source/ui_win32/GSH_OpenGL_SettingsWnd.cpp
			../../Source/ui_win32/GSH_OpenGLWin32.cpp
		)
		target_link_libraries(gsh_opengl_win32 Boost::boost glew_s gsh_opengl)
	endif()
	list(APPEND PROJECT_LIBS gsh_opengl_win32)

	if (NOT TARGET Nuanceur)
	add_subdirectory(
		${CMAKE_CURRENT_SOURCE_DIR}/../../../Nuanceur/build_cmake
		${CMAKE_CURRENT_BINARY_DIR}/Nuanceur
	)
	endif()
	list(APPEND PROJECT_LIBS Nuanceur)

	if(NOT TARGET gsh_d3d9)
		add_library(gsh_d3d9 STATIC
			../../Source/ui_win32/GSH_Direct3D9.cpp
			../../Source/ui_win32/GSH_Direct3D9_Shader.cpp
			../../Source/ui_win32/GSH_Direct3D9_Texture.cpp
		)
		target_link_libraries(gsh_d3d9 Boost::boost Nuanceur)
	endif()
	list(APPEND PROJECT_LIBS gsh_d3d9)
endif()

add_executable(autotest
	JUnitTestReportWriter.cpp
	Main.cpp
)
target_link_libraries(autotest PlayCore ${PROJECT_LIBS})
