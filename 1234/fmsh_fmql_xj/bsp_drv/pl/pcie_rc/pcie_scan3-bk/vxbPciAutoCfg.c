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
21apr17,syt  updated vxbPciResourceSort3 to slove memory spcae allocation
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

#include "vxbPciMethod3.h"
#include "pciConfigLib3.h"
#include "pciAutoConfigLib3.h"
#include "vxbPciLib3.h"
#include "dllLib3.h"


/* Debug macro */


/*#ifdef  PCI_DEBUG*/
/*#undef  PCI_DEBUG*/
#define  PCI_DEBUG



/*  int debugLevel = 0;*/

/*#ifdef   */
/*#undef   */
/*#define*/

#if 0
#define PCI_LOG_MSG(lvl, fmt, args...)        \
    do {                                      \
        if (debugLevel >= lvl)                \
            {                                 \
            if (_func_kprintf != NULL)        \
                _func_kprintf (fmt, ##args);  \
            }                                 \
       } while (FALSE)

#define PCI_LOG_MSG(lvl, fmt, args...)

#endif



/* defines */

#define PCI_DEF_LTIMER     0x40

/*#ifdef ROUND_UP*/
/*#undef ROUND_UP*/
#define ROUND_UP(n, align)  ((n + align - 1) & (~(align - 1)))
/*#endif*/


STATUS vxbPciAutoConfigFunc3 (void);
UINT32 memAlloc_Usr(int);

extern void vxbPciResourceGet3(VXB_DEV_ID3 pDev, PCI_DEVICE3 * pPciDev);



/*******************************************************************************
*
* vxbPciBarAlloc3 - alloc PCI/PCIe device's memory or IO space.
*
* This routine allocates PCI/PCIe device's memory or IO space.
*
* RETURNS: OK or ERROR if allocation failed.
*
* ERRNO
*/

STATUS vxbPciBarAlloc3
    (
    PCI_RES3 *          pRes, /* parent resource*/
    PCI_MMIO_DESC3    * pDesc
    )
{
    PCI_ADDR_RESOURCE3 * pCur;
    PCI_ADDR_RESOURCE3 * pNext;
    PCI_ADDR_RESOURCE3 * pPrev;
    PCI_ADDR_RESOURCE3 * pNew;
    UINT64              alignStart = 0;
    UINT32              nodeIndex = 0;
    PCI_BAR_ADR_POOL3 * pPool = &pRes->pool;

    if (pDesc->align == 0)
        pDesc->align = sizeof(UINT32);

    if (pDesc->size== 0)
    {
        return ERROR;
    }

    /* Take control of bar resource */
#if 0
    if (semTake (pciResSemId, WAIT_FOREVER) != OK)
        return ERROR;
#endif

    for (pCur  = (PCI_ADDR_RESOURCE3 *) DLL_FIRST3 (&pPool->list);
         pCur != NULL;
         pCur  = pNext)
        {
        nodeIndex++;

        alignStart =
            ROUND_UP (pCur->start + pCur->size, pDesc->align);

        pNext = (PCI_ADDR_RESOURCE3 *) DLL_NEXT3 (&pCur->node);

        if ((pNext != NULL) &&
            (pNext->start > alignStart) &&
            ((pNext->start - alignStart) >= pDesc->size))
            break;
        }

    if (nodeIndex == 0)
        alignStart = pPool->base;

    pNew = (PCI_ADDR_RESOURCE3 *)memAlloc_Usr(sizeof(PCI_ADDR_RESOURCE3));/*vxbMemAlloc (sizeof (PCI_ADDR_RESOURCE3));*/
    if (pNew == NULL)
    {
        printf (0, "PCI BAR pool node allocation failed\n");

#if 0
        (void) semGive (pciResSemId);
#endif

        return (ERROR);
    }

    if ((nodeIndex == 0) || (nodeIndex == pPool->nodeCnt))
    {
        pPrev = (PCI_ADDR_RESOURCE3 *) DLL_LAST3 (&pPool->list);
    }
    else
    {
        pPrev = pCur;
    }

    DLL_INSERT3(&pPool->list, &pPrev->node, &pNew->node);

    pPool->nodeCnt ++;

    pNew->start       = alignStart;
    pNew->size        = pDesc->size;
    pNew->align       = pDesc->align;
    pDesc->addr      = alignStart;

    /* Release semaphore */
#if 0
    (void) semGive (pciResSemId);
#endif

    return (OK);
}

/*******************************************************************************
*
* vxbPciResInsert3 - insert childern's resource to parent resource list.
*
* This routine inserts childern's resource to parent resource list.
*
* RETURNS: N/A
*
* ERRNO
*/
void vxbPciResInsert3
    (
    VXB_DEV_ID3  busCtrlID,
    DL_LIST3   * pParentList, /* parent resource pool */
    PCI_RES3   * pResInsert   /* resource need to insert */
    )
{
    DL_NODE3 * pNode;
    PCI_RES3 * pRestmp;
    PCI_RES3 * pResNext;

    /* Take control of bar resource */
#if 0
    if (semTake (pciResSemId, WAIT_FOREVER) != OK)
        return;
#endif

    if (DLL_EMPTY3 (pParentList))
    {
        DLL_ADD3((DL_LIST3 *)pParentList, &pResInsert->node);
#if 0
        (void) semGive (pciResSemId);
#endif
        return;
    }
    else
    {
        for (pNode = DLL_FIRST3 (pParentList); pNode != NULL;
            pNode = DLL_NEXT3 (pNode))
        {
            pRestmp = (PCI_RES3 *)pNode;

            if (pRestmp->align >= pResInsert->align)
            {
                pResNext = (PCI_RES3 *)DLL_NEXT3(pNode);

                if (pResNext == NULL)
                {

                    DLL_INSERT3(pParentList, &pRestmp->node, &pResInsert->node);
				#if 0
                    (void) semGive (pciResSemId);
				#endif
                    return;
                }
                else
                {
                    if (pResNext->align < pResInsert->align)
                    {
                        DLL_INSERT3(pParentList, &pRestmp->node, &pResInsert->node);
					#if 0
                        (void) semGive (pciResSemId);
					#endif
                        return;
                    }
                }
            }
            else /* save the resource to head */
            {
                DLL_INSERT3(pParentList, NULL, &pResInsert->node);
			#if 0
                (void) semGive (pciResSemId);
			#endif

                return;
            }
        }
	}
    
    printf( "vxbPciResInsert3(): Miss Size 0x%x  Align 0x%x type 0x%x \n",
        pResInsert->size, pResInsert->align, pResInsert->type);
#if 0
    (void) semGive (pciResSemId);
#endif

    return ;
}
	
/**********************************************************************
*
* vxbPciResourceSort3 - sort the bar size from large to small
*
* This routine sort the bar size from large to small
*
* RETURNS: N/A
*
* ERRNO
*
*/
void vxbPciResourceSort3
    (
    VXB_DEV_ID3     busCtrlID,
    PCI_DEVICE3   * pParent,
    UINT8          preIndex
    )
{
    DL_NODE3 * pNode  = NULL;
    DL_NODE3 * pNode2  = NULL;
    PCI_RES3 * pRestmp;
    UINT32      i;
    UINT32      listIndex;
    PCI_DEVICE3* pPciDev;
    UINT64      align;
    BOOL        firstNode;

	/*
		轮询设备树
		设备树中同一总线下的设备，连接在上一级总线桥的子设备链表中，通过双向链表，按照device号
		从小到大
		相邻总线的设备之间，通过父子指针进行连接
		轮询时，从设备树的最顶层设备开始，通过父子指针，进行总线号递增，通过子设备链表，进行设备号递增
		总线号递增通过递归调用实现，设备号递增通过轮询双向链表的下一节点实现
		先定位到设备树中的总线号最小的节点设备，将节点设备中的每一个bar空间挂载到上一级桥设备的mem空间链表中
		mem空间链表顺序按照bar空间最大的在最前面一个节点的方式，mem空间的大小等于所有bar空间大小之和，对齐值取最大的对齐值
		上一级桥设备的mem空间和自身的bar空间挂载到上上一级桥设备的mem空间链表中，顺序，大小，对齐的规则与上面的一致。
		资源链表就这样建立起来
	*/
    for (pNode = DLL_FIRST3((DL_LIST3 *)&pParent->children); pNode != NULL;
        pNode = DLL_NEXT3(pNode))
    {
        pPciDev = (PCI_DEVICE3 * )pNode;

        /* always use the preIndex in root node */

        if (pParent->parent == NULL)
            preIndex = pPciDev->preIndex;
		
		/*
			如果是桥，就递归调用
		*/
        if (pPciDev->type == PCI_BRIDGE_TYPE)
        {
            vxbPciResourceSort3 (busCtrlID, pPciDev, preIndex);
        }

        /* bar resource */
		/*
		节点设备空间加到上一级桥的链表中
		识别BAR空间的类型
		把设备的bar资源 放到桥里的mem空间链表
		把最大的bar空间放到最前面
		*/

        for (i = 0; i < PCI_MAX_RESOURCE_NUMBER; i ++)
        {
            if ((UINT32)pPciDev->resource[i].size != 0)
            {
                if (pPciDev->resource[i].type == PCI_BAR_SPACE_IO)
                {
                    listIndex = PCI_BRIDGE_IO_BASE_INDEX;
                }
                else if (pPciDev->resource[i].type == PCI_BAR_MEM_PREFETCH)
                {
                    if (preIndex == PCI_ROOT_RES_MEM32_IDX)
                        listIndex = PCI_BRIDGE_MEM_BASE_INDEX;
                    else
                        listIndex = PCI_BRIDGE_PRE_MEM_INDEX;
                }
                else if (pPciDev->resource[i].type == PCI_BAR_MEM_NON_PREF)
                {
                    listIndex = PCI_BRIDGE_MEM_BASE_INDEX;
                }
                else
                {
                	continue;
                }
                
				printf( "Insert resource1(%d:%d:%d) BAR(%d) to (%d:%d:%d); type(%d) size(0x%08X) align(0x%08X) preBar(0x%X) \n\n",
	                    pPciDev->bus, pPciDev->device, pPciDev->func, i,
	                    pPciDev->parent->bus, pPciDev->parent->device, pPciDev->parent->func,
	                    pPciDev->resource[i].type,
	                    (UINT32 )pPciDev->resource[i].size,
	                    (UINT32 )pPciDev->resource[i].align,
	                    listIndex);

                 vxbPciResInsert3(busCtrlID, (DL_LIST3 *)(
                     &pPciDev->parent->resource[listIndex].listSubResSort),
                     (PCI_RES3 *)&pPciDev->resource[i]);
            }
        }

        /* bridge resource */
		/*
		桥设备空间加到上一级桥空间链表中
		先统计本级桥的所有子资源链表中的大小累加
		注意按照子资源表中最大的对齐来对齐
		然后在加到上一级桥空间链表中
		这样就把本级桥设备以下的空间累加到了上一级桥空间链表中
		*/

        for (i = PCI_BRIDGE_MEM_BASE_INDEX; i <= PCI_BRIDGE_PRE_MEM_INDEX; i++)
        {
            align = (i == PCI_BRIDGE_IO_BASE_INDEX) ? 0x1000 : 0x100000;

            firstNode = TRUE;
			
			/*
			把桥设备下的资源的size先按照最大的对齐，然后都加到桥的size中
			*/
            for (pNode2 = DLL_FIRST3 (&pPciDev->resource[i].listSubResSort); pNode2 != NULL;
                pNode2 = DLL_NEXT3 (pNode2))
            {
                pRestmp = (PCI_RES3 *)pNode2;

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

			/*
			桥资源的大小最小按1M对齐
			*/
            if ((UINT32)pPciDev->resource[i].size == 0)
            {   
            	continue;
            }
            else
            {
                align = (i == PCI_BRIDGE_IO_BASE_INDEX) ? 0x1000 : 0x100000;
                pPciDev->resource[i].size = ROUND_UP(pPciDev->resource[i].size, align);
            }
            
			printf("Insert resource2 (%d:%d:%d) BAR (%d) to (%d:%d:%d) type (%d) size (0x%08x) align(0x%08x) preBar (0x%x)\n",
	                pPciDev->bus, pPciDev->device, pPciDev->func, i,
	                pPciDev->parent->bus, pPciDev->parent->device, pPciDev->parent->func,
	                pPciDev->resource[i].type,
	                (UINT32 )pPciDev->resource[i].size,
	                (UINT32 )pPciDev->resource[i].align,
	                i);
			
            vxbPciResInsert3(busCtrlID, (DL_LIST3 *)(
                 &pPciDev->parent->resource[i].listSubResSort),
                 (PCI_RES3 *)&pPciDev->resource[i]);
        }
    }
}


/**********************************************************************
*
* vxbPciResourceAssign3 - allocate resource from parent resource
*
* This routine allocate resource from parent resource
* 根据之前形成的资源链表，分配各个设备的起始地址，大小，结束地址，对齐值
*
* RETURNS: N/A
*
* ERRNO
*
*/
void vxbPciResourceAssign3
    (
    VXB_DEV_ID3     busCtrlID,/*no used*/
    PCI_DEVICE3   * pPciDev/*顶层设备*/
    )
{
    DL_NODE3 * pNode  = NULL;
    DL_NODE3 * pNode2  = NULL;
    UINT32 i;
    PCI_RES3 * pRestmp;
    PCI_RES3 * pResPool;
    PCI_MMIO_DESC3 memDesc;

	/*
	bridge resoure assign
	*/
    for (i = PCI_BRIDGE_MEM_BASE_INDEX; i <= PCI_BRIDGE_PRE_MEM_INDEX; i ++)
    {
         if (!DLL_EMPTY3(&pPciDev->resource[i].listSubResSort))  /*ff_ff_ff的子资源列表中只有1_0_0资源*/
         {
             for (pNode2 = DLL_FIRST3 (&pPciDev->resource[i].listSubResSort); pNode2 != NULL;
                 pNode2 = DLL_NEXT3 (pNode2))
             {
            	/*
				获取子资源的大小和对齐值
            	*/
                pRestmp = (PCI_RES3 *)pNode2;
                memDesc.align = pRestmp->align;
                memDesc.size = pRestmp->size;

                /* 
            		get the resource pool 
            		获取本级桥的资源
            		base&size
            	*/
				pResPool = &pPciDev->resource[i];

				/* 
            		本级桥按照子资源在链表中的先后顺序，起始地址进行累加，下一个子资源的起始地址是上一个子资源的结束地址
            	*/
                if (vxbPciBarAlloc3(pResPool, &memDesc) != OK)
                {
                    pRestmp->start = 0;
                    continue;
                }

                printf("BRIDGE ResourceAssign(%d:%d:%d) bar(%d) request(0x%08X:0x%08X) size(0x%08X) align(0x%08X) addr(0x%08X) \n\n",\
                        pPciDev->bus,pPciDev->device,pPciDev->func,
                        i,
                        (UINT32 )pPciDev->resource[i].pool.base,
                        (UINT32 )pPciDev->resource[i].pool.size,
                        (UINT32 )pRestmp->size,
                        (UINT32 )pRestmp->align,
                        (UINT32 )memDesc.addr);

				/*
					更新当前子资源的起始地址，结束地址，子资源的父指针指向本级桥
				*/
                pRestmp->start = (ULONG)memDesc.addr;
                pRestmp->end = (ULONG)memDesc.addr + pRestmp->size - 1;
                pRestmp->parentRes = (void *)&pPciDev->resource[i];
                pRestmp->type &= ~PCI_RESOURCE_ERROR;
				
				/*
					更新子资源的地址池，起始地址就是子资源的起始地址
				*/
                if (!DLL_EMPTY3(&pRestmp->listSubResSort))
                {
                    pRestmp->pool.base =  pRestmp->start;
                    pRestmp->pool.size = pRestmp->size;
                }
             }
         }
    }

	/*
		根据设备树链表，进行递归调用，每个桥都遍历到
	*/
    for (pNode = DLL_FIRST3((DL_LIST3 *)&pPciDev->children); pNode != NULL;
        pNode = DLL_NEXT3(pNode))
    {
        pPciDev = (PCI_DEVICE3 * )pNode;

        if (pPciDev->type == PCI_BRIDGE_TYPE)
            vxbPciResourceAssign3 (busCtrlID, pPciDev);
    }
}

	
/**********************************************************************
*
* vxbPciProgramBar3 - program bar into configuration space
*
* This routine programs bar into configuration space
*
* RETURNS: N/A
*
* ERRNO
*
*/
void vxbPciProgramBar3
    (
    VXB_DEV_ID3     busCtrlID,
    PCI_DEVICE3   * pPciDev
    )
{
    DL_NODE3 * pNode  = NULL;
    UINT32 i = 0;
    UINT16 cmd = 0, value = 0;
    PCI_HARDWARE_3 hardInfo;

	/*
		在链表中轮询存在的设备
	*/
    for (pNode = DLL_FIRST3((DL_LIST3 *)&pPciDev->children); pNode != NULL;
        pNode = DLL_NEXT3(pNode))
    {
        pPciDev = (PCI_DEVICE3 * )pNode;
	/*
		获取当前设备的ID
	*/
        hardInfo.pciBus = pPciDev->bus;
        hardInfo.pciDev = pPciDev->device;
        hardInfo.pciFunc = pPciDev->func;

        /* program BAR into device */

        if (pPciDev->type == PCI_DEVICE_TYPE)
        {
	 		/*
	 			当前设备是节点
			*/
            cmd = 0;
		
            for (i = 0; i < PCI_MAX_RESOURCE_NUMBER; i ++)
            {
                if ((i == PCI_BRIDGE_MEM_BASE_INDEX) ||
                   (i == PCI_BRIDGE_IO_BASE_INDEX) ||
                   (i == PCI_BRIDGE_PRE_MEM_INDEX)
                   )
                {   
                	continue;
                }
				   
				/*
					这个resource 已经获取了设备的大小
				*/
		        if (pPciDev->resource[i].size != 0)
                {
                printf("ProgramBar device(%d-%d-%d): BAR%d:[0x%08X-0x%08X] align 0x%08X size 0x%08X \n\n",
                    pPciDev->bus, pPciDev->device, pPciDev->func,i,
                    (UINT32 )pPciDev->resource[i].start,
                    (UINT32 )pPciDev->resource[i].end,
                    (UINT32 )pPciDev->resource[i].align,
                    (UINT32 )pPciDev->resource[i].size);

                if (pPciDev->resource[i].type & PCI_BAR_SPACE_IO)
                {
                    /*
                    IO SPEACE enable
                    */
                    (void) VXB_PCI_CFG_WRITE_3(busCtrlID, &hardInfo, (PCI_CFG_BASE_ADDRESS_0 + i*4), 4,
                                               (UINT32)pPciDev->resource[i].start);

                    cmd |= PCI_CMD_IO_ENABLE;
                }
                else
                {
                    /*
                    MEM SPACE ENABLE
                    */
                    if (pPciDev->resource[i].type & PCI_BAR_MEM_ADDR64)
                    {
                        /*
                        64位mem space enable
                       	*/
                        (void) VXB_PCI_CFG_WRITE_3(busCtrlID, &hardInfo,
                            PCI_CFG_BASE_ADDRESS_0 + i*4, 4,
                            (UINT32)pPciDev->resource[i].start);

						#ifdef  _WRS_CONFIG_LP64
                        (void) VXB_PCI_CFG_WRITE_3(busCtrlID, &hardInfo,
                            PCI_CFG_BASE_ADDRESS_0 + (i+1)*4, 4,
                            (UINT32)(pPciDev->resource[i].start >> 32));
						#endif /* _WRS_CONFIG_LP64  */

                        i ++ ;
                    }
                    else
                    {
                        /*
                        32位 mem space enable
                        */
                        (void) VXB_PCI_CFG_WRITE_3(busCtrlID, &hardInfo,
                            PCI_CFG_BASE_ADDRESS_0 + i*4, 4,
                            (UINT32)pPciDev->resource[i].start);
                    }

                    cmd |= PCI_CMD_MEM_ENABLE;
                }
            }
        }

		/*
				更新cmd 寄存器
		*/
	     if (cmd)
	        cmd |= PCI_CMD_MASTER_ENABLE;

	     (void) VXB_PCI_CFG_WRITE_3(busCtrlID, &hardInfo, PCI_CFG_COMMAND, 2, cmd);
    }
    else /* bridge device */
	{
		/*
			桥设备
		*/
	    cmd = 0;
	    for (i = 0; i <= PCI_BRIDGE_PRE_MEM_INDEX; i ++)
	    {
	        if ((i >= PCI_DEV_BAR2_INDEX) && (i <= PCI_DEV_BAR5_INDEX))
	        {   
	        	continue;
	        }

	        if (pPciDev->resource[i].size != 0)
	        {
	            printf("ProgramBar bridge(%d-%d-%d): BAR%d:[0x%08X-0x%08X] align 0x%08X size 0x%08X \n\n",
	                pPciDev->bus, pPciDev->device, pPciDev->func,i,
	                (UINT32 )pPciDev->resource[i].start,
	                (UINT32 )pPciDev->resource[i].end,
	                (UINT32 )pPciDev->resource[i].align,
	                (UINT32 )pPciDev->resource[i].size);
				
	        }

	        if (i == PCI_BRIDGE_MEM_BASE_INDEX)
	            {
	            /*  mem 空间地址分配					
	           		 */
	            if (pPciDev->resource[i].size == 0)
	                {
	                (void) VXB_PCI_CFG_WRITE_3(busCtrlID, &hardInfo,
	                    PCI_CFG_MEM_BASE, 4, 0x0000FFF0);
	                }
	            else
	                {

	                (void) VXB_PCI_CFG_WRITE_3(busCtrlID, &hardInfo,
	                    PCI_CFG_MEM_BASE, 4,
	                    (UINT32)((pPciDev->resource[i].end & 0xFFF00000) |
	                    ((pPciDev->resource[i].start & 0xFFF00000) >> 16)));

	                cmd |= PCI_CMD_MEM_ENABLE;

	                }
	            continue;
	            }

	        if (i == PCI_BRIDGE_IO_BASE_INDEX)
	            {
	            /*
	            			IO 空间地址分配
	            		*/
	            if (pPciDev->resource[i].size == 0)
	                {
	                (void) VXB_PCI_CFG_WRITE_3(busCtrlID, &hardInfo,
	                    PCI_CFG_IO_BASE, 2, 0x00FF);
	                (void) VXB_PCI_CFG_WRITE_3(busCtrlID, &hardInfo,
	                    PCI_CFG_IO_BASE_U, 4, 0x0000FFFFF);
	                }
	            else
	                {

	                (void) VXB_PCI_CFG_WRITE_3(busCtrlID, &hardInfo,
	                    PCI_CFG_IO_BASE, 2,
	                    (UINT16)((pPciDev->resource[i].end & 0xFC00)|
	                    ((pPciDev->resource[i].start & 0xFC00) >> 8)));

	                (void) VXB_PCI_CFG_WRITE_3(busCtrlID, &hardInfo,
	                    PCI_CFG_IO_BASE_U, 4,
	                    (UINT32)((pPciDev->resource[i].end & 0xFFF00000)|
	                    ((pPciDev->resource[i].start) >> 16)));

	                cmd |= PCI_CMD_IO_ENABLE;

	                }
	            continue;
	            }
	        if (i == PCI_BRIDGE_PRE_MEM_INDEX)
	            {
	            /*
	            			pre mem 空间地址分配
	            		*/
	            if (pPciDev->resource[i].size == 0)
	                {
	                (void) VXB_PCI_CFG_WRITE_3(busCtrlID, &hardInfo,
	                    PCI_CFG_PRE_MEM_BASE, 4,
	                    0x0000FFFF);
	                (void) VXB_PCI_CFG_WRITE_3(busCtrlID, &hardInfo,
	                    PCI_CFG_PRE_MEM_BASE_U, 4,
	                    0);
	                (void) VXB_PCI_CFG_WRITE_3(busCtrlID, &hardInfo,
	                    PCI_CFG_PRE_MEM_LIMIT_U, 4,
	                    0);
	                 }
	            else
	                {
	                (void) VXB_PCI_CFG_READ_3(busCtrlID, &hardInfo, PCI_CFG_PRE_MEM_BASE, 2, &value);
	                value &= 0xF;
	                value |= (UINT16)((pPciDev->resource[i].start & 0xFFF00000) >> 16);
	                (void) VXB_PCI_CFG_WRITE_3(busCtrlID, &hardInfo,
	                    PCI_CFG_PRE_MEM_BASE, 2, value);

	                (void) VXB_PCI_CFG_READ_3(busCtrlID, &hardInfo, PCI_CFG_PRE_MEM_LIMIT, 2, &value);
	                value &= 0xF;
	                value |= (UINT16)((pPciDev->resource[i].end & 0xFFF00000)>>16);
	                (void) VXB_PCI_CFG_WRITE_3(busCtrlID, &hardInfo,
	                    PCI_CFG_PRE_MEM_LIMIT, 2, value);

					#ifdef _WRS_CONFIG_LP64
	                (void) VXB_PCI_CFG_WRITE_3(busCtrlID, &hardInfo,
	                    PCI_CFG_PRE_MEM_BASE_U, 4,
	                    (UINT32)(pPciDev->resource[i].start  >> 32));

	                (void) VXB_PCI_CFG_WRITE_3(busCtrlID, &hardInfo,
	                    PCI_CFG_PRE_MEM_LIMIT_U, 4,
	                    (UINT32)(pPciDev->resource[i].end  >> 32));
					#else
	                (void) VXB_PCI_CFG_WRITE_3(busCtrlID, &hardInfo,
	                    PCI_CFG_PRE_MEM_BASE_U, 4,
	                    0);
	                (void) VXB_PCI_CFG_WRITE_3(busCtrlID, &hardInfo,
	                    PCI_CFG_PRE_MEM_LIMIT_U, 4,
	                    0);
					#endif /* _WRS_CONFIG_LP64 */

	                cmd |= PCI_CMD_MEM_ENABLE;
	                }

	            continue;
	            }

	        if (pPciDev->resource[i].size != 0)
	            {
	            /*
	            			桥自身的bar 空间的配置
	            		*/

	            if (pPciDev->resource[i].type & PCI_BAR_SPACE_IO)
	                {
	                (void) VXB_PCI_CFG_WRITE_3(busCtrlID, &hardInfo,
	                    PCI_CFG_BASE_ADDRESS_0 + i*4, 4,
	                    (UINT32)(pPciDev->resource[i].start));

	                cmd |= PCI_CMD_IO_ENABLE;
	                }
	            else
	                {
	                if (pPciDev->resource[i].type & PCI_BAR_MEM_ADDR64)
	                    {
	                    (void) VXB_PCI_CFG_WRITE_3(busCtrlID, &hardInfo,
	                        PCI_CFG_BASE_ADDRESS_0 + i*4, 4,
	                        (UINT32)pPciDev->resource[i].start);
					#ifdef  _WRS_CONFIG_LP64
	                    (void) VXB_PCI_CFG_WRITE_3(busCtrlID, &hardInfo,
	                        PCI_CFG_BASE_ADDRESS_0 + (i+1)*4, 4,
	                        (UINT32)(pPciDev->resource[i].start >> 32));
					#else
	                    (void) VXB_PCI_CFG_WRITE_3(busCtrlID, &hardInfo,
	                        PCI_CFG_BASE_ADDRESS_0 + (i+1)*4, 4,
	                        0);
					#endif /* _WRS_CONFIG_LP64 32 bit system only use 4G address */
	                    i ++;
	                    }
	                else
	                    {
	                    (void) VXB_PCI_CFG_WRITE_3(busCtrlID, &hardInfo,
	                        PCI_CFG_BASE_ADDRESS_0 + i*4, 4,
	                        (UINT32)pPciDev->resource[i].start);
	                    }
	                 cmd |= PCI_CMD_MEM_ENABLE;
	                }

	            }
	        }

	    if (cmd)
	        cmd |= PCI_CMD_MASTER_ENABLE;

	    (void) VXB_PCI_CFG_WRITE_3(busCtrlID, &hardInfo, PCI_CFG_COMMAND, 2, cmd);
	    }

        if (pPciDev->type == PCI_BRIDGE_TYPE)
			/*
				如果是桥设备就递归调用programBar
			*/
            vxbPciProgramBar3 (busCtrlID, pPciDev);

        }

    return;
}


/**********************************************************************
*
* vxbPciDeviceScan3 - scan all of PCI device under bus controller
*
* This routine scans all of PCI device under bus controller
* 扫描当前总线下的所有设备，将有效设备形成设备树链表
* 
* RETURNS: second bus number
*
* ERRNO
*
*/
int vxbPciDeviceScan3
    (
    VXB_DEV_ID3  busCtrlID, /* no used    */
    UINT8          bus,/*current bus number*/
    PCI_DEVICE3   * pParentDev/*prebus bridge*/
    )
{
    UINT8 device, function, secbus;
    UINT8 btemp, headerType;
    UINT8 newPribus, newSecbus, newSubbus;
    UINT32 vendId, command;
    PCI_DEVICE3 * pPciDev;
    PCI_HARDWARE_3 hardInfo;
	
    secbus = bus;

    for (device = 0; device < PCI_MAX_DEV; device ++)
 	{
		 for (function = 0; function < PCI_MAX_FUNC; function ++)
         {
            hardInfo.pciBus = bus;
            hardInfo.pciDev = device;
            hardInfo.pciFunc = function;
			
			/*
			read vendid to juedge device existent or not
			*/
            (void) VXB_PCI_CFG_READ_3( busCtrlID, &hardInfo, PCI_CFG_VENDOR_ID, 4, &vendId);

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
				
			/*
				初始化设备节点
			*/
            pPciDev = (PCI_DEVICE3 *)memAlloc_Usr(sizeof(PCI_DEVICE3));

            if (pPciDev == NULL)
            {               
                printf( "! memAlloc_Usr ERROR\n");
                return ERROR;
            }

            pPciDev->bus    = bus;
            pPciDev->device = device;
            pPciDev->func   = function;
			
			/*
				add this device to parent children list
				children list is the device under same bus 
				head -> device 1,2,3,4..N
				tail   -> device N
				parent is device primbus bridge
				当前设备加到上一级总线桥的设备树链表中
			*/
            DLL_ADD3((DL_LIST3 *)&pParentDev->children, &pPciDev->node);

            pPciDev->parent = pParentDev;
			/*
				update cmd/cache_line_size/latency_timer regs
				初始化 cmd/cache_line_size/latency_timer 寄存器
			*/
            (void) VXB_PCI_CFG_WRITE_3 (busCtrlID, &hardInfo, PCI_CFG_COMMAND, 2, 0);
			
           /* (void) VXB_PCI_CFG_WRITE (busCtrlID, &hardInfo, PCI_CFG_CACHE_LINE_SIZE, 1,  _CACHE_ALIGN_SIZE / 4);*/
            (void) VXB_PCI_CFG_WRITE_3 (busCtrlID, &hardInfo, PCI_CFG_CACHE_LINE_SIZE, 1,  (16 / 4));

            (void) VXB_PCI_CFG_WRITE_3 (busCtrlID, &hardInfo, PCI_CFG_LATENCY_TIMER, 1, PCI_DEF_LTIMER);
			
			
			/*
			 	Check to see if this function belongs to a PCI-PCI bridge 
 				查看设备类型，是桥还是endpoint
			*/       
            (void) VXB_PCI_CFG_READ_3 (busCtrlID, &hardInfo, PCI_CFG_HEADER_TYPE, 1, &headerType);
		
            headerType &= PCI_HEADER_TYPE_MASK;
			
            if ((headerType == PCI_HEADER_TYPE_BRIDGE) || (headerType == PCI_HEADER_PCI_CARDBUS))
            {
                /* 
            		PCI-to-PCI Bridge PCI-to-CardBus bridge 
            		桥设备就递归调用，寻找桥总线下的所有设备
               */
                pPciDev->type = PCI_BRIDGE_TYPE;

                printf("Find Bridge(%d:%d:%d) headerType:0x%X PID/VID:0x%08X \n\n", \
                        bus,device,function, headerType,vendId);

				#if 0 /* jc*/
				(void) VXB_PCI_CFG_READ_3(busCtrlID, &hardInfo,
					PCI_CFG_COMMAND, 2, &command);
				
				command |= PCI_CMD_IO_ENABLE |
					PCI_CMD_MEM_ENABLE | PCI_CMD_MASTER_ENABLE;
				
				/* Make sure the "disable INTx" bit is cleared too. */
				command &= (UINT16)~PCI_CMD_INTX_DISABLE;
				
				(void) VXB_PCI_CFG_WRITE_3(busCtrlID, &hardInfo,
					PCI_CFG_COMMAND, 2, command);
				
				/* Clear any error status bits too. */
				(void) VXB_PCI_CFG_WRITE_3(busCtrlID, &hardInfo,
					PCI_CFG_STATUS, 2, 0xFFFF);
				#endif
				
				/*
					update PCI_CFG_SEC_LATENCY regs
					更新 PCI_CFG_SEC_LATENCY 寄存器
				*/
                (void) VXB_PCI_CFG_WRITE_3 (busCtrlID, &hardInfo, PCI_CFG_SEC_LATENCY, 1, PCI_DEF_LTIMER);
				/*
					get bar size and other information to resource struct 
					获取桥设备的bar空间大小
				*/
                vxbPciResourceGet3 (busCtrlID, pPciDev);
				
				/*
					桥设备下的总线号加1
				*/
                secbus++;

                printf("---Next Bus: %d (%d-%d-%d)--- \n", secbus, bus, device, function);
				
				/*
					init bridge children list
					the device under it will add to this children list
					初始化 桥设备 的子设备链表为空
				*/
                DLL_INIT3((DL_LIST3 *)&pPciDev->children);

                newPribus = bus;
                newSecbus = secbus;
                newSubbus = secbus;
				
				/*
					update PCI_CFG_PRIMARY_BUS/PCI_CFG_SECONDARY_BUS
					update PCI_CFG_SUBORDINATE_BUS with temp oxff
					更新桥设备的上一级总线号，当前总线号
					设备下最大总线号默认先设置为0xff
				*/
                (void) VXB_PCI_CFG_WRITE_3 (busCtrlID, &hardInfo, PCI_CFG_PRIMARY_BUS, 1, newPribus);
                (void) VXB_PCI_CFG_WRITE_3 (busCtrlID, &hardInfo, PCI_CFG_SECONDARY_BUS, 1, newSecbus);
                (void) VXB_PCI_CFG_WRITE_3 (busCtrlID, &hardInfo, PCI_CFG_SUBORDINATE_BUS, 1, 0xff);
				
				/*
					recursion this function to init all  device under this bridge
					递归调用，寻找桥下的所有设备
					返回所有设备中最大的总线号
				*/
				/**/
				/* read remote endp status*/
				/**/
				/*if (pciRootRes->endBusNumber) <== dts: bus-range = <0 255>; */
				if (bus < PCI_MAX_BUS)  /* bridge */
                {
                	printf("\nrecall vxbPciDeviceScan3: bus=%d, secbus=%d \n\n", bus, secbus);
					secbus = (UINT8)vxbPciDeviceScan3 (busCtrlID, secbus, pPciDev);
                }
                else  /* root*/
                {
                    secbus = 0;
                    (void) VXB_PCI_CFG_WRITE_3 (busCtrlID, &hardInfo, PCI_CFG_SECONDARY_BUS, 1, secbus);
                }
                
				/*
					update PCI_CFG_SUBORDINATE_BUS with biggest secbus under this bridge
					更新桥的最大总线号
				*/
                newSubbus = secbus;
                (void) VXB_PCI_CFG_WRITE_3 (busCtrlID, &hardInfo, PCI_CFG_SUBORDINATE_BUS, 1, newSubbus);
        	}
			else /* endpoirt */
        	{
				/*
				device
				节点设备
				*/
                pPciDev->type = PCI_DEVICE_TYPE;

                printf("Find Device(%d:%d:%d) headerType:%d PID/VID:0x%08X \n\n", \
                         bus,device,function, headerType,vendId);

				/*
					get bar size and other information to resource struct 
					获取节点设备的所有bar空间大小
				*/
                vxbPciResourceGet3 (busCtrlID, pPciDev);
            }

           
			/*
  				Proceed to next device if this is a single function device
  				单功能设备属性的设备就不再查找下一功能号
			*/            
            if (function == 0)
            {
                (void) VXB_PCI_CFG_READ_3 (busCtrlID, &hardInfo, PCI_CFG_HEADER_TYPE, 1, &btemp);
                
                if ((btemp & PCI_HEADER_MULTI_FUNC) == 0)
                {    
                	break; /* No more functions - proceed to next PCI device */
                }
            }  /* if (function == 0) */
        }/* for (function = 0; function < PCI_MAX_FUNC; function ++) */
    }/* for (device = 0; device < PCI_MAX_DEV; device ++) */

	/*
	返回当前总线下的最大总线号
	*/
    return secbus;
}
	

/******************************************************************************
*
* vxbPciAutoConfigFunc3 - automatically configure and announce all PCI
*
* This routine configure all PCI headers and announce PCI device to Vxbus
* system.
*
* RETURNS: OK or ERROR when allocate VXB_PCI_CTRL failed
*
* ERRNO
*
*/

STATUS vxbPciAutoConfigFunc3(void)
{
    PCI_ROOT_CHAIN3 * pciRootChain;
    PCI_ROOT_RES3   * pciRootRes;

    UINT8 i;
    PCI_DEVICE3 * pPciDev;
	
    int busCtrlID =0;
        
    /* Root dummy pci node */
	/*
	init PCI_DEVICE3 top 
	bus:device:fun : ff_ff_ff 
	pci bridge type
	dsp all pci space is (0x60000000-0x6fffffff)
	0x60000000 - 0x60ffffff: dsp type0 address,reserved
	0x61000000 - 0x6effffff: user mem space
	0x6f000000 - 0x6f0fffff: user type space ( outbound window to send type0/1)
	0x6f100000 - 0x6fffffff: reserved
	
	resource[pci_bridge_mem_base_index].pool.base is user mem space start
	resource[pci_bridge_mem_base_index].pool.size is user mem space size
	初始化设备树顶点
	*/
    pPciDev = (PCI_DEVICE3 *)memAlloc_Usr(sizeof(PCI_DEVICE3));
    if (pPciDev == NULL)
    {
        printf( "vxbMemAlloc PCI_DEVICE3 ERROR!\n");
        return ERROR;
    }

    pPciDev->bus    = ~0;
    pPciDev->device = ~0;
    pPciDev->func   = ~0;
    pPciDev->parent = NULL;
    pPciDev->type   = PCI_BRIDGE_TYPE;

	pPciDev->resource[PCI_BRIDGE_MEM_BASE_INDEX].pool.base = PCI_MEM_BASE;
	pPciDev->resource[PCI_BRIDGE_MEM_BASE_INDEX].pool.size = PCI_MEM_SIZE;
    DLL_INIT3((DL_LIST3 *)&pPciDev->children);

	/*
	device scan 
	set all valid device/bridge bus/device/fun 
	get all valid device/bridge mem size
	扫描所有有效设备，形成设备树链表
	*/
    (void) vxbPciDeviceScan3 ( busCtrlID, PCI_BASE_BUS_NUM, pPciDev);

    if (!DLL_EMPTY3(&pPciDev->children))
    {
        /*
    	sort all device/bridge mem space
    	根据设备树链表中所有设备的资源，形成资源树链表
    	*/
        vxbPciResourceSort3 (busCtrlID, pPciDev, 0);

		/*
    	assign all device/bridge mem address
    	根据资源树链表，分配实际的地址范围
    	*/
        vxbPciResourceAssign3 (busCtrlID, pPciDev);

		/*
    	program to all device/bridge mem address
    	根据分配的地址范围，写到设备树链表下的所有设备空间中
    	*/
        vxbPciProgramBar3(busCtrlID, pPciDev);
    }

    return ( OK );
}


void pl_pcie_rc_scan(void)
{
	vxbPciAutoConfigFunc3();
	return;
}


int alloc_mem_num = 0;
/*#define ALLOC_MEM_SIZE   (0x1000)      // 4K*/
#define ALLOC_MEM_SIZE   (0x10000)      /* 64K*/


#if 0  /* bare-metal mode*/

#define ALLOC_MEM_START  (0xE0000000)

UINT32 memAlloc_Usr(int size)  /* no OS mode*/
{
	UINT32 memalloc;
	UINT32 i;

	/*memalloc = ALLOC_MEM_START + ALLOC_MEM_SIZE*lloc_mem_num;*/
	memalloc = ALLOC_MEM_START + size*alloc_mem_num;

	/*init value is 0*/
	/*this is important */
	/*for (i=memalloc ; i <= (memalloc + ALLOC_MEM_SIZE) ; i=i+4)*/
	for (i=memalloc ; i <= (memalloc + size) ; i=i+4)
	{
		*(UINT32 *)i = 0;
	}

	alloc_mem_num++;
	
	return memalloc;
}

#else  /* with OS mode*/

UINT32 memAlloc_Usr(int size)
{
	UINT32 * memalloc = NULL;
	UINT32  malloc_addr = 0;
	
	memalloc = (UINT32 *)malloc(size);
	if (memalloc != NULL)
	{
		memset((char *)memalloc, 0, size);
		malloc_addr = &memalloc[0];
	}

	return malloc_addr;
}

#endif


