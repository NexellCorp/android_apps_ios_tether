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

#include "include/CNXIPodManagerService.h"
#include "include/CNXUEventHandler.h"
#include "include/NXIPodDeviceManager.h"
#include "include/NXCCpuInfo.h"

//--------------------------------------------------------------------------------
// NXP4330 / S5P4418 / NXP5430 / S5P6818 GUID
// 069f41d2:4c3ae27d:68506bb8:33bc096f
#define NEXELL_CHIP_GUID0	0x069f41d2
#define NEXELL_CHIP_GUID1	0x4c3ae27d
#define NEXELL_CHIP_GUID2	0x68506bb8
#define NEXELL_CHIP_GUID3	0x33bc096f

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
	if (pObj->m_mode == 10)
	{
		if(ret1 == 1
			|| ret1 == 2
			|| ret1 == 3)
		{
			pObj->m_mode = ret1;
		}
		else
		{
			pObj->m_mode = 1;
		}
	}

	sprintf( value, "%d", pObj->m_mode );
	ALOGD( "[CNXIPodDeviceManagerService] Server : Change Mode : %d, Get_isIPOD:%d, get_property:%d \n", pObj->m_mode, usbEventHandler->Get_isIPOD(), ret1);

	 ret = property_set(IPOD_DEV_MODE_PROPERTY_NAME, value );
	ALOGD( "[CNXIPodDeviceManagerService] property_set:%s, value:%s, ret:%d \n", IPOD_DEV_MODE_PROPERTY_NAME, value, ret);
	
	if(usbEventHandler->Get_isIPOD())
	{
		if(pObj->m_mode == IPOD_MODE_IAP1)
		{
			//if(usbEventHandler->get_ipod_mode() == IPOD_MODE_TETHERING
			//	|| usbEventHandler->get_ipod_mode() == IPOD_MODE_TETHERING_CHANGING)
			{
				ALOGD( "[CNXIPodDeviceManagerService] system(\"usbmuxdd -X\"); \n");
				system("/system/bin/usbmuxdd -X");
				sleep(2);
			}

			ALOGD( "[CNXIPodDeviceManagerService] system(\"usbmuxdd -a\"); \n");
			system("/system/bin/usbmuxdd -a");
			usbEventHandler->set_ipod_mode(IPOD_MODE_IAP1);
		}

		if(pObj->m_mode == IPOD_MODE_IAP2)
		{
			//if(usbEventHandler->get_ipod_mode() == IPOD_MODE_TETHERING
			//	|| usbEventHandler->get_ipod_mode() == IPOD_MODE_TETHERING_CHANGING)
			{
				ALOGD( "[CNXIPodDeviceManagerService] system(\"usbmuxdd -X\"); \n");
				system("/system/bin/usbmuxdd -X");
				sleep(2);
			}
			usbEventHandler->set_ipod_mode(IPOD_MODE_IAP2);
		}

		if(pObj->m_mode == IPOD_MODE_TETHERING)
		{
			static int cnt = 0;
			char PairString[20];
			int ret = 0;

			cnt = 0;
			ALOGD( "[CNXIPodDeviceManagerService] system(\"usbmuxdd -v\"); \n");
			system("/system/bin/usbmuxdd -v");

			while(1)
			{
				ret = usbEventHandler->Read_String((char *)IPOD_PAIR_PATH, (char *)PairString, 20);
				ALOGD( "[CNXIPodDeviceManagerService] %s : %s \n", IPOD_PAIR_PATH, PairString);

				 if( !strncmp(PairString, "pair", sizeof((char *)"pair")) )
				 {
					system("echo 0 >/sys/class/iOS/ipheth/regnetdev");
					usbEventHandler->set_ipod_mode(IPOD_MODE_TETHERING);
					break;
				 }
				sleep(1);

				if(ret < 0 
					|| usbEventHandler->get_ipod_mode() == IPOD_MODE_IAP1
					|| usbEventHandler->get_ipod_mode() == IPOD_MODE_IAP2
					|| usbEventHandler->get_ipod_mode() == IPOD_MODE_NO_DEVIDE 
					|| usbEventHandler->Get_isIPOD() == 0)
				{
					break;
				}

				if(cnt ==2 )
				{
					usbEventHandler->set_ipod_mode(IPOD_MODE_TETHERING);
				}
				if(cnt < 5);	cnt ++ ;

			}
		}
	}
	else
	{
		usbEventHandler->set_ipod_mode(IPOD_MODE_NO_DEVIDE);
	}

	return NULL;
}

int32_t IIPodDeviceManagerService::ChangeMode( int32_t mode )
{
	pthread_t		m_Mode_Change_Thread;

	if(usbEventHandler->get_ipod_mode() == IPOD_MODE_CHANGING)
	{
		return usbEventHandler->get_ipod_mode();
	}

	usbEventHandler->set_ipod_mode(IPOD_MODE_CHANGING);

	if( 0 != pthread_create( &m_Mode_Change_Thread, NULL, Mode_Change_Thread, this) )
	{
		ALOGD("[CNXUEventHandler] Mode_Change_Thread thread fail!!!\n");
		return -3;
	}
	else
	{
		ALOGD( "[CNXIPodDeviceManagerService] ChangeMode : m_Mode_Change_Thread:%p \n", m_Mode_Change_Thread);
		m_mode = mode;
		return 0;
	}
}

int32_t IIPodDeviceManagerService::GetCurrentMode()
{
	int32_t ret = 0;
	ret = property_get_int32( IPOD_DEV_MODE_PROPERTY_NAME, 1 );		//	iAP1 Mode Default

	ALOGD( "[CNXIPodDeviceManagerService] Server : GetCurrentMode (ipod_mode:%d), get_property:%d \n", usbEventHandler->get_ipod_mode(), ret);
	return usbEventHandler->get_ipod_mode();
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

int32_t StartIPodDeviceManagerService(void)
{
	uint32_t guid[4] = { 0x00000000, };
	NX_CCpuInfo *pCpuInfo = new NX_CCpuInfo();
	pCpuInfo->GetGUID( guid );
	delete pCpuInfo;

	if( guid[0] != NEXELL_CHIP_GUID0 || 
		guid[1] != NEXELL_CHIP_GUID1 ||
		guid[2] != NEXELL_CHIP_GUID2 || 
		guid[3] != NEXELL_CHIP_GUID3 ) {
			ALOGD("[CNXIPodDeviceManagerService] Not Support CPU type.\n" );
			return -1;
	}

	if( usbEventHandler == NULL )
	{
		int32_t ret = 0;
		ret = property_get_int32( IPOD_DEV_MODE_PROPERTY_NAME, 1 );		//	iAP1 Mode Default

		ALOGD( "[CNXIPodDeviceManagerService] new CNXUEventHandler(), get_property:%d \n", ret);

		usbEventHandler = new CNXUEventHandler();

		usbEventHandler->Write_String((char *)IPOD_INSERT_DEVICE_PATH, (char *)"remove", sizeof(char)*6);
		usbEventHandler->Write_String((char *)IPOD_PAIR_PATH, (char *)"unpair", sizeof(char)*6);
	}

	defaultServiceManager()->addService(String16(SERVICE_NAME), new IIPodDeviceManagerService());

	ALOGD( "[CNXIPodDeviceManagerService] startThreadPool(); \n");
	android::ProcessState::self()->startThreadPool();

	ALOGD( "[CNXIPodDeviceManagerService] joinThreadPool(); \n");
	IPCThreadState::self()->joinThreadPool();


	delete usbEventHandler;

	return 0;
}
