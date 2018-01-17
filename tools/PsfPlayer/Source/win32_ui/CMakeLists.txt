cmake_minimum_required(VERSION 3.5)

set(CMAKE_MODULE_PATH
	${CMAKE_CURRENT_SOURCE_DIR}/../../../../../Dependencies/cmake-modules
	${CMAKE_MODULE_PATH}
)
include(Header)

project(PsfPlayer)

add_definitions(-D_CRT_SECURE_NO_WARNINGS)
add_definitions(-D_SCL_SECURE_NO_WARNINGS)
add_definitions(-D_LIB)
add_definitions(-D_UNICODE -DUNICODE)

if (NOT TARGET PsfCore)
	add_subdirectory(
		${CMAKE_CURRENT_SOURCE_DIR}/../
		${CMAKE_CURRENT_BINARY_DIR}/PsfCore
	)
endif()
list(APPEND PROJECT_LIBS PsfCore)

if (NOT TARGET Framework_Win32)
	add_subdirectory(
		${CMAKE_CURRENT_SOURCE_DIR}/../../../../../Framework/build_cmake/FrameworkWin32
		${CMAKE_CURRENT_BINARY_DIR}/Framework_Win32
	)
endif()
list(APPEND PROJECT_LIBS Framework_Win32)

set(AUDIO_LIBS)
find_package(OpenAL)
if(OPENAL_FOUND)
	add_library(SH_OpenAL SHARED ../SH_OpenAL.cpp SH_OpenAL/DllMain.cpp)
	target_link_libraries(SH_OpenAL PRIVATE ${OPENAL_LIBRARY} Framework_OpenAl Framework)
	list(APPEND AUDIO_LIBS SH_OpenAL)
endif()

find_package(XAudio)
if(XAudio_FOUND)
	add_library(SH_XAudio2 SHARED SH_XAudio2.cpp SH_XAudio2/DllMain.cpp)
	target_link_libraries(SH_XAudio2 PRIVATE ${XAudio_LIBRARY} Framework)
	target_include_directories(SH_XAudio2 PRIVATE ${XAudio_INCLUDE_DIR})
	list(APPEND AUDIO_LIBS SH_XAudio2)
endif()

add_library(SH_WaveOut SHARED SH_WaveOut.cpp SH_WaveOut/DllMain.cpp)
target_link_libraries(SH_WaveOut PRIVATE "winmm.lib" Framework)
list(APPEND AUDIO_LIBS SH_WaveOut)

ENABLE_LANGUAGE(ASM_MASM)

if (CMAKE_BUILD_TYPE MATCHES DEBUG)
	set(DEBUG_SRC
		../../../../Source/ui_win32/CallStackWnd.cpp
		../../../../Source/ui_win32/Debugger.cpp
		../../../../Source/ui_win32/DebugView.cpp
		../../../../Source/ui_win32/FunctionsView.cpp
		../../../../Source/ui_win32/ThreadsViewWnd.cpp
	)
endif()

if(TARGET_PLATFORM_WIN32_X64)
	set(MANIFEST Play-x64.manifest)
else()
	set(MANIFEST Play-x86.manifest)
endif()

set(WIN32_SRC
	../../../../Source/ui_win32/DirectXControl.cpp
	AboutWindow.cpp
	AcceleratorTable.cpp
	FileInformationPanel.cpp
	Main.cpp
	#Main_Aot.cpp
	MainWindow.cpp
	#MiniDebugger.cpp
	PlaylistPanel.cpp
	SpuRegView.cpp
	SpuRegViewPanel.cpp
	StdAfx.cpp
)

add_executable(PsfPlayer WIN32 ${WIN32_SRC} Res.rc)
target_link_libraries(PsfPlayer PUBLIC d3d9 uxtheme Comctl32 ${PROJECT_LIBS} ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../Dependencies/tdemu-1.10/TDEmu64.lib ${AUDIO_LIBS})
target_include_directories(PsfPlayer
	PUBLIC
		./
		../../../../Source/ui_win32
		../../../../Source/gs/GSH_OpenGL
		${CMAKE_CURRENT_SOURCE_DIR}/../../../../../Dependencies/tdemu-1.10
)
include(PrecompiledHeader)
add_precompiled_header(PsfPlayer StdAfx.h FORCEINCLUDE SOURCE_CXX StdAfx.cpp)
