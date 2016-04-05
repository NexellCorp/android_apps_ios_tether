ifeq ($(BOARD_USES_IOS_IAP_TETHERING),true)

PRODUCT_COPY_FILES	+= \
	hardware/samsung_slsi/slsiap/ios_tether/libiOSevent/lib/libiOSevent.so:system/lib/libiOSevent.so

PRODUCT_PACKAGES += \
	libiconv		\
	libxml2_ios		\
	libusb_ios		\
	libplist		\
	libusbmuxd 		\
	libimobiledevice	\
	usbmuxdd		\
	libiOSMgr		\
	ipod_dev_mgr_server	\
	ipod_dev_mgr_client
#	libiOSevent
endif
