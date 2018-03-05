cmake_minimum_required(VERSION 3.5)

set(CMAKE_MODULE_PATH
	${CMAKE_CURRENT_SOURCE_DIR}/../../../Dependencies/cmake-modules
	${CMAKE_MODULE_PATH}
)
include(Header)

project(McServTest)

if (NOT TARGET PlayCore)
	add_subdirectory(
		${CMAKE_CURRENT_SOURCE_DIR}/../../Source/
		${CMAKE_CURRENT_BINARY_DIR}/Source
	)
endif()

add_executable(McServTest
	AppConfig.cpp
	GameTestSheet.cpp
	Main.cpp
)
target_link_libraries(McServTest PlayCore)

add_test(NAME McServTest
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
	COMMAND McServTest
)
