cmake_minimum_required(VERSION 3.5)

# macOS deployment target needs to be set before 'project' to work
if(APPLE AND NOT TARGET_IOS)
	set(CMAKE_OSX_DEPLOYMENT_TARGET "10.9" CACHE STRING "Minimum OS X deployment version")
endif()

project(Play)

set(CMAKE_MODULE_PATH
	${CMAKE_CURRENT_SOURCE_DIR}/../Dependencies/cmake-modules
	${CMAKE_MODULE_PATH}
)
include(Header)

set(BUILD_PLAY ON CACHE BOOL "Build Play! Emulator")
set(BUILD_PSFPLAYER OFF CACHE BOOL "Build PsfPlayer")
set(BUILD_TESTS ON CACHE BOOL "Build Tests")
set(USE_AOT_CACHE OFF CACHE BOOL "Use AOT block cache")

set(PROJECT_NAME "Play!")
set(PROJECT_Version 0.30)
add_definitions(-DPLAY_VERSION="${PROJECT_Version}")
set(PROJECT_LIBS)

include(PrecompiledHeader)

if(BUILD_PLAY)
	if(TARGET_PLATFORM_UNIX OR TARGET_PLATFORM_MACOS OR USE_QT)
		add_subdirectory(Source/ui_qt/)
	endif(TARGET_PLATFORM_UNIX OR TARGET_PLATFORM_MACOS OR USE_QT)

	if(NOT USE_QT)
		if(TARGET_PLATFORM_IOS)
			add_subdirectory(Source/ui_ios/)
		endif(TARGET_PLATFORM_IOS)

		if(TARGET_PLATFORM_ANDROID)
			add_subdirectory(Source/ui_android/)
		endif(TARGET_PLATFORM_ANDROID)

		if(TARGET_PLATFORM_WIN32)
			add_subdirectory(Source/ui_win32)
		endif(TARGET_PLATFORM_WIN32)
	endif()
endif(BUILD_PLAY)

if(BUILD_TESTS)
	enable_testing()

	add_subdirectory(tools/AutoTest/)
	add_subdirectory(tools/McServTest/)
	add_subdirectory(tools/VuTest/)
endif()

if(BUILD_PSFPLAYER)
	add_subdirectory(tools/PsfPlayer)
endif(BUILD_PSFPLAYER)
