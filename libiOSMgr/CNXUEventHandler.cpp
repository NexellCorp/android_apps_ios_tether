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
//	Module		: Android iPod Device Manager API
//	File		: CNXUEventHandler.cpp
//	Description	: 
//	Author		: Seong-O Park(ray@nexell.co.kr)
//	Export		:
//	History		:
//
//------------------------------------------------------------------------------
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

#include <libusb.h>

#include <NXIPodDeviceManager.h>
#include <CNXUEventHandler.h>

#include <NXCCpuInfo.h>

//--------------------------------------------------------------------------------
// NXP4330 / S5P4418 / NXP5430 / S5P6818 GUID
// 069f41d2:4c3ae27d:68506bb8:33bc096f
#define NEXELL_CHIP_GUID0	0x069f41d2
#define NEXELL_CHIP_GUID1	0x4c3ae27d
#define NEXELL_CHIP_GUID2	0x68506bb8
#define NEXELL_CHIP_GUID3	0x33bc096f

#define VID_APPLE 0x5ac
#define PID_APPLE 0x1200

#define IPOD_INSERT_DEVICE_PATH "/var/lib/ipod"
#define IPOD_PAIR_PATH "/var/lib/pair"

#ifndef USB_CLASS_CCID
#define USB_CLASS_CCID			0x0b
#endif

#ifndef USB_CLASS_VIDEO
#define USB_CLASS_VIDEO			0x0e
#endif

#ifndef USB_CLASS_APPLICATION
#define USB_CLASS_APPLICATION	       	0xfe
#endif

#ifndef USB_AUDIO_CLASS_1
#define USB_AUDIO_CLASS_1		0x00
#endif

#ifndef USB_AUDIO_CLASS_2
#define USB_AUDIO_CLASS_2		0x20
#endif

#ifndef USB_DT_OTG
#define USB_DT_OTG			0x09
#endif

static CNXUEventHandler *gstpEventHandler = NULL;

static unsigned int verblevel = 1;


#if defined(HAVE_NL_LANGINFO) && defined(HAVE_ICONV)
static u_int16_t get_any_langid(libusb_device_handle *dev)
{
	unsigned char buf[4];
	int ret = libusb_get_string_descriptor(dev, 0, 0, buf, sizeof buf);
	if (ret != sizeof buf) return 0;
	return buf[2] | (buf[3] << 8);
}

static char *usb_string_to_native(char * str, size_t len)
{
	size_t num_converted;
	iconv_t conv;
	char *result, *result_end;
	size_t in_bytes_left, out_bytes_left;

	conv = iconv_open(nl_langinfo(CODESET), "UTF-16LE");

	if (conv == (iconv_t) -1)
		return NULL;

	in_bytes_left = len * 2;
	out_bytes_left = len * MB_CUR_MAX;
	result = result_end = malloc(out_bytes_left + 1);

	num_converted = iconv(conv, &str, &in_bytes_left,
	                      &result_end, &out_bytes_left);

	iconv_close(conv);
	if (num_converted == (size_t) -1) {
		free(result);
		return NULL;
	}

	*result_end = 0;
	return result;
}
#endif

static char *get_dev_string_ascii(libusb_device_handle *dev, size_t size,
                                  u_int8_t id)
{
	char *buf = (char*)malloc(size);
	int ret = libusb_get_string_descriptor_ascii(dev, id,
	                                             (unsigned char *) buf,
	                                             size);

	if (ret < 0) {
		free(buf);
		return strdup("(error)");
	}

	return buf;
}

char *get_dev_string(libusb_device_handle *dev, u_int8_t id)
{
#if defined(HAVE_NL_LANGINFO) && defined(HAVE_ICONV)
	int ret;
	char *buf, unicode_buf[254];
	u_int16_t langid;
#endif

	if (!dev || !id) return strdup("");

#if defined(HAVE_NL_LANGINFO) && defined(HAVE_ICONV)
	langid = get_any_langid(dev);
	if (!langid) return strdup("(error)");

	ret = libusb_get_string_descriptor(dev, id, langid,
	                                   (unsigned char *) unicode_buf,
	                                   sizeof unicode_buf);
	if (ret < 2) return strdup("(error)");

	if ((unsigned char)unicode_buf[0] < 2 || unicode_buf[1] != LIBUSB_DT_STRING)
		return strdup("(error)");

	buf = usb_string_to_native(unicode_buf + 2,
	                           ((unsigned char) unicode_buf[0] - 2) / 2);

	if (!buf) return get_dev_string_ascii(dev, 127, id);

	return buf;
#else
	return get_dev_string_ascii(dev, 127, id);
#endif
}

