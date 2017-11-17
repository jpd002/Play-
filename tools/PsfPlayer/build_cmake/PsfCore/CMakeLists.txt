cmake_minimum_required(VERSION 2.8)

if(CMAKE_CURRENT_SOURCE_DIR STREQUAL "${CMAKE_SOURCE_DIR}")
	option(TARGET_IOS "Enable building for iOS" OFF)

	# macOS deployment target needs to be set before 'project' to work
	if(APPLE AND NOT TARGET_IOS)
		set(CMAKE_OSX_DEPLOYMENT_TARGET "10.9" CACHE STRING "Minimum OS X deployment version")
	endif()

	if(ANDROID)
		message("-- Generating for Android --")
		set(TARGET_PLATFORM_ANDROID TRUE)
	elseif(APPLE AND TARGET_IOS)
		message("-- Generating for iOS --")
		set(TARGET_PLATFORM_IOS TRUE)
	elseif(APPLE)
		message("-- Generating for macOS --")
		set(TARGET_PLATFORM_MACOS TRUE)
	elseif(WIN32)
		message("-- Generating for Win32 --")
		string(FIND ${CMAKE_GENERATOR} "Win64" HASWIN64)
		if(NOT HASWIN64 EQUAL -1)
			message("-- Arch: x64 --")
			set(TARGET_PLATFORM_WIN32_X64 TRUE)
		else()
			message("-- Arch: x86 --")
			set(TARGET_PLATFORM_WIN32_X86 TRUE)
		endif()
		set(TARGET_PLATFORM_WIN32 TRUE)
	else()
		message("-- Generating for Unix compatible platform --")
		set(TARGET_PLATFORM_UNIX TRUE)
	endif()
endif()

project(PsfPlayer)
set(PROJECT_LIBS)
set(PSF_PROJECT_NAME "PsfPlayer")
set(PSF_PROJECT_VERSION "1.05")

set(CMAKE_MODULE_PATH
	${CMAKE_CURRENT_SOURCE_DIR}/../../../../../Dependencies/cmake-modules
	${CMAKE_MODULE_PATH}
)

# flags
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(WIN32)
	set(CMAKE_CXX_FLAGS "/EHsc")
endif()

# definitions
if(NOT MSVC)
	MESSAGE("-- Build type: ${CMAKE_BUILD_TYPE}")
	if(CMAKE_BUILD_TYPE STREQUAL "Release")
		add_definitions(-DNDEBUG)
	else()
		add_definitions(-D_DEBUG)
	endif()
endif()

if(WIN32)
	add_definitions(-D_CRT_SECURE_NO_WARNINGS)
	add_definitions(-D_SCL_SECURE_NO_WARNINGS)
	add_definitions(-D_LIB)
	add_definitions(-D_UNICODE -DUNICODE)
endif()

add_definitions(-D_IOP_EMULATE_MODULES)

if (NOT TARGET PlayCore)
	add_subdirectory(
		${CMAKE_CURRENT_SOURCE_DIR}/../../../../build_cmake/PlayCore
		${CMAKE_CURRENT_BINARY_DIR}/PlayCore
	)
endif()
list(APPEND PROJECT_LIBS PlayCore)

find_package(ZLIB)
if(NOT ZLIB_FOUND)
	MESSAGE("-- Using Provided zlib source")
	if(NOT TARGET zlibstatic)
		add_subdirectory(
			${CMAKE_CURRENT_SOURCE_DIR}/../../../../../Dependencies/build_cmake/zlib-1.2.8
			${CMAKE_CURRENT_BINARY_DIR}/zlib-1.2.8
		)
	endif()
	list(APPEND PROJECT_LIBS zlibstatic)
else()
	include_directories(${ZLIB_INCLUDE_DIRS})
	list(APPEND PROJECT_LIBS ${ZLIB_LIBRARIES})
endif()

if(NOT TARGET Boost::boost)
	add_subdirectory(
		${CMAKE_CURRENT_SOURCE_DIR}/../../../../../Dependencies/boost-cmake
		${CMAKE_CURRENT_BINARY_DIR}/boost-cmake
	)
endif()

##PsfPlayerCore
set(PSFCORE_SRC_FILES
	../../Source/AppConfig.cpp
	../../Source/Iop_PsfSubSystem.cpp
	../../Source/Playlist.cpp
	../../Source/PlaylistDiscoveryService.cpp
	../../Source/ps2/Ps2_PsfDevice.cpp
	../../Source/ps2/PsfBios.cpp
	../../Source/PsfArchive.cpp
	../../Source/PsfBase.cpp
	../../Source/PsfFs.cpp
	../../Source/PsfLoader.cpp
	../../Source/PsfPathToken.cpp
	#../../Source/PsfRarArchive.cpp
	../../Source/PsfStreamProvider.cpp
	../../Source/PsfTags.cpp
	../../Source/PsfVm.cpp
	../../Source/PsfZipArchive.cpp
	../../Source/psp/MA_ALLEGREX.cpp
	../../Source/psp/MA_ALLEGREX_Reflection.cpp
	../../Source/psp/Psp_Audio.cpp
	../../Source/psp/Psp_IoFileMgrForUser.cpp
	../../Source/psp/Psp_KernelLibrary.cpp
	../../Source/psp/Psp_PsfBios.cpp
	../../Source/psp/Psp_PsfDevice.cpp
	../../Source/psp/Psp_PsfSubSystem.cpp
	../../Source/psp/Psp_SasCore.cpp
	../../Source/psp/Psp_StdioForUser.cpp
	../../Source/psp/Psp_SysMemUserForUser.cpp
	../../Source/psp/Psp_ThreadManForUser.cpp
	../../Source/psp/PspBios.cpp
	../../Source/psx/PsxBios.cpp
)

if(TARGET_PLATFORM_MACOS)
	set(PSFCORE_PLATFORM_SPECIFIC_SRC_FILES ../../Source/macosx_ui/PsfPlayer_Prefix.pch)
endif()

add_library(PsfCore STATIC ${PSFCORE_SRC_FILES} ${PSFCORE_PLATFORM_SPECIFIC_SRC_FILES})
target_link_libraries(PsfCore Boost::boost Boost::system Boost::filesystem ${PROJECT_LIBS})
target_include_directories(PsfCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../../Source)
