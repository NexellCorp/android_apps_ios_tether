ifeq ($(BOARD_USES_IOS_IAP_TETHERING),true)

LOCAL_PATH := $(call my-dir)
include $(LOCAL_PATH)/android/jni/libiconv.mk \
	$(LOCAL_PATH)/android/jni/libplist.mk \
	$(LOCAL_PATH)/android/jni/libusbmuxd.mk \
	$(LOCAL_PATH)/android/jni/libusb.mk \
	$(LOCAL_PATH)/android/jni/libimobiledevice.mk \
	$(LOCAL_PATH)/android/jni/usbmuxd.mk \
	$(LOCAL_PATH)/ipod_mgr_service/ipod_mgr.mk

endif
