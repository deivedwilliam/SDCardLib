/**
 ******************************************************************************
  * @file    user_diskio.c
  * @brief   This file includes a diskio driver skeleton to be completed by the user.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2019 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */

#include <string.h>
#include "stdio.h"
#include "user_diskio.h"
#include "fatfs_platform.h"
#include "FSDefs.h"
#include "stm32h7xx.h"
#include "stm32h743xx.h"
#include "main.h"


//#define SD_SPI_WRITE_DEBUG
#define SD_CARD_DEBUG_READ_FUNC

#define SPI_ReadByte() SPI_WriteByte(0xFF)
#define MAX_SPEED	1
#define LOW_SPEED	0

void SetCommand(unsigned char* command_array, unsigned char start_bit, unsigned char host_bit, unsigned char command, unsigned int argument, unsigned char crc7, unsigned char end_bit);


void SetCommand(unsigned char* command_array, unsigned char start_bit, unsigned char host_bit, unsigned char command, unsigned int argument, unsigned char crc7, unsigned char end_bit)
{
    (*((unsigned long long*)command_array)) |= (((unsigned long long)start_bit) << 47);
    (*((unsigned long long*)command_array)) |= ((unsigned long long)host_bit) << 46;
    (*((unsigned long long*)command_array)) |= ((unsigned long long)command) << 40;
    (*((unsigned long long*)command_array)) |= ((unsigned long long)argument) << 8;
    (*((unsigned long long*)command_array)) |= ((unsigned char)crc7) << 1;
    (*((unsigned long long*)command_array)) |= ((unsigned char)end_bit) & 0x0000000000000001;
}

static MEDIA_INFORMATION mediaInformation;
extern SPI_HandleTypeDef hspi5;
const typMMC_CMD cmdTable[] =
{
	// cmd                      crc     response
	{cmdGO_IDLE_STATE,          0x95,   R1,     NODATA},
	{cmdSEND_OP_COND,           0xF9,   R1,     NODATA},
	{cmdSEND_IF_COND,      		0x87,   R7,     NODATA},
	{cmdSEND_CSD,               0xAF,   R1,     MOREDATA},
	{cmdSEND_CID,               0x1B,   R1,     MOREDATA},
	{cmdSTOP_TRANSMISSION,      0xC3,   R1,     NODATA},
	{cmdSEND_STATUS,            0xAF,   R2,     NODATA},
	{cmdSET_BLOCKLEN,           0xFF,   R1,     NODATA},
	{cmdREAD_SINGLE_BLOCK,      0xFF,   R1,     MOREDATA},
	{cmdREAD_MULTI_BLOCK,       0xFF,   R1,     MOREDATA},
	{cmdWRITE_SINGLE_BLOCK,     0xFF,   R1,     MOREDATA},
	{cmdWRITE_MULTI_BLOCK,      0xFF,   R1,     MOREDATA},
	{cmdTAG_SECTOR_START,       0xFF,   R1,     NODATA},
	{cmdTAG_SECTOR_END,         0xFF,   R1,     NODATA},
	{cmdERASE,                  0xDF,   R1b,    NODATA},
	{cmdAPP_CMD,                0x73,   R1,     NODATA},
	{cmdREAD_OCR,               0x25,   R7,     NODATA},
	{cmdCRC_ON_OFF,             0x25,   R1,     NODATA}
};

static DWORD MDD_SDSPI_finalLBA;
static WORD gMediaSectorSize;
static BYTE gSDMode;


static uint8_t SPI_WriteByte(uint8_t byte);
static void SD_Select(void);
static void SD_DeSelect(void);
static MMC_RESPONSE SendMMCCmd(SdCMD cmd, DWORD address);

static BYTE SD_ReadyWait(void)
{
	uint8_t res;

	SPI_ReadByte();

	do
	{
	/* 0xFF SPI communication until a value is received */
		res = SPI_ReadByte();
	}while ((res != 0xFF));

	return res;
}

