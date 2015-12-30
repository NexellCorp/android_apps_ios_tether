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
//	File		: CNXIPodManagerService.cpp
//	Description	: 
//	Author		: Seong-O Park(ray@nexell.co.kr)
//	Export		:
//	History		:
//
//------------------------------------------------------------------------------


#include "CNXIPodManagerService.h"


using namespace android;

//	Client
class BpIPodCtrl : public BpInterface<IIPodDevMgr> {
public:
	BpIPodCtrl(const sp<IBinder>& impl) : BpInterface<IIPodDevMgr>(impl) {
		ALOGD("BpIPodCtrl::BpIPodCtrl()");
	}
	int32_t ChangeMode( int32_t mode );
	int32_t GetCurrentMode();
};

int32_t BpIPodCtrl::ChangeMode( int32_t mode )
{
	Parcel data, reply;
	int32_t res;
	data.writeInterfaceToken(IIPodDevMgr::getInterfaceDescriptor());
	data.writeInt32(mode);
	ALOGD("Client : mode = %d\n", mode);
	remote()->transact(CHANGE_MODE, data, &reply);
	status_t status = reply.readInt32(&res);
	return res;
}
int32_t BpIPodCtrl::GetCurrentMode()
{
	Parcel data, reply;
	int32_t res;
	remote()->transact(GET_MODE, data, &reply);
	status_t status = reply.readInt32(&res);
	return res;
}

//
//	IMPLEMENT_META_INTERFACE
//
const android::String16 IIPodDevMgr::descriptor(SERVICE_NAME);
const android::String16& IIPodDevMgr::getInterfaceDescriptor() const {
	return IIPodDevMgr::descriptor;
}

android::sp<IIPodDevMgr> IIPodDevMgr::asInterface(const android::sp<android::IBinder>& obj) 
{
	android::sp<IIPodDevMgr> intr;
	if (obj != NULL) {
		intr = static_cast<IIPodDevMgr*>(obj->queryLocalInterface(IIPodDevMgr::descriptor).get());
		if (intr == NULL) {
			intr = new BpIPodCtrl(obj);
		}
	}
	return intr;
}

IIPodDevMgr::IIPodDevMgr() { ALOGD("IIPodDevMgr::IIPodDevMgr()"); }

IIPodDevMgr::~IIPodDevMgr() { ALOGD("IIPodDevMgr::~IIPodDevMgr()"); }


//
//		Server
//
class BnIPodCtrl : public BnInterface<IIPodDevMgr> {
	virtual status_t onTransact(uint32_t code, const Parcel& data, Parcel* reply, uint32_t flags = 0);
};

status_t BnIPodCtrl::onTransact(uint32_t code, const Parcel& data, Parcel* reply, uint32_t flags)
{
	ALOGD("BnIPodCtrl::onTransact(%i) %i", code, flags);
	data.checkInterface(this);
	switch(code)
	{
		case CHANGE_MODE:
		{
			int32_t mode = 0;
			data.readInt32(&mode);
			reply->writeInt32(ChangeMode(mode));
			return NO_ERROR;
		}
		case GET_MODE:
			reply->writeInt32(GetCurrentMode());
			return NO_ERROR;
		default:
			return BBinder::onTransact(code, data, reply, flags);
	}
}


class IIPodDeviceManagerService : public BnIPodCtrl {
	virtual int32_t ChangeMode( int32_t mode );
	virtual int32_t GetCurrentMode();
};

#include <cutils/properties.h>

#include "CNXUEventHandler.h"
static CNXUEventHandler *usbEventHandler = NULL;

#define	IPOD_DEV_MODE_PROPERTY_NAME		"persist.nx.ipod.device.mode"
int32_t IIPodDeviceManagerService::ChangeMode( int32_t mode )
{
	char value[PROPERTY_VALUE_MAX];
	sprintf( value, "%d", mode );
	int32_t ret = property_set(IPOD_DEV_MODE_PROPERTY_NAME, value );
	ALOGD( "[IIPodDeviceManagerService] Server : Change Mode : %d, ret(%d), Get_isIPOD:%d\n", mode, ret, usbEventHandler->Get_isIPOD() );

	if(usbEventHandler->Get_isIPOD())
	{
		if(mode == 0)
		{
			ALOGD( "[IIPodDeviceManagerService] system(\"usbmuxdd -X\"); \n");
			system("/system/bin/usbmuxdd -X");
			sleep(2);
			ALOGD( "[IIPodDeviceManagerService] system(\"usbAudio\"); \n");
			system("/system/bin/usbAudio 2");
		}

		if(mode == 1)
		{
			ALOGD( "[IIPodDeviceManagerService] system(\"usbmuxdd -v\"); \n");
			system("/system/bin/usbmuxdd -v");
		}
	}

	if( ret != 0 )
		return -2;
	return ret;
}

int32_t IIPodDeviceManagerService::GetCurrentMode()
{
	int32_t ret;
	ret = property_get_int32( IPOD_DEV_MODE_PROPERTY_NAME, 1 );		//	iAP1 Mode Default
	ALOGD( "[IIPodDeviceManagerService] Server : GetCurrentMode (ret = %d)\n", ret );

#if 0// PJSIN 20151223 add-- [ 1 
	if(usbEventHandler->Get_isIPOD())
	{
		if(ret == 0)
			system("usbAudio");
		if(ret == 1)
			system("usbmuxdd -v");
	}
#endif// ]-- end 

	return ret;
}


sp<IIPodDevMgr> getIPodDeviceManagerService()
{
	sp<IServiceManager> sm = defaultServiceManager();
	if( sm == 0 ){
		ALOGE("Cannot get defaultServiceManager()");
		return NULL;
		abort();
	}
	sp<IBinder> binder = sm->getService(String16(SERVICE_NAME));
	if( binder == 0 ){
		ALOGE("Cannot get %s Service", SERVICE_NAME);
		return NULL;
		abort();
	}
	sp<IIPodDevMgr> iPodCtrl = interface_cast<IIPodDevMgr>(binder);
	if( iPodCtrl == 0 ){
		ALOGE("Cannot get iPod Control Interface");
		return NULL;
		abort();
	}
	return iPodCtrl;
}

void StartIPodDeviceManagerService()
{

	if( usbEventHandler == NULL )
	{
		usbEventHandler = new CNXUEventHandler();
	}

	defaultServiceManager()->addService(String16(SERVICE_NAME), new IIPodDeviceManagerService());
	android::ProcessState::self()->startThreadPool();
	ALOGD("%s service is now ready", SERVICE_NAME);
	IPCThreadState::self()->joinThreadPool();
	ALOGD("%s service thread joined", SERVICE_NAME);

	delete usbEventHandler;
}