static const unsigned char *find_otg(const unsigned char *buf, int buflen)
{
	if (!buf)
		return 0;
	while (buflen >= 3) {
		if (buf[0] == 3 && buf[1] == USB_DT_OTG)
			return buf;
		if (buf[0] > buflen)
			return 0;
		buflen -= buf[0];
		buf += buf[0];
	}
	return 0;
}

static int do_otg(struct libusb_config_descriptor *config)
{
	unsigned	i, k;
	int		j;
	const unsigned char	*desc;

	/* each config of an otg device has an OTG descriptor */
	desc = find_otg(config->extra, config->extra_length);
	for (i = 0; !desc && i < config->bNumInterfaces; i++) {
		const struct libusb_interface *intf;

		intf = &config->interface[i];
		for (j = 0; !desc && j < intf->num_altsetting; j++) {
			const struct libusb_interface_descriptor *alt;

			alt = &intf->altsetting[j];
			desc = find_otg(alt->extra, alt->extra_length);
			for (k = 0; !desc && k < alt->bNumEndpoints; k++) {
				const struct libusb_endpoint_descriptor *ep;

				ep = &alt->endpoint[k];
				desc = find_otg(ep->extra, ep->extra_length);
			}
		}
	}
	if (!desc)
		return 0;

	ALOGD("OTG Descriptor:\n"
		"  bLength               %3u\n"
		"  bDescriptorType       %3u\n"
		"  bmAttributes         0x%02x\n"
		"%s%s",
		desc[0], desc[1], desc[2],
		(desc[2] & 0x01)
			? "    SRP (Session Request Protocol)\n" : "",
		(desc[2] & 0x02)
			? "    HNP (Host Negotiation Protocol)\n" : "");
	return 1;
}

void printf_usb_descripter(libusb_device_handle *udev, struct ios_libusb_device_descriptor *device_descriptor)
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
		char *cfg = NULL;

		config_descriptor =&device_descriptor->config_descriptor[i];

		if (udev)
			cfg = get_dev_string(udev, config_descriptor->iConfiguration);

		ALOGD("  Configuration Descriptor:\n"
		       "    bNumInterfaces      %5u\n"
		       "    bConfigurationValue %5u\n"
		       "    iConfiguration      %5u %s\n",
		       config_descriptor->bNumInterfaces,
		       config_descriptor->bConfigurationValue,
		       config_descriptor->iConfiguration, 
		       (cfg) ? cfg : "");

		if (cfg)
			free(cfg);

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

static void dump_altsetting(
	libusb_device_handle *dev,
	const struct libusb_interface_descriptor *interface,
	struct ios_libusb_interface_descriptor *ios_interface
)
{
	const unsigned char *buf;
	unsigned size, i;

	ios_interface->bInterfaceNumber = interface->bInterfaceNumber;
	ios_interface->bInterfaceClass = interface->bInterfaceClass;
	ios_interface->bInterfaceSubClass = interface->bInterfaceSubClass;
	ios_interface->bInterfaceProtocol = interface->bInterfaceProtocol;
	ios_interface->iInterface = interface->iInterface;

#if 0
	ALOGD("    Interface Descriptor:\n"
	       "      bInterfaceNumber    %5u\n"
	       "      bInterfaceClass     %5u\n"
	       "      bInterfaceSubClass  %5u\n"
	       "      iInterface          %5u\n",
	       ios_interface->bInterfaceNumber,
	       ios_interface->bInterfaceClass,
	       ios_interface->bInterfaceSubClass,
	       ios_interface->iInterface);
#endif
}

static void dump_interface(
	libusb_device_handle *dev,
	const struct libusb_interface *interface,
	struct ios_libusb_interface *ios_interface
)
{
	int i;

	ios_interface->num_altsetting = interface->num_altsetting;
	//ios_interface->size = sizeof(struct ios_libusb_interface_descriptor) * ios_interface->num_altsetting;
	//ios_interface->altsetting = (struct ios_libusb_interface_descriptor*)malloc(ios_interface->size);

	for (i = 0; i < interface->num_altsetting; i++)
		dump_altsetting(dev, &interface->altsetting[i], &ios_interface->altsetting[i]);
}


