cmake_minimum_required(VERSION 3.5)

set(CMAKE_MODULE_PATH
	${CMAKE_CURRENT_SOURCE_DIR}/../../../Dependencies/cmake-modules
	${CMAKE_MODULE_PATH}
)
include(Header)


project(PsfPlayer)
set(PROJECT_LIBS)
set(PSF_PROJECT_NAME "PsfPlayer")
set(PSF_PROJECT_VERSION "1.05")

if(TARGET_PLATFORM_UNIX)
	set(USE_QT ON CACHE BOOL "Use Qt UI")
endif()

#UI
if(TARGET_PLATFORM_MACOS AND NOT USE_QT)
	add_subdirectory(Source/macosx_ui/)
endif(TARGET_PLATFORM_MACOS  AND NOT USE_QT)

if(TARGET_PLATFORM_WIN32 AND NOT USE_QT)
	add_subdirectory(Source/win32_ui/)
endif(TARGET_PLATFORM_WIN32 AND NOT USE_QT)

if(USE_QT)
	add_subdirectory(Source/unix_ui/)
endif (USE_QT)
