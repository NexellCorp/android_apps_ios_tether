
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
#include "CNXIPodManagerService.h"
#include "NX_CCpuInfo.h"

extern void StartIPodDeviceManagerService();

int main( int argc, char *argv[] )
{
	ALOGD( "%s()  \n", __func__);

	uint32_t guid[4] = { 0x00000000, };
	NX_CCpuInfo *pCpuInfo = new NX_CCpuInfo();
	pCpuInfo->GetGUID( guid );
	delete pCpuInfo;

	if( guid[0] != NEXELL_CHIP_GUID0 || 
		guid[1] != NEXELL_CHIP_GUID1 ||
		guid[2] != NEXELL_CHIP_GUID2 || 
		guid[3] != NEXELL_CHIP_GUID3 ) {
			ALOGD("Not Support CPU type.\n" );
			return -1;
	}

	StartIPodDeviceManagerService();
	return 0;
}
