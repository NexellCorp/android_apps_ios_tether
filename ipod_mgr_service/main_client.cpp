#include <stdio.h>
#include <stdlib.h>

#include "NXIPodDeviceManager.h"

int main( int argc, char *argv[] )
{
	int32_t num = 1;
	int32_t mode = 1;
	if( argc > 1 )
		num = atoi(argv[1]);

	if( argc > 2 )
		mode = atoi(argv[2]);

	switch(num)
	{
		case 1:
			printf("Client : Call : NX_IPodChangeMode( 10 ), Ret = %d\n", NX_IPodChangeMode( mode ));
			break;
		case 2:
			printf("Client : Call : NX_IPodGetCurrentMode(), Ret = %d\n", NX_IPodGetCurrentMode());
			break;
	}
	return 0;
}
