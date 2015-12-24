
#include <stdio.h>
#include <stdlib.h>

#include "log.h"
#include "../libusb/libusb/libusb.h"


extern void NX_StartUEventHandler();
extern void StartIPodDeviceManagerService();

#if (defined(LIBUSB_API_VERSION) && (LIBUSB_API_VERSION >= 0x01000102)) || (defined(LIBUSBX_API_VERSION) && (LIBUSBX_API_VERSION >= 0x01000102))
#define HAVE_LIBUSB_HOTPLUG_API 1
#endif
#ifdef HAVE_LIBUSB_HOTPLUG_API

#define usbmuxd_log


#define VID_APPLE 0x5ac

static libusb_hotplug_callback_handle usb_hotplug_cb_handle;

static int usb_hotplug_cb(libusb_context *ctx, libusb_device *device, libusb_hotplug_event event, void *user_data)
{
	if (LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED == event) {
		printf("\e[31m%s()\e[0m LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED  \n", __func__);
	} else if (LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT == event) {
		printf("\e[31m%s()\e[0m LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT  \n", __func__);
	} else {
		printf("\e[31m%s()\e[0m Unhandled event %d \n", __func__, event);
	}
	return 0;
}


int usb_init(void)
{
	int res;

	res = libusb_init(NULL);

	if(res != 0) {
		usbmuxd_log(LL_FATAL, "libusb_init failed: %d", res);
		return -1;
	}

	if (libusb_has_capability(LIBUSB_CAP_HAS_HOTPLUG)) {
		usbmuxd_log(LL_INFO, "Registering for libusb hotplug events");
		res = libusb_hotplug_register_callback(NULL, 
											LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED | LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, 
											LIBUSB_HOTPLUG_ENUMERATE, 
											VID_APPLE,
											LIBUSB_HOTPLUG_MATCH_ANY, 
											0,
											usb_hotplug_cb, 
											NULL, 
											&usb_hotplug_cb_handle);
		if (res != LIBUSB_SUCCESS) 
		{
			usbmuxd_log(LL_ERROR, "ERROR: Could not register for libusb hotplug events (%d)", res);
		}
	} else {
		usbmuxd_log(LL_ERROR, "libusb does not support hotplug events");
	}

	return res;
}

void usb_shutdown(void)
{
	usbmuxd_log(LL_DEBUG, "usb_shutdown");

	libusb_hotplug_deregister_callback(NULL, usb_hotplug_cb_handle);

	libusb_exit(NULL);
}
#endif



int main( int argc, char *argv[] )
{
	usb_init();

	// NX_StartUEventHandler();
	//StartIPodDeviceManagerService();
	
	return 0;
}
