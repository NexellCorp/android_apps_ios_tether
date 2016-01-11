LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := libusbapi
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES :=  libusb_api.c

LOCAL_SHARED_LIBRARIES := libutils libcutils libbinder libhardware_legacy libusb
LOCAL_STATIC_LIBRARIES := 
LOCAL_C_INCLUDES += frameworks/base/include system/core/include
LOCAL_C_INCLUDES += ../libusb/libusb
LOCAL_CFLAGS := -O2 -Wall

include $(BUILD_SHARED_LIBRARY)

