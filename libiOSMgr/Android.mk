LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional
# LOCAL_CFLAGS := -O2 -Wall -lpthread -DPACKAGE_STRING=\"1.0.9\" -DPACKAGE_VERSION=\"1.0.9\" -DPACKAGE_NAME=\"usbmuxd\" -DHAVE_LIBIMOBILEDEVICE -D__ANDROID__
LOCAL_SHARED_LIBRARIES := libutils libcutils libbinder libhardware_legacy
LOCAL_STATIC_LIBRARIES := 
LOCAL_C_INCLUDES += frameworks/base/include system/core/include
LOCAL_SRC_FILES :=  	CNXIPodManagerService.cpp 	\
			CNXUEventHandler.cpp		\
			NXIPodDeviceManager.cpp		\
			NXCCpuInfo.cpp

LOCAL_MODULE := libiOSMgr

include $(BUILD_SHARED_LIBRARY)

