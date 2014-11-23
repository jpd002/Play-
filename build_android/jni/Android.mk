LOCAL_PATH:= $(call my-dir)

include $(LOCAL_PATH)/ExternalDependencies.mk

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
							../../Source/FpMulTruncate.cpp \
							../../Source/MemoryMap.cpp \
							../../Source/MemoryUtils.cpp \
							../../Source/MIPS.cpp \
							../../Source/MIPSAnalysis.cpp \
							../../Source/MIPSCoprocessor.cpp \
							../../Source/MIPSInstructionFactory.cpp \
							../../Source/MipsJitter.cpp \
							../../Source/MIPSReflection.cpp \
							../../Source/MIPSTags.cpp \
							../../Source/VUShared.cpp \
							../../Source/VUShared_Reflection.cpp
LOCAL_CFLAGS			:= -Wno-extern-c-compat
LOCAL_C_INCLUDES		:= $(BOOST_PATH) $(FRAMEWORK_PATH)/include $(CODEGEN_PATH)/include $(LOCAL_PATH)/../../include
LOCAL_CPP_FEATURES		:= exceptions rtti
LOCAL_LDLIBS 			:= -landroid
LOCAL_STATIC_LIBRARIES	:= libCodeGen libFramework

include $(BUILD_SHARED_LIBRARY)
