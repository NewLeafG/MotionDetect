LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

#opencv
#OPENCVROOT:= ${LOCAL_PATH}
OPENCV_CAMERA_MODULES:=on
OPENCV_INSTALL_MODULES:=on
OPENCV_LIB_TYPE:=SHARED
include ${LOCAL_PATH}/sdk/native/jni/OpenCV.mk

LOCAL_SRC_FILES := as_jni_part.cpp
LOCAL_LDLIBS := -lm -llog -ldl -lz
#LOCAL_CPPFLAGS += -fexceptions -frtti -std=c++11
LOCAL_MODULE := motiondetect

LOCAL_CPP_EXTENSION += .cxx .cpp

LOCAL_ARM_MODE := arm
ifeq ($(TARGET_ARCH_ABI), armeabi-v7a)
LOCAL_ARM_NEON := true
endif

LOCAL_CFLAGS += -pie -fPIE

include $(BUILD_SHARED_LIBRARY)