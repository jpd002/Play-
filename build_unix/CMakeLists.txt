cmake_minimum_required(VERSION 2.8)
project(Play)
set(PROJECT_NAME "Play!")
set(PROJECT_Version 0.30)
add_definitions(-DPLAY_VERSION="${PROJECT_Version}")
set(PROJECT_LIBS)

set(CMAKE_MODULE_PATH
	${CMAKE_CURRENT_SOURCE_DIR}/../../Dependencies/cmake-modules
	${CMAKE_MODULE_PATH}
)

# flags
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(WIN32)
	set(CMAKE_CXX_FLAGS "/EHsc")
endif()

if(APPLE)
	set(CXX_FLAGS "${CMAKE_CXX_FLAGS} -x objective-c++")
endif()

if(ANDROID)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
	include_directories("${CMAKE_ANDROID_NDK}/sources/android/cpufeatures/")
endif()

if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release CACHE STRING
		"Choose the type of build, options are: None Debug Release"
		FORCE)
endif()

# definitions
MESSAGE("-- Build type: ${CMAKE_BUILD_TYPE}")
if(NOT MSVC)
	if(CMAKE_BUILD_TYPE STREQUAL "Release")
		add_definitions(-DNDEBUG)
	else()
		add_definitions(-D_DEBUG)
	endif()
endif()

add_definitions(-D_IOP_EMULATE_MODULES)
add_definitions(-DGLEW_STATIC)

if(WIN32)
	add_definitions(-DWIN32) #TODO: replace WIN32 preprocessors with _WIN32
	add_definitions(-D_CRT_SECURE_NO_WARNINGS)
	add_definitions(-D_SCL_SECURE_NO_WARNINGS)
	add_definitions(-D_LIB)
	add_definitions(-D_UNICODE -DUNICODE)
	if(DEFINED PROFILE)
		add_definitions(-DPROFILE)
	endif()
	if(DEFINED VTUNE_ENABLED)
		add_definitions(-DVTUNE_ENABLED)
		list(APPEND PROJECT_LIBS libittnotify jitprofiling)
		if(DEFINED VTUNE_PATH)
			if(ARCH STREQUAL "x86")
				include_directories($(VTUNE_PATH)\lib32)
			else()
				include_directories($(VTUNE_PATH)\lib64)
			endif()
		else()
			MESSAGE(FATAL_ERROR "VTUNE_PATH was not defined")
		endif()
	endif()
endif()

if(ANDROID)
	add_definitions(-DGLES_COMPATIBILITY)
	add_definitions(-DDISABLE_LOGGING)
endif()

# targets, packages and dependencies
if (NOT TARGET Framework)
	add_subdirectory(
		${CMAKE_CURRENT_SOURCE_DIR}/../../Framework/build_unix/Framework
		${CMAKE_CURRENT_SOURCE_DIR}/../../Framework/build_unix/Framework/build_${CMAKE_SYSTEM_NAME}
	)
endif()
list(APPEND PROJECT_LIBS Framework)

if (NOT TARGET Framework_OpenGl)
	add_subdirectory(
		${CMAKE_CURRENT_SOURCE_DIR}/../../Framework/build_unix/FrameworkOpenGl
		${CMAKE_CURRENT_SOURCE_DIR}/../../Framework/build_unix/FrameworkOpenGl/build_${CMAKE_SYSTEM_NAME}
	)
endif()
list(APPEND PROJECT_LIBS Framework_OpenGl)

if(UNIX OR APPLE AND NOT ANDROID)
	if (NOT TARGET Framework_OpenAl)
		add_subdirectory(
			${CMAKE_CURRENT_SOURCE_DIR}/../../Framework/build_unix/FrameworkOpenAl
			${CMAKE_CURRENT_SOURCE_DIR}/../../Framework/build_unix/FrameworkOpenAl/build_${CMAKE_SYSTEM_NAME}
		)
	endif()
	list(APPEND PROJECT_LIBS Framework_OpenAl)
	find_package(OpenAL REQUIRED)
	list(APPEND PROJECT_LIBS ${OPENAL_LIBRARY})
endif()

