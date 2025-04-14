/*
*********************************************************************************************************
*
*	模块名称 :sdmmc的Fat文件系统和QSPI Flash模拟演示模块。
*	文件名称 : demo_sdmmc_fatfs.c
*	版    本 : V1.0
*	说    明 : 该例程移植FatFS文件系统（版本 R0.12c），演示如何创建文件、读取文件、创建目录和删除文件
*
*	修改记录 :
*		版本号   日期         作者        说明
*		V1.0    2020-12-31    jc    初始创建
*
*********************************************************************************************************
*/
#include <vxWorks.h>
#include <stdio.h>
#include <string.h>

#include "./ff.h"


#if 0  /* fatFS_v1*/

FIL fil;		/* File object */
FATFS fatfs;

static int g_vxbSdmmc_fs_mount = 0;

FRESULT scan_files (char* path)
{
    FRESULT res;
    FILINFO fno;
    DIR dir;
    int i;
	int idx = 0;

    res = f_opendir(&dir, path);
    if (res == FR_OK) 
	{
        i = strlen(path);
		
        for (;;) 
		{
            res = f_readdir(&dir, &fno);
			
            if (res != FR_OK || fno.fname[0] == 0) 
				break;
			
            if (fno.fattrib & AM_DIR) 
			{
                sprintf(&path[i], "/%s", fno.fname);
				
                res = scan_files(path);
				
                if (res != FR_OK) 
					break;
				
                path[i] = 0;
            } 
			else 
			{
				#if 1
                	printf("%s/%s \n", path, fno.fname);
                #else
					idx = 0;
	                while (path[idx] != '\0')
                	{
                		idx++;
                	}
					
					if (path[idx-1] == '/')
					{
						printf("%s%s \n", path, fno.fname);
					}
					else
					{
						printf("%s/%s \n", path, fno.fname);
					}
				#endif
            }
        }
    }
	
    res = f_closedir(&dir);

    return res;
}

void sd_fs_mount(void)
{
	FRESULT res_sd;
	UINT8 work[FF_MAX_SS*4] = {0};   /* Work area (larger is better for process time) */

	if (g_vxbSdmmc_fs_mount == 0)
	{
		g_vxbSdmmc_fs_mount = 1;

		res_sd = f_mount(&fatfs, "0:/", 0);
	
		/* 
		Format and create file system if no file in SD card 
		*/
		if (res_sd == FR_NO_FILESYSTEM)
		{
			TRACE_OUT(DEBUG_OUT, "> No file in SD card, will format first...\r\n");
			
			/* Format */
			res_sd = f_mkfs("0:", FM_FAT32, 0, work, sizeof(work));
			if (res_sd == FR_OK)
			{
				TRACE_OUT(DEBUG_OUT, "> SD card format succeed.\r\n");
				
				/* cancel mount after format */
				res_sd = f_mount(NULL, "0:", 0);
				
				/* remount */
				res_sd = f_mount(&fatfs, "0:", 0);
			}
			else
			{
				TRACE_OUT(DEBUG_OUT, "> SD card format failed.\r\n");
				return ERROR;
			}
		}
		else if (res_sd != FR_OK)
		{
			TRACE_OUT(DEBUG_OUT, "> SD card mount file system failed.(%d)\r\n", res_sd);
			return ERROR;
		}
		else
		{
			TRACE_OUT(DEBUG_OUT, "> File system mount succeed, begin to write/read test.\r\n");
		}
	}

	return;
}

void sdls(char * pPathName)
{
	/*FRESULT res_sd;*/
	/*UINT8 work[FF_MAX_SS*]; /* Work area (larger is better for process time) */
	
    int isDir = 0;
    int i = 0;
	
    FRESULT fr;     /* Return value */
    DIR dj;         /* Directory object */
    FILINFO fno;    /* File information */	

	/*
	mount sd FatFS
	*/
	sd_fs_mount();

	/*
	judge dir ?
	*/
	i = 0;
	do
	{
		if ((pPathName[i] == '/') || (pPathName[i] == ':'))
		{
			isDir = 1;
			break;
		}

		i++;
	} while (pPathName[i] != '\0');

	if (isDir == 1)
	{
		scan_files(pPathName);
	}
	else
	{	
	   /* fr = f_findfirst(&dj, &fno, "0:", "**);  /* Start to search for photo files */
	    fr = f_findfirst(&dj, &fno, "0:", pPathName);  /* Start to search for photo files */
		if (fr != FR_OK)
		{
			printf("empty:%d! \n", fr);
			/*f_closedir(&dj);*/
			return;
		}

	    while ((fr == FR_OK) && fno.fname[0])           /* Repeat while an item is found */
		{         
	        printf("%s \n", fno.fname);               /* Print the object name */
	        fr = f_findnext(&dj, &fno);               /* Search for next item */
	    }

	    /*f_closedir(&dj);*/
	}

	return;
}

