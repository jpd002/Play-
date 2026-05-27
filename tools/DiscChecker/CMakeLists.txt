cmake_minimum_required(VERSION 3.10.0)

set(CMAKE_MODULE_PATH
	${CMAKE_CURRENT_SOURCE_DIR}/../Play/deps/Dependencies/cmake-modules
	${CMAKE_MODULE_PATH}
)
include(Header)

project(DiscChecker)

if(NOT TARGET PlayCore)
	add_subdirectory(
		${CMAKE_CURRENT_SOURCE_DIR}/../Source
		${CMAKE_CURRENT_BINARY_DIR}/PlayCore
	)
endif()
list(APPEND DISC_CHECKER_PROJECT_LIBS PlayCore)

set(QT_COMPONENTS Widgets Core)

find_package(Qt6 COMPONENTS ${QT_COMPONENTS})
if(NOT Qt6_FOUND)
	if(TARGET_PLATFORM_UNIX)
		list(APPEND QT_COMPONENTS X11Extras)
	endif()

	find_package(Qt5 REQUIRED COMPONENTS ${QT_COMPONENTS})
endif()

list(APPEND DISC_CHECKER_PROJECT_LIBS Qt::Widgets Qt::Core)
if(TARGET_PLATFORM_UNIX AND Qt5_FOUND)
	list(APPEND DISC_CHECKER_PROJECT_LIBS Qt::X11Extras)
endif()

add_definitions(${QT_DEFINITIONS})

set(QT_RESOURCES
)

set(QT_SOURCES
	DiscUtils.cpp
	DiscUtils.h
	Main.cpp
	MainWindow.cpp
	MainWindow.h
	ProgressDialog.cpp
	ProgressDialog.h
	../../Source/ui_qt/QtUtils.cpp
	../../Source/ui_qt/QtUtils.h
)

set(QT_MOC_HEADERS
	MainWindow.h
	ProgressDialog.h
)

set(QT_UIS
	MainWindow.ui
	ProgressDialog.ui
)

QT_ADD_RESOURCES(QT_RES_SOURCES ${QT_RESOURCES})
QT_WRAP_UI(QT_UI_HEADERS ${QT_UIS})
QT_WRAP_CPP(QT_MOC_SRCS ${QT_MOC_HEADERS})

add_executable(${PROJECT_NAME} ${QT_SOURCES} ${QT_MOC_SRCS} ${QT_RES_SOURCES} ${QT_UI_HEADERS})
target_link_libraries(${PROJECT_NAME} ${DISC_CHECKER_PROJECT_LIBS} Qt::Widgets Qt::Core)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

if(TARGET_PLATFORM_WIN32)
	find_program(WINDEPLOYQT_EXE windeployqt HINTS "${QT_BINARY_DIRECTORY}")
	add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
		COMMAND "${WINDEPLOYQT_EXE}" $<IF:$<CONFIG:Debug>,--debug,--release>
									 --no-translations
									 --no-compiler-runtime
									 "$<TARGET_FILE:${PROJECT_NAME}>"
	)
endif()
