//---------------------------------------------------------------------------
// Module Name: EDID.c
// Module Description: ParseEDID
//
// Copyright ?2005-2008, SII, Inc.  All rights reserved.
//---------------------------------------------------------------------------
#include <linux/types.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/device.h>

#include "TypeDefs.h"
#include "EDID.h"
#include "SiIIIC.h"
#include "SiITX.h"
#include "SiITXHDCP.h"
#include "si_apiCEC.h"
#include "hdmi_api.h"

BYTE *EDID_Raw_Data = NULL;
int EDID_Raw_Data_Length = 0;

//#define DEBUG_PRINTK printk
BOOL CEAOnFisrtPage;  // when offset pointer (Slave Addr 0x60) is not used CEA861B extension always on second page

//---------------------------------------------------------------------------
BYTE BlockReadEDID( BYTE NBytes, BYTE Addr, BYTE * Data )
{
	TmpD.MDDC.SlaveAddr = EDID_SLV;
	TmpD.MDDC.Offset = MDDCReadOffset();
	TmpD.MDDC.RegAddr = Addr;
	TmpD.MDDC.NBytesLSB = NBytes;
	TmpD.MDDC.NBytesMSB = 0;
	TmpD.MDDC.Dummy = 0;
	if(TmpD.MDDC.Offset)
		TmpD.MDDC.Cmd = MASTER_CMD_ENH_RD;
	else
		TmpD.MDDC.Cmd = MASTER_CMD_SEQ_RD;
	//TmpD.MDDC.PData = Data;
	BlockRead_MDDC(&TmpD.MDDC, Data);
	return 0;
}

//---------------------------------------------------------------------------
static void UpdateCRC16WithByte( WORD * pCRC, BYTE Data )
{
	BYTE i;
	WORD XORedIn, CRC;

	CRC = *pCRC;
	for(i = 0; i < 8; i++)
	{
		XORedIn = (WORD)Data ^ CRC; Data >>=1;
		CRC >>=1;
		if(XORedIn & MASK_LSBit)
			CRC ^=POLYNOM;
	}
	*pCRC = CRC;
}

//---------------------------------------------------------------------------
void UpdateCRC16WithBlock( WORD * pCRC, BYTE NBytes, BYTE * Data )
{
	BYTE i;

	for (i = 0; i < NBytes; i++)
	{
		UpdateCRC16WithByte( pCRC, Data[i] );
	}
}

//---------------------------------------------------------------------------
static BYTE DecodeHeader( BYTE * HeaderError, WORD * pCRC16 )
{
	BYTE i, Error = 0;
	BYTE Data[8];

	BlockReadEDID(8, 0, Data);

	UpdateCRC16WithBlock( pCRC16, 8, Data);
	if(!(Data[0] | Data[7]))
	{
		for(i = 1; i < 7; i++)
		{
			if(Data[i]!= 0xFF)
				* HeaderError = BAD_HEADER;
		}
	}
	else
	{
		* HeaderError = BAD_HEADER;
	}
	return Error;
}

//---------------------------------------------------------------------------
static BYTE CheckCRC( BYTE Addr, BYTE * CRC, WORD * pCRC16 )
{
	BYTE  Error, i, Base;
	BYTE Data[8];

	Base = Addr;
	Addr = 0;

	for(*CRC = 0; Addr < 127; Addr = Addr + 8)
	{
		Error = BlockReadEDID(8, Base + Addr, Data);
		if(Error)
			break;

		for( i = 0; i < 8; i++)
		{
			//DEBUG_PRINTK("EDID DATA =%x\n",Data[i]);
			*CRC += Data[i];
			UpdateCRC16WithBlock( pCRC16, 8, Data);
		}
	}
	return Error;
}

//---------------------------------------------------------------------------
static BYTE CheckEDIDVersion( BYTE * VersionError, WORD * pCRC16 )
{
	BYTE Data[2];
	BYTE  Error = 0;

	* VersionError = 0;

	BlockReadEDID(2, VER_ADDR, Data);

	UpdateCRC16WithBlock( pCRC16, 2, Data);
	if(! ((Data[0] == 1) &&  (Data[1] >= 3)) )
		* VersionError  = VER_DONT_SUPPORT_861B;

	return Error;
}

//---------------------------------------------------------------------------
static BYTE BasicParse( BYTE * pSysError, WORD * pCRC16 )
{
	BYTE Error;
	BYTE CRC    =0;
	DEBUG_PRINTK("\nHEADER DECODING.....\n");
	Error = DecodeHeader(pSysError, pCRC16);
	if(!Error)
	{
		if(! (*pSysError))
		{
			DEBUG_PRINTK("\n EDID Check CRC \n");
			Error = CheckCRC(0, &CRC, pCRC16);
			if(!Error)
			{
				if(CRC)
				{
					*pSysError = _1ST_BLOCK_CRC_ERROR;
					DEBUG_PRINTK("\n EDID 1st Block  CRC Error \n");
					Error = CheckEDIDVersion(pSysError, pCRC16);
				}
			}
		}
	}
	return Error;
}

//---------------------------------------------------------------------------
static BYTE CheckFor861BExtension( BYTE * NExtensions, WORD * pCRC16 )
{
	BYTE Error;

	Error = BlockReadEDID(1, NUM_EXTENSIONS_ADDR, NExtensions);
	if(!Error)
		UpdateCRC16WithBlock( pCRC16, 1, NExtensions);
	return Error;
}

//---------------------------------------------------------------------------
static BYTE CheckCRCof2ndEDIDBlock( BYTE * pSysError, WORD * pCRC16 )
{
	BYTE Error;
	BYTE CRC;

	Error = CheckCRC(128, &CRC, pCRC16);
	if(!Error)
	{
		if(CRC)
		{
			*pSysError = _1ST_BLOCK_CRC_ERROR;
			Error = CheckEDIDVersion(pSysError, pCRC16);
		}
	}
	return Error;
}