if (NOT TARGET CodeGen)
	add_subdirectory(
		${CMAKE_CURRENT_SOURCE_DIR}/../../CodeGen/build_unix
		${CMAKE_CURRENT_SOURCE_DIR}/../../CodeGen/build_unix/build_${CMAKE_SYSTEM_NAME}
	)
endif()
list(APPEND PROJECT_LIBS CodeGen)

if(WIN32)
	if (NOT TARGET Nuanceur)
		add_subdirectory(
			${CMAKE_CURRENT_SOURCE_DIR}/../../Nuanceur/build_unix
			${CMAKE_CURRENT_SOURCE_DIR}/../../Nuanceur/build_unix/build_${CMAKE_SYSTEM_NAME}
		)
	endif()
	include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../Nuanceur/include)
	list(APPEND PROJECT_LIBS Nuanceur)
endif()

if (NOT TARGET Boost::boost)
	add_subdirectory(
		${CMAKE_CURRENT_SOURCE_DIR}/../../Dependencies/boost-cmake
		${CMAKE_CURRENT_SOURCE_DIR}/../../Dependencies/boost-cmake/build_${CMAKE_SYSTEM_NAME}
	)
endif()
list(APPEND PROJECT_LIBS Boost::boost Boost::system Boost::filesystem Boost::chrono)

if(UNIX OR WIN32 AND NOT ANDROID AND NOT APPLE)
	if (NOT TARGET glew_s)
		set(BUILD_SHARED_LIBS off)
		set(BUILD_UTILS off)
		string(REGEX REPLACE "/RTC(su|[1su])" "" CMAKE_C_FLAGS_DEBUG ${CMAKE_C_FLAGS_DEBUG})
		add_subdirectory(
			${CMAKE_CURRENT_SOURCE_DIR}/../../Dependencies/glew
			${CMAKE_CURRENT_SOURCE_DIR}/../../Dependencies/glew/build_${CMAKE_SYSTEM_NAME}
		)
	endif()
	list(APPEND PROJECT_LIBS glew_s)
	include_directories(${GLEW_SOURCE}/include)
endif()

find_package(BZip2)
if (NOT BZIP2_FOUND)
	MESSAGE("-- Using Provided BZip2 source")
	if (NOT TARGET bz2)
		add_subdirectory(
			${CMAKE_CURRENT_SOURCE_DIR}/../../Dependencies/bzip2-1.0.6
			${CMAKE_CURRENT_SOURCE_DIR}/../../Dependencies/bzip2-1.0.6/build_${CMAKE_SYSTEM_NAME}
		)
	endif()
	include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../Dependencies/bzip2-1.0.6)
	list(APPEND PROJECT_LIBS bz2)
else()
	include_directories(${BZIP2_INCLUDE_DIR})
	list(APPEND PROJECT_LIBS ${BZIP2_LIBRARIES})
endif()

find_package(ZLIB)
if (NOT ZLIB_FOUND)
	MESSAGE("-- Using Provided zlib source")
	if(NOT TARGET zlibstatic)
		add_subdirectory(
			${CMAKE_CURRENT_SOURCE_DIR}/../../Dependencies/zlib-1.2.8
			${CMAKE_CURRENT_SOURCE_DIR}/../../Dependencies/zlib-1.2.8/build_${CMAKE_SYSTEM_NAME}
		)
	endif()
	include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../Dependencies/zlib-1.2.8)
	include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../Dependencies/zlib-1.2.8/build_${CMAKE_SYSTEM_NAME})
	list(APPEND PROJECT_LIBS zlibstatic)
else()
	include_directories(${ZLIB_INCLUDE_DIRS})
	list(APPEND PROJECT_LIBS ${ZLIB_LIBRARIES})
endif()

if(NOT ANDROID)
	find_package(OpenGL REQUIRED)
	list(APPEND PROJECT_LIBS ${OPENGL_LIBRARIES})

	find_package(Threads REQUIRED)
	if(CMAKE_THREAD_LIBS_INIT)
		list(APPEND PROJECT_LIBS "${CMAKE_THREAD_LIBS_INIT}")
	endif()
endif()

