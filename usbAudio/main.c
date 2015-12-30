/*
 * main.c
 *
 * Copyright (C) 2013-2014 Martin Szulecki <m.szulecki@libimobiledevice.org>
 * Copyright (C) 2009 Hector Martin <hector@marcansoft.com>
 * Copyright (C) 2009 Nikias Bassen <nikias@gmx.li>
 * Copyright (C) 2009 Paul Sladen <libiphone@paul.sladen.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 or version 3.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#define _BSD_SOURCE
#define _GNU_SOURCE

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <fcntl.h>
#include <getopt.h>
#include <pwd.h>
#include <grp.h>

#include "log.h"

int main(int argc, char *argv[])
{
	int res = 0;
	int num = 1;

	if( argc > 1 )
		num = atoi(argv[1]);

	// printf("## \e[31m PJSMSG \e[0m [%s():%s:%d\t] num:%d \n", __FUNCTION__, strrchr(__FILE__, '/')+1, __LINE__, num);

	usbmuxd_log(LL_INFO, "Initializing USB");
	if((res = usb_init(num)) < 0)
		goto terminate;
	usbmuxd_log(LL_INFO, "%d device%s detected", res, (res==1)?"":"s");
	usbmuxd_log(LL_NOTICE, "Initialization complete");

	usb_shutdown();
	usbmuxd_log(LL_NOTICE, "Shutdown complete");

terminate:
	return res;
}