//---------------------------------------------------------------------------
static void DecodeEstablishTiming( BYTE * Data ){

	//#ifdef HDMI_DEBUG
	if(Data[0]& 0x80)
		DEBUG_PRINTK("\n720 x 400 @ 70Hz");
	if(Data[0]& 0x40)
		DEBUG_PRINTK("\n720 x 400 @ 88Hz");
	if(Data[0]& 0x20)
		DEBUG_PRINTK("\n640 x 480 @ 60Hz");
	if(Data[0]& 0x10)
		DEBUG_PRINTK("\n640 x 480 @ 67Hz");
	if(Data[0]& 0x08)
		DEBUG_PRINTK("\n640 x 480 @ 72Hz");
	if(Data[0]& 0x04)
		DEBUG_PRINTK("\n640 x 480 @ 75Hz");
	if(Data[0]& 0x02)
		DEBUG_PRINTK("\n800 x 600 @ 56Hz");
	if(Data[0]& 0x01)
		DEBUG_PRINTK("\n800 x 400 @ 60Hz");


	if(Data[1]& 0x80)
		DEBUG_PRINTK("\n800 x 600 @ 72Hz");
	if(Data[1]& 0x40)
		DEBUG_PRINTK("\n800 x 600 @ 75Hz");
	if(Data[1]& 0x20)
		DEBUG_PRINTK("\n832 x 624 @ 75Hz");
	if(Data[1]& 0x10)
		DEBUG_PRINTK("\n1024 x 768 @ 87Hz");
	if(Data[1]& 0x08)
		DEBUG_PRINTK("\n1024 x 768 @ 60Hz");
	if(Data[1]& 0x04)
		DEBUG_PRINTK("\n1024 x 768 @ 70Hz");
	if(Data[1]& 0x02)
		DEBUG_PRINTK("\n1024 x 768 @ 75Hz");
	if(Data[1]& 0x01)
		DEBUG_PRINTK("\n1280 x 1024 @ 75Hz");

	if(Data[2]& 0x80)
		DEBUG_PRINTK("\n1152 x 870 @ 75Hz");

	if((!Data[0])&&(!Data[1])&&(!Data[2]))
		DEBUG_PRINTK("\nNo established video modes");
	//#endif
}

//---------------------------------------------------------------------------
static void DecodeStandardTiming( BYTE * Data, BYTE Mode )
{
	BYTE TmpVal, i;

	Mode*=4;
	TmpVal = Mode + 4;
	for(i = 0; Mode < TmpVal; Mode++)
	{
		i++;
		DEBUG_PRINTK("\n Mode %i ",(int)Mode);
		if((Data[i] == 0x01)&&(Data[i + 1]==0x01))
		{
			DEBUG_PRINTK("Mode %d wasn't defined! ", (int)Mode);
		}
		else
		{
			DEBUG_PRINTK(" Hor Act pixels %i ", (int)((Data[Mode]+31)*8));
			DEBUG_PRINTK(" Aspect ratio: ");
			TmpVal = Data[Mode + 1] & 0xC0;
			if(TmpVal==0x00)
				DEBUG_PRINTK("16:10");
			else  if(TmpVal==0x40)
				DEBUG_PRINTK("4:3");
			else  if(TmpVal==0x80)
				DEBUG_PRINTK("5:4");
			else
				DEBUG_PRINTK("16:9");

			DEBUG_PRINTK(" Refresh rate %i Hz ", (int)((Data[Mode + 1])& 0x3F) + 60);
		}
	}
}

//---------------------------------------------------------------------------
static BYTE TestHeaderDetailedTimingDescr( BYTE * Data ){
	BYTE Res = 0;

	if(Data[0]||Data[1])
		Res = 1; // timing descriptor
	return Res;
}

//---------------------------------------------------------------------------
static void ParsingDetailedTDPart1( BYTE * Data )
{
	WORD TmpVal;

	TmpVal = (Data[2] << 4)& 0x0F00;
	TmpVal |= Data[0];
	DEBUG_PRINTK("\n H Active %i ", TmpVal );
	TmpVal = (Data[2]<<8)& 0x0F00;
	TmpVal |= Data[1];
	DEBUG_PRINTK(" H Blank %i ", TmpVal );
	TmpVal = (Data[5] << 4)& 0x0F00;
	TmpVal |= Data[3];
	DEBUG_PRINTK(" V Active %i ", TmpVal );
	TmpVal = (Data[5]<<8)& 0x0F00;
	TmpVal |= Data[4];
	DEBUG_PRINTK(" V Blank %i ", TmpVal );
}

//---------------------------------------------------------------------------
static void  ParsingDetailedTDPart2( BYTE * Data ){
	WORD TmpVal;

	TmpVal = (Data[3] << 2)& 0x0300;     TmpVal |= Data[0];
	DEBUG_PRINTK(" H Sync Offset %i", TmpVal );
	TmpVal = (Data[3] << 4)& 0x0300;     TmpVal |= Data[1];
	DEBUG_PRINTK(" H Sync Pulse W %i", TmpVal );
	TmpVal = (Data[3] << 2 ) & 0x30;      TmpVal |= (Data[2] >> 4);
	DEBUG_PRINTK(" V Sync Offset %i",   TmpVal  );
	TmpVal = (Data[3] << 4 ) & 0x30;      TmpVal |= (Data[2]& 0x0F);
	DEBUG_PRINTK(" V Sync Pulse W %i", TmpVal );
}