void test_wr_sd_file(char* file_name)
{
	FRESULT res_sd;
	UINT bw;
	
	int i;
	u8 *src, *dst; 
	int dataLen;
	u8 fileName[64] = {0};

	src = (u8*)pAddr_FPS_AHB_SRAM_BASEADDR;
	dst = (u8*)(pAddr_FPS_AHB_SRAM_BASEADDR + 0x10000);
	dataLen = 512;

	for(i=0; i<dataLen; i++)
    {
    	*src = i + 'a';
		src++;
    }
	
	src = (u8*)pAddr_FPS_AHB_SRAM_BASEADDR;	
	sprintf(src, "%s", file_name);

	memset(dst, 0, dataLen * sizeof(u8));
	
	/*
	--------------------- Write test -----------------------
	*/
	
	/*
	mount sd FatFS
	*/	
	sd_fs_mount();
	
	/* 
	Open file or create file if no file in SD card 
	*/
	sprintf((char*)(&fileName[0]), "0:%s", file_name);
	
	/*res_sd = f_open(&fil, "0:test.txt", FA_CREATE_ALWAYS|FA_WRITE);*/
	res_sd = f_open(&fil, (char*)(&fileName[0]), (FA_CREATE_ALWAYS|FA_WRITE));	
	if (res_sd == FR_OK)
	{
		printf("> Open/Create file: %s succeed! \r\n", (char*)(&fileName[0]));
		
		/* 
		Write data to the file 
		*/
		res_sd = f_write(&fil, (void*)src, dataLen, &bw);
		
		if (res_sd == FR_OK)
		{
			printf("> File(%s) write succeed, the write data is: %d \r\n", (char*)(&fileName[0]), bw);
		}
		else
		{
			printf("> File(%s) write failed: (%d) \r\n", (char*)(&fileName[0]), res_sd);
		}
		
		/* close the file */
		res_sd = f_close(&fil);
		if (res_sd != FR_OK)
		{
			return;
		}
	}
	else
	{
		printf("> File(%s) create failed: (%d) \r\n", (char*)(&fileName[0]), res_sd);
	}

	return;
}

void test_rd_sd_file(char* file_name)
{
	FRESULT res_sd;
	UINT bw;
	
	int i;
	u8 *src, *dst; 
	int dataLen;
	u8 fileName[64] = {0};

	src = (u8*)pAddr_FPS_AHB_SRAM_BASEADDR;
	dst = (u8*)(pAddr_FPS_AHB_SRAM_BASEADDR + 0x10000);
	dataLen = 512;
	
	src = (u8*)pAddr_FPS_AHB_SRAM_BASEADDR;	
	sprintf(src, "%s", file_name);

	memset(dst, 0, dataLen * sizeof(u8));

	/*
	------------------ Read test --------------------------
	*/	
	
	/*
	mount sd FatFS
	*/	
	sd_fs_mount();
	
	sprintf((char*)(&fileName[0]), "0:%s", file_name);	
	res_sd = f_open(&fil, (char*)(&fileName[0]), (FA_OPEN_EXISTING|FA_READ));
	if (res_sd == FR_OK)
	{
		printf("> open file(%s) succeed. \r\n", (char*)(&fileName[0]));
		
		res_sd = f_read(&fil, (void*)dst, dataLen, &bw);
		
		if (res_sd == FR_OK)
		{
			printf("> File(%s) read succeed, the read data is: %d\r\n", (char*)(&fileName[0]), bw);
		}
		else
		{
			printf("> File(%s) read failed: (%d)\n", (char*)(&fileName[0]), res_sd);
		}
	}
	else
	{
		printf("> File(%s) open failed: (%d) \r\n", (char*)(&fileName[0]), res_sd);
	}

	for (i=0; i<dataLen; i++)
	{
		printf("%02X", dst[i]);

		if ((i+1)%8 == 0)
		{
			printf(" ");
		}
		
		if ((i+1)%32 == 0)
		{
			printf("\n");
		}
	}
	
	/* 
	close the file 
	*/
	res_sd = f_close(&fil);
	if (res_sd != FR_OK)
	{
		return;
	}

	return;
}

#endif


/*
sd fatFS
*/
#if 1  /* fatFS_v2*/

#define FATFS_SD      /* for sd card */
#undef FATFS_EMMC     /* for emmc chip*/


#ifdef FATFS_SD

FIL fil1;		/* File object */
FATFS fatfs1;

static int g_vxbSdmmc_fs_mount = 0;


#if 1

/*BYTE Buff[4096];	/* Working buffer */

/*FATFS FatFs[2];		 Filesystem object for each logical drive */
/*FIL File[2];		/* File object */

DIR Dir;			/* Directory object */
FILINFO Finfo;
DWORD AccSize;				/* Work register for fs command */
WORD AccFiles, AccDirs;

/*BYTE RtcOk;				/* RTC is available */
/*volatile UINT Timer;	 Performance timer (100Hz increment) */

FRESULT scan_files (char* path)
{
	DIR dirs;
	FRESULT fr;
	int i;

	fr = f_opendir(&dirs, path);
	
	if (fr == FR_OK) 
	{
		while (((fr = f_readdir(&dirs, &Finfo)) == FR_OK) && Finfo.fname[0]) 
		{
			if (Finfo.fattrib & AM_DIR) 
			{
				AccDirs++;
				i = strlen(path);
				
				path[i] = '/'; strcpy(path+i+1, Finfo.fname);
				
				fr = scan_files(path);
				
				path[i] = 0;
				
				if (fr != FR_OK) 
					break;
			} 
			else 
			{
				printf("%s/%s\n", path, Finfo.fname);
				
				AccFiles++;
				AccSize += Finfo.fsize;
			}
		}
	}

	return fr;
}

