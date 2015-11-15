LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
    al_libc.c \
    al_uconv.c \
    al_detect.cpp \
    SinoDetect.cpp

LOCAL_C_INCLUDES := \
    $(call include-path-for, alsp) \
    $(TOP)/external/icu4c/common

LOCAL_SHARED_LIBRARIES := libutils libcutils libicuuc libion

LOCAL_PRELINK_MODULE := false
LOCAL_MODULE:= libalc
LOCAL_MODULE_TAGS := optional

include $(BUILD_STATIC_LIBRARY)
