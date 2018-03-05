cmake_minimum_required(VERSION 3.5)

set(CMAKE_MODULE_PATH
	${CMAKE_CURRENT_SOURCE_DIR}/../../../../../Dependencies/cmake-modules
	${CMAKE_MODULE_PATH}
)
include(Header)

project(PsfPlayer)

if(NOT TARGET PsfCore)
	add_subdirectory(
		${CMAKE_CURRENT_SOURCE_DIR}/../
		${CMAKE_CURRENT_BINARY_DIR}/PsfCore
	)
endif()
list(APPEND PROJECT_LIBS PsfCore)

set(QT_SOURCES
	main.cpp
	mainwindow.cpp
	playlistmodel.cpp
)

set(QT_MOC_HEADERS
	mainwindow.h
	playlistmodel.h
	PlaylistItem.h
)

set(QT_UIS
	Qt_ui/mainwindow.ui
)

set(QT_RESOURCES
)

find_package(Qt5 REQUIRED COMPONENTS Widgets Core)
list(APPEND PROJECT_LIBS Qt5::Widgets Qt5::Core)
add_definitions(${QT_DEFINITIONS})

QT5_ADD_RESOURCES(QT_RES_SOURCES ${QT_RESOURCES})
QT5_WRAP_UI(QT_UI_HEADERS ${QT_UIS})
QT5_WRAP_CPP(QT_MOC_SRCS ${QT_MOC_HEADERS})

add_executable(PsfPlayer ${QT_SOURCES} ${QT_MOC_SRCS} ${QT_RES_SOURCES} ${QT_UI_HEADERS})
target_link_libraries(PsfPlayer ${PROJECT_LIBS})
target_include_directories(PsfPlayer PUBLIC
	./
	${CMAKE_CURRENT_BINARY_DIR}
)
