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


using namespace android;

#define VID_APPLE 0x5ac
#define PID_APPLE 0x1200

#define IPOD_INSERT_UEVENT_STRING	"add@/devices/platform/nxp-ehci/usb2/2-1"
#define IPOD_REMOVE_UEVENT_STRING	"remove@/devices/platform/nxp-ehci/usb2/2-1"

#define USB_IDVENDOR_PATH	"/sys/devices/platform/nxp-ehci/usb2/2-1/idVendor"
#define USB_IDPRODUCT_PATH	"/sys/devices/platform/nxp-ehci/usb2/2-1/idProduct"

#define IPOD_INSERT_DEVICE_PATH "/var/lib/ipod"
#define IPOD_PAIR_PATH "/var/lib/pair"

#define IPOD_CHANGE_IAP_DEVICE "/sys/class/iuihid"


class CNXUEventHandler {
public:
	CNXUEventHandler();
	virtual ~CNXUEventHandler();

	int 	get_ipod_mode();
	void 	set_ipod_mode(int mode);
	int 	Get_isIPOD();
	int		Write_String(char * path, char *buffer, int len);
	int		Read_String(char * path, char *buffer, int len);

private:
	int			isIPOD;
	int			ipod_mode;
	char		m_Desc[4096];
	char		m_path[4096];
	pthread_t	m_hUEventThread;
	static void	*UEventMonitorThreadStub( void * arg );
	void		UEventMonitorThread();

	pthread_t	m_hiPodPairThread;
	static void	*iPodPairMonitorThreadStub( void * arg );
	void		iPodPairMonitorThread();

};

void NX_StartUEventHandler();

#endif	//	__CNXUEVENTHANDLER_H__
