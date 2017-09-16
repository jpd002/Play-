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

if(ANDROID OR IOS)
	add_definitions(-DGLES_COMPATIBILITY)
	add_definitions(-DDISABLE_LOGGING)
endif()

# targets, packages and dependencies
if (NOT TARGET Framework)
	add_subdirectory(
		${CMAKE_CURRENT_SOURCE_DIR}/../../Framework/build_cmake/Framework
		${CMAKE_CURRENT_BINARY_DIR}/Framework
	)
endif()
list(APPEND PROJECT_LIBS Framework)

if (NOT TARGET Framework_OpenGl)
	add_subdirectory(
		${CMAKE_CURRENT_SOURCE_DIR}/../../Framework/build_cmake/FrameworkOpenGl
		${CMAKE_CURRENT_BINARY_DIR}/FrameworkOpenGl
	)
endif()
list(APPEND PROJECT_LIBS Framework_OpenGl)

if(TARGET_PLATFORM_UNIX)
        find_package(LIBEVDEV REQUIRED)
		include_directories(${LIBEVDEV_INCLUDE_DIR})
		list(APPEND PROJECT_LIBS ${LIBEVDEV_LIBRARY})
endif()

if(TARGET_PLATFORM_MACOS OR TARGET_PLATFORM_IOS OR TARGET_PLATFORM_UNIX)
	if (NOT TARGET Framework_OpenAl)
		add_subdirectory(
			${CMAKE_CURRENT_SOURCE_DIR}/../../Framework/build_cmake/FrameworkOpenAl
			${CMAKE_CURRENT_BINARY_DIR}/FrameworkOpenAl
		)
	endif()
	list(APPEND PROJECT_LIBS Framework_OpenAl)
	find_package(OpenAL REQUIRED)
	list(APPEND PROJECT_LIBS ${OPENAL_LIBRARY})
endif()

if (NOT TARGET CodeGen)
	add_subdirectory(
		${CMAKE_CURRENT_SOURCE_DIR}/../../CodeGen/build_cmake
		${CMAKE_CURRENT_BINARY_DIR}/CodeGen
	)
endif()
list(APPEND PROJECT_LIBS CodeGen)

if(TARGET_PLATFORM_WIN32)
	if (NOT TARGET Nuanceur)
		add_subdirectory(
			${CMAKE_CURRENT_SOURCE_DIR}/../../Nuanceur/build_cmake
			${CMAKE_CURRENT_BINARY_DIR}/Nuanceur
		)
	endif()
	include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../Nuanceur/include)
	list(APPEND PROJECT_LIBS Nuanceur)

	if (NOT TARGET Framework_Win32)
		add_subdirectory(
			${CMAKE_CURRENT_SOURCE_DIR}/../../Framework/build_cmake/FrameworkWin32
			${CMAKE_CURRENT_BINARY_DIR}/Framework_Win32
		)
	endif()
	include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../Framework/include)
	list(APPEND PROJECT_LIBS Framework_Win32)

	#If building for Windows XP, we need to use the provided d3dcompiler since it's not available in the SDK
	if(CMAKE_GENERATOR_TOOLSET MATCHES "_xp$")
		include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../Dependencies/d3dcompiler_winxp/Include)
		if(CMAKE_SIZEOF_VOID_P EQUAL 8)
			list(APPEND PROJECT_LIBS ${CMAKE_CURRENT_SOURCE_DIR}/../../Dependencies/d3dcompiler_winxp/Lib/x64/d3dcompiler.lib)
		else()
			list(APPEND PROJECT_LIBS ${CMAKE_CURRENT_SOURCE_DIR}/../../Dependencies/d3dcompiler_winxp/Lib/x86/d3dcompiler.lib)
		endif()
	else()
		list(APPEND PROJECT_LIBS d3dcompiler)
	endif()
	
	list(APPEND PROJECT_LIBS comctl32 uxtheme opengl32 glu32 dinput8 d3d9 vfw32 winmm gdiplus Boost::date_time)

