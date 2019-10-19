#ifndef __USER_DISKIO_H
#define __USER_DISKIO_H

#ifdef __cplusplus
 extern "C" {
#endif 

#include "integer.h"
#include "stm32h7xx.h"
#include "FSDefs.h"

//#define SD_CARD_DEBUG

#define DATA_START_TOKEN_MULT_BLOCK	0xFC
#define STOP_TR_MULTI_BLOCK			0xFD

// Description: This macro represents an SD card start token
#define DATA_START_TOKEN            0xFE

// Description: This macro represents an SD card data accepted token
#define DATA_ACCEPTED               0x05

// Description: This macro indicates that the SD card expects to transmit or receive more data
#define MOREDATA    !0

// Description: This macro indicates that the SD card does not expect to transmit or receive more data
#define NODATA      0


#define SD_MODE_NORMAL  0
#define SD_MODE_HC      1

#define     cmdGO_IDLE_STATE        0
// Description: This macro defines the command code to initialize the SD card
#define     cmdSEND_OP_COND         1
// Description: This macro defined the command code to check for sector addressing
#define     cmdSEND_IF_COND         8
// Description: This macro defines the command code to get the Card Specific Data
#define     cmdSEND_CSD             9
// Description: This macro defines the command code to get the Card Information
#define     cmdSEND_CID             10
// Description: This macro defines the command code to stop transmission during a multi-block read
#define     cmdSTOP_TRANSMISSION    12
// Description: This macro defines the command code to get the card status information
#define     cmdSEND_STATUS          13
// Description: This macro defines the command code to set the block length of the card
#define     cmdSET_BLOCKLEN         16
// Description: This macro defines the command code to read one block from the card
#define     cmdREAD_SINGLE_BLOCK    17
// Description: This macro defines the command code to read multiple blocks from the card
#define     cmdREAD_MULTI_BLOCK     18
// Description: This macro defines the command code to write one block to the card
#define     cmdWRITE_SINGLE_BLOCK   24
// Description: This macro defines the command code to write multiple blocks to the card
#define     cmdWRITE_MULTI_BLOCK    25
// Description: This macro defines the command code to set the address of the start of an erase operation
#define     cmdTAG_SECTOR_START     32
// Description: This macro defines the command code to set the address of the end of an erase operation
#define     cmdTAG_SECTOR_END       33
// Description: This macro defines the command code to erase all previously selected blocks
#define     cmdERASE                38
// Description: This macro defines the command code to begin application specific command inputs
#define     cmdAPP_CMD              55
// Description: This macro defines the command code to get the OCR register information from the card
#define     cmdREAD_OCR             58
// Description: This macro defines the command code to disable CRC checking
#define     cmdCRC_ON_OFF           59

 // Description: This macro represents a floating SPI bus condition
 #define MMC_FLOATING_BUS    0xFF

 // Description: This macro represents a bad SD card response byte
 #define MMC_BAD_RESPONSE    MMC_FLOATING_BUS

// Description: Enumeration of different SD response types
typedef enum
{
	R1,     // R1 type response
	R1b,    // R1b type response
	R2,     // R2 type response
	R3,     // R3 type response
	R7      // R7 type response
}RESP;

// Summary: SD card command data structure
// Description: The typMMC_CMD structure is used to create a command table of information needed for each relevant SD command
typedef struct
{
	BYTE CmdCode;          // The command code
	BYTE sdCRC;              // The CRC value for that command
	RESP responsetype;       // The response type
	BYTE moredataexpected;   // Set to MOREDATA or NODATA, depending on whether more data is expected or not
}typMMC_CMD;



// Summary: The format of an R1 type response
// Description: This union represents different ways to access an SD card R1 type response packet.
typedef union
{
	BYTE _byte;                         // Byte-wise access
	// This structure allows bitwise access of the response
	struct
	{
		unsigned IN_IDLE_STATE:1;       // Card is in idle state
		unsigned ERASE_RESET:1;         // Erase reset flag
		unsigned ILLEGAL_CMD:1;         // Illegal command flag
		unsigned CRC_ERR:1;             // CRC error flag
		unsigned ERASE_SEQ_ERR:1;       // Erase sequence error flag
		unsigned ADDRESS_ERR:1;         // Address error flag
		unsigned PARAM_ERR:1;           // Parameter flag
		unsigned B7:1;                  // Unused bit 7
	};
}RESPONSE_1;

typedef union
{
	WORD _word;
	struct
	{
		BYTE      _byte0;
		BYTE      _byte1;
	};
	struct
	{
		unsigned IN_IDLE_STATE:1;
		unsigned ERASE_RESET:1;
		unsigned ILLEGAL_CMD:1;
		unsigned CRC_ERR:1;
		unsigned ERASE_SEQ_ERR:1;
		unsigned ADDRESS_ERR:1;
		unsigned PARAM_ERR:1;
		unsigned B7:1;
		unsigned CARD_IS_LOCKED:1;
		unsigned WP_ERASE_SKIP_LK_FAIL:1;
		unsigned ERROR:1;
		unsigned CC_ERROR:1;
		unsigned CARD_ECC_FAIL:1;
		unsigned WP_VIOLATION:1;
		unsigned ERASE_PARAM:1;
		unsigned OUTRANGE_CSD_OVERWRITE:1;
	};
}RESPONSE_2;

// Summary: The format of an R1 type response
// Description: This union represents different ways to access an SD card R1 type response packet.
typedef union
{
	struct
	{
		BYTE _byte;                         // Byte-wise access
		DWORD _returnVal;
	}bytewise;
	// This structure allows bitwise access of the response
	struct
	{
		struct
		{
			 unsigned IN_IDLE_STATE:1;       // Card is in idle state
			 unsigned ERASE_RESET:1;         // Erase reset flag
			 unsigned ILLEGAL_CMD:1;         // Illegal command flag
			 unsigned CRC_ERR:1;             // CRC error flag
			 unsigned ERASE_SEQ_ERR:1;       // Erase sequence error flag
			 unsigned ADDRESS_ERR:1;         // Address error flag
			 unsigned PARAM_ERR:1;           // Parameter flag
			 unsigned B7:1;                  // Unused bit 7
		}bits;
		DWORD _returnVal;
	}bitwise;
}RESPONSE_7;


// Summary: A union of responses from an SD card
// Description: The MMC_RESPONSE union represents any of the possible responses that an SD card can return after
//              being issued a command.
typedef union
{
	RESPONSE_1  r1;
	RESPONSE_2  r2;
	RESPONSE_7  r7;
}MMC_RESPONSE;

// Summary: A description of the card specific data register
// Description: This union represents different ways to access information in a packet with SD card CSD informaiton.  For more
//              information on the CSD register, consult an SD card user's manual.
typedef union
{
	struct
	{
		DWORD _u320;
		DWORD _u321;
		DWORD _u322;
		DWORD _u323;
	};
	struct
	{
		BYTE _byte[16];
	};
	struct
	{
		unsigned NOT_USED           :1;
		unsigned sdCRC              :7;
		unsigned ECC                :2;
		unsigned FILE_FORMAT        :2;
		unsigned TMP_WRITE_PROTECT  :1;
		unsigned PERM_WRITE_PROTECT :1;
		unsigned COPY               :1;
		unsigned FILE_FORMAT_GRP    :1;
		unsigned RESERVED_1         :5;
		unsigned WRITE_BL_PARTIAL   :1;
		unsigned WRITE_BL_LEN_L     :2;
		unsigned WRITE_BL_LEN_H     :2;
		unsigned R2W_FACTOR         :3;
		unsigned DEFAULT_ECC        :2;
		unsigned WP_GRP_ENABLE      :1;
		unsigned WP_GRP_SIZE        :5;
		unsigned ERASE_GRP_SIZE_L   :3;
		unsigned ERASE_GRP_SIZE_H   :2;
		unsigned SECTOR_SIZE        :5;
		unsigned C_SIZE_MULT_L      :1;
		unsigned C_SIZE_MULT_H      :2;
		unsigned VDD_W_CURR_MAX     :3;
		unsigned VDD_W_CUR_MIN      :3;
		unsigned VDD_R_CURR_MAX     :3;
		unsigned VDD_R_CURR_MIN     :3;
		unsigned C_SIZE_L           :2;
		unsigned C_SIZE_H           :8;
		unsigned C_SIZE_U           :2;
		unsigned RESERVED_2         :2;
		unsigned DSR_IMP            :1;
		unsigned READ_BLK_MISALIGN  :1;
		unsigned WRITE_BLK_MISALIGN :1;
		unsigned READ_BL_PARTIAL    :1;
		unsigned READ_BL_LEN        :4;
		unsigned CCC_L              :4;
		unsigned CCC_H              :8;
		unsigned TRAN_SPEED         :8;
		unsigned NSAC               :8;
		unsigned TAAC               :8;
		unsigned RESERVED_3         :2;
		unsigned SPEC_VERS          :4;
		unsigned CSD_STRUCTURE      :2;
	};
}CSD;


// Summary: A description of the card information register
// Description: This union represents different ways to access information in a packet with SD card CID register informaiton.  For more
//              information on the CID register, consult an SD card user's manual.
typedef union
{
	struct
	{
		DWORD _u320;
		DWORD _u321;
		DWORD _u322;
		DWORD _u323;
	};
	struct
	{
		BYTE _byte[16];
	};
	struct
	{
		unsigned    NOT_USED	:1;
		unsigned    sdCRC		:7;
		unsigned    MDT			:8;
		DWORD       PSN;
		unsigned    PRV			:8;
		char        PNM[6];
		WORD        OID;
		unsigned    MID			:8;
	};
}CID;

typedef enum
{
    GO_IDLE_STATE,
    SEND_OP_COND,
    SEND_IF_COND,
    SEND_CSD,
    SEND_CID,
    STOP_TRANSMISSION,
    SEND_STATUS,
    SET_BLOCKLEN,
    READ_SINGLE_BLOCK,
    READ_MULTI_BLOCK,
    WRITE_SINGLE_BLOCK,
    WRITE_MULTI_BLOCK,
    TAG_SECTOR_START,
    TAG_SECTOR_END,
    ERASE,
    APP_CMD,
    READ_OCR,
    CRC_ON_OFF
}SdCMD;

/*****************************************************************************/
/*                                 Prototypes                                */
/*****************************************************************************/

DWORD MDD_SDSPI_ReadCapacity(void);
WORD MDD_SDSPI_ReadSectorSize(void);

BOOL MDD_SDSPI_MediaDetect(void);
MEDIA_INFORMATION* MDD_SDSPI_MediaInitialize(void);
ErrorStatus MDD_SDSPI_SectorRead(DWORD sector_addr, BYTE* buffer);
ErrorStatus MDD_SDSPI_SectorWrite(DWORD sector_addr, BYTE* buffer, BYTE allowWriteToZero);
BYTE MDD_SDSPI_WriteProtectState(void);
BYTE MDD_SDSPI_ShutdownMedia(void);

extern BYTE ReadByte(BYTE* pBuffer, WORD index);
extern WORD ReadWord(BYTE* pBuffer, WORD index);
extern DWORD ReadDWord(BYTE* pBuffer, WORD index);


#ifdef __cplusplus
}
#endif

#endif
