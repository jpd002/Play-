cmake_minimum_required(VERSION 2.8)

if(CMAKE_CURRENT_SOURCE_DIR STREQUAL "${CMAKE_SOURCE_DIR}")
	option(TARGET_IOS "Enable building for iOS" OFF)

	# macOS deployment target needs to be set before 'project' to work
	if(APPLE AND NOT TARGET_IOS)
		set(CMAKE_OSX_DEPLOYMENT_TARGET "10.9" CACHE STRING "Minimum OS X deployment version")
	endif()
	
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
endif()

project(PlayCore)

set(PROJECT_LIBS)

set(CMAKE_MODULE_PATH
	${CMAKE_CURRENT_SOURCE_DIR}/../../../Dependencies/cmake-modules
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

if(TARGET_PLATFORM_ANDROID OR TARGET_PLATFORM_IOS)
	add_definitions(-DDISABLE_LOGGING)
endif()

# targets, packages and dependencies
if (NOT TARGET Framework)
	add_subdirectory(
		${CMAKE_CURRENT_SOURCE_DIR}/../../../Framework/build_cmake/Framework
		${CMAKE_CURRENT_BINARY_DIR}/Framework
	)
endif()
list(APPEND PROJECT_LIBS Framework)

if(TARGET_PLATFORM_UNIX)
        find_package(LIBEVDEV REQUIRED)
		include_directories(${LIBEVDEV_INCLUDE_DIR})
		list(APPEND PROJECT_LIBS ${LIBEVDEV_LIBRARY})
endif()

if(TARGET_PLATFORM_MACOS OR TARGET_PLATFORM_IOS OR TARGET_PLATFORM_UNIX)
	if (NOT TARGET Framework_OpenAl)
		add_subdirectory(
			${CMAKE_CURRENT_SOURCE_DIR}/../../../Framework/build_cmake/FrameworkOpenAl
			${CMAKE_CURRENT_BINARY_DIR}/FrameworkOpenAl
		)
	endif()
	list(APPEND PROJECT_LIBS Framework_OpenAl)
	find_package(OpenAL REQUIRED)
	list(APPEND PROJECT_LIBS ${OPENAL_LIBRARY})
endif()

if (NOT TARGET CodeGen)
	add_subdirectory(
		${CMAKE_CURRENT_SOURCE_DIR}/../../../CodeGen/build_cmake
		${CMAKE_CURRENT_BINARY_DIR}/CodeGen
	)
endif()
list(APPEND PROJECT_LIBS CodeGen)

if(TARGET_PLATFORM_WIN32)
	if (NOT TARGET Nuanceur)
		add_subdirectory(
			${CMAKE_CURRENT_SOURCE_DIR}/../../../Nuanceur/build_cmake
			${CMAKE_CURRENT_BINARY_DIR}/Nuanceur
		)
	endif()
	list(APPEND PROJECT_LIBS Nuanceur)

	if (NOT TARGET Framework_Win32)
		add_subdirectory(
			${CMAKE_CURRENT_SOURCE_DIR}/../../../Framework/build_cmake/FrameworkWin32
			${CMAKE_CURRENT_BINARY_DIR}/Framework_Win32
		)
	endif()
	list(APPEND PROJECT_LIBS Framework_Win32)

	#If building for Windows XP, we need to use the provided d3dcompiler since it's not available in the SDK
	if(CMAKE_GENERATOR_TOOLSET MATCHES "_xp$")
		include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../../Dependencies/d3dcompiler_winxp/Include)
		if(CMAKE_SIZEOF_VOID_P EQUAL 8)
			list(APPEND PROJECT_LIBS ${CMAKE_CURRENT_SOURCE_DIR}/../../../Dependencies/d3dcompiler_winxp/Lib/x64/d3dcompiler.lib)
		else()
			list(APPEND PROJECT_LIBS ${CMAKE_CURRENT_SOURCE_DIR}/../../../Dependencies/d3dcompiler_winxp/Lib/x86/d3dcompiler.lib)
		endif()
	else()
		list(APPEND PROJECT_LIBS d3dcompiler)
	endif()
	
	list(APPEND PROJECT_LIBS comctl32 uxtheme opengl32 glu32 dinput8 d3d9 vfw32 winmm gdiplus Boost::date_time)

endif()

