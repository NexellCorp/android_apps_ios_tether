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

#include <cutils/properties.h>

#include <CNXIPodManagerService.h>
#include <CNXUEventHandler.h>
#include <NXIPodDeviceManager.h>

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
	ALOGD("Client : ChangeMode : mode = %d\n", mode);
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
	ALOGD("Client : GetCurrentMode : res = %d\n", res);

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
	int32_t mode = 0;

	ALOGD("BnIPodCtrl::onTransact(%i) %i", code, flags);
	data.checkInterface(this);

	switch (code) {
	case CHANGE_MODE:
		data.readInt32(&mode);
		reply->writeInt32(ChangeMode(mode));
		return NO_ERROR;

	case GET_MODE:
		reply->writeInt32(GetCurrentMode());
		return NO_ERROR;

	default:
		return BBinder::onTransact(code, data, reply, flags);
	}
}

class IIPodDeviceManagerService : public BnIPodCtrl {
private:
	int32_t 		m_mode;
	static void	*Mode_Change_Thread(void *arg);

public:
	virtual int32_t ChangeMode( int32_t mode );
	virtual int32_t GetCurrentMode();

	//DECLARE_META_INTERFACE(IPodDeviceManagerService);  // Expands to 5 lines below:
	//static const android::String16 descriptor;
	//static android::sp<IIPodDevMgr> asInterface(const android::sp<android::IBinder>& obj);
	//virtual const android::String16& getInterfaceDescriptor() const;
	IIPodDeviceManagerService();
	~IIPodDeviceManagerService();

};


static CNXUEventHandler *usbEventHandler = NULL;

#define	IPOD_DEV_MODE_PROPERTY_NAME		"persist.nx.ipod.device.mode"

IIPodDeviceManagerService::IIPodDeviceManagerService()
{
}

IIPodDeviceManagerService::~IIPodDeviceManagerService()
{
}

void *IIPodDeviceManagerService::Mode_Change_Thread(void *arg)
{
	IIPodDeviceManagerService *pObj = (IIPodDeviceManagerService*)arg;
	int32_t ret = 0, ret1 = 0;
	char value[PROPERTY_VALUE_MAX];

	ALOGD( "[CNXIPodDeviceManagerService] Mode_Change_Thread()  \n");

	ret1 = property_get_int32( IPOD_DEV_MODE_PROPERTY_NAME, 1 );
	if (pObj->m_mode == 10) {
		if (ret1  <= 3)
			pObj->m_mode = ret1;
		else
			pObj->m_mode = 0;
	}

	sprintf( value, "%d", pObj->m_mode );
	ALOGD( "[CNXIPodDeviceManagerService] Server : Change Mode : %d, Get_isIPOD:%d, get_property:%d \n", pObj->m_mode, usbEventHandler->Get_isIPOD(), ret1);

	 ret = property_set(IPOD_DEV_MODE_PROPERTY_NAME, value );
	ALOGD( "[CNXIPodDeviceManagerService] property_set:%s, value:%s, ret:%d \n", IPOD_DEV_MODE_PROPERTY_NAME, value, ret);
	
	if (usbEventHandler->Get_isIPOD()) {
		struct ios_libusb_device_descriptor *ios_dev_descriptor = usbEventHandler->get_ipod_descriptors();

		if (!usbEventHandler->find_InterfaceClass(ios_dev_descriptor, pObj->m_mode, 255, 253)) {
			usbEventHandler->Set_ios_tethering(pObj->m_mode);
			usbEventHandler->set_ipod_mode(pObj->m_mode);
			ios_dev_descriptor->current_config = pObj->m_mode;
		}else {
			usbEventHandler->Set_usb_config(pObj->m_mode);
			usbEventHandler->set_ipod_mode(pObj->m_mode);
			ios_dev_descriptor->current_config = pObj->m_mode;
		}
	} else {
		usbEventHandler->set_ipod_mode(IPOD_MODE_NO_DEVIDE);
	}

	return NULL;
}

int32_t IIPodDeviceManagerService::ChangeMode( int32_t mode )
{
	pthread_t		m_Mode_Change_Thread;
	struct ios_libusb_device_descriptor *ios_dev_descriptor = usbEventHandler->get_ipod_descriptors();

	if (usbEventHandler->get_ipod_mode() == IPOD_MODE_CHANGING
		||usbEventHandler->get_ipod_mode() == mode) {
		return usbEventHandler->get_ipod_mode();
	}

	usbEventHandler->set_ipod_mode(IPOD_MODE_CHANGING);

	if ( 0  != pthread_create( &m_Mode_Change_Thread, NULL, Mode_Change_Thread, this) ) {
		ALOGD("[CNXUEventHandler] Mode_Change_Thread thread fail!!!\n");
		return -3;
	} else {
		ALOGD( "[CNXIPodDeviceManagerService] ChangeMode : m_Mode_Change_Thread:%p, mode:%d \n", m_Mode_Change_Thread, mode);
		m_mode = mode;
	}

	return 0;
}

int32_t IIPodDeviceManagerService::GetCurrentMode()
{
	int32_t ret = 0;
	ret = property_get_int32( IPOD_DEV_MODE_PROPERTY_NAME, 1 );		//	iAP1 Mode Default

	usbEventHandler->set_ipod_descriptors();

	ALOGD( "[CNXIPodDeviceManagerService] Server : GetCurrentMode (ipod_mode:%d), get_property:%d \n", usbEventHandler->get_ipod_mode(), ret);
	return usbEventHandler->get_ipod_mode();
}


sp<IIPodDevMgr> getIPodDeviceManagerService()
{
	sp<IServiceManager> sm = defaultServiceManager();
	if (sm == 0 ) {
		ALOGE("Cannot get defaultServiceManager()");
		return NULL;
		abort();
	}

	sp<IBinder> binder = sm->getService(String16(SERVICE_NAME));
	if (binder == 0 ) {
		ALOGE("Cannot get %s Service", SERVICE_NAME);
		return NULL;
		abort();
	}

	sp<IIPodDevMgr> iPodCtrl = interface_cast<IIPodDevMgr>(binder);
	if (iPodCtrl == 0 ) {
		ALOGE("Cannot get iPod Control Interface");
		return NULL;
		abort();
	}

	return iPodCtrl;
}

int32_t StartIPodDeviceManagerService(void)
{
	if (usbEventHandler == NULL ) {
		int32_t ret = 0;
		ret = property_get_int32( IPOD_DEV_MODE_PROPERTY_NAME, 1 );		//	iAP1 Mode Default

		ALOGD( "[CNXIPodDeviceManagerService] new CNXUEventHandler(), get_property:%d \n", ret);
		usbEventHandler = new CNXUEventHandler();
	}

	defaultServiceManager()->addService(String16(SERVICE_NAME), new IIPodDeviceManagerService());

	ALOGD( "[CNXIPodDeviceManagerService] startThreadPool(); \n");
	android::ProcessState::self()->startThreadPool();

	ALOGD( "[CNXIPodDeviceManagerService] joinThreadPool(); \n");
	IPCThreadState::self()->joinThreadPool();

	delete usbEventHandler;

	return 0;
}
