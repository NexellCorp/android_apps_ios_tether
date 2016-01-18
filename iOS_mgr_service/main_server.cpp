
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
#include <CNXIPodManagerService.h>


int main( int argc, char *argv[] )
{
	int32_t ret;

	ALOGD( "%s() Ver.1.00  \n", argv[0]);

	ret = StartIPodDeviceManagerService();

	if(ret < 0)
		ALOGD("Not Support CPU type.\n" );
	
	return 0;
}
