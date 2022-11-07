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

if(NOT TARGET sh_openal)
	add_subdirectory(
		${CMAKE_CURRENT_SOURCE_DIR}/../SH_OpenAL
		${CMAKE_CURRENT_BINARY_DIR}/SH_OpenAL
	)
endif()
list(APPEND PROJECT_LIBS sh_openal)

add_executable(PsfPlayer 
	Main.cpp
	PsfVmJs.cpp
	PsfVmJs.h
	SH_FileOutput.cpp
	SH_FileOutput.h
	SH_OpenALProxy.cpp
	SH_OpenALProxy.h
)
target_link_libraries(PsfPlayer PUBLIC ${PROJECT_LIBS})

target_link_options(PsfPlayer PRIVATE "--bind")
target_link_options(PsfPlayer PRIVATE "-sMODULARIZE=1")
target_link_options(PsfPlayer PRIVATE "-sEXPORT_ES6=1")
target_link_options(PsfPlayer PRIVATE "-sUSE_ES6_IMPORT_META=0")
target_link_options(PsfPlayer PRIVATE "-sEXPORTED_FUNCTIONS=['_main', '_initVm', '_EmptyBlockHandler', '_MemoryUtils_GetByteProxy', '_MemoryUtils_GetHalfProxy', '_MemoryUtils_GetWordProxy', '_MemoryUtils_SetByteProxy', '_MemoryUtils_SetHalfProxy', '_MemoryUtils_SetWordProxy', '_LWL_Proxy', '_LWR_Proxy', '_SWL_Proxy', '_SWR_Proxy']")
target_link_options(PsfPlayer PRIVATE "-sEXPORTED_RUNTIME_METHODS=['ccall', 'FS', 'NODEFS']")
target_link_options(PsfPlayer PRIVATE "-sFORCE_FILESYSTEM")
target_link_options(PsfPlayer PRIVATE "-sALLOW_TABLE_GROWTH")
target_link_options(PsfPlayer PRIVATE "-sALLOW_MEMORY_GROWTH")
#target_link_options(PsfPlayer PRIVATE "-sASSERTIONS=2")
target_link_options(PsfPlayer PRIVATE "-sMAXIMUM_MEMORY=134217728")
target_link_options(PsfPlayer PRIVATE "-lnodefs.js")
target_link_options(PsfPlayer PRIVATE "-sEXPORT_NAME=PsfPlayer")
target_link_options(PsfPlayer PRIVATE "-sPTHREAD_POOL_SIZE=1")
target_link_options(PsfPlayer PRIVATE "-sENVIRONMENT=web,worker")
