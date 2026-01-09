cmake_minimum_required(VERSION 3.18)

set(CMAKE_MODULE_PATH
	${CMAKE_CURRENT_SOURCE_DIR}/../../deps/Dependencies/cmake-modules
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
list(APPEND UI_JS_PROJECT_LIBS PlayCore)

if(NOT TARGET ui_shared)
	add_subdirectory(
		${CMAKE_CURRENT_SOURCE_DIR}/../ui_shared
		${CMAKE_CURRENT_BINARY_DIR}/ui_shared
	)
endif()
list(APPEND UI_JS_PROJECT_LIBS ui_shared)

if(NOT TARGET gsh_opengl)
	add_subdirectory(
		${CMAKE_CURRENT_SOURCE_DIR}/../gs/GSH_OpenGL
		${CMAKE_CURRENT_BINARY_DIR}/gs/GSH_OpenGL
	)
endif()
list(APPEND UI_JS_PROJECT_LIBS gsh_opengl)

if(NOT TARGET sh_openal)
	add_subdirectory(
		${CMAKE_CURRENT_SOURCE_DIR}/../sound/SH_OpenAL
		${CMAKE_CURRENT_BINARY_DIR}/SH_OpenAL
	)
endif()
list(APPEND UI_JS_PROJECT_LIBS sh_openal)

add_executable(Play
	GSH_OpenGLJs.cpp
	GSH_OpenGLJs.h
	InputProviderEmscripten.cpp
	InputProviderEmscripten.h
	Main.cpp
	Ps2VmJs.cpp
	Ps2VmJs.h
)
target_link_libraries(Play PUBLIC ${UI_JS_PROJECT_LIBS})

target_link_options(Play PRIVATE "--bind")
target_link_options(Play PRIVATE "-sMODULARIZE=1")
target_link_options(Play PRIVATE "-sEXPORT_ES6=1")
# Removed -sUSE_ES6_IMPORT_META=0 as it's no longer supported in newer Emscripten
target_link_options(Play PRIVATE "-sEXPORTED_FUNCTIONS=['_main', '_initVm', '_EmptyBlockHandler', '_MemoryUtils_GetByteProxy', '_MemoryUtils_GetHalfProxy', '_MemoryUtils_GetWordProxy', '_MemoryUtils_GetDoubleProxy', '_MemoryUtils_SetByteProxy', '_MemoryUtils_SetHalfProxy', '_MemoryUtils_SetWordProxy', '_MemoryUtils_SetDoubleProxy', '_LWL_Proxy', '_LWR_Proxy', '_LDL_Proxy', '_LDR_Proxy', '_SWL_Proxy', '_SWR_Proxy', '_SDL_Proxy', '_SDR_Proxy']")
target_link_options(Play PRIVATE "-sEXPORTED_RUNTIME_METHODS=['ccall', 'FS']")
target_link_options(Play PRIVATE "-sFORCE_FILESYSTEM")
target_link_options(Play PRIVATE "-sALLOW_TABLE_GROWTH")

# Reduced INITIAL_TABLE from 1024 to 256 to minimize memory usage
target_link_options(Play PRIVATE "-sINITIAL_TABLE=256")
# target_link_options(Play PRIVATE "-sINITIAL_TABLE=1024")

# Fixed memory mode to fix iOS 26.2 shared memory issues with pthreads + growable memory
# See: https://github.com/emscripten-core/emscripten/issues/25905
# With ALLOW_MEMORY_GROWTH=0, memory is fixed at INITIAL_MEMORY size (no growth)
target_link_options(Play PRIVATE "-sALLOW_MEMORY_GROWTH=0")

# Memory configuration for fixed memory mode (ALLOW_MEMORY_GROWTH=0)
# CRITICAL: With fixed memory, INITIAL_MEMORY is both the initial AND maximum memory size
# MAXIMUM_MEMORY is ignored when ALLOW_MEMORY_GROWTH=0
# INITIAL_MEMORY must be large enough for all memory needs from the start
# 
# With PTHREAD_POOL_SIZE=2 and shared memory, INITIAL_MEMORY must accommodate:
# - Main thread heap/stack
# - Shared memory buffer for pthread communication (same size as INITIAL_MEMORY)
# - Worker thread initialization overhead
# - initVm function table and initialization code
# - All runtime allocations (PS2 emulation, textures, audio buffers, etc.)
#
# Testing showed ~300MB (314572800 bytes) is needed for stable operation
# Lower values cause "unwind" errors during initVm due to insufficient memory
# target_link_options(Play PRIVATE "-sINITIAL_MEMORY=314572800")
# 600
target_link_options(Play PRIVATE "-sINITIAL_MEMORY=629145600")


# Increased TOTAL_STACK to 512KB per thread - 384KB was too small for worker initialization
# With 2 threads: 512KB * 2 = 1MB total stack overhead
# Smaller stack caused out-of-bounds errors during pthread initialization
target_link_options(Play PRIVATE "-sTOTAL_STACK=524288")
# Keep PTHREAD_POOL_SIZE=2 (required to prevent loading freeze)
# Cannot reduce below 2 as it causes loading to freeze
target_link_options(Play PRIVATE "-sPTHREAD_POOL_SIZE=2")
# Enable shared memory explicitly (required for pthreads with PTHREAD_POOL_SIZE > 0)
target_link_options(Play PRIVATE "-sSHARED_MEMORY")
# Note: MEMORY_GROWTH_LINEAR_STEP is not used when ALLOW_MEMORY_GROWTH=0 (fixed memory mode)

#target_link_options(Play PRIVATE "-sASSERTIONS=2")
target_link_options(Play PRIVATE "-sEXPORT_NAME=Play")
target_link_options(Play PRIVATE "-sWASM_BIGINT")
target_link_options(Play PRIVATE "-sOFFSCREEN_FRAMEBUFFER=1")
target_link_options(Play PRIVATE "-sENVIRONMENT=web,worker")
target_link_options(Play PRIVATE "-sUSE_WEBGL2=1")
