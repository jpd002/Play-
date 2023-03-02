cmake_minimum_required(VERSION 3.5)
project(PlayDebugSupport)

if(CMAKE_VERSION VERSION_LESS "3.7.0")
	set(CMAKE_INCLUDE_CURRENT_DIR ON)
endif()

find_package(Qt5 COMPONENTS Core Widgets REQUIRED)
list(APPEND PLAYDEBUGSUPPORT_LIBS Qt5::Core Qt5::Widgets)

set(CMAKE_MODULE_PATH
	${CMAKE_CURRENT_SOURCE_DIR}/../../../deps/Dependencies/cmake-modules
	${CMAKE_MODULE_PATH}
)

include(Header)

if(NOT TARGET PlayCore)
	add_subdirectory(
		${CMAKE_CURRENT_SOURCE_DIR}/../../../
		${CMAKE_CURRENT_BINARY_DIR}/Source
	)
endif()

if(NOT TARGET Framework_OpenGl)
	add_subdirectory(
		${CMAKE_CURRENT_SOURCE_DIR}/../../../deps/Framework/build_cmake/FrameworkOpenGl
		${CMAKE_CURRENT_BINARY_DIR}/FrameworkOpenGl
	)
endif()
list(APPEND PLAYDEBUGSUPPORT_LIBS Framework_OpenGl)

# We expect USE_GSH_VULKAN to be set by the parent project
if(USE_GSH_VULKAN)
	if(NOT TARGET gsh_vulkan)
		add_subdirectory(
			${CMAKE_CURRENT_SOURCE_DIR}/../gs/GSH_Vulkan
			${CMAKE_CURRENT_BINARY_DIR}/gs/GSH_Vulkan
		)
	endif()
	list(APPEND PLAYDEBUGSUPPORT_LIBS gsh_vulkan)
	list(APPEND PLAYDEBUGSUPPORT_DEFS HAS_GSH_VULKAN=1)
endif()

set(SRC_ROOT ${CMAKE_CURRENT_SOURCE_DIR})
set(SRC_FILES
	${SRC_ROOT}/CallStackWnd.cpp
	${SRC_ROOT}/CallStackWnd.h
	${SRC_ROOT}/DebugExpressionEvaluator.cpp
	${SRC_ROOT}/DebugExpressionEvaluator.h
	${SRC_ROOT}/AddressListViewWnd.cpp
	${SRC_ROOT}/AddressListViewWnd.h
	${SRC_ROOT}/DebugUtils.cpp
	${SRC_ROOT}/DebugUtils.h
	${SRC_ROOT}/DebugSupportSettings.cpp
	${SRC_ROOT}/DebugSupportSettings.h
	${SRC_ROOT}/DebugView.cpp
	${SRC_ROOT}/DebugView.h
	${SRC_ROOT}/DisAsmWnd.cpp
	${SRC_ROOT}/DisAsmWnd.h
	${SRC_ROOT}/FunctionsView.cpp
	${SRC_ROOT}/FunctionsView.h
	${SRC_ROOT}/ELFHeaderView.cpp
	${SRC_ROOT}/ELFHeaderView.h
	${SRC_ROOT}/ELFProgramView.cpp
	${SRC_ROOT}/ELFProgramView.h
	${SRC_ROOT}/ELFSectionView.cpp
	${SRC_ROOT}/ELFSectionView.h
	${SRC_ROOT}/ELFSymbolView.cpp
	${SRC_ROOT}/ELFSymbolView.h
	${SRC_ROOT}/ELFView.cpp
	${SRC_ROOT}/ELFView.h
	${SRC_ROOT}/FrameDebugger/GifPacketView.cpp
	${SRC_ROOT}/FrameDebugger/GifPacketView.h
	${SRC_ROOT}/FrameDebugger/GsContextView.cpp
	${SRC_ROOT}/FrameDebugger/GsContextView.h
	${SRC_ROOT}/FrameDebugger/GsPacketListModel.cpp
	${SRC_ROOT}/FrameDebugger/GsPacketListModel.h
	${SRC_ROOT}/FrameDebugger/GsPacketData.cpp
	${SRC_ROOT}/FrameDebugger/GsPacketData.h
	${SRC_ROOT}/FrameDebugger/GSH_OpenGLQt_Framedebugger.cpp
	${SRC_ROOT}/FrameDebugger/GSH_OpenGLQt_Framedebugger.h
	${SRC_ROOT}/FrameDebugger/GsStateUtils.cpp
	${SRC_ROOT}/FrameDebugger/GsStateUtils.h
	${SRC_ROOT}/FrameDebugger/PixelBufferView.cpp
	${SRC_ROOT}/FrameDebugger/PixelBufferView.h
	${SRC_ROOT}/FrameDebugger/QtFramedebugger.cpp
	${SRC_ROOT}/FrameDebugger/QtFramedebugger.h
	${SRC_ROOT}/FrameDebugger/Vu1ProgramView.cpp
	${SRC_ROOT}/FrameDebugger/Vu1ProgramView.h
	${SRC_ROOT}/FrameDebugger/Vu1Vm.cpp
	${SRC_ROOT}/FrameDebugger/Vu1Vm.h
	${SRC_ROOT}/KernelObjectListView.cpp
	${SRC_ROOT}/KernelObjectListView.h
	${SRC_ROOT}/MemoryViewMIPSWnd.cpp
	${SRC_ROOT}/MemoryViewMIPSWnd.h
	${SRC_ROOT}/MemoryViewTable.cpp
	${SRC_ROOT}/MemoryViewTable.h
	${SRC_ROOT}/QtMemoryViewModel.cpp
	${SRC_ROOT}/QtMemoryViewModel.h
	${SRC_ROOT}/RegViewFPU.cpp
	${SRC_ROOT}/RegViewFPU.h
	${SRC_ROOT}/RegViewGeneral.cpp
	${SRC_ROOT}/RegViewGeneral.h
	${SRC_ROOT}/RegViewPage.cpp
	${SRC_ROOT}/RegViewPage.h
	${SRC_ROOT}/RegViewSCU.cpp
	${SRC_ROOT}/RegViewSCU.h
	${SRC_ROOT}/RegViewVU.cpp
	${SRC_ROOT}/RegViewVU.h
	${SRC_ROOT}/RegViewWnd.cpp
	${SRC_ROOT}/RegViewWnd.h
	${SRC_ROOT}/QtDisAsmTableModel.cpp
	${SRC_ROOT}/QtDisAsmTableModel.h
	${SRC_ROOT}/QtDisAsmVuTableModel.cpp
	${SRC_ROOT}/QtDisAsmVuTableModel.h
	${SRC_ROOT}/QtGenericTableModel.cpp
	${SRC_ROOT}/QtGenericTableModel.h
	${SRC_ROOT}/QtDebugger.cpp
	${SRC_ROOT}/QtDebugger.h
	${SRC_ROOT}/QtDialogListWidget.cpp
	${SRC_ROOT}/QtDialogListWidget.h
)

