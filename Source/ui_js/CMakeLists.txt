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

if(NOT TARGET ui_shared)
	add_subdirectory(
		${CMAKE_CURRENT_SOURCE_DIR}/../ui_shared
		${CMAKE_CURRENT_BINARY_DIR}/ui_shared
	)
endif()
list(APPEND PROJECT_LIBS ui_shared)

if(NOT TARGET gsh_opengl)
	add_subdirectory(
		${CMAKE_CURRENT_SOURCE_DIR}/../gs/GSH_OpenGL
		${CMAKE_CURRENT_BINARY_DIR}/gs/GSH_OpenGL
	)
endif()
list(APPEND PROJECT_LIBS gsh_opengl)

if(NOT TARGET sh_openal)
	add_subdirectory(
		${CMAKE_CURRENT_SOURCE_DIR}/../../tools/PsfPlayer/Source/SH_OpenAL
		${CMAKE_CURRENT_BINARY_DIR}/SH_OpenAL
	)
endif()
list(APPEND PROJECT_LIBS sh_openal)

add_executable(Play
	GSH_OpenGLJs.cpp
	GSH_OpenGLJs.h
	InputProviderEmscripten.cpp
	InputProviderEmscripten.h
	Main.cpp
	Ps2VmJs.cpp
	Ps2VmJs.h
	${CMAKE_CURRENT_SOURCE_DIR}/../../tools/PsfPlayer/Source/ui_js/SH_OpenALProxy.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/../../tools/PsfPlayer/Source/ui_js/SH_OpenALProxy.h
)
target_link_libraries(Play PUBLIC ${PROJECT_LIBS})

target_link_options(Play PRIVATE "--bind")
target_link_options(Play PRIVATE "-sMODULARIZE=1")
target_link_options(Play PRIVATE "-sEXPORT_ES6=1")
target_link_options(Play PRIVATE "-sUSE_ES6_IMPORT_META=0")
target_link_options(Play PRIVATE "-sEXPORTED_FUNCTIONS=['_main', '_initVm', '_EmptyBlockHandler', '_MemoryUtils_GetByteProxy', '_MemoryUtils_GetHalfProxy', '_MemoryUtils_GetWordProxy', '_MemoryUtils_GetDoubleProxy', '_MemoryUtils_SetByteProxy', '_MemoryUtils_SetHalfProxy', '_MemoryUtils_SetWordProxy', '_MemoryUtils_SetDoubleProxy', '_LWL_Proxy', '_LWR_Proxy', '_LDL_Proxy', '_LDR_Proxy', '_SWL_Proxy', '_SWR_Proxy', '_SDL_Proxy', '_SDR_Proxy']")
target_link_options(Play PRIVATE "-sEXPORTED_RUNTIME_METHODS=['ccall', 'FS']")
target_link_options(Play PRIVATE "-sFORCE_FILESYSTEM")
target_link_options(Play PRIVATE "-sALLOW_TABLE_GROWTH")
target_link_options(Play PRIVATE "-sALLOW_MEMORY_GROWTH")
#target_link_options(Play PRIVATE "-sASSERTIONS=2")
target_link_options(Play PRIVATE "-sEXPORT_NAME=Play")
target_link_options(Play PRIVATE "-sPTHREAD_POOL_SIZE=2")
target_link_options(Play PRIVATE "-sWASM_BIGINT")
target_link_options(Play PRIVATE "-sOFFSCREEN_FRAMEBUFFER=1")
target_link_options(Play PRIVATE "-sENVIRONMENT=web,worker")
target_link_options(Play PRIVATE "-sUSE_WEBGL2=1")
