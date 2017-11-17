cmake_minimum_required(VERSION 2.8)

option(TARGET_IOS "Enable building for iOS" OFF)

# macOS deployment target needs to be set before 'project' to work
if(APPLE AND NOT TARGET_IOS)
	set(CMAKE_OSX_DEPLOYMENT_TARGET "10.9" CACHE STRING "Minimum OS X deployment version")
endif()

project(Play)

if(ANDROID)
	message("-- Generating for Android --")
	set(TARGET_PLATFORM_ANDROID TRUE)
elseif(APPLE AND TARGET_IOS)
	message("-- Generating for iOS --")
	set(TARGET_PLATFORM_IOS TRUE)
elseif(APPLE)
	message("-- Generating for macOS --")
	set(TARGET_PLATFORM_MACOS TRUE)
elseif(WIN32)
	message("-- Generating for Win32 --")
	string(FIND ${CMAKE_GENERATOR} "Win64" HASWIN64)
	if(NOT HASWIN64 EQUAL -1)
		message("-- Arch: x64 --")
		set(TARGET_PLATFORM_WIN32_X64 TRUE)
	else()
		message("-- Arch: x86 --")
		set(TARGET_PLATFORM_WIN32_X86 TRUE)
	endif()
	set(TARGET_PLATFORM_WIN32 TRUE)
else()
	message("-- Generating for Unix compatible platform --")
	set(TARGET_PLATFORM_UNIX TRUE)
endif()

set(PROJECT_NAME "Play!")
set(PROJECT_Version 0.30)
add_definitions(-DPLAY_VERSION="${PROJECT_Version}")
set(PROJECT_LIBS)

set(CMAKE_MODULE_PATH
	${CMAKE_CURRENT_SOURCE_DIR}/../../Dependencies/cmake-modules
	${CMAKE_MODULE_PATH}
)
include(PrecompiledHeader)

# flags
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(WIN32)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHsc /MP")
endif()

if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release CACHE STRING
		"Choose the type of build, options are: None Debug Release"
		FORCE)
endif()

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D_DEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DNDEBUG")

if(DEBUGGER_INCLUDED)
	add_definitions(-DDEBUGGER_INCLUDED)
endif()

if(PROFILE)
	add_definitions(-DPROFILE)
endif()

add_definitions(-D_IOP_EMULATE_MODULES)
add_definitions(-DGLEW_STATIC)

if(WIN32)
	add_definitions(-D_CRT_SECURE_NO_WARNINGS)
	add_definitions(-D_SCL_SECURE_NO_WARNINGS)
	add_definitions(-D_LIB)
	add_definitions(-D_UNICODE -DUNICODE)
	if(DEFINED VTUNE_ENABLED)
		add_definitions(-DVTUNE_ENABLED)
		list(APPEND PROJECT_LIBS libittnotify jitprofiling)
		if(DEFINED VTUNE_PATH)
			if(TARGET_PLATFORM_WIN32_X86)
				link_directories($(VTUNE_PATH)\lib32)
			else()
				link_directories($(VTUNE_PATH)\lib64)
			endif()
			include_directories($(VTUNE_PATH)\include)
		else()
			MESSAGE(FATAL_ERROR "VTUNE_PATH was not defined")
		endif()
	endif()
endif()


if (NOT TARGET PlayCore)
add_subdirectory(
	${CMAKE_CURRENT_SOURCE_DIR}/PlayCore
	${CMAKE_CURRENT_BINARY_DIR}/PlayCore
)
endif()
list(APPEND PROJECT_LIBS PlayCore)

if (NOT TARGET Framework_OpenGl)
add_subdirectory(
	${CMAKE_CURRENT_SOURCE_DIR}/../../Framework/build_cmake/FrameworkOpenGl
	${CMAKE_CURRENT_BINARY_DIR}/FrameworkOpenGl
)
endif()
list(APPEND PROJECT_LIBS Framework_OpenGl)

# If ICU is available, add its libraries because Framework might need its functions
find_package(ICUUC)
if(ICUUC_FOUND)
	list(APPEND PROJECT_LIBS ${ICUUC_LIBRARIES})
endif()

