cmake_minimum_required(VERSION 3.5)

set(CMAKE_MODULE_PATH
	${CMAKE_CURRENT_SOURCE_DIR}/../../../deps/Dependencies/cmake-modules
	${CMAKE_MODULE_PATH}
)

include(Header)

project(GSH_OpenGL)

if(NOT TARGET Framework_OpenGl)
	add_subdirectory(
		${CMAKE_CURRENT_SOURCE_DIR}/../../../deps/Framework/build_cmake/FrameworkOpenGl
		${CMAKE_CURRENT_BINARY_DIR}/FrameworkOpenGl
	)
endif()
list(APPEND GSH_OPENGL_PROJECT_LIBS Framework_OpenGl)

if(TARGET_PLATFORM_UNIX_ARM)
	list(APPEND GSH_OPENGL_COMPILE_OPTIONS "-mfpu=neon")
endif()

add_library(gsh_opengl STATIC 
	GSH_OpenGL.cpp
	GSH_OpenGL.h
	GSH_OpenGL_Shader.cpp
	GSH_OpenGL_Texture.cpp
)
target_link_libraries(gsh_opengl Framework_OpenGl ${GSH_OPENGL_PROJECT_LIBS})
target_include_directories(gsh_opengl PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Source/gs/GSH_OpenGL/)
target_compile_options(gsh_opengl PRIVATE ${GSH_OPENGL_COMPILE_OPTIONS})
