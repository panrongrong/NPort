/*
*********************************************************************************************************
*
*	模块名称 :QSPI的Fat文件系统和QSPI Flash模拟演示模块。
*	文件名称 : demo_qspi_fatfs.c
*	版    本 : V1.0
*	说    明 : 该例程移植FatFS文件系统（版本 R0.12c），演示如何创建文件、读取文件、创建目录和删除文件
*			   并测试了文件读写速度.支持以下8个功能，用户通过电脑端串口软件发送数字给开发板即可:
*              1 - 显示根目录下的文件列表
*              2 - 创建一个新文件armfly.txt
*              3 - 读armfly.txt文件的内容
*
*	修改记录 :
*		版本号   日期         作者        说明
*		V1.0    2018-12-12   Eric2013    正式发布
*
*	Copyright (C), 2018-2030, 安富莱电子 www.armfly.com
*
*********************************************************************************************************
*/
/*#include "bsp.h"*/
#include "ff.h"				/* FatFS文件系统模块*/
#include "ff_gen_drv.h"
/*#include "sd_diskio_dma.h"*/
/*#include "usbd_usr.h"*/


/* 用于测试读写速度 */
#define TEST_FILE_LEN			(2*1024*1024)	/* 用于测试的文件长度 */
#define BUF_SIZE				(4*1024)		/* 每次读写SD卡的最大数据长度 */

/* 仅允许本文件内调用的函数声明 */
void DispMenu(void);
void FileFormat(char *_path);
void ViewRootDir(char *_path);
void CreateNewFile(char *_path);
void ReadFileData(char *_path);

/* 变量 */
FATFS fs;
FIL file;
BYTE work[4096];
char FsReadBuf[1024];
char FsWriteBuf[1024] = {"qpsi FatFS Write Demo \r\n www.fmsh.com.cn \r\n"};
uint8_t g_TestBuf[BUF_SIZE];


DIR DirInf;
FILINFO FileInf;
char DiskPathSPI[4]; /* SD卡逻辑驱动路径，比盘符0，就是"0:/" */
char DiskPathSD[4]; /* SD卡逻辑驱动路径，比盘符0，就是"0:/" */

/* FatFs API的返回值 */
static const char * FR_Table[]= 
{
	"FR_OK：成功",				                             /* (0) Succeeded */
	"FR_DISK_ERR：底层硬件错误",			                 /* (1) A hard error occurred in the low level disk I/O layer */
	"FR_INT_ERR：断言失败",				                     /* (2) Assertion failed */
	"FR_NOT_READY：物理驱动没有工作",			             /* (3) The physical drive cannot work */
	"FR_NO_FILE：文件不存在",				                 /* (4) Could not find the file */
	"FR_NO_PATH：路径不存在",				                 /* (5) Could not find the path */
	"FR_INVALID_NAME：无效文件名",		                     /* (6) The path name format is invalid */
	"FR_DENIED：由于禁止访问或者目录已满访问被拒绝",         /* (7) Access denied due to prohibited access or directory full */
	"FR_EXIST：文件已经存在",			                     /* (8) Access denied due to prohibited access */
	"FR_INVALID_OBJECT：文件或者目录对象无效",		         /* (9) The file/directory object is invalid */
	"FR_WRITE_PROTECTED：物理驱动被写保护",		             /* (10) The physical drive is write protected */
	"FR_INVALID_DRIVE：逻辑驱动号无效",		                 /* (11) The logical drive number is invalid */
	"FR_NOT_ENABLED：卷中无工作区",			                 /* (12) The volume has no work area */
	"FR_NO_FILESYSTEM：没有有效的FAT卷",		             /* (13) There is no valid FAT volume */
	"FR_MKFS_ABORTED：由于参数错误f_mkfs()被终止",	         /* (14) The f_mkfs() aborted due to any parameter error */
	"FR_TIMEOUT：在规定的时间内无法获得访问卷的许可",		 /* (15) Could not get a grant to access the volume within defined period */
	"FR_LOCKED：由于文件共享策略操作被拒绝",				 /* (16) The operation is rejected according to the file sharing policy */
	"FR_NOT_ENOUGH_CORE：无法分配长文件名工作区",		     /* (17) LFN working buffer could not be allocated */
	"FR_TOO_MANY_OPEN_FILES：当前打开的文件数大于_FS_SHARE", /* (18) Number of open files > _FS_SHARE */
	"FR_INVALID_PARAMETER：参数无效"	                     /* (19) Given parameter is invalid */
};

extern const Diskio_drvTypeDef  SPIFlash_Driver;
/*
*********************************************************************************************************
*	函 数 名: DemoFatFS
*	功能说明: FatFS文件系统演示主程序
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/

#if 1  /* qspi-flash*/
int g_FATFS_LinkDriver_ok = 0;