if(TARGET_PLATFORM_IOS)
	if(NOT TARGET iCade_Static)
		add_subdirectory(
			${CMAKE_CURRENT_SOURCE_DIR}/../../../Dependencies/build_cmake/iCade-iOS
			${CMAKE_CURRENT_BINARY_DIR}/iCade-iOS
		)
	endif()
	list(APPEND PROJECT_LIBS iCade_Static)
endif()

if(TARGET_PLATFORM_IOS)
	if(NOT TARGET SDWebImage_Static)
		add_subdirectory(
			${CMAKE_CURRENT_SOURCE_DIR}/../../../Dependencies/build_cmake/SDWebImage
			${CMAKE_CURRENT_BINARY_DIR}/SDWebImage
		)
	endif()
	list(APPEND PROJECT_LIBS SDWebImage_Static)
endif()

if(NOT (TARGET_PLATFORM_ANDROID OR TARGET_PLATFORM_IOS))
	find_package(OpenGL REQUIRED)
	list(APPEND PROJECT_LIBS ${OPENGL_LIBRARIES})

	find_package(Threads REQUIRED)
	if(CMAKE_THREAD_LIBS_INIT)
		list(APPEND PROJECT_LIBS "${CMAKE_THREAD_LIBS_INIT}")
	endif()
endif()

if(TARGET_PLATFORM_ANDROID)
	if(NOT TARGET cpufeatures)
		add_library(cpufeatures STATIC ${ANDROID_NDK}/sources/android/cpufeatures/cpu-features.c)
	endif()
	list(APPEND PROJECT_LIBS cpufeatures)
endif()

add_library(gsh_opengl STATIC 
	../Source/gs/GSH_OpenGL/GSH_OpenGL.cpp
	../Source/gs/GSH_OpenGL/GSH_OpenGL.h
	../Source/gs/GSH_OpenGL/GSH_OpenGL_Shader.cpp
	../Source/gs/GSH_OpenGL/GSH_OpenGL_Texture.cpp
)
target_link_libraries(gsh_opengl Boost::boost Framework_OpenGl zlibstatic)
list(INSERT PROJECT_LIBS 0 gsh_opengl)

if(TARGET_PLATFORM_UNIX)
	include_directories(
		../
		../Source/ui_unix/
	)

	set(QT_SOURCES
		../Source/ui_unix/main.cpp
		../Source/ui_unix/mainwindow.cpp
		../Source/ui_unix/ElidedLabel.cpp
		../Source/ui_unix/GSH_OpenGLQt.cpp
		../Source/ui_unix/StatsManager.cpp
		../Source/ui_unix/PH_HidUnix.cpp
		../Source/ui_unix/settingsdialog.cpp
		../Source/ui_unix/openglwindow.cpp
		../Source/ui_unix/memorycardmanagerdialog.cpp
		../Source/ui_unix/vfsmanagerdialog.cpp
		../Source/ui_unix/vfsmodel.cpp
		../Source/ui_unix/vfsdiscselectordialog.cpp
		../Source/ui_unix/VfsDevice.cpp
		../Source/ui_unix/controllerconfigdialog.cpp
		../Source/ui_unix/InputBindingManager.cpp
		../Source/ui_unix/bindingmodel.cpp
		../Source/ui_unix/inputeventselectiondialog.cpp
		../Source/ui_unix/GamePad/GamePadUtils.cpp
		../Source/ui_unix/GamePad/GamePadInputEventListener.cpp
		../Source/ui_unix/GamePad/GamePadDeviceListener.cpp
	)

	set(QT_MOC_HEADERS
		../Source/ui_unix/mainwindow.h
		../Source/ui_unix/ElidedLabel.h
		../Source/ui_unix/GSH_OpenGLQt.h
		../Source/ui_unix/StatsManager.h
		../Source/ui_unix/PH_HidUnix.h
		../Source/ui_unix/settingsdialog.h
		../Source/ui_unix/PreferenceDefs.h
		../Source/ui_unix/openglwindow.h
		../Source/ui_unix/memorycardmanagerdialog.h
		../Source/ui_unix/vfsmanagerdialog.h
		../Source/ui_unix/vfsmodel.h
		../Source/ui_unix/vfsdiscselectordialog.h
		../Source/ui_unix/VfsDevice.h
		../Source/ui_unix/controllerconfigdialog.h
		../Source/ui_unix/InputBindingManager.h
		../Source/ui_unix/bindingmodel.h
		../Source/ui_unix/inputeventselectiondialog.h
		../Source/ui_unix/GamePad/GamePadUtils.h
		../Source/ui_unix/GamePad/GamePadInputEventListener.h
		../Source/ui_unix/GamePad/GamePadDeviceListener.h
	)

	set(QT_UIS
		../build_unix/mainwindow.ui
		../build_unix/settingsdialog.ui
		../build_unix/memorycardmanager.ui
		../build_unix/vfsmanagerdialog.ui
		../build_unix/vfsdiscselectordialog.ui
		../build_unix/controllerconfigdialog.ui
		../build_unix/inputeventselectiondialog.ui
	)

	set(QT_RESOURCES
		../build_unix/resources.qrc
	)

	find_package(Qt5 REQUIRED COMPONENTS Widgets Core)
	list(APPEND PROJECT_LIBS Qt5::Widgets Qt5::Core)
	add_definitions(${QT_DEFINITIONS})
	include_directories(${CMAKE_BINARY_DIR})

	QT5_ADD_RESOURCES(QT_RES_SOURCES ${QT_RESOURCES})
	QT5_WRAP_UI(QT_UI_HEADERS ${QT_UIS})
	QT5_WRAP_CPP(QT_MOC_SRCS ${QT_MOC_HEADERS})

	add_executable(Play ${QT_SOURCES} ${QT_MOC_SRCS} ${QT_RES_SOURCES} ${QT_UI_HEADERS})
	target_link_libraries(Play ${PROJECT_LIBS})
