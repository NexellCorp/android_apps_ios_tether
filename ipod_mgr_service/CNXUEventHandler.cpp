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
//	File		: CNXUEventHandler.cpp
//	Description	: 
//	Author		: Seong-O Park(ray@nexell.co.kr)
//	Export		:
//	History		:
//
//------------------------------------------------------------------------------
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

#include "NXIPodDeviceManager.h"
#include "CNXUEventHandler.h"

#if 0
//#include <libusb_api.h>
#include "../libusb/libusb/libusb.h"
#include "../libusb_api/libusb_api.h"


//extern int libusbapi_init(libusb_hotplug_callback_fn cb_fn);
//extern int libusbapi_init(void);//(libusb_hotplug_callback_fn cb_fn);

#ifdef __cplusplus
extern "C" {
#endif
	static int libusbapi_hotplug_cb(libusb_context *ctx, libusb_device *device, libusb_hotplug_event event, void *user_data)
	{
		if (LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED == event) {
			ALOGD("[CNXUEventHandler] \e[31mlibusbapi_hotplug_cb() \e[0mLIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED  \n");
		} else if (LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT == event) {
			ALOGD( "[CNXUEventHandler] \e[31mlibusbapi_hotplug_cb() \e[0mLIBUSB_HOTPLUG_EVENT_DEVICE_LEFT  \n");
		} else {
			ALOGD( "[CNXUEventHandler] \e[31mlibusbapi_hotplug_cb()  Unhandled event %d \e[0m", event);
		}
		return 0;
	}
#ifdef __cplusplus
}
#endif
#endif

static CNXUEventHandler *gstpEventHandler = NULL;

CNXUEventHandler::CNXUEventHandler()
	: isIPOD(0), ipod_mode(IPOD_MODE_NO_DEVIDE)
{
	struct stat fst;
	int ret = 0;

	if( gstpEventHandler == NULL )
	{
		if( 0 != pthread_create( &m_hUEventThread, NULL, UEventMonitorThreadStub, this ) )
		{
			ALOGD("[CNXUEventHandler] UEventMonitor thread fail!!!\n");
			return;
		}

		ALOGD("[CNXUEventHandler] UEventMonitor thread start!!!\n");

#if 0
		ret = libusbapi_init(libusbapi_hotplug_cb);
		if( ret < 0)
		{
			ALOGD("[CNXUEventHandler] \e[31musb_init fail!!! : %d \e[0m\n", ret);
		}
		else
			ALOGD("[CNXUEventHandler] libusbapi_init ok!!! : %d \n", ret);
#endif


#if 0
		if( 0 != pthread_create( &m_hiPodPairThread, NULL, iPodPairMonitorThreadStub, this ) )
		{
			//	ToDo Print Log
			ALOGD("[CNXUEventHandler] iPodPairMonitorThreadStub fail!!!\n");
			return;
		}

		ALOGD("[CNXUEventHandler] iPodPairMonitor thread start!!!\n");
#endif


	}
}

CNXUEventHandler::~CNXUEventHandler()
{
}

void *CNXUEventHandler::UEventMonitorThreadStub(void *arg)
{
	CNXUEventHandler *pObj = (CNXUEventHandler*)arg;
	pObj->UEventMonitorThread();
	return (void*)0xDeadFace;
}

