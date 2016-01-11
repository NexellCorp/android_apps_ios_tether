#ifndef LIBUSBAPI_H
#define LIBUSBAPI_H

//#include <libusb.h>
//#include "../libusb/libusb/libusb.h"
#ifdef __cplusplus
extern "C" {
#endif

#if (defined(LIBUSB_API_VERSION) && (LIBUSB_API_VERSION >= 0x01000102)) || (defined(LIBUSBX_API_VERSION) && (LIBUSBX_API_VERSION >= 0x01000102))
#define HAVE_LIBUSB_HOTPLUG_API 1
#else
# 'error HAVE_LIBUSB_HOTPLUG_API'
#endif

#define VID_APPLE 0x5ac

extern int libusbapi_init(libusb_hotplug_callback_fn cb_fn);
extern void libusbapi_shutdown(void);

#ifdef __cplusplus
}
#endif
#endif


