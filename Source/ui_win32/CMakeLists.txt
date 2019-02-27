cmake_minimum_required(VERSION 3.5)

set(CMAKE_MODULE_PATH
	${CMAKE_CURRENT_SOURCE_DIR}/../../../Dependencies/cmake-modules
	${CMAKE_MODULE_PATH}
)
include(Header)

project(PlayUI)

if(NOT TARGET PlayCore)
	add_subdirectory(
		${CMAKE_CURRENT_SOURCE_DIR}/../
		${CMAKE_CURRENT_BINARY_DIR}/Source
	)
endif()
list(APPEND PROJECT_LIBS PlayCore)

if(NOT TARGET gsh_opengl)
	add_subdirectory(
		${CMAKE_CURRENT_SOURCE_DIR}/../gs/GSH_OpenGL
		${CMAKE_CURRENT_BINARY_DIR}/gs/GSH_OpenGL
	)
endif()
list(INSERT PROJECT_LIBS 0 gsh_opengl)

if(NOT TARGET gsh_opengl_win32)
	add_library(gsh_opengl_win32 STATIC
		GSH_OpenGL_SettingsWnd.cpp
		GSH_OpenGLWin32.cpp
	)
	target_link_libraries(gsh_opengl_win32 PUBLIC Boost::boost Framework gsh_opengl)
endif()
list(APPEND PROJECT_LIBS gsh_opengl_win32)

if(NOT TARGET gsh_d3d9)
	add_subdirectory(
		${CMAKE_CURRENT_SOURCE_DIR}/GSH_Direct3D9
		${CMAKE_CURRENT_BINARY_DIR}/gs/GSH_Direct3D9
	)
endif()
list(APPEND PROJECT_LIBS gsh_d3d9)

if(TARGET_PLATFORM_WIN32)
	if(NOT TARGET Framework_Win32)
		add_subdirectory(
			${CMAKE_CURRENT_SOURCE_DIR}/../../../Framework/build_cmake/FrameworkWin32
			${CMAKE_CURRENT_BINARY_DIR}/Framework_Win32
		)
	endif()
list(APPEND PROJECT_LIBS Framework_Win32)

list(APPEND PROJECT_LIBS uxtheme opengl32 glu32 dinput8 vfw32 winmm gdiplus Boost::date_time)

endif()

if(DEBUGGER_INCLUDED)
	set(DEBUG_SRC
		CallStackWnd.cpp
		Debugger.cpp
		DebugView.cpp
		FunctionsView.cpp
		ThreadsViewWnd.cpp

		ElfViewRes.rc
	)
endif()

if(TARGET_PLATFORM_WIN32_X86)
	set(MANIFEST Play-x86.manifest)
else()
	set(MANIFEST Play-x64.manifest)
endif()

set(WIN32_SRC
	AboutWnd.cpp
	AviStream.cpp
	CdromSelectionWnd.cpp
	CommandSink.cpp
	DebugExpressionEvaluator.cpp
	Debugger/AddressListViewWnd.cpp
	DebugUtils.cpp
	DirectXControl.cpp
	DisAsm.cpp
	DisAsmVu.cpp
	DisAsmWnd.cpp
	ELFHeaderView.cpp
	ELFProgramView.cpp
	ELFSectionView.cpp
	ELFSymbolView.cpp
	ELFView.cpp
	FrameDebugger/FrameDebugger.cpp
	FrameDebugger/GifPacketView.cpp
	FrameDebugger/GsContextStateView.cpp
	FrameDebugger/GsContextView.cpp
	FrameDebugger/GsInputStateView.cpp
	FrameDebugger/GsPacketListView.cpp
	FrameDebugger/GsStateUtils.cpp
	FrameDebugger/PixelBufferView.cpp
	FrameDebugger/PixelBufferViewOverlay.cpp
	FrameDebugger/TabHost.cpp
	FrameDebugger/Vu1ProgramView.cpp
	FrameDebugger/Vu1Vm.cpp
	IconMesh.cpp
	Main.cpp
	MainWindow.cpp
	McManagerWnd.cpp
	MemoryCardView.cpp
	MemoryView.cpp
	MemoryViewMIPS.cpp
	MemoryViewMIPSWnd.cpp
	MemoryViewPtr.cpp
	OptionWnd.cpp
	OutputWnd.cpp
	PH_DirectInput.cpp
	PH_DirectInput/ControllerSettingsWnd.cpp
	PH_DirectInput/InputBindingSelectionWindow.cpp
	PH_DirectInput/InputManager.cpp
	RegViewFPU.cpp
	RegViewGeneral.cpp
	RegViewPage.cpp
	RegViewSCU.cpp
	RegViewVU.cpp
	RegViewWnd.cpp
	SaveIconView.cpp
	SaveView.cpp
	StatsOverlayWindow.cpp
	StdAfx.cpp
	SysInfoWnd.cpp
	ThreadCallStackViewWnd.cpp
	VFSManagerWnd.cpp
	VirtualPad/VirtualPadButton.cpp
	VirtualPad/VirtualPadItem.cpp
	VirtualPad/VirtualPadStick.cpp
	VirtualPad/VirtualPadWindow.cpp
	WinUtils.cpp
	../../tools/PsfPlayer/Source/win32_ui/SH_WaveOut.cpp

	${DEBUG_SRC}
	${MANIFEST}
)

add_executable(Play WIN32 ${WIN32_SRC} Res.rc)
target_link_libraries(Play PUBLIC ${PROJECT_LIBS})
target_include_directories(Play PRIVATE
	./
)
add_precompiled_header(Play StdAfx.h FORCEINCLUDE SOURCE_CXX StdAfx.cpp)
