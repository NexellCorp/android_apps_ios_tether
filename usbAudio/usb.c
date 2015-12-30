/*
 * usb.c
 *
 * Copyright (C) 2009 Hector Martin <hector@marcansoft.com>
 * Copyright (C) 2009 Nikias Bassen <nikias@gmx.li>
 * Copyright (C) 2009 Martin Szulecki <opensuse@sukimashita.com>
 * Copyright (C) 2014 Mikkel Kamstrup Erlandsen <mikkel.kamstrup@xamarin.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 or version 3.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <libusb.h>

#include "usb.h"
#include "log.h"
#include "utils.h"

#if (defined(LIBUSB_API_VERSION) && (LIBUSB_API_VERSION >= 0x01000102)) || (defined(LIBUSBX_API_VERSION) && (LIBUSBX_API_VERSION >= 0x01000102))
#define HAVE_LIBUSB_HOTPLUG_API 1
#endif

// interval for device connection/disconnection polling, in milliseconds
// we need this because there is currently no asynchronous device discovery mechanism in libusb
#define DEVICE_POLL_TIME 1000

// Number of parallel bulk transfers we have running for reading data from the device.
// Older versions of usbmuxd kept only 1, which leads to a mostly dormant USB port.
// 3 seems to be an all round sensible number - giving better read perf than
// Apples usbmuxd, at least.
#define NUM_RX_LOOPS 3

struct usb_device {
	libusb_device_handle *dev;
	uint8_t bus, address;
	uint16_t vid, pid;
	char serial[256];
	int alive;
	uint8_t interface, ep_in, ep_out;
	struct collection rx_xfers;
	struct collection tx_xfers;
	int wMaxPacketSize;
	uint64_t speed;
};

static struct collection device_list;

static struct timeval next_dev_poll_time;

static int devlist_failures;
static int device_polling;
static int device_hotplug = 1;

static void usb_disconnect(struct usb_device *dev)
{
	if(!dev->dev) {
		return;
	}

	// kill the rx xfer and tx xfers and try to make sure the callbacks
	// get called before we free the device
	FOREACH(struct libusb_transfer *xfer, &dev->rx_xfers) {
		usbmuxd_log(LL_DEBUG, "usb_disconnect: cancelling RX xfer %p", xfer);
		libusb_cancel_transfer(xfer);
	} ENDFOREACH

	FOREACH(struct libusb_transfer *xfer, &dev->tx_xfers) {
		usbmuxd_log(LL_DEBUG, "usb_disconnect: cancelling TX xfer %p", xfer);
		libusb_cancel_transfer(xfer);
	} ENDFOREACH

	// Busy-wait until all xfers are closed
	while(collection_count(&dev->rx_xfers) || collection_count(&dev->tx_xfers)) {
		struct timeval tv;
		int res;

		tv.tv_sec = 0;
		tv.tv_usec = 1000;
		if((res = libusb_handle_events_timeout(NULL, &tv)) < 0) {
			usbmuxd_log(LL_ERROR, "libusb_handle_events_timeout for usb_disconnect failed: %d", res);
			break;
		}
	}

	collection_free(&dev->tx_xfers);
	collection_free(&dev->rx_xfers);
	libusb_release_interface(dev->dev, dev->interface);
	libusb_close(dev->dev);
	dev->dev = NULL;
	collection_remove(&device_list, dev);
	free(dev);
}

static void reap_dead_devices(void) {
	FOREACH(struct usb_device *usbdev, &device_list) {
		if(!usbdev->alive) {
			device_remove(usbdev);
			usb_disconnect(usbdev);
		}
	} ENDFOREACH
}

// Callback from write operation
static void tx_callback(struct libusb_transfer *xfer)
{
	struct usb_device *dev = xfer->user_data;
	usbmuxd_log(LL_SPEW, "TX callback dev %d-%d len %d -> %d status %d", dev->bus, dev->address, xfer->length, xfer->actual_length, xfer->status);
	if(xfer->status != LIBUSB_TRANSFER_COMPLETED) {
		switch(xfer->status) {
			case LIBUSB_TRANSFER_COMPLETED: //shut up compiler
			case LIBUSB_TRANSFER_ERROR:
				// funny, this happens when we disconnect the device while waiting for a transfer, sometimes
				usbmuxd_log(LL_INFO, "Device %d-%d TX aborted due to error or disconnect", dev->bus, dev->address);
				break;
			case LIBUSB_TRANSFER_TIMED_OUT:
				usbmuxd_log(LL_ERROR, "TX transfer timed out for device %d-%d", dev->bus, dev->address);
				break;
			case LIBUSB_TRANSFER_CANCELLED:
				usbmuxd_log(LL_DEBUG, "Device %d-%d TX transfer cancelled", dev->bus, dev->address);
				break;
			case LIBUSB_TRANSFER_STALL:
				usbmuxd_log(LL_ERROR, "TX transfer stalled for device %d-%d", dev->bus, dev->address);
				break;
			case LIBUSB_TRANSFER_NO_DEVICE:
				// other times, this happens, and also even when we abort the transfer after device removal
				usbmuxd_log(LL_INFO, "Device %d-%d TX aborted due to disconnect", dev->bus, dev->address);
				break;
			case LIBUSB_TRANSFER_OVERFLOW:
				usbmuxd_log(LL_ERROR, "TX transfer overflow for device %d-%d", dev->bus, dev->address);
				break;
			// and nothing happens (this never gets called) if the device is freed after a disconnect! (bad)
			default:
				// this should never be reached.
				break;
		}
		// we can't usb_disconnect here due to a deadlock, so instead mark it as dead and reap it after processing events
		// we'll do device_remove there too
		dev->alive = 0;
	}
	if(xfer->buffer)
		free(xfer->buffer);
	collection_remove(&dev->tx_xfers, xfer);
	libusb_free_transfer(xfer);
}

int usb_send(struct usb_device *dev, const unsigned char *buf, int length)
{
	int res;
	struct libusb_transfer *xfer = libusb_alloc_transfer(0);
	libusb_fill_bulk_transfer(xfer, dev->dev, dev->ep_out, (void*)buf, length, tx_callback, dev, 0);
	if((res = libusb_submit_transfer(xfer)) < 0) {
		usbmuxd_log(LL_ERROR, "Failed to submit TX transfer %p len %d to device %d-%d: %d", buf, length, dev->bus, dev->address, res);
		libusb_free_transfer(xfer);
		return res;
	}
	collection_add(&dev->tx_xfers, xfer);
	if (length % dev->wMaxPacketSize == 0) {
		usbmuxd_log(LL_DEBUG, "Send ZLP");
		// Send Zero Length Packet
		xfer = libusb_alloc_transfer(0);
		void *buffer = malloc(1);
		libusb_fill_bulk_transfer(xfer, dev->dev, dev->ep_out, buffer, 0, tx_callback, dev, 0);
		if((res = libusb_submit_transfer(xfer)) < 0) {
			usbmuxd_log(LL_ERROR, "Failed to submit TX ZLP transfer to device %d-%d: %d", dev->bus, dev->address, res);
			libusb_free_transfer(xfer);
			return res;
		}
		collection_add(&dev->tx_xfers, xfer);
	}
	return 0;
}

// Callback from read operation
// Under normal operation this issues a new read transfer request immediately,
// doing a kind of read-callback loop
static void rx_callback(struct libusb_transfer *xfer)
{
	struct usb_device *dev = xfer->user_data;
	usbmuxd_log(LL_SPEW, "RX callback dev %d-%d len %d status %d", dev->bus, dev->address, xfer->actual_length, xfer->status);
	if(xfer->status == LIBUSB_TRANSFER_COMPLETED) {
		device_data_input(dev, xfer->buffer, xfer->actual_length);
		libusb_submit_transfer(xfer);
	} else {
		switch(xfer->status) {
			case LIBUSB_TRANSFER_COMPLETED: //shut up compiler
			case LIBUSB_TRANSFER_ERROR:
				// funny, this happens when we disconnect the device while waiting for a transfer, sometimes
				usbmuxd_log(LL_INFO, "Device %d-%d RX aborted due to error or disconnect", dev->bus, dev->address);
				break;
			case LIBUSB_TRANSFER_TIMED_OUT:
				usbmuxd_log(LL_ERROR, "RX transfer timed out for device %d-%d", dev->bus, dev->address);
				break;
			case LIBUSB_TRANSFER_CANCELLED:
				usbmuxd_log(LL_DEBUG, "Device %d-%d RX transfer cancelled", dev->bus, dev->address);
				break;
			case LIBUSB_TRANSFER_STALL:
				usbmuxd_log(LL_ERROR, "RX transfer stalled for device %d-%d", dev->bus, dev->address);
				break;
			case LIBUSB_TRANSFER_NO_DEVICE:
				// other times, this happens, and also even when we abort the transfer after device removal
				usbmuxd_log(LL_INFO, "Device %d-%d RX aborted due to disconnect", dev->bus, dev->address);
				break;
			case LIBUSB_TRANSFER_OVERFLOW:
				usbmuxd_log(LL_ERROR, "RX transfer overflow for device %d-%d", dev->bus, dev->address);
				break;
			// and nothing happens (this never gets called) if the device is freed after a disconnect! (bad)
			default:
				// this should never be reached.
				break;
		}

		free(xfer->buffer);
		collection_remove(&dev->rx_xfers, xfer);
		libusb_free_transfer(xfer);

		// we can't usb_disconnect here due to a deadlock, so instead mark it as dead and reap it after processing events
		// we'll do device_remove there too
		dev->alive = 0;
	}
}

// Start a read-callback loop for this device
static int start_rx_loop(struct usb_device *dev)
{
	int res;
	void *buf;
	struct libusb_transfer *xfer = libusb_alloc_transfer(0);
	buf = malloc(USB_MRU);
	libusb_fill_bulk_transfer(xfer, dev->dev, dev->ep_in, buf, USB_MRU, rx_callback, dev, 0);
	if((res = libusb_submit_transfer(xfer)) != 0) {
		usbmuxd_log(LL_ERROR, "Failed to submit RX transfer to device %d-%d: %d", dev->bus, dev->address, res);
		libusb_free_transfer(xfer);
		return res;
	}

	collection_add(&dev->rx_xfers, xfer);

	return 0;
}

static int usb_device_add(libusb_device* dev)
{
	int j, res;
	// the following are non-blocking operations on the device list
	uint8_t bus = libusb_get_bus_number(dev);
	uint8_t address = libusb_get_device_address(dev);
	struct libusb_device_descriptor devdesc;
	int found = 0;
	//printf("## \e[31m PJSMSG \e[0m [%s():%s:%d\t]  \n", __func__, strrchr(__FILE__, '/')+1, __LINE__);

	FOREACH(struct usb_device *usbdev, &device_list) {
		if(usbdev->bus == bus && usbdev->address == address) {
			usbdev->alive = 1;
			found = 1;
			break;
		}
	} ENDFOREACH
	if(found)
		return 0; //device already found

	if((res = libusb_get_device_descriptor(dev, &devdesc)) != 0) {
		usbmuxd_log(LL_WARNING, "Could not get device descriptor for device %d-%d: %d", bus, address, res);
		return -1;
	}
	if(devdesc.idVendor != VID_APPLE)
		return -1;
	if((devdesc.idProduct < PID_RANGE_LOW) ||
		(devdesc.idProduct > PID_RANGE_MAX))
		return -1;

	libusb_device_handle *handle;
	usbmuxd_log(LL_INFO, "Found new device with v/p %04x:%04x at %d-%d", devdesc.idVendor, devdesc.idProduct, bus, address);

	if((res = libusb_open(dev, &handle)) != 0) {
		usbmuxd_log(LL_WARNING, "Could not open device %d-%d: %d", bus, address, res);
		return -1;
	}

	int current_config = 0;
	if((res = libusb_get_configuration(handle, &current_config)) != 0) {
		usbmuxd_log(LL_WARNING, "Could not get configuration for device %d-%d: %d", bus, address, res);
		libusb_close(handle);
		return -1;
	}
	if (current_config != devdesc.bNumConfigurations) {
		struct libusb_config_descriptor *config;
		if((res = libusb_get_active_config_descriptor(dev, &config)) != 0) {
			usbmuxd_log(LL_NOTICE, "Could not get old configuration descriptor for device %d-%d: %d", bus, address, res);
		} else {
			for(j=0; j<config->bNumInterfaces; j++) {
				const struct libusb_interface_descriptor *intf = &config->interface[j].altsetting[0];
				if((res = libusb_kernel_driver_active(handle, intf->bInterfaceNumber)) < 0) {
					usbmuxd_log(LL_NOTICE, "Could not check kernel ownership of interface %d for device %d-%d: %d", intf->bInterfaceNumber, bus, address, res);
					continue;
				}
				if(res == 1) {
					usbmuxd_log(LL_INFO, "Detaching kernel driver for device %d-%d, interface %d", bus, address, intf->bInterfaceNumber);
					if((res = libusb_detach_kernel_driver(handle, intf->bInterfaceNumber)) < 0) {
						usbmuxd_log(LL_WARNING, "Could not detach kernel driver (%d), configuration change will probably fail!", res);
						continue;
					}
				}
			}
			libusb_free_config_descriptor(config);
		}

		printf( "Setting configuration for device %d-%d, from %d to %d \n", bus, address, current_config, devdesc.bNumConfigurations);

		//res = libusb_set_configuration(handle, devdesc.bNumConfigurations);
		res = libusb_set_configuration(handle, 2);
		//printf("## \e[31m PJSMSG \e[0m [%s():%s:%d\t] libusb_set_configuration res:%d \n", __func__, strrchr(__FILE__, '/')+1, __LINE__, res);

		if((res) != 0) {
			printf( "Could not set configuration %d for device %d-%d: %d \n", devdesc.bNumConfigurations, bus, address, res);
			libusb_close(handle);
			return -1;
		}
	}

	libusb_close(handle);

	return 0;
}

int usb_discover(void)
{
	int cnt, i;
	int valid_count = 0;
	libusb_device **devs;

	cnt = libusb_get_device_list(NULL, &devs);
	if(cnt < 0) {
		usbmuxd_log(LL_WARNING, "Could not get device list: %d", cnt);
		devlist_failures++;
		// sometimes libusb fails getting the device list if you've just removed something
		if(devlist_failures > 5) {
			usbmuxd_log(LL_FATAL, "Too many errors getting device list");
			return cnt;
		} else {
			get_tick_count(&next_dev_poll_time);
			next_dev_poll_time.tv_usec += DEVICE_POLL_TIME * 1000;
			next_dev_poll_time.tv_sec += next_dev_poll_time.tv_usec / 1000000;
			next_dev_poll_time.tv_usec = next_dev_poll_time.tv_usec % 1000000;
			return 0;
		}
	}
	devlist_failures = 0;

	usbmuxd_log(LL_SPEW, "usb_discover: scanning %d devices", cnt);

	// Mark all devices as dead, and do a mark-sweep like
	// collection of dead devices
	FOREACH(struct usb_device *usbdev, &device_list) {
		usbdev->alive = 0;
	} ENDFOREACH

	// Enumerate all USB devices and mark the ones we already know
	// about as live, again
	for(i=0; i<cnt; i++) {
		libusb_device *dev = devs[i];
		if (usb_device_add(dev) < 0) {
			continue;
		}
		valid_count++;
	}

	// Clean out any device we didn't mark back as live
	reap_dead_devices();

	libusb_free_device_list(devs, 1);

	get_tick_count(&next_dev_poll_time);
	next_dev_poll_time.tv_usec += DEVICE_POLL_TIME * 1000;
	next_dev_poll_time.tv_sec += next_dev_poll_time.tv_usec / 1000000;
	next_dev_poll_time.tv_usec = next_dev_poll_time.tv_usec % 1000000;

	return valid_count;
}

const char *usb_get_serial(struct usb_device *dev)
{
	if(!dev->dev) {
		return NULL;
	}
	return dev->serial;
}

uint32_t usb_get_location(struct usb_device *dev)
{
	if(!dev->dev) {
		return 0;
	}
	return (dev->bus << 16) | dev->address;
}

uint16_t usb_get_pid(struct usb_device *dev)
{
	if(!dev->dev) {
		return 0;
	}
	return dev->pid;
}

uint64_t usb_get_speed(struct usb_device *dev)
{
	if (!dev->dev) {
		return 0;
	}
	return dev->speed;
}

void usb_get_fds(struct fdlist *list)
{
	const struct libusb_pollfd **usbfds;
	const struct libusb_pollfd **p;
	usbfds = libusb_get_pollfds(NULL);
	if(!usbfds) {
		usbmuxd_log(LL_ERROR, "libusb_get_pollfds failed");
		return;
	}
	p = usbfds;
	while(*p) {
		fdlist_add(list, FD_USB, (*p)->fd, (*p)->events);
		p++;
	}
	free(usbfds);
}

void usb_autodiscover(int enable)
{
	usbmuxd_log(LL_DEBUG, "usb polling enable: %d", enable);
	device_polling = enable;
	device_hotplug = enable;
}

static int dev_poll_remain_ms(void)
{
	int msecs;
	struct timeval tv;
	if(!device_polling)
		return 100000; // devices will never be polled if this is > 0
	get_tick_count(&tv);
	msecs = (next_dev_poll_time.tv_sec - tv.tv_sec) * 1000;
	msecs += (next_dev_poll_time.tv_usec - tv.tv_usec) / 1000;
	if(msecs < 0)
		return 0;
	return msecs;
}

int usb_get_timeout(void)
{
	struct timeval tv;
	int msec;
	int res;
	int pollrem;
	pollrem = dev_poll_remain_ms();
	res = libusb_get_next_timeout(NULL, &tv);
	if(res == 0)
		return pollrem;
	if(res < 0) {
		usbmuxd_log(LL_ERROR, "libusb_get_next_timeout failed: %d", res);
		return pollrem;
	}
	msec = tv.tv_sec * 1000;
	msec += tv.tv_usec / 1000;
	if(msec > pollrem)
		return pollrem;
	return msec;
}

int usb_process(void)
{
	int res;
	struct timeval tv;
	tv.tv_sec = tv.tv_usec = 0;
	res = libusb_handle_events_timeout(NULL, &tv);
	if(res < 0) {
		usbmuxd_log(LL_ERROR, "libusb_handle_events_timeout failed: %d", res);
		return res;
	}

	// reap devices marked dead due to an RX error
	reap_dead_devices();

	if(dev_poll_remain_ms() <= 0) {
		res = usb_discover();
		if(res < 0) {
			usbmuxd_log(LL_ERROR, "usb_discover failed: %d", res);
			return res;
		}
	}
	return 0;
}

int usb_process_timeout(int msec)
{
	int res;
	struct timeval tleft, tcur, tfin;
	get_tick_count(&tcur);
	tfin.tv_sec = tcur.tv_sec + (msec / 1000);
	tfin.tv_usec = tcur.tv_usec + (msec % 1000) * 1000;
	tfin.tv_sec += tfin.tv_usec / 1000000;
	tfin.tv_usec %= 1000000;
	while((tfin.tv_sec > tcur.tv_sec) || ((tfin.tv_sec == tcur.tv_sec) && (tfin.tv_usec > tcur.tv_usec))) {
		tleft.tv_sec = tfin.tv_sec - tcur.tv_sec;
		tleft.tv_usec = tfin.tv_usec - tcur.tv_usec;
		if(tleft.tv_usec < 0) {
			tleft.tv_usec += 1000000;
			tleft.tv_sec -= 1;
		}
		res = libusb_handle_events_timeout(NULL, &tleft);
		if(res < 0) {
			usbmuxd_log(LL_ERROR, "libusb_handle_events_timeout failed: %d", res);
			return res;
		}
		// reap devices marked dead due to an RX error
		reap_dead_devices();
		get_tick_count(&tcur);
	}
	return 0;
}

#ifdef HAVE_LIBUSB_HOTPLUG_API
static libusb_hotplug_callback_handle usb_hotplug_cb_handle;

static int usb_hotplug_cb(libusb_context *ctx, libusb_device *device, libusb_hotplug_event event, void *user_data)
{
	if (LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED == event) {
		if (device_hotplug) {
			usb_device_add(device);
		}
	} else if (LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT == event) {
		uint8_t bus = libusb_get_bus_number(device);
		uint8_t address = libusb_get_device_address(device);
		FOREACH(struct usb_device *usbdev, &device_list) {
			if(usbdev->bus == bus && usbdev->address == address) {
				usbdev->alive = 0;
				device_remove(usbdev);
				break;
			}
		} ENDFOREACH
	} else {
		usbmuxd_log(LL_ERROR, "Unhandled event %d", event);
	}
	return 0;
}
#endif

static int set_config_usb_device(libusb_device* dev, int config_id)
{
	int j, res;
	// the following are non-blocking operations on the device list
	uint8_t bus = libusb_get_bus_number(dev);
	uint8_t address = libusb_get_device_address(dev);
	struct libusb_device_descriptor devdesc;
	int found = 0;

	//printf("## \e[31m PJSMSG \e[0m [%s():%s:%d\t]  \n", __func__, strrchr(__FILE__, '/')+1, __LINE__);

	FOREACH(struct usb_device *usbdev, &device_list) {
		if(usbdev->bus == bus && usbdev->address == address) {
			usbdev->alive = 1;
			found = 1;
			break;
		}
	} ENDFOREACH
	if(found)
		return 0; //device already found

	if((res = libusb_get_device_descriptor(dev, &devdesc)) != 0) {
		printf("## \e[31m Could not get device descriptor for device %d-%d: %d \e[0m\n", bus, address, res);
		return -1;
	}
	if(devdesc.idVendor != VID_APPLE)
		return -1;
	if((devdesc.idProduct < PID_RANGE_LOW) ||
		(devdesc.idProduct > PID_RANGE_MAX))
		return -1;

	libusb_device_handle *handle;
	printf("##  Found new device with v/p %04x:%04x at %d-%d\n", devdesc.idVendor, devdesc.idProduct, bus, address);

	if((res = libusb_open(dev, &handle)) != 0) {
		printf("## \e[31m Could not open device %d-%d: %d\e[0m\n", bus, address, res);
		return -1;
	}

	int current_config = 0;
	if((res = libusb_get_configuration(handle, &current_config)) != 0) {
		printf("## \e[31m Could not get configuration for device %d-%d: %d\e[0m\n", bus, address, res);
		libusb_close(handle);
		return -1;
	}

	printf("Configuration status. config_id:%d, current_config:%d \n", config_id, current_config);

	if (config_id != current_config) 
	{
		struct libusb_config_descriptor *config;
		if((res = libusb_get_active_config_descriptor(dev, &config)) != 0) {
			usbmuxd_log(LL_NOTICE, "Could not get old configuration descriptor for device %d-%d: %d", bus, address, res);
		} else {
			for(j=0; j<config->bNumInterfaces; j++) {
				const struct libusb_interface_descriptor *intf = &config->interface[j].altsetting[0];
				if((res = libusb_kernel_driver_active(handle, intf->bInterfaceNumber)) < 0) {
					usbmuxd_log(LL_NOTICE, "Could not check kernel ownership of interface %d for device %d-%d: %d", intf->bInterfaceNumber, bus, address, res);
					continue;
				}
				if(res == 1) {
					usbmuxd_log(LL_INFO, "Detaching kernel driver for device %d-%d, interface %d", bus, address, intf->bInterfaceNumber);
					if((res = libusb_detach_kernel_driver(handle, intf->bInterfaceNumber)) < 0) {
						usbmuxd_log(LL_WARNING, "Could not detach kernel driver (%d), configuration change will probably fail!", res);
						continue;
					}
				}
			}
			libusb_free_config_descriptor(config);
		}

		printf( "Setting configuration for device %d-%d, from %d to %d \n", bus, address, current_config, config_id);

		//res = libusb_set_configuration(handle, devdesc.bNumConfigurations);
		res = libusb_set_configuration(handle, config_id);
		//printf("## \e[31m PJSMSG \e[0m [%s():%s:%d\t] libusb_set_configuration res:%d \n", __func__, strrchr(__FILE__, '/')+1, __LINE__, res);

		if((res) != 0) {
			printf("## \e[31m Could not set configuration %d for device %d-%d: %d \e[0m\n", devdesc.bNumConfigurations, bus, address, res);
			libusb_close(handle);
			return -1;
		}
	}

	libusb_close(handle);

	return 0;
}


int set_config_ipod_Audio(int config_id)
{
	int cnt, i;
	int valid_count = 0;
	libusb_device **devs;

	cnt = libusb_get_device_list(NULL, &devs);

	//printf("## \e[31m PJSMSG \e[0m [%s():%s:%d\t] cnt:%d \n", __func__, strrchr(__FILE__, '/')+1, __LINE__, cnt);

	if(cnt < 0) {
		usbmuxd_log(LL_WARNING, "Could not get device list: %d", cnt);
		devlist_failures++;
		// sometimes libusb fails getting the device list if you've just removed something
		if(devlist_failures > 5) {
			usbmuxd_log(LL_FATAL, "Too many errors getting device list");
			return cnt;
		} else {
			return 0;
		}
	}
	devlist_failures = 0;

	usbmuxd_log(LL_SPEW, "usb_discover: scanning %d devices", cnt);

	// Mark all devices as dead, and do a mark-sweep like
	// collection of dead devices
	FOREACH(struct usb_device *usbdev, &device_list) {
		usbdev->alive = 0;
	} ENDFOREACH

	// Enumerate all USB devices and mark the ones we already know
	// about as live, again
	for(i=0; i<cnt; i++) {
		libusb_device *dev = devs[i];
		if (set_config_usb_device(dev, config_id) < 0) {
			continue;
		}
		valid_count++;
	}

	libusb_free_device_list(devs, 1);


	return valid_count;
}

int usb_init(int config_id)
{
	int res;
	usbmuxd_log(LL_DEBUG, "usb_init for linux / libusb 1.0");
	//printf("## \e[31m PJSMSG \e[0m [%s():%s:%d\t]  \n", __func__, strrchr(__FILE__, '/')+1, __LINE__);

	res = libusb_init(NULL);
	if(res != 0) {
		//printf("## \e[31m PJSMSG  [%s():%s:%d\t] libusb_init failed: %d \e[0m\n", __func__, strrchr(__FILE__, '/')+1, __LINE__, res);
		usbmuxd_log(LL_FATAL, "libusb_init failed: %d", res);
		return -1;
	}

	collection_init(&device_list);

	res = set_config_ipod_Audio(config_id);
	
	return res;
}

void usb_shutdown(void)
{
	//printf("## \e[31m PJSMSG \e[0m [%s():%s:%d\t]  \n", __func__, strrchr(__FILE__, '/')+1, __LINE__);

	usbmuxd_log(LL_DEBUG, "usb_shutdown");

	collection_free(&device_list);

	libusb_exit(NULL);
}
