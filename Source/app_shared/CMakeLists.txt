cmake_minimum_required(VERSION 3.5)

set(CMAKE_MODULE_PATH
	${CMAKE_CURRENT_SOURCE_DIR}/../../deps/Dependencies/cmake-modules
	${CMAKE_MODULE_PATH}
)
include(Header)

project(app_shared)

if (NOT TARGET Framework)
	add_subdirectory(
		${CMAKE_CURRENT_SOURCE_DIR}/../../Framework/build_cmake/Framework
		${CMAKE_CURRENT_BINARY_DIR}/Framework
	)
endif()

if(TARGET_PLATFORM_ANDROID OR TARGET_PLATFORM_IOS OR TARGET_PLATFORM_JS)
	list(APPEND APP_SHARED_DEFINITIONS_LIST LOGGING_ENABLED=0)
endif()

set(COMMON_SRC_FILES
	AppConfig.cpp
	AppConfig.h
	Log.cpp
	Log.h
)

add_library(app_shared STATIC ${COMMON_SRC_FILES})
target_link_libraries(app_shared PUBLIC Framework)
target_include_directories(app_shared PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(app_shared PUBLIC ${APP_SHARED_DEFINITIONS_LIST})
