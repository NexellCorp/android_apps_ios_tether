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
//	File		: NXIPodDeviceManager.cpp
//	Description	: 
//	Author		: Seong-O Park(ray@nexell.co.kr)
//	Export		:
//	History		:
//
//------------------------------------------------------------------------------

#include <stdint.h>

#include "CNXIPodManagerService.h"
#include "NXIPodDeviceManager.h"

int32_t	NX_IPodChangeMode( int32_t mode )
{
	sp<IIPodDevMgr> iPodDevMgr = getIPodDeviceManagerService();
	if( iPodDevMgr != 0)
	{
		return iPodDevMgr->ChangeMode( mode );
	}
	return ERR_BINDER_CON;
}


int32_t NX_IPodGetCurrentMode()
{
	sp<IIPodDevMgr> iPodDevMgr = getIPodDeviceManagerService();
	if( iPodDevMgr != 0)
	{
		return iPodDevMgr->GetCurrentMode();
	}
	return ERR_BINDER_CON;
}
