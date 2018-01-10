cmake_minimum_required(VERSION 3.5)

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

if(TARGET_PLATFORM_ANDROID OR TARGET_PLATFORM_IOS)
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
	list(APPEND PROJECT_LIBS Nuanceur)

	if (NOT TARGET Framework_Win32)
		add_subdirectory(
			${CMAKE_CURRENT_SOURCE_DIR}/../../Framework/build_cmake/FrameworkWin32
			${CMAKE_CURRENT_BINARY_DIR}/Framework_Win32
		)
	endif()
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
endif()

find_package(BZip2)
if(NOT BZIP2_FOUND)
	MESSAGE("-- Using Provided BZip2 source")
	if (NOT TARGET BZip2::BZip2)
		add_subdirectory(
			${CMAKE_CURRENT_SOURCE_DIR}/../../Dependencies/build_cmake/bzip2-1.0.6
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
			${CMAKE_CURRENT_SOURCE_DIR}/../../Dependencies/build_cmake/zlib-1.2.8
			${CMAKE_CURRENT_BINARY_DIR}/zlib-1.2.8
		)
	endif()
endif()
list(APPEND PROJECT_LIBS ZLIB::ZLIB)

set(COMMON_SRC_FILES
	AppConfig.cpp
	AppConfig.h
	BasicBlock.cpp
	BasicBlock.h
	ControllerInfo.cpp
	ControllerInfo.h
	COP_FPU.cpp
	COP_FPU.h
	COP_FPU_Reflection.cpp
	COP_SCU.cpp
	COP_SCU.h
	COP_SCU_Reflection.cpp
	CsoImageStream.cpp
	CsoImageStream.h
	DiskUtils.cpp
	DiskUtils.h
	ee/COP_VU.cpp
	ee/COP_VU.h
	ee/COP_VU_Reflection.cpp
	ee/DMAC.cpp
	ee/DMAC.h
	ee/Dmac_Channel.cpp
	ee/Dmac_Channel.h
	ee/Ee_SubSystem.cpp
	ee/Ee_SubSystem.h
	ee/EEAssembler.cpp
	ee/EEAssembler.h
	ee/EeExecutor.cpp
	ee/EeExecutor.h
	ee/FpAddTruncate.cpp
	ee/FpAddTruncate.h
	ee/FpMulTruncate.cpp
	ee/FpMulTruncate.h
	ee/GIF.cpp
	ee/GIF.h
	ee/INTC.cpp
	ee/INTC.h
	ee/IPU.cpp
	ee/IPU.h
	ee/IPU_DmVectorTable.cpp
	ee/IPU_DmVectorTable.h
	ee/IPU_MacroblockAddressIncrementTable.cpp
	ee/IPU_MacroblockAddressIncrementTable.h
	ee/IPU_MacroblockTypeBTable.cpp
	ee/IPU_MacroblockTypeBTable.h
	ee/IPU_MacroblockTypeITable.cpp
	ee/IPU_MacroblockTypeITable.h
	ee/IPU_MacroblockTypePTable.cpp
	ee/IPU_MacroblockTypePTable.h
	ee/IPU_MotionCodeTable.cpp
	ee/IPU_MotionCodeTable.h
	ee/MA_EE.cpp
	ee/MA_EE.h
	ee/MA_EE_Reflection.cpp
	ee/MA_VU.cpp
	ee/MA_VU.h
	ee/MA_VU_Lower.cpp
	ee/MA_VU_LowerReflection.cpp
	ee/MA_VU_Upper.cpp
	ee/MA_VU_UpperReflection.cpp
	ee/PS2OS.cpp
	ee/PS2OS.h
	ee/SIF.cpp
	ee/SIF.h
	ee/Timer.cpp
	ee/Timer.h
	ee/Vif.cpp
	ee/Vif.h
	ee/Vif1.cpp
	ee/Vif1.h
	ee/Vpu.cpp
	ee/Vpu.h
	ee/VuAnalysis.cpp
	ee/VuAnalysis.h
	ee/VuBasicBlock.cpp
	ee/VuBasicBlock.h
	ee/VuExecutor.cpp
	ee/VuExecutor.h
	ee/VUShared.cpp
	ee/VUShared.h
	ee/VUShared_Reflection.cpp
	ELF.cpp
	ELF.h
	ElfFile.cpp
	ElfFile.h
	FrameDump.cpp
	FrameDump.h
	gs/GsCachedArea.cpp
	gs/GsCachedArea.h
	gs/GSH_Null.cpp
	gs/GSH_Null.h
	gs/GSHandler.cpp
	gs/GSHandler.h
	gs/GsPixelFormats.cpp
	gs/GsPixelFormats.h
	iop/ArgumentIterator.cpp
	iop/ArgumentIterator.h
	iop/DirectoryDevice.cpp
	iop/DirectoryDevice.h
	iop/Iop_Cdvdfsv.cpp
	iop/Iop_Cdvdfsv.h
	iop/Iop_Cdvdman.cpp
	iop/Iop_Cdvdman.h
	iop/Iop_Dmac.cpp
	iop/Iop_Dmac.h
	iop/Iop_DmacChannel.cpp
	iop/Iop_DmacChannel.h
	iop/Iop_Dynamic.cpp
	iop/Iop_Dynamic.h
	iop/Iop_FileIo.cpp
	iop/Iop_FileIo.h
	iop/Iop_FileIoHandler1000.cpp
	iop/Iop_FileIoHandler1000.h
	iop/Iop_FileIoHandler2100.cpp
	iop/Iop_FileIoHandler2100.h
	iop/Iop_FileIoHandler2240.cpp
	iop/Iop_FileIoHandler2240.h
	iop/Iop_Heaplib.cpp
	iop/Iop_Heaplib.h
	iop/Iop_Intc.cpp
	iop/Iop_Intc.h
	iop/Iop_Intrman.cpp
	iop/Iop_Intrman.h
	iop/Iop_Ioman.cpp
	iop/Iop_Ioman.h
	iop/Iop_LibSd.cpp
	iop/Iop_LibSd.h
	iop/Iop_Loadcore.cpp
	iop/Iop_Loadcore.h
	iop/Iop_McServ.cpp
	iop/Iop_McServ.h
	iop/Iop_Modload.cpp
	iop/Iop_Modload.h
	iop/Iop_Module.cpp
	iop/Iop_Module.h
	iop/Iop_MtapMan.cpp
	iop/Iop_MtapMan.h
	iop/Iop_PadMan.cpp
	iop/Iop_PadMan.h
	iop/Iop_RootCounters.cpp
	iop/Iop_RootCounters.h
	iop/Iop_SifCmd.cpp
	iop/Iop_SifCmd.h
	iop/Iop_SifDynamic.cpp
	iop/Iop_SifDynamic.h
	iop/Iop_SifMan.cpp
	iop/Iop_SifMan.h
	iop/Iop_SifManNull.cpp
	iop/Iop_SifManNull.h
	iop/Iop_SifManPs2.cpp
	iop/Iop_SifManPs2.h
	iop/Iop_Sio2.cpp
	iop/Iop_Sio2.h
	iop/Iop_Spu.cpp
	iop/Iop_Spu.h
	iop/Iop_Spu2.cpp
	iop/Iop_Spu2.h
	iop/Iop_Spu2_Core.cpp
	iop/Iop_Spu2_Core.h
	iop/Iop_SpuBase.cpp
	iop/Iop_SpuBase.h
	iop/Iop_Stdio.cpp
	iop/Iop_Stdio.h
	iop/Iop_SubSystem.cpp
	iop/Iop_SubSystem.h
	iop/Iop_Sysclib.cpp
	iop/Iop_Sysclib.h
	iop/Iop_Sysmem.cpp
	iop/Iop_Sysmem.h
	iop/Iop_Thbase.cpp
	iop/Iop_Thbase.h
	iop/Iop_Thevent.cpp
	iop/Iop_Thevent.h
	iop/Iop_Thmsgbx.cpp
	iop/Iop_Thmsgbx.h
	iop/Iop_Thsema.cpp
	iop/Iop_Thsema.h
	iop/Iop_Thvpool.cpp
	iop/Iop_Thvpool.h
	iop/Iop_Timrman.cpp
	iop/Iop_Timrman.h
	iop/Iop_Vblank.cpp
	iop/Iop_Vblank.h
	iop/IopBios.cpp
	iop/IopBios.h
	iop/OpticalMediaDevice.cpp
	iop/OpticalMediaDevice.h
	ISO9660/DirectoryRecord.cpp
	ISO9660/DirectoryRecord.h
	ISO9660/File.cpp
	ISO9660/File.h
	ISO9660/ISO9660.cpp
	ISO9660/ISO9660.h
	ISO9660/PathTable.cpp
	ISO9660/PathTable.h
	ISO9660/PathTableRecord.cpp
	ISO9660/PathTableRecord.h
	ISO9660/VolumeDescriptor.cpp
	ISO9660/VolumeDescriptor.h
	IszImageStream.cpp
	IszImageStream.h
	Log.cpp
	Log.h
	MA_MIPSIV.cpp
	MA_MIPSIV.h
	MA_MIPSIV_Reflection.cpp
	MA_MIPSIV_Templates.cpp
	MailBox.cpp
	MailBox.h
	MemoryMap.cpp
	MemoryMap.h
	MemoryStateFile.cpp
	MemoryStateFile.h
	MemoryUtils.cpp
	MemoryUtils.h
	MIPS.cpp
	MIPS.h
	MIPSAnalysis.cpp
	MIPSAnalysis.h
	MIPSArchitecture.cpp
	MIPSArchitecture.h
	MIPSAssembler.cpp
	MIPSAssembler.h
	MIPSCoprocessor.cpp
	MIPSCoprocessor.h
	MipsExecutor.cpp
	MipsExecutor.h
	MipsFunctionPatternDb.cpp
	MipsFunctionPatternDb.h
	MIPSInstructionFactory.cpp
	MIPSInstructionFactory.h
	MipsJitter.cpp
	MipsJitter.h
	MIPSReflection.cpp
	MIPSReflection.h
	MIPSTags.cpp
	MIPSTags.h
	OpticalMedia.cpp
	OpticalMedia.h
	PadHandler.cpp
	PadHandler.h
	PadListener.cpp
	PadListener.h
	Pch.cpp
	Pch.h
	PH_Generic.cpp
	PH_Generic.h
	Profiler.cpp
	Profiler.h
	PS2VM.cpp
	PS2VM.h
	PS2VM_Preferences.h
	RegisterStateFile.cpp
	RegisterStateFile.h
	saves/Icon.cpp
	saves/Icon.h
	saves/MaxSaveImporter.cpp
	saves/MaxSaveImporter.h
	saves/MemoryCard.cpp
	saves/PsuSaveImporter.cpp
	saves/PsuSaveImporter.h
	saves/Save.cpp
	saves/Save.h
	saves/SaveExporter.cpp
	saves/SaveExporter.h
	saves/SaveImporterBase.cpp
	saves/SaveImporterBase.h
	saves/SaveImporter.cpp
	saves/SaveImporter.h
	saves/XpsSaveImporter.cpp
	saves/XpsSaveImporter.h
	ScopedVmPauser.cpp
	ScopedVmPauser.h
	ScreenShotUtils.cpp
	ScreenShotUtils.h
	StructCollectionStateFile.cpp
	StructCollectionStateFile.h
	StructFile.cpp
	StructFile.h
	Utils.cpp
	Utils.h
	VirtualPad.cpp
	VirtualPad.h
)