#endif


void sd_fs_mount(void)
{
	FRESULT res_sd;
	UINT8 work[FF_MAX_SS*4] = {0};   /* Work area (larger is better for process time) */
	
	/*
	VolToPart[0].pd	= 0; 
    VolToPart[0].pt	= 0;
	
	VolToPart[1].pd	= 1; 
    VolToPart[1].pt	= 1;
	*/

	if (g_vxbSdmmc_fs_mount == 0)
	{
		g_vxbSdmmc_fs_mount = 1;

		res_sd = f_mount(&fatfs1, "0:/", 0);
		/*res_sd = f_mount(&fatfs1, "sd:0/", 0);*/
	
		/* 
		Format and create file system if no file in SD card 
		*/
		if (res_sd == FR_NO_FILESYSTEM)
		{
			printf("> No file in SD card, will format first...\r\n");
			
			/* Format */
	#if 0  /* old version*/
			/*
			FRESULT f_mkfs (
				const TCHAR* path,	// Logical drive number 
				BYTE opt,			// Format option 
				DWORD au,			// Size of allocation unit (cluster) [byte] 
				void* work, 		// Pointer to working buffer (null: use heap memory) 
				UINT len			// Size of working buffer [byte] 
			)
			*/
			/*res_sd = f_mkfs("0:", FM_FAT32, 0, work, sizeof(work));*/
	#else		
			/*
			FRESULT f_mkfs (
				const TCHAR* path,		// Logical drive number 
				const MKFS_PARM* opt,	// Format options 
				void* work, 			// Pointer to working buffer (null: use heap memory) 
				UINT len				// Size of working buffer [byte] 
			)
			*/
			/*res_sd = f_mkfs("0:", FM_FAT32, work, sizeof(work));*/
	#endif
			if (res_sd == FR_OK)
			{
				printf("> SD card format succeed.\r\n");
				
				/* cancel mount after format */
				res_sd = f_mount(NULL, "0:", 0);
				
				/* remount */
				res_sd = f_mount(&fatfs1, "0:", 0);
			}
			else
			{
				printf("> SD card format failed.\r\n");
				return ERROR;
			}
		}
		else if (res_sd != FR_OK)
		{
			printf("> SD card mount file system failed.(%d)\r\n", res_sd);
			return ERROR;
		}
		else
		{
			/*printf("> File system mount succeed, begin to write/read test. \n");*/
			/*printf("> shell cmd: sdls \"0:\" \n");*/
		}
	}

	return;
}


/*
cmd example:
------------
sdls2 "0:"
sdls2 "0:/"
*/
void sdls2(char * pPathName)
{
    int isDir = 0;
    int i = 0;
	
    FRESULT fr;     /* Return value */
    DIR dj;         /* Directory object */
    FILINFO fno;    /* File information */

	if (pPathName == NULL)
	{
		printf("shell cmd: sdls \"0:\" or sdls \"0:/ \" \n");
	}

	/*
	mount sd FatFS
	*/
	sd_fs_mount();

	/*
	judge dir ?
	*/
	i = 0;
	do
	{
		if ((pPathName[i] == '/') || (pPathName[i] == ':'))
		{
			isDir = 1;
			break;
		}

		i++;
	} while (pPathName[i] != '\0');

	if (isDir == 1)
	{
		scan_files(pPathName);
	}
	else
	{	
	   /* fr = f_findfirst(&dj, &fno, "0:", "**);  /* Start to search for photo files */
	    fr = f_findfirst(&dj, &fno, "0:", pPathName);  /* Start to search for photo files */
		if (fr != FR_OK)
		{
			printf("empty:%d! \n", fr);
			
			f_closedir(&dj);
			return;
		}

	    while ((fr == FR_OK) && fno.fname[0])           /* Repeat while an item is found */
		{         
	        printf("%s \n", fno.fname);               /* Print the object name */
	        fr = f_findnext(&dj, &fno);               /* Search for next item */
	    }

	    f_closedir(&dj);
	}

	return;
}


UINT8* pAddr_FPS_AHB_SRAM_BASEADDR = NULL;

