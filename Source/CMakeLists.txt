cmake_minimum_required(VERSION 3.5)

set(CMAKE_MODULE_PATH
	${CMAKE_CURRENT_SOURCE_DIR}/../../Dependencies/cmake-modules
	${CMAKE_MODULE_PATH}
)
include(Header)

project(PlayCore)

set(PROJECT_LIBS)

include(PrecompiledHeader)

if(DEBUGGER_INCLUDED)
	list(APPEND DEFINITIONS_LIST DEBUGGER_INCLUDED=1)
endif()
if(PROFILE)
	list(APPEND DEFINITIONS_LIST PROFILE=1)
endif()
list(APPEND DEFINITIONS_LIST _IOP_EMULATE_MODULES=1)

if(USE_AOT_CACHE)
	list(APPEND DEFINITIONS_LIST AOT_USE_CACHE=1)
endif()

if(BUILD_AOT_CACHE)
	list(APPEND DEFINITIONS_LIST AOT_BUILD_CACHE=1)
endif()

if(TARGET_PLATFORM_ANDROID OR TARGET_PLATFORM_IOS OR BUILD_AOT_CACHE)
	list(APPEND DEFINITIONS_LIST DISABLE_LOGGING=1)
endif()

# targets, packages and dependencies
if(NOT TARGET Framework)
	add_subdirectory(
		${CMAKE_CURRENT_SOURCE_DIR}/../../Framework/build_cmake/Framework
		${CMAKE_CURRENT_BINARY_DIR}/Framework
	)
endif()
list(APPEND PROJECT_LIBS Framework)

if(NOT TARGET Framework_Http)
	add_subdirectory(
		${CMAKE_CURRENT_SOURCE_DIR}/../../Framework/build_cmake/FrameworkHttp
		${CMAKE_CURRENT_BINARY_DIR}/FrameworkHttp
	)
endif()
list(APPEND PROJECT_LIBS Framework_Http)

if(NOT TARGET Framework_Sqlite)
	add_subdirectory(
		${CMAKE_CURRENT_SOURCE_DIR}/../../Framework/build_cmake/FrameworkSqlite
		${CMAKE_CURRENT_BINARY_DIR}/FrameworkSqlite
	)
endif()
list(INSERT PROJECT_LIBS 0 Framework_Sqlite)

if(NOT TARGET CodeGen)
	add_subdirectory(
		${CMAKE_CURRENT_SOURCE_DIR}/../../CodeGen/build_cmake
		${CMAKE_CURRENT_BINARY_DIR}/CodeGen
	)
endif()
list(APPEND PROJECT_LIBS CodeGen)

if(NOT TARGET Boost::boost)
	add_subdirectory(
		${CMAKE_CURRENT_SOURCE_DIR}/../../Dependencies/boost-cmake
		${CMAKE_CURRENT_BINARY_DIR}/boost-cmake
	)
endif()
list(APPEND PROJECT_LIBS Boost::boost Boost::system Boost::filesystem)

find_package(BZip2)
if(NOT BZIP2_FOUND)
	MESSAGE("-- Using Provided BZip2 source")
	if(NOT TARGET BZip2::BZip2)
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

# If ICU is available, add its libraries because Framework might need its functions
find_package(ICUUC)
if(ICUUC_FOUND)
	list(APPEND PROJECT_LIBS ${ICUUC_LIBRARIES})
endif()

if(NOT (TARGET_PLATFORM_ANDROID OR TARGET_PLATFORM_IOS))
	find_package(Threads REQUIRED)
	if(CMAKE_THREAD_LIBS_INIT)
		list(APPEND PROJECT_LIBS "${CMAKE_THREAD_LIBS_INIT}")
	endif()
endif()

if(NOT TARGET nlohmann_json)
	set(JSON_BuildTests OFF CACHE BOOL "Disable test build")
	add_subdirectory(
		${CMAKE_CURRENT_SOURCE_DIR}/../../Dependencies/build_cmake/nlohmann_json
		${CMAKE_CURRENT_BINARY_DIR}/nlohmann_json
		EXCLUDE_FROM_ALL
	)
endif()
list(APPEND PROJECT_LIBS nlohmann_json)

set(COMMON_SRC_FILES
	AppConfig.cpp
	AppConfig.h
	BasicBlock.cpp
	BasicBlock.h
	BlockLookupOneWay.h
	BlockLookupTwoWay.h
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
	FpUtils.cpp
	FpUtils.h
	FrameDump.cpp
	FrameDump.h
	GenericMipsExecutor.h
	gs/GsCachedArea.cpp
	gs/GsCachedArea.h
	gs/GSH_Null.cpp
	gs/GSH_Null.h
	gs/GSHandler.cpp
	gs/GSHandler.h
	gs/GsPixelFormats.cpp
	gs/GsPixelFormats.h
	input/InputBindingManager.cpp
	input/InputBindingManager.h
	input/InputProvider.h
	input/PH_GenericInput.cpp
	input/PH_GenericInput.h
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
	iop/Iop_Thfpool.cpp
	iop/Iop_Thfpool.h
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
	MdsDiscImage.cpp
	MdsDiscImage.h
	MemoryMap.cpp
	MemoryMap.h
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
	Ps2Const.h
	PS2VM.cpp
	PS2VM.h
	PS2VM_Preferences.h
	psx/PsxBios.cpp
	psx/PsxBios.h
	s3stream/AmazonS3Client.cpp
	s3stream/S3ObjectStream.cpp
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
	states/MemoryStateFile.cpp
	states/MemoryStateFile.h
	states/RegisterStateFile.cpp
	states/RegisterStateFile.h
	states/StructCollectionStateFile.cpp
	states/StructCollectionStateFile.h
	states/StructFile.cpp
	states/StructFile.h
	states/XmlStateFile.cpp
	states/XmlStateFile.h
	ScopedVmPauser.cpp
	ScopedVmPauser.h
	ScreenShotUtils.cpp
	ScreenShotUtils.h
	SifDefs.h
	ui_shared/BootablesDbClient.cpp
	ui_shared/BootablesDbClient.h
	ui_shared/BootablesProcesses.cpp
	ui_shared/BootablesProcesses.h
	ui_shared/TheGamesDbClient.cpp
	ui_shared/TheGamesDbClient.h
	VirtualPad.cpp
	VirtualPad.h
)

if(TARGET_PLATFORM_WIN32)
	set(PLATFORM_SPECIFIC_SRC_FILES VolumeStream.cpp)
endif()

if(TARGET_PLATFORM_MACOS OR TARGET_PLATFORM_UNIX)
	set(PLATFORM_SPECIFIC_SRC_FILES Posix_VolumeStream.cpp)
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
target_compile_definitions(PlayCore PUBLIC ${DEFINITIONS_LIST})
if(NOT ANDROID)
	if(THREADS_HAVE_PTHREAD_ARG)
		target_compile_options(PUBLIC PlayCore "-pthread")
	endif()
endif()

if(TARGET_PLATFORM_WIN32)
	add_precompiled_header(PlayCore Pch.h FORCEINCLUDE SOURCE_CXX Pch.cpp)
endif()
