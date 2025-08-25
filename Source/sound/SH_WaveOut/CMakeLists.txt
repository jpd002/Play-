include(Header)

project(SH_WaveOut)

if(NOT TARGET Framework)
	add_subdirectory(
		${CMAKE_CURRENT_SOURCE_DIR}/../../../deps/Framework/build_cmake/Framework
		${CMAKE_CURRENT_BINARY_DIR}/Framework
	)
endif()
list(APPEND SH_WAVEOUT_PROJECT_LIBS Framework)

add_library(sh_waveout STATIC
	SH_WaveOut.cpp
	SH_WaveOut.h
)
target_link_libraries(sh_waveout ${SH_WAVEOUT_PROJECT_LIBS})
