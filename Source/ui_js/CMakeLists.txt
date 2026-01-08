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

# target_link_options(Play PRIVATE "-sINITIAL_TABLE=1024")
target_link_options(Play PRIVATE "-sINITIAL_TABLE=128")

target_link_options(Play PRIVATE "-sALLOW_MEMORY_GROWTH")

# Increased initial memory to 128MB to prevent out-of-bounds errors during initialization
# when using pthreads with shared memory. Each thread needs stack space and the shared
# memory must be large enough for all threads during initialization.
target_link_options(Play PRIVATE "-sINITIAL_MEMORY=134217728")
# MAXIMUM_MEMORY must match the shared memory maximum (32768 pages = 2GB) when using pthreads
target_link_options(Play PRIVATE "-sMAXIMUM_MEMORY=2147483648")
# Increased stack size to 2MB to ensure sufficient space for each thread during initialization
target_link_options(Play PRIVATE "-sTOTAL_STACK=2097152")

# Disable pthreads to avoid out-of-bounds memory access errors in worker threads
# Pthreads can cause issues in WebKit/WebView environments, especially during initialization
# Explicitly set pool size to 0 to ensure no worker threads are created
# (Even though -pthread flag is present, no workers will spawn with PTHREAD_POOL_SIZE=0)
# target_link_options(Play PRIVATE "-sPTHREAD_POOL_SIZE=0")
target_link_options(Play PRIVATE "-sPTHREAD_POOL_SIZE=2")
# Do not enable shared memory when pool size is 0
# target_link_options(Play PRIVATE "-sSHARED_MEMORY")

#target_link_options(Play PRIVATE "-sASSERTIONS=2")
target_link_options(Play PRIVATE "-sEXPORT_NAME=Play")
target_link_options(Play PRIVATE "-sWASM_BIGINT")
target_link_options(Play PRIVATE "-sOFFSCREEN_FRAMEBUFFER=1")
# Must include 'worker' when -pthread flag is present (even if pthreads are disabled via PTHREAD_POOL_SIZE=0)
# Since we've disabled PTHREAD_POOL_SIZE and SHARED_MEMORY, no actual worker threads will be created
target_link_options(Play PRIVATE "-sENVIRONMENT=web,worker")
target_link_options(Play PRIVATE "-sUSE_WEBGL2=1")