endif(TARGET_PLATFORM_UNIX)

if(TARGET_PLATFORM_MACOS)

	include_directories(
		../
		../Source/ui_macosx/
	)

	set (OSX_SOURCES
		../Source/ui_macosx/ApplicationDelegate.mm
		../Source/ui_macosx/AudioSettingsViewController.mm
		../Source/ui_macosx/Globals.cpp
		../Source/ui_macosx/GSH_OpenGLMacOSX.cpp
		../Source/ui_macosx/main.mm
		../Source/ui_macosx/OutputWindowController.mm
		../Source/ui_macosx/OutputWindow.mm
		../Source/ui_macosx/PH_HidMacOSX.cpp
		../Source/ui_macosx/PreferencesWindowController.mm
		../Source/ui_macosx/VfsManagerBindings.mm
		../Source/ui_macosx/VfsManagerViewController.mm
		../Source/ui_macosx/VideoSettingsViewController.mm
	)

	set (OSX_HEADERS
		../Source/ui_macosx/Purei_Prefix.pch
		../Source/ui_macosx/ApplicationDelegate.h
		../Source/ui_macosx/AudioSettingsViewController.h
		../Source/ui_macosx/Globals.h
		../Source/ui_macosx/GSH_OpenGLMacOSX.h
		../Source/ui_macosx/OutputWindowController.h
		../Source/ui_macosx/OutputWindow.h
		../Source/ui_macosx/PH_HidMacOSX.h
		../Source/ui_macosx/PreferenceDefs.h
		../Source/ui_macosx/PreferencesWindowController.h
		../Source/ui_macosx/VfsManagerBindings.h
		../Source/ui_macosx/VfsManagerViewController.h
		../Source/ui_macosx/VideoSettingsViewController.h
	)

	# these are the OS X Interface Builder Files
	set (OSX_XIBS
		English.lproj/PreferencesWindow
		English.lproj/MainMenu
		English.lproj/VideoSettingsView
		English.lproj/OutputWindow
		English.lproj/AudioSettingsView
		English.lproj/VfsManagerView
	)

	set(OSX_RES
		${CMAKE_CURRENT_SOURCE_DIR}/../Source/ui_macosx/AppIcon.icns
		${CMAKE_CURRENT_SOURCE_DIR}/../patches.xml
	)
	# Add our Executable
	add_executable(Play MACOSX_BUNDLE ${OSX_SOURCES} ${OSX_HEADERS} ${OSX_RES})
	set_target_properties(Play PROPERTIES COMPILE_FLAGS "-x objective-c++ -fobjc-arc")

	# Probably a better way to set the framework link libraries.
	target_link_libraries(Play ${PROJECT_LIBS} "-ObjC -framework Cocoa -framework OpenGL -framework OpenAL -framework IOKit -framework AppKit -framework CoreData -framework Foundation")

	# Set a custom plist file for the app bundle
	# NOTE: for these values to be used Info.plist has to be edited
	# NOTE: from cmake 3.7.0 you can use %b for month name abbreviations
	string(TIMESTAMP DATE "%d-%m-%Y")
	set_target_properties(
		Play
		PROPERTIES
			MACOSX_BUNDLE_INFO_STRING "${PROJECT_NAME}"
			MACOSX_BUNDLE_GUI_IDENTIFIER "com.virtualapplications.Play"
			MACOSX_BUNDLE_LONG_VERSION_STRING "${PROJECT_NAME} Version ${PROJECT_Version}"
			MACOSX_BUNDLE_BUNDLE_NAME ${PROJECT_NAME}
			MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_Version}"
			MACOSX_BUNDLE_BUNDLE_VERSION ${DATE}
			MACOSX_BUNDLE_COPYRIGHT "© Virtual Applications, 2017"
			MACOSX_BUNDLE_ICON_FILE "AppIcon.icns"
			MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/../Source/ui_macosx/Info.plist.in"
		RESOURCE "${OSX_RES}"
	)

	# Make sure we can find the 'ibtool' program. If we can NOT find it we
	# skip generation of this project
	find_program(IBTOOL ibtool HINTS "/usr/bin" "${OSX_DEVELOPER_ROOT}/usr/bin")
	if (${IBTOOL} STREQUAL "IBTOOL-NOTFOUND")
		message(SEND_ERROR "ibtool can not be found and is needed to compile the .xib files. It should have been installed with the Apple developer tools. The default system paths were searched in addition to ${OSX_DEVELOPER_ROOT}/usr/bin")
	endif()

	# Compile the .xib files using the 'ibtool' program with the destination being the app package
	foreach(xib ${OSX_XIBS})
		add_custom_command (TARGET Play POST_BUILD 
				COMMAND ${IBTOOL} --errors --warnings --notices --output-format human-readable-text 
					--compile ${CMAKE_CFG_INTDIR}/Play.app/Contents/Resources/${xib}.nib
					${CMAKE_CURRENT_SOURCE_DIR}/../Source/ui_macosx/${xib}.xib
				COMMENT "${CMAKE_CURRENT_SOURCE_DIR}/../Source/ui_macosx/${xib}.xib")

	endforeach()
