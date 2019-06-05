cmake_minimum_required(VERSION 3.5)

set(CMAKE_MODULE_PATH
	${CMAKE_CURRENT_SOURCE_DIR}/../../../../Dependencies/cmake-modules
	${CMAKE_MODULE_PATH}
)

include(Header)

project(gsh_d3d9)

#If building for Windows XP, we need to use the provided d3dcompiler since it's not available in the SDK
if(CMAKE_GENERATOR_TOOLSET MATCHES "_xp$")
	if(NOT TARGET d3dcompiler)
		add_subdirectory(
			${CMAKE_CURRENT_SOURCE_DIR}/../../../../Dependencies/d3dcompiler_winxp
			${CMAKE_CURRENT_BINARY_DIR}/d3dcompiler_winxp
		)
	endif()
	list(APPEND gsh_d3d9_PROJECT_LIBS d3dcompiler)
endif()

if(NOT TARGET Framework_Win32)
	add_subdirectory(
		${CMAKE_CURRENT_SOURCE_DIR}/../../../../Framework/build_cmake/FrameworkWin32
		${CMAKE_CURRENT_BINARY_DIR}/Framework_Win32
	)
endif()
list(APPEND gsh_d3d9_PROJECT_LIBS Framework_Win32)

if(NOT TARGET Nuanceur)
add_subdirectory(
	${CMAKE_CURRENT_SOURCE_DIR}/../../../../Nuanceur/build_cmake
	${CMAKE_CURRENT_BINARY_DIR}/Nuanceur
)
endif()
list(APPEND gsh_d3d9_PROJECT_LIBS Nuanceur)

add_library(gsh_d3d9 STATIC 
	GSH_Direct3D9.cpp
	GSH_Direct3D9.h
	GSH_Direct3D9_Shader.cpp
	GSH_Direct3D9_Texture.cpp
)

target_link_libraries(gsh_d3d9 d3d9 ${gsh_d3d9_PROJECT_LIBS})