void MX_SPI5_FreqSwitch(unsigned int freq)
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};

	if(HAL_SPI_DeInit(&hspi5) != HAL_OK)
	{
		Error_Handler();
	}
	if(freq)
	{
//		HAL_GPIO_DeInit(GPIOF, GPIO_PIN_7|GPIO_PIN_8|GPIO_PIN_9);
//		GPIO_InitStruct.Pin = GPIO_PIN_7|GPIO_PIN_8|GPIO_PIN_9;
//		GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
//		GPIO_InitStruct.Pull = GPIO_NOPULL;
//		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
//		GPIO_InitStruct.Alternate = GPIO_AF5_SPI5;
//		HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);
		hspi5.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_4;
	}
	else if(freq == 0)
	{
		hspi5.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_256;

	}

	if(HAL_SPI_Init(&hspi5) != HAL_OK)
	{
		printf("SPI init error\n");
		Error_Handler();
	}
}


static uint8_t SPI_WriteByte(uint8_t byte)
{
	uint8_t receive;

	while(HAL_SPI_GetState(&hspi5) != HAL_SPI_STATE_READY);
	HAL_SPI_TransmitReceive(&hspi5, &byte, &receive, 1, 1000);

	return receive;
}


static void SD_Select(void)
{
	HAL_GPIO_WritePin(SD_SELECT_GPIO_Port, SD_SELECT_Pin, GPIO_PIN_RESET);
}

static void SD_DeSelect(void)
{
	HAL_GPIO_WritePin(SD_SELECT_GPIO_Port, SD_SELECT_Pin, GPIO_PIN_SET);
}


BOOL MDD_SDSPI_MediaDetect(void)
{
	if(HAL_GPIO_ReadPin(SD_DETECT_GPIO_Port, SD_DETECT_Pin) != GPIO_PIN_RESET)
	{
		return FALSE;
	}

	return TRUE;
}

static MMC_RESPONSE SendMMCCmd(SdCMD cmd, DWORD address)
{
	BYTE cmd_array[8] = {0, 0, 0, 0, 0, 0, 0, 0};
	MMC_RESPONSE response;
	int i;
	int timeout = 0x8;

	SetCommand(cmd_array, 0, 1, cmdTable[cmd].CmdCode, address, cmdTable[cmd].sdCRC >> 1, 1);

#ifdef SD_SPI_WRITE_DEBUG
	printf(".........................\n");
	printf("CMD %i\n", cmd_array[5]);
	printf("addr %i\n", cmd_array[4]);
	printf("addr %i\n", cmd_array[3]);
	printf("addr %i\n", cmd_array[2]);
	printf("addr %i\n", cmd_array[1]);
	printf("crc %i\n", cmd_array[0]);
	printf(".........................\n");
#endif
//	if(SD_ReadyWait() != 0xFF)
//	{
//		response.r1._byte = 0xFF;
//		return response;
//	}
	SD_Select();
	(void)SPI_WriteByte(cmd_array[5]);
	(void)SPI_WriteByte(cmd_array[4]);
	(void)SPI_WriteByte(cmd_array[3]);
	(void)SPI_WriteByte(cmd_array[2]);
	(void)SPI_WriteByte(cmd_array[1]);
	(void)SPI_WriteByte(cmd_array[0]);

	if(cmdTable[cmd].responsetype == R1 || cmdTable[cmd].responsetype == R1b  || cmdTable[cmd].responsetype == R7)
	{
		do
		{
			response.r1._byte = SPI_ReadByte();
			timeout--;
		}while(response.r1._byte == MMC_FLOATING_BUS && timeout != 0);
	}
	else if(cmdTable[cmd].responsetype == R2)
	{
		(void)SPI_WriteByte(0xFF);
		response.r2._byte1 = SPI_ReadByte();
		response.r2._byte0 = SPI_ReadByte();
	}
	if(cmdTable[cmd].responsetype == R1b)
	{
		response.r1._byte = 0x00;
		for(i = 0; i < 0xFF && response.r1._byte != 0x00; i++)
		{
			timeout = 0xFFFF;
			do
			{
				response.r1._byte = SPI_ReadByte();
				timeout--;
			}while((response.r1._byte != 0x00) && (timeout != 0));
		}
	}

	if(cmdTable[cmd].responsetype == R7)
	{
		response.r7.bytewise._returnVal = ((DWORD)SPI_ReadByte()) << 24;
		response.r7.bytewise._returnVal += ((DWORD)SPI_ReadByte()) << 16;
		response.r7.bytewise._returnVal += ((DWORD)SPI_ReadByte()) << 8;
		response.r7.bytewise._returnVal += ((DWORD)SPI_ReadByte());
	}

	(void)SPI_WriteByte(0xFF);
	if(!(cmdTable[cmd].moredataexpected))
		SD_DeSelect();

	return response;
}