void test_wr_sd_file(char* file_name)
{
	FRESULT res_sd;
	UINT bw;
	
	int i;
	UINT8 *src, *dst; 
	int dataLen;
	UINT8 fileName[64] = {0};

	pAddr_FPS_AHB_SRAM_BASEADDR = (UINT8*)malloc(0x20000);

	src = (UINT8*)pAddr_FPS_AHB_SRAM_BASEADDR;
	dst = (UINT8*)(pAddr_FPS_AHB_SRAM_BASEADDR + 0x10000);
	dataLen = 512;

	for(i=0; i<dataLen; i++)
    {
    	*src = i + 'a';
		src++;
    }
	
	src = (UINT8*)pAddr_FPS_AHB_SRAM_BASEADDR;	
	sprintf(src, "%s", file_name);

	memset(dst, 0, dataLen * sizeof(UINT8));
	
	/*
	--------------------- Write test -----------------------
	*/
	
	/*
	mount sd FatFS
	*/	
	sd_fs_mount();
	
	/* 
	Open file or create file if no file in SD card 
	*/
	sprintf((char*)(&fileName[0]), "0:%s", file_name);
	
	/*res_sd = f_open(&fil, "0:test.txt", FA_CREATE_ALWAYS|FA_WRITE);*/
	res_sd = f_open(&fil1, (char*)(&fileName[0]), (FA_CREATE_ALWAYS|FA_WRITE));	
	if (res_sd == FR_OK)
	{
		printf("> Open/Create file: %s succeed! \r\n", (char*)(&fileName[0]));
		
		/* 
		Write data to the file 
		*/
		res_sd = f_write(&fil1, (void*)src, dataLen, &bw);
		
		if (res_sd == FR_OK)
		{
			printf("> File(%s) write succeed, the write data is: %d \r\n", (char*)(&fileName[0]), bw);
		}
		else
		{
			printf("> File(%s) write failed: (%d) \r\n", (char*)(&fileName[0]), res_sd);
		}
		
		/* close the file */
		res_sd = f_close(&fil1);
		if (res_sd != FR_OK)
		{
			return;
		}
	}
	else
	{
		printf("> File(%s) create failed: (%d) \r\n", (char*)(&fileName[0]), res_sd);
	}

	free(pAddr_FPS_AHB_SRAM_BASEADDR);

	return;
}

void test_rd_sd_file(char* file_name)
{
	FRESULT res_sd;
	UINT bw;
	
	int i;
	UINT8 *src, *dst; 
	int dataLen;
	UINT8 fileName[64] = {0};
	int open_file_ok = 0;
	
	/*pAddr_FPS_AHB_SRAM_BASEADDR = (UINT8*malloc(0x20000);*/
	pAddr_FPS_AHB_SRAM_BASEADDR = (UINT8*)malloc(0x00D00000);

	/*src = (UINT8*pAddr_FPS_AHB_SRAM_BASEADDR;*/
	
	/*dst = (UINT8*(pAddr_FPS_AHB_SRAM_BASEADDR + 0x10000);*/
	dst = (UINT8*)(pAddr_FPS_AHB_SRAM_BASEADDR);
	
	/*dataLen = 512;*/
	dataLen = 0xCB4500;
	
	/*src = (UINT8*pAddr_FPS_AHB_SRAM_BASEADDR;	*/
	/*sprintf(src, "%s", file_name);*/

	memset(dst, 0, dataLen * sizeof(UINT8));

	/*
	------------------ Read test --------------------------
	*/	
	
	/*
	mount sd FatFS
	*/	
	sd_fs_mount();
	
	sprintf((char*)(&fileName[0]), "0:%s", file_name);	
	res_sd = f_open(&fil1, (char*)(&fileName[0]), (FA_OPEN_EXISTING|FA_READ));
	if (res_sd == FR_OK)
	{
		printf("> open file(%s) succeed. \r\n", (char*)(&fileName[0]));
		
		res_sd = f_read(&fil1, (void*)dst, dataLen, &bw);
		
		if (res_sd == FR_OK)
		{
			open_file_ok = 1;
			printf("> File(%s) read succeed, the read data is: %d\r\n", (char*)(&fileName[0]), bw);
		}
		else
		{
			printf("> File(%s) read failed: (%d)\n", (char*)(&fileName[0]), res_sd);
		}
	}
	else
	{
		printf("> File(%s) open failed: (%d) \r\n", (char*)(&fileName[0]), res_sd);
	}

	if (open_file_ok == 1)
	{
		for (i=0; i<256; i++)
		{
			printf("%02X", dst[i]);

			if ((i+1)%8 == 0)
			{
				printf(" ");
			}
			
			if ((i+1)%32 == 0)
			{
				printf("\n");
			}
		}
		
		printf("\n");
		
		for (i=0xCB44BC-256; i<0xCB44BF; i++)
		{
			printf("%02X", dst[i]);

			if ((i+1)%8 == 0)
			{
				printf(" ");
			}
			
			if ((i+1)%32 == 0)
			{
				printf("\n");
			}
		}
	}
	
	free(pAddr_FPS_AHB_SRAM_BASEADDR);
	
	/* 
	close the file 
	*/
	res_sd = f_close(&fil1);
	if (res_sd != FR_OK)
	{
		return;
	}

	return;
}

#if 0  /* for test big fpga bit file*/
extern void devC_Map_ddr(void);

