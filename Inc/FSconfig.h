#ifndef FSCONFIG_H_
#define FSCONFIG_H_

#define FS_MAX_FILES_OPEN 	3
#define MEDIA_SECTOR_SIZE 	512
#define ALLOW_FILESEARCH
#define ALLOW_WRITES
#define ALLOW_FORMATS
#define ALLOW_DIRS
#define SUPPORT_FAT32




/************************************************************************/
// Set this preprocessor option to '1' to use dynamic FSFILE object allocation.  It will
// be necessary to allocate a heap when dynamically allocating FSFILE objects.
// Set this option to '0' to use static FSFILE object allocation.
/************************************************************************/


#if 0
	#define FS_DYNAMIC_MEM
	#define FS_malloc	malloc
	#define FS_free		free
#endif




// Description: Function pointer to the Media Initialize Physical Layer function
#define MDD_MediaInitialize     MDD_SDSPI_MediaInitialize

// Description: Function pointer to the Media Detect Physical Layer function
#define MDD_MediaDetect         MDD_SDSPI_MediaDetect

// Description: Function pointer to the Sector Read Physical Layer function
#define MDD_SectorRead          MDD_SDSPI_SectorRead

// Description: Function pointer to the Sector Write Physical Layer function
#define MDD_SectorWrite         MDD_SDSPI_SectorWrite

// Description: Function pointer to the I/O Initialization Physical Layer function
#define MDD_InitIO              MDD_SDSPI_InitIO

// Description: Function pointer to the Media Shutdown Physical Layer function
#define MDD_ShutdownMedia       MDD_SDSPI_ShutdownMedia

// Description: Function pointer to the Write Protect Check Physical Layer function
#define MDD_WriteProtectState   MDD_SDSPI_WriteProtectState

// Description: Function pointer to the Read Capacity Physical Layer function
#define MDD_ReadCapacity        MDD_SDSPI_ReadCapacity

// Description: Function pointer to the Read Sector Size Physical Layer Function
#define MDD_ReadSectorSize      MDD_SDSPI_ReadSectorSize


#endif
