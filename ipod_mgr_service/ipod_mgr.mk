LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := ipod_dev_mgr_server
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES :=  CNXIPodManagerService.cpp 	\
			CNXUEventHandler.cpp	\
			main_server.cpp

LOCAL_SHARED_LIBRARIES := libutils libcutils libbinder libhardware_legacy
LOCAL_STATIC_LIBRARIES := 
LOCAL_C_INCLUDES += frameworks/base/include system/core/include
LOCAL_CFLAGS := -O2 -Wall -lpthread -DPACKAGE_STRING=\"1.0.9\" -DPACKAGE_VERSION=\"1.0.9\" -DPACKAGE_NAME=\"usbmuxd\" -DHAVE_LIBIMOBILEDEVICE -D__ANDROID__

include $(BUILD_EXECUTABLE)


include $(CLEAR_VARS)
LOCAL_MODULE := ipod_dev_mgr_client
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES :=  CNXIPodManagerService.cpp 	\
			NXIPodDeviceManager.cpp	\
			main_client.cpp
LOCAL_SHARED_LIBRARIES := libutils libcutils libbinder libhardware_legacy
LOCAL_STATIC_LIBRARIES := 
LOCAL_C_INCLUDES += frameworks/base/include system/core/include
include $(BUILD_EXECUTABLE)
