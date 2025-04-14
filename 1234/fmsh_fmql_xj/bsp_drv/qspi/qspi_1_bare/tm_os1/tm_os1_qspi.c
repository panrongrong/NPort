/*
tm_os1_qspi.c -  the tianmai os1 funcitons interface for qspiflash
*/

/*
 * Copyright (c) xxx, Corp..
 *
 * The right to copy, distribute, modify or otherwise make use
 * of this software may be licensed only pursuant to the terms
 * of an applicable QingSong license agreement.
 */

/*
DESCRIPTION:

This file contains the functions define: init, readid, erase, read, write etc.
*/

/*
modification history:
--------------------
01a,24Jun21, jc  create
*/

#include "./qspips_v1_0/fmsh_qspips.h" 
#include "./qspips_v1_0/fmsh_qspips_sw.h" 

extern u32 FQspiPs_ReadId(FQspiPs_T* qspi, void* id);
extern int FQspiPs_FastSendBytes(FQspiPs_T* qspiPtr, u32 offset, u32 byteCount, u8* sendBuffer);
extern int FQspiPs_FastRecvBytes(FQspiPs_T* qspiPtr, u32 offset, u32 byteCount, u8* recvBuffer);

/*
qspi_0
*/
static FQspiPs_T g_qspi_0_os1;
static FQspiPs_T * g_pQspi0_os1 = (FQspiPs_T *)(&g_qspi_0_os1.version);

/*
qspi_1
*/
#if 0
static FQspiPs_T g_qspi_1_os1;
static FQspiPs_T* g_pQspi1_os1 = (FQspiPs_T*)(&g_qspi_1_os1);
#endif

/*
tianmain os1 interface qspiflash sdk bare-metal code
*/


void qspiFlash_readId(void)
{	
	UINT32 flash_id[2] = 0;

	FQspiPs_ReadId(g_pQspi0_os1, &(flash_id[0]));
	printf("flash_id:0x%08X_%08X\n", flash_id[0], flash_id[1]);
	return;
}

/******************************************************************************
* @description: indirect mode without dma(x1, x2, x4) verify
*
* return    0x1:transfer error
*           0x2:data compare error
*
******************************************************************************/

extern int FQspiPs_Initialize(FQspiPs_T* qspiPtr, u16 deviceId);
extern int FQspiPs_InitHw(FQspiPs_T* qspiPtr, FQspiPs_Caps* capsPtr);
/*
QSPI_CAPS(indac_test) = 
{
    .hasIntr = 1,
    .hasDma = 0,
    .hasXIP = 0,
    .txEmptyLvl = 32,
    .rxFullLvl = 480,
    .rdMode = QOR_CMD,
    .baudRate = 4, 
    .sampleDelay = 0,
    .addressBytes = 3,
};
*/

FQspiPs_Caps flash_caps = 
{
	0,       /* hasIntr: 1-irq, 0-poll */
	0,       /* hasDma*/
	0,       /* hasXIP*/
	32,      /* txEmptyLvl*/
	480,     /* rxFullLvl*/
	QOR_CMD, /* rdMode*/
	4,       /* baudRate*/
	0,       /* sampleDelay*/
	3        /* addressBytes*/
};


STATUS qspi_init(void)
{	
	u8 status = 0;
	FQspiPs_T * pQspi = g_pQspi0_os1;	
	
	u16 deviceId = FPS_QSPI0_DEVICE_ID;	 /* qspi_0*/
	
	FQspiPs_Initialize(pQspi, deviceId); 
	
    /* qspi init*/
    FQspiPs_InitHw(pQspi, &flash_caps);
	
    status = FQspiFlash_GetStatus1(pQspi);	
    if (status & 0x1c)
        FQspiPs_UnlockFlash(pQspi);

	return OK;
}

STATUS qspiFlashWrite(UINT32 addr, char *buf, UINT32 byteLen)
{	
	FQspiPs_T * pQspi = g_pQspi0_os1;		
	return FQspiPs_FastSendBytes(pQspi, addr, byteLen, buf);

}

STATUS qspiFlashRead(UINT32 offset, char * readBuffer, UINT32 byteLen)
{	
	FQspiPs_T * pQspi = g_pQspi0_os1;		
	return FQspiPs_FastRecvBytes(pQspi, offset, byteLen, readBuffer);
}

int FQspiFlash_SectorErase_OS1(UINT32 start_offset)
{
	FQspiPs_T * pQspi = g_pQspi0_os1;		
	return FQspiFlash_SectorErase(pQspi, start_offset, SE_CMD);
}

/**/
/* for test */
/**/
#if  1

