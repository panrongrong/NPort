/* vxbPciAutoCfg.c - PCI auto config file */

/*
 * Copyright (c) 2015-2017 Wind River Systems, Inc.
 *
 * The right to copy, distribute, modify or otherwise make use
 * of this software may be licensed only pursuant to the terms
 * of an applicable Wind River license agreement.
 */

/*
modification history
--------------------
21apr17,syt  updated vxbPciResourceSort to slove memory spcae allocation
             issue.(V7PRO-3693)
26aug16,y_y  add prefetch memory 64bit support. (V7PRO-3155)
28jul16,y_y  Move vxbPciCtrlList to vxbPci.c to support IA. (V7PRO-3215)
10aug15,wap  Move _func_vxbPciAutoConfig to vxbPci.c to break linker
             dependency
07aug15,wap  Fix copyright date
31jul15mwap  Fix size calculation for bridge windows to properly account
             for alignment constraints, clean up some debug messages
17jun15,wap  Adjust types in some debug print statements to match
             changes to structure member types, initialize debug
             level to 0.
10jun15,wap  Make bridge detection logic consistent with other cases,
             make PCI resource-related variables fixed-size types,
             make consistent use of resource protection semaphore
             (instead of alternately using the semaphore in one
             function and intCpuLock() in another)
08jun15,wap  Broken out from vxbPci.c
*/

/*
DESCRIPTION
This library contains the support routines for PCI autoconfiguration. This
code is used on platforms where VxWorks must initialize the PCI BAR values
for devices itself, rather than relying on pre-existing configiration
provided by BIOS or system firmware. 
*/

/* includes */

#include <vxWorks.h>
#include <errnoLib.h>
#include <iv.h>
#include <taskLib.h>
#include <string.h>
#include <stdio.h>
#include <logLib.h>
#include <intLib.h>
#include <vmLib.h>
#include <hwif/vxBus.h>
#include <hwif/buslib/vxbPciLib.h>
#include <hwif/buslib/vxbPciMsi.h>
#include <hwif/buslib/pciDefines.h>
#include <subsys/int/vxbIntLib.h>

/* Debug macro */

#undef  PCI_DEBUG
#ifdef  PCI_DEBUG

#include <private/kwriteLibP.h>

LOCAL int debugLevel = 0;

#ifdef  LOCAL
#undef  LOCAL
#define LOCAL
#endif