if (UNIX AND NOT APPLE AND NOT ANDROID)
	find_package(ICUUC REQUIRED)
	list(APPEND PROJECT_LIBS ${ICUUC_LIBRARIES})
endif()

if(ANDROID)
	if(NOT TARGET cpufeatures)
		add_library(cpufeatures STATIC ${CMAKE_ANDROID_NDK}/sources/android/cpufeatures/cpu-features.c)
	endif()
	list(APPEND PROJECT_LIBS cpufeatures)
	list(APPEND PROJECT_LIBS android log OpenSLES GLESv3 EGL)
endif()

include_directories(../Source ../../Framework/include ../../CodeGen/include)

set(COMMON_SRC_FILES
	../Source/AppConfig.cpp 
	../Source/BasicBlock.cpp 
	../Source/ControllerInfo.cpp 
	../Source/COP_FPU.cpp 
	../Source/COP_FPU_Reflection.cpp 
	../Source/COP_SCU.cpp 
	../Source/COP_SCU_Reflection.cpp 
	../Source/CsoImageStream.cpp 
	../Source/DiskUtils.cpp 
	../Source/ee/COP_VU.cpp 
	../Source/ee/COP_VU_Reflection.cpp 
	../Source/ee/DMAC.cpp 
	../Source/ee/Dmac_Channel.cpp 
	../Source/ee/Ee_SubSystem.cpp 
	../Source/ee/EEAssembler.cpp 
	../Source/ee/EeExecutor.cpp 
	../Source/ee/FpAddTruncate.cpp 
	../Source/ee/FpMulTruncate.cpp 
	../Source/ee/GIF.cpp 
	../Source/ee/INTC.cpp 
	../Source/ee/IPU.cpp 
	../Source/ee/IPU_DmVectorTable.cpp 
	../Source/ee/IPU_MacroblockAddressIncrementTable.cpp 
	../Source/ee/IPU_MacroblockTypeBTable.cpp 
	../Source/ee/IPU_MacroblockTypeITable.cpp 
	../Source/ee/IPU_MacroblockTypePTable.cpp 
	../Source/ee/IPU_MotionCodeTable.cpp 
	../Source/ee/MA_EE.cpp 
	../Source/ee/MA_EE_Reflection.cpp 
	../Source/ee/MA_VU.cpp 
	../Source/ee/MA_VU_Lower.cpp 
	../Source/ee/MA_VU_LowerReflection.cpp 
	../Source/ee/MA_VU_Upper.cpp 
	../Source/ee/MA_VU_UpperReflection.cpp 
	../Source/ee/PS2OS.cpp 
	../Source/ee/SIF.cpp 
	../Source/ee/Timer.cpp 
	../Source/ee/Vif.cpp 
	../Source/ee/Vif1.cpp 
	../Source/ee/Vpu.cpp 
	../Source/ee/VuAnalysis.cpp 
	../Source/ee/VuBasicBlock.cpp 
	../Source/ee/VuExecutor.cpp 
	../Source/ee/VUShared.cpp 
	../Source/ee/VUShared_Reflection.cpp 
	../Source/ELF.cpp 
	../Source/ElfFile.cpp 
	../Source/FrameDump.cpp 
	../Source/gs/GsCachedArea.cpp 
	../Source/gs/GSH_Null.cpp 
	../Source/gs/GSHandler.cpp 
	../Source/gs/GSH_OpenGL/GSH_OpenGL.cpp 
	../Source/gs/GSH_OpenGL/GSH_OpenGL_Shader.cpp 
	../Source/gs/GSH_OpenGL/GSH_OpenGL_Texture.cpp 
	../Source/gs/GsPixelFormats.cpp 
	../Source/iop/ArgumentIterator.cpp 
	../Source/iop/DirectoryDevice.cpp 
	../Source/iop/Iop_Cdvdfsv.cpp 
	../Source/iop/Iop_Cdvdman.cpp 
	../Source/iop/Iop_Dmac.cpp 
	../Source/iop/Iop_DmacChannel.cpp 
	../Source/iop/Iop_Dynamic.cpp 
	../Source/iop/Iop_FileIo.cpp 
	../Source/iop/Iop_FileIoHandler1000.cpp 
	../Source/iop/Iop_FileIoHandler2100.cpp 
	../Source/iop/Iop_FileIoHandler2240.cpp 
	../Source/iop/Iop_Heaplib.cpp
	../Source/iop/Iop_Intc.cpp 
	../Source/iop/Iop_Intrman.cpp 
	../Source/iop/Iop_Ioman.cpp 
	../Source/iop/Iop_LibSd.cpp 
	../Source/iop/Iop_Loadcore.cpp 
	../Source/iop/Iop_McServ.cpp 
	../Source/iop/Iop_Modload.cpp
	../Source/iop/Iop_Module.cpp 
	../Source/iop/Iop_MtapMan.cpp
	../Source/iop/Iop_PadMan.cpp 
	../Source/iop/Iop_RootCounters.cpp 
	../Source/iop/Iop_SifCmd.cpp 
	../Source/iop/Iop_SifDynamic.cpp 
	../Source/iop/Iop_SifMan.cpp 
	../Source/iop/Iop_SifManNull.cpp 
	../Source/iop/Iop_SifManPs2.cpp 
	../Source/iop/Iop_Sio2.cpp 
	../Source/iop/Iop_Spu.cpp 
	../Source/iop/Iop_Spu2.cpp 
	../Source/iop/Iop_Spu2_Core.cpp 
	../Source/iop/Iop_SpuBase.cpp 
	../Source/iop/Iop_Stdio.cpp 
	../Source/iop/Iop_SubSystem.cpp 
	../Source/iop/Iop_Sysclib.cpp 
	../Source/iop/Iop_Sysmem.cpp 
	../Source/iop/Iop_Thbase.cpp 
	../Source/iop/Iop_Thevent.cpp 
	../Source/iop/Iop_Thmsgbx.cpp 
	../Source/iop/Iop_Thsema.cpp 
	../Source/iop/Iop_Thvpool.cpp
	../Source/iop/Iop_Timrman.cpp 
	../Source/iop/Iop_Vblank.cpp 
	../Source/iop/IopBios.cpp 
	../Source/iop/IsoDevice.cpp 
	../Source/ISO9660/DirectoryRecord.cpp 
	../Source/ISO9660/File.cpp 
	../Source/ISO9660/ISO9660.cpp 
	../Source/ISO9660/PathTable.cpp 
	../Source/ISO9660/PathTableRecord.cpp 
	../Source/ISO9660/VolumeDescriptor.cpp 
	../Source/IszImageStream.cpp 
	../Source/Log.cpp 
	../Source/MA_MIPSIV.cpp 
	../Source/MA_MIPSIV_Reflection.cpp 
	../Source/MA_MIPSIV_Templates.cpp 
	../Source/MailBox.cpp 
	../Source/MemoryMap.cpp 
	../Source/MemoryStateFile.cpp 
	../Source/MemoryUtils.cpp 
	../Source/MIPS.cpp 
	../Source/MIPSAnalysis.cpp 
	../Source/MIPSArchitecture.cpp 
	../Source/MIPSAssembler.cpp 
	../Source/MIPSCoprocessor.cpp 
	../Source/MipsExecutor.cpp 
	../Source/MIPSInstructionFactory.cpp 
	../Source/MipsJitter.cpp 
	../Source/MIPSReflection.cpp 
	../Source/MIPSTags.cpp 
	../Source/PadHandler.cpp 
	../Source/PadListener.cpp
	../Source/PH_Generic.cpp 
	../Source/Profiler.cpp 
	../Source/PS2VM.cpp 
	../Source/RegisterStateFile.cpp 
	../Source/ScopedVmPauser.cpp
	../Source/StructCollectionStateFile.cpp 
	../Source/StructFile.cpp 
	../Source/Utils.cpp
	../Source/VirtualPad.cpp
)