void test_rd_big_file(char* file_name)
{
	FRESULT res_sd;
	UINT bw;
	
	int i;
	UINT8 *src, *dst; 
	int dataLen;
	UINT8 fileName[64] = {0};

	int open_file_ok = 0;

	devC_Map_ddr();

	dst = (UINT8*)(0x30000000);  /* virt_addr: 0x3000_0000 -> 0x2000_0000(phy_addr)*/
	dataLen = 0x01000000;  /* 16M	*/

	memset(dst, 0, dataLen * sizeof(UINT8));

	/*
	------------------ Read test --------------------------
	*/	
	
	/*
	mount sd FatFS
	*/	
	sd_fs_mount();
	
	sprintf((char*)(&fileName[0]), "0:%s", file_name);	
	
	res_sd = f_open(&fil1, (char*)(&fileName[0]), (FA_OPEN_EXISTING|FA_READ));
	
	if (res_sd == FR_OK)
	{
		printf("> open file(%s) succeed. \r\n", (char*)(&fileName[0]));
		
		res_sd = f_read(&fil1, (void*)dst, dataLen, &bw);
		
		if (res_sd == FR_OK)
		{
			open_file_ok = 1;
			printf("> File(%s) read succeed, the read data is: %d\r\n", (char*)(&fileName[0]), bw);
		}
		else
		{
			printf("> File(%s) read failed: (%d)\n", (char*)(&fileName[0]), res_sd);
		}

	}
	else
	{
		printf("> File(%s) open failed: (%d) \r\n", (char*)(&fileName[0]), res_sd);
	}

	if (open_file_ok == 1)
	{
		for (i=0; i<256; i++)
		{
			printf("%02X", dst[i]);

			if ((i+1)%8 == 0)
			{
				printf(" ");
			}
			
			if ((i+1)%32 == 0)
			{
				printf("\n");
			}
		}
		
		printf("\n");
		
		for (i=0xCB44BC-256; i<0xCB44BF; i++)
		{
			printf("%02X", dst[i]);

			if ((i+1)%8 == 0)
			{
				printf(" ");
			}
			
			if ((i+1)%32 == 0)
			{
				printf("\n");
			}
		}
	}
	
	/*free(pAddr_FPS_AHB_SRAM_BASEADDR);*/
	
	/* 
	close the file 
	*/
	res_sd = f_close(&fil1);
	if (res_sd != FR_OK)
	{
		return;
	}

	return;
}
#endif

#endif

/*
emmc fatFS
*/
#ifdef FATFS_EMMC

FIL fil2;		/* File object */
FATFS fatfs2;

static int g_vxbSdmmc_fs_mount2 = 0;

FRESULT scan_files2 (char* path)
{
    FRESULT res;
    FILINFO fno;
    DIR dir;
    int i;
	int idx = 0;

    res = f_opendir(&dir, path);
    if (res == FR_OK) 
	{
        i = strlen(path);
		
        for (;;) 
		{
            res = f_readdir(&dir, &fno);
			
            if (res != FR_OK || fno.fname[0] == 0) 
				break;
			
            if (fno.fattrib & AM_DIR) 
			{
                sprintf(&path[i], "/%s", fno.fname);
				
                res = scan_files2(path);
				
                if (res != FR_OK) 
					break;
				
                path[i] = 0;
            } 
			else 
			{
				#if 1
                	printf("%s/%s \n", path, fno.fname);
                #else
					idx = 0;
	                while (path[idx] != '\0')
                	{
                		idx++;
                	}
					
					if (path[idx-1] == '/')
					{
						printf("%s%s \n", path, fno.fname);
					}
					else
					{
						printf("%s/%s \n", path, fno.fname);
					}
				#endif
            }
        }
    }
	
    res = f_closedir(&dir);

    return res;
}

void sd_fs_mount2(void)
{
	FRESULT res_sd;
	UINT8 work[FF_MAX_SS*4] = {0};   /* Work area (larger is better for process time) */
	
	/*
	VolToPart[0].pd	= 0; 
    VolToPart[0].pt	= 0;
	
	VolToPart[1].pd	= 1; 
    VolToPart[1].pt	= 1;
	*/

	if (g_vxbSdmmc_fs_mount2 == 0)
	{
		g_vxbSdmmc_fs_mount2 = 1;

		res_sd = f_mount(&fatfs2, "1:/", 0);
	
		/* 
		Format and create file system if no file in SD card 
		*/
		if (res_sd == FR_NO_FILESYSTEM)
		{
			printf("> No file in SD card, will format first...\r\n");
			
			/* Format */
			res_sd = f_mkfs("1:", FM_FAT32, 0, work, sizeof(work));
			if (res_sd == FR_OK)
			{
				printf("> SD card format succeed.\r\n");
				
				/* cancel mount after format */
				res_sd = f_mount(NULL, "1:", 0);
				
				/* remount */
				res_sd = f_mount(&fatfs2, "1:", 0);
			}
			else
			{
				printf("> SD card format failed.\r\n");
				return ERROR;
			}
		}
		else if (res_sd != FR_OK)
		{
			printf("> SD card mount file system failed.(%d)\r\n", res_sd);
			return ERROR;
		}
		else
		{
			printf("> File system mount2 succeed, begin to write/read test. \n");
			printf("> shell cmd: sdls2 \"1:\" \n");
		}
	}

	return;
}


