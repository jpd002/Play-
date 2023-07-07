cmake_minimum_required(VERSION 3.5)

set(CMAKE_MODULE_PATH
	${CMAKE_CURRENT_SOURCE_DIR}/../../../../deps/Dependencies/cmake-modules
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
	mainwindow.h
	playlistmodel.cpp
	playlistmodel.h
	../../../../Source/ui_qt/QStringUtils.cpp
	../../../../Source/ui_qt/QStringUtils.h
)

set(QT_MOC_HEADERS
	mainwindow.h
	playlistmodel.h
)

set(QT_UIS
	Qt_ui/mainwindow.ui
)

set(QT_RESOURCES
)

if(TARGET_PLATFORM_WIN32)
	set(QT_SOURCES
		${QT_SOURCES}
		../ui_win32/SH_WaveOut.cpp
	)
	list(APPEND PROJECT_LIBS winmm)
endif()

if(TARGET_PLATFORM_MACOS OR TARGET_PLATFORM_UNIX)
	if(NOT TARGET sh_openal)
		add_subdirectory(
			${CMAKE_CURRENT_SOURCE_DIR}/SH_OpenAL
			${CMAKE_CURRENT_BINARY_DIR}/SH_OpenAL
		)
	endif()
	list(APPEND PROJECT_LIBS sh_openal)
endif()

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

if(TARGET_PLATFORM_WIN32)
	find_program(WINDEPLOYQT_EXE windeployqt HINTS "${QT_BINARY_DIRECTORY}")
	add_custom_command(TARGET PsfPlayer POST_BUILD
		COMMAND "${WINDEPLOYQT_EXE}" $<IF:$<CONFIG:Debug>,--debug,--release>
									 --no-translations
									 --no-compiler-runtime
									 "$<TARGET_FILE:PsfPlayer>"
	)
endif()
