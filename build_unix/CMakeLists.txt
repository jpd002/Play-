cmake_minimum_required(VERSION 2.8)
project(Play)
set(PROJECT_NAME "Play!")
set(PROJECT_Version 0.30)
add_definitions(-DPLAY_VERSION="0.30")

if (NOT TARGET Framework)
	add_subdirectory(
		${Play_SOURCE_DIR}/../../Framework/build_unix
		${Play_SOURCE_DIR}/../../Framework/build_unix/build
	)
endif()

if (NOT TARGET CodeGen)
	add_subdirectory(
		${Play_SOURCE_DIR}/../../CodeGen/build_unix
		${Play_SOURCE_DIR}/../../CodeGen/build_unix/build
	)
endif()

set(CMAKE_MODULE_PATH
	${Play_SOURCE_DIR}/Modules
	${CMAKE_MODULE_PATH}
)

if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release CACHE STRING
		"Choose the type of build, options are: None Debug Release"
		FORCE)
endif()

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D_DEBUG")
if(APPLE)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -x objective-c++")
endif(APPLE)
add_definitions(-D_IOP_EMULATE_MODULES)

set(PROJECT_LIBS)

set(Boost_FIND_REQUIRED TRUE)
find_package(Boost COMPONENTS system filesystem chrono)
include_directories(${Boost_INCLUDE_DIRS})
list(APPEND PROJECT_LIBS ${Boost_LIBRARIES})

find_package(OpenAL REQUIRED)
list(APPEND PROJECT_LIBS ${OPENAL_LIBRARY})

find_package(OpenGL REQUIRED)
list(APPEND PROJECT_LIBS ${OPENGL_LIBRARIES})

find_package(GLEW REQUIRED)
list(APPEND PROJECT_LIBS ${GLEW_LIBRARIES})

find_package(BZip2 REQUIRED)
list(APPEND PROJECT_LIBS ${BZIP2_LIBRARIES})

find_package(ZLIB REQUIRED)
list(APPEND PROJECT_LIBS ${ZLIB_LIBRARIES})

find_package(Threads REQUIRED)
if(CMAKE_THREAD_LIBS_INIT)
       list(APPEND PROJECT_LIBS "${CMAKE_THREAD_LIBS_INIT}")
endif()

if (UNIX AND NOT APPLE)
find_package(ICUUC REQUIRED)
list(APPEND PROJECT_LIBS ${ICUUC_LIBRARIES})
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
	../Source/saves/Icon.cpp 
	../Source/saves/MaxSaveImporter.cpp 
	../Source/saves/PsuSaveImporter.cpp 
	../Source/saves/Save.cpp 
	../Source/saves/SaveExporter.cpp 
	../Source/saves/SaveImporterBase.cpp 
	../Source/saves/SaveImporter.cpp 
	../Source/saves/XpsSaveImporter.cpp
	../Source/ScopedVmPauser.cpp
	../Source/StructCollectionStateFile.cpp 
	../Source/StructFile.cpp 
	../Source/Utils.cpp
	../tools/PsfPlayer/Source/SH_OpenAL.cpp
)

if (WIN32)
	set(PLATFORM_SPECIFIC_SRC_FILES ../Source/VolumeStream.cpp)
endif (WIN32)

if (UNIX)
    set(PLATFORM_SPECIFIC_SRC_FILES ../Source/Posix_VolumeStream.cpp)
endif (UNIX)

if (APPLE)
	set(PLATFORM_SPECIFIC_SRC_FILES ../Source/Posix_VolumeStream.cpp)
endif (APPLE)

add_library(Play ${COMMON_SRC_FILES} ${PLATFORM_SPECIFIC_SRC_FILES})

if(THREADS_HAVE_PTHREAD_ARG)
	target_compile_options(PUBLIC Play "-pthread")
endif()

target_link_libraries(Play Framework CodeGen ${PROJECT_LIBS})

enable_testing()

add_executable(autotest
	../tools/AutoTest/JUnitTestReportWriter.cpp
	../tools/AutoTest/Main.cpp
)
target_link_libraries(autotest Play)

add_executable(McServTest
	../tools/McServTest/AppConfig.cpp
	../tools/McServTest/GameTestSheet.cpp
	../tools/McServTest/Main.cpp
)
target_link_libraries(McServTest Play)
add_test(NAME McServTest
	WORKING_DIRECTORY ${Play_SOURCE_DIR}/../tools/McServTest
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
target_link_libraries(VuTest Play)
add_test(NAME VuTest
	COMMAND VuTest
)

if (UNIX AND NOT APPLE)
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

	add_executable(Play-UI ${QT_SOURCES} ${QT_MOC_SRCS} ${QT_RES_SOURCES} ${QT_UI_HEADERS})
	target_link_libraries(Play-UI Play Framework CodeGen ${PROJECT_LIBS})
endif (UNIX AND NOT APPLE)

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
	add_executable(Play-UI MACOSX_BUNDLE ${OSX_SOURCES} ${OSX_HEADERS})

	# Probably a better way to set the framework link libraries.
	target_link_libraries(Play-UI Play Framework CodeGen ${PROJECT_LIBS} "-ObjC -framework Cocoa -framework OpenGL -framework OpenAL -framework IOKit  -framework AppKit  -framework CoreData  -framework Foundation")

	# Set a custom plist file for the app bundle
	# NOTE: for these values to be used Info.plist has to be edited
	set_target_properties(
		Play-UI
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
			MACOSX_BUNDLE_INFO_PLIST "${Play_SOURCE_DIR}/../Source/ui_macosx/Info.plist"
)

	# Make sure we can find the 'ibtool' program. If we can NOT find it we
	# skip generation of this project
	find_program(IBTOOL ibtool HINTS "/usr/bin" "${OSX_DEVELOPER_ROOT}/usr/bin")
	if (${IBTOOL} STREQUAL "IBTOOL-NOTFOUND")
		message(SEND_ERROR "ibtool can not be found and is needed to compile the .xib files. It should have been installed with the Apple developer tools. The default system paths were searched in addition to ${OSX_DEVELOPER_ROOT}/usr/bin")
	endif()

	# Make sure the 'Resources' Directory is correctly created before we build
	add_custom_command (TARGET Play-UI PRE_BUILD COMMAND mkdir -p ${CMAKE_CFG_INTDIR}/Play-UI.app/Contents/Resources)

	# Copy Icon and patches
	add_custom_command (TARGET Play-UI POST_BUILD 
		COMMAND cp ${Play_SOURCE_DIR}/../build_macosx/AppIcon.icns ${CMAKE_CFG_INTDIR}/Play.app/Contents/Resources/AppIcon.icns
	)
	add_custom_command (TARGET Play-UI POST_BUILD 
		COMMAND cp ${Play_SOURCE_DIR}/../patches.xml ${CMAKE_CFG_INTDIR}/Play.app/Contents/Resources/patches.xml
	)

	# Compile the .xib files using the 'ibtool' program with the destination being the app package
	foreach(xib ${OSX_XIBS})
		add_custom_command (TARGET Play-UI POST_BUILD 
				COMMAND ${IBTOOL} --errors --warnings --notices --output-format human-readable-text 
					--compile ${CMAKE_CFG_INTDIR}/Play-UI.app/Contents/Resources/${xib}.nib
					${Play_SOURCE_DIR}/../Source/ui_macosx/${xib}.xib
				COMMENT "${Play_SOURCE_DIR}/../Source/ui_macosx/${xib}.xib")

	endforeach()
endif(APPLE)