endif()

if(NOT TARGET Boost::boost)
	add_subdirectory(
		${CMAKE_CURRENT_SOURCE_DIR}/../../Dependencies/boost-cmake
		${CMAKE_CURRENT_BINARY_DIR}/boost-cmake
	)
endif()
list(APPEND PROJECT_LIBS Boost::boost Boost::system Boost::filesystem)

if(TARGET_PLATFORM_WIN32 OR TARGET_PLATFORM_UNIX)
	if (NOT TARGET glew_s)
		add_subdirectory(
			${CMAKE_CURRENT_SOURCE_DIR}/../../Dependencies/glew-2.0.0
			${CMAKE_CURRENT_BINARY_DIR}/glew-2.0.0
		)
	endif()
	list(APPEND PROJECT_LIBS glew_s)
	include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../Dependencies/glew-2.0.0/include)
endif()

find_package(BZip2)
if(NOT BZIP2_FOUND)
	MESSAGE("-- Using Provided BZip2 source")
	if (NOT TARGET bz2)
		add_subdirectory(
			${CMAKE_CURRENT_SOURCE_DIR}/../../Dependencies/build_cmake/bzip2-1.0.6
			${CMAKE_CURRENT_BINARY_DIR}/bzip2-1.0.6
		)
	endif()
	include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../Dependencies/bzip2-1.0.6)
	list(APPEND PROJECT_LIBS bz2)
else()
	include_directories(${BZIP2_INCLUDE_DIR})
	list(APPEND PROJECT_LIBS ${BZIP2_LIBRARIES})
endif()

find_package(ZLIB)
if(NOT ZLIB_FOUND)
	MESSAGE("-- Using Provided zlib source")
	if(NOT TARGET zlibstatic)
		add_subdirectory(
			${CMAKE_CURRENT_SOURCE_DIR}/../../Dependencies/build_cmake/zlib-1.2.8
			${CMAKE_CURRENT_BINARY_DIR}/zlib-1.2.8
		)
	endif()
	include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../Dependencies/zlib-1.2.8)
	list(APPEND PROJECT_LIBS zlibstatic)
else()
	include_directories(${ZLIB_INCLUDE_DIRS})
	list(APPEND PROJECT_LIBS ${ZLIB_LIBRARIES})
endif()

# If ICU is available, add its libraries because Framework might need its functions
find_package(ICUUC)
if(ICUUC_FOUND)
	list(APPEND PROJECT_LIBS ${ICUUC_LIBRARIES})
endif()

if(TARGET_PLATFORM_IOS)
	if(NOT TARGET iCade_Static)
		add_subdirectory(
			${CMAKE_CURRENT_SOURCE_DIR}/../../Dependencies/build_cmake/iCade-iOS
			${CMAKE_CURRENT_BINARY_DIR}/iCade-iOS
		)
	endif()
	include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../Dependencies/iCade-iOS)
	list(APPEND PROJECT_LIBS iCade_Static)
endif()

if(TARGET_PLATFORM_IOS)
	if(NOT TARGET SDWebImage_Static)
		add_subdirectory(
			${CMAKE_CURRENT_SOURCE_DIR}/../../Dependencies/build_cmake/SDWebImage
			${CMAKE_CURRENT_BINARY_DIR}/SDWebImage
		)
	endif()
	include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../Dependencies/SDWebImage)
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

if(APPLE)
	list(APPEND PROJECT_LIBS "-framework Foundation")
endif()

include_directories(../Source ../../Framework/include ../../CodeGen/include)

