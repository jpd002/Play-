include(Header)

project(SH_OpenAL)

if(NOT TARGET Framework)
	add_subdirectory(
		${CMAKE_CURRENT_SOURCE_DIR}/../../../deps/Framework/build_cmake/Framework
		${CMAKE_CURRENT_BINARY_DIR}/Framework
	)
endif()
list(APPEND SH_OPENAL_PROJECT_LIBS Framework)

if(NOT TARGET Framework_OpenAl)
	add_subdirectory(
		${CMAKE_CURRENT_SOURCE_DIR}/../../../deps/Framework/build_cmake/FrameworkOpenAl
		${CMAKE_CURRENT_BINARY_DIR}/FrameworkOpenAl
	)
endif()
list(APPEND SH_OPENAL_PROJECT_LIBS Framework_OpenAl)

if(TARGET_PLATFORM_JS)
	set(SH_OPENAL_PLATFORM_SPECIFIC_FILES
		SH_OpenALProxy.cpp
		SH_OpenALProxy.h
	)
endif()

add_library(sh_openal STATIC
	SH_OpenAL.cpp
	SH_OpenAL.h
	${SH_OPENAL_PLATFORM_SPECIFIC_FILES}
)
target_link_libraries(sh_openal ${SH_OPENAL_PROJECT_LIBS})
