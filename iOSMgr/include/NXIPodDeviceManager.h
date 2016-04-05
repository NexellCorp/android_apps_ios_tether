//------------------------------------------------------------------------------
//
//	Copyright (C) 2015 Nexell Co. All Rights Reserved
//	Nexell Co. Proprietary & Confidential
//
//	NEXELL INFORMS THAT THIS CODE AND INFORMATION IS PROVIDED "AS IS" BASE
//  AND	WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING
//  BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS
//  FOR A PARTICULAR PURPOSE.
//
//	Module		: Android iPod Device Manager API
//	File		: NXIPodDeviceManager.h
//	Description	: 
//	Author		: Seong-O Park(ray@nexell.co.kr)
//	Export		:
//	History		:
//
//------------------------------------------------------------------------------

#ifndef __NXIPODDEVICEMANAGER_H__
#define	__NXIPODDEVICEMANAGER_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C"{
#endif

enum {
	//	API Error Code
	ERR_CHANGE_MODE		= -3,		//	Change mode Error
	ERR_PROPERTY_ACC	= -2,		//	Property Access Error
	ERR_BINDER_CON		= -1,		//	Binder Connection Error

	//	iPod Device Mode
	IPOD_MODE_CHANGING	= 98,		//	Mode Changing
	IPOD_MODE_NO_DEVIDE	= 99,		//	No device
};

#define USB_INTERFACE_DES_CNT 10
#define USB_INTERFACE_CNT 20
#define USB_DES_CNT 10


#define IPOD_USB_DES_PATH "/var/lib/usb_descriptor"

struct ios_libusb_interface_descriptor {
	/** Number of this interface */
	uint8_t  bInterfaceNumber;

	/** USB-IF class code for this interface. See \ref libusb_class_code. */
	uint8_t  bInterfaceClass;

	/** USB-IF subclass code for this interface, qualified by the
	 * bInterfaceClass value */
	uint8_t  bInterfaceSubClass;

	/** USB-IF protocol code for this interface, qualified by the
	 * bInterfaceClass and bInterfaceSubClass values */
	uint8_t  bInterfaceProtocol;

	/** Index of string descriptor describing this interface */
	uint8_t  iInterface;
};

struct ios_libusb_interface {
	/** Array of interface descriptors. The length of this array is determined
	 * by the num_altsetting field. */
	struct ios_libusb_interface_descriptor altsetting[USB_INTERFACE_DES_CNT];

	/** The number of alternate settings that belong to this interface */
	int num_altsetting;
};

struct ios_libusb_config_descriptor {

	/** Number of interfaces supported by this configuration */
	uint8_t  bNumInterfaces;

	/** Identifier value for this configuration */
	uint8_t  bConfigurationValue;

	/** Index of string descriptor describing this configuration */
	uint8_t  iConfiguration;

	/** Array of interfaces supported by this configuration. The length of
	 * this array is determined by the bNumInterfaces field. */
	struct ios_libusb_interface interface[USB_INTERFACE_CNT];
};

struct ios_libusb_device_descriptor {
	/** USB-IF class code for the device. See \ref libusb_class_code. */
	uint8_t  bDeviceClass;

	/** USB-IF subclass code for the device, qualified by the bDeviceClass
	 * value */
	uint8_t  bDeviceSubClass;

	/** USB-IF protocol code for the device, qualified by the bDeviceClass and
	 * bDeviceSubClass values */
	uint8_t  bDeviceProtocol;

	/** USB-IF vendor ID */
	uint16_t idVendor;

	/** USB-IF product ID */
	uint16_t idProduct;

	/** Number of possible configurations */
	uint8_t  bNumConfigurations;

	int current_config;

	struct ios_libusb_config_descriptor config_descriptor[USB_DES_CNT];
};

//
//	returns
//		 0 : OK
//		-1 : Binder Connection Error!
//		-2 : Property Setting Error
//
int32_t	NX_IPodChangeMode( int32_t mode );

//
//	returns
//		>0 : Current Mode Value
//		-1 : Binder Connection Error!
//		-2 : Property Read Error
//
int32_t NX_IPodGetCurrentMode(void);

#ifdef __cplusplus
}
#endif

#endif	//	__NXIPODDEVICEMANAGER_H__
