cmake_minimum_required(VERSION 3.5)

set(CMAKE_MODULE_PATH
	${CMAKE_CURRENT_SOURCE_DIR}/../../../Dependencies/cmake-modules
	${CMAKE_MODULE_PATH}
)
include(Header)

project(VuTest)

if (NOT TARGET PlayCore)
	add_subdirectory(
		${CMAKE_CURRENT_SOURCE_DIR}/../../Source/
		${CMAKE_CURRENT_BINARY_DIR}/Source
	)
endif()

add_executable(VuTest
	AddTest.cpp
	FlagsTest2.cpp
	FlagsTest.cpp
	Main.cpp
	TestVm.cpp
	TriAceTest.cpp
	VuAssembler.cpp
)
target_link_libraries(VuTest PlayCore)
add_test(NAME VuTest
	COMMAND VuTest
)