void CNXUEventHandler::UEventMonitorThread()
{
	struct pollfd fds;
	ALOGD("[CNXUEventHandler] UEventMonitorThread() start\n");
	uevent_init();
	fds.fd		= uevent_get_fd();
	fds.events	= POLLIN;
	while( 1 )
	{
		int32_t err = poll( &fds, 1, 1000 );
		if( err > 0 )
		{ 
			if( fds.revents & POLLIN )
			{
				int32_t len = uevent_next_event(m_Desc, sizeof(m_Desc)-1);

				if( len < 1)	continue;

				//ALOGD("[CNXUEventHandler] m_Desc : %s\n", m_Desc);

				 if( !strncmp(m_Desc, "add@", strlen("add@")) )
				 {
					struct stat fst;
					char idVendor[8];
					//char idProduct[8];
					int int_idVendor;
					int int_idProduct;
					int ret;
					char	*string;
					char	path[4096];


					string = strchr(m_Desc, '@');
					string++;

					memset(path, 0, 4096);
					strcat(path, "/sys");
					strcat(path, string);
					ALOGD("[CNXUEventHandler] add@ : %s\n", path);

					if( !strncmp(path, IPOD_CHANGE_IAP_DEVICE, strlen(IPOD_CHANGE_IAP_DEVICE)) )
					{
						ipod_mode = IPOD_MODE_IAP1;
						continue;
					}

					strcat(path, "/idVendor");


					if (stat(path, &fst) != 0)	continue;

					ret = Read_String((char*)path, (char*)idVendor, 5);
					if(ret < 0) 	continue;

					//memset(path, 0, 4096);
					//strcat(path, "/sys");
					//strcat(path, string);
					//strcat(path, "/idProduct");
					//ret = Read_String((char*)path, (char*)idProduct, 5);
					//if(ret < 0) 	continue;

					int_idVendor = strtol(idVendor, NULL, 16);
					//int_idProduct = strtol(idProduct, NULL, 16);

					if(int_idVendor == VID_APPLE)
					{
						isIPOD = 1;
						ipod_mode = IPOD_MODE_DEFAULT;
						memcpy(m_path, string, strlen(string));
						ALOGD("[CNXUEventHandler] Insert iPod.\n");
					}
					ALOGD("[CNXUEventHandler] int_idVendor:0x%x, isIPOD:%d \n", int_idVendor, isIPOD);
					//ALOGD("[CNXUEventHandler] int_idVendor:0x%x, int_idProduct:0x%x, isIPOD:%d \n", int_idVendor, int_idProduct, isIPOD);
				 }

				 if( !strncmp(m_Desc, "remove@", strlen("remove@")) )
				 {
					char	*string;

					string = strchr(m_Desc, '@');
					string++;

					ALOGD("[CNXUEventHandler] remove@ : %s\n", string);

					 if( !strcmp(m_path, string) )
				 	{
						if(isIPOD)
						{
							isIPOD = 0;
							ALOGD("[CNXUEventHandler] Remove iPod.\n");

							if(ipod_mode == IPOD_MODE_TETHERING)
							{
								ALOGD( "[CNXUEventHandler] system(\"usbmuxdd -X\"); \n");
								system("usbmuxdd -X");
							}
							ipod_mode = IPOD_MODE_NO_DEVIDE;
						}
					 }
				 }

			}
		}
	}
}


void *CNXUEventHandler::iPodPairMonitorThreadStub(void *arg)
{
	CNXUEventHandler *pObj = (CNXUEventHandler*)arg;
	pObj->iPodPairMonitorThread();
	return (void*)0xDeadFace;
}

void CNXUEventHandler::iPodPairMonitorThread()
{
	struct pollfd fds;
	ALOGD("[CNXUEventHandler] iPodPairMonitorThread() start\n");
	fds.fd		= open(IPOD_PAIR_PATH, O_RDONLY | O_NONBLOCK);
	fds.events	= POLLIN;

	while( 1 )
	{
		int32_t err = poll( &fds, 1, 1000000);
		if( err > 0 )
		{ 
			if( fds.revents & POLLIN )
			{
				char PairString[20];
				int ret;

				ret = Read_String((char*)IPOD_PAIR_PATH, (char*)PairString, 10);
				ALOGD("[CNXUEventHandler] PairString : %s\n", PairString);

				memset(PairString, 0, sizeof(char)*20);

				ret = Read_String((char*)IPOD_INSERT_DEVICE_PATH, (char*)PairString, 10);
				ALOGD("[CNXUEventHandler] PairString : %s\n", PairString);

			}
		}
	}
}

int CNXUEventHandler::get_ipod_mode()
{
	return ipod_mode;
}

void CNXUEventHandler::set_ipod_mode(int mode)
{
	ipod_mode = mode;
}

int CNXUEventHandler::Get_isIPOD()
{
	return isIPOD;
}

int CNXUEventHandler::Write_String(char * path, char *buffer, int len)
{
	FILE *fd;
	int ret = 0;

	fd = fopen(( char * )path, "wb");
	if (fd) {
		fwrite(buffer, sizeof(char), len, fd);
		fclose(fd);
	}
	else
	{
		ALOGD("[CNXUEventHandler] Write_String() %s: Open Fail!! \n", path);
		ret = -1;
	}

	return ret;
}

int CNXUEventHandler::Read_String(char * path, char *buffer, int len)
{
	FILE *fd = NULL;
	int ret = 0;

	if ( ( fd = fopen( ( char * ) path, "r" ) ) != NULL ) 
	{
		fgets( ( char * ) buffer, len, fd );
		fclose( fd );
	}
	else
	{
		ALOGD("[CNXUEventHandler] Read_String() %s: Open Fail!! \n", path);
		ret = -1;
	}

	return ret;
}