MEDIA_INFORMATION* MDD_SDSPI_MediaInitialize(void)
{

	int i;
    MMC_RESPONSE response;
    WORD timeout;
	BYTE CSDResponse[20];
	DWORD c_size;
	BYTE c_size_mult;
	BYTE block_len;

	MDD_SDSPI_finalLBA = 0;
    MX_SPI5_FreqSwitch(LOW_SPEED);

	SD_DeSelect();

	for(i = 0; i < 10; i++)
	{
		(void)SPI_WriteByte(0xFF);
	}
	SD_Select();

	response = SendMMCCmd(GO_IDLE_STATE, 0);

	if((response.r1._byte == MMC_BAD_RESPONSE) || ((response.r1._byte & 0xF7) != 0x01) )
	{
#ifdef SD_CARD_DEBUG
		DEBUG_Message("Sd not Initialized",__FUNCTION__ ,__FILE__, __LINE__);
#endif
		SD_DeSelect();
		mediaInformation.errorCode = MEDIA_CANNOT_INITIALIZE;
		return &mediaInformation;
	}

	response = SendMMCCmd(SEND_IF_COND, 0x1AA);

#ifdef SD_CARD_DEBUG
	printf("Response[SEND_IF_COND] = 0x%08x\n",  response.r7.bytewise._returnVal);
#endif
	if(((response.r7.bytewise._returnVal & 0xFFF) == 0x1AA) && (!response.r7.bitwise.bits.ILLEGAL_CMD))
	{
		timeout = 0xFFF;
		do
		{
			response = SendMMCCmd(SEND_OP_COND, 0x40000000);
			timeout--;
		}
		while(response.r1._byte  != 0x00 && timeout != 0);

#ifdef SD_CARD_DEBUG
		printf("Response[SEND_OP_COND] = 0x%08x, Timeout[%i\n]\n",  response.r1._byte, timeout);
#endif
		response = SendMMCCmd(READ_OCR, 0x0);

#ifdef SD_CARD_DEBUG
		printf("Response[READ_OCR] = 0x%08x\n",  response.r7.bytewise._returnVal);
#endif
		if(((response.r7.bytewise._returnVal & 0xC0000000) == 0xC0000000) && (response.r7.bytewise._byte == 0))
		{
			gSDMode = SD_MODE_HC;
		}
		else //if (((response.r7.bytewise._returnVal & 0xC0000000) == 0x80000000) && (response.r7.bytewise._byte == 0))
		{
#ifdef SD_CARD_DEBUG
			DEBUG_Message("SD card Normal nomde\n", __FUNCTION__, __FILE__, __LINE__);
#endif
			gSDMode = SD_MODE_NORMAL;
		}
	}
	else
	{
		gSDMode = SD_MODE_NORMAL;

		// According to spec cmd1 must be repeated until the card is fully initialized
		timeout = 0xFFF;
		do
		{
			response = SendMMCCmd(SEND_OP_COND, 0x0);
			timeout--;
		}while(response.r1._byte != 0x00 && timeout != 0);

#ifdef SD_CARD_DEBUG
		printf("Response[SEND_OP_COND] = 0x%08x, Timeout[%i]\n",  response.r1._byte, timeout);
#endif

	}
	if(timeout == 0)
	{
		mediaInformation.errorCode = MEDIA_CANNOT_INITIALIZE;
#ifdef SD_CARD_DEBUG
		DEBUG_Message("MEDIA CANNOT INITIALIZE", __FUNCTION__, __FILE__, __LINE__);
#endif
		SD_DeSelect();
	}
	else
	{
		HAL_Delay(2);
		MX_SPI5_FreqSwitch(MAX_SPEED);
		SD_DeSelect();
		/* Send the CMD9 to read the CSD register */
		timeout = 0xFFF;
		do
		{
			response = SendMMCCmd(SEND_CSD, 0x00);
			timeout--;
		}while((response.r1._byte != 0x00) && (timeout != 0));

		/* According to the simplified spec, section 7.2.6, the card will respond
		with a standard response token, followed by a data block of 16 bytes
		suffixed with a 16-bit CRC.*/
		int index = 0;
		for(i = 0; i < 20; i++)
		{
			CSDResponse[index] = SPI_ReadByte();
			printf("[%i]\n", CSDResponse[index]);
			index++;
			/* Hopefully the first byte is the datatoken, however, some cards do
			not send the response token before the CSD register.*/
			if((i == 0) && (CSDResponse[0] == DATA_START_TOKEN))
			{
				/* As the first byte was the datatoken, we can drop it. */
				index = 0;
			}
		}

		gMediaSectorSize = 512u;
		mediaInformation.validityFlags.bits.sectorSize = TRUE;
		mediaInformation.sectorSize = gMediaSectorSize;
		//-------------------------------------------------------------

		//Calculate the MDD_SDSPI_finalLBA (see SD card physical layer simplified spec 2.0, section 5.3.2).
		//In USB mass storage applications, we will need this information to
		//correctly respond to SCSI get capacity requests.  Note: method of computing
		//MDD_SDSPI_finalLBA depends on CSD structure spec version (either v1 or v2).
		if(CSDResponse[0] & 0xC0)	//Check CSD_STRUCTURE field for v2+ struct device
		{
			//Must be a v2 device (or a reserved higher version, that doesn't currently exist)

			//Extract the C_SIZE field from the response.  It is a 22-bit number in bit position 69:48.  This is different from v1.
			//It spans bytes 7, 8, and 9 of the response.
			c_size = (((DWORD)CSDResponse[7] & 0x3F) << 16) | ((WORD)CSDResponse[8] << 8) | CSDResponse[9];

			MDD_SDSPI_finalLBA = ((DWORD)(c_size + 1) * (WORD)(1024u)) - 1; //-1 on end is correction factor, since LBA = 0 is valid.
		}
		else //if(CSDResponse[0] & 0xC0)	//Check CSD_STRUCTURE field for v1 struct device
		{
			//Must be a v1 device.
			//Extract the C_SIZE field from the response.  It is a 12-bit number in bit position 73:62.
			//Although it is only a 12-bit number, it spans bytes 6, 7, and 8, since it isn't byte aligned.
			c_size = ((DWORD)CSDResponse[6] << 16) | ((WORD)CSDResponse[7] << 8) | CSDResponse[8];	//Get the bytes in the correct positions
			c_size &= 0x0003FFC0;	//Clear all bits that aren't part of the C_SIZE
			c_size = c_size >> 6;	//Shift value down, so the 12-bit C_SIZE is properly right justified in the DWORD.

			//Extract the C_SIZE_MULT field from the response.  It is a 3-bit number in bit position 49:47.
			c_size_mult = ((WORD)((CSDResponse[9] & 0x03) << 1)) | ((WORD)((CSDResponse[10] & 0x80) >> 7));

			//Extract the BLOCK_LEN field from the response. It is a 4-bit number in bit position 83:80.
			block_len = CSDResponse[5] & 0x0F;

			block_len = 1 << (block_len - 9); //-9 because we report the size in sectors of 512 bytes each

			//Calculate the MDD_SDSPI_finalLBA (see SD card physical layer simplified spec 2.0, section 5.3.2).
			//In USB mass storage applications, we will need this information to
			//correctly respond to SCSI get capacity requests (which will cause MDD_SDSPI_ReadCapacity() to get called).
			MDD_SDSPI_finalLBA = ((DWORD)(c_size + 1) * (WORD)((WORD)1 << (c_size_mult + 2)) * block_len) - 1;	//-1 on end is correction factor, since LBA = 0 is valid.

		}

		printf("%i\n", MDD_SDSPI_finalLBA);
		// Turn off CRC7 if we can, might be an invalid cmd on some cards (CMD59)
		response = SendMMCCmd(CRC_ON_OFF, 0x0);
#ifdef SD_CARD_DEBUG
		printf("Response[CRC_ON_OFF] = 0x%08x\n",  response.r1._byte);
#endif
		// Now set the block length to media sector size. It should be already
		response = SendMMCCmd(SET_BLOCKLEN, gMediaSectorSize);
#ifdef SD_CARD_DEBUG
		printf("Response[SET_BLOCKLEN] = 0x%08x\n",  response.r1._byte);
		printf("Blocks in card %u\n", MDD_SDSPI_finalLBA);
		printf("SD MODE: %s\n", gSDMode == SD_MODE_NORMAL?"SD NORMAL MODE": "SD MODE HC");
#endif

	}

	SPI_WriteByte(0xFF);
	SD_DeSelect();


	//MX_SPI5_FreqSwitch(MAX_FREQ);

    return &mediaInformation;
}
 

