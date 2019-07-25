cmake_minimum_required(VERSION 3.5)

set(CMAKE_MODULE_PATH
	${CMAKE_CURRENT_SOURCE_DIR}/../../../Dependencies/cmake-modules
	${CMAKE_MODULE_PATH}
)

project(ui_shared)

if(NOT TARGET Framework_Sqlite)
	add_subdirectory(
		${CMAKE_CURRENT_SOURCE_DIR}/../../../Framework/build_cmake/FrameworkSqlite
		${CMAKE_CURRENT_BINARY_DIR}/FrameworkSqlite
	)
endif()
list(APPEND SHARED_UI_PROJECT_LIBS Framework_Sqlite)

if(NOT TARGET Framework_Http)
	add_subdirectory(
		${CMAKE_CURRENT_SOURCE_DIR}/../../../Framework/build_cmake/FrameworkHttp
		${CMAKE_CURRENT_BINARY_DIR}/FrameworkHttp
	)
endif()
list(APPEND SHARED_UI_PROJECT_LIBS Framework_Http)

if(NOT TARGET nlohmann_json)
	set(JSON_BuildTests OFF CACHE BOOL "Disable test build")
	add_subdirectory(
		${CMAKE_CURRENT_SOURCE_DIR}/../../../Dependencies/build_cmake/nlohmann_json
		${CMAKE_CURRENT_BINARY_DIR}/nlohmann_json
		EXCLUDE_FROM_ALL
	)
endif()
list(APPEND SHARED_UI_PROJECT_LIBS nlohmann_json)

if(NOT TARGET PlayCore)
	add_subdirectory(
		${CMAKE_CURRENT_SOURCE_DIR}/../
		${CMAKE_CURRENT_BINARY_DIR}/Source
	)
endif()
list(APPEND SHARED_UI_PROJECT_LIBS PlayCore)

set(SHARED_UI_SRC_FILES
	BootablesDbClient.cpp
	BootablesDbClient.h
	BootablesProcesses.cpp
	BootablesProcesses.h
	TheGamesDbClient.cpp
	TheGamesDbClient.h
	StatsManager.cpp
	StatsManager.h
)

add_library(ui_shared STATIC ${SHARED_UI_SRC_FILES})
target_link_libraries(ui_shared ${SHARED_UI_PROJECT_LIBS})
target_include_directories(ui_shared PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)
