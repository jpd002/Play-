cmake_minimum_required(VERSION 3.5)

set(CMAKE_MODULE_PATH
	${CMAKE_CURRENT_SOURCE_DIR}/../../../deps/Dependencies/cmake-modules
	${CMAKE_MODULE_PATH}
)
include(Header)

project(PsfCore)

set(PROJECT_LIBS)
set(PSF_PROJECT_NAME "PsfPlayer")
set(PSF_PROJECT_VERSION "1.05")
set(PSF_DEFINITIONS)
set(PSF_RAR_SUPPORT ON)

if(NOT TARGET PlayCore)
	add_subdirectory(
		${CMAKE_CURRENT_SOURCE_DIR}/../../../Source
		${CMAKE_CURRENT_BINARY_DIR}/PlayCore
	)
endif()
list(APPEND PROJECT_LIBS PlayCore)

if(PSF_RAR_SUPPORT)
	if(NOT TARGET unrar)
		add_subdirectory(
			${CMAKE_CURRENT_SOURCE_DIR}/../../../deps/Dependencies/build_cmake/unrarsrc-5.2.5
			${CMAKE_CURRENT_BINARY_DIR}/unrarsrc-5.2.5
		)
	endif()
	list(APPEND PROJECT_LIBS unrar)
endif()

##PsfPlayerCore
set(PSFCORE_SRC_FILES
	AppConfig.cpp
	AppConfig.h
	Iop_PsfSubSystem.cpp
	Iop_PsfSubSystem.h
	PlaybackController.cpp
	PlaybackController.h
	Playlist.cpp
	Playlist.h
	PlaylistDiscoveryService.cpp
	PlaylistDiscoveryService.h
	ps2/Ps2_PsfDevice.cpp
	ps2/Ps2_PsfDevice.h
	PsfArchive.cpp
	PsfArchive.h
	PsfBase.cpp
	PsfBase.h
	PsfFs.cpp
	PsfFs.h
	PsfLoader.cpp
	PsfLoader.h
	PsfPathToken.cpp
	PsfPathToken.h
	PsfStreamProvider.cpp
	PsfStreamProvider.h
	PsfTags.cpp
	PsfTags.h
	PsfVm.cpp
	PsfVm.h
	PsfZipArchive.cpp
	PsfZipArchive.h
	psp/MA_ALLEGREX.cpp
	psp/MA_ALLEGREX.h
	psp/MA_ALLEGREX_Reflection.cpp
	psp/Psp_Audio.cpp
	psp/Psp_Audio.h
	psp/Psp_IoDevice.h
	psp/Psp_IoFileMgrForUser.cpp
	psp/Psp_IoFileMgrForUser.h
	psp/Psp_KernelLibrary.cpp
	psp/Psp_KernelLibrary.h
	psp/Psp_PsfBios.cpp
	psp/Psp_PsfBios.h
	psp/Psp_PsfDevice.cpp
	psp/Psp_PsfDevice.h
	psp/Psp_PsfSubSystem.cpp
	psp/Psp_PsfSubSystem.h
	psp/Psp_SasCore.cpp
	psp/Psp_SasCore.h
	psp/Psp_StdioForUser.cpp
	psp/Psp_StdioForUser.h
	psp/Psp_SysMemUserForUser.cpp
	psp/Psp_SysMemUserForUser.h
	psp/Psp_ThreadManForUser.cpp
	psp/Psp_ThreadManForUser.h
	psp/Psp_UmdUser.cpp
	psp/Psp_UmdUser.h
	psp/PspBios.cpp
	psp/PspBios.h
)

if(PSF_RAR_SUPPORT)
	list(APPEND PSFCORE_SRC_FILES PsfRarArchive.cpp)
	list(APPEND PSF_DEFINITIONS -DRAR_SUPPORT)
endif()

add_library(PsfCore STATIC ${PSFCORE_SRC_FILES})
target_link_libraries(PsfCore ${PROJECT_LIBS})
target_include_directories(PsfCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(PsfCore PRIVATE ${PSF_DEFINITIONS})
