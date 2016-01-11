
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

extern void NX_StartUEventHandler();
extern void StartIPodDeviceManagerService();

int main( int argc, char *argv[] )
{
	// NX_StartUEventHandler();
	ALOGD( "%s()  \n", __func__);

	StartIPodDeviceManagerService();
	return 0;
}