//---------------------------------------------------------------------------
static void  ParsingDetailedTDPart3( BYTE * Data ){
	WORD TmpVal;

	TmpVal = Data[0] |( Data[2] << 4);
	DEBUG_PRINTK(" H Image Size %i", TmpVal );
	TmpVal = Data[1] |(( Data[2] << 8)&0x0F00);
	DEBUG_PRINTK(" V Image Size %i", TmpVal );
	DEBUG_PRINTK(" H Border %i", (int)Data[3]);
	DEBUG_PRINTK(" V Border %i", (int)Data[4]);
	if(Data[5] & 0x80)
		DEBUG_PRINTK("\n Interlaced");
	else
		DEBUG_PRINTK("\n Non-interlaced");
	if(Data[5] & 0x60)
		DEBUG_PRINTK("\n Table 3.17 for defenition");
	else
		DEBUG_PRINTK("\n Normal Display:");
	if(Data[5] & 0x10)
		DEBUG_PRINTK(" Digital");
	else
		DEBUG_PRINTK(" Analog");
}

//---------------------------------------------------------------------------
static BYTE ParseTimingDescriptors( WORD * pCRC16 )
{
	BYTE Data[8];
	BYTE Error, i, BaseAddr;

	Error = BlockReadEDID(3, 0x23, Data);

	if(!Error)
	{
		UpdateCRC16WithBlock( pCRC16, 3, Data);
		DecodeEstablishTiming(Data);
		Error = BlockReadEDID(8, 0x26, Data);
		if(!Error)
		{
			DEBUG_PRINTK("\nStandard Timing:");
			DecodeStandardTiming(Data, 0);
			UpdateCRC16WithBlock( pCRC16, 8, Data);
			Error = BlockReadEDID(8, 0x2E, Data);
			if(!Error)
			{
				DecodeStandardTiming(Data, 1);
				UpdateCRC16WithBlock( pCRC16, 8, Data);
				BaseAddr = 0x36;
				DEBUG_PRINTK("\n Detailed Timing: ");
				for(i = 0; i < 4; i ++)
				{
					DEBUG_PRINTK("\n Descriptor %i:\n", (int)i);
					Error = BlockReadEDID(4, BaseAddr, Data);
					if(!Error)
					{
						UpdateCRC16WithBlock( pCRC16, 4, Data);
						if(TestHeaderDetailedTimingDescr(Data))
						{
							Error = BlockReadEDID(6, BaseAddr + 2, Data);
							if(!Error)
							{
								UpdateCRC16WithBlock( pCRC16, 6, Data);
								ParsingDetailedTDPart1(Data);
								Error = BlockReadEDID(4, BaseAddr + 8, Data);
								if(!Error)
								{
									UpdateCRC16WithBlock( pCRC16, 4, Data);
									ParsingDetailedTDPart2(Data);
									Error = BlockReadEDID(6, BaseAddr + 12, Data);
									if(!Error)
									{
										UpdateCRC16WithBlock( pCRC16, 6, Data);
										ParsingDetailedTDPart3(Data);
									}
								}
							}
						}
					}
					if(Error)
						break;
					BaseAddr+=18;
				}
			}
		}
	}

	return Error;
}

//---------------------------------------------------------------------------
static BYTE MonitorCapable861( WORD * pCRC16 ){
	BYTE CapN, Error;

	Error = BlockReadEDID(1, 0x83, &CapN);
	if(!Error){
		UpdateCRC16WithBlock( pCRC16, 1, &CapN);
		DEBUG_PRINTK("\n DTV monitor supports: ");
		if(CapN & 0x80)
			DEBUG_PRINTK(" Underscan");
		if(CapN & 0x40)
			DEBUG_PRINTK(" Basic audio");
		if(CapN & 0x20){
			DEBUG_PRINTK("YCbCr 4:4:4\n");
			siiTX.siiTXOutputVideoType = 1;
		}
		else if(CapN & 0x10){
			DEBUG_PRINTK("YCbCr 4:2:2\n");
			siiTX.siiTXOutputVideoType = 2;
		}
		else{
			DEBUG_PRINTK("RGB 4:4:4\n");
			siiTX.siiTXOutputVideoType = 0;
		}

		if(!siiTX.siiTXInputVideoType){ // For SiI9030 only(No RGB2YUV conversion), when siiTXInputVideoType is RGB, siiTXOutputVideoType must be RGB.
			DEBUG_PRINTK(" Input/Output is RGB 4:4:4");
			siiTX.siiTXOutputVideoType = 0;
		}

		DEBUG_PRINTK("\n Native formats in detail descriptors %i",(int)(CapN & 0x0F));
	}
	return Error;
}

//---------------------------------------------------------------------------
static BYTE GetCEA861BBlockTag( WORD * pCRC16, BYTE * Addr ){
	BYTE Error;
	BYTE Data;

	Error = BlockReadEDID(1, *Addr, &Data);
	if(!Error){
		UpdateCRC16WithBlock( pCRC16, 1, &Data);
		*Addr = Data;
	}
	return Error;
}

//---------------------------------------------------------------------------
static void ParsingShortTimeDescriptorBlock( BYTE * Data, BYTE Size )
{
	BYTE i;

	DEBUG_PRINTK("\nCEA861 Modes:");
	for(i = 0; i < Size; i++)
	{
		DEBUG_PRINTK("\n ID %i",(int)(Data[i]&0x7F));
		if(Data[i]&0x80)
			DEBUG_PRINTK(" Native mode");
	}
}

