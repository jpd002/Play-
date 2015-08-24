cmake_minimum_required(VERSION 2.8)
project(Play)

set(CMAKE_MODULE_PATH
	${Play_SOURCE_DIR}/Modules
	${CMAKE_MODULE_PATH}
)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_definitions(-D_DEBUG -D_IOP_EMULATE_MODULES)

set(PROJECT_LIBS)

set(Boost_FIND_REQUIRED TRUE)
find_package(Boost COMPONENTS system filesystem chrono)
include_directories($Boost_INCLUDE_DIRS)
list(APPEND PROJECT_LIBS ${Boost_LIBRARIES})

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


include_directories(../Source ../../Framework/include ../../CodeGen/include)

add_library(Play
	${Play_SOURCE_DIR}/../../Framework/build_unix/build/libFramework.a
	${Play_SOURCE_DIR}/../../Framework/build_unix/build/libFramework_MySql.a
	${Play_SOURCE_DIR}/../../CodeGen/build_unix/build/libCodeGen.a
	../Source/AppConfig.cpp 
	../Source/BasicBlock.cpp 
	../Source/ControllerInfo.cpp 
	../Source/COP_FPU.cpp 
	../Source/COP_FPU_Reflection.cpp 
	../Source/COP_SCU.cpp 
	../Source/COP_SCU_Reflection.cpp 
	../Source/CsoImageStream.cpp 
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
	../Source/iop/Iop_FileIoHandler2300.cpp 
	../Source/iop/Iop_Intc.cpp 
	../Source/iop/Iop_Intrman.cpp 
	../Source/iop/Iop_Ioman.cpp 
	../Source/iop/Iop_LibSd.cpp 
	../Source/iop/Iop_Loadcore.cpp 
	../Source/iop/Iop_McServ.cpp 
	../Source/iop/Iop_Modload.cpp 
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
	../Source/Posix_VolumeStream.cpp
	../Source/Profiler.cpp 
	../Source/PS2VM.cpp 
	../Source/RegisterStateFile.cpp 
	../Source/StructCollectionStateFile.cpp 
	../Source/StructFile.cpp 
	../Source/Utils.cpp
)

if(THREADS_HAVE_PTHREAD_ARG)
  target_compile_options(PUBLIC Play "-pthread")
endif()

target_link_libraries(Play ${PROJECT_LIBS})

