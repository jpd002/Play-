cmake_minimum_required(VERSION 3.18)
project(VulkanTest)

if(NOT TARGET gsh_vulkan)
	add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../Source/gs/GSH_Vulkan
	                 ${CMAKE_CURRENT_BINARY_DIR}/GSH_Vulkan)
endif()

add_executable(VulkanTest Main.cpp)
target_link_libraries(VulkanTest gsh_vulkan PlayCore)
add_test(NAME VulkanTest COMMAND VulkanTest)
set_tests_properties(VulkanTest PROPERTIES LABELS "GPU;Vulkan" TIMEOUT 120)