static void dump_config(
	libusb_device_handle *dev,
	struct libusb_config_descriptor *config,
	unsigned speed,
	struct ios_libusb_config_descriptor *config_descriptor
)
{
	//char *cfg;
	int i;

	config_descriptor->bNumInterfaces = config->bNumInterfaces;
	config_descriptor->bConfigurationValue = config->bConfigurationValue;
	config_descriptor->iConfiguration = config->iConfiguration;

#if 0
	cfg = get_dev_string(dev, config->iConfiguration);
	ALOGD("  Configuration Descriptor:\n"
	       "    bNumInterfaces      %5u\n"
	       "    bConfigurationValue %5u\n"
	       "    iConfiguration      %5u %s\n",
	       config_descriptor->bNumInterfaces,
	       config_descriptor->bConfigurationValue,
	       config_descriptor->iConfiguration, cfg);
	free(cfg);
#endif

	//config_descriptor->size = sizeof(struct ios_libusb_interface) * config->bNumInterfaces;
	//config_descriptor->interface = (struct ios_libusb_interface*)malloc(config_descriptor->size);

	for (i = 0 ; i < config->bNumInterfaces ; i++)
		dump_interface(dev, &config->interface[i], &config_descriptor->interface[i]);
}

static void dump_device(
	libusb_device_handle *dev,
	struct libusb_device_descriptor *descriptor,
	struct ios_libusb_device_descriptor *device_descriptor
)
{
	device_descriptor->bDeviceClass = descriptor->bDeviceClass;
	device_descriptor->bDeviceSubClass = descriptor->bDeviceSubClass;
	device_descriptor->bDeviceProtocol = descriptor->bDeviceProtocol;
	device_descriptor->idVendor = descriptor->idVendor;
	device_descriptor->idProduct = descriptor->idProduct;
	device_descriptor->bNumConfigurations = descriptor->bNumConfigurations;

#if 0
	ALOGD("Device Descriptor:\n"
	       "  bcdUSB              %2x.%02x\n"
	       "  bDeviceClass        %5u\n"
	       "  bDeviceSubClass     %5u\n"
	       "  bDeviceProtocol     %5u\n"
	       "  idVendor           0x%04x\n"
	       "  idProduct          0x%04x\n"
	       "  bcdDevice           %2x.%02x\n"
	       "  bNumConfigurations  %5u\n",
	       descriptor->bcdUSB >> 8, descriptor->bcdUSB & 0xff,
	       device_descriptor->bDeviceClass,
	       device_descriptor->bDeviceSubClass,
	       device_descriptor->bDeviceProtocol,
	       device_descriptor->idVendor,
	       device_descriptor->idProduct,
	       descriptor->bcdDevice >> 8, descriptor->bcdDevice & 0xff,
	       device_descriptor->bNumConfigurations);
#endif
}

static void dump_dev(libusb_device *dev, struct ios_libusb_device_descriptor *device_descriptor)
{
	libusb_device_handle *udev;
	struct libusb_device_descriptor desc;
	int i, ret;
	int otg, wireless;

	otg = wireless = 0;
	ret = libusb_open(dev, &udev);
	if (ret) {
		ALOGE("Couldn't open device, some information "
			"will be missing\n");
		udev = NULL;
	}

	libusb_get_configuration(udev, &device_descriptor->current_config);
	libusb_get_device_descriptor(dev, &desc);
	dump_device(udev, &desc, device_descriptor);

	if (desc.bNumConfigurations) {
		struct libusb_config_descriptor *config;

		ret = libusb_get_config_descriptor(dev, 0, &config);
		if (ret) {
			ALOGE("Couldn't get configuration descriptor 0, "
					"some information will be missing\n");
		} else {
			otg = do_otg(config) || otg;
			libusb_free_config_descriptor(config);
		}

		//device_descriptor->size = sizeof(struct ios_libusb_config_descriptor) * desc.bNumConfigurations;
		//device_descriptor->config_descriptor = (struct ios_libusb_config_descriptor*)malloc(device_descriptor->size);

		for (i = 0; i < desc.bNumConfigurations; ++i) {
			ret = libusb_get_config_descriptor(dev, i, &config);
			if (ret) {
				ALOGE("Couldn't get configuration "
						"descriptor %d, some information will "
						"be missing\n", i);
			} else {
				dump_config(udev, config, desc.bcdUSB, &device_descriptor->config_descriptor[i]);
				libusb_free_config_descriptor(config);
			}
		}
	}
	if (!udev)
		return;

	//printf_usb_descripter(udev, device_descriptor);

	libusb_close(udev);
}

