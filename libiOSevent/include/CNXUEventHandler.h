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
//	File		: CNXUEventHandler.h
//	Description	: 
//	Author		: Seong-O Park(ray@nexell.co.kr)
//	Export		:
//	History		:
//
//------------------------------------------------------------------------------


#ifndef __CNXUEVENTHANDLER_H__
#define __CNXUEVENTHANDLER_H__

#include <stdint.h>

#include <binder/IInterface.h>
#include <binder/IBinder.h>
#include <binder/ProcessState.h>
#include <binder/IServiceManager.h>
#include <binder/IPCThreadState.h>

#include <NXIPodDeviceManager.h>


using namespace android;


class CNXUEventHandler {
public:
	CNXUEventHandler();
	virtual ~CNXUEventHandler();

	struct ios_libusb_device_descriptor *get_ipod_descriptors(void);
	int set_ipod_descriptors(void);
	int find_InterfaceClass(
		struct ios_libusb_device_descriptor *dev_descriptor,
		uint8_t cfg_value,
		uint8_t InterfaceClass,
		uint8_t InterfaceSubClass);
	int get_ipod_mode();
	void set_ipod_mode(int mode);
	int Get_isIPOD();
	int Set_usb_config(int num);
	int Set_ios_tethering(int num);

private:
	struct ios_libusb_device_descriptor m_device_descriptor;
	int isCPU;
	int isIPOD;
	int ipod_mode;
	int m_idVendor;
	int m_idProduct;
	char m_Desc[4096];
	char m_path[4096];

	pthread_t m_hUEventThread;
	static void	*UEventMonitorThreadStub( void * arg );
	void UEventMonitorThread();

	pthread_t m_hiPodPairThread;
	static void	*iPodPairMonitorThreadStub( void * arg );

	void iPodPairMonitorThread();
	int Check_guid(void);
	int Write_String(char * path, char *buffer, int len);
	int Read_String(char * path, char *buffer, int len);
};

void NX_StartUEventHandler();

#endif	//	__CNXUEVENTHANDLER_H__
