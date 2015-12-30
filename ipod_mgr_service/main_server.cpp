
#include <sys/types.h>
#include <sys/stat.h>
#include <linux/fs.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>		//	atoi
#include <unistd.h>		//	getopt & optarg
#include <errno.h>
#include <poll.h>
#include <pthread.h>
#include <hardware_legacy/uevent.h>
#include "CNXIPodManagerService.h"

//#include <libusb.h>
#include "../libusb/libusb/libusb.h"

extern void NX_StartUEventHandler();
extern void StartIPodDeviceManagerService();

#if (defined(LIBUSB_API_VERSION) && (LIBUSB_API_VERSION >= 0x01000102)) || (defined(LIBUSBX_API_VERSION) && (LIBUSBX_API_VERSION >= 0x01000102))
//#define HAVE_LIBUSB_HOTPLUG_API 1
#endif
#ifdef HAVE_LIBUSB_HOTPLUG_API

#define usbmuxd_log

#define VID_APPLE 0x5ac

int usb_hotplug_cb_handle;

static int usb_hotplug_cb(libusb_context *ctx, libusb_device *device, libusb_hotplug_event event, void *user_data)
{
	if (LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED == event) {
		ALOGD("\e[31m%s() \e[0mLIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED  \n", __func__);
	} else if (LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT == event) {
		ALOGD( "\e[31m%s() \e[0mLIBUSB_HOTPLUG_EVENT_DEVICE_LEFT  \n", __func__);
	} else {
		ALOGD( "\e[31m%s()  Unhandled event %d \e[0m", event);
	}
	return 0;
}

int usb_init(void)
{
	int res;
	ALOGD( "%s()  \n", __func__);

	res = libusb_init(NULL);

	if(res != 0) {
		ALOGE( "libusb_init failed: %d", res);
		return -1;
	}

	if (libusb_has_capability(LIBUSB_CAP_HAS_HOTPLUG)) {
		ALOGD("Registering for libusb hotplug events");
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
			ALOGE( "ERROR: Could not register for libusb hotplug events (%d)", res);
		}
	} else {
		ALOGE("libusb does not support hotplug events");
	}

	return res;
}

void usb_shutdown(void)
{
	ALOGD("usb_shutdown");

	libusb_hotplug_deregister_callback(NULL, usb_hotplug_cb_handle);

	libusb_exit(NULL);
}
#endif



int main( int argc, char *argv[] )
{
	// NX_StartUEventHandler();
	ALOGD( "%s()  \n", __func__);

#ifdef HAVE_LIBUSB_HOTPLUG_API
	usb_init();
#endif

	StartIPodDeviceManagerService();
	return 0;
}