static int list_devices(
	libusb_context *ctx,
	int busnum,
	int devnum,
	int vendorid,
	int productid,
	struct ios_libusb_device_descriptor *device_descriptor
)
{
	libusb_device **list;
	struct libusb_device_descriptor desc;
	char vendor[128], product[128];
	int status;
	ssize_t num_devs, i;

	status = 1; /* 1 device not found, 0 device found */

	num_devs = libusb_get_device_list(ctx, &list);

	ALOGD("%s() num_devs:%d\n", __FUNCTION__, num_devs);

	if (num_devs < 0)
		goto error;

	for (i = 0; i < num_devs; ++i) {
		libusb_device *dev = list[i];
		uint8_t bnum = libusb_get_bus_number(dev);
		uint8_t dnum = libusb_get_device_address(dev);

		if ((busnum != -1 && busnum != bnum) ||
		    (devnum != -1 && devnum != dnum))
			continue;
		libusb_get_device_descriptor(dev, &desc);
		if ((vendorid != -1 && vendorid != desc.idVendor) ||
		    (productid != -1 && productid != desc.idProduct))
			continue;
		status = 0;
		ALOGD("Bus %03u Device %03u: ID %04x:%04x \n",
				bnum, dnum,
				desc.idVendor,
				desc.idProduct);
		dump_dev(dev, device_descriptor);
	}

	libusb_free_device_list(list, 0);
error:
	return status;
}

static int ipod_create_descriptors(
	int vendor,
	int product,
	struct ios_libusb_device_descriptor *device_descriptor
)
{
	int err = 0;
	libusb_context *ctx;
	int bus = -1, devnum = -1;
	int status = -1;

	memset((void *)device_descriptor, 0, sizeof(struct ios_libusb_device_descriptor));

	err = libusb_init(&ctx);
	if (err) {
		ALOGE("\e[31m unable to initialize libusb: %i \e[0m\n", err);
		return status;
	}

	ALOGD("\n");
	ALOGD("\e[31mipod_create_descriptors() \e[0m\n");

	status = list_devices(ctx, bus, devnum, vendor, product, device_descriptor);

	libusb_exit(ctx);

	return status;
}

CNXUEventHandler::CNXUEventHandler()
	: isCPU(0), isIPOD(0), ipod_mode(IPOD_MODE_NO_DEVIDE), m_idVendor(0),  m_idProduct(0)
{
	struct stat fst;
	int ret = 0;

	if (gstpEventHandler == NULL) {
		if (0 != pthread_create( &m_hUEventThread, NULL, UEventMonitorThreadStub, this)) {
			ALOGD("[CNXUEventHandler] UEventMonitor thread fail!!!\n");
			return;
		}
		ALOGD("[CNXUEventHandler] UEventMonitor thread start!!!\n");

		Check_guid();

		Write_String((char *)IPOD_INSERT_DEVICE_PATH, (char *)"remove", sizeof(char)*6);
		system("chmod 744 "IPOD_INSERT_DEVICE_PATH);
		Write_String((char *)IPOD_PAIR_PATH, (char *)"unpair", sizeof(char)*6);
		system("chmod 744 "IPOD_PAIR_PATH);
		Write_String((char *)IPOD_USB_DES_PATH, (char *)"", sizeof(char));
		system("chmod 744 "IPOD_USB_DES_PATH);
	}
}

CNXUEventHandler::~CNXUEventHandler()
{
}

void *CNXUEventHandler::UEventMonitorThreadStub(void *arg)
{
	CNXUEventHandler *pObj = (CNXUEventHandler*)arg;

	pObj->UEventMonitorThread();

	return (void*)0xDeadFace;
}

