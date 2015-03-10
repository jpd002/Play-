LOCAL_PATH:= $(call my-dir)

include $(LOCAL_PATH)/ExternalDependencies.mk

include $(CLEAR_VARS)

LOCAL_MODULE			:= libboost
LOCAL_SRC_FILES 		:= $(BOOST_PATH)/build_android/obj/local/$(TARGET_ARCH_ABI)/libboost.a

include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)

LOCAL_MODULE			:= libFramework
LOCAL_SRC_FILES 		:= $(FRAMEWORK_PATH)/build_android/obj/local/$(TARGET_ARCH_ABI)/libFramework.a

include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)

LOCAL_MODULE			:= libCodeGen
LOCAL_SRC_FILES 		:= $(CODEGEN_PATH)/build_android/obj/local/$(TARGET_ARCH_ABI)/libCodeGen.a

include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)

LOCAL_MODULE			:= libPlay
LOCAL_SRC_FILES			:=	../../Source/AppConfig.cpp \
							../../Source/androidui/GSH_OpenGLAndroid.cpp \
							../../Source/androidui/NativeInterop.cpp \
							../../Source/BasicBlock.cpp \
							../../Source/COP_FPU.cpp \
							../../Source/COP_FPU_Reflection.cpp \
							../../Source/COP_SCU.cpp \
							../../Source/COP_SCU_Reflection.cpp \
							../../Source/COP_VU.cpp \
							../../Source/COP_VU_Reflection.cpp \
							../../Source/DMAC.cpp \
							../../Source/Dmac_Channel.cpp \
							../../Source/Ee_SubSystem.cpp \
							../../Source/EEAssembler.cpp \
							../../Source/ELF.cpp \
							../../Source/ElfFile.cpp \
							../../Source/FpMulTruncate.cpp \
							../../Source/FrameDump.cpp \
							../../Source/GIF.cpp \
							../../Source/GSHandler.cpp \
							../../Source/GSH_Null.cpp \
							../../Source/GSH_OpenGL.cpp \
							../../Source/GSH_OpenGL_Shader.cpp \
							../../Source/GSH_OpenGL_Texture.cpp \
							../../Source/GsCachedArea.cpp \
							../../Source/GsPixelFormats.cpp \
							../../Source/INTC.cpp \
							../../Source/iop/ArgumentIterator.cpp \
							../../Source/iop/DirectoryDevice.cpp \
							../../Source/iop/Iop_Cdvdfsv.cpp \
							../../Source/iop/Iop_Cdvdman.cpp \
							../../Source/iop/Iop_Dmac.cpp \
							../../Source/iop/Iop_DmacChannel.cpp \
							../../Source/iop/Iop_Dynamic.cpp \
							../../Source/iop/Iop_FileIo.cpp \
							../../Source/iop/Iop_FileIoHandler1000.cpp \
							../../Source/iop/Iop_FileIoHandler2100.cpp \
							../../Source/iop/Iop_FileIoHandler2300.cpp \
							../../Source/iop/Iop_Intc.cpp \
							../../Source/iop/Iop_Intrman.cpp \
							../../Source/iop/Iop_Ioman.cpp \
							../../Source/iop/Iop_LibSd.cpp \
							../../Source/iop/Iop_Loadcore.cpp \
							../../Source/iop/Iop_McServ.cpp \
							../../Source/iop/Iop_Modload.cpp \
							../../Source/iop/Iop_PadMan.cpp \
							../../Source/iop/Iop_RootCounters.cpp \
							../../Source/iop/Iop_SifCmd.cpp \
							../../Source/iop/Iop_SifDynamic.cpp \
							../../Source/iop/Iop_SifMan.cpp \
							../../Source/iop/Iop_SifManNull.cpp \
							../../Source/iop/Iop_SifManPs2.cpp \
							../../Source/iop/Iop_Sio2.cpp \
							../../Source/iop/Iop_Spu.cpp \
							../../Source/iop/Iop_Spu2.cpp \
							../../Source/iop/Iop_Spu2_Core.cpp \
							../../Source/iop/Iop_SpuBase.cpp \
							../../Source/iop/Iop_Stdio.cpp \
							../../Source/iop/Iop_SubSystem.cpp \
							../../Source/iop/Iop_Sysclib.cpp \
							../../Source/iop/Iop_Sysmem.cpp \
							../../Source/iop/Iop_Thbase.cpp \
							../../Source/iop/Iop_Thevent.cpp \
							../../Source/iop/Iop_Thmsgbx.cpp \
							../../Source/iop/Iop_Thsema.cpp \
							../../Source/iop/Iop_Timrman.cpp \
							../../Source/iop/Iop_Vblank.cpp \
							../../Source/iop/IopBios.cpp \
							../../Source/iop/IsoDevice.cpp \
							../../Source/ISO9660/DirectoryRecord.cpp \
							../../Source/ISO9660/File.cpp \
							../../Source/ISO9660/ISO9660.cpp \
							../../Source/ISO9660/PathTable.cpp \
							../../Source/ISO9660/PathTableRecord.cpp \
							../../Source/ISO9660/VolumeDescriptor.cpp \
							../../Source/IPU.cpp \
							../../Source/IPU_DmVectorTable.cpp \
							../../Source/IPU_MacroblockAddressIncrementTable.cpp \
							../../Source/IPU_MacroblockTypeBTable.cpp \
							../../Source/IPU_MacroblockTypeITable.cpp \
							../../Source/IPU_MacroblockTypePTable.cpp \
							../../Source/IPU_MotionCodeTable.cpp \
							../../Source/Log.cpp \
							../../Source/MA_EE.cpp \
							../../Source/MA_EE_Reflection.cpp \
							../../Source/MA_MIPSIV.cpp \
							../../Source/MA_MIPSIV_Reflection.cpp \
							../../Source/MA_MIPSIV_Templates.cpp \
							../../Source/MA_VU.cpp \
							../../Source/MA_VU_Lower.cpp \
							../../Source/MA_VU_LowerReflection.cpp \
							../../Source/MA_VU_Upper.cpp \
							../../Source/MA_VU_UpperReflection.cpp \
							../../Source/MailBox.cpp \
							../../Source/MemoryMap.cpp \
							../../Source/MemoryStateFile.cpp \
							../../Source/MemoryUtils.cpp \
							../../Source/MIPS.cpp \
							../../Source/MIPSAnalysis.cpp \
							../../Source/MIPSArchitecture.cpp \
							../../Source/MIPSAssembler.cpp \
							../../Source/MIPSCoprocessor.cpp \
							../../Source/MipsExecutor.cpp \
							../../Source/MIPSInstructionFactory.cpp \
							../../Source/MipsJitter.cpp \
							../../Source/MIPSReflection.cpp \
							../../Source/MIPSTags.cpp \
							../../Source/PadHandler.cpp \
							../../Source/PadListener.cpp \
							../../Source/Profiler.cpp \
							../../Source/PS2OS.cpp \
							../../Source/PS2VM.cpp \
							../../Source/RegisterStateFile.cpp \
							../../Source/SIF.cpp \
							../../Source/StructCollectionStateFile.cpp \
							../../Source/StructFile.cpp \
							../../Source/Timer.cpp \
							../../Source/Utils.cpp \
							../../Source/VIF.cpp \
							../../Source/VPU.cpp \
							../../Source/VPU1.cpp \
							../../Source/VuAnalysis.cpp \
							../../Source/VuBasicBlock.cpp \
							../../Source/VuExecutor.cpp \
							../../Source/VUShared.cpp \
							../../Source/VUShared_Reflection.cpp
LOCAL_CFLAGS			:= -Wno-extern-c-compat -D_IOP_EMULATE_MODULES -D_DEBUG -DDISABLE_LOGGING
LOCAL_C_INCLUDES		:= $(BOOST_PATH) $(FRAMEWORK_PATH)/include $(CODEGEN_PATH)/include $(LOCAL_PATH)/../../include
LOCAL_CPP_FEATURES		:= exceptions rtti
LOCAL_LDLIBS 			:= -landroid -llog -lGLESv3 -lEGL -lz
LOCAL_STATIC_LIBRARIES	:= libCodeGen libFramework libboost

include $(BUILD_SHARED_LIBRARY)
