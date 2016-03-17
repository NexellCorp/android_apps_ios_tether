ifeq ($(BOARD_USES_IOS_IAP_TETHERING),true)

PRODUCT_COPY_FILES	+= \
	hardware/samsung_slsi/slsiap/ios_tether/libiOSMgr/lib/libiOSMgr.so:system/lib/libiOSMgr.so	\
	hardware/samsung_slsi/slsiap/ios_tether/iOS_mgr_service/ipod_dev_mgr_server:system/bin/ipod_dev_mgr_server	\
	hardware/samsung_slsi/slsiap/ios_tether/iOS_mgr_service/ipod_dev_mgr_client:system/bin/ipod_dev_mgr_client

PRODUCT_PACKAGES += \
	libiconv		\
	libxml2_ios		\
	libusb_ios		\
	libplist		\
	libusbmuxd 		\
	libimobiledevice	\
	usbmuxdd

endif