UINT8 RcvBuf2_os1[QSPIFLASH_SECTOR_64K] = {0};
UINT8 SndBuf2_os1[QSPIFLASH_SECTOR_64K] = {0};
int g_qspi_init_flag = 0;
void test_qspi_rd_sect_os1(int sect_idx)
{
	FQspiPs_T  * pQspi = g_pQspi0_os1;
	UINT8* pBuf8 = (UINT8 *)(&RcvBuf2_os1[0]);
	
	int j = 0, byteCnt = 0;
	int offset = 0;
	
	int phy_idx = 0;

	if (g_qspi_init_flag == 0)
	{
		qspi_init();
		g_qspi_init_flag = 1;
	}	
	
	byteCnt = pQspi->sectorSize;
	offset = sect_idx * pQspi->sectorSize;

	/**/
	/* read one sector*/
	/**/
	qspiFlashRead(offset, pBuf8, byteCnt);

	printf("-----read(sect-%d):%dKB----\n", sect_idx, byteCnt);	
	
	for (j=0; j<512; j++)
	{
		printf("%02X", pBuf8[j]);
		
		if ((j+1)%4 == 0)
		{
			printf(" ");
		}
		
		if ((j+1)%16 == 0)
		{
			printf(" \n");
		}
		
		if ((j+1)%256 == 0)
		{
			printf(" \n");
		}
	}

	printf(" ... ... ... ... \n\n");
	
	for (j=(byteCnt-512); j<byteCnt; j++)
	{
		printf("%02X", pBuf8[j]);
		
		if ((j+1)%4 == 0)
		{
			printf(" ");
		}
		
		if ((j+1)%16 == 0)
		{
			printf(" \n");
		}
		
		if ((j+1)%256 == 0)
		{
			printf(" \n");
		}
	}
	printf("------------\n");

	return;
}


int g_test_qspi2_os1 = 0;
void test_qspi_wr_sect_os1(int sect_idx)
{
	FQspiPs_T  * pQspi = g_pQspi0_os1;	
	UINT8 * pBuf8 = (UINT8 *)(&SndBuf2_os1[0]);
	
	int i = 0, j = 0, byteCnt = 0;
	int offset = 0;
	
	if (g_qspi_init_flag == 0)
	{
		qspi_init();
		g_qspi_init_flag = 1;
	}	
	
	byteCnt = pQspi->sectorSize;	
	offset = sect_idx * pQspi->sectorSize;
	
	for (i=0; i<byteCnt; i++)
	{
		pBuf8[i] = g_test_qspi2_os1 + i;
	}
	g_test_qspi2_os1++;

	/* head*/
	*((UINT32*)(&pBuf8[0])) = 0xabcd1234;
	*((UINT32*)(&pBuf8[4])) = sect_idx;
	*((UINT32*)(&pBuf8[8])) = sect_idx;
	*((UINT32*)(&pBuf8[0xC])) = sect_idx;

	/* tail*/
	*((UINT32*)(&pBuf8[byteCnt-0x10])) = sect_idx;
	*((UINT32*)(&pBuf8[byteCnt-0xC])) = sect_idx;
	*((UINT32*)(&pBuf8[byteCnt-8])) = sect_idx;
	*((UINT32*)(&pBuf8[byteCnt-4])) = 0xcdef6789;	
	
	/**/
	/* write one sector*/
	/**/
	qspiFlashWrite(offset, pBuf8, byteCnt);
	
	printf("-----write(sect-%d):%dKB----\n", sect_idx, byteCnt);	
	
	for (j=0; j<512; j++)
	{
		printf("%02X", pBuf8[j]);
		
		if ((j+1)%4 == 0)
		{
			printf(" ");
		}
		
		if ((j+1)%16 == 0)
		{
			printf(" \n");
		}
		
		if ((j+1)%256 == 0)
		{
			printf(" \n");
		}
	}

	printf(" ... ... ... ... \n\n");
	
	for (j=(byteCnt-512); j<byteCnt; j++)
	{
		printf("%02X", pBuf8[j]);
		
		if ((j+1)%4 == 0)
		{
			printf(" ");
		}
		
		if ((j+1)%16 == 0)
		{
			printf(" \n");
		}
		
		if ((j+1)%256 == 0)
		{
			printf(" \n");
		}
	}
	printf("------------\n");
	
	return;
}

void test_qspi_erase_sect_os1(int sect_idx)
{
	FQspiPs_T  * pQspi = g_pQspi0_os1;	
	int ret = 0;
	UINT32 start_offset = 0;
	
	
	if (g_qspi_init_flag == 0)
	{
		qspi_init();
		g_qspi_init_flag = 1;
	}	
	
	start_offset = sect_idx * pQspi->sectorSize;
	
	/**/
	/* erase one sector*/
	/**/
	ret = FQspiFlash_SectorErase_OS1(start_offset);
	
    if (ret == FMSH_FAILURE)
    {
    	printf("ctrl_0: qspiflash erase sect: %d fail! \n", sect_idx);
    }
	else
	{
		printf("ctrl_0: qspiflash erase sect: %d ok! \n", sect_idx);
	}
	
	return;
}
#endif