endif(TARGET_PLATFORM_MACOS)

if(TARGET_PLATFORM_IOS)
	include_directories(
		../
		../Source/ui_ios/
	)

	find_library(UIKIT_LIBRARY NAMES UIKit)
	find_library(COREGRAPHICS_LIBRARY NAMES CoreGraphics)
	find_library(GAMECONTROLLER_LIBRARY NAMES GameController)
	find_library(QUARTZCORE_LIBRARY NAMES QuartzCore)
	find_library(IMAGEIO_LIBRARY NAMES ImageIO)

	set (OSX_SOURCES
		../Source/ui_ios/AppDelegate.mm
		../Source/ui_ios/BackgroundLayer.m
		../Source/ui_ios/CollectionView.m
		../Source/ui_ios/CoverViewCell.m
		../Source/ui_ios/CoverViewController.m
		../Source/ui_ios/EmulatorViewController.mm
		../Source/ui_ios/GlEsView.mm
		../Source/ui_ios/GSH_OpenGLiOS.cpp
		../Source/ui_ios/IosUtils.cpp
		../Source/ui_ios/main.mm
		../Source/ui_ios/SettingsViewController.mm
		../Source/ui_ios/SqliteDatabase.m
		../Source/ui_ios/VirtualPadButton.mm
		../Source/ui_ios/VirtualPadItem.mm
		../Source/ui_ios/VirtualPadStick.mm
		../Source/ui_ios/VirtualPadView.mm
	)

	set (OSX_HEADERS
		../Source/ui_ios/AppDelegate.h
		../Source/ui_ios/BackgroundLayer.h
		../Source/ui_ios/CollectionView.h
		../Source/ui_ios/CoverViewCell.h
		../Source/ui_ios/CoverViewController.h
		../Source/ui_ios/EmulatorViewController.h
		../Source/ui_ios/GlEsView.h
		../Source/ui_ios/GSH_OpenGLiOS.h
		../Source/ui_ios/IosUtils.h
		../Source/ui_ios/SettingsViewController.h
		../Source/ui_ios/SqliteDatabase.h
		../Source/ui_ios/VirtualPadButton.h
		../Source/ui_ios/VirtualPadItem.h
		../Source/ui_ios/VirtualPadStick.h
		../Source/ui_ios/VirtualPadView.h
	)

	set(OSX_RES
		${CMAKE_CURRENT_SOURCE_DIR}/../games.db
		${CMAKE_CURRENT_SOURCE_DIR}/../patches.xml
		${CMAKE_CURRENT_SOURCE_DIR}/../Source/ui_ios/Base.lproj/Main.storyboard
		${CMAKE_CURRENT_SOURCE_DIR}/../Source/ui_ios/Resources/icon@2x.png
		${CMAKE_CURRENT_SOURCE_DIR}/../Source/ui_ios/Resources/boxart.png
		${CMAKE_CURRENT_SOURCE_DIR}/../Source/ui_ios/Resources/select.png
		${CMAKE_CURRENT_SOURCE_DIR}/../Source/ui_ios/Resources/start.png
		${CMAKE_CURRENT_SOURCE_DIR}/../Source/ui_ios/Resources/up.png
		${CMAKE_CURRENT_SOURCE_DIR}/../Source/ui_ios/Resources/down.png
		${CMAKE_CURRENT_SOURCE_DIR}/../Source/ui_ios/Resources/left.png
		${CMAKE_CURRENT_SOURCE_DIR}/../Source/ui_ios/Resources/right.png
		${CMAKE_CURRENT_SOURCE_DIR}/../Source/ui_ios/Resources/triangle.png
		${CMAKE_CURRENT_SOURCE_DIR}/../Source/ui_ios/Resources/cross.png
		${CMAKE_CURRENT_SOURCE_DIR}/../Source/ui_ios/Resources/circle.png
		${CMAKE_CURRENT_SOURCE_DIR}/../Source/ui_ios/Resources/square.png
		${CMAKE_CURRENT_SOURCE_DIR}/../Source/ui_ios/Resources/lr.png
		${CMAKE_CURRENT_SOURCE_DIR}/../Source/ui_ios/Resources/analogstick.png
	)

	# Add our Executable
	add_executable(Play MACOSX_BUNDLE ${OSX_SOURCES} ${OSX_HEADERS} ${OSX_RES})
	set_target_properties(Play PROPERTIES COMPILE_FLAGS "-x objective-c++ -fobjc-arc")

	# Probably a better way to set the framework link libraries.
	target_link_libraries(Play ${UIKIT_LIBRARY})
	target_link_libraries(Play ${COREGRAPHICS_LIBRARY})
	target_link_libraries(Play ${GAMECONTROLLER_LIBRARY})
	target_link_libraries(Play ${QUARTZCORE_LIBRARY})
	target_link_libraries(Play ${IMAGEIO_LIBRARY})
	target_link_libraries(Play ${PROJECT_LIBS} "-ObjC -lsqlite3 -framework OpenGLES -framework OpenAL")

	# Set a custom plist file for the app bundle
	# NOTE: for these values to be used Info.plist has to be edited
	# NOTE: from cmake 3.7.0 you can use %b for month name abbreviations
	string(TIMESTAMP DATE "%Y-%m-%d")
	set_target_properties(
		Play
		PROPERTIES
			MACOSX_BUNDLE_INFO_STRING "${PROJECT_NAME}"
			MACOSX_BUNDLE_GUI_IDENTIFIER "com.virtualapplications.play"
			MACOSX_BUNDLE_LONG_VERSION_STRING "${PROJECT_NAME} Version ${PROJECT_Version}"
			MACOSX_BUNDLE_BUNDLE_NAME ${PROJECT_NAME}
			MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_Version}"
			MACOSX_BUNDLE_BUNDLE_VERSION ${DATE}
			MACOSX_BUNDLE_COPYRIGHT "© Virtual Applications, 2017"
			MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/../Source/ui_ios/Info.plist.in"
			XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2"
			RESOURCE "${OSX_RES}"
	)