void CNXUEventHandler::UEventMonitorThread()
{
	struct pollfd fds;

	ALOGD("[CNXUEventHandler] UEventMonitorThread() start\n");
	uevent_init();
	fds.fd		= uevent_get_fd();
	fds.events	= POLLIN;

	while (1) {
		int32_t err = poll( &fds, 1, 1000 );
		if (err > 0) { 
			if (fds.revents & POLLIN) {
				int32_t len = uevent_next_event(m_Desc, sizeof(m_Desc)-1);

				if (len < 1)	continue;

				 if (!strncmp(m_Desc, "add@", strlen("add@"))) {
					struct stat fst;
					char idVendor[8];
					char idProduct[8];
					int int_idVendor;
					int int_idProduct;
					int ret;
					char	*string;
					char	path[4096];

					string = strchr(m_Desc, '@');
					string++;

					memset(path, 0, 4096);
					strcat(path, "/sys");
					strcat(path, string);
					ALOGD("[CNXUEventHandler] add@ : %s\n", path);

					strcat(path, "/idVendor");

					if (stat(path, &fst) != 0)	continue;

					ret = Read_String((char*)path, (char*)idVendor, 5);
					if (ret < 0) 	continue;

					memset(path, 0, 4096);
					strcat(path, "/sys");
					strcat(path, string);
					strcat(path, "/idProduct");
					ret = Read_String((char*)path, (char*)idProduct, 5);
					if(ret < 0) 	continue;

					int_idVendor = strtol(idVendor, NULL, 16);
					int_idProduct = strtol(idProduct, NULL, 16);

					if ((int_idVendor == VID_APPLE)
						&& ((int_idProduct & 0xff00)==PID_APPLE)
						&& !Check_guid()
						) {

						isIPOD = 1;
						m_idVendor = int_idVendor;
						m_idProduct = int_idProduct;

						memcpy(m_path, string, strlen(string));
						ALOGD("[CNXUEventHandler] Insert iPod.\n");

						set_ipod_descriptors();

						ipod_mode = m_device_descriptor.current_config;
					}
					ALOGD("[CNXUEventHandler] int_idVendor:0x%x, int_idProduct:0x%x:0x%x, isIPOD:%d \n", int_idVendor, int_idProduct, (int_idProduct & 0xff00), isIPOD);
				 }

				 if (!strncmp(m_Desc, "remove@", strlen("remove@"))) {
					char	*string;

					string = strchr(m_Desc, '@');
					string++;

					ALOGD("[CNXUEventHandler] remove@ : %s\n", string);

					 if (!strcmp(m_path, string)) {
						if (isIPOD) {

							isIPOD = 0;
							m_idVendor = 0;
							m_idProduct = 0;

							ALOGD("[CNXUEventHandler] Remove iPod.\n");
							system("usbmuxdd -X");

							ipod_mode = IPOD_MODE_NO_DEVIDE;

							set_ipod_descriptors();
						}
					 }
				 }
			}
		}
	}
}


void *CNXUEventHandler::iPodPairMonitorThreadStub(void *arg)
{
	CNXUEventHandler *pObj = (CNXUEventHandler*)arg;

	pObj->iPodPairMonitorThread();

	return (void*)0xDeadFace;
}

void CNXUEventHandler::iPodPairMonitorThread()
{
	struct pollfd fds;

	ALOGD("[CNXUEventHandler] iPodPairMonitorThread() start\n");
	fds.fd		= open(IPOD_PAIR_PATH, O_RDONLY | O_NONBLOCK);
	fds.events	= POLLIN;

	while (1)	 {
		int32_t err = poll( &fds, 1, 1000000);
		if (err > 0) { 
			if (fds.revents & POLLIN) {
				char PairString[20];
				int ret;

				ret = Read_String((char*)IPOD_PAIR_PATH, (char*)PairString, 10);
				ALOGD("[CNXUEventHandler] PairString : %s\n", PairString);

				memset(PairString, 0, sizeof(char)*20);

				ret = Read_String((char*)IPOD_INSERT_DEVICE_PATH, (char*)PairString, 10);
				ALOGD("[CNXUEventHandler] PairString : %s\n", PairString);

			}
		}
	}
}

int CNXUEventHandler::Check_guid(void)
{
	uint32_t guid[4] = { 0x00000000, };
	NX_CCpuInfo *pCpuInfo = new NX_CCpuInfo();

	pCpuInfo->GetGUID( guid );
	delete pCpuInfo;

	if (guid[0] != NEXELL_CHIP_GUID0 || 
		guid[1] != NEXELL_CHIP_GUID1 ||
		guid[2] != NEXELL_CHIP_GUID2 || 
		guid[3] != NEXELL_CHIP_GUID3 ) {
			ALOGE("[CNXIPodDeviceManagerService]\e[31m Not Support CPU type.\e[0m\n" );
			isCPU = 1;
	} else {
		isCPU = 0;
	}
	return isCPU;
}