//---------------------------------------------------------------------------
static BYTE ParsingVideoDATABlock( WORD * pCRC16, BYTE BaseAddr )
{
	BYTE Data[8];
	BYTE Error, i;
	BYTE NBytes;

	Error = BlockReadEDID(1, BaseAddr++, &NBytes);
	if(!Error)
	{
		UpdateCRC16WithBlock( pCRC16, 1, &NBytes);
		NBytes&=0x1F;
		for(i = 0 ; i < 4; i++)
		{
			if(NBytes <= 8)
			{
				Error = BlockReadEDID(NBytes, BaseAddr, Data);
				if(!Error)
				{
					UpdateCRC16WithBlock( pCRC16, NBytes, Data);
					ParsingShortTimeDescriptorBlock(Data, NBytes);
				}

				break;
			}
			else
			{
				Error = BlockReadEDID(8, BaseAddr, Data);
				if(!Error)
				{
					UpdateCRC16WithBlock( pCRC16, 8, Data);
					ParsingShortTimeDescriptorBlock(Data, 8);
				}
				else
					break;

				NBytes-=8;
			}
		}
	}
	return Error;
}

//---------------------------------------------------------------------------
static void ParsingShortAudioDescriptor( BYTE * Data ){
	BYTE AudioFormatCode;

	AudioFormatCode = (Data[0]&0xF8)>>3;
	DEBUG_PRINTK("\nAudio Format Code %i",(int)AudioFormatCode);
	switch (AudioFormatCode){
		case 1: DEBUG_PRINTK("Liniar PCM"); break;
		case 2: DEBUG_PRINTK("AC-3"); break;
		case 3: DEBUG_PRINTK("MPEG-1"); break;
		case 4: DEBUG_PRINTK("MP3"); break;
		case 5: DEBUG_PRINTK("MPEG2"); break;
		case 6: DEBUG_PRINTK("ACC"); break;
		case 7: DEBUG_PRINTK("DTS"); break;
		case 8: DEBUG_PRINTK("ATRAC"); break;
		default: DEBUG_PRINTK("reserved");
	}
	DEBUG_PRINTK("\nMax N of channels %i",(int)((Data[0]&0x03)+1));
	DEBUG_PRINTK("\nFs: ");
	if(Data[1] & 0x01)
		DEBUG_PRINTK(" 32 ");
	if(Data[1] & 0x02)
		DEBUG_PRINTK(" 44 ");
	if(Data[1] & 0x04)
		DEBUG_PRINTK(" 48 ");
	if(Data[1] & 0x08)
		DEBUG_PRINTK(" 88 ");
	if(Data[1] & 0x10)
		DEBUG_PRINTK(" 96");
	if(Data[1] & 0x20)
		DEBUG_PRINTK(" 176 ");
	if(Data[1] & 0x40)
		DEBUG_PRINTK(" 192 ");
	DEBUG_PRINTK("KHz \n");
	if(AudioFormatCode == 1){
		DEBUG_PRINTK("Supported length: ");
		if(Data[2]&0x01)
			DEBUG_PRINTK("16 ");
		if(Data[2]&0x02)
			DEBUG_PRINTK("20 ");
		if(Data[2]&0x04)
			DEBUG_PRINTK("24 ");
		DEBUG_PRINTK("bits");
	}
	else
		DEBUG_PRINTK(" Maximum bit rate %i KHz", (int)(Data[2]<<3));
}

//---------------------------------------------------------------------------
static BYTE ParsingAudioDATABlock( WORD * pCRC16, BYTE BaseAddr )
{
	BYTE Data[3];
	BYTE Error, i = 0;
	BYTE NBytes;

	Error = BlockReadEDID(1, BaseAddr++, &NBytes);
	if(!Error)
	{
		UpdateCRC16WithBlock( pCRC16, 1, &NBytes);
		NBytes&=0x1F;
		do {
			Error = BlockReadEDID(3, BaseAddr, Data);
			if(!Error)
			{
				UpdateCRC16WithBlock( pCRC16, 3, Data);
				ParsingShortAudioDescriptor(Data);
			}
			else
				break;

			i+=3;
			BaseAddr+=3;
		} while (i < NBytes);
	}
	return Error;
}

//---------------------------------------------------------------------------
static BYTE ParsingSpeakerDATABlock( WORD * pCRC16, BYTE BaseAddr ){
	BYTE Data;
	BYTE Error;

	Error = BlockReadEDID(1, ++BaseAddr, &Data);
	if(!Error){
		UpdateCRC16WithBlock( pCRC16, 1, &Data);
		DEBUG_PRINTK("\nSpeakers' allocation: ");
		if(Data & 0x01)
			DEBUG_PRINTK("FL/FR");
		if(Data & 0x02)
			DEBUG_PRINTK("LFE");
		if(Data & 0x04)
			DEBUG_PRINTK("FC");
		if(Data & 0x08)
			DEBUG_PRINTK("RL/RR");
		if(Data & 0x10)
			DEBUG_PRINTK("RC");
		if(Data & 0x20)
			DEBUG_PRINTK("FLC/FRC");
		if(Data & 0x40)
			DEBUG_PRINTK("RLC/RRC");
	}
	return Error;
}

static BYTE ParsingExtendedTagBlock( WORD * pCRC16, BYTE BaseAddr )
{
	BYTE Data;
	BYTE Error;

	Error = BlockReadEDID(1, ++BaseAddr, &Data);
	if(!Error){
		UpdateCRC16WithBlock( pCRC16, 1, &Data);
		printk("data = 0x%x", Data);
	}

	return 0;
}