endif(TARGET_PLATFORM_IOS)

if(TARGET_PLATFORM_ANDROID)

	set(ANDROID_SRC_FILES
		../Source/ui_android/GSH_OpenGLAndroid.cpp
		../Source/ui_android/InputManager.cpp
		../Source/ui_android/NativeInterop.cpp
		../Source/ui_android/NativeShared.cpp
		../Source/ui_android/SettingsManager.cpp
		../Source/ui_android/SH_OpenSL.cpp
		../Source/ui_android/StatsManager.cpp
		)
	
	list(APPEND PROJECT_LIBS android log OpenSLES GLESv3 EGL)
	
	add_library(Play SHARED ${ANDROID_SRC_FILES})
	target_link_libraries(Play ${PROJECT_LIBS})
	
endif(TARGET_PLATFORM_ANDROID)

if(TARGET_PLATFORM_WIN32)
	add_library(gsh_opengl_win32 STATIC 
		../Source/ui_win32/GSH_OpenGL_SettingsWnd.cpp
		../Source/ui_win32/GSH_OpenGLWin32.cpp
	)
	target_link_libraries(gsh_opengl_win32 PUBLIC Boost::boost Framework glew_s)
	list(APPEND PROJECT_LIBS gsh_opengl_win32)
	
	add_library(gsh_d3d9 STATIC 
		../Source/ui_win32/GSH_Direct3D9.cpp
		../Source/ui_win32/GSH_Direct3D9_Shader.cpp
		../Source/ui_win32/GSH_Direct3D9_Texture.cpp
	)
	target_link_libraries(gsh_d3d9 Boost::boost Framework zlibstatic Nuanceur)
	list(APPEND PROJECT_LIBS gsh_d3d9)
	
	include_directories(
		../
		../Source/ui_win32/
		../Source/gs/GSH_OpenGL
	)
	if (DEBUGGER_INCLUDED)
		set(DEBUG_SRC
			../Source/ui_win32/CallStackWnd.cpp
			../Source/ui_win32/Debugger.cpp
			../Source/ui_win32/DebugView.cpp
			../Source/ui_win32/FunctionsView.cpp
			../Source/ui_win32/ThreadsViewWnd.cpp

			../Source/ui_win32/ElfViewRes.rc
		)
	endif()
	if(TARGET_PLATFORM_WIN32_X86)
		set(MANIFEST ../Source/ui_win32/Play-x86.manifest)
	else()
		set(MANIFEST ../Source/ui_win32/Play-x64.manifest)
	endif()
	set(WIN32_SRC
		../Source/ui_win32/AboutWnd.cpp
		../Source/ui_win32/AviStream.cpp
		../Source/ui_win32/CdromSelectionWnd.cpp
		../Source/ui_win32/CommandSink.cpp
		../Source/ui_win32/DebugExpressionEvaluator.cpp
		../Source/ui_win32/Debugger/AddressListViewWnd.cpp
		../Source/ui_win32/DebugUtils.cpp
		../Source/ui_win32/DirectXControl.cpp
		../Source/ui_win32/DisAsm.cpp
		../Source/ui_win32/DisAsmVu.cpp
		../Source/ui_win32/DisAsmWnd.cpp
		../Source/ui_win32/ELFHeaderView.cpp
		../Source/ui_win32/ELFProgramView.cpp
		../Source/ui_win32/ELFSectionView.cpp
		../Source/ui_win32/ELFSymbolView.cpp
		../Source/ui_win32/ELFView.cpp
		../Source/ui_win32/FrameDebugger/FrameDebugger.cpp
		../Source/ui_win32/FrameDebugger/GifPacketView.cpp
		../Source/ui_win32/FrameDebugger/GsContextStateView.cpp
		../Source/ui_win32/FrameDebugger/GsContextView.cpp
		../Source/ui_win32/FrameDebugger/GsInputStateView.cpp
		../Source/ui_win32/FrameDebugger/GsPacketListView.cpp
		../Source/ui_win32/FrameDebugger/GsStateUtils.cpp
		../Source/ui_win32/FrameDebugger/PixelBufferView.cpp
		../Source/ui_win32/FrameDebugger/PixelBufferViewOverlay.cpp
		../Source/ui_win32/FrameDebugger/TabHost.cpp
		../Source/ui_win32/FrameDebugger/Vu1ProgramView.cpp
		../Source/ui_win32/FrameDebugger/Vu1Vm.cpp
		../Source/ui_win32/IconMesh.cpp
		../Source/ui_win32/Main.cpp
		../Source/ui_win32/MainWindow.cpp
		../Source/ui_win32/McManagerWnd.cpp
		../Source/ui_win32/MemoryCardView.cpp
		../Source/ui_win32/MemoryView.cpp
		../Source/ui_win32/MemoryViewMIPS.cpp
		../Source/ui_win32/MemoryViewMIPSWnd.cpp
		../Source/ui_win32/MemoryViewPtr.cpp
		../Source/ui_win32/OptionWnd.cpp
		../Source/ui_win32/OutputWnd.cpp
		../Source/ui_win32/PH_DirectInput.cpp
		../Source/ui_win32/PH_DirectInput/ControllerSettingsWnd.cpp
		../Source/ui_win32/PH_DirectInput/InputBindingSelectionWindow.cpp
		../Source/ui_win32/PH_DirectInput/InputManager.cpp
		../Source/ui_win32/RegViewFPU.cpp
		../Source/ui_win32/RegViewGeneral.cpp
		../Source/ui_win32/RegViewPage.cpp
		../Source/ui_win32/RegViewSCU.cpp
		../Source/ui_win32/RegViewVU.cpp
		../Source/ui_win32/RegViewWnd.cpp
		../Source/ui_win32/SaveIconView.cpp
		../Source/ui_win32/SaveView.cpp
		../Source/ui_win32/StatsOverlayWindow.cpp
		../Source/ui_win32/StdAfx.cpp
		../Source/ui_win32/SysInfoWnd.cpp
		../Source/ui_win32/ThreadCallStackViewWnd.cpp
		../Source/ui_win32/VFSManagerWnd.cpp
		../Source/ui_win32/VirtualPad/VirtualPadButton.cpp
		../Source/ui_win32/VirtualPad/VirtualPadItem.cpp
		../Source/ui_win32/VirtualPad/VirtualPadStick.cpp
		../Source/ui_win32/VirtualPad/VirtualPadWindow.cpp
		../Source/ui_win32/WinUtils.cpp
		../tools/PsfPlayer/Source/win32_ui/SH_WaveOut.cpp
		
		${DEBUG_SRC}
		${MANIFEST}
	)

	add_executable(Play WIN32 ${WIN32_SRC} ../Source/ui_win32/Res.rc)
	target_link_libraries(Play PUBLIC ${PROJECT_LIBS})
	add_precompiled_header(Play StdAfx.h FORCEINCLUDE SOURCE_CXX ../Source/ui_win32/StdAfx.cpp)
endif(TARGET_PLATFORM_WIN32)

enable_testing()

add_executable(autotest
	../tools/AutoTest/JUnitTestReportWriter.cpp
	../tools/AutoTest/Main.cpp
)
target_link_libraries(autotest ${PROJECT_LIBS})

add_executable(McServTest
	../tools/McServTest/AppConfig.cpp
	../tools/McServTest/GameTestSheet.cpp
	../tools/McServTest/Main.cpp
)
target_link_libraries(McServTest ${PROJECT_LIBS})

add_test(NAME McServTest
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../tools/McServTest
	COMMAND McServTest
)

add_executable(VuTest
	../tools/VuTest/AddTest.cpp
	../tools/VuTest/FlagsTest2.cpp
	../tools/VuTest/FlagsTest.cpp
	../tools/VuTest/Main.cpp
	../tools/VuTest/TestVm.cpp
	../tools/VuTest/TriAceTest.cpp
	../tools/VuTest/VuAssembler.cpp
)
target_link_libraries(VuTest ${PROJECT_LIBS})
add_test(NAME VuTest
	COMMAND VuTest
)