int CNXUEventHandler::find_InterfaceClass(struct ios_libusb_device_descriptor *dev_descriptor, uint8_t cfg_value, uint8_t InterfaceClass, uint8_t InterfaceSubClass)
{
	int i, j, y;
	int ret = -1;
	struct ios_libusb_config_descriptor *cfg_des;
	struct ios_libusb_interface *bInterface;
	struct ios_libusb_interface_descriptor *altsetting;

	for (i=0; i<dev_descriptor->bNumConfigurations; i++) {

		cfg_des = &dev_descriptor->config_descriptor[i];

		if (cfg_des->bConfigurationValue == cfg_value) {

			for (j=0; j<cfg_des->bNumInterfaces; j++) {

				bInterface = &cfg_des->interface[j];

				for (y=0; y<bInterface->num_altsetting; y++) {

					altsetting = &bInterface->altsetting[y];

					if ((altsetting->bInterfaceClass == InterfaceClass)
						&& (altsetting->bInterfaceSubClass == InterfaceSubClass))
						return 0;
				}
			}
		}
	}

	return ret;
}

struct ios_libusb_device_descriptor *CNXUEventHandler::get_ipod_descriptors()
{
	return &m_device_descriptor;
}

int CNXUEventHandler::set_ipod_descriptors(void)
{
	FILE *fd;
	int ret = -1;

	ipod_create_descriptors(m_idVendor, m_idProduct, &m_device_descriptor);

	fd = fopen(( char * )IPOD_USB_DES_PATH, "wb");
	if  (fd) {
		fwrite(&m_device_descriptor, sizeof(char), sizeof(struct ios_libusb_device_descriptor), fd);
		fclose(fd);
		ret = 0;
	} else {
		ALOGD("[CNXUEventHandler] Write_String() %s: Open Fail!! \n", IPOD_USB_DES_PATH);
		ret = -1;
	}

	return ret;
}

int CNXUEventHandler::get_ipod_mode()
{
	return ipod_mode;
}

void CNXUEventHandler::set_ipod_mode(int mode)
{
	ipod_mode = mode;
}

int CNXUEventHandler::Get_isIPOD()
{
	return isIPOD;
}

int CNXUEventHandler::Write_String(char * path, char *buffer, int len)
{
	FILE *fd;
	int ret = -1;

	fd = fopen(( char * )path, "wb");
	if  (fd) {
		fwrite(buffer, sizeof(char), len, fd);
		fclose(fd);
		ret = 0;
	} else {
		ALOGD("[CNXUEventHandler] Write_String() %s: Open Fail!! \n", path);
		ret = -1;
	}

	return ret;
}

int CNXUEventHandler::Read_String(char * path, char *buffer, int len)
{
	FILE *fd = NULL;
	int ret = -1;

	if (( fd = fopen( ( char * ) path, "r" ) ) != NULL )  {
		fgets( ( char * ) buffer, len, fd );
		fclose( fd );
		ret = 0;
	} else {
		ALOGD("[CNXUEventHandler] Read_String() %s: Open Fail!! \n", path);
		ret = -1;
	}

	return ret;
}


int CNXUEventHandler::Set_usb_config(int num)
{
	int ret = 0;
	char sys_cmd[64];

	system("/system/bin/usbmuxdd -X");
	sleep(2);

	sprintf(sys_cmd, "/system/bin/usbmuxdd -c %d", num);
	system(sys_cmd);

	return ret;
}

int CNXUEventHandler::Set_ios_tethering(int num)
{
	int cnt = 0, cur_mode = num;
	char PairString[20];
	int ret = 0;

	system("/system/bin/usbmuxdd -v");

	while (1) {
		ret = Read_String((char *)IPOD_PAIR_PATH, (char *)PairString, 20);

		 if ( !strncmp(PairString, "pair", sizeof((char *)"pair")) ) {
			system("echo 0 >/sys/class/iOS/ipheth/regnetdev");
			set_ipod_mode(cur_mode);
			break;
		 }
		sleep(1);

		ALOGD( "[CNXIPodDeviceManagerService] %s : %s : get_ipod_mode():%d, cur_mode:%d\n", IPOD_PAIR_PATH, PairString, get_ipod_mode(), cur_mode);

		if (ret < 0 
			|| (get_ipod_mode() != cur_mode && get_ipod_mode() != IPOD_MODE_CHANGING)
			|| get_ipod_mode() == IPOD_MODE_NO_DEVIDE 
			|| Get_isIPOD() == 0) {
			break;
		}

		if (cnt ==2 ) {
			set_ipod_mode(cur_mode);
			m_device_descriptor.current_config = cur_mode;
		}

		if (cnt < 5)	cnt ++ ;

	}

	return ret;
}



