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
21apr17,syt  initialized align size value for type1 device' BAR0
             in vxbPciResourceGet.(V7PRO-3676)
26aug16,y_y  add prefetch memory 64bit support. (V7PRO-3155)
03oct16,wap  Use MAX_DEV_NAME_LEN as unit address buffer size
20sep16,wap  Set up name-address field (F7769)
28jul16,wap  Fix compiler warning in 64-bit build
27jul16,wap  Newly probed PCI devices should always have MSI/MSI-X
             capabilities disabled. If they don't, this means a previous
             OS context (e.g. bootrom) left them on and we should force
             them off before allowing a driver to claim the device.
28jul16,y_y  Move vxbPciCtrlList to vxbPci.c to support IA (V7PRO-3215)
07jul16,wap  Don't return success in the ioctl handler when built
             with _WRS_CONFIG_VXBUS_BASIC and an unhandled ioctl
             command is requested
12may16,pmr  resolve compiler warnings (US81418)
18apr16,wep  Correct the handling of the data argument to the
             VXB_PCI_CFG_READ() method in vxbPciCfgReadStub()
12apr16,wap  Add PCI config read/write method call stubs for user-mode
             drivers
09mar16,wap  Remove initialization for pProp/numProp fields. This
             should be done in the driver's VXB_PCI_INT_ASSIGN method.
28jan16,wap  Remove platform-specific code, add initialization for
             pProp/numProp fields in interrupt resources (F6448)
07jan16,y_y  add vxbPcieExtCapFind(). (F4655)
03jan16,l_z  don't round up for none-autoconfig condition. (V7PRO-2837)
07jan16,l_z  add pointer check for vxbPciDevMatch(). (V7PRO-2623)
21sep15,wyt  fix several logic error for resource assign. (V7PRO-2161)
08sep15,wap  Use baseBusNumber when calling vxbPciBusAddDev() in
             vxbPciAutoConfig()
10aug15,wap  Add library init routine, move _func_vxbPciAutoConfig here from
             the autoconfig componment to break linker dependency
30jul15,wap  Correctly set alignment for memory-mapped device BARs,
             simplify usage of vxbPciDeviceBarSize()
29jul15,wap  Correctly obtain header type value in vxbPciExtCapFind()
22jul15,wap  Add vxbPciExtCapFind() function (to avoid duplicating it
             elsewhere)
15jul15,wap  Do not abort the entire bus scan if we have difficulty
             allocating resources for a single device node
02jul15,wap  Clean up detach logic
17jun15,wap  Correct initalization of BAR alignment values
12jun15,wap  Add device manipulation ioctls (F3973)
11jun15,wap  Correct the definition of the PCI bus type
10jun15,wap  Remember to test for pre-fetchable windows in
             vxbPciAutoConfig()
10jun15,wap  Fix BAR alignment handling, rework PCI to CPU address
             translation
13may15,wap  Update bus type name string
17mar15,wap  Rework to support PCI bus bridge nodes and drivers (F3973)
02apr15,l_z  initialize child resource list. (V7PRO-1939)
27may15,l_z  enable the MSI Per-vector Mask. (V7PRO-1625)
06feb15,y_y  fix resource assign not correct issue. (V7PRO-1661)
30jan15,to   US45794 - add VXB_IOAPIC_LVEC_BASE to lVec for IOAPIC
22jan15,d_l  fix gnu warning
28nov14,y_y  fix prefetch mem 64bit resource adjust issue. (V7PRO-1477)
27nov14,l_z  mask of the upper 16 bits for IO BAR. (V7PRO-1459)
04nov14,rbc  US44720 - Fix static analysis errors
17oct14,jmz  V7PRO-1403 - Update pin swizzle in vxbPciIntAssign()
             Correct loop indexing error in vxbPciAddDevice()
04nov14,l_z  fix static analysis issue. (V7PRO-1181)
29oct14,l_z  fix the wrong swizzle logic. (V7PRO-1421)
23sep14,l_z  fix wrong device type for IDE device. (V7PRO-1051)
18aug14,l_z  fix static analysis issue. (V7PRO-1181)
18jul14,scm  US42878 - conform to vxbus hVec usage...
07jul14,jmz  V7PRO-1070 - Restore BAR register original value during enumeration
25jun14,jmz  US24660 - Fix static analysis warnings
16jun14,jmz  US35979 - Updated for IA arch
03jun14,l_z  fix static analysis issue. (V7PRO-951)
31mar14,l_z  enhance PCI library. (US37630)
25mar14,ylu  correct the DEBUG macro PCI_LOG_MSG.
08jan14,y_y  code clean
14sep13,y_y  Add MSI support.
17apr13,j_z  Created.
*/

/*
DESCRIPTION
This library contains the support routines for PCI libary
*/

#include <vxWorks.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <hwif/vxBus.h>
#include <hwif/vxbus/vxbIoctlLib.h>
#ifdef _WRS_CONFIG_VXBUS_USER
#include <hwif/vxbus/vxbRtpIoctlLib.h>
#endif /* _WRS_CONFIG_VXBUS_USER */
#include <hwif/buslib/pciDefines.h>
#include <hwif/buslib/vxbPciLib.h>
#ifdef _WRS_CONFIG_FDT
#include <hwif/buslib/vxbFdtPci.h>
#endif /* _WRS_CONFIG_FDT */
#ifdef _WRS_CONFIG_PCIE_AER
#include <hwif/buslib/vxbPcieAer.h>
#endif  /* _WRS_CONFIG_PCIE_AER */

BUSTYPE_DEF(vxbPciBus, "PCI bus type");

STATUS (*_func_vxbPciAutoConfig)(VXB_DEV_ID);

SL_LIST vxbPciCtrlList;
BOOL vxbPciCtrlListInited = FALSE;

/* Debug macro */

#undef  PCI_DEBUG
#ifdef  PCI_DEBUG

#include <private/kwriteLibP.h>

LOCAL int debugLevel = 0;

#ifdef  LOCAL
#undef  LOCAL
#define LOCAL
#endif

