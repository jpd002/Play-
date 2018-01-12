cmake_minimum_required(VERSION 3.5)

set(CMAKE_MODULE_PATH
	${CMAKE_CURRENT_SOURCE_DIR}/../../../Dependencies/cmake-modules
	${CMAKE_MODULE_PATH}
)
include(Header)

project(Play)

set(PROJECT_NAME "Play!")
set(PROJECT_Version 0.30)
add_definitions(-DPLAY_VERSION="${PROJECT_Version}")

if(NOT TARGET PlayCore)
	add_subdirectory(
		${CMAKE_CURRENT_SOURCE_DIR}/../
		${CMAKE_CURRENT_BINARY_DIR}/Source
	)
endif()
list(APPEND PROJECT_LIBS PlayCore)

if(NOT TARGET gsh_opengl)
	add_subdirectory(
		${CMAKE_CURRENT_SOURCE_DIR}/../gs/GSH_OpenGL
		${CMAKE_CURRENT_BINARY_DIR}/gs/GSH_OpenGL
	)
endif()
list(INSERT PROJECT_LIBS 0 gsh_opengl)

# If ICU is available, add its libraries because Framework might need its functions
find_package(ICUUC)
if(ICUUC_FOUND)
	list(APPEND PROJECT_LIBS ${ICUUC_LIBRARIES})
endif()

find_package(ZLIB)
if(NOT ZLIB_FOUND)
	MESSAGE("-- Using Provided zlib source")
	if(NOT TARGET ZLIB::ZLIB)
		add_subdirectory(
			${CMAKE_CURRENT_SOURCE_DIR}/../../../Dependencies/build_cmake/zlib-1.2.8
			${CMAKE_CURRENT_BINARY_DIR}/zlib-1.2.8
		)
	endif()
endif()
list(APPEND PROJECT_LIBS ZLIB::ZLIB)

set(ANDROID_SRC_FILES
	GSH_OpenGLAndroid.cpp
	InputManager.cpp
	NativeInterop.cpp
	NativeShared.cpp
	SettingsManager.cpp
	SH_OpenSL.cpp
	StatsManager.cpp
)

list(APPEND PROJECT_LIBS android log OpenSLES GLESv3 EGL)

add_library(Play SHARED ${ANDROID_SRC_FILES})
target_link_libraries(Play ${PROJECT_LIBS})