//---------------------------------------------------------------------------
static BYTE ParsingCEADataBlockCollection( WORD * pCRC16 ){
	BYTE Error;
	BYTE AddrTag, D, Addr;

	// 0. get block boundaries boundary
	// 1. Get TAG is OK if calc next Tag Address
	//    decode tag, call tag parser, if not TAG exit, if address exceed boundary exit too, otherwise go to 1
	if(CEAOnFisrtPage)
	{
		Error = BlockReadEDID(1, 0x02, &D);
		Addr = 0x04;
	}
	else
	{
		Error = BlockReadEDID(1, 0x82, &D);
		Addr = 0x84;
	}
	if(!Error){
		D+=Addr;
		AddrTag = Addr;
		do {
			Error = GetCEA861BBlockTag(pCRC16, &AddrTag);
			DEBUG_PRINTK("AddrTag = 0x%x\n",AddrTag);
			if(!Error){
				switch(AddrTag&0xE0) {
					case VIDEO_TAG:
						Error = ParsingVideoDATABlock(pCRC16, Addr); break;
					case AUDIO_TAG:
						Error = ParsingAudioDATABlock(pCRC16, Addr); break;
					case SPEAKER_TAG:
						Error = ParsingSpeakerDATABlock(pCRC16, Addr); break;
					case VENDOR_TAG: break;
					case USE_EXTENDED_TAG:
						printk("55555555555555555555555555555555555555555555555555555555555555555555555555\n");
						Error = ParsingExtendedTagBlock(pCRC16, Addr);
						break;
				}
				Addr += ( AddrTag & 0x1F ) ;   // next Tag Address
				AddrTag = ++Addr;
			}
			if(Error)
				break;
		}  while (Addr < D);
	}
	return Error;
}

//---------------------------------------------------------------------------
static BYTE ParseCEADetailedTimingDescriptors( WORD * pCRC16 )
{
	BYTE Data[6];
	BYTE BaseAddr, Error;

	// Get Start Address
	if(CEAOnFisrtPage)
		BaseAddr = 0x02;
	else
		BaseAddr = 0x82;
	Error = BlockReadEDID(1, BaseAddr, Data);
	if(!Error){
		UpdateCRC16WithBlock( pCRC16, 1, Data);
		if(CEAOnFisrtPage)
			BaseAddr = Data[0];
		else
			BaseAddr = Data[0] + 0x80;

		Error = BlockReadEDID(2, BaseAddr, Data);
		if(!Error)   {
			UpdateCRC16WithBlock( pCRC16, 2, Data);
			while(TestHeaderDetailedTimingDescr(Data)){
				Error = BlockReadEDID(6, BaseAddr + 2, Data);
				if(!Error){
					UpdateCRC16WithBlock( pCRC16, 6, Data);
					ParsingDetailedTDPart1(Data);
					Error = BlockReadEDID(4, BaseAddr + 8, Data);
					if(!Error){
						UpdateCRC16WithBlock( pCRC16, 4, Data);
						ParsingDetailedTDPart2(Data);
						Error = BlockReadEDID(6, BaseAddr + 12, Data);
						if(!Error){
							UpdateCRC16WithBlock( pCRC16, 6, Data);
							ParsingDetailedTDPart3(Data);
						}
					}
				}
				if(Error)
					break;
				else if(CEAOnFisrtPage && (BaseAddr > 0x6C)) {
					break;
				}
				else if (BaseAddr > 0xEC)
					break;
				BaseAddr+=18;
				Error = BlockReadEDID(2, BaseAddr, Data);
				if(!Error)
					UpdateCRC16WithBlock( pCRC16, 2, Data);
				else
					break;

			}
		}
	}
	return Error;
}


//////////////////////////////////////////////////////////////////////////////
//
// FUNCTION     :   byte Parse861ShortDescriptors()
//
// PURPOSE      :   Parse CEA-861 extension short descriptors of the EDID block
//                  passed as a parameter and save them in global structure
//                  EDID_Data.
//
// INPUT PARAMS :   A pointer to the EDID 861 Extension block being parsed.
//
// OUTPUT PARAMS:   None
//
// GLOBALS USED :   EDID_Data
//
// RETURNS      :   EDID_PARSED_OK if EDID parsed correctly. Error code if failed.
//
// NOTE         :   Fields that are not supported by the 9022/4 (such as deep
//                  color) are not parsed.
//
//////////////////////////////////////////////////////////////////////////////
#define byte unsigned char
Type_EDID_Descriptors EDID_Data;        // holds parsed EDID data needed by the FW

#define EDID_TAG_ADDR       0x00
#define EDID_REV_ADDR       0x01
#define EDID_TAG_IDX        0x02
#define LONG_DESCR_PTR_IDX  0x02
#define MISC_SUPPORT_IDX    0x03


#define CEC_PHYS_ADDR_LEN   0x02
#define EDID_EXTENSION_TAG  0x02
#define EDID_REV_THREE      0x03
#define EDID_DATA_START     0x04

#define THREE_LSBITS            0x07
#define FOUR_LSBITS             0x0F
#define FIVE_LSBITS             0x1F

#define VENDOR_SPEC_D_BLOCK 0x03


extern uint16_t SI_CecGetDevicePA ( void );
extern	void SI_CecSetDevicePA (uint16_t);


