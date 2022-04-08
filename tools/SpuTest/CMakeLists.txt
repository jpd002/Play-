cmake_minimum_required(VERSION 3.5)

set(CMAKE_MODULE_PATH
	${CMAKE_CURRENT_SOURCE_DIR}/../../deps/Dependencies/cmake-modules
	${CMAKE_MODULE_PATH}
)
include(Header)

project(SpuTest)

if (NOT TARGET PlayCore)
	add_subdirectory(
		${CMAKE_CURRENT_SOURCE_DIR}/../../Source/
		${CMAKE_CURRENT_BINARY_DIR}/Source
	)
endif()

add_executable(SpuTest
	KeyOnOffTest.cpp
	Main.cpp
	SimpleIrqTest.cpp
	SweepTest.cpp
	Test.cpp

	KeyOnOffTest.h
	SimpleIrqTest.h
	SweepTest.h
	Test.h
)

target_link_libraries(SpuTest PlayCore)
add_test(NAME SpuTest
	COMMAND SpuTest
)