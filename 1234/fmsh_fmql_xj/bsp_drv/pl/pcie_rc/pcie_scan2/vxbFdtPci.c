/* vxbFdtPci.c - vxBus pci fdt */

/*
 * Copyright (c) 2013-2016 Wind River Systems, Inc.
 *
 * The right to copy, distribute, modify or otherwise make use
 * of this software may be licensed only pursuant to the terms
 * of an applicable Wind River license agreement.
 */

/*
modification history
--------------------
28jun16,wap  Correct more compiler warnings
29jun15,xms  fixed compiler warning. (F2518)
03jun14,l_z  fix static analysis issue. (V7PRO-951)
31mar14,l_z  rename vxbFdtPciGetRouteInfo() to vxbFdtPciIntrGet(). (US37630)
15jan14,y_y  code clean
25jul13,j_z  written
*/

#include <vxWorks.h>
#include <stdlib.h>
#include <vxFdtLib.h>
#include <hwif/vxBus.h>
#include <hwif/buslib/vxbFdtLib.h>
#include <hwif/buslib/vxbPciLib.h>
#include <hwif/buslib/pciDefines.h>
#include <hwif/vxbus/vxbBusType.h>
#include <subsys/int/vxbIntLib.h>
#include <errno.h>
#include <hwif/buslib/vxbFdtPci.h>

#define FDT_RANGES_CELLS    ((3 + 3 + 2) * 2)

/******************************************************************************
*
* vxbFdtAddrSizeCells - PCI address and size cells retrieve routine
*
* This routine retrieves the PCI address and size cells info.
*
* RETURNS: OK, or ERROR
*
* ERRNO
*/

LOCAL STATUS vxbFdtAddrSizeCells
    (
    VXB_FDT_DEV * node,
    int *addrCells,
    int *sizeCells
    )
    {
    const void * pCell;
    int cellSize;

    /* Retrieve (address,size) cells. */

    cellSize = sizeof(UINT32);

    pCell = vxFdtPropGet(node->offset, "#address-cells", &cellSize);

    if (pCell == NULL)
        return ERROR;

    *addrCells = vxFdt32ToCpu(*(UINT32 *)pCell);

    pCell = vxFdtPropGet(node->offset, "#size-cells", &cellSize);

    if (pCell == NULL)
        return ERROR;

    *sizeCells = vxFdt32ToCpu(*(UINT32 *)pCell);

    if (*addrCells > 3 || *sizeCells > 2)
        return (ERROR);

    return OK;
    }

/******************************************************************************
*
* vxbFdtAddrCells - PCI address cells retrieve routine
*
* This routine retrieves the PCI address cells info.
*
* RETURNS: OK, or ERROR
*
* ERRNO
*/

LOCAL STATUS vxbFdtAddrCells
    (
    VXB_FDT_DEV * node,
    int *addrCells
    )
    {
    const void * pCell;
    int cellSize;

    pCell = vxFdtPropGet(node->offset, "#address-cells", &cellSize);

    if (pCell == NULL)
        return (ERROR);

    *addrCells = vxFdt32ToCpu(*(UINT32 *)pCell);

    if (*addrCells > 3)
        return (ERROR);

    return (OK);
    }

/******************************************************************************
*
* vxbFdtIntrCells - PCI interrupt cells retrieve routine
*
* This routine retrieves the PCI interrupt cells info.
*
* RETURNS: value
*
* ERRNO
*/

LOCAL int vxbFdtIntrCells
    (
    VXB_FDT_DEV *  node
    )
    {
    UINT32 intrCells;
    int cellSize;
    const void * temp = NULL;

    temp = vxFdtPropGet(node->offset, "#interrupt-cells", &cellSize);

    if (temp == NULL)
         return ERROR;

    intrCells = *(UINT32 *)temp;

    intrCells = vxFdt32ToCpu(intrCells);

    return ((int)intrCells);
    }

/******************************************************************************
*
* vxbFdtPciGetBusRange - PCI bus-range cells retrieve routine
*
* This routine retrieves the PCI bus-range cells info.
*
* RETURNS: value
*
* ERRNO
*/

STATUS vxbFdtPciGetBusRange
    (
    VXB_FDT_DEV * node,
    UINT32 * start,
    UINT32 * end
    )
    {
    UINT32 * busRange;
    int  len;

    busRange = (UINT32 *)vxFdtPropGet(node->offset, "bus-range", &len);

    if (busRange == NULL)
        return ERROR;

    *start = vxFdt32ToCpu(busRange[0]);
    *end = vxFdt32ToCpu(busRange[1]);

    return OK;
    }

/******************************************************************************
*
* vxbFdtDataGet - convert the FDT data to CPU
*
* This routine converts the FDT data order to CPU.
*
* RETURNS: value
*
* ERRNO
*/

LOCAL PHYS_ADDR vxbFdtDataGet
    (
    void *data,
    int cells
    )
    {
    if (cells == 1)
        return (PHYS_ADDR)(vxFdt32ToCpu(*((uint32_t *)data)));

    return (PHYS_ADDR)(vxFdt64ToCpu(*((uint64_t *)data)));
    }

/******************************************************************************
*
* vxbFdtPciRanges - get FDT pci range
*
* This routine gets the FDT pci mempry and IO range.
*
* RETURNS: OK, or ERROR
*
* ERRNO
*/

