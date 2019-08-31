cmake_minimum_required(VERSION 3.5)
project(PlayDebugSupport)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
if(CMAKE_VERSION VERSION_LESS "3.7.0")
	set(CMAKE_INCLUDE_CURRENT_DIR ON)
endif()

find_package(Qt5 COMPONENTS Core Widgets REQUIRED)

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

set(SRC_ROOT ${CMAKE_CURRENT_SOURCE_DIR})
set(SRC_FILES
	#[[	${SRC_ROOT}/CallStackWnd.cpp
	${SRC_ROOT}/CallStackWnd.h
	${SRC_ROOT}/DebugExpressionEvaluator.cpp
	${SRC_ROOT}/DebugExpressionEvaluator.h]]
	${SRC_ROOT}/Debugger.cpp
	${SRC_ROOT}/Debugger.h
	#[[${SRC_ROOT}/Debugger/AddressListViewWnd.cpp
	${SRC_ROOT}/Debugger/AddressListViewWnd.h
	${SRC_ROOT}/DebugUtils.cpp
	${SRC_ROOT}/DebugUtils.h ]]
	${SRC_ROOT}/DebugView.cpp
	${SRC_ROOT}/DebugView.h
	#[[
	${SRC_ROOT}/DirectXControl.cpp
	${SRC_ROOT}/DirectXControl.h
	${SRC_ROOT}/DisAsm.cpp
	${SRC_ROOT}/DisAsm.h
	${SRC_ROOT}/DisAsmVu.cpp
	${SRC_ROOT}/DisAsmVu.h
	${SRC_ROOT}/DisAsmWnd.cpp
	${SRC_ROOT}/DisAsmWnd.h
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
	${SRC_ROOT}/ElfViewRes.h
	${SRC_ROOT}/FunctionsView.cpp
	${SRC_ROOT}/FunctionsView.h
	${SRC_ROOT}/FrameDebugger/FrameDebugger.cpp
	${SRC_ROOT}/FrameDebugger/FrameDebugger.h	
	${SRC_ROOT}/FrameDebugger/GifPacketView.cpp
	${SRC_ROOT}/FrameDebugger/GifPacketView.h
	${SRC_ROOT}/FrameDebugger/GsContextView.cpp
	${SRC_ROOT}/FrameDebugger/GsContextView.h
	${SRC_ROOT}/FrameDebugger/GsContextStateView.cpp
	${SRC_ROOT}/FrameDebugger/GsContextStateView.h
	${SRC_ROOT}/FrameDebugger/GsInputStateView.cpp
	${SRC_ROOT}/FrameDebugger/GsInputStateView.h
	${SRC_ROOT}/FrameDebugger/GsPacketListView.cpp
	${SRC_ROOT}/FrameDebugger/GsPacketListView.h
	${SRC_ROOT}/FrameDebugger/GsStateUtils.cpp
	${SRC_ROOT}/FrameDebugger/GsStateUtils.h
	${SRC_ROOT}/FrameDebugger/PixelBufferView.cpp
	${SRC_ROOT}/FrameDebugger/PixelBufferView.h
	${SRC_ROOT}/FrameDebugger/PixelBufferViewOverlay.cpp
	${SRC_ROOT}/FrameDebugger/PixelBufferViewOverlay.h
	${SRC_ROOT}/FrameDebugger/TabHost.cpp
	${SRC_ROOT}/FrameDebugger/TabHost.h
	${SRC_ROOT}/FrameDebugger/Vu1ProgramView.cpp
	${SRC_ROOT}/FrameDebugger/Vu1ProgramView.h
	${SRC_ROOT}/FrameDebugger/Vu1Vm.cpp
	${SRC_ROOT}/FrameDebugger/Vu1Vm.h
	${SRC_ROOT}/MemoryView.cpp
	${SRC_ROOT}/MemoryView.h
	${SRC_ROOT}/MemoryViewMIPS.cpp
	${SRC_ROOT}/MemoryViewMIPS.h
	${SRC_ROOT}/MemoryViewMIPSWnd.cpp
	${SRC_ROOT}/MemoryViewMIPSWnd.h
	${SRC_ROOT}/MemoryViewPtr.cpp
	${SRC_ROOT}/MemoryViewPtr.h
	${SRC_ROOT}/OptionWnd.cpp
	${SRC_ROOT}/OptionWnd.h
	${SRC_ROOT}/OutputWnd.cpp
	${SRC_ROOT}/OutputWnd.h]]
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
	#[[
	${SRC_ROOT}/ThreadCallStackViewWnd.cpp
	${SRC_ROOT}/ThreadCallStackViewWnd.h
	${SRC_ROOT}/ThreadsViewWnd.cpp
	${SRC_ROOT}/ThreadsViewWnd.h
	${SRC_ROOT}/VirtualMachineStateView.h
	${SRC_ROOT}/WinUtils.cpp
	${SRC_ROOT}/WinUtils.h]]
)

add_library(PlayDebugSupport STATIC ${SRC_FILES})
target_link_libraries(PlayDebugSupport PlayCore Qt5::Core Qt5::Widgets)
target_compile_definitions(PlayDebugSupport PRIVATE NOMINMAX)
target_include_directories(PlayDebugSupport PRIVATE ../../${CMAKE_CURRENT_SOURCE_DIR})
