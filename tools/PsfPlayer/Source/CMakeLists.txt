cmake_minimum_required(VERSION 3.5)

set(CMAKE_MODULE_PATH
	${CMAKE_CURRENT_SOURCE_DIR}/../../../../Dependencies/cmake-modules
	${CMAKE_MODULE_PATH}
)
include(Header)

project(PsfCore)

set(PROJECT_LIBS)
set(PSF_PROJECT_NAME "PsfPlayer")
set(PSF_PROJECT_VERSION "1.05")

if(NOT TARGET PlayCore)
	add_subdirectory(
		${CMAKE_CURRENT_SOURCE_DIR}/../../../Source
		${CMAKE_CURRENT_BINARY_DIR}/PlayCore
	)
endif()
list(APPEND PROJECT_LIBS PlayCore)

find_package(OpenAL)
if(OPENAL_FOUND)
	if(TARGET_PLATFORM_MACOS OR TARGET_PLATFORM_IOS OR TARGET_PLATFORM_UNIX OR TARGET_PLATFORM_WIN32)
		if(NOT TARGET Framework_OpenAl)
			add_subdirectory(
				${CMAKE_CURRENT_SOURCE_DIR}/../../../../Framework/build_cmake/FrameworkOpenAl
				${CMAKE_CURRENT_BINARY_DIR}/FrameworkOpenAl
			)
		endif()
		list(APPEND PROJECT_LIBS Framework_OpenAl)
		list(APPEND PROJECT_LIBS ${OPENAL_LIBRARY})
	endif()
endif()

##PsfPlayerCore
set(PSFCORE_SRC_FILES
	AppConfig.cpp
	Iop_PsfSubSystem.cpp
	Playlist.cpp
	PlaylistDiscoveryService.cpp
	ps2/Ps2_PsfDevice.cpp
	ps2/PsfBios.cpp
	PsfArchive.cpp
	PsfBase.cpp
	PsfFs.cpp
	PsfLoader.cpp
	PsfPathToken.cpp
	#PsfRarArchive.cpp
	PsfStreamProvider.cpp
	PsfTags.cpp
	PsfVm.cpp
	PsfZipArchive.cpp
	psp/MA_ALLEGREX.cpp
	psp/MA_ALLEGREX_Reflection.cpp
	psp/Psp_Audio.cpp
	psp/Psp_IoFileMgrForUser.cpp
	psp/Psp_KernelLibrary.cpp
	psp/Psp_PsfBios.cpp
	psp/Psp_PsfDevice.cpp
	psp/Psp_PsfSubSystem.cpp
	psp/Psp_SasCore.cpp
	psp/Psp_StdioForUser.cpp
	psp/Psp_SysMemUserForUser.cpp
	psp/Psp_ThreadManForUser.cpp
	psp/PspBios.cpp
	psx/PsxBios.cpp
)

if(TARGET_PLATFORM_MACOS)
	set(PSFCORE_PLATFORM_SPECIFIC_SRC_FILES macosx_ui/PsfPlayer_Prefix.pch)
endif()

add_library(PsfCore STATIC ${PSFCORE_SRC_FILES} ${PSFCORE_PLATFORM_SPECIFIC_SRC_FILES})
target_link_libraries(PsfCore Boost::boost Boost::system Boost::filesystem ${PROJECT_LIBS})
target_include_directories(PsfCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