if(NOT TARGET Boost::boost)
	add_subdirectory(
		${CMAKE_CURRENT_SOURCE_DIR}/../../../Dependencies/boost-cmake
		${CMAKE_CURRENT_BINARY_DIR}/boost-cmake
	)
endif()
list(APPEND PROJECT_LIBS Boost::boost Boost::system Boost::filesystem)

if(TARGET_PLATFORM_WIN32 OR TARGET_PLATFORM_UNIX)
	if (NOT TARGET glew_s)
		add_subdirectory(
			${CMAKE_CURRENT_SOURCE_DIR}/../../../Dependencies/glew-2.0.0
			${CMAKE_CURRENT_BINARY_DIR}/glew-2.0.0
		)
	endif()
	list(APPEND PROJECT_LIBS glew_s)
endif()

find_package(BZip2)
if(NOT BZIP2_FOUND)
	MESSAGE("-- Using Provided BZip2 source")
	if (NOT TARGET BZip2::BZip2)
		add_subdirectory(
			${CMAKE_CURRENT_SOURCE_DIR}/../../../Dependencies/build_cmake/bzip2-1.0.6
			${CMAKE_CURRENT_BINARY_DIR}/bzip2-1.0.6
		)
	endif()
	list(APPEND PROJECT_LIBS BZip2::BZip2)
else()
	include_directories(${BZIP2_INCLUDE_DIR})
	list(APPEND PROJECT_LIBS ${BZIP2_LIBRARIES})
endif()

find_package(ZLIB)
if(NOT ZLIB_FOUND)
	MESSAGE("-- Using Provided zlib source")
	if(NOT TARGET zlibstatic)
		add_subdirectory(
			${CMAKE_CURRENT_SOURCE_DIR}/../../../Dependencies/build_cmake/zlib-1.2.8
			${CMAKE_CURRENT_BINARY_DIR}/zlib-1.2.8
		)
	endif()
endif()
list(APPEND PROJECT_LIBS ZLIB::ZLIB)

