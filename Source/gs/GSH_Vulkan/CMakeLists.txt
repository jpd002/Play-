cmake_minimum_required(VERSION 3.5)

set(CMAKE_MODULE_PATH
	${CMAKE_CURRENT_SOURCE_DIR}/../../../deps/Dependencies/cmake-modules
	${CMAKE_MODULE_PATH}
)

include(Header)

project(GSH_Vulkan)

if(NOT TARGET Framework_Vulkan)
	add_subdirectory(
		${CMAKE_CURRENT_SOURCE_DIR}/../../../deps/Framework/build_cmake/FrameworkVulkan
		${CMAKE_CURRENT_BINARY_DIR}/FrameworkVulkan
	)
endif()
list(APPEND PROJECT_LIBS Framework_Vulkan)

add_library(gsh_vulkan STATIC
	GSH_Vulkan.cpp
	GSH_Vulkan.h
)

target_link_libraries(gsh_vulkan ${PROJECT_LIBS})
target_include_directories(gsh_vulkan PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Source/gs/GSH_Vulkan/)
