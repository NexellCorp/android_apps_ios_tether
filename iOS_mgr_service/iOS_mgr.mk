LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional
LOCAL_CFLAGS := -O2 -Wall -lpthread -DPACKAGE_STRING=\"1.0.9\" -DPACKAGE_VERSION=\"1.0.9\" -DPACKAGE_NAME=\"usbmuxd\" -DHAVE_LIBIMOBILEDEVICE -D__ANDROID__
LOCAL_SHARED_LIBRARIES := libutils libcutils libbinder libhardware_legacy libiOSMgr
LOCAL_STATIC_LIBRARIES := 
LOCAL_C_INCLUDES += frameworks/base/include system/core/include hardware/samsung_slsi/slsiap/ios_tether/libiOSMgr/include
LOCAL_SRC_FILES := main_server.cpp

LOCAL_MODULE := ipod_dev_mgr_server

include $(BUILD_EXECUTABLE)



include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional
LOCAL_SHARED_LIBRARIES := libutils libcutils libbinder libhardware_legacy libiOSMgr
LOCAL_STATIC_LIBRARIES := 
LOCAL_C_INCLUDES += frameworks/base/include system/core/include hardware/samsung_slsi/slsiap/ios_tether/libiOSMgr/include
LOCAL_SRC_FILES := main_client.cpp

LOCAL_MODULE := ipod_dev_mgr_client

include $(BUILD_EXECUTABLE)
