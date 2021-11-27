cmake_minimum_required(VERSION 3.5)

set(CMAKE_MODULE_PATH
	${CMAKE_CURRENT_SOURCE_DIR}/../../../../deps/Dependencies/cmake-modules
	${CMAKE_MODULE_PATH}
)
include(Header)

project(Play)

if(NOT TARGET PlayCore)
	add_subdirectory(
		${CMAKE_CURRENT_SOURCE_DIR}/../
		${CMAKE_CURRENT_BINARY_DIR}/Source
	)
endif()
list(APPEND PROJECT_LIBS PlayCore)

add_executable(Play
	Main.cpp
	Ps2VmJs.cpp
	Ps2VmJs.h
)
target_link_libraries(Play PUBLIC ${PROJECT_LIBS})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --bind")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s MODULARIZE=1")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s EXPORTED_FUNCTIONS=\"['_main', '_initVm', '_EmptyBlockHandler', '_MemoryUtils_GetByteProxy', '_MemoryUtils_GetHalfProxy', '_MemoryUtils_GetWordProxy', '_MemoryUtils_SetByteProxy', '_MemoryUtils_SetHalfProxy', '_MemoryUtils_SetWordProxy', '_LWL_Proxy', '_LWR_Proxy', '_SWL_Proxy', '_SWR_Proxy']\"")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s EXPORTED_RUNTIME_METHODS=\"['ccall', 'FS']\"")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s FORCE_FILESYSTEM")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s ALLOW_TABLE_GROWTH")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s ALLOW_MEMORY_GROWTH")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s ASSERTIONS=2")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s EXPORT_NAME=Play")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s PTHREAD_POOL_SIZE=4")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s ENVIRONMENT=web,worker")