/**
  * @brief  Reads Sector(s) 
  * @param  *buffer: Data buffer to store read data
  * @param  sector_addr: Sector address (LBA)
  * @param  count: Number of sectors to read (1..128)
  * @retval OPStatus: Operation result ERROR/SUCCESS
  */
ErrorStatus MDD_SDSPI_SectorRead(DWORD sector_addr, BYTE* buffer)
{
	WORD index;
	WORD delay;
	MMC_RESPONSE response;
	BYTE data_token;
	BYTE crc_byte0, crc_byte1;
	DWORD new_addr;

	if(gSDMode == SD_MODE_NORMAL)
	{
		new_addr = sector_addr << 9;
	}
	else
	{
		new_addr = sector_addr;
	}

	SD_Select();

	response = SendMMCCmd(READ_SINGLE_BLOCK, new_addr);
	if(response.r1._byte != 0x00)
	{
		response = SendMMCCmd(READ_SINGLE_BLOCK, new_addr);

		if(response.r1._byte != 0x00)
		{
#ifdef SD_CARD_DEBUG_READ_FUNC
			printf("Error command READ_SINGLE_BLOCK 0x%08x\n", response.r1._byte);
#endif
			return ERROR;
		}
	}


	index = 0x2FF;

	// Timing delay- at least 8 clock cycles
	delay = 0x40;
	while (delay)
		delay--;

	//Now, must wait for the start token of data block
	do
	{
		data_token = SPI_ReadByte();
		index--;

		delay = 0x40;
		while(delay)
			delay--;

	}while((data_token == MMC_FLOATING_BUS) && (index != 0));

		 // Hopefully that zero is the datatoken
	if((index == 0) || (data_token != DATA_START_TOKEN))
	{
#ifdef SD_CARD_DEBUG_READ_FUNC
		DEBUG_Message("Return Error", __FUNCTION__, __FILE__, __LINE__);
#endif
		return ERROR;
	}
	else
	{
		if(buffer != NULL)
		{
			for(index = 0; index < gMediaSectorSize; index++)
			{
				buffer[index] = SPI_ReadByte();
			}

			crc_byte1 = SPI_ReadByte();
			crc_byte0 = SPI_ReadByte();

#ifdef SD_CARD_DEBUG_READ_FUNC
			printf("CRC_byte1 = %i\n", crc_byte1);
			printf("CRC_byte0 = %i\n", crc_byte0);
#endif
		}
	}

	SPI_WriteByte(0xFF);
	SD_DeSelect();

    return SUCCESS;
}