void demoFatFS_spi(int cmd_in)
{
	uint8_t cmd = cmd_in;

	if (g_FATFS_LinkDriver_ok == 0)
	{
		/* 打印命令列表，用户可以通过串口操作指令 */
		DispMenu();
		
		/* 注册SD卡驱动 */
		FATFS_LinkDriver(&SPIFlash_Driver, DiskPathSPI);
		
		g_FATFS_LinkDriver_ok = 1;
	}	
		
	switch (cmd)
	{
		case 0:
			printf(" [ 0 - FileFormat QSPI Flash ] \r\n");
			FileFormat(DiskPathSPI);
			break;
		
		case 1:
			printf(" [ 1 - ViewRootDir  QSPI Flash ] \r\n");
			ViewRootDir(DiskPathSPI);	
			break;

		case 2:
			printf(" [ 2 - CreateNewFile  QSPI Flash ] \r\n");
			CreateNewFile(DiskPathSPI);	
			break;

		case 3:
			printf(" [ 3 - ReadFileData  QSPI Flash ] \r\n");
			ReadFileData(DiskPathSPI);	
			break;
		
		default:
			DispMenu();
			break;
	}

	printf("\r\n\r\n");
	
	return;
}

#endif


#if 0  /* sd */

int g_FATFS_LinkDriver_ok2 = 0;

void demoFatFS_sd(int cmd_in)
{
	uint8_t cmd = cmd_in;

	if (g_FATFS_LinkDriver_ok2 == 0)
	{
		/* 打印命令列表，用户可以通过串口操作指令 */
		DispMenu();
		
		/* 注册SD卡驱动 */		
		FATFS_LinkDriver(&SD_Driver, DiskPathSD);
		
		g_FATFS_LinkDriver_ok2 = 1;
	}	
		
	switch (cmd)
	{
		case 0:
			printf(" [ 0 - no need FileFormat SD ] \r\n");
			/*FileFormat(DiskPathSPI);*/
			break;
		
		case 1:
			printf(" [ 1 - ViewRootDir  SD ] \r\n");
			ViewRootDir(DiskPathSD);				
			break;

		case 2:
			printf(" [ 2 - CreateNewFile  SD ] \r\n");
			CreateNewFile(DiskPathSD);	
			break;

		case 3:
			printf(" [ 3 - ReadFileData  SD ] \r\n");
			ReadFileData(DiskPathSD);					
			break;
		
		default:
			DispMenu();
			break;
	}

	printf("\r\n\r\n");
	
	return;
}
#endif

/*
*********************************************************************************************************
*	函 数 名: DispMenu
*	功能说明: 显示操作提示菜单
*	形    参：无
*	返 回 值: 无
*********************************************************************************************************
*/
void DispMenu(void)
{
	printf("\r\n------------------------------------------------\r\n");
	printf("首次使用必须使用命令0进行格式化QSPI Flash！！！，请选择操作命令:\r\n");
	printf("0 - QSPI Flash & SD卡文件系统格式化\r\n");
	printf("1 - 显示QSPI Flash & SD卡根目录下的文件列表\r\n");
	printf("2 - 创建一个新文件fmsh.txt\r\n");
	printf("3 - 读fmsh.txt文件的内容\r\n");
}

/*
*********************************************************************************************************
*	函 数 名: FileFormat
*	功能说明: 文件系统格式化
*	形    参：无
*	返 回 值: 无
*********************************************************************************************************
*/
void FileFormat(char *_path)
{
	/* 本函数使用的局部变量占用较多，请修改启动文件，保证堆栈空间够用 */
	FRESULT result;
	FATFS fs;

	/* 挂载文件系统 */
	result = f_mount(&fs, _path, 0);	
	if (result != FR_OK)
	{
		printf("挂载文件系统失败 (%s)\r\n", FR_Table[result]);
	}
	else
	{
		printf("挂载文件系统成功 (%s)\r\n", FR_Table[result]);
	}
	
	/* 第一次使用必须进行格式化 */
	/*result = f_mkfs("", FM_ANY, 0, work, sizeof work);  // error*/
	result = f_mkfs("0:", FM_FAT, 0, work, sizeof work);  /* only FM_FAT ok*/
	
	if (result != FR_OK)
	{
		printf("格式化失败 (%s)\r\n", FR_Table[result]);
	}
	else
	{
		printf("格式化成功 (%s)\r\n", FR_Table[result]);
	}

	/* 卸载文件系统 */
	result  = f_mount(NULL, _path, 0);
	if (result != FR_OK)
	{
		printf("卸载文件系统失败 (%s)\r\n", FR_Table[result]);
	}
	else
	{
		printf("卸载文件系统成功 (%s)\r\n", FR_Table[result]);
	}
	/*
	*/

	return;
}

