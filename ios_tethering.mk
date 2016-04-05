ifeq ($(BOARD_USES_IOS_IAP_TETHERING),true)

PRODUCT_COPY_FILES	+= \
	hardware/samsung_slsi/slsiap/ios_tether/libiOSMgr/lib/libiOSMgr.so:system/lib/libiOSMgr.so

PRODUCT_PACKAGES += \
	libiconv		\
	libxml2_ios		\
	libusb_ios		\
	libplist		\
	libusbmuxd 		\
	libimobiledevice	\
	usbmuxdd		\
	libiOSService		\
	ipod_dev_mgr_server	\
	ipod_dev_mgr_client
#	libiOSMgr
endif
