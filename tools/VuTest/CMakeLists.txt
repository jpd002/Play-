cmake_minimum_required(VERSION 3.5)

set(CMAKE_MODULE_PATH
	${CMAKE_CURRENT_SOURCE_DIR}/../../deps/Dependencies/cmake-modules
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
	BranchTest.cpp
	FlagsTest.cpp
	FlagsTest2.cpp
	FlagsTest3.cpp
	FlagsTest4.cpp
	Main.cpp
	MinMaxFlagsTest.cpp
	MinMaxTest.cpp
	StallTest.cpp
	StallTest2.cpp
	StallTest3.cpp
	StallTest4.cpp
	StallTest5.cpp
	TestVm.cpp
	TriAceTest.cpp
	VuAssembler.cpp

	AddTest.h
	BranchTest.h
	FlagsTest.h
	FlagsTest2.h
	FlagsTest3.h
	FlagsTest4.h
	MinMaxFlagsTest.h
	MinMaxTest.h
	StallTest.h
	StallTest2.h
	StallTest3.h
	StallTest4.h
	StallTest5.h
	Test.h
	TestVm.h
	TriAceTest.h
	VuAssembler.h
)
target_link_libraries(VuTest PlayCore)
add_test(NAME VuTest
	COMMAND VuTest
)