/*
cmd example:
------------
sdls "0:"
sdls "0:/"
*/
void sdls2(char * pPathName)
{
    int isDir = 0;
    int i = 0;
	
    FRESULT fr;     /* Return value */
    DIR dj;         /* Directory object */
    FILINFO fno;    /* File information */

	if (pPathName == NULL)
	{
		printf("shell cmd: sdls2 \"1:\" or sdls \"1:/ \" \n");
	}

	/*
	mount sd FatFS
	*/
	sd_fs_mount2();

	/*
	judge dir ?
	*/
	i = 0;
	do
	{
		if ((pPathName[i] == '/') || (pPathName[i] == ':'))
		{
			isDir = 1;
			break;
		}

		i++;
	} while (pPathName[i] != '\0');

	if (isDir == 1)
	{
		scan_files2(pPathName);
	}
	else
	{	
	   /* fr = f_findfirst(&dj, &fno, "0:", "**);  /* Start to search for photo files */
	    fr = f_findfirst(&dj, &fno, "1:", pPathName);  /* Start to search for photo files */
		if (fr != FR_OK)
		{
			printf("empty:%d! \n", fr);
			/*f_closedir(&dj);*/
			return;
		}

	    while ((fr == FR_OK) && fno.fname[0])           /* Repeat while an item is found */
		{         
	        printf("%s \n", fno.fname);               /* Print the object name */
	        fr = f_findnext(&dj, &fno);               /* Search for next item */
	    }

	    /*f_closedir(&dj);*/
	}

	return;
}

UINT8* FPS_AHB_SRAM_BASEADDR2 = NULL;

void test_wr_sd_file2(char* file_name)
{
	FRESULT res_sd;
	UINT bw;
	
	int i;
	UINT8 *src, *dst; 
	int dataLen;
	UINT8 fileName[64] = {0};

	FPS_AHB_SRAM_BASEADDR2 = (UINT8*)malloc(0x20000);

	src = (UINT8*)FPS_AHB_SRAM_BASEADDR2;
	dst = (UINT8*)(FPS_AHB_SRAM_BASEADDR2 + 0x10000);
	dataLen = 512;

	for(i=0; i<dataLen; i++)
    {
    	*src = i + 'a';
		src++;
    }
	
	src = (UINT8*)FPS_AHB_SRAM_BASEADDR2;	
	sprintf(src, "%s", file_name);

	memset(dst, 0, dataLen * sizeof(UINT8));
	
	/*
	--------------------- Write test -----------------------
	*/
	
	/*
	mount sd FatFS
	*/	
	sd_fs_mount2();
	
	/* 
	Open file or create file if no file in SD card 
	*/
	sprintf((char*)(&fileName[0]), "1:%s", file_name);
	
	/*res_sd = f_open(&fil, "0:test.txt", FA_CREATE_ALWAYS|FA_WRITE);*/
	res_sd = f_open(&fil2, (char*)(&fileName[0]), (FA_CREATE_ALWAYS|FA_WRITE));	
	if (res_sd == FR_OK)
	{
		printf("> Open/Create file: %s succeed! \r\n", (char*)(&fileName[0]));
		
		/* 
		Write data to the file 
		*/
		res_sd = f_write(&fil2, (void*)src, dataLen, &bw);
		
		if (res_sd == FR_OK)
		{
			printf("> File(%s) write succeed, the write data is: %d \r\n", (char*)(&fileName[0]), bw);
		}
		else
		{
			printf("> File(%s) write failed: (%d) \r\n", (char*)(&fileName[0]), res_sd);
		}
		
		/* close the file */
		res_sd = f_close(&fil2);
		if (res_sd != FR_OK)
		{
			free(FPS_AHB_SRAM_BASEADDR2);
			return;
		}
	}
	else
	{
		printf("> File(%s) create failed: (%d) \r\n", (char*)(&fileName[0]), res_sd);
	}

	free(FPS_AHB_SRAM_BASEADDR2);

	return;
}

void test_rd_sd_file2(char* file_name)
{
	FRESULT res_sd;
	UINT bw;
	
	int i;
	UINT8 *src, *dst; 
	int dataLen;
	UINT8 fileName[64] = {0};
	
	FPS_AHB_SRAM_BASEADDR2 = (UINT8*)malloc(0x20000);

	src = (UINT8*)FPS_AHB_SRAM_BASEADDR2;
	dst = (UINT8*)(FPS_AHB_SRAM_BASEADDR2 + 0x10000);
	dataLen = 512;
	
	src = (UINT8*)FPS_AHB_SRAM_BASEADDR2;	
	sprintf(src, "%s", file_name);

	memset(dst, 0, dataLen * sizeof(UINT8));

	/*
	------------------ Read test --------------------------
	*/	
	
	/*
	mount sd FatFS
	*/	
	sd_fs_mount2();
	
	sprintf((char*)(&fileName[0]), "1:%s", file_name);	
	
	res_sd = f_open(&fil2, (char*)(&fileName[0]), (FA_OPEN_EXISTING|FA_READ));
	if (res_sd == FR_OK)
	{
		printf("> open file(%s) succeed. \r\n", (char*)(&fileName[0]));
		
		res_sd = f_read(&fil2, (void*)dst, dataLen, &bw);
		
		if (res_sd == FR_OK)
		{
			printf("> File(%s) read succeed, the read data is: %d\r\n", (char*)(&fileName[0]), bw);
		}
		else
		{
			printf("> File(%s) read failed: (%d)\n", (char*)(&fileName[0]), res_sd);
		}
	}
	else
	{
		printf("> File(%s) open failed: (%d) \r\n", (char*)(&fileName[0]), res_sd);
	}

	for (i=0; i<dataLen; i++)
	{
		printf("%02X", dst[i]);

		if ((i+1)%8 == 0)
		{
			printf(" ");
		}
		
		if ((i+1)%32 == 0)
		{
			printf("\n");
		}
	}
	
	free(FPS_AHB_SRAM_BASEADDR2);
	
	/* 
	close the file 
	*/
	res_sd = f_close(&fil2);
	if (res_sd != FR_OK)
	{
		return;
	}

	return;
}


