cmake_minimum_required(VERSION 3.5)

set(CMAKE_MODULE_PATH
	${CMAKE_CURRENT_SOURCE_DIR}/../../deps/Dependencies/cmake-modules
	${CMAKE_MODULE_PATH}
)
include(Header)

project(PlayUI)

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

if(NOT TARGET gsh_vulkan)
	add_subdirectory(
		${CMAKE_CURRENT_SOURCE_DIR}/../gs/GSH_Vulkan
		${CMAKE_CURRENT_BINARY_DIR}/gs/GSH_Vulkan
	)
endif()
list(INSERT PROJECT_LIBS 0 gsh_vulkan)

if(NOT TARGET ui_shared)
	add_subdirectory(
		${CMAKE_CURRENT_SOURCE_DIR}/../ui_shared
		${CMAKE_CURRENT_BINARY_DIR}/ui_shared
	)
endif()
list(APPEND PROJECT_LIBS ui_shared)

set(ANDROID_SRC_FILES
	BootablesInterop.cpp
	com_virtualapplications_play_Bootable.cpp
	com_virtualapplications_play_Bootable.h
	GSH_OpenGLAndroid.cpp
	GSH_OpenGLAndroid.h
	GSH_VulkanAndroid.cpp
	GSH_VulkanAndroid.h
	InputManager.cpp
	NativeInterop.cpp
	NativeShared.cpp
	NativeWindowUpdateListener.h
	SettingsManager.cpp
	SH_OpenSL.cpp
	StatsInterop.cpp
)

list(APPEND PROJECT_LIBS android log OpenSLES)

add_library(Play SHARED ${ANDROID_SRC_FILES})
target_link_libraries(Play ${PROJECT_LIBS})
