#include <stdio.h>
#include <stdlib.h>

#include <CNXIPodManagerService.h>
#include <NXIPodDeviceManager.h>


int main( int argc, char *argv[] )
{
	int32_t num = 1;
	int32_t mode = 1;
	int32_t ret = 0;
		
	if( argc > 1 )
		num = atoi(argv[1]);

	if( argc > 2 )
		mode = atoi(argv[2]);

	switch(num)
	{
		case 1:
			if(mode == IPOD_MODE_IAP1)
				ALOGD("Client : Call : NX_IPodChangeMode( IPOD_MODE_IAP1 ), Ret = %d\n", NX_IPodChangeMode( IPOD_MODE_IAP1 ));
			else if(mode == IPOD_MODE_IAP2)
				ALOGD("Client : Call : NX_IPodChangeMode( IPOD_MODE_IAP2 ), Ret = %d\n", NX_IPodChangeMode( IPOD_MODE_IAP2 ));
			else if(mode == IPOD_MODE_TETHERING)
				ALOGD("Client : Call : NX_IPodChangeMode( IPOD_MODE_TETHERING ), Ret = %d\n", NX_IPodChangeMode( IPOD_MODE_TETHERING ));
			else 
				ALOGD("Client : Not Call : NX_IPodChangeMode(). \n");
				
			break;

		case 2:
			ret = NX_IPodGetCurrentMode();

			if(ret == IPOD_MODE_DEFAULT)
				ALOGD("Client : Call : NX_IPodGetCurrentMode() : IPOD_MODE_DEFAULT Mode. \n");
			else if(ret == IPOD_MODE_IAP1)
				ALOGD("Client : Call : NX_IPodGetCurrentMode() : IPOD_MODE_IAP1 Mode. \n");
			else if(ret == IPOD_MODE_IAP2)
				ALOGD("Client : Call : NX_IPodGetCurrentMode() : IPOD_MODE_IAP2 Mode. \n");
			else if(ret == IPOD_MODE_TETHERING)
				ALOGD("Client : Call : NX_IPodGetCurrentMode() : IPOD_MODE_TETHERING Mode. \n");
			else if(ret == IPOD_MODE_CHANGING)
				ALOGD("Client : Call : NX_IPodGetCurrentMode() : IPOD_MODE_CHANGING Mode. \n");
			else if(ret == IPOD_MODE_NO_DEVIDE)
				ALOGD("Client : Call : NX_IPodGetCurrentMode() : IPOD_MODE_NO_DEVIDE Mode. \n");
			else
				ALOGD("Client : Call : NX_IPodGetCurrentMode() : unkown Mode. \n");
			break;
	}
	return 0;
}
