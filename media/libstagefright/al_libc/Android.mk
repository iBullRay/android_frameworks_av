LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
    al_libc.c \
    al_uconv.c \
    al_detect.cpp \
    SinoDetect.cpp

LOCAL_C_INCLUDES := \
    $(call include-path-for, alsp) \
    $(TOP)/external/icu4c/common

LOCAL_SHARED_LIBRARIES := libutils libcutils libicuuc libion

LOCAL_PRELINK_MODULE := false
LOCAL_MODULE_TAGS := optional

LOCAL_MODULE := libalc

include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
    al_libc.c

LOCAL_C_INCLUDES := \
    $(call include-path-for, alsp) \
    $(TOP)/frameworks/av/include/alsp/inc

LOCAL_MODULE := libalc
include $(BUILD_STATIC_LIBRARY)
