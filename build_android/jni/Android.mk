LOCAL_PATH:= $(call my-dir)

include $(LOCAL_PATH)/ExternalDependencies.mk

include $(CLEAR_VARS)

LOCAL_MODULE			:= libboost
LOCAL_SRC_FILES 		:= $(DEPENDENCIES_PATH)/build_android/obj/local/$(TARGET_ARCH_ABI)/libboost.a

include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)

LOCAL_MODULE			:= libbzip2
LOCAL_SRC_FILES 		:= $(DEPENDENCIES_PATH)/build_android/obj/local/$(TARGET_ARCH_ABI)/libbzip2.a

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
							../../Source/BasicBlock.cpp \
							../../Source/ControllerInfo.cpp \
							../../Source/COP_FPU.cpp \
							../../Source/COP_FPU_Reflection.cpp \
							../../Source/COP_SCU.cpp \
							../../Source/COP_SCU_Reflection.cpp \
							../../Source/CsoImageStream.cpp \
							../../Source/ee/COP_VU.cpp \
							../../Source/ee/COP_VU_Reflection.cpp \
							../../Source/ee/DMAC.cpp \
							../../Source/ee/Dmac_Channel.cpp \
							../../Source/ee/Ee_SubSystem.cpp \
							../../Source/ee/EEAssembler.cpp \
							../../Source/ee/EeExecutor.cpp \
							../../Source/ee/FpAddTruncate.cpp \
							../../Source/ee/FpMulTruncate.cpp \
							../../Source/ee/GIF.cpp \
							../../Source/ee/INTC.cpp \
							../../Source/ee/IPU.cpp \
							../../Source/ee/IPU_DmVectorTable.cpp \
							../../Source/ee/IPU_MacroblockAddressIncrementTable.cpp \
							../../Source/ee/IPU_MacroblockTypeBTable.cpp \
							../../Source/ee/IPU_MacroblockTypeITable.cpp \
							../../Source/ee/IPU_MacroblockTypePTable.cpp \
							../../Source/ee/IPU_MotionCodeTable.cpp \
							../../Source/ee/MA_EE.cpp \
							../../Source/ee/MA_EE_Reflection.cpp \
							../../Source/ee/MA_VU.cpp \
							../../Source/ee/MA_VU_Lower.cpp \
							../../Source/ee/MA_VU_LowerReflection.cpp \
							../../Source/ee/MA_VU_Upper.cpp \
							../../Source/ee/MA_VU_UpperReflection.cpp \
							../../Source/ee/PS2OS.cpp \
							../../Source/ee/SIF.cpp \
							../../Source/ee/Timer.cpp \
							../../Source/ee/Vif.cpp \
							../../Source/ee/Vif1.cpp \
							../../Source/ee/Vpu.cpp \
							../../Source/ee/VuAnalysis.cpp \
							../../Source/ee/VuBasicBlock.cpp \
							../../Source/ee/VuExecutor.cpp \
							../../Source/ee/VUShared.cpp \
							../../Source/ee/VUShared_Reflection.cpp \
							../../Source/ELF.cpp \
							../../Source/ElfFile.cpp \
							../../Source/FrameDump.cpp \
							../../Source/gs/GsCachedArea.cpp \
							../../Source/gs/GSH_Null.cpp \
							../../Source/gs/GSHandler.cpp \
							../../Source/gs/GSH_OpenGL/GSH_OpenGL.cpp \
							../../Source/gs/GSH_OpenGL/GSH_OpenGL_Shader.cpp \
							../../Source/gs/GSH_OpenGL/GSH_OpenGL_Texture.cpp \
							../../Source/gs/GsPixelFormats.cpp \
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
							../../Source/IszImageStream.cpp \
							../../Source/Log.cpp \
							../../Source/MA_MIPSIV.cpp \
							../../Source/MA_MIPSIV_Reflection.cpp \
							../../Source/MA_MIPSIV_Templates.cpp \
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
							../../Source/PS2VM.cpp \
							../../Source/RegisterStateFile.cpp \
							../../Source/StructCollectionStateFile.cpp \
							../../Source/StructFile.cpp \
							../../Source/ui_android/GSH_OpenGLAndroid.cpp \
							../../Source/ui_android/InputManager.cpp \
							../../Source/ui_android/NativeInterop.cpp \
							../../Source/ui_android/NativeShared.cpp \
							../../Source/ui_android/PH_Android.cpp \
							../../Source/ui_android/SettingsManager.cpp \
							../../Source/ui_android/StatsManager.cpp \
							../../Source/Utils.cpp
LOCAL_CFLAGS			:= -mcpu=cortex-a7 -Wno-extern-c-compat -D_IOP_EMULATE_MODULES -DDISABLE_LOGGING -DGLES_COMPATIBILITY
LOCAL_C_INCLUDES		:= $(BOOST_PATH) $(DEPENDENCIES_PATH)/bzip2-1.0.6 $(FRAMEWORK_PATH)/include $(CODEGEN_PATH)/include $(LOCAL_PATH)/../../include
LOCAL_CPP_FEATURES		:= exceptions rtti
LOCAL_LDLIBS 			:= -landroid -llog -lGLESv3 -lEGL -lz
LOCAL_STATIC_LIBRARIES	:= libCodeGen libFramework libbzip2 libboost cpufeatures
LOCAL_ARM_NEON			:= true

ifeq ($(APP_OPTIM),debug)
LOCAL_CFLAGS			+= -D_DEBUG
endif

include $(BUILD_SHARED_LIBRARY)

$(call import-module,android/cpufeatures)
