cmake_minimum_required(VERSION 3.5)

set(CMAKE_MODULE_PATH
	${CMAKE_CURRENT_SOURCE_DIR}/../../../../deps/Dependencies/cmake-modules
	${CMAKE_MODULE_PATH}
)
include(Header)

project(PsfPlayer)

if(NOT TARGET PsfCore)
	add_subdirectory(
		${CMAKE_CURRENT_SOURCE_DIR}/../
		${CMAKE_CURRENT_BINARY_DIR}/PsfCore
	)
endif()
list(APPEND PROJECT_LIBS PsfCore)

add_executable(PsfPlayer 
	Main.cpp
	SH_FileOutput.cpp
	SH_FileOutput.h
)
target_link_libraries(PsfPlayer PUBLIC ${PROJECT_LIBS})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s MODULARIZE=1 -s EXPORTED_FUNCTIONS=\"['_main', '_initVm', '_loadPsf', '_step', '_EmptyBlockHandler', '_MemoryUtils_GetByteProxy', '_MemoryUtils_GetHalfProxy', '_MemoryUtils_GetWordProxy', '_MemoryUtils_SetByteProxy', '_MemoryUtils_SetHalfProxy', '_MemoryUtils_SetWordProxy']\" -s EXPORTED_RUNTIME_METHODS=\"['ccall', 'FS', 'NODEFS']\" -s ASYNCIFY -s ASYNCIFY_IMPORTS=\"WasmCreateFunction\" -s ASSERTIONS=2 -s FORCE_FILESYSTEM -s ALLOW_TABLE_GROWTH -s ALLOW_MEMORY_GROWTH -lnodefs.js -fexceptions")