byte Parse861CEC(byte *Data)
{
	byte LongDescriptorOffset;
	byte DataBlockLength;
	byte DataIndex;
	byte VSDB_BaseOffset = 0;

	byte TagCode;

	if (Data[EDID_TAG_ADDR] != EDID_EXTENSION_TAG)
	{

		return -1;//EDID_EXT_TAG_ERROR;
	}

	if (Data[EDID_REV_ADDR] > EDID_REV_THREE)
	{
		printk("EDID -> Revision Error\n");
		return -1;//EDID_REV_ADDR_ERROR;
	}

	LongDescriptorOffset = Data[LONG_DESCR_PTR_IDX];    // block offset where long descriptors start

	DataIndex = EDID_DATA_START;            // 4

	while (DataIndex < LongDescriptorOffset)
	{
		TagCode = (Data[DataIndex] >> 5) & THREE_LSBITS;
		DataBlockLength = Data[DataIndex++] & FIVE_LSBITS;
		if ((DataIndex + DataBlockLength) > LongDescriptorOffset)
		{

			//return -1;//EDID_V_DESCR_OVERFLOW;
		}
		//printk("TagCode == %x DataIndex == %x \n",TagCode,DataIndex);

		switch (TagCode)
		{
			case VENDOR_SPEC_D_BLOCK:
				VSDB_BaseOffset = DataIndex - 1;

				if ((Data[DataIndex++] == 0x03) &&    // check if sink is HDMI compatible
						(Data[DataIndex++] == 0x0C) &&
						(Data[DataIndex++] == 0x00))

					EDID_Data.HDMI_Sink = TRUE;
				else
					EDID_Data.HDMI_Sink = FALSE;

				EDID_Data.CEC_A_B = Data[DataIndex++];  // CEC Physical address
				EDID_Data.CEC_C_D = Data[DataIndex++];

				// Take the Address that was passed in the EDID and use this API
				// to set the physical address for CEC.
				{
					unsigned short phyAddr;
					phyAddr = (unsigned short)EDID_Data.CEC_C_D;	 // Low-order nibbles
					phyAddr |= ((unsigned short)EDID_Data.CEC_A_B << 8); // Hi-order nibbles
					// Is the new PA different from the current PA?
					if (phyAddr != SI_CecGetDevicePA ())
					{
						printk("phyAddr === %x \n",phyAddr);
						// Yes!  So change the PA
						SI_CecSetDevicePA (phyAddr);
					}
				}
				break;

			default:
				//printk("YY\n");
				break;

		}                   // End, Switch statement
	}                       // End, while (DataIndex < LongDescriptorOffset) statement

	return 0;
}

//---------------------------------------------------------------------------
static BYTE Parse861BExtension( WORD * pCRC16 ){
	BYTE CapN[128], Error;

	Error = BlockReadEDID(128, 0x80, CapN);
	if(!Error)
		Parse861CEC(CapN);

	Error = MonitorCapable861(pCRC16);
	if(!Error)
		Error = ParsingCEADataBlockCollection(pCRC16);
	if(!Error)
		Error = ParseCEADetailedTimingDescriptors(pCRC16);
	// if(Error)
	return Error;
}

//---------------------------------------------------------------------------
static BYTE NewSearchForVendorSpecificBlock( BYTE * SearchRes, WORD * pCRC16 )
{
	BYTE Error, VSpecificBoundary, BlockAddr;
	BYTE Data[2];
	*SearchRes = 0;

	if(CEAOnFisrtPage) {
		BlockReadEDID(2, EXTENSION_ADDR_1StP + 2, Data);
	}
	else {
		BlockReadEDID(2, EXTENSION_ADDR + 2, Data);
	}

	UpdateCRC16WithBlock( pCRC16, 2, Data);
	if(Data[0] < 4)
		return Error;

	//    VSpecificBoundary = CEA_DATA_BLOCK_COLLECTION_ADDR + Data[0] - 1;
	VSpecificBoundary = EXTENSION_ADDR + Data[0];

	BlockAddr = CEA_DATA_BLOCK_COLLECTION_ADDR;
	if(CEAOnFisrtPage) {
		VSpecificBoundary = CEA_DATA_BLOCK_COLLECTION_ADDR_1StP + Data[0] - 1;
		BlockAddr = CEA_DATA_BLOCK_COLLECTION_ADDR_1StP;
	}

    //printk("@@@BlockAddr is %d, VSpecificBoundary is %d\n", BlockAddr, VSpecificBoundary);
	while( BlockAddr < VSpecificBoundary) {
		BlockReadEDID(1, BlockAddr, Data);
        //printk("@@@data is 0x%x\n", Data[0]);
		UpdateCRC16WithBlock( pCRC16, 1, Data);
		if((Data[0] & 0xE0)== TAG_VENDOR_DATA_BLOCK){
            DEBUG_PRINTK("@@@got vsdb.\n");
			* SearchRes = BlockAddr;
			break;
		}
		BlockAddr = BlockAddr + (Data[0] & 0x1F) + 1;
	}
	return Error;
}

/*
//---------------------------------------------------------------------------
static BYTE NewCheckHDMISignature( BYTE * pSysError, WORD * pCRC16 ){
BYTE Error, Addr;
BYTE Data[3];

NewSearchForVendorSpecificBlock(&Addr, pCRC16);

if (!Addr)             /// HDMI Signature block not found
 *pSysError = NO_HDMI_SIGNATURE;
 else
 {
 Error = BlockReadEDID(3, ++Addr, Data);
 if(! Error)
 {
 UpdateCRC16WithBlock( pCRC16, 3, Data);
 if((Data[0] != 0x03)||(Data[1] != 0x0C)||(Data[2] != 0x0))
 *pSysError = NO_HDMI_SIGNATURE;
 }
 }
 return Error;
 }*/