void format2_fs(void)
{
	/* Initialize a brand-new disk drive mapped to physical drive 0 */
	
	BYTE work[FF_MAX_SS];		  /* Working buffer */
	
	/*LBA_t plist[] = {0x10000000, 100};  */
	DWORD plist[] = {0x10000000, 100};  
				 /* Divide the drive into two partitions */
				 /* {0x10000000, 100}; 256M sectors for 1st partition and left all for 2nd partition */
				 /* {20, 20, 20, 0}; 20% for 3 partitions each and remaing space is left not allocated */

	f_fdisk(1, plist, work);					/* Divide physical drive 0 */

	/*f_mkfs("0:", 0, work, sizeof work); /* Create FAT volume on the logical drive 0 */
	f_mkfs("1:", FM_FAT32, 0, work, sizeof(work)); /* Create FAT volume on the logical drive 1 */
	return;
}

#endif


#if 1

/*
cmd example:
------------
sdls "0:"
sdls "0:/"
sdls "0:/sd_45"
*/

/*
-> sdls "0:/"
 -att-      -time-       -size-     -name-   
 ------------------------------------------- 
 D-HS- 2020/04/13 21:58         0  SYSTEM~1
 ----A 2010/01/01 00:00       512  TEST1.TXT
 ----A 2010/01/01 00:00       512  TEST2.TXT
 D---- 2020/04/26 15:59         0  ZCU102
 D---- 2020/04/26 16:18         0  ZC706_~1
 D---- 2020/05/22 14:54         0  706
 D---- 2020/05/22 14:53         0  SD_45
 ----A 2020/01/09 13:43     16294  SYSTEM~1.DTB
 ----A 2020/03/12 16:13       174  UENV.TXT
 ----A 2018/11/20 13:55  22924246  U-ROOT~1
 ----A 2020/05/15 19:26   4162936  ZIMAGE
 ----A 2018/10/10 09:19   8407412  DWC_GM~1.PDF
 ----A 2020/05/27 10:18    228914  GMA_DMA.PDF
 ----A 2020/05/27 10:30    445713  BINDER~1.PDF
 ----A 2019/12/23 11:19    748672  BOOT.BIN
 ----A 2019/12/23 11:23  13321503  PL_TOP.BIT
 
  11 File(s),  50256888 bytes total
   5 Dir(s),    7517056KiB free
value = 21 = 0x15
*/

BYTE Buff[4096];	/* Working buffer */

void sdls(char* pPathName)
{
	char line[120];
	char *ptr, *ptr2;
	long p1, p2, p3;
	LBA_t lba;
	FRESULT fr;
	BYTE b1, *bp;
	UINT s1, s2, cnt, blen = sizeof Buff;
	DWORD ofs, sect = 0;
	static const char* const fst[] = {"", "FAT12", "FAT16", "FAT32", "exFAT"};
	FATFS *fs;

	/*
	mount sd FatFS
	*/
	sd_fs_mount();
	
	/*while (*tr == ' ') */
	/*		ptr++;*/
	ptr = pPathName;
					
	fr = f_opendir(&Dir, ptr);
	if (fr) 
	{
		printf("f_opendir fail(%d) \n", fr); 
		printf("shell cmd: sdls \"0:/\" or sdls \"0:\" \n");
		return;
	}
	
	p1 = s1 = s2 = 0;
	
	printf(" -att-      -time-       -size-     -name-   \n");
	printf(" ------------------------------------------- \n");
		  /* -att- ------time------ --size--- ---name--- 
		 	 D-HS- 2020/04/13 21:58         0  SYSTEM~1
		 	 */
	for (;;) 
	{
		fr = f_readdir(&Dir, &Finfo);
		
		if ((fr != FR_OK) || !Finfo.fname[0]) 
		{	
			break;
		}
		
		if (Finfo.fattrib & AM_DIR) 
		{
			s2++;
		} 
		else 
		{
			s1++; 
			p1 += Finfo.fsize;
		}		
		
		printf(" %c%c%c%c%c %u/%02u/%02u %02u:%02u %9lu  %s\n", 
					(Finfo.fattrib & AM_DIR) ? 'D' : '-',
					(Finfo.fattrib & AM_RDO) ? 'R' : '-',
					(Finfo.fattrib & AM_HID) ? 'H' : '-',
					(Finfo.fattrib & AM_SYS) ? 'S' : '-',
					(Finfo.fattrib & AM_ARC) ? 'A' : '-',
					(Finfo.fdate >> 9) + 1980, (Finfo.fdate >> 5) & 15, Finfo.fdate & 31,
					(Finfo.ftime >> 11), (Finfo.ftime >> 5) & 63,
					(DWORD)Finfo.fsize, 
					Finfo.fname);
	}
	
	printf(" \n");
	
	if (fr == FR_OK) 
	{
		printf("%4u File(s),%10lu bytes total\n%4u Dir(s)", s1, p1, s2);
		
		if (f_getfree(ptr, (DWORD*)&p1, &fs) == FR_OK) 
		{
			printf(", %10luKiB free\n", p1 * fs->csize / 2);
		}
	}
	
	if (fr) 
	{
		printf("f_readdir fail(%d) \n", fr); 		
	}

	return;
}

