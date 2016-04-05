#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <linux/fs.h>

#include <CNXIPodManagerService.h>
#include <NXIPodDeviceManager.h>


static void printf_ipod_usb_descripter(struct ios_libusb_device_descriptor *device_descriptor)
{
	int i, j, y;
	struct ios_libusb_config_descriptor *config_descriptor= device_descriptor->config_descriptor;
	struct ios_libusb_interface *ios_interface;
	struct ios_libusb_interface_descriptor *ios_altsetting;

	ALOGD("\e[31mprintf_usb_descripter() \e[0m\n");
	ALOGD("Device Descriptor:\n"
	       "  bDeviceClass        %5u\n"
	       "  bDeviceSubClass     %5u\n"
	       "  bDeviceProtocol     %5u\n"
	       "  idVendor           0x%04x\n"
	       "  idProduct          0x%04x\n"
	       "  bNumConfigurations  %5u\n",
	       device_descriptor->bDeviceClass,
	       device_descriptor->bDeviceSubClass,
	       device_descriptor->bDeviceProtocol,
	       device_descriptor->idVendor,
	       device_descriptor->idProduct,
	       device_descriptor->bNumConfigurations);

	for (i=0; i <  device_descriptor->bNumConfigurations; i++) {
		config_descriptor =&device_descriptor->config_descriptor[i];
		ALOGD("  Configuration Descriptor:\n"
		       "    bNumInterfaces      %5u\n"
		       "    bConfigurationValue %5u\n"
		       "    iConfiguration      %5u\n",
		       config_descriptor->bNumInterfaces,
		       config_descriptor->bConfigurationValue,
		       config_descriptor->iConfiguration);

		for (j=0; j < config_descriptor->bNumInterfaces; j++) {
			ios_interface = &config_descriptor->interface[j];

			for (y=0; y < ios_interface->num_altsetting; y++) {
				ios_altsetting = &ios_interface->altsetting[y];

				ALOGD("    Interface Descriptor:\n"
				       "      bInterfaceNumber    %5u\n"
				       "      bInterfaceClass     %5u\n"
				       "      bInterfaceSubClass  %5u\n"
				       "      iInterface          %5u\n",
				       ios_altsetting->bInterfaceNumber,
				       ios_altsetting->bInterfaceClass,
				       ios_altsetting->bInterfaceSubClass,
				       ios_altsetting->iInterface);
			}
		}
	}

	ALOGD("  Current_config    %5u\n", device_descriptor->current_config);
}

int main( int argc, char *argv[] )
{
	struct ios_libusb_device_descriptor  ipod_usb_descriptor;
	FILE *fd;

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
			ALOGD("Client : Call : NX_IPodChangeMode(), Ret = %d\n", NX_IPodChangeMode( mode ));
			break;

		case 2:
			ret = NX_IPodGetCurrentMode();

			fd = fopen(( char * )IPOD_USB_DES_PATH, "r");
			if  (fd) {
				fread((void *)&ipod_usb_descriptor, sizeof(struct ios_libusb_device_descriptor), 1, fd);
				fclose(fd);
				printf_ipod_usb_descripter(&ipod_usb_descriptor);
			} else  {
				ALOGD("%s() open fail!!! \n", IPOD_USB_DES_PATH);
			}

			ALOGD("Client : Call : NX_IPodGetCurrentMode(), ret = %d\n", ret);

			break;
	}
	return 0;
}
