/*
-> InitRamFsEnv("ram:/data",3*1024*1024) 
Retrieved old volume params with %33 confidence:
Volume Parameters: FAT type: FAT32, sectors per cluster 0
  0 FAT copies, 0 clusters, 0 sectors per FAT
  Sectors reserved 0, hidden 0, FAT sectors 0
  Root dir entries 0, sysId (null)  , serial number 54f0000
  Label:"           " ...
Disk with 6144 sectors of 512 bytes will be formatted with:
Volume Parameters: FAT type: FAT12, sectors per cluster 2
  2 FAT copies, 3046 clusters, 9 sectors per FAT
  Sectors reserved 1, hidden 0, FAT sectors 18
  Root dir entries 512, sysId VXDOS12 , serial number 54f0000
  Label:"           " ...
value = 3145728 = 0x300000

-> DeleteRamDisk("ram:/data")
value = 0 = 0x0
*/


#include "vxWorks.h"

#include "drv/hdisk/ataDrv.h"
#include "string.h"
#include "stdlib.h"
#include "stdio.h"
#include "ioLib.h"
#include "iosLib.h"

#include "dosFsLib.h"

#if 1

#define CreateRamDiskTest

STATUS CreateRamDisk(char * name,int size)
{
	int nBlock = NULL ;
	BLK_DEV * pBlkDev = NULL ;
	DOS_VOL_DESC * pVolDesc = NULL ;
	
	/* the disksize should be integral multiple of the blocksize.*/
	size = size - size%512;
	nBlock = size/512;
	
	/* You can simultaneously open 20 files*/
	/*dosFsInit(20);*/
	
	dosFsLibInit(20, DOS_OPT_DEFAULT);
	/* dosFsLibInit(20); */
	
	/* Create a ram-disk.
	 The base address is the return value of alloc.
	 The block size is 512.
	 nBlock blocks per track
	 Total nBlock blocks.
	 The base address offset is 0.
	 */
	pBlkDev = ramDevCreate(0,512,nBlock,nBlock,0) ;
	if (NULL==pBlkDev)
	{
		fprintf(stderr,"Can not create ram block device.n") ;
		return ERROR ;
	}
	
	/* Make DOSFS by a ram block device.*/
	
	/*dosFsVolFormat ("/ramDrv:0", DOS_OPT_BLANK, NULL);*/
	dosFsVolFormat ("/ramDrv:0", DOS_OPT_FAT16, NULL);
	
	pVolDesc = dosFsMkfs(name, pBlkDev);
	if (NULL==pVolDesc)
	{
		fprintf(stderr,"Can not create ram-dos-fs.n") ;
		return ERROR ;
	}
	
	/* The size is actualy disk size.*/
	return size ;
}


/**********************************************************************
Function: Delete a ram disk device
Parameters:
name -> device name, such as "ramdisk0:".
Returned:
Return OK if the device is removed successfuly.
Otherwise return ERROR.
**********************************************************************/
STATUS DeleteRamDisk(char * name)
{
	DEV_HDR * pDevHdr = NULL ;
	
	/* Find ram-disk device by name*/
	if ( NULL==(pDevHdr=iosDevFind(name,NULL)) )
	{
		fprintf(stderr,"Can not find device (%s).n",name) ;
		return ERROR ;
	}
	
	/* Delete the device and free the alloced memory*/
	iosDevDelete(pDevHdr) ;
	free(pDevHdr) ;
	return OK ;
}


/**********************************************************************
Function: Create a ram disk device & set is as default path.
Parameters:
name -> device name, such as "ramdisk0:".
size -> block device size.
Returned:
The actualy disk size. Or ERROR.
**********************************************************************/
STATUS InitRamFsEnv(char * name,int size)
{
	#ifdef CreateRamDiskTest
		STATUS iReturn = CreateRamDisk(name,size) ;
		if (ERROR!=iReturn) 
			ioDefPathSet(name) ;
		
		return iReturn ;
	#else 
		BLK_DEV *pBlkDev;
		DOS_VOL_DESC *pVolDesc;
		
		pBlkDev = ramDevCreate (0,  512,  400,  400,  0);
		pVolDesc = dosFsMkfs ("DEV1:", pBlkDev);
		
		if (NULL==pVolDesc)
		{ 
			fprintf(stderr,"Can not create ram-dos-fs.n") ;
			return ERROR ;
		}
		
		return TRUE;
	#endif
}
#endif

void test_ramdisk(void)
{
	BLK_DEV *pBlkDev;
	
	pBlkDev = ramDevCreate (NULL,  512,  32,  416,	0);
	xbdBlkDevCreate (pBlkDev, "/ramDrv");
	/*dosFsVolFormat ("/ramDrv:0", DOS_OPT_BLANK, NULL);*/

	return;
}


