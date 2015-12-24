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
//	File		: CNXUEventHandler.h
//	Description	: 
//	Author		: Seong-O Park(ray@nexell.co.kr)
//	Export		:
//	History		:
//
//------------------------------------------------------------------------------


#ifndef __CNXUEVENTHANDLER_H__
#define __CNXUEVENTHANDLER_H__

#include <stdint.h>

class CNXUEventHandler {
public:
	CNXUEventHandler();
	virtual ~CNXUEventHandler();
	int 	Get_isIPOD();
private:
	int			isIPOD;
	char		m_Desc[4096];
	pthread_t	m_hUEventThread;
	static void	*UEventMonitorThreadStub( void * arg );
	void		UEventMonitorThread();
	int			Read_ID(char * path, char *buffer, int len);
};

void NX_StartUEventHandler();

#endif	//	__CNXUEVENTHANDLER_H__