set(QT_DEBUGGER_MOC_HEADERS
	${SRC_ROOT}/AddressListViewWnd.h
	${SRC_ROOT}/FunctionsView.h
	${SRC_ROOT}/QtDisAsmTableModel.h
	${SRC_ROOT}/QtDisAsmVuTableModel.h
	${SRC_ROOT}/QtGenericTableModel.h
	${SRC_ROOT}/QtDebugger.h
	${SRC_ROOT}/QtDialogListWidget.h
	${SRC_ROOT}/QtMemoryViewModel.h
	${SRC_ROOT}/FrameDebugger/GsPacketListModel.h
	${SRC_ROOT}/FrameDebugger/PixelBufferView.h
	${SRC_ROOT}/FrameDebugger/QtFramedebugger.h
	${SRC_ROOT}/FrameDebugger/Vu1ProgramView.h
)

set(QT_DEBUGGER_UI
	${SRC_ROOT}/Qt_ui/AddressListViewWnd.ui
	${SRC_ROOT}/Qt_ui/MemoryViewMIPSWnd.ui

	${SRC_ROOT}/Qt_ui/QtDebugger.ui
	${SRC_ROOT}/Qt_ui/QtDialogListWidget.ui
	${SRC_ROOT}/FrameDebugger/Qt_ui/QtFramedebugger.ui
)

set(QT_RESOURCES
	../Qt_ui/resources.qrc
)

QT5_ADD_RESOURCES(QT_RES_SOURCES ${QT_RESOURCES})
QT5_WRAP_UI(QT_DEBUGGER_UI_HEADERS ${QT_DEBUGGER_UI})
QT5_WRAP_CPP(QT_DEBUGGER_MOC_SRCS ${QT_DEBUGGER_MOC_HEADERS})

add_library(PlayDebugSupport STATIC ${SRC_FILES} ${QT_DEBUGGER_UI_HEADERS} ${QT_DEBUGGER_MOC_SRCS} ${QT_RES_SOURCES})
target_link_libraries(PlayDebugSupport PlayCore ${PLAYDEBUGSUPPORT_LIBS} Framework_OpenGl Qt5::Core Qt5::Widgets)
target_compile_definitions(PlayDebugSupport PRIVATE NOMINMAX ${PLAYDEBUGSUPPORT_DEFS})
target_include_directories(PlayDebugSupport PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ../../${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})