//---------------------------------------------------------------------------
static BYTE NewCheckHDMISignature( BYTE * pSysError, WORD * pCRC16 ){
	BYTE Error = 0, Addr, VSDB;
	BYTE Data[12];
	NewSearchForVendorSpecificBlock(&Addr, pCRC16);

	if (!Addr)             /// HDMI Signature block not found
		*pSysError = NO_HDMI_SIGNATURE;
	else
	{
		Error = BlockReadEDID(1, Addr, &VSDB);
		if(!Error)
		{
			UpdateCRC16WithBlock( pCRC16, 1, &VSDB);
			Error = BlockReadEDID( (((VSDB & 0x1F) <= 12) ? (VSDB & 0x1F) : 12), ++Addr, Data );
			if(!Error)
			{
				UpdateCRC16WithBlock( pCRC16, (VSDB & 0x1F), Data);
				if((Data[0] != 0x03)||(Data[1] != 0x0C)||(Data[2] != 0x0)) {
					*pSysError = NO_HDMI_SIGNATURE;
					return Error;
				}

				if((VSDB & 0x1F) <= 5)
					siiTX.siiTXOutputColorDepth = 0;   // 24 bit capability
				else if(!(Data[5] & 0x70))
					siiTX.siiTXOutputColorDepth = 0;   // 24 bit capability
				else if(Data[5] & 0x20)
					siiTX.siiTXOutputColorDepth = 2;   // 36 bit capability
				else if(Data[5] & 0x10)
					siiTX.siiTXOutputColorDepth = 1;   // 30 bit capability

#ifdef SiI9034_App
				siiTX.siiTXOutputColorDepth = 0;   // 24 bit capability
#endif
				DEBUG_PRINTK("\nEDID siiTXOutputColorDepth is %d. (0: 24bit, 1: 30bit, 2: 36bit)\n", (int)siiTX.siiTXOutputColorDepth);
			}

		}
	}
	return Error;
}

//---------------------------------------------------------------------------
static BYTE GetBlockMapTAG( BYTE * FormatError, WORD * pCRC16 ){
	BYTE Data;
	BYTE Error;

	Error = BlockReadEDID(1, 0x80, &Data);
	if(!Error){
		UpdateCRC16WithBlock( pCRC16, 1, &Data);
		if(Data != 0xF0)
			*FormatError = BLOCK_MAP_ERROR;
	}
	return Error;
}

//---------------------------------------------------------------------------
static BYTE CheckFor861BExtensionOffset( BYTE * Result, BYTE Addr, WORD * pCRC16 ){
	BYTE Error;
	BYTE Data;
	Error = BlockReadEDID(1, Addr , &Data);
	if(!Error){
		UpdateCRC16WithBlock( pCRC16, 1, &Data);
		if(Data==0x02)
			*Result  = 1;
		else
			*Result = 0;
	}
	return Error;
}

//---------------------------------------------------------------------------
static BYTE CheckCRCof1stEDIDBlock( BYTE * pSysError, WORD * pCRC16 ){
	BYTE Error;
	BYTE CRC;

	Error = CheckCRC(0, &CRC, pCRC16);
	if(!Error){
		if(CRC){
			*pSysError = _1ST_BLOCK_CRC_ERROR;
			Error = CheckEDIDVersion(pSysError, pCRC16);
		}
	}
	return Error;
}

//---------------------------------------------------------------------------
static BYTE ParseEDID(EDIDParsedDataType * ParsedData)
{
	BYTE Error, NExtensions, Addr;
	BYTE SearchResult = 0;

	CEAOnFisrtPage = 0;
	ParsedData->ErrorCode = NO_ERR;
	ParsedData->DisplayType = UNKNOWN_DISPLAY;
	ParsedData->CRC16 = 0x0000;
	DEBUG_PRINTK("\nEDID Basic Parsing...\n");
	Error = BasicParse(&ParsedData->ErrorCode, &ParsedData->CRC16);
	if(Error||ParsedData->ErrorCode)
		return Error;
	DEBUG_PRINTK("\nEDID Check 861BExtension...\n");
	// Check 861B extension
	if((Error = CheckFor861BExtension(&NExtensions, &ParsedData->CRC16)))
		return Error;
	if(!NExtensions)
	{
		ParsedData->ErrorCode =  NO_861B_EXTENSION;
		return Error;
	}
	DEBUG_PRINTK("\nEDID Check CRC ...\n");
	// Check CRC, sum of extension (128 bytes) must be Zero (0)
	Error = CheckCRCof2ndEDIDBlock(&ParsedData->ErrorCode, &ParsedData->CRC16);
	if(Error||ParsedData->ErrorCode)
		return Error;
	DEBUG_PRINTK("\nEDID Read First 128 Bytes...\n");
	if(ReadFullEDIDParserType())    // Read Cfg EEPROM option
	{
		Error = ParseTimingDescriptors(&ParsedData->CRC16);        // First 128 bytes
	}
	if(Error)
		return Error;
	DEBUG_PRINTK("\nEDID Read Others...\n");
	if(NExtensions == 1)
	{
		if(ReadFullEDIDParserType())       //// MISSING BRACES {
			Error = Parse861BExtension(&ParsedData->CRC16);
		if(Error)
			return Error;              //// MISSING BRACES }
		Error = NewCheckHDMISignature(&ParsedData->ErrorCode, &ParsedData->CRC16);
		if(Error||ParsedData->ErrorCode)
			return Error;
        DEBUG_PRINTK("@@@HDMI_DISPLAY %d\n", __LINE__);
		ParsedData->DisplayType = HDMI_DISPLAY;
	}
	else
	{
		Error = GetBlockMapTAG(&ParsedData->ErrorCode,  &ParsedData->CRC16);
		if((!Error)&&(!(ParsedData->ErrorCode)))
		{
			Addr = 1;
			do
			{
				MDDCWriteOffset(Addr++);
				Error = CheckFor861BExtensionOffset(&SearchResult, 0, &ParsedData->CRC16);
				if(SearchResult && (!Error))
				{
					Error = CheckCRCof1stEDIDBlock(&ParsedData->ErrorCode, &ParsedData->CRC16);
					if(Error||ParsedData->ErrorCode)
						break;

					// 1 set offest pointer
					CEAOnFisrtPage = 1;
					Error = NewCheckHDMISignature(&ParsedData->ErrorCode, &ParsedData->CRC16);
					if(Error||ParsedData->ErrorCode)
						break;
					else
					{
                        DEBUG_PRINTK("@@@HDMI_DISPLAY %d\n", __LINE__);
						ParsedData->DisplayType = HDMI_DISPLAY;
					}
					if(ReadFullEDIDParserType())
					{
						Error =  Parse861BExtension(&ParsedData->CRC16);
						if(Error)
							break;

						CEAOnFisrtPage = 0;
						Error = CheckCRCof2ndEDIDBlock(&ParsedData->ErrorCode, &ParsedData->CRC16);
						if(Error||ParsedData->ErrorCode)
							break;
						Error =  Parse861BExtension(&ParsedData->CRC16);
						if(Error)
							break;
					}
				}
				if(Error)
					break;
				if(ParsedData->DisplayType == HDMI_DISPLAY)
				{
                    DEBUG_PRINTK("@@@HDMI_DISPLAY %d\n", __LINE__);
					break;
				}
			}
			while (NExtensions--);
		}
	}

	MDDCWriteOffset(0);
	return Error;
}