if (WIN32)
	set(PLATFORM_SPECIFIC_SRC_FILES ../Source/VolumeStream.cpp)
endif ()

if (UNIX AND NOT APPLE)
    set(PLATFORM_SPECIFIC_SRC_FILES ../Source/Posix_VolumeStream.cpp)
endif ()

if (APPLE)
	set(PLATFORM_SPECIFIC_SRC_FILES ../Source/Posix_VolumeStream.cpp)
endif ()

if(ANDROID)
	set(PLATFORM_SPECIFIC_SRC_FILES
		../Source/ui_android/GSH_OpenGLAndroid.cpp
		../Source/ui_android/InputManager.cpp
		../Source/ui_android/NativeInterop.cpp
		../Source/ui_android/NativeShared.cpp
		../Source/ui_android/SettingsManager.cpp
		../Source/ui_android/SH_OpenSL.cpp
		../Source/ui_android/StatsManager.cpp
		)
endif()

if(UNIX OR APPLE AND NOT ANDROID)
	set(PLATFORM_SPECIFIC_SRC_FILES
		${PLATFORM_SPECIFIC_SRC_FILES}
		../tools/PsfPlayer/Source/SH_OpenAL.cpp
	)
endif()

if(NOT ANDROID)
	set(PLATFORM_SPECIFIC_SRC_FILES
		${PLATFORM_SPECIFIC_SRC_FILES}
		../Source/saves/Icon.cpp
		../Source/saves/MaxSaveImporter.cpp
		../Source/saves/PsuSaveImporter.cpp
		../Source/saves/Save.cpp
		../Source/saves/SaveExporter.cpp
		../Source/saves/SaveImporterBase.cpp
		../Source/saves/SaveImporter.cpp
		../Source/saves/XpsSaveImporter.cpp
	)