set(COMMON_SRC_FILES
	../../Source/AppConfig.cpp
	../../Source/AppConfig.h
	../../Source/BasicBlock.cpp
	../../Source/BasicBlock.h
	../../Source/ControllerInfo.cpp
	../../Source/ControllerInfo.h
	../../Source/COP_FPU.cpp
	../../Source/COP_FPU.h
	../../Source/COP_FPU_Reflection.cpp
	../../Source/COP_SCU.cpp
	../../Source/COP_SCU.h
	../../Source/COP_SCU_Reflection.cpp
	../../Source/CsoImageStream.cpp
	../../Source/CsoImageStream.h
	../../Source/DiskUtils.cpp
	../../Source/DiskUtils.h
	../../Source/ee/COP_VU.cpp
	../../Source/ee/COP_VU.h
	../../Source/ee/COP_VU_Reflection.cpp
	../../Source/ee/DMAC.cpp
	../../Source/ee/DMAC.h
	../../Source/ee/Dmac_Channel.cpp
	../../Source/ee/Dmac_Channel.h
	../../Source/ee/Ee_SubSystem.cpp
	../../Source/ee/Ee_SubSystem.h
	../../Source/ee/EEAssembler.cpp
	../../Source/ee/EEAssembler.h
	../../Source/ee/EeExecutor.cpp
	../../Source/ee/EeExecutor.h
	../../Source/ee/FpAddTruncate.cpp
	../../Source/ee/FpAddTruncate.h
	../../Source/ee/FpMulTruncate.cpp
	../../Source/ee/FpMulTruncate.h
	../../Source/ee/GIF.cpp
	../../Source/ee/GIF.h
	../../Source/ee/INTC.cpp
	../../Source/ee/INTC.h
	../../Source/ee/IPU.cpp
	../../Source/ee/IPU.h
	../../Source/ee/IPU_DmVectorTable.cpp
	../../Source/ee/IPU_DmVectorTable.h
	../../Source/ee/IPU_MacroblockAddressIncrementTable.cpp
	../../Source/ee/IPU_MacroblockAddressIncrementTable.h
	../../Source/ee/IPU_MacroblockTypeBTable.cpp
	../../Source/ee/IPU_MacroblockTypeBTable.h
	../../Source/ee/IPU_MacroblockTypeITable.cpp
	../../Source/ee/IPU_MacroblockTypeITable.h
	../../Source/ee/IPU_MacroblockTypePTable.cpp
	../../Source/ee/IPU_MacroblockTypePTable.h
	../../Source/ee/IPU_MotionCodeTable.cpp
	../../Source/ee/IPU_MotionCodeTable.h
	../../Source/ee/MA_EE.cpp
	../../Source/ee/MA_EE.h
	../../Source/ee/MA_EE_Reflection.cpp
	../../Source/ee/MA_VU.cpp
	../../Source/ee/MA_VU.h
	../../Source/ee/MA_VU_Lower.cpp
	../../Source/ee/MA_VU_LowerReflection.cpp
	../../Source/ee/MA_VU_Upper.cpp
	../../Source/ee/MA_VU_UpperReflection.cpp
	../../Source/ee/PS2OS.cpp
	../../Source/ee/PS2OS.h
	../../Source/ee/SIF.cpp
	../../Source/ee/SIF.h
	../../Source/ee/Timer.cpp
	../../Source/ee/Timer.h
	../../Source/ee/Vif.cpp
	../../Source/ee/Vif.h
	../../Source/ee/Vif1.cpp
	../../Source/ee/Vif1.h
	../../Source/ee/Vpu.cpp
	../../Source/ee/Vpu.h
	../../Source/ee/VuAnalysis.cpp
	../../Source/ee/VuAnalysis.h
	../../Source/ee/VuBasicBlock.cpp
	../../Source/ee/VuBasicBlock.h
	../../Source/ee/VuExecutor.cpp
	../../Source/ee/VuExecutor.h
	../../Source/ee/VUShared.cpp
	../../Source/ee/VUShared.h
	../../Source/ee/VUShared_Reflection.cpp
	../../Source/ELF.cpp
	../../Source/ELF.h
	../../Source/ElfFile.cpp
	../../Source/ElfFile.h
	../../Source/FrameDump.cpp
	../../Source/FrameDump.h
	../../Source/gs/GsCachedArea.cpp
	../../Source/gs/GsCachedArea.h
	../../Source/gs/GSH_Null.cpp
	../../Source/gs/GSH_Null.h
	../../Source/gs/GSHandler.cpp
	../../Source/gs/GSHandler.h
	../../Source/gs/GsPixelFormats.cpp
	../../Source/gs/GsPixelFormats.h
	../../Source/iop/ArgumentIterator.cpp
	../../Source/iop/ArgumentIterator.h
	../../Source/iop/DirectoryDevice.cpp
	../../Source/iop/DirectoryDevice.h
	../../Source/iop/Iop_Cdvdfsv.cpp
	../../Source/iop/Iop_Cdvdfsv.h
	../../Source/iop/Iop_Cdvdman.cpp
	../../Source/iop/Iop_Cdvdman.h
	../../Source/iop/Iop_Dmac.cpp
	../../Source/iop/Iop_Dmac.h
	../../Source/iop/Iop_DmacChannel.cpp
	../../Source/iop/Iop_DmacChannel.h
	../../Source/iop/Iop_Dynamic.cpp
	../../Source/iop/Iop_Dynamic.h
	../../Source/iop/Iop_FileIo.cpp
	../../Source/iop/Iop_FileIo.h
	../../Source/iop/Iop_FileIoHandler1000.cpp
	../../Source/iop/Iop_FileIoHandler1000.h
	../../Source/iop/Iop_FileIoHandler2100.cpp
	../../Source/iop/Iop_FileIoHandler2100.h
	../../Source/iop/Iop_FileIoHandler2240.cpp
	../../Source/iop/Iop_FileIoHandler2240.h
	../../Source/iop/Iop_Heaplib.cpp
	../../Source/iop/Iop_Heaplib.h
	../../Source/iop/Iop_Intc.cpp
	../../Source/iop/Iop_Intc.h
	../../Source/iop/Iop_Intrman.cpp
	../../Source/iop/Iop_Intrman.h
	../../Source/iop/Iop_Ioman.cpp
	../../Source/iop/Iop_Ioman.h
	../../Source/iop/Iop_LibSd.cpp
	../../Source/iop/Iop_LibSd.h
	../../Source/iop/Iop_Loadcore.cpp
	../../Source/iop/Iop_Loadcore.h
	../../Source/iop/Iop_McServ.cpp
	../../Source/iop/Iop_McServ.h
	../../Source/iop/Iop_Modload.cpp
	../../Source/iop/Iop_Modload.h
	../../Source/iop/Iop_Module.cpp
	../../Source/iop/Iop_Module.h
	../../Source/iop/Iop_MtapMan.cpp
	../../Source/iop/Iop_MtapMan.h
	../../Source/iop/Iop_PadMan.cpp
	../../Source/iop/Iop_PadMan.h
	../../Source/iop/Iop_RootCounters.cpp
	../../Source/iop/Iop_RootCounters.h
	../../Source/iop/Iop_SifCmd.cpp
	../../Source/iop/Iop_SifCmd.h
	../../Source/iop/Iop_SifDynamic.cpp
	../../Source/iop/Iop_SifDynamic.h
	../../Source/iop/Iop_SifMan.cpp
	../../Source/iop/Iop_SifMan.h
	../../Source/iop/Iop_SifManNull.cpp
	../../Source/iop/Iop_SifManNull.h
	../../Source/iop/Iop_SifManPs2.cpp
	../../Source/iop/Iop_SifManPs2.h
	../../Source/iop/Iop_Sio2.cpp
	../../Source/iop/Iop_Sio2.h
	../../Source/iop/Iop_Spu.cpp
	../../Source/iop/Iop_Spu.h
	../../Source/iop/Iop_Spu2.cpp
	../../Source/iop/Iop_Spu2.h
	../../Source/iop/Iop_Spu2_Core.cpp
	../../Source/iop/Iop_Spu2_Core.h
	../../Source/iop/Iop_SpuBase.cpp
	../../Source/iop/Iop_SpuBase.h
	../../Source/iop/Iop_Stdio.cpp
	../../Source/iop/Iop_Stdio.h
	../../Source/iop/Iop_SubSystem.cpp
	../../Source/iop/Iop_SubSystem.h
	../../Source/iop/Iop_Sysclib.cpp
	../../Source/iop/Iop_Sysclib.h
	../../Source/iop/Iop_Sysmem.cpp
	../../Source/iop/Iop_Sysmem.h
	../../Source/iop/Iop_Thbase.cpp
	../../Source/iop/Iop_Thbase.h
	../../Source/iop/Iop_Thevent.cpp
	../../Source/iop/Iop_Thevent.h
	../../Source/iop/Iop_Thmsgbx.cpp
	../../Source/iop/Iop_Thmsgbx.h
	../../Source/iop/Iop_Thsema.cpp
	../../Source/iop/Iop_Thsema.h
	../../Source/iop/Iop_Thvpool.cpp
	../../Source/iop/Iop_Thvpool.h
	../../Source/iop/Iop_Timrman.cpp
	../../Source/iop/Iop_Timrman.h
	../../Source/iop/Iop_Vblank.cpp
	../../Source/iop/Iop_Vblank.h
	../../Source/iop/IopBios.cpp
	../../Source/iop/IopBios.h
	../../Source/iop/OpticalMediaDevice.cpp
	../../Source/iop/OpticalMediaDevice.h
	../../Source/ISO9660/DirectoryRecord.cpp
	../../Source/ISO9660/DirectoryRecord.h
	../../Source/ISO9660/File.cpp
	../../Source/ISO9660/File.h
	../../Source/ISO9660/ISO9660.cpp
	../../Source/ISO9660/ISO9660.h
	../../Source/ISO9660/PathTable.cpp
	../../Source/ISO9660/PathTable.h
	../../Source/ISO9660/PathTableRecord.cpp
	../../Source/ISO9660/PathTableRecord.h
	../../Source/ISO9660/VolumeDescriptor.cpp
	../../Source/ISO9660/VolumeDescriptor.h
	../../Source/IszImageStream.cpp
	../../Source/IszImageStream.h
	../../Source/Log.cpp
	../../Source/Log.h
	../../Source/MA_MIPSIV.cpp
	../../Source/MA_MIPSIV.h
	../../Source/MA_MIPSIV_Reflection.cpp
	../../Source/MA_MIPSIV_Templates.cpp
	../../Source/MailBox.cpp
	../../Source/MailBox.h
	../../Source/MemoryMap.cpp
	../../Source/MemoryMap.h
	../../Source/MemoryStateFile.cpp
	../../Source/MemoryStateFile.h
	../../Source/MemoryUtils.cpp
	../../Source/MemoryUtils.h
	../../Source/MIPS.cpp
	../../Source/MIPS.h
	../../Source/MIPSAnalysis.cpp
	../../Source/MIPSAnalysis.h
	../../Source/MIPSArchitecture.cpp
	../../Source/MIPSArchitecture.h
	../../Source/MIPSAssembler.cpp
	../../Source/MIPSAssembler.h
	../../Source/MIPSCoprocessor.cpp
	../../Source/MIPSCoprocessor.h
	../../Source/MipsExecutor.cpp
	../../Source/MipsExecutor.h
	../../Source/MipsFunctionPatternDb.cpp
	../../Source/MipsFunctionPatternDb.h
	../../Source/MIPSInstructionFactory.cpp
	../../Source/MIPSInstructionFactory.h
	../../Source/MipsJitter.cpp
	../../Source/MipsJitter.h
	../../Source/MIPSReflection.cpp
	../../Source/MIPSReflection.h
	../../Source/MIPSTags.cpp
	../../Source/MIPSTags.h
	../../Source/OpticalMedia.cpp
	../../Source/OpticalMedia.h
	../../Source/PadHandler.cpp
	../../Source/PadHandler.h
	../../Source/PadListener.cpp
	../../Source/PadListener.h
	../../Source/Pch.cpp
	../../Source/Pch.h
	../../Source/PH_Generic.cpp
	../../Source/PH_Generic.h
	../../Source/Profiler.cpp
	../../Source/Profiler.h
	../../Source/PS2VM.cpp
	../../Source/PS2VM.h
	../../Source/PS2VM_Preferences.h
	../../Source/RegisterStateFile.cpp
	../../Source/RegisterStateFile.h
	../../Source/saves/Icon.cpp
	../../Source/saves/Icon.h
	../../Source/saves/MaxSaveImporter.cpp
	../../Source/saves/MaxSaveImporter.h
	../../Source/saves/MemoryCard.cpp
	../../Source/saves/PsuSaveImporter.cpp
	../../Source/saves/PsuSaveImporter.h
	../../Source/saves/Save.cpp
	../../Source/saves/Save.h
	../../Source/saves/SaveExporter.cpp
	../../Source/saves/SaveExporter.h
	../../Source/saves/SaveImporterBase.cpp
	../../Source/saves/SaveImporterBase.h
	../../Source/saves/SaveImporter.cpp
	../../Source/saves/SaveImporter.h
	../../Source/saves/XpsSaveImporter.cpp
	../../Source/saves/XpsSaveImporter.h
	../../Source/ScopedVmPauser.cpp
	../../Source/ScopedVmPauser.h
	../../Source/ScreenShotUtils.cpp
	../../Source/ScreenShotUtils.h
	../../Source/StructCollectionStateFile.cpp
	../../Source/StructCollectionStateFile.h
	../../Source/StructFile.cpp
	../../Source/StructFile.h
	../../Source/Utils.cpp
	../../Source/Utils.h
	../../Source/VirtualPad.cpp
	../../Source/VirtualPad.h
)

