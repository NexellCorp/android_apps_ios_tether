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

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <CNXIPodManagerService.h>
#include <include/NXCCpuInfo.h>

#define NX_DTAG "[NX_CCpuInfo]"
//#include "NX_DbgMsg.h"

//------------------------------------------------------------------------------
#define GUID_FILE_PATH		"/sys/devices/platform/cpu/guid"	// Global Chip ID			>> 128 Bit HEX
#define UUID_FILE_PATH		"/sys/devices/platform/cpu/uuid"	// Unique Chip ID ( ECID )	>> 128 Bit HEX

//------------------------------------------------------------------------------
NX_CCpuInfo::NX_CCpuInfo()
{
	memset( m_CpuGuid, 0x00, sizeof( m_CpuGuid) );
	memset( m_CpuUuid, 0x00, sizeof( m_CpuUuid) );

	ReadGUID();
	ReadUUID();
}

//------------------------------------------------------------------------------
NX_CCpuInfo::~NX_CCpuInfo()
{

}

//------------------------------------------------------------------------------
int32_t NX_CCpuInfo::ReadGUID( void )
{
	int32_t fd;
	char buffer[128];

	if( access(GUID_FILE_PATH, F_OK) ) {
		ALOGE("Fail, access file. ( %s )\n", GUID_FILE_PATH );
		return -1;
	}

	if( 0 > (fd = open(GUID_FILE_PATH, O_RDONLY) ) ) {
		ALOGE("Fail, open file. ( %s )\n", GUID_FILE_PATH );
		return -1;
	}

	if( 0 > read( fd, buffer, sizeof(buffer) ) ) {
		ALOGE("Fail, read file.\n" );
		close( fd );
		return -1;
	}

	close( fd );

	sscanf( buffer, "%x:%x:%x:%x", &m_CpuGuid[0], &m_CpuGuid[1], &m_CpuGuid[2], &m_CpuGuid[3] );
	ALOGE("GUID = %08x:%08x:%08x:%08x", m_CpuGuid[0], m_CpuGuid[1], m_CpuGuid[2], m_CpuGuid[3] );

	return 0;
}

//------------------------------------------------------------------------------
int32_t NX_CCpuInfo::ReadUUID( void )
{
	int32_t fd;
	char buffer[128];

	if( access(UUID_FILE_PATH, F_OK) ) {
		ALOGE( "Fail, access file. ( %s )\n", UUID_FILE_PATH );
		return -1;
	}

	if( 0 > (fd = open(UUID_FILE_PATH, O_RDONLY) ) ) {
		ALOGE("Fail, open file. ( %s )\n", UUID_FILE_PATH );
		return -1;
	}

	if( 0 > read( fd, buffer, sizeof(buffer) ) ) {
		ALOGE("Fail, read file.\n" );
		close( fd );
		return -1;
	}

	close( fd );

	sscanf( buffer, "%x:%x:%x:%x", &m_CpuUuid[0], &m_CpuUuid[1], &m_CpuUuid[2], &m_CpuUuid[3] );
	// ALOGI("UUID = %08x:%08x:%08x:%08x", m_CpuUuid[0], m_CpuUuid[1], m_CpuUuid[2], m_CpuUuid[3] );

	return 0;
}

//------------------------------------------------------------------------------
static unsigned int ConvertMSBLSB( uint32_t data, int bits )
{
	uint32_t result = 0;
	uint32_t mask = 1;

	int i=0;
	for( i=0; i<bits ; i++ )
	{
		if( data&(1<<i) )
		{
			result |= mask<<(bits-i-1);
		}
	}
	return result;
}

//------------------------------------------------------------------------------
static const char gst36StrTable[36] = 
{
	'0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
	'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J',
	'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',
	'U', 'V', 'W', 'X', 'Y', 'Z'
};

//------------------------------------------------------------------------------
static void LotIDNum2String( uint32_t lotId, char str[6] )
{
	uint32_t value[3];
	uint32_t mad[3];

	value[0] = lotId / 36;
	mad[0] = lotId % 36;

	value[1] = value[0] / 36;
	mad[1] = value[0] % 36;

	value[2] = value[1] / 36;
	mad[2] = value[1] % 36;

	// Lot ID String
	// value[2] mad[2] mad[1] mad[0]
	str[0] = 'N';
	str[1] = gst36StrTable[ value[2] ];
	str[2] = gst36StrTable[ mad[2] ];
	str[3] = gst36StrTable[ mad[1] ];
	str[4] = gst36StrTable[ mad[0] ];
	str[5] = '\0';
}

//------------------------------------------------------------------------------
void NX_CCpuInfo::GetGUID( uint32_t guid[4] )
{
	for( int32_t i = 0; i < 4; i++ )
		guid[i] = m_CpuGuid[i];
}

//------------------------------------------------------------------------------
void NX_CCpuInfo::GetUUID( uint32_t uuid[4] )
{
	for( int32_t i = 0; i < 4; i++ )
		uuid[i] = m_CpuUuid[i];
}

//------------------------------------------------------------------------------
void NX_CCpuInfo::ParseUUID( NX_CHIP_INFO *pChipInfo )
{
	pChipInfo->lotID   = ConvertMSBLSB( m_CpuUuid[0] & 0x1FFFFF, 21 );
	pChipInfo->waferNo = ConvertMSBLSB( (m_CpuUuid[0]>>21) & 0x1F, 5 );
	pChipInfo->xPos    = ConvertMSBLSB( ((m_CpuUuid[0]>>26) & 0x3F) | ((m_CpuUuid[1]&0x3)<<6), 8 );
	pChipInfo->yPos    = ConvertMSBLSB( (m_CpuUuid[1]>>2) & 0xFF, 8 );
	pChipInfo->ids     = ConvertMSBLSB( (m_CpuUuid[1]>>16) & 0xFF, 8 );
	pChipInfo->ro      = ConvertMSBLSB( (m_CpuUuid[1]>>24) & 0xFF, 8 );
	pChipInfo->pid     = m_CpuUuid[3] & 0xFFFF;
	pChipInfo->vid     = (m_CpuUuid[3]>>16) & 0xFFFF;

	LotIDNum2String( pChipInfo->lotID, pChipInfo->strLotID );
	ALOGI("CPU Info : lot( %s ), wafer( %d ), x( %d ), y( %d ), ids( %d ), ro( %d ), vid( 0x%04x ), pid( 0x%04x )\n",
		pChipInfo->strLotID, pChipInfo->waferNo,	//	Wafer Information
		pChipInfo->xPos, pChipInfo->yPos,			//	Chip Postion
		pChipInfo->ids, pChipInfo->ro,				//	IDS & RO
		pChipInfo->vid, pChipInfo->pid);			//	Vendor ID & Product ID
}