endif()

if(ANDROID)
	add_library(PlayCore SHARED ${COMMON_SRC_FILES} ${PLATFORM_SPECIFIC_SRC_FILES})
else()
	add_library(PlayCore STATIC ${COMMON_SRC_FILES} ${PLATFORM_SPECIFIC_SRC_FILES})
endif()


if(NOT ANDROID)
	if(THREADS_HAVE_PTHREAD_ARG)
		target_compile_options(PUBLIC PlayCore "-pthread")
	endif()
endif()

target_link_libraries(PlayCore ${PROJECT_LIBS})
list(APPEND PROJECT_LIBS PlayCore)

#UI
if (UNIX AND NOT APPLE AND NOT ANDROID)
	include_directories(
		../
		../Source/ui_unix/
	)

	set(QT_SOURCES
		../Source/ui_unix/main.cpp
		../Source/ui_unix/mainwindow.cpp
		../Source/ui_unix/GSH_OpenGLQt.cpp
		../Source/ui_unix/StatsManager.cpp
		../Source/ui_unix/PH_HidUnix.cpp
		../Source/ui_unix/settingsdialog.cpp
		../Source/ui_unix/openglwindow.cpp
		../Source/ui_unix/memorycardmanagerdialog.cpp
		../Source/ui_unix/MemoryCard.cpp
		../Source/ui_unix/vfsmanagerdialog.cpp
		../Source/ui_unix/vfsmodel.cpp
		../Source/ui_unix/vfsdiscselectordialog.cpp
		../Source/ui_unix/VfsDevice.cpp
		../Source/ui_unix/controllerconfigdialog.cpp
		../Source/ui_unix/padqwidgetext.cpp
	)

	set(QT_MOC_HEADERS
		../Source/ui_unix/mainwindow.h
		../Source/ui_unix/GSH_OpenGLQt.h
		../Source/ui_unix/StatsManager.h
		../Source/ui_unix/PH_HidUnix.h
		../Source/ui_unix/settingsdialog.h
		../Source/ui_unix/PreferenceDefs.h
		../Source/ui_unix/openglwindow.h
		../Source/ui_unix/memorycardmanagerdialog.h
		../Source/ui_unix/MemoryCard.h
		../Source/ui_unix/vfsmanagerdialog.h
		../Source/ui_unix/vfsmodel.h
		../Source/ui_unix/vfsdiscselectordialog.h
		../Source/ui_unix/VfsDevice.h
		../Source/ui_unix/controllerconfigdialog.h
		../Source/ui_unix/padqwidgetext.h
	)

	set(QT_UIS
		mainwindow.ui
		settingsdialog.ui
		memorycardmanager.ui
		vfsmanagerdialog.ui
		vfsdiscselectordialog.ui
		controllerconfigdialog.ui
	)

	set(QT_RESOURCES
		resources.qrc
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
endif (UNIX AND NOT APPLE AND NOT ANDROID)

if(APPLE)
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

	# Add our Executable
	add_executable(Play MACOSX_BUNDLE ${OSX_SOURCES} ${OSX_HEADERS})
	set_target_properties(Play PROPERTIES COMPILE_FLAGS "-x objective-c++")

	# Probably a better way to set the framework link libraries.
	target_link_libraries(Play ${PROJECT_LIBS} "-ObjC -framework Cocoa -framework OpenGL -framework OpenAL -framework IOKit  -framework AppKit  -framework CoreData  -framework Foundation")

	# Set a custom plist file for the app bundle
	# NOTE: for these values to be used Info.plist has to be edited
	set_target_properties(
		Play
		PROPERTIES
			MACOSX_BUNDLE_INFO_STRING "${PROJECT_NAME}"
			MACOSX_BUNDLE_GUI_IDENTIFIER "com.virtualapplications.Play"
			MACOSX_BUNDLE_LONG_VERSION_STRING "${PROJECT_NAME} Version ${PROJECT_Version}"
			MACOSX_BUNDLE_BUNDLE_NAME ${PROJECT_NAME}
			MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_Version}"
			MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_Version}
			MACOSX_BUNDLE_COPYRIGHT "Copyright (c) 2006-2017, Jean-Philip Desjardins. All Rights Reserved."
			MACOSX_BUNDLE_ICON_FILE "AppIcon.icns"
			MACOSX_BUNDLE_NSMAIN_NIB_FILE "MainMenu"
			MACOSX_BUNDLE_NSPRINCIPAL_CLASS "NSApplication"
			MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/../Source/ui_macosx/Info.plist"
)

	# Make sure we can find the 'ibtool' program. If we can NOT find it we
	# skip generation of this project
	find_program(IBTOOL ibtool HINTS "/usr/bin" "${OSX_DEVELOPER_ROOT}/usr/bin")
	if (${IBTOOL} STREQUAL "IBTOOL-NOTFOUND")
		message(SEND_ERROR "ibtool can not be found and is needed to compile the .xib files. It should have been installed with the Apple developer tools. The default system paths were searched in addition to ${OSX_DEVELOPER_ROOT}/usr/bin")
	endif()

	# Make sure the 'Resources' Directory is correctly created before we build
	add_custom_command (TARGET Play PRE_BUILD COMMAND mkdir -p ${CMAKE_CFG_INTDIR}/Play.app/Contents/Resources)

	# Copy Icon and patches
	add_custom_command (TARGET Play POST_BUILD 
		COMMAND cp ${CMAKE_CURRENT_SOURCE_DIR}/../build_macosx/AppIcon.icns ${CMAKE_CFG_INTDIR}/Play.app/Contents/Resources/AppIcon.icns
	)
	add_custom_command (TARGET Play POST_BUILD 
		COMMAND cp ${CMAKE_CURRENT_SOURCE_DIR}/../patches.xml ${CMAKE_CFG_INTDIR}/Play.app/Contents/Resources/patches.xml
	)

	# Compile the .xib files using the 'ibtool' program with the destination being the app package
	foreach(xib ${OSX_XIBS})
		add_custom_command (TARGET Play POST_BUILD 
				COMMAND ${IBTOOL} --errors --warnings --notices --output-format human-readable-text 
					--compile ${CMAKE_CFG_INTDIR}/Play.app/Contents/Resources/${xib}.nib
					${CMAKE_CURRENT_SOURCE_DIR}/../Source/ui_macosx/${xib}.xib
				COMMENT "${CMAKE_CURRENT_SOURCE_DIR}/../Source/ui_macosx/${xib}.xib")

	endforeach()