/**
  * @brief  Writes Sector(s)
  * @param  *buffer: Data to be written
  * @param  sector_addr: Sector address (LBA)
  * @retval OPStatus: Operation result
  */

ErrorStatus MDD_SDSPI_SectorWrite(DWORD sector_addr, BYTE* buffer, BYTE allowWriteToZero)
{
	WORD index;
	//WORD delay;
	MMC_RESPONSE response;
	//BYTE data_token;
	BYTE crc_byte0, crc_byte1;
	DWORD new_addr;
	BYTE data_response = 0;

	if(gSDMode == SD_MODE_NORMAL)
	{
		new_addr = sector_addr << 9;
	}
	else
	{
		new_addr = sector_addr;
	}

	SD_Select();


	printf("Write on block\n");
	response = SendMMCCmd(WRITE_SINGLE_BLOCK, new_addr);

	if(response.r1._byte != 0x00)
	{
		return ERROR;
	}

	SPI_WriteByte(DATA_START_TOKEN);

	for(index = 0; index < gMediaSectorSize; index++)
	{
		SPI_WriteByte(buffer[index]);
	}

	crc_byte1 = SPI_ReadByte();
	crc_byte0 = SPI_ReadByte();

#ifdef SD_CARD_DEBUG
		printf("SD Card Write CRC_byte1 = %i\n", crc_byte1);
		printf("SD Card Write CRC_byte0 = %i\n", crc_byte0);
#endif
	data_response = SPI_ReadByte();

#ifdef SD_CARD_DEBUG
		printf("SD Card Data Write response after send block %i\n", data_response);
#endif

	if((data_response & 0x0F) != DATA_ACCEPTED)
	{
		return ERROR;
	}

	while(SPI_ReadByte() == 0x00);


	SPI_WriteByte(0xFF);
	SD_DeSelect();


    return SUCCESS;
}

