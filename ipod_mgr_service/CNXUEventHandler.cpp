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

#include "CNXUEventHandler.h"


#define VID_APPLE 0x5ac

#define IPOD_INSERT_UEVENT_STRING	"add@/devices/platform/nxp-ehci/usb2/2-1"
#define IPOD_REMOVE_UEVENT_STRING	"remove@/devices/platform/nxp-ehci/usb2/2-1"

#define USB_IDVENDOR_PATH	"/sys/devices/platform/nxp-ehci/usb2/2-1/idVendor"
#define USB_IDPRODUCT_PATH	"/sys/devices/platform/nxp-ehci/usb2/2-1/idProduct"


static CNXUEventHandler *gstpEventHandler = NULL;

int CNXUEventHandler::Read_ID(char * path, char *buffer, int len)
{
	FILE *fd = NULL;
	int ret = 0;

	if ( ( fd = fopen( ( char * ) path, "r" ) ) != NULL ) 
	{
		fgets( ( char * ) buffer, 5, fd );
		fclose( fd );
		//printf("[CNXUEventHandler] %s: %s \n", path, buffer);
	}
	else
	{
		printf("[CNXUEventHandler] %s: Open Fail!! \n", path);
		ret = -1;
	}

	return ret;
}


int CNXUEventHandler::Get_isIPOD()
{
	return isIPOD;
}

CNXUEventHandler::CNXUEventHandler()
	: isIPOD(0)
{
	struct stat fst;

	if( gstpEventHandler == NULL )
	{
		printf("[CNXUEventHandler] thread start!!!\n");
		if( 0 != pthread_create( &m_hUEventThread, NULL, UEventMonitorThreadStub, this ) )
		{
			//	ToDo Print Log
			printf("[CNXUEventHandler] thread fail!!!\n");
			return;
		}

		if (stat(USB_IDVENDOR_PATH, &fst) == 0) 
		{
			char idVendor[8];
			char idProduct[8];
			int int_idVendor;
			int int_idProduct;
			int ret;

			ret = Read_ID((char*)USB_IDVENDOR_PATH, (char*)idVendor, 0);
			if(ret < 0) 	return;
			ret = Read_ID((char*)USB_IDPRODUCT_PATH, (char*)idProduct, 0);
			if(ret < 0) 	return;

			int_idVendor = strtol(idVendor, NULL, 16);
			int_idProduct = strtol(idProduct, NULL, 16);

			if(int_idVendor == VID_APPLE)
				isIPOD = 1;

			printf("[CNXUEventHandler] int_idVendor:0x%x, int_idProduct:0x%x, isIPOD:%d \n", int_idVendor, int_idProduct, isIPOD);
		}

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

				 if( !strcmp(m_Desc, IPOD_INSERT_UEVENT_STRING) )
				 {
					char idVendor[8];
					char idProduct[8];
					int int_idVendor;
					int int_idProduct;
					int ret;

					ret = Read_ID((char*)USB_IDVENDOR_PATH, (char*)idVendor, 0);
					if(ret < 0) 	continue;
					ret = Read_ID((char*)USB_IDPRODUCT_PATH, (char*)idProduct, 0);
					if(ret < 0) 	continue;

					int_idVendor = strtol(idVendor, NULL, 16);
					int_idProduct = strtol(idProduct, NULL, 16);

					if(int_idVendor == VID_APPLE)
						isIPOD = 1;

					printf("[CNXUEventHandler] Insert iPod.\n");
					printf("[CNXUEventHandler] int_idVendor:0x%x, int_idProduct:0x%x, isIPOD:%d \n", int_idVendor, int_idProduct, isIPOD);

				 }
				 if( !strcmp(m_Desc, IPOD_REMOVE_UEVENT_STRING) )
				 {
					if(isIPOD)
					{
						isIPOD = 0;
						printf("[CNXUEventHandler] Remove iPod.\n");
						system("usbmuxdd -X");
					}
				 }

				printf("[CNXUEventHandler] Descriptor: isIPOD:%d  %s\n", isIPOD, m_Desc);
			}
		}
	}
}




void NX_StartUEventHandler()
{
	if( gstpEventHandler == NULL )
	{
		gstpEventHandler = new CNXUEventHandler();
	}
}
