

#include "../libusb/libusb/libusb.h"
#include "libusb_api.h"


#ifdef HAVE_LIBUSB_HOTPLUG_API

static libusb_hotplug_callback_handle usb_hotplug_cb_handle;

int libusbapi_init(libusb_hotplug_callback_fn cb_fn)
{
	int res;

	res = libusb_init(NULL);
	if(res != 0) 
	{
		return -1;
	}

	if (libusb_has_capability(LIBUSB_CAP_HAS_HOTPLUG)) 
	{
		res = libusb_hotplug_register_callback(NULL, 
											LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED | LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, 
											LIBUSB_HOTPLUG_ENUMERATE, 
											VID_APPLE,
											LIBUSB_HOTPLUG_MATCH_ANY, 
											0,
											cb_fn, 
											NULL, 
											&usb_hotplug_cb_handle);
	} 

	return res;
}

void libusbapi_shutdown(void)
{
	libusb_hotplug_deregister_callback(NULL, usb_hotplug_cb_handle);
	libusb_exit(NULL);
}
#endif


