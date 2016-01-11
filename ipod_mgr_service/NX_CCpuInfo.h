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

//--------------------------------------------------------------------------------
// NXP4330 / S5P4418 / NXP5430 / S5P6818 GUID
// 069f41d2:4c3ae27d:68506bb8:33bc096f
#define NEXELL_CHIP_GUID0	0x069f41d2
#define NEXELL_CHIP_GUID1	0x4c3ae27d
#define NEXELL_CHIP_GUID2	0x68506bb8
#define NEXELL_CHIP_GUID3	0x33bc096f

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