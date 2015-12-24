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
	ERR_PROPERTY_ACC	= -2,		//	Property Access Error
	ERR_BINDER_CON		= -1,		//	Binder Connection Error

	//	iPod Device Mode
	IPOD_MODE_DEFAULT	= 1,		//	Default iAP1 Mode
	IPOD_MODE_IAP1 		= 1,
	IPOD_MODE_TETHERING,
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
int32_t NX_IPodGetCurrentMode();


#ifdef __cplusplus
}
#endif

#endif	//	__NXIPODDEVICEMANAGER_H__