#define PCI_DBG_MSG(lvl, fmt, args...)        \
    do {                                      \
        if (debugLevel >= lvl)                \
            {                                 \
            if (_func_kprintf != NULL)        \
                _func_kprintf (fmt, ##args);  \
            }                                 \
       } while (FALSE)
#else
#define PCI_DBG_MSG(lvl, fmt, args...)

#endif  /* PCI_DEBUG */

LOCAL PHYS_ADDR vxbPciAddr2Cpu (VXB_DEV_ID pDev, UINT64 pciAddr);
LOCAL STATUS vxbPciResourceInit (VXB_DEV_ID pDev,
    VXB_DEV_ID pChild, PCI_DEVICE * pPciDev);

LOCAL STATUS vxbPciPrivateExtCapFind (VXB_DEV_ID pDev,
    PCI_HARDWARE * hardInfo, UINT8 requestedCapId, UINT8 * pOffset);

/*******************************************************************************
*
* vxbPciInit - initialization routine for PCI library
*
* This routine initializes the PCI bus library. Currently it's just a stub
* provided as an init hook for the PCI component definition file.
*
* RETURNS: always OK
*
* ERRNO: N/A
*/

STATUS vxbPciInit (void)
    {
    return (OK);
    }

/*******************************************************************************
*
* vxbPciBusIoctl - ioctl method for PCI bus device
*
* This routine supprt commands to list/add/remove child device for IO system
*
* RETURNS: Ok or ERROR when command run failed
*
* ERRNO: N/A
*/

STATUS vxbPciBusIoctl
    (
    VXB_DEV_ID pDev,
    int cmd,
    void * pArg
    )
    {
    UINT8 bus;
    STATUS err = OK;

    switch (cmd)
        {
        case VXBIODEVRESCAN:

            /*
             * Note: the rescan operation here only covers the case
             * where a device is newly added to the system. It doesn't
             * correctly handle the case where a device is removed.
             * vxbPciBusAddDev() just walks the bus looking for
             * devices, it doesn't remove any that have disappeared
             * since the last time we looked.
             */

            VXB_PCI_BUS_CFG_READ(pDev, PCI_CFG_SECONDARY_BUS, 1, bus);
            err = vxbPciBusAddDev (pDev, bus);
            if (err != OK)
                err = EIO;
            break;

        default:
#ifdef _WRS_CONFIG_VXBUS_BASIC
            err = ENOTSUP;
#else
            /*
             * For all other ioctl()s (LOAN/RECLAIM/RESET), fall through
             * to the generic helper routine for now.
             */

            err = vxbDevIoctlHelper (pDev, cmd, pArg);
            if (err != OK)
                err = EIO;
#endif /* !_WRS_CONFIG_VXBUS_BASIC */
            break;
        }

    return (err);
    }

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

void vxbPciSetBarResPool
    (
    PCI_ROOT_RES  * pRes,
    struct barRes * res
    )
    {
    UINT32 i;

    for (i = 0 ; i < PCI_ROOT_RES_MAX_IDX; i++)
        {
        if (res[i].len == 0)
            continue;

        pRes->barRes[i].start = res[i].base;
        pRes->barRes[i].cpuBase= res[i].baseParent;
        pRes->barRes[i].size= res[i].len;
        pRes->barRes[i].end = res[i].base + res[i].len - 1;
        pRes->barRes[i].adjust= res[i].adjust;

        pRes->barRes[i].pool.base = res[i].base;
        pRes->barRes[i].pool.size = res[i].len;
        }

    }

/*****************************************************************************
*
* vxbPciResFree - free resource from the resource list for PCI device
*
* This routine free resource from resource list.
*
* RETURNS: N/A
*
* ERRNO:
*/

LOCAL void vxbPciResFree
    (
    void *pArg,
    VXB_RESOURCE * pRes
    )
    {
    VXB_RESOURCE_IRQ * vxbResIrq = NULL;

    (void) vxbResourceRemove ((VXB_RESOURCE_LIST *)pArg, pRes);

    if (VXB_RES_TYPE(pRes->id) == VXB_RES_IRQ)
        {
        vxbResIrq = pRes->pRes;
        if (vxbResIrq->pVxbIntrEntry != NULL)
            {
            vxbMemFree (vxbResIrq->pVxbIntrEntry);
            }
        }

    vxbMemFree (pRes->pRes);
    vxbMemFree (pRes);
    }

/*****************************************************************************
*
* vxbPciResFreeAll - free all resource from the resource list for PCI device
*
* This routine free all resource from resource list.
*
* RETURNS: N/A
*
* ERRNO:
*/

LOCAL void vxbPciResFreeAll
    (
    VXB_RESOURCE_LIST  * pResList
    )
    {
    (void) vxbResourceIterate (pResList, vxbPciResFree, pResList);
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

LOCAL STATUS vxbPciDevBarSize
    (
    VXB_DEV_ID  pDev,
    PCI_HARDWARE * hardInfo,
    UINT8       offset,
    ULONG *     size
    )
    {
    UINT32 adrlo, adrhi, temp1, temp2;
    UINT16 command;
    ULONG adr64;
    UINT32 barMask;

    (void) VXB_PCI_CFG_READ (pDev, hardInfo, offset, 4, &temp1);

    /* Save command register */

    (void) VXB_PCI_CFG_READ (pDev, hardInfo, PCI_CFG_COMMAND, 2, &command);
    (void) VXB_PCI_CFG_WRITE (pDev, hardInfo, PCI_CFG_COMMAND, 2, 0);

    (void) VXB_PCI_CFG_WRITE (pDev, hardInfo, offset, 4, 0xFFFFFFFF);

    (void) VXB_PCI_CFG_READ (pDev, hardInfo, offset, 4, &adrlo);
    (void) VXB_PCI_CFG_WRITE (pDev, hardInfo, offset, 4, temp1);
    (void) VXB_PCI_CFG_WRITE (pDev, hardInfo, PCI_CFG_COMMAND, 2, command);

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
        (void) VXB_PCI_CFG_WRITE (pDev, hardInfo, PCI_CFG_COMMAND, 2, 0);

        (void) VXB_PCI_CFG_READ(pDev, hardInfo, offset + 4, 4, &temp2);

        (void) VXB_PCI_CFG_WRITE (pDev, hardInfo, offset + 4, 4, 0xFFFFFFFF);

        (void) VXB_PCI_CFG_READ(pDev, hardInfo, offset + 4, 4, &adrhi);

        adr64 = ~(ULONG)((UINT64)adrhi << 32 | (adrlo & barMask)) + 1;

        (void) VXB_PCI_CFG_WRITE (pDev, hardInfo, offset + 4, 4, temp2);

        (void) VXB_PCI_CFG_WRITE (pDev, hardInfo, PCI_CFG_COMMAND, 2, command);

        * size = adr64;

#ifdef  _WRS_CONFIG_LP64
        return (OK);
#else /* _WRS_CONFIG_LP64 */

        if (temp2 != 0)
            {
            PCI_DBG_MSG(0, "32 Bit system can't process 64 bit bar Disable it "
                "This bar needs be reconfigure (%d:%d:%d:0x%x) \n",
                busNo,deviceNo,funcNo,offset + 4);

            (void) VXB_PCI_CFG_WRITE (pDev, hardInfo, offset + 4, 4, 0);

            return (ERROR);
            }
#endif /* _WRS_CONFIG_LP64 */
        }

    *size = (~(adrlo & barMask) + 1);

    return (OK);
    }

/******************************************************************************
*
* vxbPciResourceGet - retreive the resource from the BARx
*
* This routine retreives the resource from the BARx
*
* RETURNS: N/A
*
* ERRNO
*
*/

void vxbPciResourceGet
    (
    VXB_DEV_ID   pDev,
    PCI_DEVICE * pPciDev
    )
    {
    ULONG barSize;
    UINT32 ltemp, ltemp2, barMask = 0;
    UINT32 adrlo;
    STATUS ret;
    UINT8 i;
    UINT16 preMemBase;
    PCI_HARDWARE hardInfo;
    BOOL autoConfig;
    VXB_DEV_ID pRootDev ;
    PCI_ROOT_CHAIN * pciDevChain;

    if ((pciDevChain = (PCI_ROOT_CHAIN *) vxbDevSoftcGet (pDev)) == NULL)
        return;

    if ((pRootDev = pciDevChain->pRootDev) == NULL)
        return;

    if ((pciDevChain = (PCI_ROOT_CHAIN *) vxbDevSoftcGet (pRootDev)) == NULL)
        return;

    autoConfig = pciDevChain->autoConfig;

    hardInfo.pciBus = pPciDev->bus;
    hardInfo.pciDev = pPciDev->device;
    hardInfo.pciFunc = pPciDev->func;

    /* ROM BASE */

    (void) VXB_PCI_CFG_READ(pDev, &hardInfo, PCI_CFG_EXPANSION_ROM, 4, &ltemp);

    barMask = PCI_MEMBASE_MASK;

    pPciDev->resource[PCI_ROM_BASE_INDEX].type = PCI_BAR_MEM_PREFETCH;

    (void) VXB_PCI_CFG_WRITE(pDev, &hardInfo, PCI_CFG_EXPANSION_ROM, 4, 0xFFFFFFFF);

    (void) VXB_PCI_CFG_READ(pDev, &hardInfo, PCI_CFG_EXPANSION_ROM, 4, &adrlo);

    if (adrlo == 0xffffffff)
        {
        barSize = 0;
        }
    else
        {
        barSize = (~(adrlo & barMask) + 1);
        }

    (void) VXB_PCI_CFG_WRITE (pDev, &hardInfo, PCI_CFG_EXPANSION_ROM, 4, ltemp);

    pPciDev->resource[PCI_ROM_BASE_INDEX].start = ltemp & barMask;

    if (autoConfig == TRUE)
        pPciDev->resource[PCI_ROM_BASE_INDEX].size = ROUND_UP(barSize, 0x1000);
    else
        pPciDev->resource[PCI_ROM_BASE_INDEX].size = barSize;

    if (pPciDev->type == PCI_BRIDGE_TYPE)
        {
        /* BAR0, BAR1 */

        (void) VXB_PCI_CFG_READ (pDev, &hardInfo, PCI_CFG_BASE_ADDRESS_0, 4, &ltemp);

        barMask = ltemp & PCI_BAR_SPACE_IO ? PCI_IOBASE_MASK : PCI_MEMBASE_MASK;

        if ((barMask == PCI_MEMBASE_MASK) && (ltemp & PCI_BAR_MEM_ADDR64))
            {
            (void) VXB_PCI_CFG_READ (pDev, &hardInfo, PCI_CFG_BASE_ADDRESS_1, 4, &ltemp2);

            pPciDev->resource[PCI_DEV_BAR0_INDEX].start =
                (ULONG)((UINT64)ltemp2 << 32) | (ltemp & barMask);

            pPciDev->resource[PCI_DEV_BAR0_INDEX].type = ltemp & (~barMask);

            ret = vxbPciDevBarSize (pDev, &hardInfo,
                PCI_CFG_BASE_ADDRESS_0, &barSize);

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
            pPciDev->resource[PCI_DEV_BAR0_INDEX].start = ltemp & barMask;
            pPciDev->resource[PCI_DEV_BAR0_INDEX].type = ltemp & (~barMask);

            if (vxbPciDevBarSize (pDev, &hardInfo,
                PCI_CFG_BASE_ADDRESS_0, &barSize) == ERROR || barSize == 0)
                pPciDev->resource[PCI_DEV_BAR0_INDEX].type |= PCI_RESOURCE_ERROR;

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

            (void) VXB_PCI_CFG_READ (pDev, &hardInfo, PCI_CFG_BASE_ADDRESS_1, 4, &ltemp2);

            barMask = ltemp2 & PCI_BAR_SPACE_IO ? PCI_IOBASE_MASK : PCI_MEMBASE_MASK;

            pPciDev->resource[PCI_DEV_BAR1_INDEX].start = ltemp2 & barMask;
            pPciDev->resource[PCI_DEV_BAR1_INDEX].type = ltemp2 & (~barMask);
            if (vxbPciDevBarSize (pDev, &hardInfo,
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

        pPciDev->resource[PCI_BRIDGE_MEM_BASE_INDEX].type = PCI_BAR_MEM_ADDR32;
        pPciDev->resource[PCI_BRIDGE_IO_BASE_INDEX].type = PCI_BAR_SPACE_IO;

        /* PREF 64 MEM */

        pPciDev->resource[PCI_BRIDGE_PRE_MEM_INDEX].type = PCI_BAR_MEM_PREFETCH;

        (void) VXB_PCI_CFG_READ (pDev, &hardInfo, PCI_CFG_PRE_MEM_BASE, 2, &preMemBase);

        if ((preMemBase & PCI_PREF_MEM_DECODE_64) == PCI_PREF_MEM_DECODE_64)
            pPciDev->resource[PCI_BRIDGE_PRE_MEM_INDEX].type |=  PCI_BAR_MEM_ADDR64;
        else
            pPciDev->resource[PCI_BRIDGE_PRE_MEM_INDEX].type |= PCI_BAR_MEM_ADDR32;
        }
    else
        {
        for (i = 0; i <= PCI_DEV_BAR5_INDEX; i ++)
            {
            (void) VXB_PCI_CFG_READ (pDev, &hardInfo, i*4 + PCI_CFG_BASE_ADDRESS_0, 4, &ltemp);

            barMask = ltemp & PCI_BAR_SPACE_IO ? PCI_IOBASE_MASK : PCI_MEMBASE_MASK;

            if ((barMask == PCI_MEMBASE_MASK) && (ltemp & PCI_BAR_MEM_ADDR64))
                {
                (void) VXB_PCI_CFG_READ (pDev, &hardInfo, (i+1)*4 + PCI_CFG_BASE_ADDRESS_0, 4, &ltemp2);
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

                if (vxbPciDevBarSize (pDev, &hardInfo,
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
                pPciDev->resource[i].start = ltemp & barMask;

                pPciDev->resource[i].type = ltemp & (~barMask);

                if (vxbPciDevBarSize (pDev, &hardInfo,
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
                }

            }
        }

    return;
    }

/******************************************************************************
*
* vxbPciResourceInit - create VxBus resources for a PCI device
*
* This routine is a helper function for creating VxBus resource structures
* for a given PCI child device, <pChild>, that resides on a bus specified
* by <pDev>. The <pPciDev> argument points to a PCI_DEVICE structure which
* will be populated with the PCI bus information about the child device,
* including its bus, device and function number, and the hardware information
* associated with it (which is obtained vxbPciResourceGet()).
*
* If setting up any of the resource structures fails, then the routine is
* aborted and previously allocated resources are released.
*
* RETURNS: OK or ERROR if creating any of the resources fails
*
* ERRNO: N/A
*
*/

LOCAL STATUS vxbPciResourceInit
    (
    VXB_DEV_ID pDev,
    VXB_DEV_ID pChild,
    PCI_DEVICE * pPciDev
    )
    {
    PCI_HARDWARE * pIvars;
    PCI_ROOT_CHAIN * pSoftc;
    VXB_RESOURCE_ADR * pResAdr = NULL;
    VXB_RESOURCE_IRQ * pResIrq = NULL;
    VXB_RESOURCE * pRes = NULL;
    VXB_INTR_ENTRY * pIntr = NULL;
    PCI_RES * pPciRes;
    UINT8 intPin;
    UINT8 irq;
    int barCnt;
    int i;

    pIvars = (PCI_HARDWARE *)vxbDevIvarsGet (pChild);
    pSoftc = (PCI_ROOT_CHAIN *)vxbDevSoftcGet (pDev);

    if (pIvars == NULL || pSoftc == NULL)
        return (ERROR);

    /* Acquire IO BAR information. */

    vxbPciResourceGet (pDev, pPciDev);

    /* Convert IO BAR info into VxBus resources */

    if (pPciDev->type == PCI_DEVICE_TYPE)
        barCnt = 6;
    else
        barCnt = 2;

    for (i = 0; i < barCnt; i++)
        {
        pPciRes = &pPciDev->resource[i];

        if (pPciRes->start == 0 && pPciRes->size == 0)
            continue;

        pResAdr = (VXB_RESOURCE_ADR *)vxbMemAlloc (sizeof(VXB_RESOURCE_ADR));

        if (pResAdr == NULL)
           goto fail;

        pRes = (VXB_RESOURCE *)vxbMemAlloc (sizeof(VXB_RESOURCE));
        if (pRes == NULL)
            {
            vxbMemFree ((char *)pResAdr);
            goto fail;
            }

        pRes->pRes = (void *)pResAdr;
        if ((pPciRes->type & PCI_BAR_SPACE_IO) == PCI_BAR_SPACE_IO)
            pRes->id = VXB_RES_ID_CREATE(VXB_RES_IO, i);
        else
            pRes->id = VXB_RES_ID_CREATE(VXB_RES_MEMORY, i);

        pResAdr->start = vxbPciAddr2Cpu (pDev, pPciRes->start);
        pResAdr->size = (size_t)pPciRes->size;

        if (vxbResourceAdd (&pIvars->vxbResList, pRes) != OK)
            {
            vxbMemFree ((char *)pRes);
            vxbMemFree ((char *)pResAdr);
            goto fail;
            }
        }

    /* Acquire interrupt pin info */

    (void) VXB_PCI_CFG_READ(pDev, pIvars, PCI_CFG_DEV_INT_PIN, 1, &intPin);
    pIvars->pciIntPin = intPin;

    /*
     * Assign IRQ value and convert to VxBus resource. Note that
     * this sets up the legacy INTx interrupt only.
     */

    if (intPin != 0)
        {
        pIntr = (VXB_INTR_ENTRY *)vxbMemAlloc (sizeof(VXB_INTR_ENTRY));

        if (pIntr == NULL)
            goto fail;

        /*
         * Ask the parent device to assign an IRQ for this child.
         *
         * Don't treat the failure to assign an interrupt resource
         * as a fatal error here. Ideally this should never fail,
         * but on the x86 architecture we are relying on being able
         * to find a PCI interrupt routing entry in order to match
         * the device to an IRQ, and the code that does that is a
         * little brittle.
         */

        if (VXB_PCI_INT_ASSIGN(pDev, pIvars, intPin, &irq, pIntr) != OK)
            {
            vxbMemFree (pIntr);
            pIntr = NULL;
            goto skip;
            }

        /* Save it to the child's intLine register. */

        (void) VXB_PCI_CFG_WRITE(pDev, pIvars, PCI_CFG_DEV_INT_LINE, 1, irq);

        /* Set up the VxBus resource for this IRQ. */

        pResIrq = (VXB_RESOURCE_IRQ *)vxbMemAlloc (sizeof(VXB_RESOURCE_IRQ));

        if (pResIrq == NULL)
            goto fail;

        pRes = (VXB_RESOURCE *)vxbMemAlloc (sizeof(VXB_RESOURCE));

        if (pRes == NULL)
            goto fail;

        pRes->pRes = (void *)pResIrq;
        pRes->id = VXB_RES_ID_CREATE(VXB_RES_IRQ, 0); /* INTX */

        pResIrq->hVec = irq;
        pResIrq->flag |= VXB_INT_FLAG_STATIC;
        pResIrq->pVxbIntrEntry = (void *)pIntr;
        pIntr = NULL;

        if (vxbResourceAdd (&pIvars->vxbResList, pRes) != OK)
            {
            vxbMemFree ((char *)pIntr);
            vxbMemFree ((char *)pRes);
            vxbMemFree ((char *)pResIrq);
            goto fail;
            }

        }
skip:
    return (OK);

fail:

    vxbPciResFreeAll (&pIvars->vxbResList);
    if (pIntr != NULL)
        {
        vxbMemFree ((char *)pIntr);
        }

    return (ERROR);
    }

/******************************************************************************
*
* vxbPciBusAddDev - announce PCI Device under specific bus
*
* This routine scans all of PCI device under specific bus and announces them
* to the VxBus subsystem. For each discovered device, a VxBus device node
* will be created and populated with resource information, including the
* device's I/O BARs and its INTx interrupt information.
*
* If one of the devices on the bus is a bridge (either PCI<->PCI bridge,
* PCIe<->PCI bridge, or PCIe switch), then the PCI bridge driver will be
* attached to it and that driver will recursively call this routine again
* to probe the subordinate bus. (This requires that the PCI bridge driver be
* configured into the system.)
*
* RETURNS: OK or ERROR when add PCI device failed
*
* ERRNO
*
*/

STATUS vxbPciBusAddDev
    (
    VXB_DEV_ID pDev,
    UINT8 bus
    )
    {
    VXB_DEV_ID pParent;
    PCI_HARDWARE * pHardInfo;
    UINT8 device, function, headerType,btemp;
    PCI_DEVICE * pPciDev;
    UINT32 vendId;
    UINT16 command;
    VXB_DEV_ID pVxbDev ;
    PCI_HARDWARE * pPciDevInfo;
    PCI_ROOT_CHAIN * pciRootChain;
    PCI_ROOT_CHAIN * pciParentRootChain;
    char * pName;
    UINT8 msiCap;

    pciRootChain = (PCI_ROOT_CHAIN *) vxbDevSoftcGet (pDev);
    pPciDev = vxbMemAlloc (sizeof(PCI_DEVICE));

    if (pPciDev == NULL)
        return (ERROR);

    pHardInfo = vxbMemAlloc (sizeof(PCI_HARDWARE));

    if (pHardInfo == NULL)
        {
        vxbMemFree (pPciDev);
        return (ERROR);
        }

    pName = vxbMemAlloc (MAX_DEV_NAME_LEN + 1);

    if (pName == NULL)
        {
        vxbMemFree (pPciDev);
        vxbMemFree (pHardInfo);
        return (ERROR);
        }


    if (pciRootChain == NULL)
        return (ERROR);

    /*
     * If this device has a pRootDev already assigned, then
     * it's the root of this PCI bus tree. Otherwise, it's a bridge,
     * and we set pRootDev to point to the root of the tree.
     * This allows each successive bridge layer to maintain a
     * reference to the tree root, which is necessary in some
     * cases.
     */

    if (pciRootChain->pRootDev == NULL)
        {
        pParent = vxbDevParent (pDev);
        pciParentRootChain = (PCI_ROOT_CHAIN *) vxbDevSoftcGet (pParent);
        pciRootChain->pRootDev = pciParentRootChain->pRootDev;
        }

    /*
     * Bus is typically 0 for root, or some non-zero value for
     * a bridge subordinate bus. For non-Intel platforms, each
     * PCIe root complex or PCI host bridge is a separate "hose"
     * with its own bus/dev/func namespace. For example, on a Freescale
     * QorIQ chip with three PCI Express controllers, each controller
     * is considered a separate "hose" and has its own tree, starting
     * at bus0/dev0/func0. The Intel platform is different: while
     * there may be multiple "hoses," the same bus/dev/func namespace
     * is used for all of them. In rare cases, there may be two root
     * buses (i.e. two hoses) which are not bridged to each other.
     * (For example, there may be a bus 0,0,0 and a bus 128,0,0,
     * which are logically distinct.) This means it should not be
     * assumed that the root of a PCI tree will always start at
     * 0,0,0.
     */

    for (device = 0; device < PCI_MAX_DEV; device ++)
        {
        for (function = 0; function < PCI_MAX_FUNC; function ++)
            {

            bzero (pHardInfo, sizeof(PCI_HARDWARE));
            bzero (pPciDev, sizeof(PCI_DEVICE));

            pHardInfo->pciBus = bus;
            pHardInfo->pciDev = device;
            pHardInfo->pciFunc = function;

            (void) VXB_PCI_CFG_READ(pDev, pHardInfo,
                PCI_CFG_VENDOR_ID, 4, &vendId);

            /* If function 0 then check next dev else check next function */

            if (((vendId & 0x0000ffff) == PCI_CONFIG_ABSENT_F) ||
                ((vendId & 0x0000ffff) == PCI_CONFIG_ABSENT_0) )
                {
                if (function == 0)
                    {
                    break;     /* non-existent device, goto next device */
                    }
                else
                    continue;  /* function empty, try the next function */
                }


            (void) VXB_PCI_CFG_READ (pDev, pHardInfo,
                PCI_CFG_HEADER_TYPE, 1, &headerType);

            pPciDev->bus    = bus;
            pPciDev->device = device;
            pPciDev->func   = function;

            headerType &= PCI_HEADER_TYPE_MASK;

            if ((headerType == PCI_HEADER_TYPE_BRIDGE) ||
                (headerType == PCI_HEADER_PCI_CARDBUS))
                {
                pPciDev->type = PCI_BRIDGE_TYPE;
                }
            else
                {
                pPciDev->type = PCI_DEVICE_TYPE;
                }

            pVxbDev = NULL; /* must set to NULL */

            if (vxbDevCreate (&pVxbDev) != OK)
                {
                PCI_DBG_MSG(0, "vxbDevCreate pVxbDev ERROR\r\n");
                vxbMemFree (pPciDev);
                vxbMemFree (pHardInfo);
                vxbMemFree (pName);
                return (ERROR);
                }

            pPciDevInfo = (PCI_HARDWARE *)vxbMemAlloc(sizeof (PCI_HARDWARE));

            (void) VXB_PCI_CFG_READ(pDev, pHardInfo,
               PCI_CFG_VENDOR_ID, 2, &pPciDevInfo->pciVendId);
            (void) VXB_PCI_CFG_READ(pDev, pHardInfo,
                PCI_CFG_DEVICE_ID, 2, &pPciDevInfo->pciDevId);

            pPciDevInfo->pciBus =  bus;
            pPciDevInfo->pciDev =  device;
            pPciDevInfo->pciFunc = function;

            /* coverity:[noescape : FALSE] */
            (void) vxbResourceInit (&pPciDevInfo->vxbResList);

            vxbDevClassSet (pVxbDev, VXB_BUSID_PCI);
            vxbDevIvarsSet (pVxbDev, pPciDevInfo);

            if (pPciDev->type == PCI_BRIDGE_TYPE)
                vxbDevNameSet (pVxbDev, "pci bridge", FALSE);
            else
                {
                (void) strncpy (pName, "pci device", MAX_DEV_NAME_LEN);
                (void) snprintf ((pName + 10), 21,  "(%04x:%04x)",
                    pPciDevInfo->pciVendId, pPciDevInfo->pciDevId);
                vxbDevNameSet (pVxbDev, pName, TRUE);
                }

            if (function == 0)
                {
                (void) snprintf (pName, MAX_DEV_NAME_LEN, "%x",
                    device);
                }
            else
                {
                (void) snprintf (pName, MAX_DEV_NAME_LEN, "%x,%x",
                    device, function);
                }

            vxbDevNameAddrSet (pVxbDev, pName, TRUE);

            /*
             * Note: if we fail to set up resources, don't treat
             * this as a fatal error. We don't want the failure to
             * configure a single device to abort the scan of the
             * entire bus.
             */

            if (vxbPciResourceInit (pDev, pVxbDev, pPciDev) != OK)
                {
                vxbMemFree (pPciDevInfo);
                (void) vxbDevDestroy (pVxbDev);
                continue;
                }

            /*
             * We need to make sure that MSI support is disabled before
             * handing off the device to any drivers. There is a potential
             * failure that can occur if we boot from a bootapp or other
             * context which enabled MSI support for a device and forgot
             * to turn it off. If the driver is not expecting MSI to be
             * turned on and it tries to use INTx interruptx, then the
             * the system could hang, because the first time an interrupt
             * fires, it will be directed to an MSI vector that will not
             * be bound to any ISR.
             *
             * Note that the control register is the first one in the
             * capability block.
             */

            msiCap = 0;
            (void) vxbPciPrivateExtCapFind (pDev, pHardInfo,
                PCI_EXT_CAP_MSI, &msiCap);
            if (msiCap != 0)
                {
                (void) VXB_PCI_CFG_WRITE(pDev, pHardInfo, msiCap, 1, 0);
                }

            msiCap = 0;
            (void) vxbPciPrivateExtCapFind (pDev, pHardInfo,
                PCI_EXT_CAP_MSIX, &msiCap);
            if (msiCap != 0)
                {
                (void) VXB_PCI_CFG_WRITE(pDev, pHardInfo, msiCap, 2, 0);
                }

            /*
             * Before we add the device to the tree, make sure its
             * command register has the necessary access I/O and DMA
             * access bits enabled. In theory each individual driver
             * should be responsible for doing this for each function
             * that it supports, but many drivers don't, so we try to
             * make up for that here.
             */

            (void) VXB_PCI_CFG_READ(pDev, pHardInfo,
                PCI_CFG_COMMAND, 2, &command);
            command |= PCI_CMD_IO_ENABLE |
                PCI_CMD_MEM_ENABLE | PCI_CMD_MASTER_ENABLE;

            /* Make sure the "disable INTx" bit is cleared too. */
            command &= (UINT16)~PCI_CMD_INTX_DISABLE;

            (void) VXB_PCI_CFG_WRITE(pDev, pHardInfo,
                PCI_CFG_COMMAND, 2, command);

            /* Clear any error status bits too. */
            (void) VXB_PCI_CFG_WRITE(pDev, pHardInfo,
                PCI_CFG_STATUS, 2, 0xFFFF);

            (void) vxbDevAdd (pDev, pVxbDev);

            /* Proceed to next device if this is a single function device */
            if (function == 0)
                {
                (void) VXB_PCI_CFG_READ (pDev, pHardInfo,
                    PCI_CFG_HEADER_TYPE, 1, &btemp);

                if ((btemp & PCI_HEADER_MULTI_FUNC) == 0)
                    break; /* No more functions - proceed to next PCI device */
                }
            }
        }

    vxbMemFree (pPciDev);
    vxbMemFree (pHardInfo);
    vxbMemFree (pName);

    return (OK);
    }

/******************************************************************************
*
* vxbPciDevFree - free a PCI device
*
* This routine free a  PCI device
*
* RETURNS: VOID
*
* ERRNO
*
*/

LOCAL STATUS vxbPciDevFree
    (
    VXB_DEV_ID pChild
    )
    {
    PCI_HARDWARE * pPciDevInfo;

    pPciDevInfo = vxbDevIvarsGet (pChild);

    if (pPciDevInfo == NULL)
        return (ERROR);

    vxbDevIvarsSet (pChild, NULL);

#ifndef _WRS_CONFIG_VXBUS_BASIC
    if (vxbDevRemove (pChild) != OK)
        return (ERROR);
#endif /* !_WRS_CONFIG_VXBUS_BASIC */

    /* Release device Ivars. */

    vxbPciResFreeAll (&pPciDevInfo->vxbResList);

    vxbMemFree (pPciDevInfo);

    return (OK);
    }


/******************************************************************************
*
* vxbPciBusShutdown - shutdown a  PCI bus controller or a bridge
*
* This routine shutdown a  PCI bus controller
*
* RETURNS: OK
*
* ERRNO
*
*/

STATUS vxbPciBusShutdown
    (
    VXB_DEV_ID     pDev
    )
    {
    return (OK);
    }

/******************************************************************************
*
* vxbPciBusDetach - detach a  PCI bus controller
*
* This routine detach a  PCI bus controller
*
* RETURNS: OK or ERROR when detach failed
*
* ERRNO
*
*/

STATUS vxbPciBusDetach
    (
    VXB_DEV_ID pDev
    )
    {
    VXB_DEV_ID pChild;
    PCI_ROOT_CHAIN * pciRootChain;

    /* Remove child devices */

    FOREVER
        {

        /*
         * Each time we remove a child, the list shrinks by one
         * member, so we keep trying to grab the first child from
         * the list until there's none left.
         */

        pChild = vxbDevFirstChildGet (pDev);

#ifdef _WRS_CONFIG_PCIE_AER
        vxbPcieAerCapDisable(pDev);
#endif

        if (pChild == NULL)
            break;

        if (vxbPciDevFree (pChild) != OK)
            return (ERROR);
        }

    /* Release our per-device context */

    pciRootChain = vxbDevSoftcGet (pDev);

    if (pciRootChain != NULL)
        vxbMemFree (pciRootChain);

    return (OK);
    }

/******************************************************************************
*
* vxbPciAutoConfig - automatically configure and announce all PCI
*
* This routine scans all of PCI device under the controller bus and announces
* them to the VxBus subsystem. It may also initiate resource autoconfiguration
* if the controller specifies that it's needed.
*
* This routine is only called from top-level PCI bus drivers. The PCI bridge
* driver will use vxbPciBusAddDev() directly.
*
* RETURNS: OK or ERROR
*
* ERRNO
*
*/

STATUS vxbPciAutoConfig
    (
    VXB_DEV_ID  pDev
    )
    {
    PCI_ROOT_CHAIN * pSoftc;
    PCI_ROOT_RES * pRes;
#ifdef _WRS_CONFIG_FDT
    VXB_FDT_DEV * pFdt;
    struct fdtPciRange range;
#endif
    VXB_PCI_CTRL   * pPciCtrl;

    pSoftc = vxbDevSoftcGet (pDev);
    pRes = pSoftc->pRootRes;

#ifdef _WRS_CONFIG_FDT
    /*
     * Someone decided that rather than just storing the CPU-side
     * base value from the PCI bus range in the .dts file, they
     * would store a pre-calculated adjustment offset instead.
     * This means that all existing PCI bus drivers don't save the
     * CPU base (baseParent) value when they read the bus ranges
     * from the FDT: instead they store an "adjust" value calculated
     * using CPU base - PCI base.
     *
     * This might seem like a good idea, but it's not.
     *
     * The CPU base + length and the PCI base + length form
     * translation windows. Eventually, we will need to translate
     * addresses from one window to the other. We can not do
     * this correctly if we don't know the window base addresses.
     * The pre-computed adjustment value doesn't help: we can't
     * tell if the adjustment is positive or negative, meaning
     * that unless the window base values are chosen in a
     * specific, limited, and not immediately obvious way, the
     * PCI to CPU address translation can generate bogus
     * results.
     *
     * The PCI bus drivers need to be updated to save the
     * parentBase values during their attach routines. However
     * until they're all fixed, we need to apply a workaround here.
     * If we are doing resource autoconfiguration, and the parent
     * PCI bus driver doesn't supply values for parentBase in the
     * resource pool entries, we retrieve the parentBase values
     * ourselves here.
     *
     * Note that we can't do this in the vxbPciSetBarResPool()
     * function because someone _also_ decided that that function
     * shouldn't accept the PCI bus device handle as an argument.
     */

    pFdt = vxbFdtDevGet (pDev);

    if ((pFdt != NULL) && (pSoftc->autoConfig == TRUE))
        {
        if (pRes->barRes[PCI_ROOT_RES_IO_IDX].cpuBase == 0)
            {
            if (vxbFdtPciRanges (pFdt, FDT_PCI_SPA_IO, &range) == OK)
                pRes->barRes[PCI_ROOT_RES_IO_IDX].cpuBase =
                    range.baseParent;
            }
        if (pRes->barRes[PCI_ROOT_RES_MEM32_IDX].cpuBase == 0)
            {
            if (vxbFdtPciRanges (pFdt, FDT_PCI_SPA_MEM, &range) == OK)
                pRes->barRes[PCI_ROOT_RES_MEM32_IDX].cpuBase =
                    range.baseParent;
            }
        if (pRes->barRes[PCI_ROOT_RES_PREMEM32_IDX].cpuBase == 0)
            {
            if (vxbFdtPciRanges (pFdt, FDT_PCI_SPA_MEM_PRE, &range) == OK)
                pRes->barRes[PCI_ROOT_RES_PREMEM32_IDX].cpuBase =
                    range.baseParent;
            }
        }
#endif /* _WRS_CONFIG_FDT */

    /*
     * This routine is only called by the driver for the root complex or host
     * bridge at the root of a given PCI device tree. We preserve a handle
     * to its device in the root chain structure, which will be inherited
     * by all PCI bridges that are discovered in the tree.
     */

    pSoftc->pRootDev = pDev;

    if (!vxbPciCtrlListInited)
        {
        SLL_INIT(&vxbPciCtrlList);
        vxbPciCtrlListInited = TRUE;
        }

    pPciCtrl = (VXB_PCI_CTRL *)vxbMemAlloc(sizeof(VXB_PCI_CTRL));

    if (pPciCtrl == NULL)
        return ERROR;

    pPciCtrl->pDev = pDev;

    SLL_PUT_AT_TAIL (&vxbPciCtrlList, &pPciCtrl->node);
    
    if (pSoftc->autoConfig == TRUE)
        {
        if (_func_vxbPciAutoConfig != NULL)
            _func_vxbPciAutoConfig (pDev);
        else
            return (ERROR);
        }

    return (vxbPciBusAddDev (pDev, pRes->baseBusNumber));
    }

/*******************************************************************************
*
* vxbPciAddr2Cpu - translate the PCI address to CPU address
*
* This function is a helper routine for calculating the CPU-side address of
* a PCI BAR address. When autoconfiguration is enabled, we are responsible for
* assigning the PCI device BAR values. The address programmed into a BAR
* corresponds to a physical address in the host CPU's address space. The BSP
* by way of the .dts file) allocates some CPU-side address space regions for
* each different kind of BAR (I/O or memory mapped). For each CPU address
* space range, there is a corresponding PCI-bus side range. For example:
*
*    ranges = <0x2000000 0 0x60000000 0x60000000 0 0xa000000
*              0x1000000 0 0x6a000000 0x6a000000 0 0x100000>;
*
* Here, the memory mapped BAR is listed first. The CPU-side base address is
* 0x60000000. The corresponding PCI-side address is also 0x60000000, and the
* range is 0xa000000 in size. This means that the first device discovered
* on the bus will have its memory-mapped BAR programmed with an address of
* 0x60000000, and that BAR will be mapped to address 0x60000000 on the CPU
* as well. (Thus, when software reads or writes at address 0x60000000, it
* will be reading/writing the device's memory mapped registers.)
*
* In this case, there is a 1:1 relationship between the CPU-side and PCI-side
* addresses. This is the simplest case. However, the PCI root complex or host
* bridge may offer the ability to provide address translation, such that the
* PCI-side addresses may be different from the CPU-side addresses. For example,
* we could have:
*
*    ranges = <0x2000000 0 0x90000000 0x60000000 0 0xa000000
*              0x1000000 0 0x6a000000 0x6a000000 0 0x100000>;
*
* In this case, the base address of the CPU-side memory-mapped BAR window is
* 0x90000000. But the PCI-side range still starts at 0x60000000. (The device
* does not realize that there isn't a 1:1 translation anymore.)
*
* If the .dts file specifies such a non-identity mapping, we must account
* for it here. The simplest method is to calculate the offset of the BAR
* from the base address of the PCI-side window and add that to the base
* address of the CPU-side window.
*
* \NOMANUAL
*
* RETURNS: translated CPU-side address
*
* ERRNO: N/A
*/

LOCAL PHYS_ADDR vxbPciAddr2Cpu
    (
    VXB_DEV_ID     pDev,        /* device info */
    UINT64         pciAddr      /* PCI address */
    )
    {
    PCI_ROOT_CHAIN * pSoftc;
    PCI_ROOT_RES   * pciRootRes;
    UINT64 phyAdjust = 0;
    UINT8 i, j;

    /* Get our per-device structure. */

    pSoftc = vxbDevSoftcGet (pDev);

    /* Now get the root node's per-device structure. */

    pSoftc = vxbDevSoftcGet (pSoftc->pRootDev);

    if (pSoftc->autoConfig == FALSE)
        return ((PHYS_ADDR)pciAddr);

    /*
     * The .dts file will have specified one or more resource ranges.
     * We need to iterate over the range list and find the one which
     * contains the PCI address we're trying to translate.
     */

    for (i = 0; i < pSoftc->segCount; i++)
        {
        pciRootRes = (PCI_ROOT_RES *)&pSoftc->pRootRes[i];

        for (j = 0; j < PCI_ROOT_RES_MAX_IDX; j++)
            {
            if ((pciAddr >= pciRootRes->barRes[j].start) &&
                (pciAddr < pciRootRes->barRes[j].end))
                {
                phyAdjust = pciAddr - pciRootRes->barRes[j].start;
                phyAdjust += pciRootRes->barRes[j].cpuBase;
                break;
                }
            }
        }

    return ((PHYS_ADDR)phyAdjust);
    }

/*****************************************************************************
*
* vxbPciDevMatch - match the PCI type device with driver.
*
* This function match the PCI type device with driver, the PCI device has
* PCI_HARDWARE to describe the device, drive also have identify table which list
* supported devices, this routine compare them to check whether they are match.
*
* RETURNS: OK when matched, otherwise ERROR;
*
* ERRNO: N/A
*/

STATUS vxbPciDevMatch
    (
    VXB_DEV_ID                      pDev,             /* device to do match */
    const VXB_PCI_DEV_MATCH_ENTRY * pMatchTbl,        /* pointer to match table */
    VXB_PCI_DEV_MATCH_ENTRY **      pMatchedEntry     /* best matched entry */
    )
    {
    PCI_HARDWARE * pPciDevInfo ;

    if ((pDev == NULL) || (pMatchTbl == NULL))
        return ERROR;

    pPciDevInfo = (PCI_HARDWARE *)vxbDevIvarsGet(pDev);

    if (pPciDevInfo == NULL)
        return ERROR;

    while (pMatchTbl->vendorId != 0)
        {

        if ((pMatchTbl->vendorId == pPciDevInfo->pciVendId) &&
            (pMatchTbl->deviceId == pPciDevInfo->pciDevId))
            {
            if (pMatchedEntry != NULL)
                {
                *pMatchedEntry = (VXB_PCI_DEV_MATCH_ENTRY *)pMatchTbl;
                }
            return OK;
            }

        pMatchTbl++;
        }

    return ERROR;
    }

/*******************************************************************************
*
* vxbPciConfigBdfPack - pack parameters for the Configuration Address Register
*
* This routine packs three parameters into one integer for accessing the
* Configuration Address Register
*
* RETURNS: packed integer encoded version of bus, device, and function numbers.
*
* ERRNO
*
*/

int vxbPciConfigBdfPack
    (
    int busNo,    /* bus number */
    int deviceNo, /* device number */
    int funcNo    /* function number */
    )
    {
    return(((busNo    << 16) & 0x00ff0000) |
       ((deviceNo << 11) & 0x0000f800) |
       ((funcNo   << 8)  & 0x00000700));
    }

/******************************************************************************
*
* vxbPciExtCapFind - search for an extended capability the in ECP linked list
*
* This routine searches for an extended capability in the linked list of
* capabilities in config space for the device specified by <pDev>. If found,
* the offset of the first byte of the capability of interest in config space
* is returned via <pOffset>.
*
* RETURNS: OK if Extended Capability found, ERROR otherwise
*
* ERRNO: N/A
*
*/

STATUS vxbPciExtCapFind
    (
    VXB_DEV_ID pDev,
    UINT8 requestedCapId,	/* Extended capabilities ID to search for */
    UINT8 * pOffset		/* returned config space offset */
    )
    {
    STATUS retStat = ERROR;
    UINT16 tmpStat;
    UINT8 tmpOffset;
    UINT8 hdrType;
    UINT8 capPtr;
    UINT8 capOffset = 0;
    UINT8 capId = 0;

    /* Test for extended capabilities support. */

    VXB_PCI_BUS_CFG_READ(pDev, PCI_CFG_STATUS, 2, tmpStat);

    if ((tmpStat == (UINT16)0xFFFF) || ((tmpStat & PCI_STATUS_NEW_CAP) == 0))
        {
        return (retStat);
        }

    VXB_PCI_BUS_CFG_READ (pDev, PCI_CFG_HEADER_TYPE, 1, hdrType);

    hdrType &= PCI_HEADER_TYPE_MASK;

    /* Obtain start pointer of capabilities list. */

    switch (hdrType)
        {
        case PCI_HEADER_TYPE0:
        case PCI_HEADER_TYPE_BRIDGE:
            capPtr = PCI_CFG_CAP_PTR;
            break;
        case PCI_HEADER_PCI_CARDBUS:
            capPtr = PCI_CFG_CB_CAP_PTR;
            break;
        default:
            return (ERROR);
            break;
        }

    VXB_PCI_BUS_CFG_READ(pDev, capPtr, 1, capOffset);

    capOffset = (UINT8)(capOffset & ~0x02);

    /* Bounds check the ECP offset */

    if (capOffset < 0x40)
        {
        return (retStat);
        }

    /* Traverse the Extended Cap items in the linked list */

    while (capOffset != 0)
        {
        /* Get the Capability ID and check */

        VXB_PCI_BUS_CFG_READ(pDev, (int)capOffset, 1, capId);

        if (capId == requestedCapId)
            {
            *pOffset = (UINT8)(capOffset + 0x02);
            retStat = OK;
            break;
            }

        /* Get the offset to the next New Capabilities item */

        tmpOffset = (UINT8)(capOffset + 0x01);

        VXB_PCI_BUS_CFG_READ(pDev, (int)tmpOffset, 1, capOffset);
        }

    return (retStat);
    }

/******************************************************************************
*
* vxbPciPrivateExtCapFind - search for an extended capabilities
*
* This routine searches for an extended capability in the linked list of
* capabilities in config space for the device specified by <pDev>. If found,
* the offset of the first byte of the capability of interest in config space
* is returned via <pOffset>. It differs from vxbPciExtCapFind() in that it
* uses the hardInfo structure so that it can be called for devices that
* haven't been tied to the VxBus tree yet.
*
* RETURNS: OK if Extended Capability found, ERROR otherwise
*
* ERRNO: N/A
*
* \NOMANUAL
*/

LOCAL STATUS vxbPciPrivateExtCapFind
    (
    VXB_DEV_ID pDev,
    PCI_HARDWARE * hardInfo,
    UINT8 requestedCapId,	/* Extended capabilities ID to search for */
    UINT8 * pOffset		/* returned config space offset */
    )
    {
    STATUS retStat = ERROR;
    UINT16 tmpStat;
    UINT8 tmpOffset;
    UINT8 hdrType;
    UINT8 capPtr;
    UINT8 capOffset = 0;
    UINT8 capId = 0;

    /* Test for extended capabilities support. */

    (void) VXB_PCI_CFG_READ(pDev, hardInfo, PCI_CFG_STATUS, 2, &tmpStat);

    if ((tmpStat == (UINT16)0xFFFF) || ((tmpStat & PCI_STATUS_NEW_CAP) == 0))
        {
        return (retStat);
        }

    (void) VXB_PCI_CFG_READ (pDev, hardInfo, PCI_CFG_HEADER_TYPE, 1, &hdrType);

    hdrType &= PCI_HEADER_TYPE_MASK;

    /* Obtain start pointer of capabilities list. */

    switch (hdrType)
        {
        case PCI_HEADER_TYPE0:
        case PCI_HEADER_TYPE_BRIDGE:
            capPtr = PCI_CFG_CAP_PTR;
            break;
        case PCI_HEADER_PCI_CARDBUS:
            capPtr = PCI_CFG_CB_CAP_PTR;
            break;
        default:
            return (ERROR);
            break;
        }

    (void) VXB_PCI_CFG_READ(pDev, hardInfo, capPtr, 1, &capOffset);

    capOffset = (UINT8)(capOffset & ~0x02);

    /* Bounds check the ECP offset */

    if (capOffset < 0x40)
        {
        return (retStat);
        }

    /* Traverse the Extended Cap items in the linked list */

    while (capOffset != 0)
        {
        /* Get the Capability ID and check */

        (void) VXB_PCI_CFG_READ(pDev, hardInfo, (int)capOffset, 1, &capId);

        if (capId == requestedCapId)
            {
            *pOffset = (UINT8)(capOffset + 0x02);
            retStat = OK;
            break;
            }

        /* Get the offset to the next New Capabilities item */

        tmpOffset = (UINT8)(capOffset + 0x01);

        (void) VXB_PCI_CFG_READ(pDev, hardInfo, (int)tmpOffset, 1, &capOffset);
        }

    return (retStat);
    }

#ifdef _WRS_CONFIG_VXBUS_USER

/******************************************************************************
*
* vxbPciCfgReadStub - PCI config read method stub for user-mode drivers
*
* This function serves as the kernel-side stub for the PCI configuration read
* method when called from a user-mode driver. The user-mode method call
* passthrough mechanism requires that method call requests from user-space
* drivers be handled by a proxy stub routine inside the kernel. The stub
* decodes the arguments and then calls the underlying method using the
* supplied arguments.
*
* RETURNS: OK PCI configuration access succeeds, otherwise ERROR
*
* ERRNO: N/A
*/

LOCAL STATUS vxbPciCfgReadStub
    (
    void * pArgs,
    UINT32 argLen
    )
    {
    VXB_PCI_CFG_ARGS * pCfg;
    VXB_DEV_ID pBus;
    VXB_DEV_ID pChild;
    PCI_HARDWARE * pIvars;
    STATUS r;
    void * p;
    UINT8 b;
    UINT16 w;
    UINT32 l;

    pCfg = (VXB_PCI_CFG_ARGS *)pArgs;

    switch (pCfg->vxbWidth)
        {
        case 1:
            p = (void *)&b;
            break;
        case 2:
            p = (void *)&w;
            break;
        case 4:
            p = (void *)&l;
            break;
        default:
            return (ERROR);
            /* NOTREACHED */
            break;
        }

    pChild = vxbDevAcquireBySerial (pCfg->vxbNodeKey);
    if (pChild == NULL)
        return (ERROR);
    pIvars = (PCI_HARDWARE *)vxbDevIvarsGet (pChild);
    pBus = vxbDevAcquireBySerial (pCfg->vxbTargetKey);
    if (pBus == NULL || pIvars == NULL)
        {
        if (pBus != NULL)
            vxbDevRelease (pBus);
        vxbDevRelease (pChild);
        return (ERROR);
        }

    r = VXB_PCI_CFG_READ(pBus, pIvars, pCfg->vxbOffset,
        pCfg->vxbWidth, p);

    switch (pCfg->vxbWidth)
        {
        case 1:
            pCfg->vxbVal = b;
            break;
        case 2:
            pCfg->vxbVal = w;
            break;
        case 4:
            pCfg->vxbVal = l;
            break;
        default:
            break;
        }

    vxbDevRelease (pChild);
    vxbDevRelease (pBus);

    return (r);
    }

/******************************************************************************
*
* vxbPciCfgWriteStub - PCI config write method stub for user-mode drivers
*
* This function serves as the kernel-side stub for the PCI configuration write
* method when called from a user-mode driver. The user-mode method call
* passthrough mechanism requires that method call requests from user-space
* drivers be handled by a proxy stub routine inside the kernel. The stub
* decodes the arguments and then calls the underlying method using the
* supplied arguments.
*
* RETURNS: OK PCI configuration access succeeds, otherwise ERROR
*
* ERRNO: N/A
*/

LOCAL STATUS vxbPciCfgWriteStub
    (
    void * pArgs,
    UINT32 argLen
    )
    {
    VXB_PCI_CFG_ARGS * pCfg;
    VXB_DEV_ID pBus;
    VXB_DEV_ID pChild;
    PCI_HARDWARE * pIvars;
    STATUS r;

    pCfg = (VXB_PCI_CFG_ARGS *)pArgs;

    pChild = vxbDevAcquireBySerial (pCfg->vxbNodeKey);
    if (pChild == NULL)
        return (ERROR);
    pIvars = (PCI_HARDWARE *)vxbDevIvarsGet (pChild);
    pBus = vxbDevAcquireBySerial (pCfg->vxbTargetKey);
    if (pBus == NULL || pIvars == NULL)
        {
        if (pBus != NULL)
            vxbDevRelease (pBus);
        vxbDevRelease (pChild);
        return (ERROR);
        }

    r = VXB_PCI_CFG_WRITE(pBus, pIvars, pCfg->vxbOffset,
        pCfg->vxbWidth, pCfg->vxbVal);

    vxbDevRelease (pChild);
    vxbDevRelease (pBus);

    return (r);
    }

LOCAL VXB_METHOD_LINK vxbPciCfgReadMethod =
    {
        { "vxbPciCfgRead" },		/* Method name */
        vxbPciCfgReadStub		/* Method function */
    };

LOCAL VXB_METHOD_LINK vxbPciCfgWriteMethod =
    {
        { "vxbPciCfgWrite" },		/* Method name */
        vxbPciCfgWriteStub		/* Method function */
    };

VXB_METHOD_DEF(vxbPciCfgReadMethod);
VXB_METHOD_DEF(vxbPciCfgWriteMethod);

#endif /* _WRS_CONFIG_VXBUS_USER */


/******************************************************************************
*
* vxbPcieExtCapFind - search for an extended capability
*
* This routine searches for an extended capability in the linked list of
* capabilities in config space for the device specified by <pDev>. If found,
* the offset of the first byte of the capability of interest in config space
* is returned via <pOffset>. Possible values for @cap:
*
*  PCI_EXT_CAP_ID_ERR		Advanced Error Reporting
*  PCI_EXT_CAP_ID_VC		  Virtual Channel
*  PCI_EXT_CAP_ID_DSN		Device Serial Number
*  PCI_EXT_CAP_ID_PWR		Power Budgeting
*
* RETURNS: OK if Extended Capability found, ERROR otherwise
*
* ERRNO: N/A
*
*/

STATUS vxbPcieExtCapFind
    (
    VXB_DEV_ID  pDev,
    UINT16      capability,
    UINT16 *    capreg
    )
    {
    UINT32 ecap = 0;
    UINT16 ptr;
    STATUS retStat = ERROR;

    /* Only supported for PCI-express devices. */

    (void) vxbPciExtCapFind (pDev, PCI_EXT_CAP_EXP, (UINT8*)&ecap);

    if(!ecap)
        return retStat;

    ptr = PCI_EXT_CAP_BASE;

    ecap = 0;

    VXB_PCI_BUS_CFG_READ(pDev, ptr, 4, ecap);

    if (ecap == 0xffffffff || ecap == 0)
        return retStat;

    for (;;)
        {
        if (PCI_EXT_CAP_ID(ecap) == capability)
            {
        	if (capreg != NULL)
        	    {
        	  	*capreg = ptr;
            	retStat = OK;
                }
            break;
            }

        ptr = PCI_EXT_CAP_NEXT(ecap);

        if (ptr == 0)
            break;

        VXB_PCI_BUS_CFG_READ(pDev, ptr, 4, ecap);
        }

    return retStat;
    }

/******************************************************************************
*
* vxbPcieDevTypeGet - indicates the type of PCI Express logical device
*
* This routine indicates the type of PCI Express logical device by reading
* PCI Express Capabilities Register in Configuration Space.
*
* RETURNS: logical device type, ERROR otherwise
*
* ERRNO: N/A
*
*/

int vxbPcieDevTypeGet
    (
    VXB_DEV_ID pDev
    )
    {
    UINT8 pos = 0;
    UINT16 capReg=0;
    int type=0;

    (void) vxbPciExtCapFind (pDev, PCI_EXT_CAP_EXP, &pos);

    if (!pos)
        return ERROR;

    /* PCI Express Capabilities Register  */

    VXB_PCI_BUS_CFG_READ(pDev, pos, 2, capReg);

    type = ((capReg & PCI_EXP_CAP_PORT_TYPE) >> 4);

    return type;
}