set(COMMON_SRC_FILES
	../Source/AppConfig.cpp
	../Source/AppConfig.h
	../Source/BasicBlock.cpp
	../Source/BasicBlock.h
	../Source/ControllerInfo.cpp
	../Source/ControllerInfo.h
	../Source/COP_FPU.cpp
	../Source/COP_FPU.h
	../Source/COP_FPU_Reflection.cpp
	../Source/COP_SCU.cpp
	../Source/COP_SCU.h
	../Source/COP_SCU_Reflection.cpp
	../Source/CsoImageStream.cpp
	../Source/CsoImageStream.h
	../Source/DiskUtils.cpp
	../Source/DiskUtils.h
	../Source/ee/COP_VU.cpp
	../Source/ee/COP_VU.h
	../Source/ee/COP_VU_Reflection.cpp
	../Source/ee/DMAC.cpp
	../Source/ee/DMAC.h
	../Source/ee/Dmac_Channel.cpp
	../Source/ee/Dmac_Channel.h
	../Source/ee/Ee_SubSystem.cpp
	../Source/ee/Ee_SubSystem.h
	../Source/ee/EEAssembler.cpp
	../Source/ee/EEAssembler.h
	../Source/ee/EeExecutor.cpp
	../Source/ee/EeExecutor.h
	../Source/ee/FpAddTruncate.cpp
	../Source/ee/FpAddTruncate.h
	../Source/ee/FpMulTruncate.cpp
	../Source/ee/FpMulTruncate.h
	../Source/ee/GIF.cpp
	../Source/ee/GIF.h
	../Source/ee/INTC.cpp
	../Source/ee/INTC.h
	../Source/ee/IPU.cpp
	../Source/ee/IPU.h
	../Source/ee/IPU_DmVectorTable.cpp
	../Source/ee/IPU_DmVectorTable.h
	../Source/ee/IPU_MacroblockAddressIncrementTable.cpp
	../Source/ee/IPU_MacroblockAddressIncrementTable.h
	../Source/ee/IPU_MacroblockTypeBTable.cpp
	../Source/ee/IPU_MacroblockTypeBTable.h
	../Source/ee/IPU_MacroblockTypeITable.cpp
	../Source/ee/IPU_MacroblockTypeITable.h
	../Source/ee/IPU_MacroblockTypePTable.cpp
	../Source/ee/IPU_MacroblockTypePTable.h
	../Source/ee/IPU_MotionCodeTable.cpp
	../Source/ee/IPU_MotionCodeTable.h
	../Source/ee/MA_EE.cpp
	../Source/ee/MA_EE.h
	../Source/ee/MA_EE_Reflection.cpp
	../Source/ee/MA_VU.cpp
	../Source/ee/MA_VU.h
	../Source/ee/MA_VU_Lower.cpp
	../Source/ee/MA_VU_LowerReflection.cpp
	../Source/ee/MA_VU_Upper.cpp
	../Source/ee/MA_VU_UpperReflection.cpp
	../Source/ee/PS2OS.cpp
	../Source/ee/PS2OS.h
	../Source/ee/SIF.cpp
	../Source/ee/SIF.h
	../Source/ee/Timer.cpp
	../Source/ee/Timer.h
	../Source/ee/Vif.cpp
	../Source/ee/Vif.h
	../Source/ee/Vif1.cpp
	../Source/ee/Vif1.h
	../Source/ee/Vpu.cpp
	../Source/ee/Vpu.h
	../Source/ee/VuAnalysis.cpp
	../Source/ee/VuAnalysis.h
	../Source/ee/VuBasicBlock.cpp
	../Source/ee/VuBasicBlock.h
	../Source/ee/VuExecutor.cpp
	../Source/ee/VuExecutor.h
	../Source/ee/VUShared.cpp
	../Source/ee/VUShared.h
	../Source/ee/VUShared_Reflection.cpp
	../Source/ELF.cpp
	../Source/ELF.h
	../Source/ElfFile.cpp
	../Source/ElfFile.h
	../Source/FrameDump.cpp
	../Source/FrameDump.h
	../Source/gs/GsCachedArea.cpp
	../Source/gs/GsCachedArea.h
	../Source/gs/GSH_Null.cpp
	../Source/gs/GSH_Null.h
	../Source/gs/GSHandler.cpp
	../Source/gs/GSHandler.h
	../Source/gs/GsPixelFormats.cpp
	../Source/gs/GsPixelFormats.h
	../Source/iop/ArgumentIterator.cpp
	../Source/iop/ArgumentIterator.h
	../Source/iop/DirectoryDevice.cpp
	../Source/iop/DirectoryDevice.h
	../Source/iop/Iop_Cdvdfsv.cpp
	../Source/iop/Iop_Cdvdfsv.h
	../Source/iop/Iop_Cdvdman.cpp
	../Source/iop/Iop_Cdvdman.h
	../Source/iop/Iop_Dmac.cpp
	../Source/iop/Iop_Dmac.h
	../Source/iop/Iop_DmacChannel.cpp
	../Source/iop/Iop_DmacChannel.h
	../Source/iop/Iop_Dynamic.cpp
	../Source/iop/Iop_Dynamic.h
	../Source/iop/Iop_FileIo.cpp
	../Source/iop/Iop_FileIo.h
	../Source/iop/Iop_FileIoHandler1000.cpp
	../Source/iop/Iop_FileIoHandler1000.h
	../Source/iop/Iop_FileIoHandler2100.cpp
	../Source/iop/Iop_FileIoHandler2100.h
	../Source/iop/Iop_FileIoHandler2240.cpp
	../Source/iop/Iop_FileIoHandler2240.h
	../Source/iop/Iop_Heaplib.cpp
	../Source/iop/Iop_Heaplib.h
	../Source/iop/Iop_Intc.cpp
	../Source/iop/Iop_Intc.h
	../Source/iop/Iop_Intrman.cpp
	../Source/iop/Iop_Intrman.h
	../Source/iop/Iop_Ioman.cpp
	../Source/iop/Iop_Ioman.h
	../Source/iop/Iop_LibSd.cpp
	../Source/iop/Iop_LibSd.h
	../Source/iop/Iop_Loadcore.cpp
	../Source/iop/Iop_Loadcore.h
	../Source/iop/Iop_McServ.cpp
	../Source/iop/Iop_McServ.h
	../Source/iop/Iop_Modload.cpp
	../Source/iop/Iop_Modload.h
	../Source/iop/Iop_Module.cpp
	../Source/iop/Iop_Module.h
	../Source/iop/Iop_MtapMan.cpp
	../Source/iop/Iop_MtapMan.h
	../Source/iop/Iop_PadMan.cpp
	../Source/iop/Iop_PadMan.h
	../Source/iop/Iop_RootCounters.cpp
	../Source/iop/Iop_RootCounters.h
	../Source/iop/Iop_SifCmd.cpp
	../Source/iop/Iop_SifCmd.h
	../Source/iop/Iop_SifDynamic.cpp
	../Source/iop/Iop_SifDynamic.h
	../Source/iop/Iop_SifMan.cpp
	../Source/iop/Iop_SifMan.h
	../Source/iop/Iop_SifManNull.cpp
	../Source/iop/Iop_SifManNull.h
	../Source/iop/Iop_SifManPs2.cpp
	../Source/iop/Iop_SifManPs2.h
	../Source/iop/Iop_Sio2.cpp
	../Source/iop/Iop_Sio2.h
	../Source/iop/Iop_Spu.cpp
	../Source/iop/Iop_Spu.h
	../Source/iop/Iop_Spu2.cpp
	../Source/iop/Iop_Spu2.h
	../Source/iop/Iop_Spu2_Core.cpp
	../Source/iop/Iop_Spu2_Core.h
	../Source/iop/Iop_SpuBase.cpp
	../Source/iop/Iop_SpuBase.h
	../Source/iop/Iop_Stdio.cpp
	../Source/iop/Iop_Stdio.h
	../Source/iop/Iop_SubSystem.cpp
	../Source/iop/Iop_SubSystem.h
	../Source/iop/Iop_Sysclib.cpp
	../Source/iop/Iop_Sysclib.h
	../Source/iop/Iop_Sysmem.cpp
	../Source/iop/Iop_Sysmem.h
	../Source/iop/Iop_Thbase.cpp
	../Source/iop/Iop_Thbase.h
	../Source/iop/Iop_Thevent.cpp
	../Source/iop/Iop_Thevent.h
	../Source/iop/Iop_Thmsgbx.cpp
	../Source/iop/Iop_Thmsgbx.h
	../Source/iop/Iop_Thsema.cpp
	../Source/iop/Iop_Thsema.h
	../Source/iop/Iop_Thvpool.cpp
	../Source/iop/Iop_Thvpool.h
	../Source/iop/Iop_Timrman.cpp
	../Source/iop/Iop_Timrman.h
	../Source/iop/Iop_Vblank.cpp
	../Source/iop/Iop_Vblank.h
	../Source/iop/IopBios.cpp
	../Source/iop/IopBios.h
	../Source/iop/OpticalMediaDevice.cpp
	../Source/iop/OpticalMediaDevice.h
	../Source/ISO9660/DirectoryRecord.cpp
	../Source/ISO9660/DirectoryRecord.h
	../Source/ISO9660/File.cpp
	../Source/ISO9660/File.h
	../Source/ISO9660/ISO9660.cpp
	../Source/ISO9660/ISO9660.h
	../Source/ISO9660/PathTable.cpp
	../Source/ISO9660/PathTable.h
	../Source/ISO9660/PathTableRecord.cpp
	../Source/ISO9660/PathTableRecord.h
	../Source/ISO9660/VolumeDescriptor.cpp
	../Source/ISO9660/VolumeDescriptor.h
	../Source/IszImageStream.cpp
	../Source/IszImageStream.h
	../Source/Log.cpp
	../Source/Log.h
	../Source/MA_MIPSIV.cpp
	../Source/MA_MIPSIV.h
	../Source/MA_MIPSIV_Reflection.cpp
	../Source/MA_MIPSIV_Templates.cpp
	../Source/MailBox.cpp
	../Source/MailBox.h
	../Source/MemoryMap.cpp
	../Source/MemoryMap.h
	../Source/MemoryStateFile.cpp
	../Source/MemoryStateFile.h
	../Source/MemoryUtils.cpp
	../Source/MemoryUtils.h
	../Source/MIPS.cpp
	../Source/MIPS.h
	../Source/MIPSAnalysis.cpp
	../Source/MIPSAnalysis.h
	../Source/MIPSArchitecture.cpp
	../Source/MIPSArchitecture.h
	../Source/MIPSAssembler.cpp
	../Source/MIPSAssembler.h
	../Source/MIPSCoprocessor.cpp
	../Source/MIPSCoprocessor.h
	../Source/MipsExecutor.cpp
	../Source/MipsExecutor.h
	../Source/MipsFunctionPatternDb.cpp
	../Source/MipsFunctionPatternDb.h
	../Source/MIPSInstructionFactory.cpp
	../Source/MIPSInstructionFactory.h
	../Source/MipsJitter.cpp
	../Source/MipsJitter.h
	../Source/MIPSReflection.cpp
	../Source/MIPSReflection.h
	../Source/MIPSTags.cpp
	../Source/MIPSTags.h
	../Source/OpticalMedia.cpp
	../Source/OpticalMedia.h
	../Source/PadHandler.cpp
	../Source/PadHandler.h
	../Source/PadListener.cpp
	../Source/PadListener.h
	../Source/Pch.cpp
	../Source/Pch.h
	../Source/PH_Generic.cpp
	../Source/PH_Generic.h
	../Source/Profiler.cpp
	../Source/Profiler.h
	../Source/PS2VM.cpp
	../Source/PS2VM.h
	../Source/RegisterStateFile.cpp
	../Source/RegisterStateFile.h
	../Source/saves/Icon.cpp
	../Source/saves/Icon.h
	../Source/saves/MaxSaveImporter.cpp
	../Source/saves/MaxSaveImporter.h
	../Source/saves/PsuSaveImporter.cpp
	../Source/saves/PsuSaveImporter.h
	../Source/saves/Save.cpp
	../Source/saves/Save.h
	../Source/saves/SaveExporter.cpp
	../Source/saves/SaveExporter.h
	../Source/saves/SaveImporterBase.cpp
	../Source/saves/SaveImporterBase.h
	../Source/saves/SaveImporter.cpp
	../Source/saves/SaveImporter.h
	../Source/saves/XpsSaveImporter.cpp
	../Source/saves/XpsSaveImporter.h
	../Source/ScopedVmPauser.cpp
	../Source/ScopedVmPauser.h
	../Source/ScreenShotUtils.cpp
	../Source/ScreenShotUtils.h
	../Source/StructCollectionStateFile.cpp
	../Source/StructCollectionStateFile.h
	../Source/StructFile.cpp
	../Source/StructFile.h
	../Source/Utils.cpp
	../Source/Utils.h
	../Source/VirtualPad.cpp
	../Source/VirtualPad.h
)

