//------------------------------------------------------------------------------
//
//	Copyright (C) 2014 Nexell Co. All Rights Reserved
//	Nexell Co. Proprietary & Confidential
//
//	NEXELL INFORMS THAT THIS CODE AND INFORMATION IS PROVIDED "AS IS" BASE
//  AND	WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING
//  BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS
//  FOR A PARTICULAR PURPOSE.
//
//	Module		:
//	File		:
//	Description	:
//	Author		: 
//	Export		:
//	History		:
//
//------------------------------------------------------------------------------

#ifndef __NX_CCPUINFO_H__
#define __NX_CCPUINFO_H__

#include <stdint.h>

typedef struct NX_CHIP_INFO {
	uint32_t	lotID;
	uint32_t	waferNo;
	uint32_t	xPos;
	uint32_t	yPos;
	uint32_t	ids;
	uint32_t	ro;
	uint32_t	vid;
	uint32_t	pid;
	char		strLotID[6];	//	Additional Information
} NX_CHIP_INFO;

class NX_CCpuInfo {
public:
	NX_CCpuInfo();
	~NX_CCpuInfo();

public:
	void		GetGUID( uint32_t guid[4] );
	void		GetUUID( uint32_t uuid[4] );
	
	void 		ParseUUID( NX_CHIP_INFO *pChipInfo );

private:
	int32_t		ReadGUID( void );
	int32_t		ReadUUID( void );

private:
	uint32_t	m_CpuGuid[4];
	uint32_t	m_CpuUuid[4];

private:
	NX_CCpuInfo (NX_CCpuInfo &Ref);
	NX_CCpuInfo &operator=(NX_CCpuInfo &Ref);	
};

#endif	// __NX_CCPUINFO_H__