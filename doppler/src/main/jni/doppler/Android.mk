LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_COMMON := ./

# LOCAL_C_INCLUDES += $(LOCAL_PATH)/../Zplayer/android  $(LOCAL_PATH)/source  \
#                     $(LOCAL_PATH)/../SoundTouch 

LOCAL_MODULE    := doppler

LOCAL_SRC_FILES := $(LOCAL_COMMON)/doppler_wrap.cpp \
                   $(LOCAL_COMMON)/dopplerpro.c \
                   $(LOCAL_COMMON)/calibrator.c \
                   $(LOCAL_COMMON)/fft.c

LOCAL_LDLIBS  += -llog -landroid 

# LOCAL_SHARED_LIBRARIES += ffmpeg yuv
# LOCAL_STATIC_LIBRARIES += soundtouch

LOCAL_CFLAGS += -fvisibility=hidden -fdata-sections -ffunction-sections  -D__STDC_FORMAT_MACROS -D__STDC_CONSTANT_MACROS

include $(BUILD_SHARED_LIBRARY)