endif(APPLE)

if (WIN32)
	ENABLE_LANGUAGE(ASM_MASM)

	include_directories(
		../
		../Source/ui_win32/
		../Source/gs/GSH_OpenGL
	)
	if (CMAKE_BUILD_TYPE MATCHES DEBUG)
		set(DEBUG_SRC
			../Source/ui_win32/CallStackWnd.cpp
			../Source/ui_win32/Debugger.cpp
			../Source/ui_win32/DebugView.cpp
			../Source/ui_win32/FunctionsView.cpp
			../Source/ui_win32/ThreadsViewWnd.cpp
		)
	endif()
	if(ARCH STREQUAL "x86")
		set(MANIFEST ../Source/ui_win32/manifest.xml)
	else()
		set(MANIFEST ../Source/ui_win32/manifest64.xml)
	endif()
	set(WIN32_SRC
		../Source/ui_win32/AboutWnd.cpp
		../Source/ui_win32/AviStream.cpp
		../Source/ui_win32/CdromSelectionWnd.cpp
		../Source/ui_win32/CommandSink.cpp
		../Source/ui_win32/DebugExpressionEvaluator.cpp
		../Source/ui_win32/DebuggerChildWnd.cpp
		../Source/ui_win32/Debugger/FindCallersViewWnd.cpp
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
		../Source/ui_win32/GSH_Direct3D9.cpp
		../Source/ui_win32/GSH_Direct3D9_Shader.cpp
		../Source/ui_win32/GSH_Direct3D9_Texture.cpp
		../Source/ui_win32/GSH_OpenGLWin32.cpp
		../Source/ui_win32/IconMesh.cpp
		../Source/ui_win32/Main.cpp
		../Source/ui_win32/MainWindow.cpp
		../Source/ui_win32/McManagerWnd.cpp
		../Source/ui_win32/MemoryCard.cpp
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
		../Source/ui_win32/GSH_OpenGL_SettingsWnd.cpp
		../Source/ui_win32/SaveIconView.cpp
		../Source/ui_win32/SaveView.cpp
		../Source/ui_win32/StatsOverlayWindow.cpp
		../Source/ui_win32/StdAfx.cpp
		../Source/ui_win32/SysInfo_x64.asm
		../Source/ui_win32/SysInfoWnd.cpp
		../Source/ui_win32/ThreadCallStackViewWnd.cpp
		../Source/ui_win32/VFSManagerWnd.cpp
		../Source/ui_win32/VirtualPad/VirtualPadButton.cpp
		../Source/ui_win32/VirtualPad/VirtualPadItem.cpp
		../Source/ui_win32/VirtualPad/VirtualPadStick.cpp
		../Source/ui_win32/VirtualPad/VirtualPadWindow.cpp
		../Source/ui_win32/WinUtils.cpp
		../tools/PsfPlayer/Source/win32_ui/SH_WaveOut.cpp
		../Source/VirtualPad.cpp
		
		${DEBUG_SRC}
		${MANIFEST}
	)

	add_executable(Play WIN32 ${WIN32_SRC} ../Source/ui_win32/Res.rc)
	target_link_libraries(Play PUBLIC ${PROJECT_LIBS})
	include(PrecompiledHeader)
	set(pch_source ../Source/ui_win32/StdAfx.cpp)
	add_precompiled_header(Play ../Source/ui_win32/StdAfx.h c++-header)
	target_use_precompiled_header(Play ../Source/ui_win32/StdAfx.h c++-header FORCEINCLUDE)
endif (WIN32)

enable_testing()

add_executable(autotest
	../tools/AutoTest/JUnitTestReportWriter.cpp
	../tools/AutoTest/Main.cpp
)
set_target_properties(autotest PROPERTIES COMPILE_FLAGS "${CXX_FLAGS}")
target_link_libraries(autotest PlayCore "-framework Foundation")

add_executable(McServTest
	../tools/McServTest/AppConfig.cpp
	../tools/McServTest/GameTestSheet.cpp
	../tools/McServTest/Main.cpp
)
target_link_libraries(McServTest PlayCore "-framework Foundation")
set_target_properties(McServTest PROPERTIES COMPILE_FLAGS "${CXX_FLAGS}")
target_link_libraries(McServTest PlayCore)

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
target_link_libraries(VuTest PlayCore)
add_test(NAME VuTest
	COMMAND VuTest
)
