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
//	Module		: Android iPod Device Manager
//	File		: CNXIPodManagerService.h
//	Description	: 
//	Author		: Seong-O Park(ray@nexell.co.kr)
//	Export		:
//	History		:
//
//------------------------------------------------------------------------------

#ifndef __CNXIPODDEVICEMANAGERSERVICE_H__
#define __CNXIPODDEVICEMANAGERSERVICE_H__

#include <stdint.h>

#include <binder/IInterface.h>
#include <binder/IBinder.h>
#include <binder/ProcessState.h>
#include <binder/IServiceManager.h>
#include <binder/IPCThreadState.h>


using namespace android;

#define	SERVICE_NAME	"IIPodDevMgr"

//
//  IPod Control Interface (Share Client & Server)
//
class IIPodDevMgr : public IInterface {
public:
	//
	//  Player Control Commands
	//
	enum {
		CHANGE_MODE = IBinder::FIRST_CALL_TRANSACTION,
		GET_MODE,
	};

	virtual int32_t ChangeMode( int32_t mode ) = 0;
	virtual int32_t GetCurrentMode()           = 0;

	DECLARE_META_INTERFACE(IPodDevMgr);  // Expands to 5 lines below:
	//static const android::String16 descriptor;
	//static android::sp<IIPodDevMgr> asInterface(const android::sp<android::IBinder>& obj);
	//virtual const android::String16& getInterfaceDescriptor() const;
	//IIPodDevMgr();
	//virtual ~IIPodDevMgr();
};

//	For Client
sp<IIPodDevMgr> getIPodDeviceManagerService();

//	For Server
int32_t StartIPodDeviceManagerService(void);


 #endif //  __CNXIPODDEVICEMANAGERSERVICE_H__