if(TARGET_PLATFORM_WIN32)
	set(PLATFORM_SPECIFIC_SRC_FILES ../Source/VolumeStream.cpp)
endif ()

if(TARGET_PLATFORM_MACOS OR TARGET_PLATFORM_UNIX)
	set(PLATFORM_SPECIFIC_SRC_FILES ../Source/Posix_VolumeStream.cpp)
endif()

if(OPENAL_FOUND)
	list(APPEND PLATFORM_SPECIFIC_SRC_FILES
		../tools/PsfPlayer/Source/SH_OpenAL.cpp
	)
endif()

add_library(PlayCore STATIC ${COMMON_SRC_FILES} ${PLATFORM_SPECIFIC_SRC_FILES})
target_link_libraries(PlayCore Boost::boost)

if(NOT ANDROID)
	if(THREADS_HAVE_PTHREAD_ARG)
		target_compile_options(PUBLIC PlayCore "-pthread")
	endif()
endif()

list(INSERT PROJECT_LIBS 0 PlayCore)

add_library(gsh_opengl STATIC 
	../Source/gs/GSH_OpenGL/GSH_OpenGL.cpp
	../Source/gs/GSH_OpenGL/GSH_OpenGL.h
	../Source/gs/GSH_OpenGL/GSH_OpenGL_Shader.cpp
	../Source/gs/GSH_OpenGL/GSH_OpenGL_Texture.cpp
)
target_link_libraries(gsh_opengl Boost::boost)
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
		../Source/ui_unix/MemoryCard.cpp
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
		../Source/ui_unix/MemoryCard.h
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
	set_target_properties(Play PROPERTIES COMPILE_FLAGS "-x objective-c++")

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
		${CMAKE_CURRENT_SOURCE_DIR}/../patches.xml
		${CMAKE_CURRENT_SOURCE_DIR}/../Source/ui_ios/Base.lproj/Main.storyboard
		${CMAKE_CURRENT_SOURCE_DIR}/../Source/ui_ios/Resources/icon@2x.png
		${CMAKE_CURRENT_SOURCE_DIR}/../Source/ui_ios/Resources/games.db
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
	target_link_libraries(gsh_opengl_win32 Boost::boost)
	list(APPEND PROJECT_LIBS gsh_opengl_win32)
	
	add_library(gsh_d3d9 STATIC 
		../Source/ui_win32/GSH_Direct3D9.cpp
		../Source/ui_win32/GSH_Direct3D9_Shader.cpp
		../Source/ui_win32/GSH_Direct3D9_Texture.cpp
	)
	target_link_libraries(gsh_d3d9 Boost::boost)
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
	add_precompiled_header(PlayCore Pch.h FORCEINCLUDE SOURCE_CXX ../Source/Pch.cpp)
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