/*********************************************************
  Function:
    WORD MDD_SDSPI_ReadSectorSize (void)
  Summary:
    Determines the current sector size on the SD card
  Conditions:
    MDD_MediaInitialize() is complete
  Input:
    None
  Return:
    The size of the sectors for the physical media
  Side Effects:
    None.
  Description:
    The MDD_SDSPI_ReadSectorSize function is used by the
    USB mass storage class to return the card's sector
    size to the PC on request.
  Remarks:
    None
  *********************************************************/

WORD MDD_SDSPI_ReadSectorSize(void)
{
	return gMediaSectorSize;
}


/*********************************************************
  Function:
    DWORD MDD_SDSPI_ReadCapacity (void)
  Summary:
    Determines the current capacity of the SD card
  Conditions:
    MDD_MediaInitialize() is complete
  Input:
    None
  Return:
    The capacity of the device
  Side Effects:
    None.
  Description:
    The MDD_SDSPI_ReadCapacity function is used by the
    USB mass storage class to return the total number
    of sectors on the card.
  Remarks:
    None
  *********************************************************/
DWORD MDD_SDSPI_ReadCapacity(void)
{
    return (MDD_SDSPI_finalLBA);
}

BYTE MDD_SDSPI_ShutdownMedia(void)
{
   SD_DeSelect();

    return 0;
}

/*****************************************************************************
  Function:
    BYTE MDD_SDSPI_WriteProtectState
  Summary:
    Indicates whether the card is write-protected.
  Conditions:
    The MDD_WriteProtectState function pointer must be pointing to this function.
  Input:
    None.
  Return Values:
    TRUE -  The card is write-protected
    FALSE - The card is not write-protected
  Side Effects:
    None.
  Description:
    The MDD_SDSPI_WriteProtectState function will determine if the SD card is
    write protected by checking the electrical signal that corresponds to the
    physical write-protect switch.
  Remarks:
    None
  ***************************************************************************************/

BYTE MDD_SDSPI_WriteProtectState(void)
{
    return 0;
}
