cmake_minimum_required(VERSION 3.5)

set(CMAKE_MODULE_PATH
	${CMAKE_CURRENT_SOURCE_DIR}/../../../deps/Dependencies/cmake-modules
	${CMAKE_MODULE_PATH}
)

include(Header)

project(GSH_Vulkan)

find_package(Vulkan REQUIRED)

add_library(gsh_vulkan STATIC 
	GSH_Vulkan.cpp
	GSH_Vulkan.h
)
target_link_libraries(gsh_vulkan ${PROJECT_LIBS} ${VULKAN_LIBRARY})
target_include_directories(gsh_vulkan PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Source/gs/GSH_Vulkan/ ${VULKAN_INCLUDE_DIR})