if(TARGET_PLATFORM_WIN32)
	set(PLATFORM_SPECIFIC_SRC_FILES VolumeStream.cpp)
endif ()

if(TARGET_PLATFORM_MACOS OR TARGET_PLATFORM_UNIX)
	set(PLATFORM_SPECIFIC_SRC_FILES Posix_VolumeStream.cpp)
endif()

if(OPENAL_FOUND)
	list(APPEND PLATFORM_SPECIFIC_SRC_FILES
		../tools/PsfPlayer/Source/SH_OpenAL.cpp
	)
endif()

add_library(PlayCore STATIC ${COMMON_SRC_FILES} ${PLATFORM_SPECIFIC_SRC_FILES})
target_link_libraries(PlayCore Boost::boost ${PROJECT_LIBS})
target_include_directories(PlayCore
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}
	PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/../../Framework/include
		${CMAKE_CURRENT_SOURCE_DIR}/../../CodeGen/include
)
if(NOT ANDROID)
	if(THREADS_HAVE_PTHREAD_ARG)
		target_compile_options(PUBLIC PlayCore "-pthread")
	endif()
endif()

if(TARGET_PLATFORM_WIN32)
	add_precompiled_header(PlayCore Pch.h FORCEINCLUDE SOURCE_CXX Pch.cpp)
endif()

set(PROJECT_LIBS)
