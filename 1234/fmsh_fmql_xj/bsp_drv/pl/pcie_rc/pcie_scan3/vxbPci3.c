/* vxbPci.c - PCI libary */

/*
 * Copyright (c) 2013-2017 Wind River Systems, Inc.
 *
 * The right to copy, distribute, modify or otherwise make use
 * of this software may be licensed only pursuant to the terms
 * of an applicable Wind River license agreement.
 */

/*
modification history
--------------------
21sep22,jc  Created.
*/

/*
DESCRIPTION
This library contains the support routines for PCI libary
*/

#include "vxbPciMethod3.h"
#include "pciConfigLib3.h"
#include "pciAutoConfigLib3.h"
#include "vxbPciLib3.h"
#include "dllLib3.h"


/* Debug macro */
#ifdef  PCI_DEBUG
#undef  PCI_DEBUG
#define PCI_DEBUG
#endif

/*#include <private/kwriteLibP.h>*/

/*#ifdef ROUND_UP*/
/*#undef ROUND_UP*/
#define ROUND_UP(n, align)  ((n + align - 1) & (~(align - 1)))
/*#endif*/


#if 0
#define PCI_DBG_MSG(lvl, fmt, args...)        \
    do {                                      \
        if (debugLevel >= lvl)                \
            {                                 \
            if (_func_kprintf != NULL)        \
                _func_kprintf (fmt, ##args);  \
            }                                 \
       } while (FALSE)

#define PCI_DBG_MSG(lvl, fmt, args...)
#endif

int debugLevel = 0;

/* PCI_DEBUG */
#if 0
  PHYS_ADDR vxbPciAddr2Cpu (VXB_DEV_ID3 pDev, UINT64 pciAddr);
  STATUS vxbPciResourceInit (VXB_DEV_ID3 pDev,
    VXB_DEV_ID3 pChild, PCI_DEVICE3 * pPciDev);

  STATUS vxbPciPrivateExtCapFind (VXB_DEV_ID3 pDev,
    PCI_HARDWARE_3 * hardInfo, UINT8 requestedCapId, UINT8 * pOffset);
#endif

/*******************************************************************************
*
* vxbPciSetBarResPool - set resource pool
*
* This routine set the bar resource pool, such as IO/MEM, prefetch or
* none-prefetch type resource, this API is called by bus controller driver
* when it needs the PCI library to do the configuration for PCI device
*
* RETURNS: N/A
*
* ERRNO
*
*/

void vxbPciSetBarResPool3
    (
    PCI_ROOT_RES3  * pRes,
    struct barRes3 * res
    )
	{
	UINT32 i;

	for (i = 0 ; i < PCI_ROOT_RES_MAX_IDX; i++)
	{
		if (res[i].len == 0)
		    continue;

		pRes->barRes[i].start   = res[i].base;
		pRes->barRes[i].cpuBase = res[i].baseParent;
		pRes->barRes[i].size    = res[i].len;
		pRes->barRes[i].end     = res[i].base + res[i].len - 1;
		pRes->barRes[i].adjust  = res[i].adjust;

		pRes->barRes[i].pool.base = res[i].base;
		pRes->barRes[i].pool.size = res[i].len;
	}

	return;
}
	

/******************************************************************************
*
* vxbPciDeviceBarSize - calculate BAR size
*
* This routine calculate BAR size
*
* RETURNS: OK or ERROR when the PCI bar is assgined with a 64bit address
* and the higher 32bit isn't zero in 32bit system.
*
* ERRNO
*
*/
STATUS vxbPciDevBarSize3
    (
    VXB_DEV_ID3  pDev,
    PCI_HARDWARE_3 * hardInfo,
    UINT8       offset,
    ULONG *     size
    )
{
    UINT32 adrlo, adrhi, temp1, temp2;
    UINT16 command;
    ULONG adr64;
    UINT32 barMask;

    (void) VXB_PCI_CFG_READ_3 (pDev, hardInfo, offset, 4, &temp1);

    /* Save command register */

    (void) VXB_PCI_CFG_READ_3 (pDev, hardInfo, PCI_CFG_COMMAND, 2, &command);
    (void) VXB_PCI_CFG_WRITE_3 (pDev, hardInfo, PCI_CFG_COMMAND, 2, 0);

    (void) VXB_PCI_CFG_WRITE_3 (pDev, hardInfo, offset, 4, 0xFFFFFFFF);

    (void) VXB_PCI_CFG_READ_3 (pDev, hardInfo, offset, 4, &adrlo);
    (void) VXB_PCI_CFG_WRITE_3 (pDev, hardInfo, offset, 4, temp1);
    (void) VXB_PCI_CFG_WRITE_3 (pDev, hardInfo, PCI_CFG_COMMAND, 2, command);

    if (adrlo == 0)
        {
        * size = 0;

        return (OK);
        }

    /*
     * Note: for I/O space BARs on x86, the address is never larger than 65535.
     * This is because I/O address bus on the x86 platform (which is the only
     * platform where I/O space actually has any meaning) is limited to 16
     * bits. This means the when we write all 1s to the BAR, the upper
     * 16 bits may stay set to 0. This a problem because our size calculation
     * logic will break if the address and mask aren't both sign-extended
     * out to 32-bits. So we force the upper 16 bits on here.
     */

    if (adrlo & PCI_BAR_SPACE_IO)
        adrlo |= 0xFFFF0000;

    barMask = adrlo & PCI_BAR_SPACE_IO ? PCI_IOBASE_MASK : PCI_MEMBASE_MASK;

    if ((barMask == PCI_MEMBASE_MASK) && (adrlo & PCI_BAR_MEM_ADDR64))
        {
        (void) VXB_PCI_CFG_WRITE_3 (pDev, hardInfo, PCI_CFG_COMMAND, 2, 0);

        (void) VXB_PCI_CFG_READ_3(pDev, hardInfo, offset + 4, 4, &temp2);

        (void) VXB_PCI_CFG_WRITE_3 (pDev, hardInfo, offset + 4, 4, 0xFFFFFFFF);

        (void) VXB_PCI_CFG_READ_3(pDev, hardInfo, offset + 4, 4, &adrhi);

        adr64 = ~(ULONG)((UINT64)adrhi << 32 | (adrlo & barMask)) + 1;

        (void) VXB_PCI_CFG_WRITE_3 (pDev, hardInfo, offset + 4, 4, temp2);

        (void) VXB_PCI_CFG_WRITE_3 (pDev, hardInfo, PCI_CFG_COMMAND, 2, command);

        * size = adr64;

#ifdef  _WRS_CONFIG_LP64
        return (OK);
#else /* _WRS_CONFIG_LP64 */

        if (temp2 != 0)
            {
            /*PCI_DBG_MSG(0, "32 Bit system can't process 64 bit bar Disable it "*/
            /*    "This bar needs be reconfigure (%d:%d:%d:0x%x) \n",*/
            /*    busNo,deviceNo,funcNo,offset + 4);*/
            
            printf( "32 Bit system can't process 64 bit bar Disable it "
                    "This bar needs be reconfigure (%d:%d:%d:0x%x) \n",
                    hardInfo->pciBus,hardInfo->pciDev,hardInfo->pciFunc, offset + 4);
			
            (void) VXB_PCI_CFG_WRITE_3 (pDev, hardInfo, offset + 4, 4, 0);

            return (ERROR);
            }
#endif /* _WRS_CONFIG_LP64 */
        }

#if 0  /* ref_lq*/
    *size = ((~(adrlo & barMask) + 1) > PCIE_EP_BAR_MAX_SIZE) ? PCIE_EP_BAR_MAX_SIZE : (~(adrlo & barMask) + 1);
#else  /* origin*/
    *size = (~(adrlo & barMask) + 1);
#endif

    return (OK);
}
	
/******************************************************************************
*
* vxbPciResourceGet3 - retreive the resource from the BARx
*
* This routine retreives the resource from the BARx
* 获取设备的所有bar空间大小
* RETURNS: N/A
*
* ERRNO
*
*/
void vxbPciResourceGet3
    (
    VXB_DEV_ID3   pDev,
    PCI_DEVICE3 * pPciDev
    )
{
    ULONG barSize;
    UINT32 ltemp, ltemp2, barMask = 0;
    UINT32 adrlo;
    STATUS ret;
    UINT8 i;
    UINT16 preMemBase;
    PCI_HARDWARE_3 hardInfo;
    BOOL autoConfig;
    VXB_DEV_ID3 pRootDev ;
	
    PCI_ROOT_CHAIN3 * pciDevChain;

    autoConfig = 1;  /* jc: 0->1*/

    hardInfo.pciBus = pPciDev->bus;
    hardInfo.pciDev = pPciDev->device;
    hardInfo.pciFunc = pPciDev->func;

    /* 
		expansion_ROM space
	忽略 expansion_ROM 空间,一般不用
	*/
#if 1
    (void) VXB_PCI_CFG_READ_3(pDev, &hardInfo, PCI_CFG_EXPANSION_ROM, 4, &ltemp);

    barMask = PCI_MEMBASE_MASK;

    pPciDev->resource[PCI_ROM_BASE_INDEX].type = PCI_BAR_MEM_PREFETCH;

    (void) VXB_PCI_CFG_WRITE_3(pDev, &hardInfo, PCI_CFG_EXPANSION_ROM, 4, 0xFFFFFFFF);

    (void) VXB_PCI_CFG_READ_3(pDev, &hardInfo, PCI_CFG_EXPANSION_ROM, 4, &adrlo);

    if (adrlo == 0xffffffff)
        {
        barSize = 0;
        }
    else
        {
        barSize = (~(adrlo & barMask) + 1);
        }

    (void) VXB_PCI_CFG_WRITE_3 (pDev, &hardInfo, PCI_CFG_EXPANSION_ROM, 4, ltemp);

    pPciDev->resource[PCI_ROM_BASE_INDEX].start = ltemp & barMask;

    if (autoConfig == TRUE)
        pPciDev->resource[PCI_ROM_BASE_INDEX].size = ROUND_UP(barSize, 0x1000);
    else
        pPciDev->resource[PCI_ROM_BASE_INDEX].size = barSize;
#endif


    if (pPciDev->type == PCI_BRIDGE_TYPE)
    {
        /* BAR0, BAR1 */
		/*
		read bar0[3:0] 
				[0]:1:io space 0: mem space
				[2]:1:64bit address 0:32bit address
		桥设备只有bar0/1 空间，默认就32bit mem 地址空间
		*/
        (void) VXB_PCI_CFG_READ_3 (pDev, &hardInfo, PCI_CFG_BASE_ADDRESS_0, 4, &ltemp);
		
		printf("PCI_BRIDGE_TYPE:0x010=0x%08X \n", ltemp);

        barMask = ltemp & PCI_BAR_SPACE_IO ? PCI_IOBASE_MASK : PCI_MEMBASE_MASK;

		if ((barMask == PCI_MEMBASE_MASK) && (ltemp & PCI_BAR_MEM_ADDR64))
        {
	        (void) VXB_PCI_CFG_READ_3 (pDev, &hardInfo, PCI_CFG_BASE_ADDRESS_1, 4, &ltemp2);

	        pPciDev->resource[PCI_DEV_BAR0_INDEX].start = (ULONG)((UINT64)ltemp2 << 32) | (ltemp & barMask);

	        pPciDev->resource[PCI_DEV_BAR0_INDEX].type = ltemp & (~barMask);

	        ret = vxbPciDevBarSize3 (pDev, &hardInfo, PCI_CFG_BASE_ADDRESS_0, &barSize);

	        if (ret == ERROR || barSize == 0)
	            pPciDev->resource[PCI_DEV_BAR0_INDEX].type |= PCI_RESOURCE_ERROR;

	        if (autoConfig == TRUE)
	            pPciDev->resource[PCI_DEV_BAR0_INDEX].size =
	                (barMask == PCI_IOBASE_MASK)? ROUND_UP(barSize, 4):ROUND_UP(barSize, 0x1000);
	        else
	            pPciDev->resource[PCI_DEV_BAR0_INDEX].size = barSize;


	        pPciDev->resource[PCI_DEV_BAR0_INDEX].end = pPciDev->resource[PCI_DEV_BAR0_INDEX].start +
	            pPciDev->resource[PCI_DEV_BAR0_INDEX].size;

	        pPciDev->resource[PCI_DEV_BAR0_INDEX].align =
	            pPciDev->resource[PCI_DEV_BAR0_INDEX].size;
        }
        else
		{
			 /*
				bridge bar0 has 32bit mem bar or io space
				start = bar0 default
				type  = 4bit mem bar
				size  = caculate bar0+bar1 size
				end   = start + size
				align = size
				获取bar0 空间的大小
				起始地址设置为默认值，类型设置为PCI_DEV_BAR0_INDEX
				对齐大小就是空间大小
				*/
			pPciDev->resource[PCI_DEV_BAR0_INDEX].start = ltemp & barMask;
			pPciDev->resource[PCI_DEV_BAR0_INDEX].type = ltemp & (~barMask);

			if (vxbPciDevBarSize3 (pDev, &hardInfo, PCI_CFG_BASE_ADDRESS_0, &barSize) == ERROR || barSize == 0)
			{    
				pPciDev->resource[PCI_DEV_BAR0_INDEX].type |= PCI_RESOURCE_ERROR;
			}

			if (autoConfig == TRUE)
			    pPciDev->resource[PCI_DEV_BAR0_INDEX].size =
			        (barMask == PCI_IOBASE_MASK)? ROUND_UP(barSize, 4):ROUND_UP(barSize, 0x1000);
			else
			    pPciDev->resource[PCI_DEV_BAR0_INDEX].size = barSize;

			pPciDev->resource[PCI_DEV_BAR0_INDEX].end =
			        pPciDev->resource[PCI_DEV_BAR0_INDEX].start +
			        pPciDev->resource[PCI_DEV_BAR0_INDEX].size;

			pPciDev->resource[PCI_DEV_BAR0_INDEX].align =
			        pPciDev->resource[PCI_DEV_BAR0_INDEX].size;
			
			 /*
				bridge bar1 has 32bit mem bar or io space
				start = bar1 default
				type  = 4bit mem bar
				size  = caculate bar0+bar1 size
				end   = start + size
				align = size
				获取bar1 空间的大小
				起始地址设置为默认值，类型设置为PCI_DEV_BAR1_INDEX
				对齐大小就是空间大小
				*/
			(void) VXB_PCI_CFG_READ_3 (pDev, &hardInfo, PCI_CFG_BASE_ADDRESS_1, 4, &ltemp2);

			barMask = ltemp2 & PCI_BAR_SPACE_IO ? PCI_IOBASE_MASK : PCI_MEMBASE_MASK;

			pPciDev->resource[PCI_DEV_BAR1_INDEX].start = ltemp2 & barMask;
			pPciDev->resource[PCI_DEV_BAR1_INDEX].type = ltemp2 & (~barMask);
			
			if (vxbPciDevBarSize3 (pDev, &hardInfo,
			    PCI_CFG_BASE_ADDRESS_1,&barSize) == ERROR || barSize == 0)
			    pPciDev->resource[PCI_DEV_BAR1_INDEX].type |= PCI_RESOURCE_ERROR;

			if (autoConfig == TRUE)
			    pPciDev->resource[PCI_DEV_BAR1_INDEX].size =
			        (barMask == PCI_IOBASE_MASK)? ROUND_UP(barSize, 4):ROUND_UP(barSize, 0x1000);
			else
			    pPciDev->resource[PCI_DEV_BAR1_INDEX].size = barSize;

			pPciDev->resource[PCI_DEV_BAR1_INDEX].end =
			        pPciDev->resource[PCI_DEV_BAR1_INDEX].start +
			        pPciDev->resource[PCI_DEV_BAR1_INDEX].size;

			pPciDev->resource[PCI_DEV_BAR1_INDEX].align =
			    pPciDev->resource[PCI_DEV_BAR1_INDEX].size;
		}

		/*
		bridge mem base/io base/pre_mem base
		*/
        pPciDev->resource[PCI_BRIDGE_MEM_BASE_INDEX].type = PCI_BAR_MEM_ADDR32;
        pPciDev->resource[PCI_BRIDGE_IO_BASE_INDEX].type = PCI_BAR_SPACE_IO;       
		/* PREF 64 MEM */
        pPciDev->resource[PCI_BRIDGE_PRE_MEM_INDEX].type = PCI_BAR_MEM_PREFETCH;
		
        (void) VXB_PCI_CFG_READ_3 (pDev, &hardInfo, PCI_CFG_PRE_MEM_BASE, 2, &preMemBase);
		
        if ((preMemBase & PCI_PREF_MEM_DECODE_64) == PCI_PREF_MEM_DECODE_64)
            pPciDev->resource[PCI_BRIDGE_PRE_MEM_INDEX].type |=  PCI_BAR_MEM_ADDR64;
        else		
        	pPciDev->resource[PCI_BRIDGE_PRE_MEM_INDEX].type |= PCI_BAR_MEM_ADDR32;
    }
    else
    {
      /*
      	device bar0-5 
      	节点设备一般有6个bar空间，由于dsp 作为节点时，有两个bar空间
      	而且两个bar空间大小都为256MB，无法给它分配这么大空间，所以限制节点设备
      	只扫描一个节点
        */
		/*for (i = 0; i <= PCI_DEV_BAR0_INDEX; i ++)  // for dsp*/
		for (i = 0; i <= PCI_DEV_BAR5_INDEX; i ++)
	 	{
            (void) VXB_PCI_CFG_READ_3 (pDev, &hardInfo, i*4 + PCI_CFG_BASE_ADDRESS_0, 4, &ltemp);
			
			printf("PCI_DEV_TYPE:0x%X=0x%08X \n", (i*4 + PCI_CFG_BASE_ADDRESS_0), ltemp);

            barMask = ltemp & PCI_BAR_SPACE_IO ? PCI_IOBASE_MASK : PCI_MEMBASE_MASK;
			
			/*
			bar is 64bit mem
			忽略64bit mem空间
			*/
            if ((barMask == PCI_MEMBASE_MASK) && (ltemp & PCI_BAR_MEM_ADDR64))
            {
                (void) VXB_PCI_CFG_READ_3 (pDev, &hardInfo, (i+1)*4 + PCI_CFG_BASE_ADDRESS_0, 4, &ltemp2);
			#ifndef _WRS_CONFIG_LP64
                if (ltemp2 != 0)
                    {
                    pPciDev->resource[i].type = PCI_RESOURCE_ERROR;
                    i ++;
                    continue;
                    }
			#endif /* _WRS_CONFIG_LP64 */

                pPciDev->resource[i].start = (ULONG)((UINT64)ltemp2 << 32) | (ltemp & barMask);

                pPciDev->resource[i].type = ltemp & (~barMask);

                if (vxbPciDevBarSize3 (pDev, &hardInfo,
                    (UINT8)(i*4 + PCI_CFG_BASE_ADDRESS_0), &barSize) == ERROR || barSize == 0)
                    pPciDev->resource[i].type = PCI_RESOURCE_ERROR;

                if (autoConfig == TRUE)
                    pPciDev->resource[i].size =
                        (barMask == PCI_IOBASE_MASK)? ROUND_UP(barSize, 4):ROUND_UP(barSize, 0x1000);
                else
                    pPciDev->resource[i].size = barSize;

                pPciDev->resource[i].align = pPciDev->resource[i].size;

                pPciDev->resource[i].end = pPciDev->resource[i].start +
                    pPciDev->resource[i].size;

                i ++;
            }
            else
            {
            	/*
				bar is 32bit mem or io
				start = bar0
            		type  = 4bit mem bar
            		size  = caculate bar0+bar1 size
            		end   = start + size
            		align = size
            		获取bar0 空间的大小
            		起始地址设置为默认值，类型设置为PCI_BAR_MEM_ADDR32=0
            		对齐大小就是空间大小
				*/
                pPciDev->resource[i].start = ltemp & barMask;

                pPciDev->resource[i].type = ltemp & (~barMask);

                if (vxbPciDevBarSize3 (pDev, &hardInfo, (UINT8)(i*4 + PCI_CFG_BASE_ADDRESS_0), &barSize) == ERROR || barSize == 0)
                {    
                	pPciDev->resource[i].type = PCI_RESOURCE_ERROR;
                }

                if (autoConfig == TRUE)
                {    
                	pPciDev->resource[i].size = (barMask == PCI_IOBASE_MASK)? ROUND_UP(barSize, 4):ROUND_UP(barSize, 0x1000);
                }
                else
                {    
                	pPciDev->resource[i].size = barSize;
                }

                pPciDev->resource[i].align = pPciDev->resource[i].size;

                pPciDev->resource[i].end = pPciDev->resource[i].start + pPciDev->resource[i].size;
            }
        }
    }

    return;
}


