cmake_minimum_required(VERSION 3.5)

set(CMAKE_MODULE_PATH
	${CMAKE_CURRENT_SOURCE_DIR}/../../../../Dependencies/cmake-modules
	${CMAKE_MODULE_PATH}
)

include(Header)

project(SH_OpenAL)

if(NOT TARGET Framework)
	add_subdirectory(
		${CMAKE_CURRENT_SOURCE_DIR}/../../../../../Framework/build_cmake/Framework
		${CMAKE_CURRENT_BINARY_DIR}/Framework
	)
endif()
list(APPEND PROJECT_LIBS Framework)

if(NOT TARGET Framework_OpenAl)
add_subdirectory(
	${CMAKE_CURRENT_SOURCE_DIR}/../../../../../Framework/build_cmake/FrameworkOpenAl
	${CMAKE_CURRENT_BINARY_DIR}/FrameworkOpenAl
)
endif()
list(APPEND PROJECT_LIBS Framework_OpenAl)

add_library(sh_openal STATIC
	../SH_OpenAL.cpp
	../SH_OpenAL.h
)
target_link_libraries(sh_openal ${PROJECT_LIBS})