#define PCI_LOG_MSG(lvl, fmt, args...)        \
    do {                                      \
        if (debugLevel >= lvl)                \
            {                                 \
            if (_func_kprintf != NULL)        \
                _func_kprintf (fmt, ##args);  \
            }                                 \
       } while (FALSE)
#else
#define PCI_LOG_MSG(lvl, fmt, args...)

#endif  /* PCI_DEBUG */

/* defines */

#define PCI_DEF_LTIMER     0x40

#undef ROUND_UP
#define ROUND_UP(n, align)  ((n + align - 1) & (~(align - 1)))

/* declaration */

IMPORT STATUS (*_func_vxbPciAutoConfig)(VXB_DEV_ID);

LOCAL STATUS vxbPciAutoConfigFunc (VXB_DEV_ID);

LOCAL SEM_ID pciResSemId;   /* semaphore */

/***********************************************************************
*
* vxbPciConfigInit - Init VxWorks PCI Auto config library
*
*  This routine Init VxWorks PCI Auto config library
*
*  RETURNS: OK, always
*/

void vxbPciConfigInit (void)
    {
    /* Create semaphore for locking access to resource */

    pciResSemId = semMCreate (SEM_Q_PRIORITY | SEM_DELETE_SAFE);

    /* Can't create semaphore */

    if (pciResSemId == NULL)
        {
        PCI_LOG_MSG (0, "vxbPciConfigInit create semaphore failed\n");
        return;
        }

    _func_vxbPciAutoConfig = vxbPciAutoConfigFunc;
    }

/*******************************************************************************
*
* vxbPciBarAlloc - alloc PCI/PCIe device's memory or IO space.
*
* This routine allocates PCI/PCIe device's memory or IO space.
*
* RETURNS: OK or ERROR if allocation failed.
*
* ERRNO
*/

LOCAL STATUS vxbPciBarAlloc
    (
    PCI_RES *          pRes, /* parent resource*/
    PCI_MMIO_DESC    * pDesc
    )
    {
    PCI_ADDR_RESOURCE * pCur;
    PCI_ADDR_RESOURCE * pNext;
    PCI_ADDR_RESOURCE * pPrev;
    PCI_ADDR_RESOURCE * pNew;
    UINT64              alignStart = 0;
    UINT32              nodeIndex = 0;
    PCI_BAR_ADR_POOL * pPool = &pRes->pool;

    if (pDesc->align == 0)
        pDesc->align = sizeof(UINT32);

    if (pDesc->size== 0)
        {
        return ERROR;
        }

    /* Take control of bar resource */

    if (semTake (pciResSemId, WAIT_FOREVER) != OK)
        return ERROR;

    for (pCur  = (PCI_ADDR_RESOURCE *) DLL_FIRST (&pPool->list);
         pCur != NULL;
         pCur  = pNext)
        {
        nodeIndex++;

        alignStart =
            ROUND_UP (pCur->start + pCur->size, pDesc->align);

        pNext = (PCI_ADDR_RESOURCE *) DLL_NEXT (&pCur->node);

        if ((pNext != NULL) &&
            (pNext->start > alignStart) &&
            ((pNext->start - alignStart) >= pDesc->size))
            break;
        }

    if (nodeIndex == 0)
        alignStart = pPool->base;

    pNew = (PCI_ADDR_RESOURCE *) vxbMemAlloc (sizeof (PCI_ADDR_RESOURCE));

    if (pNew == NULL)
        {
        PCI_LOG_MSG (0, "PCI BAR pool node allocation failed\n");
        (void) semGive (pciResSemId);

        return (ERROR);
        }

    if ((nodeIndex == 0) || (nodeIndex == pPool->nodeCnt))
        {
        pPrev = (PCI_ADDR_RESOURCE *) DLL_LAST (&pPool->list);
        }
    else
        {
        pPrev = pCur;
        }

    DLL_INSERT(&pPool->list, &pPrev->node, &pNew->node);

    pPool->nodeCnt ++;

    pNew->start       = alignStart;
    pNew->size        = pDesc->size;
    pNew->align       = pDesc->align;
    pDesc->addr      = alignStart;

    /* Release semaphore */

    (void) semGive (pciResSemId);

    return (OK);
    }

/*******************************************************************************
*
* vxbPciResInsert - insert childern's resource to parent resource list.
*
* This routine inserts childern's resource to parent resource list.
*
* RETURNS: N/A
*
* ERRNO
*/

LOCAL void vxbPciResInsert
    (
    VXB_DEV_ID  busCtrlID,
    DL_LIST   * pParentList, /* parent resource pool */
    PCI_RES   * pResInsert   /* resource need to insert */
    )
    {
    DL_NODE * pNode;
    PCI_RES * pRestmp;
    PCI_RES * pResNext;

    /* Take control of bar resource */

    if (semTake (pciResSemId, WAIT_FOREVER) != OK)
        return;

    if (DLL_EMPTY (pParentList))
        {
        DLL_ADD((DL_LIST *)pParentList, &pResInsert->node);

        (void) semGive (pciResSemId);

        return;
        }
    else
        {
        for (pNode = DLL_FIRST (pParentList); pNode != NULL;
            pNode = DLL_NEXT (pNode))
            {
            pRestmp = (PCI_RES *)pNode;

            if (pRestmp->align >= pResInsert->align)
                {
                pResNext = (PCI_RES *)DLL_NEXT(pNode);

                if (pResNext == NULL)
                    {

                    DLL_INSERT(pParentList, &pRestmp->node, &pResInsert->node);

                    (void) semGive (pciResSemId);

                    return;
                    }
                else
                    {
                    if (pResNext->align < pResInsert->align)
                        {
                        DLL_INSERT(pParentList, &pRestmp->node, &pResInsert->node);

                        (void) semGive (pciResSemId);

                        return;
                        }
                    }
                }
            else /* save the resource to head */
                {
                DLL_INSERT(pParentList, NULL, &pResInsert->node);

                (void) semGive (pciResSemId);

                return;
                }
            }
        }

    PCI_LOG_MSG(1000, "vxbPciResInsert(): Miss Size 0x%x  Align 0x%x type 0x%x \n",
        pResInsert->size, pResInsert->align, pResInsert->type);

    (void) semGive (pciResSemId);

    return ;
    }

/**********************************************************************
*
* vxbPciResCheck - check for 64-bit BARs
*
* This routine checks to see if any devices require 64-bit resources
*
* RETURNS: N/A
*
* ERRNO
*
*/

LOCAL void vxbPciResCheck
    (
    VXB_DEV_ID     busCtrlID,
    PCI_DEVICE   * pParent,
    BOOL         * pSup64
    )
    {
    PCI_DEVICE* pPciDev;
    DL_NODE *   pNode  = NULL;
    UINT32 i;

    /* if bridge bar doesn't support 64bit then assign false and return */

    if ((pParent->resource[PCI_BRIDGE_PRE_MEM_INDEX].type & PCI_BAR_MEM_ADDR64) != PCI_BAR_MEM_ADDR64)
        {
        *pSup64 = FALSE;
        return;
        }

    for (pNode = DLL_FIRST((DL_LIST *)&pParent->children); pNode != NULL;
        pNode = DLL_NEXT(pNode))
        {
        pPciDev = (PCI_DEVICE * )pNode;

        for (i = 0; i < PCI_MAX_RESOURCE_NUMBER; i ++)
            {
            if ((i >= PCI_BRIDGE_MEM_BASE_INDEX) &&
                (i <= PCI_BRIDGE_PRE_MEM_INDEX))
                {
                continue;
                }

            if (pPciDev->resource[i].type == (PCI_BAR_MEM_ADDR64 | PCI_BAR_MEM_PREFETCH))
                {
                *pSup64 = TRUE;
                return;
                }
            }

        if (pPciDev->type == PCI_BRIDGE_TYPE)
            {
            vxbPciResCheck (busCtrlID, pPciDev, pSup64);
            }
        }
    }

/**********************************************************************
*
* vxbPciResAdjust  - adjust prefetch memory
*
* This routine adjusts prefetch memory
*
* RETURNS: N/A
*
* ERRNO
*
*/

LOCAL void vxbPciResAdjust
    (
    VXB_DEV_ID     busCtrlID,
    PCI_ROOT_RES * pciRootRes,
    PCI_DEVICE   * pPciDummy
    )
    {
    PCI_DEVICE* pPciDev;
    DL_NODE *   pNode = NULL;
    BOOL        sup64 = FALSE;

    for (pNode = DLL_FIRST((DL_LIST *)&pPciDummy->children); pNode != NULL;
        pNode = DLL_NEXT(pNode))
        {
        pPciDev = (PCI_DEVICE * )pNode;

        if ((pciRootRes->barRes[PCI_ROOT_RES_PREMEM64_IDX].size == 0) &&
            (pciRootRes->barRes[PCI_ROOT_RES_PREMEM32_IDX].size == 0))
            {

            /* if no prefetch resource, just assign the none-prefetch resource */

            PCI_LOG_MSG(1000, "bridge  (%d:%d:%d) doesn't support prefetch memory\n",
                         pPciDev->bus, pPciDev->device, pPciDev->func);

            pPciDev->preIndex = PCI_ROOT_RES_MEM32_IDX;
            pPciDummy->preIndex = PCI_ROOT_RES_MEM32_IDX;
            return;
            }

        vxbPciResCheck (busCtrlID, pPciDev, &sup64);

        if ((pciRootRes->barRes[PCI_ROOT_RES_PREMEM64_IDX].size != 0) &&
            (sup64 == TRUE))
            {
            PCI_LOG_MSG(1000, "bridge  (%d:%d:%d) use 64 bit prefetch memory \n",
                         pPciDev->bus, pPciDev->device, pPciDev->func);

            pPciDev->preIndex = PCI_ROOT_RES_PREMEM64_IDX;
            pPciDummy->preIndex = PCI_ROOT_RES_PREMEM64_IDX;
            }
         else if (pciRootRes->barRes[PCI_ROOT_RES_PREMEM32_IDX].size != 0)
            {
            PCI_LOG_MSG(1000, "bridge  (%d:%d:%d) use 32 bit prefetch memory \n",
                         pPciDev->bus, pPciDev->device, pPciDev->func);

            pPciDev->preIndex = PCI_ROOT_RES_PREMEM32_IDX;
            pPciDummy->preIndex = PCI_ROOT_RES_PREMEM32_IDX;
            }
         else
            {
            PCI_LOG_MSG(1000, "bridge  (%d:%d:%d) use none prefetch memory \n",
                         pPciDev->bus, pPciDev->device, pPciDev->func);

            pPciDev->preIndex = PCI_ROOT_RES_MEM32_IDX;
            pPciDummy->preIndex = PCI_ROOT_RES_MEM32_IDX;
            }
        }
    }

/**********************************************************************
*
* vxbPciResourceSort - sort the bar size from large to small
*
* This routine sort the bar size from large to small
*
* RETURNS: N/A
*
* ERRNO
*
*/

LOCAL void vxbPciResourceSort
    (
    VXB_DEV_ID     busCtrlID,
    PCI_DEVICE   * pParent,
    UINT8          preIndex
    )
    {
    DL_NODE * pNode  = NULL;
    DL_NODE * pNode2  = NULL;
    PCI_RES * pRestmp;
    UINT32      i;
    UINT32      listIndex;
    PCI_DEVICE* pPciDev;
    UINT64      align;
    BOOL        firstNode;

    for (pNode = DLL_FIRST((DL_LIST *)&pParent->children); pNode != NULL;
        pNode = DLL_NEXT(pNode))
        {
        pPciDev = (PCI_DEVICE * )pNode;

        /* always use the preIndex in root node */

        if (pParent->parent == NULL)
            preIndex = pPciDev->preIndex;

        if (pPciDev->type == PCI_BRIDGE_TYPE)
            {
            vxbPciResourceSort (busCtrlID, pPciDev, preIndex);
            }

        /* bar resource */

        for (i = 0; i < PCI_MAX_RESOURCE_NUMBER; i ++)
            {
            if (pPciDev->resource[i].size != 0)
                {

                if (pPciDev->resource[i].type & PCI_BAR_SPACE_IO)
                    {
                    listIndex = PCI_BRIDGE_IO_BASE_INDEX;
                    }
                else if (pPciDev->resource[i].type & PCI_BAR_MEM_PREFETCH)
                    {
                    if (preIndex == PCI_ROOT_RES_MEM32_IDX)
                        listIndex = PCI_BRIDGE_MEM_BASE_INDEX;
                    else
                        listIndex = PCI_BRIDGE_PRE_MEM_INDEX;
                    }
                else
                    {
                    listIndex = PCI_BRIDGE_MEM_BASE_INDEX;
                    }

                PCI_LOG_MSG(1000, "Insert resource1 (%d:%d:%d) BAR (%d) to (%d:%d:%d) type (%d) size (0x%llx) align(0x%llx) preBar (0x%x)\n",
                    pPciDev->bus, pPciDev->device, pPciDev->func, i,
                    pPciDev->parent->bus, pPciDev->parent->device, pPciDev->parent->device,
                    pPciDev->resource[i].type,
                    pPciDev->resource[i].size,
                    pPciDev->resource[i].align,
                    listIndex);

                 vxbPciResInsert(busCtrlID, (DL_LIST *)(
                     &pPciDev->parent->resource[listIndex].listSubResSort),
                     (PCI_RES *)&pPciDev->resource[i]);
                }
            }

        /* bridge resource */

        for (i = PCI_BRIDGE_MEM_BASE_INDEX; i <= PCI_BRIDGE_PRE_MEM_INDEX; i ++)
            {
            align = (i == PCI_BRIDGE_IO_BASE_INDEX) ? 0x1000 : 0x100000;

            firstNode = TRUE;

            for (pNode2 = DLL_FIRST (&pPciDev->resource[i].listSubResSort); pNode2 != NULL;
                pNode2 = DLL_NEXT (pNode2))
                {
                pRestmp = (PCI_RES *)pNode2;

                /* get the align from the first node, the first node has the biggest align */

                if (firstNode == TRUE)
                    {
                    align = (pRestmp->align >align)?pRestmp->align:align;
                    firstNode = FALSE;
                    }

                /* adjust the size to ensure the node have enough space */

                pPciDev->resource[i].size = ROUND_UP(pPciDev->resource[i].size, pRestmp->align);
                pPciDev->resource[i].size += pRestmp->size;
                }

            pPciDev->resource[i].align = align;

            if (pPciDev->resource[i].size == 0)
                continue;
            else
                {
                align = (i == PCI_BRIDGE_IO_BASE_INDEX) ? 0x1000 : 0x100000;
                pPciDev->resource[i].size = ROUND_UP(pPciDev->resource[i].size, align);
                }

            PCI_LOG_MSG(1000, "Insert resource2 (%d:%d:%d) BAR (%d) to (%d:%d:%d) type (%d) size (0x%llx) align(0x%llx) preBar (0x%x)\n",
                pPciDev->bus, pPciDev->device, pPciDev->func, i,
                pPciDev->parent->bus, pPciDev->parent->device, pPciDev->parent->device,
                pPciDev->resource[i].type,
                pPciDev->resource[i].size,
                pPciDev->resource[i].align,
                listIndex);

            vxbPciResInsert(busCtrlID, (DL_LIST *)(
                 &pPciDev->parent->resource[i].listSubResSort),
                 (PCI_RES *)&pPciDev->resource[i]);
            }
        }
    }

/**********************************************************************
*
* vxbPciResourceAssign - allocate resource from parent resource
*
* This routine allocate resource from parent resource
*
* RETURNS: N/A
*
* ERRNO
*
*/

LOCAL void vxbPciResourceAssign
    (
    VXB_DEV_ID     busCtrlID,
    PCI_ROOT_RES * pciRootRes,
    PCI_DEVICE   * pPciDev
    )
    {
    DL_NODE * pNode  = NULL;
    DL_NODE * pNode2  = NULL;
    UINT32 i;
    PCI_RES * pRestmp;
    PCI_RES * pResPool;
    PCI_MMIO_DESC memDesc;

    for (i = PCI_BRIDGE_MEM_BASE_INDEX; i <= PCI_BRIDGE_PRE_MEM_INDEX; i ++)
        {
         if (!DLL_EMPTY(&pPciDev->resource[i].listSubResSort))
             {
             for (pNode2 = DLL_FIRST (&pPciDev->resource[i].listSubResSort); pNode2 != NULL;
                 pNode2 = DLL_NEXT (pNode2))
                {
                pRestmp = (PCI_RES *)pNode2;
                memDesc.align = pRestmp->align;
                memDesc.size = pRestmp->size;

                if (pRestmp->parentRes != NULL)
                    continue;

                /* get the resource pool */

                if (pPciDev->parent == NULL)
                    {
                    if ((i == PCI_BRIDGE_IO_BASE_INDEX)  ||
                        (i == PCI_BRIDGE_MEM_BASE_INDEX))
                        {
                        /* IO and memory resource */
                        pResPool = &pciRootRes->barRes[i - PCI_BRIDGE_MEM_BASE_INDEX];
                        }
                    else
                        {
                        /* prefetch memory */
                        pResPool = &pciRootRes->barRes[pPciDev->preIndex];
                        }
                    }
                else
                    {
                    pResPool = &pPciDev->resource[i];
                    }

                if (vxbPciBarAlloc(pResPool, &memDesc) != OK)
                    {
                    pRestmp->start = 0;
                    continue;
                    }

                PCI_LOG_MSG(500, "(%d:%d:%d) bar(%d) request (0x%llx:0x%llx) size(0x%llx) align(0x%x) addr(0x%llx)\n",pPciDev->bus,pPciDev->device,pPciDev->func,
                        i,
                        pPciDev->resource[i].pool.base,
                        pPciDev->resource[i].pool.size,
                        pRestmp->size,
                        pRestmp->align,
                        memDesc.addr);

                pRestmp->start = (ULONG)memDesc.addr;
                pRestmp->end = (ULONG)memDesc.addr + pRestmp->size - 1;
                pRestmp->parentRes = (void *)&pPciDev->resource[i];
                pRestmp->type &= ~PCI_RESOURCE_ERROR;

                if (!DLL_EMPTY(&pRestmp->listSubResSort))
                    {
                    pRestmp->pool.base =  pRestmp->start;
                    pRestmp->pool.size = pRestmp->size;
                    }
                }
             }
        }

    for (pNode = DLL_FIRST((DL_LIST *)&pPciDev->children); pNode != NULL;
        pNode = DLL_NEXT(pNode))
       {
        pPciDev = (PCI_DEVICE * )pNode;

        if (pPciDev->type == PCI_BRIDGE_TYPE)
            vxbPciResourceAssign (busCtrlID, pciRootRes, pPciDev);
       }
    }

/**********************************************************************
*
* vxbPciProgramBar - program bar into configuration space
*
* This routine programs bar into configuration space
*
* RETURNS: N/A
*
* ERRNO
*
*/

LOCAL void vxbPciProgramBar
    (
    VXB_DEV_ID     busCtrlID,
    PCI_DEVICE   * pPciDev
    )
    {
    DL_NODE * pNode  = NULL;
    UINT32 i = 0;
    UINT16 cmd = 0, value = 0;
    PCI_HARDWARE hardInfo;

    for (pNode = DLL_FIRST((DL_LIST *)&pPciDev->children); pNode != NULL;
        pNode = DLL_NEXT(pNode))
        {
        pPciDev = (PCI_DEVICE * )pNode;

        hardInfo.pciBus = pPciDev->bus;
        hardInfo.pciDev = pPciDev->device;
        hardInfo.pciFunc = pPciDev->func;

        /* program BAR into device */

        if (pPciDev->type == PCI_DEVICE_TYPE)
            {
            cmd = 0;
            for (i = 0; i < PCI_MAX_RESOURCE_NUMBER; i ++)
                {
                if ((i == PCI_BRIDGE_MEM_BASE_INDEX) ||
                   (i == PCI_BRIDGE_IO_BASE_INDEX) ||
                   (i == PCI_BRIDGE_PRE_MEM_INDEX)
                   )
                   continue;

                if (pPciDev->resource[i].size != 0)
                    {
                    PCI_LOG_MSG(600, "ProgramBar device(%d%d%d): BAR%d:[0x%llx-0x%llx] align 0x%llx size 0x%llx\n",
                        pPciDev->bus, pPciDev->device, pPciDev->func,i,
                        pPciDev->resource[i].start,
                        pPciDev->resource[i].end,
                        pPciDev->resource[i].align,
                        pPciDev->resource[i].size);

                    if (pPciDev->resource[i].type & PCI_BAR_SPACE_IO)
                        {
                        (void) VXB_PCI_CFG_WRITE(busCtrlID, &hardInfo,
                            PCI_CFG_BASE_ADDRESS_0 + i*4, 4,
                            (UINT32)pPciDev->resource[i].start);

                        cmd |= PCI_CMD_IO_ENABLE;
                        }
                    else
                        {
                        if (pPciDev->resource[i].type & PCI_BAR_MEM_ADDR64)
                            {
                            (void) VXB_PCI_CFG_WRITE(busCtrlID, &hardInfo,
                                PCI_CFG_BASE_ADDRESS_0 + i*4, 4,
                                (UINT32)pPciDev->resource[i].start);

#ifdef  _WRS_CONFIG_LP64
                            (void) VXB_PCI_CFG_WRITE(busCtrlID, &hardInfo,
                                PCI_CFG_BASE_ADDRESS_0 + (i+1)*4, 4,
                                (UINT32)(pPciDev->resource[i].start >> 32));
#endif /* _WRS_CONFIG_LP64  */

                            i ++ ;
                            }
                        else
                            {
                            (void) VXB_PCI_CFG_WRITE(busCtrlID, &hardInfo,
                                PCI_CFG_BASE_ADDRESS_0 + i*4, 4,
                                (UINT32)pPciDev->resource[i].start);
                            }

                        cmd |= PCI_CMD_MEM_ENABLE;
                        }
                    }
                }

             if (cmd)
                cmd |= PCI_CMD_MASTER_ENABLE;

             (void) VXB_PCI_CFG_WRITE(busCtrlID, &hardInfo, PCI_CFG_COMMAND, 2, cmd);
            }
        else /* bridge device */
            {
            cmd = 0;
            for (i = 0; i <= PCI_BRIDGE_PRE_MEM_INDEX; i ++)
                {
                if ((i >= PCI_DEV_BAR2_INDEX) &&
                   (i <= PCI_DEV_BAR5_INDEX)
                   )
                   continue;

                if (pPciDev->resource[i].size != 0)
                    {
                    PCI_LOG_MSG(600, "ProgramBar bridge(%d%d%d): BAR%d:[0x%llx-0x%llx] align 0x%llx size 0x%llx\n",
                        pPciDev->bus, pPciDev->device, pPciDev->func,i,
                        pPciDev->resource[i].start,
                        pPciDev->resource[i].end,
                        pPciDev->resource[i].align,
                        pPciDev->resource[i].size);
                    }

                if (i == PCI_BRIDGE_MEM_BASE_INDEX)
                    {
                    if (pPciDev->resource[i].size == 0)
                        {
                        (void) VXB_PCI_CFG_WRITE(busCtrlID, &hardInfo,
                            PCI_CFG_MEM_BASE, 4, 0x0000FFF0);
                        }
                    else
                        {
                        (void) VXB_PCI_CFG_WRITE(busCtrlID, &hardInfo,
                            PCI_CFG_MEM_BASE, 4,
                            (UINT32)((pPciDev->resource[i].end & 0xFFF00000) |
                            ((pPciDev->resource[i].start & 0xFFF00000) >> 16)));

                        cmd |= PCI_CMD_MEM_ENABLE;

                        }
                    continue;
                    }

                if (i == PCI_BRIDGE_IO_BASE_INDEX)
                    {
                    if (pPciDev->resource[i].size == 0)
                        {
                        (void) VXB_PCI_CFG_WRITE(busCtrlID, &hardInfo,
                            PCI_CFG_IO_BASE, 2, 0x00FF);
                        (void) VXB_PCI_CFG_WRITE(busCtrlID, &hardInfo,
                            PCI_CFG_IO_BASE_U, 4, 0x0000FFFFF);
                        }
                    else
                        {

                        (void) VXB_PCI_CFG_WRITE(busCtrlID, &hardInfo,
                            PCI_CFG_IO_BASE, 2,
                            (UINT16)((pPciDev->resource[i].end & 0xFC00)|
                            ((pPciDev->resource[i].start & 0xFC00) >> 8)));

                        (void) VXB_PCI_CFG_WRITE(busCtrlID, &hardInfo,
                            PCI_CFG_IO_BASE_U, 4,
                            (UINT32)((pPciDev->resource[i].end & 0xFFF00000)|
                            ((pPciDev->resource[i].start) >> 16)));

                        cmd |= PCI_CMD_IO_ENABLE;

                        }
                    continue;
                    }
                if (i == PCI_BRIDGE_PRE_MEM_INDEX)
                    {
                    if (pPciDev->resource[i].size == 0)
                        {
                        (void) VXB_PCI_CFG_WRITE(busCtrlID, &hardInfo,
                            PCI_CFG_PRE_MEM_BASE, 4,
                            0x0000FFFF);
                        (void) VXB_PCI_CFG_WRITE(busCtrlID, &hardInfo,
                            PCI_CFG_PRE_MEM_BASE_U, 4,
                            0);
                        (void) VXB_PCI_CFG_WRITE(busCtrlID, &hardInfo,
                            PCI_CFG_PRE_MEM_LIMIT_U, 4,
                            0);
                         }
                    else
                        {
                        (void) VXB_PCI_CFG_READ(busCtrlID, &hardInfo, PCI_CFG_PRE_MEM_BASE, 2, &value);
                        value &= 0xF;
                        value |= (UINT16)((pPciDev->resource[i].start & 0xFFF00000) >> 16);
                        (void) VXB_PCI_CFG_WRITE(busCtrlID, &hardInfo,
                            PCI_CFG_PRE_MEM_BASE, 2, value);

                        (void) VXB_PCI_CFG_READ(busCtrlID, &hardInfo, PCI_CFG_PRE_MEM_LIMIT, 2, &value);
                        value &= 0xF;
                        value |= (UINT16)((pPciDev->resource[i].end & 0xFFF00000)>>16);
                        (void) VXB_PCI_CFG_WRITE(busCtrlID, &hardInfo,
                            PCI_CFG_PRE_MEM_LIMIT, 2, value);

#ifdef _WRS_CONFIG_LP64
                        (void) VXB_PCI_CFG_WRITE(busCtrlID, &hardInfo,
                            PCI_CFG_PRE_MEM_BASE_U, 4,
                            (UINT32)(pPciDev->resource[i].start  >> 32));

                        (void) VXB_PCI_CFG_WRITE(busCtrlID, &hardInfo,
                            PCI_CFG_PRE_MEM_LIMIT_U, 4,
                            (UINT32)(pPciDev->resource[i].end  >> 32));
#else
                        (void) VXB_PCI_CFG_WRITE(busCtrlID, &hardInfo,
                            PCI_CFG_PRE_MEM_BASE_U, 4,
                            0);
                        (void) VXB_PCI_CFG_WRITE(busCtrlID, &hardInfo,
                            PCI_CFG_PRE_MEM_LIMIT_U, 4,
                            0);
#endif /* _WRS_CONFIG_LP64 */

                        cmd |= PCI_CMD_MEM_ENABLE;
                        }

                    continue;
                    }

                if (pPciDev->resource[i].size != 0)
                    {

                    if (pPciDev->resource[i].type & PCI_BAR_SPACE_IO)
                        {
                        (void) VXB_PCI_CFG_WRITE(busCtrlID, &hardInfo,
                            PCI_CFG_BASE_ADDRESS_0 + i*4, 4,
                            (UINT32)(pPciDev->resource[i].start));

                        cmd |= PCI_CMD_IO_ENABLE;
                        }
                    else
                        {
                        if (pPciDev->resource[i].type & PCI_BAR_MEM_ADDR64)
                            {
                            (void) VXB_PCI_CFG_WRITE(busCtrlID, &hardInfo,
                                PCI_CFG_BASE_ADDRESS_0 + i*4, 4,
                                (UINT32)pPciDev->resource[i].start);
#ifdef  _WRS_CONFIG_LP64
                            (void) VXB_PCI_CFG_WRITE(busCtrlID, &hardInfo,
                                PCI_CFG_BASE_ADDRESS_0 + (i+1)*4, 4,
                                (UINT32)(pPciDev->resource[i].start >> 32));
#else
                            (void) VXB_PCI_CFG_WRITE(busCtrlID, &hardInfo,
                                PCI_CFG_BASE_ADDRESS_0 + (i+1)*4, 4,
                                0);
#endif /* _WRS_CONFIG_LP64 32 bit system only use 4G address */
                            i ++;
                            }
                        else
                            {
                            (void) VXB_PCI_CFG_WRITE(busCtrlID, &hardInfo,
                                PCI_CFG_BASE_ADDRESS_0 + i*4, 4,
                                (UINT32)pPciDev->resource[i].start);
                            }
                         cmd |= PCI_CMD_MEM_ENABLE;
                        }

                    }
                }

            if (cmd)
                cmd |= PCI_CMD_MASTER_ENABLE;

            (void) VXB_PCI_CFG_WRITE(busCtrlID, &hardInfo, PCI_CFG_COMMAND, 2, cmd);
            }

        if (pPciDev->type == PCI_BRIDGE_TYPE)
            vxbPciProgramBar (busCtrlID, pPciDev);

        }

    return;
    }

/**********************************************************************
*
* vxbPciDeviceScan - scan all of PCI device under bus controller
*
* This routine scans all of PCI device under bus controller
*
* RETURNS: second bus number
*
* ERRNO
*
*/

LOCAL int vxbPciDeviceScan
    (
    VXB_DEV_ID     busCtrlID,
    PCI_ROOT_RES * pciRootRes,
    UINT8          bus,
    PCI_DEVICE   * pParentDev
    )
    {
    UINT8 device, function, secbus;
    UINT8 btemp, headerType;
    UINT8 newPribus, newSecbus, newSubbus;
    UINT32 vendId;
    PCI_DEVICE * pPciDev;
    PCI_HARDWARE hardInfo;
    secbus = bus;

    for (device = 0; device < PCI_MAX_DEV; device ++)
        {
        for (function = 0; function < PCI_MAX_FUNC; function ++)
            {
            hardInfo.pciBus = bus;
            hardInfo.pciDev = device;
            hardInfo.pciFunc = function;

            (void) VXB_PCI_CFG_READ(busCtrlID, &hardInfo, PCI_CFG_VENDOR_ID, 4, &vendId);

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

            pPciDev = (PCI_DEVICE *)vxbMemAlloc (sizeof(PCI_DEVICE));

            if (pPciDev == NULL)
                {
                PCI_LOG_MSG(0, "! vxbMemAlloc ERROR\n");
                return ERROR;
                }

            pPciDev->bus    = bus;
            pPciDev->device = device;
            pPciDev->func   = function;

            DLL_ADD((DL_LIST *)&pParentDev->children, &pPciDev->node);

            pPciDev->parent = pParentDev;

            (void) VXB_PCI_CFG_WRITE (busCtrlID, &hardInfo, PCI_CFG_COMMAND, 2, 0);

            (void) VXB_PCI_CFG_WRITE (busCtrlID, &hardInfo, PCI_CFG_CACHE_LINE_SIZE, 1,  _CACHE_ALIGN_SIZE / 4);

            (void) VXB_PCI_CFG_WRITE (busCtrlID, &hardInfo, PCI_CFG_LATENCY_TIMER, 1,
                               PCI_DEF_LTIMER);

            /* Check to see if this function belongs to a PCI-PCI bridge */
            (void) VXB_PCI_CFG_READ (busCtrlID, &hardInfo, PCI_CFG_HEADER_TYPE, 1, &headerType);

            headerType &= PCI_HEADER_TYPE_MASK;

            if ((headerType == PCI_HEADER_TYPE_BRIDGE) ||
                (headerType == PCI_HEADER_PCI_CARDBUS))
                {
                /* PCI-to-PCI Bridge PCI-to-CardBus bridge */

                pPciDev->type = PCI_BRIDGE_TYPE;

                PCI_LOG_MSG(100, "Find Bridge(%d:%d:%d)"
                                 " headerType 0x%x PID/VID 0x%08x\n",
                    bus,device,function, headerType,vendId);

                (void) VXB_PCI_CFG_WRITE (busCtrlID, &hardInfo, PCI_CFG_SEC_LATENCY, 1, PCI_DEF_LTIMER);

                vxbPciResourceGet (busCtrlID, pPciDev);

                secbus ++;

                PCI_LOG_MSG(1000, "Next Bus %d --------\n", secbus);

                DLL_INIT((DL_LIST *)&pPciDev->children);

                newPribus = bus;
                newSecbus = secbus;
                newSubbus = secbus;

                (void) VXB_PCI_CFG_WRITE (busCtrlID, &hardInfo, PCI_CFG_PRIMARY_BUS, 1, newPribus);
                (void) VXB_PCI_CFG_WRITE (busCtrlID, &hardInfo, PCI_CFG_SECONDARY_BUS, 1, newSecbus);
                (void) VXB_PCI_CFG_WRITE (busCtrlID, &hardInfo, PCI_CFG_SUBORDINATE_BUS, 1, 0xff);

				/**/
				/* read remote endp status*/
				/**/
                if (pciRootRes->endBusNumber)
                {
                    secbus = (UINT8)vxbPciDeviceScan (busCtrlID, pciRootRes, secbus, pPciDev);
                }
                else  /* root*/
                {
                    secbus = 0;
                    (void) VXB_PCI_CFG_WRITE (busCtrlID, &hardInfo, PCI_CFG_SECONDARY_BUS, 1, secbus);
                }

                newSubbus = secbus;
                (void) VXB_PCI_CFG_WRITE (busCtrlID, &hardInfo, PCI_CFG_SUBORDINATE_BUS, 1, newSubbus);
				
            }
            else
                {

                pPciDev->type = PCI_DEVICE_TYPE;

                PCI_LOG_MSG(50, "Find Device(%d:%d:%d) headerType %d PID/VID 0x%08x\n",
                    bus,device,function, headerType,vendId);

                vxbPciResourceGet (busCtrlID, pPciDev);
                }

            /* Proceed to next device if this is a single function device */

            if (function == 0)
                {
                (void) VXB_PCI_CFG_READ (busCtrlID, &hardInfo, PCI_CFG_HEADER_TYPE, 1, &btemp);

                if ((btemp & PCI_HEADER_MULTI_FUNC) == 0)
                    break; /* No more functions - proceed to next PCI device */
                }

            }
        }

    return secbus;
    }

/******************************************************************************
*
* vxbPciAutoConfigFunc - automatically configure and announce all PCI
*
* This routine configure all PCI headers and announce PCI device to Vxbus
* system.
*
* RETURNS: OK or ERROR when allocate VXB_PCI_CTRL failed
*
* ERRNO
*
*/

LOCAL STATUS vxbPciAutoConfigFunc
    (
    VXB_DEV_ID  busCtrlID
    )
{
    /*PCI_ROOT_CHAIN *pciRootChain = vxbDevSoftcGet(busCtrlID);*/
    PCI_ROOT_CHAIN * pciRootChain = vxbDevSoftcGet(busCtrlID);
	
    PCI_ROOT_RES   * pciRootRes;
    UINT8 i;
    PCI_DEVICE * pPciDev;
    
    for (i = 0; i < pciRootChain->segCount; i++)
        {
        pciRootRes = (PCI_ROOT_RES *)&pciRootChain->pRootRes[i];

        PCI_LOG_MSG(10, "vxbPciAutoConfig(): bus number[%d - %d] pciRootRes %p\n",
            pciRootRes->baseBusNumber, pciRootRes->endBusNumber, pciRootRes);

        /* Root dummy pci node */
        pPciDev = (PCI_DEVICE *)vxbMemAlloc (sizeof(PCI_DEVICE));

        if (pPciDev == NULL)
            {
            PCI_LOG_MSG(0, "vxbMemAlloc PCI_DEVICE ERROR!\n");
            return ERROR;
            }

        pPciDev->bus    = ~0;
        pPciDev->device = ~0;
        pPciDev->func   = ~0;
        pPciDev->parent = NULL;
        pPciDev->type = PCI_BRIDGE_TYPE;

        pciRootRes->pPciDev = pPciDev;

        (void) vxbPciDeviceScan (busCtrlID, pciRootRes, pciRootRes->baseBusNumber, pPciDev);

        if (!DLL_EMPTY(&pPciDev->children))
            {
            vxbPciResAdjust (busCtrlID, pciRootRes, pPciDev);

            vxbPciResourceSort (busCtrlID, pPciDev, 0);

            vxbPciResourceAssign (busCtrlID, pciRootRes, pPciDev);

            vxbPciProgramBar(busCtrlID, pPciDev);
            }
        }

    return(OK);
    }
