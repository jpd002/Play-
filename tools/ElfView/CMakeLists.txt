cmake_minimum_required(VERSION 3.5)

set(CMAKE_MODULE_PATH
    ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/Dependencies/cmake-modules
    ${CMAKE_MODULE_PATH}
)
include(Header)

project(ElfView)

if(NOT TARGET PlayCore)
    add_subdirectory(
        ${CMAKE_CURRENT_SOURCE_DIR}/../../Source
        ${CMAKE_CURRENT_BINARY_DIR}/PlayCore
    )
endif()
list(APPEND PROJECT_LIBS PlayCore)

set(QT_COMPONENTS Widgets Core)
find_package(Qt6 COMPONENTS ${QT_COMPONENTS})
if(NOT Qt6_FOUND)
	find_package(Qt5 REQUIRED COMPONENTS ${QT_COMPONENTS})
endif()
list(APPEND PROJECT_LIBS Qt::Widgets Qt::Core)

set(DEBUG_SUPPORT_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../../Source/ui_qt/DebugSupport)

set(ELFVIEW_SRC
	${DEBUG_SUPPORT_ROOT}/DebugExpressionEvaluator.cpp
	${DEBUG_SUPPORT_ROOT}/DebugExpressionEvaluator.h
	${DEBUG_SUPPORT_ROOT}/DebugSupportSettings.cpp
	${DEBUG_SUPPORT_ROOT}/DebugSupportSettings.h
	${DEBUG_SUPPORT_ROOT}/DebugUtils.cpp
	${DEBUG_SUPPORT_ROOT}/DebugUtils.h
	${DEBUG_SUPPORT_ROOT}/ELFHeaderView.cpp
	${DEBUG_SUPPORT_ROOT}/ELFHeaderView.h
	${DEBUG_SUPPORT_ROOT}/ELFProgramView.cpp
	${DEBUG_SUPPORT_ROOT}/ELFProgramView.h
	${DEBUG_SUPPORT_ROOT}/ELFSectionView.cpp
	${DEBUG_SUPPORT_ROOT}/ELFSectionView.h
	${DEBUG_SUPPORT_ROOT}/ELFSymbolView.cpp
	${DEBUG_SUPPORT_ROOT}/ELFSymbolView.h
	${DEBUG_SUPPORT_ROOT}/ELFView.cpp
	${DEBUG_SUPPORT_ROOT}/ELFView.h
	${DEBUG_SUPPORT_ROOT}/MemoryViewTable.cpp
	${DEBUG_SUPPORT_ROOT}/MemoryViewTable.h
	${DEBUG_SUPPORT_ROOT}/QtMemoryViewModel.cpp
	${DEBUG_SUPPORT_ROOT}/QtMemoryViewModel.h
	ElfViewEx.cpp
	ElfViewEx.h
	main.cpp
	mainwindow.cpp
	mainwindow.h
)

set(ELFVIEW_MOC_HEADERS
	${DEBUG_SUPPORT_ROOT}/QtMemoryViewModel.h
	mainwindow.h
)

set(ELFVIEW_UI
	qt_ui/mainwindow.ui
)

set(QT_RESOURCES
	qt_ui/resources.qrc
)

QT_ADD_RESOURCES(QT_RES_SOURCES ${QT_RESOURCES})
QT_WRAP_UI(ELFVIEW_UI_HEADERS ${ELFVIEW_UI})
QT_WRAP_CPP(ELFVIEW_MOC_SRCS ${ELFVIEW_MOC_HEADERS})

add_executable(ElfView WIN32 ${ELFVIEW_SRC} ${ELFVIEW_MOC_SRCS} ${QT_RES_SOURCES} ${ELFVIEW_UI_HEADERS})
target_link_libraries(ElfView PRIVATE ${PROJECT_LIBS})
target_include_directories(ElfView PRIVATE ${CMAKE_CURRENT_BINARY_DIR} ${DEBUG_SUPPORT_ROOT})

if(TARGET_PLATFORM_WIN32)
	find_program(WINDEPLOYQT_EXE windeployqt HINTS "${QT_BINARY_DIRECTORY}")
	add_custom_command(TARGET ElfView POST_BUILD
		COMMAND "${WINDEPLOYQT_EXE}" $<IF:$<CONFIG:Debug>,--debug,--release>
									 --no-translations
									 --no-compiler-runtime
									 "$<TARGET_FILE:ElfView>"
	)
endif()
