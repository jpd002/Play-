LOCAL_PATH := $(call my-dir)

include $(LOCAL_PATH)/ExternalDependencies.mk

PROJECT_PATH      := $(realpath $(LOCAL_PATH))/../../../../
DEPENDENCIES_PATH := $(realpath $(LOCAL_PATH))/../../../../../Dependencies
FRAMEWORK_PATH    := $(realpath $(LOCAL_PATH))/../../../../../Framework
CODEGEN_PATH      := $(realpath $(LOCAL_PATH))/../../../../../CodeGen

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
LOCAL_SRC_FILES 		:= $(CODEGEN_PATH)/build_android/src/main/obj/local/$(TARGET_ARCH_ABI)/libCodeGen.a

include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)

LOCAL_MODULE			:= libPlay
LOCAL_SRC_FILES			:=	$(PROJECT_PATH)/Source/AppConfig.cpp \
							$(PROJECT_PATH)/Source/BasicBlock.cpp \
							$(PROJECT_PATH)/Source/ControllerInfo.cpp \
							$(PROJECT_PATH)/Source/COP_FPU.cpp \
							$(PROJECT_PATH)/Source/COP_FPU_Reflection.cpp \
							$(PROJECT_PATH)/Source/COP_SCU.cpp \
							$(PROJECT_PATH)/Source/COP_SCU_Reflection.cpp \
							$(PROJECT_PATH)/Source/CsoImageStream.cpp \
							$(PROJECT_PATH)/Source/DiskUtils.cpp \
							$(PROJECT_PATH)/Source/ee/COP_VU.cpp \
							$(PROJECT_PATH)/Source/ee/COP_VU_Reflection.cpp \
							$(PROJECT_PATH)/Source/ee/DMAC.cpp \
							$(PROJECT_PATH)/Source/ee/Dmac_Channel.cpp \
							$(PROJECT_PATH)/Source/ee/Ee_SubSystem.cpp \
							$(PROJECT_PATH)/Source/ee/EEAssembler.cpp \
							$(PROJECT_PATH)/Source/ee/EeExecutor.cpp \
							$(PROJECT_PATH)/Source/ee/FpAddTruncate.cpp \
							$(PROJECT_PATH)/Source/ee/FpMulTruncate.cpp \
							$(PROJECT_PATH)/Source/ee/GIF.cpp \
							$(PROJECT_PATH)/Source/ee/INTC.cpp \
							$(PROJECT_PATH)/Source/ee/IPU.cpp \
							$(PROJECT_PATH)/Source/ee/IPU_DmVectorTable.cpp \
							$(PROJECT_PATH)/Source/ee/IPU_MacroblockAddressIncrementTable.cpp \
							$(PROJECT_PATH)/Source/ee/IPU_MacroblockTypeBTable.cpp \
							$(PROJECT_PATH)/Source/ee/IPU_MacroblockTypeITable.cpp \
							$(PROJECT_PATH)/Source/ee/IPU_MacroblockTypePTable.cpp \
							$(PROJECT_PATH)/Source/ee/IPU_MotionCodeTable.cpp \
							$(PROJECT_PATH)/Source/ee/MA_EE.cpp \
							$(PROJECT_PATH)/Source/ee/MA_EE_Reflection.cpp \
							$(PROJECT_PATH)/Source/ee/MA_VU.cpp \
							$(PROJECT_PATH)/Source/ee/MA_VU_Lower.cpp \
							$(PROJECT_PATH)/Source/ee/MA_VU_LowerReflection.cpp \
							$(PROJECT_PATH)/Source/ee/MA_VU_Upper.cpp \
							$(PROJECT_PATH)/Source/ee/MA_VU_UpperReflection.cpp \
							$(PROJECT_PATH)/Source/ee/PS2OS.cpp \
							$(PROJECT_PATH)/Source/ee/SIF.cpp \
							$(PROJECT_PATH)/Source/ee/Timer.cpp \
							$(PROJECT_PATH)/Source/ee/Vif.cpp \
							$(PROJECT_PATH)/Source/ee/Vif1.cpp \
							$(PROJECT_PATH)/Source/ee/Vpu.cpp \
							$(PROJECT_PATH)/Source/ee/VuAnalysis.cpp \
							$(PROJECT_PATH)/Source/ee/VuBasicBlock.cpp \
							$(PROJECT_PATH)/Source/ee/VuExecutor.cpp \
							$(PROJECT_PATH)/Source/ee/VUShared.cpp \
							$(PROJECT_PATH)/Source/ee/VUShared_Reflection.cpp \
							$(PROJECT_PATH)/Source/ELF.cpp \
							$(PROJECT_PATH)/Source/ElfFile.cpp \
							$(PROJECT_PATH)/Source/FrameDump.cpp \
							$(PROJECT_PATH)/Source/gs/GsCachedArea.cpp \
							$(PROJECT_PATH)/Source/gs/GSH_Null.cpp \
							$(PROJECT_PATH)/Source/gs/GSHandler.cpp \
							$(PROJECT_PATH)/Source/gs/GSH_OpenGL/GSH_OpenGL.cpp \
							$(PROJECT_PATH)/Source/gs/GSH_OpenGL/GSH_OpenGL_Shader.cpp \
							$(PROJECT_PATH)/Source/gs/GSH_OpenGL/GSH_OpenGL_Texture.cpp \
							$(PROJECT_PATH)/Source/gs/GsPixelFormats.cpp \
							$(PROJECT_PATH)/Source/iop/ArgumentIterator.cpp \
							$(PROJECT_PATH)/Source/iop/DirectoryDevice.cpp \
							$(PROJECT_PATH)/Source/iop/Iop_Cdvdfsv.cpp \
							$(PROJECT_PATH)/Source/iop/Iop_Cdvdman.cpp \
							$(PROJECT_PATH)/Source/iop/Iop_Dmac.cpp \
							$(PROJECT_PATH)/Source/iop/Iop_DmacChannel.cpp \
							$(PROJECT_PATH)/Source/iop/Iop_Dynamic.cpp \
							$(PROJECT_PATH)/Source/iop/Iop_FileIo.cpp \
							$(PROJECT_PATH)/Source/iop/Iop_FileIoHandler1000.cpp \
							$(PROJECT_PATH)/Source/iop/Iop_FileIoHandler2100.cpp \
							$(PROJECT_PATH)/Source/iop/Iop_FileIoHandler2300.cpp \
							$(PROJECT_PATH)/Source/iop/Iop_Intc.cpp \
							$(PROJECT_PATH)/Source/iop/Iop_Intrman.cpp \
							$(PROJECT_PATH)/Source/iop/Iop_Ioman.cpp \
							$(PROJECT_PATH)/Source/iop/Iop_LibSd.cpp \
							$(PROJECT_PATH)/Source/iop/Iop_Loadcore.cpp \
							$(PROJECT_PATH)/Source/iop/Iop_McServ.cpp \
							$(PROJECT_PATH)/Source/iop/Iop_Modload.cpp \
							$(PROJECT_PATH)/Source/iop/Iop_MtapMan.cpp \
							$(PROJECT_PATH)/Source/iop/Iop_PadMan.cpp \
							$(PROJECT_PATH)/Source/iop/Iop_RootCounters.cpp \
							$(PROJECT_PATH)/Source/iop/Iop_SifCmd.cpp \
							$(PROJECT_PATH)/Source/iop/Iop_SifDynamic.cpp \
							$(PROJECT_PATH)/Source/iop/Iop_SifMan.cpp \
							$(PROJECT_PATH)/Source/iop/Iop_SifManNull.cpp \
							$(PROJECT_PATH)/Source/iop/Iop_SifManPs2.cpp \
							$(PROJECT_PATH)/Source/iop/Iop_Sio2.cpp \
							$(PROJECT_PATH)/Source/iop/Iop_Spu.cpp \
							$(PROJECT_PATH)/Source/iop/Iop_Spu2.cpp \
							$(PROJECT_PATH)/Source/iop/Iop_Spu2_Core.cpp \
							$(PROJECT_PATH)/Source/iop/Iop_SpuBase.cpp \
							$(PROJECT_PATH)/Source/iop/Iop_Stdio.cpp \
							$(PROJECT_PATH)/Source/iop/Iop_SubSystem.cpp \
							$(PROJECT_PATH)/Source/iop/Iop_Sysclib.cpp \
							$(PROJECT_PATH)/Source/iop/Iop_Sysmem.cpp \
							$(PROJECT_PATH)/Source/iop/Iop_Thbase.cpp \
							$(PROJECT_PATH)/Source/iop/Iop_Thevent.cpp \
							$(PROJECT_PATH)/Source/iop/Iop_Thmsgbx.cpp \
							$(PROJECT_PATH)/Source/iop/Iop_Thsema.cpp \
							$(PROJECT_PATH)/Source/iop/Iop_Thvpool.cpp \
							$(PROJECT_PATH)/Source/iop/Iop_Timrman.cpp \
							$(PROJECT_PATH)/Source/iop/Iop_Vblank.cpp \
							$(PROJECT_PATH)/Source/iop/IopBios.cpp \
							$(PROJECT_PATH)/Source/iop/IsoDevice.cpp \
							$(PROJECT_PATH)/Source/ISO9660/DirectoryRecord.cpp \
							$(PROJECT_PATH)/Source/ISO9660/File.cpp \
							$(PROJECT_PATH)/Source/ISO9660/ISO9660.cpp \
							$(PROJECT_PATH)/Source/ISO9660/PathTable.cpp \
							$(PROJECT_PATH)/Source/ISO9660/PathTableRecord.cpp \
							$(PROJECT_PATH)/Source/ISO9660/VolumeDescriptor.cpp \
							$(PROJECT_PATH)/Source/IszImageStream.cpp \
							$(PROJECT_PATH)/Source/Log.cpp \
							$(PROJECT_PATH)/Source/MA_MIPSIV.cpp \
							$(PROJECT_PATH)/Source/MA_MIPSIV_Reflection.cpp \
							$(PROJECT_PATH)/Source/MA_MIPSIV_Templates.cpp \
							$(PROJECT_PATH)/Source/MailBox.cpp \
							$(PROJECT_PATH)/Source/MemoryMap.cpp \
							$(PROJECT_PATH)/Source/MemoryStateFile.cpp \
							$(PROJECT_PATH)/Source/MemoryUtils.cpp \
							$(PROJECT_PATH)/Source/MIPS.cpp \
							$(PROJECT_PATH)/Source/MIPSAnalysis.cpp \
							$(PROJECT_PATH)/Source/MIPSArchitecture.cpp \
							$(PROJECT_PATH)/Source/MIPSAssembler.cpp \
							$(PROJECT_PATH)/Source/MIPSCoprocessor.cpp \
							$(PROJECT_PATH)/Source/MipsExecutor.cpp \
							$(PROJECT_PATH)/Source/MIPSInstructionFactory.cpp \
							$(PROJECT_PATH)/Source/MipsJitter.cpp \
							$(PROJECT_PATH)/Source/MIPSReflection.cpp \
							$(PROJECT_PATH)/Source/MIPSTags.cpp \
							$(PROJECT_PATH)/Source/PadHandler.cpp \
							$(PROJECT_PATH)/Source/PadListener.cpp \
							$(PROJECT_PATH)/Source/PH_Generic.cpp \
							$(PROJECT_PATH)/Source/Profiler.cpp \
							$(PROJECT_PATH)/Source/PS2VM.cpp \
							$(PROJECT_PATH)/Source/RegisterStateFile.cpp \
							$(PROJECT_PATH)/Source/StructCollectionStateFile.cpp \
							$(PROJECT_PATH)/Source/StructFile.cpp \
							$(PROJECT_PATH)/Source/VirtualPad.cpp \
							$(PROJECT_PATH)/Source/ui_android/GSH_OpenGLAndroid.cpp \
							$(PROJECT_PATH)/Source/ui_android/InputManager.cpp \
							$(PROJECT_PATH)/Source/ui_android/NativeInterop.cpp \
							$(PROJECT_PATH)/Source/ui_android/NativeShared.cpp \
							$(PROJECT_PATH)/Source/ui_android/SettingsManager.cpp \
							$(PROJECT_PATH)/Source/ui_android/SH_OpenSL.cpp \
							$(PROJECT_PATH)/Source/ui_android/StatsManager.cpp \
							$(PROJECT_PATH)/Source/Utils.cpp
LOCAL_CFLAGS			:= -Wno-extern-c-compat -D_IOP_EMULATE_MODULES -DDISABLE_LOGGING -DGLES_COMPATIBILITY
LOCAL_C_INCLUDES		:= $(BOOST_PATH) $(DEPENDENCIES_PATH)/bzip2-1.0.6 $(FRAMEWORK_PATH)/include $(CODEGEN_PATH)/include $(PROJECT_PATH)/include
LOCAL_CPP_FEATURES		:= exceptions rtti
LOCAL_LDLIBS 			:= -landroid -llog -lOpenSLES -lGLESv3 -lEGL -lz
LOCAL_STATIC_LIBRARIES	:= libCodeGen libFramework libbzip2 libboost cpufeatures

ifeq ($(APP_OPTIM),debug)
LOCAL_CFLAGS			+= -D_DEBUG
endif

ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
LOCAL_CFLAGS			+= -mcpu=cortex-a7
LOCAL_ARM_NEON			:= true
endif

include $(BUILD_SHARED_LIBRARY)

$(call import-module,android/cpufeatures)