if(TARGET_PLATFORM_WIN32)
	set(PLATFORM_SPECIFIC_SRC_FILES ../../Source/VolumeStream.cpp)
endif ()

if(TARGET_PLATFORM_MACOS OR TARGET_PLATFORM_UNIX)
	set(PLATFORM_SPECIFIC_SRC_FILES ../../Source/Posix_VolumeStream.cpp)
endif()

if(OPENAL_FOUND)
	list(APPEND PLATFORM_SPECIFIC_SRC_FILES
		../../tools/PsfPlayer/Source/SH_OpenAL.cpp
	)
endif()

add_library(PlayCore STATIC ${COMMON_SRC_FILES} ${PLATFORM_SPECIFIC_SRC_FILES})
target_link_libraries(PlayCore Boost::boost ${PROJECT_LIBS})
target_include_directories(PlayCore
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}/../../Source
	PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/../../../Framework/include
		${CMAKE_CURRENT_SOURCE_DIR}/../../../CodeGen/include
)
if(NOT ANDROID)
	if(THREADS_HAVE_PTHREAD_ARG)
		target_compile_options(PUBLIC PlayCore "-pthread")
	endif()
endif()

if(TARGET_PLATFORM_WIN32)
	add_precompiled_header(PlayCore Pch.h FORCEINCLUDE SOURCE_CXX ../../Source/Pch.cpp)
endif()

set(PROJECT_LIBS)