/*
cmd example:
------------
sdcd "0:/"
sdcd "0:/sd_45"
sdcd "sd_45"
*/
/* 
<pPathName> - Change current directory
*/
void sdcd(char* pPathName)
{
	char *ptr;
	FRESULT fr;

	/* fg <path> - Change current directory */
	/*while (*tr == ' ') */
	/*	ptr++;*/
	ptr = pPathName;
	
	fr = f_chdir(ptr);
	
	if (fr == FR_OK) 
	{
		printf("sdcd to: %s ok! \n", pPathName);
	}
	else
	{
		printf("sdcd to: %s fail! \n", pPathName);
	}

	return;
}


/* 
Show current dir path
*/
void sdpwd(void)
{
	char line[120];
	char *ptr;
	FRESULT fr;
	
	/* fq - Show current dir path */
	fr = f_getcwd(line, sizeof(line));
	
	if (fr == FR_OK) 
	{
		printf("%s\n", line);
	} 
	else
	{
		printf("sdpwd fail! \n");
	}

	return;
}

/*
sd disk_status show:
待完善...
*/
void sdds(void)
{
	char line[120];
	char *ptr, *ptr2;
	long p1, p2, p3;
	LBA_t lba;
	FRESULT fr;
	BYTE b1, *bp;
	UINT s1, s2, cnt, blen = sizeof Buff;
	DWORD ofs, sect = 0;
	static const char* const fst[] = {"", "FAT12", "FAT16", "FAT32", "exFAT"};
	FATFS *fs;
	int i = 0;
	
	/* ds <pd#> - Show disk status */
	/*if (!xatoi(&ptr, &p1)) */
	/*	break;*/
	p1 = 0;
	
	if (disk_ioctl((BYTE)p1, GET_SECTOR_COUNT, &lba) == RES_OK) 
	{
		printf("Drive size: %lu sectors \n", (DWORD)lba);
	}
	
	if (disk_ioctl((BYTE)p1, GET_BLOCK_SIZE, &p2) == RES_OK) 
	{
		printf("Erase block: %lu sectors \n", p2);
	}
	
	if (disk_ioctl((BYTE)p1, MMC_GET_TYPE, &b1) == RES_OK) 
	{
		printf("Card type: %u \n", b1);
	}
	
	if (disk_ioctl((BYTE)p1, MMC_GET_CSD, Buff) == RES_OK)
	{
		printf("CSD: 0x"); 
		for (i=0; i<16; i++)
		{
			printf("%02X", Buff[i]);
			if (i == 8)
			{
				printf("_");
			}
		}
		printf("\n");
	}
	
	if (disk_ioctl((BYTE)p1, MMC_GET_CID, Buff) == RES_OK) 
	{
		printf("CID: 0x"); 
		for (i=0; i<16; i++)
		{
			printf("%02X", Buff[i]);
			if (i == 8)
			{
				printf("_");
			}
		}
		printf("\n");
	}
	
	if (disk_ioctl((BYTE)p1, MMC_GET_OCR, Buff) == RES_OK) 
	{
		printf("OCR: 0x"); 
		for (i=0; i<4; i++)
		{
			printf("%02X", Buff[i]);
			if (i == 8)
			{
				printf("_");
			}
		}
		printf("\n");
	}
	
	if (disk_ioctl((BYTE)p1, MMC_GET_SDSTAT, Buff) == RES_OK)
	{
		printf("SD Status: ");
		for (s1 = 0; s1 < 64; s1 += 16) 
		{	
			/*put_dump(Buff+s1, s1, 16);			*/
			for (i=0; i<16; i++)
			{
				printf("%02X", Buff[i+s1]);
				if (i == 8)
				{
					printf("_");
				}
			}
			printf("\n");
		}
	}
	
	if (disk_ioctl((BYTE)p1, ATA_GET_MODEL, line) == RES_OK) 
	{
		line[40] = '\0'; 
		printf("Model: %s \n", line);
	}
	
	if (disk_ioctl((BYTE)p1, ATA_GET_SN, line) == RES_OK) 
	{
		line[20] = '\0'; 
		printf("S/N: %s \n", line);
	}

	return;
}

#endif
#endif

