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
LOCAL_SRC_FILES			:=	../../Source/BasicBlock.cpp \
							../../Source/COP_FPU.cpp \
							../../Source/COP_FPU_Reflection.cpp \
							../../Source/COP_SCU.cpp \
							../../Source/COP_SCU_Reflection.cpp \
							../../Source/COP_VU.cpp \
							../../Source/COP_VU_Reflection.cpp \
							../../Source/DMAC.cpp \
							../../Source/Dmac_Channel.cpp \
							../../Source/EEAssembler.cpp \
							../../Source/ELF.cpp \
							../../Source/ElfFile.cpp \
							../../Source/FpMulTruncate.cpp \
							../../Source/FrameDump.cpp \
							../../Source/GIF.cpp \
							../../Source/GSHandler.cpp \
							../../Source/GsPixelFormats.cpp \
							../../Source/INTC.cpp \
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
LOCAL_CFLAGS			:= -Wno-extern-c-compat -D_IOP_EMULATE_MODULES
LOCAL_C_INCLUDES		:= $(BOOST_PATH) $(FRAMEWORK_PATH)/include $(CODEGEN_PATH)/include $(LOCAL_PATH)/../../include
LOCAL_CPP_FEATURES		:= exceptions rtti
LOCAL_LDLIBS 			:= -landroid -lz
LOCAL_STATIC_LIBRARIES	:= libCodeGen libFramework libboost

include $(BUILD_SHARED_LIBRARY)
