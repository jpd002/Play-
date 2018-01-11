cmake_minimum_required(VERSION 3.5)

set(CMAKE_MODULE_PATH
	${CMAKE_CURRENT_SOURCE_DIR}/../Dependencies/cmake-modules
	${CMAKE_MODULE_PATH}
)
include(Header)

set(BUILD_PSFPLAYER OFF CACHE BOOL "Build PsfPlayer")
set(BUILD_PLAY ON CACHE BOOL "Build Play! Emulator")

set(PROJECT_NAME "Play!")
set(PROJECT_Version 0.30)
add_definitions(-DPLAY_VERSION="${PROJECT_Version}")
set(PROJECT_LIBS)


include(PrecompiledHeader)

if(DEBUGGER_INCLUDED)
	add_definitions(-DDEBUGGER_INCLUDED)
endif()

#TODO figure out if these go into Core or UI
if(PROFILE)
	add_definitions(-DPROFILE)
endif()

add_definitions(-D_IOP_EMULATE_MODULES)
add_definitions(-DGLEW_STATIC)

if(WIN32)
	add_definitions(-D_CRT_SECURE_NO_WARNINGS)
	add_definitions(-D_SCL_SECURE_NO_WARNINGS)
	add_definitions(-D_LIB)
	add_definitions(-D_UNICODE -DUNICODE)
	if(DEFINED VTUNE_ENABLED)
		add_definitions(-DVTUNE_ENABLED)
		list(APPEND PROJECT_LIBS libittnotify jitprofiling)
		if(DEFINED VTUNE_PATH)
			if(TARGET_PLATFORM_WIN32_X86)
				link_directories($(VTUNE_PATH)\lib32)
			else()
				link_directories($(VTUNE_PATH)\lib64)
			endif()
			include_directories($(VTUNE_PATH)\include)
		else()
			MESSAGE(FATAL_ERROR "VTUNE_PATH was not defined")
		endif()
	endif()
endif()

if(BUILD_PLAY)
	if(TARGET_PLATFORM_UNIX)
		add_subdirectory(Source/ui_unix/)
	endif(TARGET_PLATFORM_UNIX)

	if(TARGET_PLATFORM_MACOS)
		add_subdirectory(Source/ui_macosx/)
	endif(TARGET_PLATFORM_MACOS)

	if(TARGET_PLATFORM_IOS)
		add_subdirectory(Source/ui_ios/)
	endif(TARGET_PLATFORM_IOS)

	if(TARGET_PLATFORM_ANDROID)
		add_subdirectory(Source/ui_android/)
	endif(TARGET_PLATFORM_ANDROID)

	if(TARGET_PLATFORM_WIN32)
		add_subdirectory(Source/ui_win32)
	endif(TARGET_PLATFORM_WIN32)
endif(BUILD_PLAY)

if(BUILD_PSFPLAYER)
	add_subdirectory(tools/PsfPlayer)
endif(BUILD_PSFPLAYER)