//---------------------------------------------------------------------------
static void AssignOutputState( BYTE DisplType ){

	if( DisplType == HDMI_DISPLAY )
		siiTX.siiTXOutputState = CABLE_PLUGIN_HDMI_OUT;
	else
		siiTX.siiTXOutputState = CABLE_PLUGIN_DVI_OUT;
}

static int raw_data_checksum(unsigned char *d)
{
	int i;
	unsigned char sum = 0;

	for(i=0; i<128; i++)
		sum += d[i];

	if (sum !=0)
		return -1;

	return 0;
}

//---------------------------------------------------------------------------
void EDIDProcessing( void )
{
	EDIDParsedDataType EDIDParsedData;
	BYTE edid_ext_num = 0xff;
	int i;

	DEBUG_PRINTK(".................start Func EDIDProcessing.......0.............\n");
#if 1
	// save EDID data for application use, added by xp 2012-03-18
	BlockReadEDID(0x1,0x7E,&edid_ext_num);
	if (edid_ext_num == 0xff)
	{
		printk("read edid data length fail\n");
	}
	else
	{
		if (EDID_Raw_Data_Length == (edid_ext_num + 1) * 128)
		{
			for (i=0; i<=edid_ext_num; i++)
			{
                if (i <= 1)
    				BlockReadEDID(0x80,i*0x80,EDID_Raw_Data+i*0x80);
                else
                {
                    MDDCWriteOffset(i/2);
                    BlockReadEDID(0x80,(i%2)*0x80,EDID_Raw_Data+i*0x80);
                    MDDCWriteOffset(0);
                }
				if(raw_data_checksum(EDID_Raw_Data+i*0x80) != 0)
				{
					printk("checksum fail, won't save EDID data\n");
					EDID_Raw_Data_Length = 0;
					if (EDID_Raw_Data != NULL)
					{
						kfree(EDID_Raw_Data);
						EDID_Raw_Data = NULL;
					}
					break;
				}
			}
		}
		else
		{
			EDID_Raw_Data_Length = (edid_ext_num + 1) * 128;
			if (EDID_Raw_Data != NULL)
			{
				kfree(EDID_Raw_Data);
				EDID_Raw_Data = NULL;
			}
			EDID_Raw_Data = (BYTE *)kmalloc(EDID_Raw_Data_Length, GFP_KERNEL);
			if (EDID_Raw_Data == NULL)
				EDID_Raw_Data_Length = 0;
			else
			{
				for (i=0; i<=edid_ext_num; i++)
				{
                    if (i <= 1)
                        BlockReadEDID(0x80,i*0x80,EDID_Raw_Data+i*0x80);
                    else
                    {
                        MDDCWriteOffset(i/2);
                        BlockReadEDID(0x80,(i%2)*0x80,EDID_Raw_Data+i*0x80);
                        MDDCWriteOffset(0);
                    }
					if(raw_data_checksum(EDID_Raw_Data+i*0x80) != 0)
					{
						printk("checksum fail, won't save EDID data\n");
						EDID_Raw_Data_Length = 0;
						if (EDID_Raw_Data != NULL)
						{
							kfree(EDID_Raw_Data);
							EDID_Raw_Data = NULL;
						}
						break;
					}
				}
			}
		}
	}
#endif
	if(!ParseEDID(&EDIDParsedData))
	{
		DEBUG_PRINTK(".................start Func EDIDProcessing.......1.............\n");
		if(!EDIDParsedData.ErrorCode)
		{
			AssignOutputState( EDIDParsedData.DisplayType );
			DEBUG_PRINTK("@@@AssignOutputState : %d\n", EDIDParsedData.DisplayType);
		}
		else if( EDIDParsedData.ErrorCode >= 8 )
		{
			// EDID error, or not supported version
			DEBUG_PRINTK("\n@@@EDID Error %02i \n", (int)EDIDParsedData.ErrorCode);
			AssignOutputState( CABLE_PLUGIN_DVI_OUT ); // after improper EDID, it should be replaced by DVI initialization //majia change to HDMI (CABLE_PLUGIN_DVI_OUT)
		}
	}
	else
	{	DEBUG_PRINTK(".................start Func EDIDProcessing.......3.............\n");
		siiTX.siiTXOutputState = CABLE_UNPLUG_;
	}
}

const BYTE * CSTVOUT_GetHDMIEdidRawData(unsigned int *edid_data_len)
{
	if (EDID_Raw_Data_Length == 0)
	{
		*edid_data_len = 0;
		return NULL;
	}

	*edid_data_len = EDID_Raw_Data_Length;
	return EDID_Raw_Data;
}