/*
*********************************************************************************************************
*	函 数 名: ViewRootDir
*	功能说明: 显示SD卡根目录下的文件名
*	形    参：无
*	返 回 值: 无
*********************************************************************************************************
*/
void ViewRootDir(char *_path)
{
	FRESULT result;
	uint32_t cnt = 0;
	FILINFO fno;
	
	
 	/* 挂载文件系统 */
	result = f_mount(&fs, _path, 0);	/* Mount a logical drive */
	if (result != FR_OK)
	{
		printf("挂载文件系统失败 (%s)\r\n", FR_Table[result]);
	}

	/* 打开根文件夹 */
	result = f_opendir(&DirInf, _path); /* 如果不带参数，则从当前目录开始 */
	if (result != FR_OK)
	{
		printf("打开根目录失败  (%s)\r\n", FR_Table[result]);
		return;
	}

	printf("属性        |  文件大小 | 短文件名 | 长文件名\r\n");
	for (cnt = 0; ;cnt++)
	{
		result = f_readdir(&DirInf, &FileInf); 		/* 读取目录项，索引会自动下移 */
		if (result != FR_OK || FileInf.fname[0] == 0)
		{
			break;
		}

		if (FileInf.fname[0] == '.')
		{
			continue;
		}

		/* 判断是文件还是子目录 */
		if (FileInf.fattrib & AM_DIR)
		{
			printf("(0x%02d)目录  ", FileInf.fattrib);
		}
		else
		{
			printf("(0x%02d)文件  ", FileInf.fattrib);
		}

		f_stat(FileInf.fname, &fno);
		
		/* 打印文件大小, 最大4G */
		printf(" %10d", (int)fno.fsize);


		printf("  %s\r\n", (char *)FileInf.fname);	/* 长文件名 */
	}
 
	/* 卸载文件系统 */
	 f_mount(NULL, _path, 0);
}

/*
*********************************************************************************************************
*	函 数 名: CreateNewFile
*	功能说明: 在SD卡创建一个新文件，文件内容填写“www.armfly.com”
*	形    参：无
*	返 回 值: 无
*********************************************************************************************************
*/
void CreateNewFile(char *_path)
{
	FRESULT result;
	uint32_t bw;
	char path[32];


 	/* 挂载文件系统 */
	result = f_mount(&fs, _path, 0);			/* Mount a logical drive */
	if (result != FR_OK)
	{
		printf("挂载文件系统失败 (%s)\r\n", FR_Table[result]);
	}

	/* 打开文件 */
	sprintf(path, "%sfmsh.txt", _path);
	result = f_open(&file, path, FA_CREATE_ALWAYS | FA_WRITE);
	if (result == FR_OK)
	{
		printf("fmsh.txt 文件打开成功\r\n");
	}
	else
	{
		printf("fmsh.txt 文件打开失败  (%s)\r\n", FR_Table[result]);
	}

	/* 写一串数据 */
	result = f_write(&file, FsWriteBuf, strlen(FsWriteBuf), &bw);
	if (result == FR_OK)
	{
		printf("fmsh.txt 文件写入成功\r\n");
	}
	else
	{
		printf("fmsh.txt 文件写入失败  (%s)\r\n", FR_Table[result]);
	}

	/* 关闭文件*/
	f_close(&file);

	/* 卸载文件系统 */
	f_mount(NULL, _path, 0);
}

/*
*********************************************************************************************************
*	函 数 名: ReadFileData
*	功能说明: 读取文件armfly.txt前128个字符，并打印到串口
*	形    参：无
*	返 回 值: 无
*********************************************************************************************************
*/
void ReadFileData(char *_path)
{
	FRESULT result;
	uint32_t bw;
	char path[64];

	
 	/* 挂载文件系统 */
	result = f_mount(&fs, _path, 0);			/* Mount a logical drive */
	if (result != FR_OK)
	{
		printf("挂载文件系统失败 (%s)\r\n", FR_Table[result]);
	}

	/* 打开文件 */
	sprintf(path, "%sfmsh.txt", _path);
	result = f_open(&file, path, FA_OPEN_EXISTING | FA_READ);
	if (result !=  FR_OK)
	{
		printf("Don't Find File : fmsh.txt\r\n");
		return;
	}

	/* 读取文件 */
	result = f_read(&file, FsReadBuf, sizeof(FsReadBuf), &bw);
	if (bw > 0)
	{
		FsReadBuf[bw] = 0;
		printf("\r\n fmsh.txt 文件内容 : \r\n%s\r\n", FsReadBuf);
	}
	else
	{
		printf("\r\n fmsh.txt 文件内容 : \r\n");
	}

	/* 关闭文件*/
	f_close(&file);

	/* 卸载文件系统 */
	f_mount(NULL, _path, 0);
}

/***************************** 安富莱电子 www.armfly.com (END OF FILE) *********************************/