STATUS vxbFdtPciRanges
    (
    VXB_FDT_DEV * node,
    UINT32        type,
    struct fdtPciRange *space
    )
    {
    struct fdtPciRange * pciSpace = NULL;
    int addrCells, sizeCells, parAddrCells;
    UINT32 *rangesptr;
    UINT32 cell0;
    int tupleSize, tuples, i, offsetCells, len;
    int pOffset;
    const void * pCell;
    int cellSize, parAddrCellSize;

    /* Retrieve 'ranges' property. */

    if ((vxbFdtAddrSizeCells(node, &addrCells, &sizeCells)) != 0)
        return (ERROR);

   if (addrCells != 3 || sizeCells != 2)
        return (ERROR);

    pOffset = vxFdtParentOffset(node->offset);

    pCell = vxFdtPropGet(pOffset, "#address-cells", &cellSize);

    if (pCell == NULL)
        return ERROR;

    parAddrCells = vxFdt32ToCpu(*(UINT32 *)pCell);

    if ((rangesptr = (UINT32 *)vxFdtPropGet(node->offset, "ranges", &len)) == NULL)
        return (ERROR);

    tupleSize = (UINT32)(sizeof(UINT32) * (addrCells + parAddrCells + sizeCells));
    tuples = len / tupleSize;

    offsetCells = 0;

    for (i = 0; i < tuples; i++)
        {
        cell0 = (UINT32)vxbFdtDataGet((void *)rangesptr, 1);
        rangesptr++;

        if (cell0 == type)
            {
            pciSpace = space;
            }
        else
            {
            rangesptr += (tupleSize>>2) -1;
            continue;
            }

        pciSpace->basePci = (ULONG)vxbFdtDataGet((void *)rangesptr, 2);
        rangesptr += 2;

        if (parAddrCells == 3)
            {
            /*
             * This is a PCI subnode 'ranges'. Skip cell0 and
             * cell1 of this entry and only use cell2.
             */
            offsetCells = 2;
            rangesptr += offsetCells;
            }

        parAddrCellSize = parAddrCells - offsetCells;

        pciSpace->baseParent = vxbFdtDataGet((void *)rangesptr, parAddrCellSize);
        rangesptr += parAddrCellSize;

        pciSpace->len = (size_t)vxbFdtDataGet((void *)rangesptr, sizeCells);
        rangesptr += sizeCells;
        }

   if (pciSpace == NULL)
        return ERROR;

    return (OK);
    }

/******************************************************************************
*
* vxbFdtPciIntrInfo - get pci interrupt info
*
* This routine gets the pci interrupt info.
*
* RETURNS: OK, or ERROR
*
* ERRNO
*/

STATUS vxbFdtPciIntrInfo
    (
    VXB_FDT_DEV *       node,
    struct fdtPciIntr * intrInfo
    )
    {
    int addrCells, icells;
    int len, mlen;

    if ((vxbFdtAddrCells(node, &addrCells)) != OK)
        return (ERROR);

    icells = vxbFdtIntrCells(node);

    /* Retrieve the interrupt map and mask properties. */

    intrInfo->mask = (UINT32*)vxFdtPropGet(node->offset, "interrupt-map-mask", &mlen);

    intrInfo->map = (UINT32*)vxFdtPropGet(node->offset, "interrupt-map", &len);

    intrInfo->mapLen = len;
    intrInfo->addrCells = addrCells;
    intrInfo->intrCells = icells;

    return (OK);
    }

/******************************************************************************
*
* vxbFdtPciIntrGet - get pci interrupt routine info
*
* This routine gets the pci interrupt routine info.
*
* RETURNS: OK, or ERROR
*
* ERRNO
*/

STATUS vxbFdtPciIntrGet
    (
    struct fdtPciIntr *  intrInfo,
    int                  bus,
    int                  slot,
    int                  func,
    int                  pin,
    UINT8 *              interrupt,
    VXB_INTR_ENTRY   *   pIntrEntry
    )
    {
    UINT32 childSpec[4], masked[4];
    UINT32 *mapPtr;
    uint32_t addr;
    UINT32 i, j, mapLen;
    int parIntrcells, childSpecCells, rowCells;
    int specIdx;
    int pOffset;
    const void * pCell;
    int cellSize;

    childSpecCells = intrInfo->addrCells + intrInfo->intrCells;
    if (childSpecCells > sizeof(childSpec) / sizeof(UINT32))
        return (ERROR);

    addr = (bus << 16) | (slot << 11) | (func << 8);
    childSpec[0] = addr;
    childSpec[1] = 0;
    childSpec[2] = 0;
    childSpec[3] = pin;

    mapLen = intrInfo->mapLen;
    mapPtr = intrInfo->map;

    i = 0;

    while (i < mapLen)
        {
        pOffset = vxFdtNodeOffsetByPhandle(vxFdt32ToCpu(mapPtr[childSpecCells]));

        pCell = vxFdtPropGet(pOffset, "#interrupt-cells", &cellSize);

        if (pCell == NULL)
            return ERROR;

        parIntrcells = vxFdt32ToCpu(*(UINT32 *)pCell);

        rowCells = childSpecCells + 1 +  parIntrcells;

        /* Apply mask and look up the entry in interrupt map. */

        for (j = 0; j < childSpecCells; j++)
            {
            masked[j] = childSpec[j] &
                vxFdt32ToCpu(intrInfo->mask[j]);

            if (masked[j] != vxFdt32ToCpu(mapPtr[j]))
                goto next;
            }

        /* Decode interrupt of the parent intr controller. */

        specIdx = childSpecCells + 1;

        *interrupt = (UINT8)vxFdt32ToCpu(mapPtr[specIdx]);

        pIntrEntry->node = pOffset;
        pIntrEntry->pProp = (UINT32 *)&mapPtr[specIdx + 1];
        pIntrEntry->numProp = (parIntrcells - 1);

        return OK ;

next:
        mapPtr += rowCells;
        i += (UINT32)(rowCells * sizeof(UINT32));
        }

    return ERROR;
    }
