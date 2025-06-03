/* vxbFdtZynq7kAxiEnd.c - Xilinx Zynq-7000 AXI ETHERNET VxBus END driver */

/*
 * Copyright (c) 2014-2016 Wind River Systems, Inc.
 *
 * The right to copy, distribute, modify or otherwise make use
 * of this software may be licensed only pursuant to the terms
 * of an applicable Wind River license agreement.
 */
 
/*
modification history
--------------------
20may16,m_w  fixed DMA tail descriptor update issue.(V7PRO-3062)
04apr16,dlk  EIOCGFLAGS argument is an INT32 pointer (V7PRO-2981).
30mar16,cfm  corrected EIOCSADDR case in ioctl (V7PRO-2948)
28mar16,y_f  corrected return value for adding/deleting multicast addresses
             (V7PRO-2922)
22jan16,m_w  fix multicast filter issue. (V7PRO-2789)
23oct15,m_w  update poll mode function. (V7PRO-2525)
06aug15,m_w  initlialize the ifPollInterval in EIOCGPOLLCONF case. (V7PRO-2249)
13jul15,m_w  cacheInvalidate mblk after DMA done in RX path. (V7PRO-2020)
20apr15,m_w  cacheInvalidate new mblk in RX path. (V7PRO-1694)
24apr15,y_c  increased AXI_TUPLE_CNT value and some minor update. (V7PRO-1992) 
14feb15,yjw  get host clock frequency from DTS (V7PRO-1751)
06oct14,y_c  added pMblk number check in fmqlAxiEndEncap. (V7PRO-1442)
24oct14,y_c  added fmqlAxiMdioSetup before phy operation. (V7PRO-1304)
09oct14,y_c  updated poll receive function. (V7PRO-1310)
20aug14,y_c  added multicast support. (V7PRO-1183)
18jun14,y_c  created from vxbZynq7kAxiEnd.c. (US40549)
*/

/*
DESCRIPTION
This module implements a Xilinx LogiCORE IP AXI Ethernet module
network interface driver. The AXI driver is fully compliant with the IEEE 802.3
802.3 10Base-T and 100Base-T specifications. Hardware support of
the Media Independent Interface (MII) is built-in in the chip.

The AXI driver uses the ifmedia interface, which allows media selection
to be controlled by the ifconfig utility, or the SIOCGIFMEDIA/SIOCSIFMEDIA
ioctl API.  It also uses the miiBus subsystem to manage its PHYs, so no
MII handling code is needed within the AXI driver itself.
 
To add the driver to the vxWorks image, add the following component to the
kernel configuration.

\cs
vxprj component add DRV_END_FDT_XLNX_ZYNQ7K_AXI
\ce

The AXI Ethernet Controller device should be bound to a device tree node which
requires below properties:

\cs
compatible:     Specify the programming model for the device.
                It should be set to "fmsh,axi-enet" and is used
                by vxbus GEN2 for device driver selection.

device_type:    Specify the device type for the device. It should be set to 
                "network".

interrupts:     Specify interrupt vector of the interrupts that are generated
                by this device.

interrupt-parent: This property is available to define an interrupt
                parent. if it is missing from a device, it's interrupt parent
                is assumed to be its device tree parent.
                
reg:            Specify the address of the device's resources within
                the address space defined by its parent bus.
 
phy-handle:     Specify the phy information of the device.
\ce

Below is an example:

\cs
        axi_eth_0: ethernet@83C00000
            {
            #size-cells = <0>;
            #address-cells = <1>;
            device_type = "network";
            compatible = "fmsh,axi-enet";
            reg = <0x83C00000 0x40000>, /@ AXI MAC controller @/
                  <0x80400000 0x10000>; /@ AXI DMA controller @/
            clock-frequency = <76923080>;
            local-mac-address = [ 00 0A 35 11 22 44 ];
            interrupts = <89>,
                         <90>;
            interrupt-parent = <&intc>;
            phy-handle = <&phy1>;

            phy1: phy@0
                {
                #size-cells = <0>;
                #address-cells = <1>;
                compatible = "genericPhy";
                reg = <0>;
                };
            };
\ce

SEE ALSO: vxBus, miiBus, ifLib, endLib
\tb "pg138-axi-ethernet(AXI 1G/2.5G Ethernet Subsystem LogiCORE IP Product Guide).pdf"
\tb "pg021-axi-dma(AXI DMA LogiCORE IP Product Guide).pdf"
*/

/* includes */

#include <vxWorks.h>
#include <intLib.h>
#include <logLib.h>
#include <muxLib.h>
#include <netLib.h>
#include <netBufLib.h>
#include <semLib.h>
#include <sysLib.h>
#include <wdLib.h>
#include <etherMultiLib.h>
#include <end.h>
#define END_MACROS
#include <endLib.h>
//#include <vmLib.h>
#include <cacheLib.h>
#include <hwif/vxbus/vxBus.h>
#include <hwif/vxbus/hwConf.h>
#include <hwif/vxbus/vxbPlbLib.h>
#include <hwif/util/vxbParamSys.h>
#include "../h/mii/miiBus.h"
#include "../h/vxbus/vxbAccess.h"
#include "../h/hEnd/hEnd.h"

//#include <hwif/buslib/vxbFdtLib.h>
//#include <hwif/methods/vxbMiiMethod.h>
//#include <subsys/int/vxbIntLib.h>
//#include <subsys/timer/vxbTimerLib.h>
//#include <miiBus.h>

#include "vxbFdtFmqlAxiEnd.h"

/* define */

#define FMQL_AXI_END_DEBUG
#undef FMQL_AXI_END_DEBUG

#ifdef FMQL_AXI_END_DEBUG
#include <private/kwriteLibP.h>         /* _func_kprintf */

#define FMQL_AXI_END_DBG_OFF     0x00000000
#define FMQL_AXI_END_DBG_WARN    0x00000001
#define FMQL_AXI_END_DBG_ERR     0x00000002
#define FMQL_AXI_END_DBG_INFO    0x00000004
#define FMQL_AXI_END_DBG_ALL     0xffffffff
LOCAL UINT32 gvFmqlAxiEndDbgMask = FMQL_AXI_END_DBG_ALL;

#define FMQL_AXI_END_DBG_MSG
#define FMQL_AXI_END_DBG_MSG(mask,...)                                   \
do                                                          \
{                                                           \
    if ((gvFmqlAxiEndDbgMask & (mask)) || ((mask) == FMQL_AXI_END_DBG_ALL))          \
    {                                                       \
        if (_func_kprintf != NULL)                          \
        {                                                   \
        (* _func_kprintf)("%s,%d, ",__FUNCTION__,__LINE__); \
        (* _func_kprintf)(__VA_ARGS__);                     \
        }                                                   \
        else                                                \
        {                                                   \
        printf("%s,%d, ",__FUNCTION__,__LINE__);            \
        printf(__VA_ARGS__);                                \
        }                                                   \
    }                                                       \
}while (0)

#else

#define FMQL_AXI_END_DBG_MSG(...)

#endif  /* FMQL_AXI_END_DEBUG */

#define AXI_TX_DRV_STAT /* Enable driver Tx stat */
#define AXI_END_DROP_RX_MULTI_DESC

/* forward declarations */

IMPORT FUNCPTR _func_m2PollStatsIfPoll;

//
// add by jc
//
//#define PL_POLL_MODE
void pl_hp_init(void);


/* VxBus methods */

/* miiBus methods */

LOCAL STATUS fmqlAxiPhyRead(
    VXB_DEVICE_ID pDev,
    UINT8 phyAddr,
    UINT8 regAddr,
    UINT16 * dataVal
    );
LOCAL STATUS fmqlAxiPhyWrite   (VXB_DEVICE_ID, UINT8, UINT8, UINT16);
LOCAL STATUS fmqlAxiLinkUpdate (VXB_DEVICE_ID);
LOCAL STATUS fmqlAxiEndInstUnlink
    (
    VXB_DEVICE_ID pDev,  /* pointer to device */
    void * unused       /* unused pointer */
    );

/* mux methods */

LOCAL void fmqlAxiMuxConnect (VXB_DEVICE_ID, void *);

LOCAL struct vxbDeviceMethod fmqlAxiEthMethods[] =
   {
   //{ DEVMETHOD(vxbDevProbe),    (FUNCPTR)fmqlAxiEndProbe},
   //{ DEVMETHOD(vxbDevAttach),   (FUNCPTR)fmqlAxiEndAttach},
   DEVMETHOD(miiRead,        (FUNCPTR)fmqlAxiPhyRead),
   DEVMETHOD(miiWrite,       (FUNCPTR)fmqlAxiPhyWrite),
   DEVMETHOD(miiMediaUpdate, (FUNCPTR)fmqlAxiLinkUpdate),
   DEVMETHOD(muxDevConnect,    (FUNCPTR)fmqlAxiMuxConnect),
   DEVMETHOD(vxbDrvUnlink,   (FUNCPTR)fmqlAxiEndInstUnlink),
   {0, 0}
   };  

/* END functions */
LOCAL void fmqlAxiEndInstInit(VXB_DEVICE_ID pDev);
LOCAL void fmqlAxiEndInstInit(VXB_DEVICE_ID pDev);
LOCAL void fmqlAxiEndInstInit2(VXB_DEVICE_ID pDev);
LOCAL void fmqlAxiEndInstConnect(VXB_DEVICE_ID pDev);

LOCAL STATUS    fmqlAxiReset              (VXB_DEVICE_ID pDev);
LOCAL END_OBJ * fmqlAxiEndLoad            (char *, void *);
LOCAL STATUS    fmqlAxiEndUnload          (END_OBJ *);
LOCAL int       fmqlAxiEndIoctl           (END_OBJ *, int, caddr_t);
LOCAL STATUS    fmqlAxiEndMCastAddrAdd    (END_OBJ *, char *);
LOCAL STATUS    fmqlAxiEndMCastAddrDel    (END_OBJ *, char *);
LOCAL STATUS    fmqlAxiEndMCastAddrGet    (END_OBJ *, MULTI_TABLE *);
LOCAL void      fmqlAxiEndHashTblPopulate (AXI_DRV_CTRL *);
LOCAL STATUS    fmqlAxiEndStatsDump       (AXI_DRV_CTRL *);
LOCAL void      fmqlAxiEndRxConfig        (AXI_DRV_CTRL *);
LOCAL STATUS    fmqlAxiMdioSetup          (AXI_DRV_CTRL *);
LOCAL STATUS    fmqlAxiEndStart           (END_OBJ *);
LOCAL STATUS    fmqlAxiEndStop            (END_OBJ *);
LOCAL int       fmqlAxiEndSend            (END_OBJ *, M_BLK_ID);
LOCAL STATUS    fmqlAxiEndPollStart       (AXI_DRV_CTRL * pDrvCtrl);
LOCAL STATUS    fmqlAxiEndPollStop        (AXI_DRV_CTRL * pDrvCtrl);
LOCAL int       fmqlAxiEndEncap           (AXI_DRV_CTRL * pDrvCtrl,
                                             M_BLK_ID pMblk);
LOCAL STATUS    fmqlAxiEndPollSend        (END_OBJ *, M_BLK_ID);
LOCAL int       fmqlAxiEndPollReceive     (END_OBJ *, M_BLK_ID);
LOCAL void      fmqlAxiEndDmaRxInt        (AXI_DRV_CTRL *);
LOCAL void      fmqlAxiEndDmaTxInt        (AXI_DRV_CTRL *);
LOCAL void      fmqlAxiEndRxHandle        (void *);
LOCAL void      fmqlAxiEndTxHandle        (void *);

LOCAL NET_FUNCS fmqlAxiNetFuncs = 
    {
    fmqlAxiEndStart,          /* start func.                        */
    fmqlAxiEndStop,           /* stop func.                         */
    fmqlAxiEndUnload,         /* unload func.                       */
    fmqlAxiEndIoctl,          /* ioctl func.                        */
    fmqlAxiEndSend,           /* send func.                         */
    fmqlAxiEndMCastAddrAdd,   /* multicast add func.                */
    fmqlAxiEndMCastAddrDel,   /* multicast delete func.             */
    fmqlAxiEndMCastAddrGet,   /* multicast get fun.                 */
    fmqlAxiEndPollSend,       /* polling send func.                 */
    fmqlAxiEndPollReceive,    /* polling receive func.              */
    endEtherAddressForm,        /* put address info into a NET_BUFFER */
    endEtherPacketDataGet,      /* get pointer to data in NET_BUFFER  */
    endEtherPacketAddrGet       /* Get packet addresses               */
    };
	

/* default queue parameters */
LOCAL HEND_RX_QUEUE_PARAM fmqlAxiEndRxQueueDefault =
    {
    NULL,       /* jobQueId */
    0,          /* priority */
    0,          /* rbdNum */
    0,          /* rbdTupleRatio */
    0,          /* rxBufSize */
    NULL,       /* pBufMemBase */
    0,          /* rxBufMemSize */
    0,          /* rxBufMemAttributes */
    NULL,       /* rxBufMemFreeMethod */
    NULL,       /* pRxBdBase */
    0,          /* rxBdMemSize */
    0,          /* rxBdMemAttributes */
    NULL        /* rxBdMemFreeMethod */
    };

LOCAL HEND_TX_QUEUE_PARAM fmqlAxiEndTxQueueDefault =
    {
    NULL,       /* jobQueId */
    0,          /* priority */
    0,          /* tbdNum */
    0,          /* allowedFrags */
    NULL,       /* pTxBdBase */
    0,          /* txBdMemSize */
    0,          /* txBdMemAttributes */
    NULL        /* txBdMemFreeMethod */
    };

LOCAL VXB_PARAMETERS fmqlAxiEndParamDefaults [] =
    {
        {
        "rxQueue00", VXB_PARAM_POINTER,
            {
            (void *) &fmqlAxiEndRxQueueDefault
            }
        },
        
        {
        "txQueue00", VXB_PARAM_POINTER,
            {
            (void *) &fmqlAxiEndTxQueueDefault
            }
        },
        
        {"jumboEnable", VXB_PARAM_INT32, {(void *)0}},
        
        {
        NULL, VXB_PARAM_END_OF_LIST,
            {
            NULL
            }
        }
    };


LOCAL struct drvBusFuncs fmqlAxiEndFuncs =
    {
    fmqlAxiEndInstInit,     /* devInstanceInit  */
    fmqlAxiEndInstInit2,    /* devInstanceInit2 */
    fmqlAxiEndInstConnect   /* devConnect       */
    };


LOCAL struct vxbPlbRegister fmqlAxiEndDevPlbRegistration =
    {
        {
        NULL,               /* pNext */
        VXB_DEVID_DEVICE,   /* devID */
        VXB_BUSID_PLB,      /* busID = PLB */
        VXB_VER_4_0_0,      /* vxbVersion */
        AXI_END_NAME,       /* drvName */
        &fmqlAxiEndFuncs, /* pDrvBusFuncs */
        NULL,               /* pMethods */
        NULL,               /* devProbe */
        fmqlAxiEndParamDefaults /* pParamDefaults */
        },
    };

void fmqlAxiEndRegister (void)
    {
    	vxbDevRegister ((struct vxbDevRegInfo *) &fmqlAxiEndDevPlbRegistration);
    }

LOCAL void fmqlAxiEndInstInit
    (
    VXB_DEVICE_ID pDev  /* pointer to device */
    )
    {
    const struct hcfDevice * pHcf;

    /* Always use the unit number allocated to us in the hwconf file */
    pHcf = (struct hcfDevice *) pDev->pBusSpecificDevInfo;

    vxbInstUnitSet (pDev, pHcf->devUnit);

    pDev->pMethods = &fmqlAxiEthMethods [0];

    }

LOCAL void fmqlAxiEndInstInit2
    (
    VXB_DEVICE_ID pDev
    )
    {
    AXI_DRV_CTRL     * pDrvCtrl;
    //VXB_FDT_DEV      * pFdtDev = (VXB_FDT_DEV *)vxbDevIvarsGet(pDev);
    //VXB_RESOURCE_ADR * pResAdr = NULL;
    //VXB_RESOURCE     * pRes;
    int                proplen = 0;
    UINT32           * prop;

    pDrvCtrl = malloc(sizeof(AXI_DRV_CTRL));

    if (pDrvCtrl == NULL)
        {
        return ERROR;
        }
	
	bzero ((char *) pDrvCtrl, sizeof (AXI_DRV_CTRL));
	
    pDev->pDrvCtrl = pDrvCtrl;
	
	pDrvCtrl->hostClock = HARD_OSC_HZ;
	
    pDrvCtrl->axiEthDev = pDev;
    pDrvCtrl->axiBar = pDev->pRegBase [0];
    vxbRegMap (pDev, 0, &pDrvCtrl->handle[0]);

    //pDrvCtrl->axiEthDev = pDev;
    pDrvCtrl->axiDmaBar = pDev->pRegBase [1];
    vxbRegMap (pDev, 1, &pDrvCtrl->handle[1]);	


	pDrvCtrl->axiDevSem = semMCreate(SEM_Q_PRIORITY 
                          | SEM_DELETE_SAFE | SEM_INVERSION_SAFE);

    /* using the direct way, no vxdmabuflib anymore */

    pDrvCtrl->axiRxDescMem = 
        cacheDmaMalloc (sizeof(AXI_DESC) * AXI_RX_DESC_CNT);

    if (!pDrvCtrl->axiRxDescMem)
        {
        FMQL_AXI_END_DBG_MSG (FMQL_AXI_END_DBG_ERR, "allocate rx descriptor space failed\n");
        goto FAIL;
        }

    memset (pDrvCtrl->axiRxDescMem, 0, 
            sizeof(AXI_DESC) * AXI_RX_DESC_CNT);

    pDrvCtrl->axiTxDescMem = 
        cacheDmaMalloc (sizeof(AXI_DESC) * AXI_TX_DESC_CNT);

    if (!pDrvCtrl->axiTxDescMem)
        {
        FMQL_AXI_END_DBG_MSG (FMQL_AXI_END_DBG_ERR, "allocate tx descriptor space failed\n");
        goto FAIL;
        }

    memset (pDrvCtrl->axiTxDescMem, 0, 
            sizeof(AXI_DESC) * AXI_TX_DESC_CNT);

    pDrvCtrl->coalesceCountRx = AXI_DMA_DFT_RX_THRESHOLD;
    pDrvCtrl->coalesceCountTx = AXI_DMA_TX_THRESHOLD;

    (void)vxbIntConnect (pDev, 0, fmqlAxiEndDmaTxInt, pDrvCtrl);
    (void)vxbIntConnect (pDev, 1, fmqlAxiEndDmaRxInt, pDrvCtrl);

	/* configure MDIO before phy operation */
    if (fmqlAxiMdioSetup(pDrvCtrl) == ERROR)
        {
        FMQL_AXI_END_DBG_MSG (FMQL_AXI_END_DBG_ERR, "fmqlAxiMdioSetup() return ERROR\n");
        goto FAIL;
        }


	//
	// dma to ddr: HP channel init & reopen, add by jc 
	//
	//pl_hp_init();
    return OK;

FAIL:
    if (pDrvCtrl)
        {
        if (pDrvCtrl->axiTxDescMem)
           (void)cacheDmaFree (pDrvCtrl->axiTxDescMem);

        if (pDrvCtrl->axiRxDescMem)
           (void)cacheDmaFree (pDrvCtrl->axiRxDescMem);
		semDelete (pDrvCtrl->axiDevSem);
        free (pDrvCtrl);
        }
        
    return ERROR;
    }

/*******************************************************************************
*
* altSocGen5DwInstConnect -  VxBus instConnect handler
*
* This function implements the VxBus instConnect handler for an altSocGen5Dw
* device instance.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

LOCAL void fmqlAxiEndInstConnect
    (
    VXB_DEVICE_ID pDev /* pointer to device */
    )
    {
    return;
    }

LOCAL STATUS fmqlAxiEndInstUnlink
    (
    VXB_DEVICE_ID pDev,  /* pointer to device */
    void * unused       /* unused pointer */
    )
    {
    AXI_DRV_CTRL * pDrvCtrl;
    pDrvCtrl = pDev->pDrvCtrl;

    /*
     * Stop the device and detach from the MUX.
     * Note: it's possible someone might try to delete
     * us after our vxBus instantiation has completed,
     * but before anyone has called our muxConnect method.
     * In this case, there'll be no MUX connection to
     * tear down, so we can skip this step.
     */

    if (pDrvCtrl->axiMuxDevCookie != NULL)
        {
        if (muxDevStop (pDrvCtrl->axiMuxDevCookie) != OK)
            {
            DBG_LOG_MSG("ERR: muxDevStop did not return OK, returning ERROR\n",
                        1,2,3,4,5,6);
            return (ERROR);
            }

        /* Detach from the MUX */

        if (muxDevUnload (pDev->pName, pDev->unitNumber) != OK)
            {
            DBG_LOG_MSG("ERR: muxDevUnload did not return OK, returning ERROR\n"
                        ,1,2,3,4,5,6);
            return (ERROR);
            }
        }

    /* free the memory of ring descriptor */

    if (pDrvCtrl->axiRxDescMem != NULL)
        {
        cacheDmaFree (pDrvCtrl->axiRxDescMem);
        pDrvCtrl->axiRxDescMem = NULL;
        }

    if (pDrvCtrl->axiTxDescMem != NULL)
        {
        cacheDmaFree (pDrvCtrl->axiTxDescMem);
        pDrvCtrl->axiTxDescMem = NULL;
        }

    /* destroy our MII bus and child PHYs */

    if (pDrvCtrl->axiMiiBus != NULL)
        {
        miiBusDelete (pDrvCtrl->axiMiiBus);
        semDelete (pDrvCtrl->axiMiiBus);
        }

    /* disconnect from the ISR */
    vxbIntDisconnect (pDev, 0, fmqlAxiEndDmaRxInt, pDrvCtrl);
	vxbIntDisconnect (pDev, 1, fmqlAxiEndDmaTxInt, pDrvCtrl);

    /* destroy the adapter context */
    free (pDrvCtrl);
    pDev->pDrvCtrl = NULL;

    return (OK);
    }

/*******************************************************************************
 *
 * fmqlAxiPhyRead - miiBus miiRead method
 *
 * This function implements an miiRead() method that allows PHYs on the miiBus
 * to access our MII management registers.
 *
 * RETURNS: ERROR if invalid PHY addr or register is specified, else OK
 *
 * ERRNO: N/A
 */

LOCAL STATUS fmqlAxiPhyRead
    (
    VXB_DEVICE_ID pDev,
    UINT8 phyAddr,
    UINT8 regAddr,
    UINT16 * dataVal
    )
    {
    AXI_DRV_CTRL * pDrvCtrl;
    UINT32 value, rc, data;
    int timeoutUs = AXI_PHY_TIMEOUT;

    if (pDev == NULL)
        {
        DBG_LOG_MSG("pDev is NULL, returning ERROR\n",1,2,3,4,5,6);
        return (ERROR);
        }

    pDrvCtrl = pDev->pDrvCtrl;

    if (phyAddr > 31)
        {
        return ERROR;
        }

    /* return value check is not needed */

    (void)semTake (pDrvCtrl->axiDevSem, WAIT_FOREVER);

    value = (((phyAddr << AXI_ENET_MDIO_MCR_PHYAD_SHIFT) & AXI_ENET_MDIO_MCR_PHYAD_MASK)
             | ((regAddr << AXI_ENET_MDIO_MCR_REGAD_SHIFT) & AXI_ENET_MDIO_MCR_REGAD_MASK)
             | AXI_ENET_MDIO_MCR_INITIATE_MASK | AXI_ENET_MDIO_MCR_OP_READ_MASK);

    /* set MDIO-MCR register to get PHY register value */

    AXI_WRITE_4(pDrvCtrl, AXI_ENET_MDIO_MCR_OFFSET, value);

    do
        {
        data = AXI_READ_4(pDrvCtrl, AXI_ENET_MDIO_MCR_OFFSET);
        if (timeoutUs == 0)
            {
            (void)semGive (pDrvCtrl->axiDevSem);
            FMQL_AXI_END_DBG_MSG (FMQL_AXI_END_DBG_ERR, "TIMEOUT: AXI MDIO wait until ready\n");
            return (ERROR);
            }
        timeoutUs--;
        (void)vxbUsDelay(1);
        } while ((data & AXI_ENET_MDIO_MCR_READY_MASK) == 0);

    rc = AXI_READ_4(pDrvCtrl, AXI_ENET_MDIO_MRD_OFFSET);

    /* read MDIO-MRD register to get PHY register value */

    *dataVal = rc & 0x0000FFFF; 

    /* return value check is not needed */

    (void)semGive (pDrvCtrl->axiDevSem);

    return (OK);
    }

/*******************************************************************************
 *
 * fmqlAxiPhyWrite - miiBus miiWrite method
 *
 * This function implements an miiWrite() method that allows PHYs on the miiBus
 * to access our MII management registers.
 *
 * RETURNS: ERROR if invalid PHY addr or register is specified, else OK
 *
 * ERRNO: N/A
 */

LOCAL STATUS fmqlAxiPhyWrite
    (
    VXB_DEVICE_ID pDev,
    UINT8      phyAddr,
    UINT8      regAddr,
    UINT16     dataVal
    )
    {
    AXI_DRV_CTRL * pDrvCtrl;
    STATUS rval = ERROR;
    UINT32 data, val = (UINT32) dataVal;
    int timeoutUs = AXI_PHY_TIMEOUT;

    if (pDev == NULL)
        {
        DBG_LOG_MSG("pDev is NULL, returning ERROR\n",1,2,3,4,5,6);
        return (ERROR);
        }

    pDrvCtrl = pDev->pDrvCtrl;

    if (phyAddr > 31)
        {
        return ERROR;
        }

    (void)semTake (pDrvCtrl->axiDevSem, WAIT_FOREVER);

    AXI_WRITE_4(pDrvCtrl, AXI_ENET_MDIO_MWD_OFFSET, val);

    val = (((phyAddr << AXI_ENET_MDIO_MCR_PHYAD_SHIFT) & AXI_ENET_MDIO_MCR_PHYAD_MASK)
            | ((regAddr << AXI_ENET_MDIO_MCR_REGAD_SHIFT) & AXI_ENET_MDIO_MCR_REGAD_MASK)
            | AXI_ENET_MDIO_MCR_INITIATE_MASK | AXI_ENET_MDIO_MCR_OP_WRITE_MASK);

    AXI_WRITE_4(pDrvCtrl, AXI_ENET_MDIO_MCR_OFFSET, val);

    do
        {
        data = AXI_READ_4(pDrvCtrl, AXI_ENET_MDIO_MCR_OFFSET);

        if (timeoutUs == 0)
            {
            (void)semGive (pDrvCtrl->axiDevSem);
            FMQL_AXI_END_DBG_MSG (FMQL_AXI_END_DBG_ERR,"TIMEOUT: AXI MDIO wait until ready\n");
            return ERROR;
            }

        timeoutUs--;
        (void)vxbUsDelay(1);
        } while ((data & AXI_ENET_MDIO_MCR_READY_MASK) == 0);

    rval = OK;

    (void)semGive (pDrvCtrl->axiDevSem);

    return (rval);
    }

/*******************************************************************************
 *
 * fmqlAxiLinkUpdate - miiBus miiLinkUpdate method
 *
 * This function implements an miiLinkUpdate() method that allows
 * miiBus to notify us of link state changes. This routine will be
 * invoked by the miiMonitor task when it detects a change in link
 * status. Normally, the miiMonitor task checks for link events every
 * two seconds.
 *
 * Once we determine the new link state, we will announce the change
 * to any bound protocols via muxError(). We also update the ifSpeed
 * fields in the MIB2 structures so that SNMP queries can detect the
 * correct link speed.
 *
 * RETURNS: ERROR if obtaining the new media setting fails, else OK
 *
 * ERRNO: N/A
 */

LOCAL STATUS fmqlAxiLinkUpdate
    (
    VXB_DEVICE_ID pDev
    )
    {
    AXI_DRV_CTRL * pDrvCtrl;
    UINT32 oldStatus;

    VXB_DEVICE_ID pPhy;
    MII_DRV_CTRL *pMiiDrvCtrl;



    pDrvCtrl = pDev->pDrvCtrl;
	
    if (pDrvCtrl == NULL)
        {
        return (ERROR);
        }

    (void)semTake (pDrvCtrl->axiDevSem, WAIT_FOREVER);
	
    oldStatus = pDrvCtrl->axiCurStatus;


	pMiiDrvCtrl = ((MII_DRV_CTRL *)pDrvCtrl->axiMiiBus->pDrvCtrl);
    pPhy = pMiiDrvCtrl->miiActivePhy;
	

    if (miiBusModeGet(pDrvCtrl->axiMiiBus, &pDrvCtrl->axiCurMedia,
                      &pDrvCtrl->axiCurStatus) == ERROR)
        {
        FMQL_AXI_END_DBG_MSG (FMQL_AXI_END_DBG_ERR,"miiBusModeGet() return ERROR\n");
        (void)semGive (pDrvCtrl->axiDevSem);
        return (ERROR);
        }

    /* configure full duplex mode accordingly */

    if (IFM_SUBTYPE(pDrvCtrl->axiCurMedia) == IFM_1000_T)
        {
        AXI_WRITE_4(pDrvCtrl, AXI_ENET_EMMC_OFFSET,
                    AXI_ENET_EMMC_LINKSPD_1000);
        pDrvCtrl->axiEndObj.mib2Tbl.ifSpeed = 1000000000;
        }
    else if (IFM_SUBTYPE(pDrvCtrl->axiCurMedia) == IFM_100_TX)
        {
        AXI_WRITE_4(pDrvCtrl, AXI_ENET_EMMC_OFFSET,
                    AXI_ENET_EMMC_LINKSPD_100);
        pDrvCtrl->axiEndObj.mib2Tbl.ifSpeed = 100000000;
        }
    else
        {
        AXI_WRITE_4(pDrvCtrl, AXI_ENET_EMMC_OFFSET,
                    AXI_ENET_EMMC_LINKSPD_10);
        pDrvCtrl->axiEndObj.mib2Tbl.ifSpeed = 10000000;
        }

    if ((pDrvCtrl->axiEndObj.flags & IFF_UP) == 0)
        {
        (void)semGive (pDrvCtrl->axiDevSem);
        return (OK);
        }

    /* if status went from down to up, announce link up */

    if (pDrvCtrl->axiCurStatus & IFM_ACTIVE && !(oldStatus & IFM_ACTIVE))
        {
        if (pDrvCtrl->axiEndObj.pMib2Tbl != NULL )
            {
            pDrvCtrl->axiEndObj.pMib2Tbl->m2Data.mibIfTbl.ifSpeed =
            pDrvCtrl->axiEndObj.mib2Tbl.ifSpeed;
            }

        (void)jobQueueStdPost(pDrvCtrl->axiJobQueue,
                              NET_TASK_QJOB_PRI,
                              muxLinkUpNotify,
                              &pDrvCtrl->axiEndObj,
                              NULL, NULL, NULL, NULL );
        }

    /* if status went from up to down, announce link down */

    else if (!(pDrvCtrl->axiCurStatus & IFM_ACTIVE) && oldStatus & IFM_ACTIVE)
        {
        (void)jobQueueStdPost(pDrvCtrl->axiJobQueue,
                              NET_TASK_QJOB_PRI,
                              muxLinkDownNotify,
                              &pDrvCtrl->axiEndObj,
                              NULL, NULL, NULL, NULL );
        }

    (void)semGive (pDrvCtrl->axiDevSem);

    return (OK);
    }

/*******************************************************************************
 *
 * fmqlAxiMuxConnect - muxConnect method handler
 *
 * This function handles muxConnect() events, which may be triggered
 * manually or (more likely) by the bootstrap code. Most VxBus
 * initialization occurs before the MUX has been fully initialized,
 * so the usual muxDevLoad()/muxDevStart() sequence must be defered
 * until the networking subsystem is ready. This routine will ultimately
 * trigger a call to fmqlAxiEndLoad() to create the END interface instance.
 *
 * RETURNS: N/A
 *
 * ERRNO: N/A
 */

LOCAL void fmqlAxiMuxConnect
    (
    VXB_DEVICE_ID pDev,
    void     * unUsed
    )
    {
    AXI_DRV_CTRL * pDrvCtrl;
    const struct hcfDevice * pHcf;
    VXB_DEVICE_ID miiDev;
    char * miiIfName;
    INT32 miiIfUnit;

    if (pDev == NULL)
        {
        DBG_LOG_MSG("pDev is NULL, returning...\n",1,2,3,4,5,6);
        return;
        }

    pDrvCtrl = pDev->pDrvCtrl;

    /*
     * Initialize MII bus.
     * Note that we defer this until the muxConnect stage in this driver,
     * unlike the others.  This is because all of the PHYs are managed through
     * the EMAC interface, and we have to allow primary initialization of EMAC
     * to complete before we can start searching for PHYs.
     */

    pHcf = hcfDeviceGet (pDev);
    if (pHcf == NULL)
        return;
	
    /*
     * resourceDesc {
     * The phyAddr resource specifies the MII management address (0-31) of the
     * PHY for this particular EMAC device.  Each EMAC typically has at least
     * one PHY allocated to it. }
     */

    (void) devResourceGet (pHcf, "phyAddr", HCF_RES_INT,
            (void *) &pDrvCtrl->axiMiiPhyAddr);

    /*
     * resourceDesc {
     * The miiIfName resource specifies the name of the driver that provides
     * the MII interface for this EMAC unit.  On boards that have multiple EMAC
     * devices, the management pins for all of the PHYs will all be wired to
     * the MDIO pins on just one controller.  The <miiIfName> resource (and the
     * <miiIfUnit> resource below) are used to tell each EMAC instance which
     * one is the management controller.  If a device is not the management
     * controller, it will just forward its PHY register read and write
     * requests to the one that is. }
     */

    (void) devResourceGet (pHcf, "miiIfName", HCF_RES_STRING, (void *) &miiIfName);

    /*
     * resourceDesc {
     * The miiIfUnit resource specifies the unit number of the device that
     * provides the MII management methods for this EMAC instance. }
     */

    (void) devResourceGet (pHcf, "miiIfUnit", HCF_RES_INT, (void *) &miiIfUnit);

	/* jc*/
	int i = 0;
    pDrvCtrl->miiIfUnit = miiIfUnit;  /* for two gmac*/
	/*pDrvCtrl->miiIfUnit = miiIfUnit;*//*modify by hyf*/
	
    miiDev = vxbInstByNameFind (miiIfName, miiIfUnit);

    pDrvCtrl->axiMiiDev = miiDev;
    pDrvCtrl->axiMiiPhyRead = vxbDevMethodGet (miiDev,
                                                (UINT32) &miiRead_desc);
    pDrvCtrl->axiMiiPhyWrite = vxbDevMethodGet (miiDev,
                                                 (UINT32) &miiWrite_desc);

    /* create our MII bus */
    miiBusCreate (pDev, &pDrvCtrl->axiMiiBus);
    miiBusMediaListGet (pDrvCtrl->axiMiiBus, &pDrvCtrl->axiMediaList);
    miiBusModeSet (pDrvCtrl->axiMiiBus,
                   pDrvCtrl->axiMediaList->endMediaListDefault);
	

    /* save the cookie */
	//printf("fmqlAxiMuxConnect Load unit %d\n",pDev->unitNumber);
    pDrvCtrl->axiMuxDevCookie = muxDevLoad(pDev->unitNumber,
                                           fmqlAxiEndLoad,
                                           "", TRUE, pDev);

    if (pDrvCtrl->axiMuxDevCookie != NULL )
        {
        (void)muxDevStart(pDrvCtrl->axiMuxDevCookie);
        }

    if (_func_m2PollStatsIfPoll != NULL )
        {
        (void)endPollStatsInit(pDrvCtrl->axiMuxDevCookie,
                               _func_m2PollStatsIfPoll);
        }
	
    }

/*******************************************************************************
 *
 * fmqlAxiReset - reset the controller
 *
 * This function resets the AXI controller, and also stops any DMA
 * operations currently in progress,
 *
 * RETURNS: ERROR if the reset bit never clears, otherwise OK
 *
 * ERRNO: N/A
 */

LOCAL STATUS fmqlAxiReset 
    (
    VXB_DEVICE_ID pDev
    )
    {
    UINT32          axienetStatus, macData;
    UINT32          varTx, varRx;
    AXI_DRV_CTRL  * pDrvCtrl;
    UINT32          cr;
    int             i;
    M_BLK_ID        pMblk = NULL;
    AXI_DESC      * pDesc;
    STATUS          status = OK;

    if (pDev->pDrvCtrl == NULL)
        return (ERROR);
	
	pDrvCtrl = (EMAC_DRV_CTRL *)pDev->pDrvCtrl;
    /* 
    * Reset Axi DMA. This would reset Axi Ethernet core as well. The reset
    * process of Axi DMA takes a while to complete as all pending
    * commands/transfers will be flushed or completed during this
    * reset process.
    */

    AXIDMA_WRITE_4(pDrvCtrl, AXI_DMA_TX_CR_OFFSET, AXI_DMA_CR_RESET_MASK);
    AXIDMA_WRITE_4(pDrvCtrl, AXI_DMA_RX_CR_OFFSET, AXI_DMA_CR_RESET_MASK);

    (void)vxbMsDelay(1);

    varTx = AXIDMA_READ_4(pDrvCtrl, AXI_DMA_TX_CR_OFFSET);
    varRx = AXIDMA_READ_4(pDrvCtrl, AXI_DMA_RX_CR_OFFSET);

    if ((varTx & AXI_DMA_CR_RESET_MASK) && (varRx & AXI_DMA_CR_RESET_MASK))
        {
        FMQL_AXI_END_DBG_MSG (FMQL_AXI_END_DBG_INFO,"DMA reset timeout\n");
        }

    pDrvCtrl->maxFrameSize = pDrvCtrl->axiMaxMtu 
                             + AXI_ENET_HDR_VLAN_SIZE
                             + AXI_ENET_TRL_SIZE;

    if ((pDrvCtrl->axiMaxMtu > AXI_ENET_MTU)
        && (pDrvCtrl->axiMaxMtu <= AXI_ENET_JUMBO_MTU))
        {
        FMQL_AXI_END_DBG_MSG (FMQL_AXI_END_DBG_INFO,"axiMaxMtu=%d - set JUMBO frame\n",
                 pDrvCtrl->axiMaxMtu);

        axienetStatus = AXI_READ_4(pDrvCtrl, AXI_ENET_RCW1_OFFSET);
        axienetStatus |= (AXI_ENET_RCW1_VLAN_MASK | AXI_ENET_RCW1_JUM_MASK);
        AXI_WRITE_4(pDrvCtrl, AXI_ENET_RCW1_OFFSET, axienetStatus);

        axienetStatus = AXI_READ_4(pDrvCtrl, AXI_ENET_TC_OFFSET);
        axienetStatus |= (AXI_ENET_TC_VLAN_MASK | AXI_ENET_TC_JUM_MASK);
        AXI_WRITE_4(pDrvCtrl, AXI_ENET_TC_OFFSET, axienetStatus);
        }
    else
        {
        axienetStatus = AXI_READ_4(pDrvCtrl, AXI_ENET_RCW1_OFFSET);
        axienetStatus |= (AXI_ENET_RCW1_VLAN_MASK & (~AXI_ENET_RCW1_JUM_MASK));
        AXI_WRITE_4(pDrvCtrl, AXI_ENET_RCW1_OFFSET, axienetStatus);

        axienetStatus = AXI_READ_4(pDrvCtrl, AXI_ENET_TC_OFFSET);
        axienetStatus |= (AXI_ENET_TC_VLAN_MASK & (~AXI_ENET_TC_JUM_MASK));

        AXI_WRITE_4(pDrvCtrl, AXI_ENET_TC_OFFSET, axienetStatus);
        }

    /* Set up the RX ring. */

    for (i = 0; i < AXI_RX_DESC_CNT; i++)
        {
        pMblk = endPoolTupleGet(pDrvCtrl->axiEndObj.pNetPool);
        if (pMblk == NULL )
            {
            FMQL_AXI_END_DBG_MSG (FMQL_AXI_END_DBG_ERR,"endPoolTupleGet() return NULL\n");
            status = ERROR;
            break;
            }

        pMblk->m_next = NULL;
        
        (void)cacheInvalidate(DATA_CACHE, pMblk->m_data, pMblk->m_len);
        
        pDrvCtrl->axiRxMblk[i] = pMblk;
        pDesc = &pDrvCtrl->axiRxDescMem[i];
        pDesc->next = (UINT32) &pDrvCtrl->axiRxDescMem[((i + 1)
                                                    % AXI_RX_DESC_CNT)];
        pDesc->swIdOffset = (UINT32) 0;
        pDesc->cntrl = pDrvCtrl->maxFrameSize;
        pDesc->phys = (UINT32)(mtod(pMblk, UINT32));
        pDesc->status = 0;
        }

        /* Set up TX ring */

    for (i = 0; i < AXI_TX_DESC_CNT; i++)
        {
        pDesc = &pDrvCtrl->axiTxDescMem[i];
        pDesc->next = (UINT32) (&pDrvCtrl->axiTxDescMem[((i + 1)
                                                   % AXI_TX_DESC_CNT)]);
        pDesc->phys = 0;
        pDesc->app0 = 0;
        pDesc->app1 = 0;
        pDesc->app2 = 0;
        pDesc->app3 = 0;
        pDesc->app4 = 0;
        pDesc->status = 0;
        }

    /* Start updating the Rx channel control register */

    cr = AXIDMA_READ_4(pDrvCtrl, AXI_DMA_RX_CR_OFFSET);

    /* Update the interrupt coalesce count */

    cr = ((cr & ~AXI_DMA_CR_COALESCE_MASK)
        | ((pDrvCtrl->coalesceCountRx) << AXI_DMA_CR_COALESCE_SHIFT));

    /* Update the delay timer count */

    cr = ((cr & ~AXI_DMA_CR_DELAY_MASK)
        | ((UINT32) AXI_DMA_DFT_RX_WAITBOUND << AXI_DMA_CR_DELAY_SHIFT));

    /* Enable coalesce, delay timer and error interrupts */

    cr |= AXI_DMA_IRQ_ALL_MASK;

    /* Write to the Rx channel control register */

    AXIDMA_WRITE_4(pDrvCtrl, AXI_DMA_RX_CR_OFFSET, cr);

    /* Start updating the Tx channel control register */

    cr = AXIDMA_READ_4(pDrvCtrl, AXI_DMA_TX_CR_OFFSET);

    /* Update the interrupt coalesce count */

    cr = (((cr & ~AXI_DMA_CR_COALESCE_MASK))
        | ((pDrvCtrl->coalesceCountTx) << AXI_DMA_CR_COALESCE_SHIFT));

    /* Update the delay timer count */

    cr = (((cr & ~AXI_DMA_CR_DELAY_MASK))
        | ((UINT32) AXI_DMA_DFT_TX_WAITBOUND << AXI_DMA_CR_DELAY_SHIFT));

    /* Enable coalesce, delay timer and error interrupts */

    cr |= AXI_DMA_IRQ_ALL_MASK;

    /* Write to the Tx channel control register */

    AXIDMA_WRITE_4(pDrvCtrl, AXI_DMA_TX_CR_OFFSET, cr);

    /* 
    * Populate the tail pointer and bring the Rx Axi DMA engine out of
    * halted state. This will make the Rx side ready for reception.
    */

    AXIDMA_WRITE_4(pDrvCtrl, AXI_DMA_RX_CDESC_OFFSET,
                   (UINT32) pDrvCtrl->axiRxDescMem);
    cr = AXIDMA_READ_4(pDrvCtrl, AXI_DMA_RX_CR_OFFSET);
    AXIDMA_WRITE_4(pDrvCtrl, AXI_DMA_RX_CR_OFFSET,
                   cr | AXI_DMA_CR_RUNSTOP_MASK);
    AXIDMA_WRITE_4(pDrvCtrl, AXI_DMA_RX_TDESC_OFFSET,
                   (UINT32) 
                   &pDrvCtrl->axiRxDescMem[(AXI_RX_DESC_CNT - 1)]);

    /* 
     * Write to the RS (Run-stop) bit in the Tx channel control register.
     * Tx channel is now ready to run. But to start transmitting we must write
     * to thetail pointer register   
     */

    AXIDMA_WRITE_4(pDrvCtrl, AXI_DMA_TX_CDESC_OFFSET,
                   (UINT32) pDrvCtrl->axiTxDescMem);
    cr = AXIDMA_READ_4(pDrvCtrl, AXI_DMA_TX_CR_OFFSET);
    AXIDMA_WRITE_4(pDrvCtrl, AXI_DMA_TX_CR_OFFSET,
    cr | AXI_DMA_CR_RUNSTOP_MASK);

    if (status != OK)
        {
        FMQL_AXI_END_DBG_MSG (FMQL_AXI_END_DBG_ERR, "fmqlAxiReset descriptor allocation failed\n");
        }

    axienetStatus = AXI_READ_4(pDrvCtrl, AXI_ENET_RCW1_OFFSET);
    axienetStatus &= ~AXI_ENET_RCW1_RX_MASK;
    AXI_WRITE_4(pDrvCtrl, AXI_ENET_RCW1_OFFSET, axienetStatus);

    axienetStatus = AXI_READ_4(pDrvCtrl, AXI_ENET_IP_OFFSET);
    if (axienetStatus & AXI_ENET_INT_RXRJECT_MASK) 
        AXI_WRITE_4(pDrvCtrl, AXI_ENET_IS_OFFSET, AXI_ENET_INT_RXRJECT_MASK);

    AXI_WRITE_4(pDrvCtrl, AXI_ENET_FCC_OFFSET, AXI_ENET_FCC_FCRX_MASK);

    /* 
    * Sync default options with HW but leave receiver and
    * transmitter disabled.
    */

    axienetStatus = AXI_READ_4(pDrvCtrl, AXI_ENET_TC_OFFSET);
    axienetStatus &= ~AXI_ENET_TC_TX_MASK;
    AXI_WRITE_4(pDrvCtrl, AXI_ENET_TC_OFFSET, axienetStatus);

    AXI_WRITE_4(pDrvCtrl, AXI_ENET_UAW0_OFFSET,
                (pDrvCtrl->axiEnetMacAddr[0]) | (pDrvCtrl->axiEnetMacAddr[1] << 8)
                | (pDrvCtrl->axiEnetMacAddr[2] << 16) 
                | (pDrvCtrl->axiEnetMacAddr[3] << 24));

    macData = AXI_READ_4(pDrvCtrl, AXI_ENET_UAW1_OFFSET);
    AXI_WRITE_4(pDrvCtrl, AXI_ENET_UAW1_OFFSET,
                ((macData & ~AXI_ENET_UAW1_UNICASTADDR_MASK) 
                | (pDrvCtrl->axiEnetMacAddr[4] 
                | (pDrvCtrl->axiEnetMacAddr[5] << 8))));

    return (OK);
    }

/*******************************************************************************
 *
 * fmqlAxiEndLoad - END driver entry point
 *
 * This routine initializes the END interface instance associated
 * with this device. With VxBus, this function is invoked automatically
 * whenever this driver's muxConnect() method is called.
 *
 * For older END drivers, the load string would contain various
 * configuration parameters, but with VxBus this use is deprecated.
 * The load string should just be an empty string. The second
 * argument should be a pointer to the VxBus device instance
 * associated with this device. Like older END drivers, this routine
 * will still return the device name if the init string is empty,
 * since this behavior is still expected by the MUX. The MUX will
 * invoke this function twice: once to obtain the device name,
 * and then again to create the actual END_OBJ instance.
 *
 * When this function is called the second time, it will initialize
 * the END object, perform MIB2 setup, allocate a buffer pool, and
 * initialize the supported END capabilities. The only special
 * capability we support is VLAN_MTU, since we can receive slightly
 * larger than normal frames.
 *
 * RETURNS: An END object pointer, or NULL on error, or 0 and the name
 * of the device if the <loadStr> was empty.
 *
 * ERRNO: N/A
 */

LOCAL END_OBJ * fmqlAxiEndLoad
    (
    char * loadStr,
    void * pArg
    )
    {
    AXI_DRV_CTRL * pDrvCtrl;
    VXB_DEVICE_ID pDev;
    UINT32 macData;

    /* make the MUX happy */

    if (loadStr == NULL )
        {
        return NULL ;
        }

    if (loadStr[0] == 0)
        {
        bcopy(AXI_END_NAME, loadStr, sizeof(AXI_END_NAME));
        return NULL;
        }

    pDev = pArg;
    pDrvCtrl = pDev->pDrvCtrl;

    if (END_OBJ_INIT (&pDrvCtrl->axiEndObj, NULL, AXI_END_NAME,
                      pDev->unitNumber, &fmqlAxiNetFuncs, 
                      "AXI VxBus END Driver") == ERROR)
        {
        FMQL_AXI_END_DBG_MSG (FMQL_AXI_END_DBG_INFO, "%s%d: END_OBJ_INIT failed\n", 
                 AXI_END_NAME, pDev->unitNumber);
        
        return (NULL );
        }

    FMQL_AXI_END_DBG_MSG (FMQL_AXI_END_DBG_INFO, "MAC addr %02x:%02x:%02x:%02x:%02x:%02x\n",
                  pDrvCtrl->axiEnetMacAddr[0], pDrvCtrl->axiEnetMacAddr[1],
                  pDrvCtrl->axiEnetMacAddr[2], pDrvCtrl->axiEnetMacAddr[3],
                  pDrvCtrl->axiEnetMacAddr[4], pDrvCtrl->axiEnetMacAddr[5]);

    AXI_WRITE_4(pDrvCtrl, AXI_ENET_UAW0_OFFSET,
                (pDrvCtrl->axiEnetMacAddr[0]) | 
                (pDrvCtrl->axiEnetMacAddr[1] << 8) | 
                (pDrvCtrl->axiEnetMacAddr[2] << 16) | 
                (pDrvCtrl->axiEnetMacAddr[3] << 24));

    macData = AXI_READ_4(pDrvCtrl, AXI_ENET_UAW1_OFFSET);
    AXI_WRITE_4(pDrvCtrl, AXI_ENET_UAW1_OFFSET,
                ((macData & ~AXI_ENET_UAW1_UNICASTADDR_MASK) | 
                 (pDrvCtrl->axiEnetMacAddr[4] | 
                 (pDrvCtrl->axiEnetMacAddr[5] << 8))));

    if (endM2Init(&pDrvCtrl->axiEndObj, M2_ifType_ethernet_csmacd,
                  pDrvCtrl->axiEnetMacAddr, ETHER_ADDR_LEN, ETHERMTU,
                  100000000, IFF_NOTRAILERS | IFF_SIMPLEX | 
                  IFF_MULTICAST |IFF_BROADCAST) == ERROR)
        {
        FMQL_AXI_END_DBG_MSG (FMQL_AXI_END_DBG_ERR, "%s%d: endM2Init failed\n",
                 AXI_END_NAME, pDev->unitNumber);
        
        return (NULL);
        }

    /* allocate a buffer pool */

    pDrvCtrl->axiMaxMtu = AXI_MTU;

    if (endPoolCreate(AXI_TUPLE_CNT, &pDrvCtrl->axiEndObj.pNetPool) == ERROR)
        {
        FMQL_AXI_END_DBG_MSG (FMQL_AXI_END_DBG_ERR, "%s%d: pool creation failed\n",
                 AXI_END_NAME, pDev->unitNumber);
        
        return (NULL);
        }

    pDrvCtrl->axiPollBuf = endPoolTupleGet(pDrvCtrl->axiEndObj.pNetPool);

    /* set up polling stats */
	//pDrvCtrl->emacEndStatsConf.ifPollInterval = sysClkRateGet ();
    pDrvCtrl->axiEndStatsConf.ifEndObj = &pDrvCtrl->axiEndObj;
    pDrvCtrl->axiEndStatsConf.ifWatchdog = NULL;
    pDrvCtrl->axiEndStatsConf.ifValidCounters = (END_IFINUCASTPKTS_VALID
                  | END_IFINMULTICASTPKTS_VALID | END_IFINBROADCASTPKTS_VALID
                  | END_IFINOCTETS_VALID | END_IFINERRORS_VALID
                  | END_IFINDISCARDS_VALID | END_IFOUTUCASTPKTS_VALID
                  | END_IFOUTMULTICASTPKTS_VALID | END_IFOUTBROADCASTPKTS_VALID
                  | END_IFOUTOCTETS_VALID | END_IFOUTERRORS_VALID);

    /* set up capabilities */

    pDrvCtrl->axiCaps.cap_available = IFCAP_VLAN_MTU;
    pDrvCtrl->axiCaps.cap_enabled = IFCAP_VLAN_MTU;

    return (&pDrvCtrl->axiEndObj);
    }

/*******************************************************************************
 *
 * fmqlAxiEndUnload - unload END driver instance
 *
 * This routine undoes the effects of fmqlAxiEndLoad(). The END object
 * is destroyed, our network pool is released, the endM2 structures
 * are released, and the polling stats watchdog is terminated.
 *
 * Note that the END interface instance can't be unloaded if the
 * device is still running. The device must be stopped with muxDevStop()
 * first.
 *
 * RETURNS: ERROR if device is still in the IFF_UP state, otherwise OK
 *
 * RETURN: ERROR or EALREADY
 * ERRNO: N/A
 */

LOCAL STATUS fmqlAxiEndUnload 
    (
    END_OBJ * pEnd
    )
    {
    AXI_DRV_CTRL * pDrvCtrl;

    if (pEnd->flags & IFF_UP)
        {
        return (ERROR);
        }

    pDrvCtrl = (AXI_DRV_CTRL *)pEnd;

    netMblkClChainFree (pDrvCtrl->axiPollBuf);

    /* Relase our buffer pool */

    (void)endPoolDestroy (pDrvCtrl->axiEndObj.pNetPool);

    /* terminate stats polling */

    (void)wdDelete (pDrvCtrl->axiEndStatsConf.ifWatchdog);

    (void)endM2Free (&pDrvCtrl->axiEndObj);

    END_OBJECT_UNLOAD (&pDrvCtrl->axiEndObj);

    return (EALREADY);  /* prevent freeing of pDrvCtrl */
    }

/*******************************************************************************
 *
 * fmqlAxiEndHashTblPopulate - populate the multicast hash filter
 *
 * This function programs the fmql controller's multicast hash
 * filter to receive frames sent to the multicast groups specified
 * in the multicast address list attached to the END object. If
 * the interface is in IFF_ALLMULTI mode, the filter will be
 * programmed to receive all multicast packets by setting all the
 * bits in the hash table to one.
 *
 * RETURNS: N/A
 *
 * ERRNO: N/A
 */

LOCAL void fmqlAxiEndHashTblPopulate
    (
    AXI_DRV_CTRL * pDrvCtrl
    )
    {
    UINT32 ctrlVal;         /* Filter Mask Index Register */
    UINT32 afReg0;          /* Address Filter Register 0  */
    UINT32 afReg1;          /* Address Filter Register 1  */
    UINT32 filterEntryIdx;  /* Filter entry index         */
    ETHER_MULTI * mCastNode = NULL;
    END_OBJ     * pEnd = NULL;

    pEnd = &pDrvCtrl->axiEndObj;

    /*
     * According to Xilinx LogiCORE IP AXI Ethernet Product Specification
     * (v3.01a), Table 46: Filter Mask Index Register Bit Definitions
     * There are only four filter entries can be used for address filtering,
     * so if there are more than four multicast address, hardware could do
     * nothing except make hardware enter promiscuous mode to receive all
     * packets, it depend on up software stack layer to handle the multicast
     * address in this case.
     */

    if ((pEnd->nMulti > AXI_ENET_MULTICAST_CAM_TABLE_NUM) ||
        (pDrvCtrl->axiEndObj.flags & IFF_ALLMULTI))
        {
        /* Enter promiscuous mode to receive all packets */

        AXI_SETBIT_4(pDrvCtrl, AXI_ENET_FMI_OFFSET, AXI_ENET_FMI_PM_MASK);

        return;
        }

    /* Exit promiscuous mode regardless of previous mode */

    AXI_CLRBIT_4(pDrvCtrl, AXI_ENET_FMI_OFFSET, AXI_ENET_FMI_PM_MASK);

    /* Clean the previous filter entry */

    for (filterEntryIdx = 0;
         filterEntryIdx < AXI_ENET_MULTICAST_CAM_TABLE_NUM;
         filterEntryIdx++)
        {
        ctrlVal = AXI_READ_4(pDrvCtrl, AXI_ENET_FMI_OFFSET) & 0xFFFFFF00;
        ctrlVal |= filterEntryIdx;

        AXI_WRITE_4(pDrvCtrl, AXI_ENET_FMI_OFFSET, ctrlVal);
        AXI_WRITE_4(pDrvCtrl, AXI_ENET_AF0_OFFSET, 0);
        AXI_WRITE_4(pDrvCtrl, AXI_ENET_AF1_OFFSET, 0);
        }

    /* Now repopulate the multicast address info into hardware */

    filterEntryIdx = 0;
    for (mCastNode = (ETHER_MULTI *) lstFirst (&pDrvCtrl->axiEndObj.multiList);
         mCastNode != NULL;
         mCastNode  = (ETHER_MULTI *) lstNext (&mCastNode->node))
        {
        afReg0 = (((UINT32)mCastNode->addr[0]) << 0) |
                 (((UINT32)mCastNode->addr[1]) << 8) |
                 (((UINT32)mCastNode->addr[2]) << 16)|
                 (((UINT32)mCastNode->addr[3]) << 24);

        afReg1 = (((UINT32)mCastNode->addr[4]) << 0) |
                 (((UINT32)mCastNode->addr[5]) << 8);

        ctrlVal = AXI_READ_4(pDrvCtrl, AXI_ENET_FMI_OFFSET) & 0xFFFFFF00;
        ctrlVal |= filterEntryIdx;

        filterEntryIdx++;

        AXI_WRITE_4(pDrvCtrl, AXI_ENET_FMI_OFFSET, ctrlVal);
        AXI_WRITE_4(pDrvCtrl, AXI_ENET_AF0_OFFSET, afReg0);
        AXI_WRITE_4(pDrvCtrl, AXI_ENET_AF1_OFFSET, afReg1);
        }
    }

/*******************************************************************************
 *
 * fmqlAxiEndMCastAddrAdd - add a multicast address for the device
 *
 * This routine adds a multicast address to whatever the driver
 * is already listening for.  It then resets the address filter.
 *
 * RETURNS: OK, or ERROR when 1) the parameter is not a valid multicast address
 * and 2) there is no enough resource for etherMultiAdd()
 *
 * ERRNO: N/A
 */

LOCAL STATUS fmqlAxiEndMCastAddrAdd
    (
    END_OBJ * pEnd,
    char * pAddr
    )
    {
    int retVal;
    AXI_DRV_CTRL * pDrvCtrl;

    pDrvCtrl = (AXI_DRV_CTRL *)pEnd;

    (void)semTake (pDrvCtrl->axiDevSem, WAIT_FOREVER);

    retVal = etherMultiAdd(&pEnd->multiList, pAddr);

    if (retVal == ENETRESET)
        {
        pEnd->nMulti++;
        fmqlAxiEndHashTblPopulate((AXI_DRV_CTRL *) pEnd);
        }

    (void)semGive(pDrvCtrl->axiDevSem);

    if ((retVal == ENETRESET) || (retVal == OK))
        {
        return OK;
        }
    else
        {
        return ERROR;
        }
    }

/*******************************************************************************
 *
 * fmqlAxiEndMCastAddrDel - delete a multicast address for the device
 *
 * This routine removes a multicast address from whatever the driver
 * is listening for.  It then resets the address filter.
 *
 * RETURNS: OK, or ERROR when the multicast address does not exist.
 *
 * ERRNO: N/A
 */

LOCAL STATUS fmqlAxiEndMCastAddrDel
    (
    END_OBJ * pEnd,
    char * pAddr
    )
    {
    int retVal;
    AXI_DRV_CTRL * pDrvCtrl;

    pDrvCtrl = (AXI_DRV_CTRL *)pEnd;

    (void)semTake (pDrvCtrl->axiDevSem, WAIT_FOREVER);

    retVal = etherMultiDel(&pEnd->multiList, pAddr);

    if (retVal == ENETRESET)
        {
        pEnd->nMulti--;
        fmqlAxiEndHashTblPopulate((AXI_DRV_CTRL *) pEnd);
        }

    (void)semGive(pDrvCtrl->axiDevSem);

    if ((retVal == ENETRESET) || (retVal == OK))
        {
        return OK;
        }
    else
        {
        return ERROR;
        }
    }

/*******************************************************************************
 *
 * fmqlAxiEndMCastAddrGet - get the multicast address list for the device
 *
 * This routine gets the multicast list of whatever the driver
 * is already listening for.
 *
 * RETURNS: OK or ERROR.
 *
 * ERRNO: N/A
 */

LOCAL STATUS fmqlAxiEndMCastAddrGet
    (
    END_OBJ * pEnd,
    MULTI_TABLE * pTable
    )
    {
    STATUS rval;
    AXI_DRV_CTRL * pDrvCtrl;

    pDrvCtrl = (AXI_DRV_CTRL *)pEnd;

    (void)semTake (pDrvCtrl->axiDevSem, WAIT_FOREVER);

    rval = etherMultiGet(&pEnd->multiList, pTable);

    (void)semGive(pDrvCtrl->axiDevSem);

    return rval;
    }

/*******************************************************************************
 *
 * fmqlAxiEndStatsDump - return polled statistics counts
 *
 * This routine is automatically invoked periodically by the polled statistics
 * watchdog.  All stats are available from the MIB registers.
 *
 * RETURNS: always OK
 *
 * ERRNO: N/A
 */

LOCAL STATUS fmqlAxiEndStatsDump
    (
    AXI_DRV_CTRL * pDrvCtrl
    )
    {
    END_IFCOUNTERS * pEndStatsCounters;

    pEndStatsCounters = &pDrvCtrl->axiEndStatsCounters;

    pEndStatsCounters->ifInOctets = pDrvCtrl->axiInOctets;
    pDrvCtrl->axiInOctets = 0;

    pEndStatsCounters->ifInUcastPkts = pDrvCtrl->axiInUcasts;
    pDrvCtrl->axiInUcasts = 0;

    pEndStatsCounters->ifInMulticastPkts = pDrvCtrl->axiInMcasts;
    pDrvCtrl->axiInMcasts = 0;

    pEndStatsCounters->ifInBroadcastPkts = pDrvCtrl->axiInBcasts;
    pDrvCtrl->axiInBcasts = 0;

    pEndStatsCounters->ifInErrors = pDrvCtrl->axiInErrors;
    pDrvCtrl->axiInErrors = 0;

    pEndStatsCounters->ifInDiscards = pDrvCtrl->axiInDiscards;
    pDrvCtrl->axiInDiscards = 0;

    pEndStatsCounters->ifOutOctets = pDrvCtrl->axiOutOctets;
    pDrvCtrl->axiOutOctets = 0;

    pEndStatsCounters->ifOutUcastPkts = pDrvCtrl->axiOutUcasts;
    pDrvCtrl->axiOutUcasts = 0;

    pEndStatsCounters->ifOutMulticastPkts = pDrvCtrl->axiOutMcasts;
    pDrvCtrl->axiOutMcasts = 0;

    pEndStatsCounters->ifOutBroadcastPkts = pDrvCtrl->axiOutBcasts;
    pDrvCtrl->axiOutBcasts = 0;

    pEndStatsCounters->ifOutErrors = pDrvCtrl->axiOutErrors;
    pDrvCtrl->axiOutErrors = 0;

    return (OK);
    }

/*******************************************************************************
 *
 * fmqlAxiEndIoctl - the driver I/O control routine
 *
 * This function processes ioctl requests supplied via the muxIoctl()
 * routine. In addition to the normal boilerplate END ioctls, this
 * driver supports the IFMEDIA ioctls, END capabilities ioctls, and
 * polled stats ioctls.
 *
 * RETURNS: A command specific response, usually OK or ERROR.
 *
 * ERRNO: N/A
 */

LOCAL int fmqlAxiEndIoctl
    (
    END_OBJ * pEnd,
    int cmd,
    caddr_t data
    )
    {
    AXI_DRV_CTRL     * pDrvCtrl;
    END_MEDIALIST    * mediaList;
    END_CAPABILITIES * hwCaps;
    END_MEDIA        * pMedia;
    END_RCVJOBQ_INFO * qinfo;
    UINT32             nQs;
    INT32              value;
    INT32              error = OK;
    INT32              i;

    pDrvCtrl = (AXI_DRV_CTRL *) pEnd;

    if (cmd != EIOCPOLLSTART && cmd != EIOCPOLLSTOP)
        {
        (void)semTake (pDrvCtrl->axiDevSem, WAIT_FOREVER);
        }

    switch (cmd)
    {
    case EIOCSADDR:
        if (data == NULL )
            {
            error = EINVAL;
            }
        else
            {
            bcopy ((char *) data, (char *) pDrvCtrl->axiEnetMacAddr,
                   ETHER_ADDR_LEN);

            bcopy ((char *) data,
                   (char *) pEnd->mib2Tbl.ifPhysAddress.phyAddress,
                   ETHER_ADDR_LEN);
            if (pEnd->pMib2Tbl != NULL)
                bcopy ((char *) data,
                       (char *) pEnd->pMib2Tbl->m2Data.mibIfTbl.ifPhysAddress.phyAddress,
                       ETHER_ADDR_LEN);

            fmqlAxiEndRxConfig (pDrvCtrl);
            }

        break;

    case EIOCGADDR:
        if (data == NULL )
            error = EINVAL;
        else
            bcopy((char *) pDrvCtrl->axiEnetMacAddr, (char *) data,
            ETHER_ADDR_LEN);
        break;

    case EIOCSFLAGS:
        value = (INT32) data;
        if (value < 0)
            {
            value = -value;
            value--;
            END_FLAGS_CLR(pEnd, value);
            }
        else
            END_FLAGS_SET(pEnd, value);

        fmqlAxiEndRxConfig(pDrvCtrl);
        break;

    case EIOCGFLAGS:
        if (data == NULL )
            error = EINVAL;
        else
            *(INT32 *) data = END_FLAGS_GET(pEnd);
        break;

    case EIOCMULTIADD:
        error = fmqlAxiEndMCastAddrAdd(pEnd, (char *) data);
        break;

    case EIOCMULTIDEL:
        error = fmqlAxiEndMCastAddrDel(pEnd, (char *) data);
        break;

    case EIOCMULTIGET:
        error = fmqlAxiEndMCastAddrGet(pEnd, (MULTI_TABLE *) data);
        break;

    case EIOCPOLLSTART:
        (void)fmqlAxiEndPollStart(pDrvCtrl);
        break;

    case EIOCPOLLSTOP:
        (void)fmqlAxiEndPollStop(pDrvCtrl);
        break;

    case EIOCGMIB2233:
    case EIOCGMIB2:
        error = endM2Ioctl(&pDrvCtrl->axiEndObj, cmd, data);
        break;

    case EIOCGPOLLCONF:
        if (data == NULL )
            error = EINVAL;
        else
            {
            pDrvCtrl->axiEndStatsConf.ifPollInterval = sysClkRateGet();
            *((END_IFDRVCONF **) data) = &pDrvCtrl->axiEndStatsConf;
            }
        break;

    case EIOCGPOLLSTATS:
        if (data == NULL )
            error = EINVAL;
        else
            {
            error = fmqlAxiEndStatsDump(pDrvCtrl);
            if (error == OK) 
                *((END_IFCOUNTERS **) data) = &pDrvCtrl->axiEndStatsCounters;
            }
        break;

    case EIOCGMEDIALIST:
        if (data == NULL )
            {
            error = EINVAL;
            break;
            }
        if (pDrvCtrl->axiMediaList->endMediaListLen == 0)
            {
            error = ENOTSUP;
            break;
            }

        mediaList = (END_MEDIALIST *) data;
        if (mediaList->endMediaListLen < pDrvCtrl->axiMediaList->endMediaListLen)
            {
            mediaList->endMediaListLen =
            pDrvCtrl->axiMediaList->endMediaListLen;
            error = ENOSPC;
            break;
            }

        do
            {
            UINT32* pM = (UINT32*) pDrvCtrl->axiMediaList;
            for (i = 0; i < pDrvCtrl->axiMediaList->endMediaListLen; i++)
                {
                pM++;
                }
            } while (0);

        bcopy((char *) pDrvCtrl->axiMediaList, (char *) mediaList,
              sizeof(END_MEDIALIST) + 
              (sizeof(UINT32) * pDrvCtrl->axiMediaList->endMediaListLen));
        break;

    case EIOCGIFMEDIA:
        if (data == NULL )
            error = EINVAL;
        else
            {
            pMedia = (END_MEDIA *) data;
            pMedia->endMediaActive = pDrvCtrl->axiCurMedia;
            pMedia->endMediaStatus = pDrvCtrl->axiCurStatus;
            }
        break;

    case EIOCSIFMEDIA:
        if (data == NULL )
            error = EINVAL;
        else
            {
            pMedia = (END_MEDIA *) data;
            if (miiBusModeSet(pDrvCtrl->axiMiiBus,
                pMedia->endMediaActive) != OK)
                {
                error = ENOSYS;
                break;
                }
            (void)fmqlAxiLinkUpdate(pDrvCtrl->axiEthDev);
            error = OK;
            }
        break;

    case EIOCGIFCAP:
        hwCaps = (END_CAPABILITIES *) data;
        if (hwCaps == NULL )
            {
            error = EINVAL;
            break;
            }
        hwCaps->cap_available = pDrvCtrl->axiCaps.cap_available;
        hwCaps->cap_enabled = pDrvCtrl->axiCaps.cap_enabled;
        break;

    /*
     * The only special capability we support is VLAN_MTU, and
     * it can never be turned off.
     */

    case EIOCSIFCAP:
        error = ENOTSUP;
        break;

    case EIOCGIFMTU:
        if (data == NULL )
            error = EINVAL;
        else
            *(INT32 *) data = pEnd->mib2Tbl.ifMtu;
        break;

    case EIOCSIFMTU:
        value = (INT32) data;
        if (value <= 0 || value > pDrvCtrl->axiMaxMtu)
            {
            error = EINVAL;
            break;
            }
        pEnd->mib2Tbl.ifMtu = value;
        if (pEnd->pMib2Tbl != NULL )
            {
            pEnd->pMib2Tbl->m2Data.mibIfTbl.ifMtu = value;
            }
        break;

    case EIOCGRCVJOBQ :
        if (data == NULL )
            {
            error = EINVAL;
            break;
            }

        qinfo = (END_RCVJOBQ_INFO *) data;
        nQs = qinfo->numRcvJobQs;
        qinfo->numRcvJobQs = 1;
        if (nQs < 1)
            error = ENOSPC;
        else
            qinfo->qIds[0] = pDrvCtrl->axiJobQueue;

        break;

    default:
        error = EINVAL;
        break;
    }

    if (cmd != EIOCPOLLSTART && cmd != EIOCPOLLSTOP)
        {
        (void)semGive(pDrvCtrl->axiDevSem);
        }

    return (error);
    }

/*******************************************************************************
 *
 * fmqlAxiEndRxConfig - configure the FCC's RX filter
 *
 * This is a helper routine used by fmqlAxiEndIoctl() and fmqlAxiEndStart() to
 * configure the controller's RX filter. The unicast address filter is
 * programmed with the currently configured MAC address, and the RX
 * configuration is set to allow unicast and broadcast frames to be
 * received. If the interface is in IFF_PROMISC mode, the RX_PROMISC
 * bit is set, which allows all packets to be received.
 *
 * The fmqlAxiEndHashTblPopulate() routine is also called to update the
 * multicast filter.
 *
 * RETURNS: N/A
 *
 * ERRNO: N/A
 */

LOCAL void fmqlAxiEndRxConfig
    (
    AXI_DRV_CTRL * pDrvCtrl
    )
    {
    UINT32 macData, Reg;

    AXI_WRITE_4(pDrvCtrl, AXI_ENET_UAW0_OFFSET,
                (pDrvCtrl->axiEnetMacAddr[0]) | 
                (pDrvCtrl->axiEnetMacAddr[1] << 8) |
                (pDrvCtrl->axiEnetMacAddr[2] << 16) | 
                (pDrvCtrl->axiEnetMacAddr[3] << 24));

    macData = AXI_READ_4(pDrvCtrl, AXI_ENET_UAW1_OFFSET);
    
    AXI_WRITE_4(pDrvCtrl, AXI_ENET_UAW1_OFFSET,
                ((macData & ~AXI_ENET_UAW1_UNICASTADDR_MASK) | 
                (pDrvCtrl->axiEnetMacAddr[4] |
                (pDrvCtrl->axiEnetMacAddr[5] << 8))));

    Reg = AXI_READ_4(pDrvCtrl, AXI_ENET_TC_OFFSET);
    
    if (!(Reg & AXI_ENET_TC_TX_MASK))
        {
        AXI_WRITE_4(pDrvCtrl, AXI_ENET_TC_OFFSET, Reg | AXI_ENET_TC_TX_MASK);
        Reg = AXI_READ_4(pDrvCtrl, AXI_ENET_TC_OFFSET);
        }

    Reg = AXI_READ_4(pDrvCtrl, AXI_ENET_RCW1_OFFSET);
    if (!(Reg & AXI_ENET_RCW1_RX_MASK))
        {
        AXI_WRITE_4(pDrvCtrl, AXI_ENET_RCW1_OFFSET, Reg | AXI_ENET_RCW1_RX_MASK);
        Reg = AXI_READ_4(pDrvCtrl, AXI_ENET_TC_OFFSET);
        }

    /* Enable promisc mode, if specified */

    if (pDrvCtrl->axiEndObj.flags & IFF_PROMISC)
        {
        AXI_SETBIT_4(pDrvCtrl, AXI_ENET_FMI_OFFSET, AXI_ENET_FMI_PM_MASK);
        }
    else
        {
        AXI_CLRBIT_4(pDrvCtrl, AXI_ENET_FMI_OFFSET, AXI_ENET_FMI_PM_MASK);
        }

    /* Program the multicast filter */

    fmqlAxiEndHashTblPopulate (pDrvCtrl);
    }

/*******************************************************************************
 *
 * fmqlAxiMdioSetup - configure MDIO.
 *
 * This function resets the MDIO to put it into a known state.
 *
 * RETURNS: ERROR if device initialization failed, otherwise OK
 *
 * ERRNO: N/A
 */

LOCAL STATUS fmqlAxiMdioSetup
    (
    AXI_DRV_CTRL * pDrvCtrl
    )
    {
    STATUS ret = OK;
    UINT32 clk_div;
    UINT16 mdioRegVal;
    UINT32 data;
    UINT32 value;
    int    timeoutUs;

    /* Reset MDIO to a known state */

    clk_div = (pDrvCtrl->hostClock / (MAX_MDIO_FREQ * 2)) - 1;

    /* 
     * If there is any remainder from the division of
     * fHOST / (MAX_MDIO_FREQ * 2), then we need to add 1
     * to the clock divisor or we will surely be
     * above 2.5 MHz. 
     */

    if (pDrvCtrl->hostClock % (MAX_MDIO_FREQ * 2))
        {
        clk_div++;
        }

    FMQL_AXI_END_DBG_MSG (FMQL_AXI_END_DBG_INFO, "Setting MDIO clock divisor to %u based on %u Hz host clock.\n",
             clk_div, pDrvCtrl->hostClock);

    AXI_WRITE_4(pDrvCtrl, AXI_ENET_MDIO_MC_OFFSET,
                (((UINT32) clk_div) | AXI_ENET_MDIO_MC_MDIOEN_MASK));

    value = (((pDrvCtrl->axiMiiPhyAddr << AXI_ENET_MDIO_MCR_PHYAD_SHIFT)
            & AXI_ENET_MDIO_MCR_PHYAD_MASK)
            | ((0 << AXI_ENET_MDIO_MCR_REGAD_SHIFT) & AXI_ENET_MDIO_MCR_REGAD_MASK)
            | AXI_ENET_MDIO_MCR_INITIATE_MASK | AXI_ENET_MDIO_MCR_OP_READ_MASK);

    AXI_WRITE_4(pDrvCtrl, AXI_ENET_MDIO_MCR_OFFSET, value);

    /* set MDIO-MCR register to get PHY register value */

    timeoutUs = AXI_PHY_TIMEOUT;
    
    do
        {
        data = AXI_READ_4(pDrvCtrl, AXI_ENET_MDIO_MCR_OFFSET);
        if (timeoutUs == 0)
            {
            FMQL_AXI_END_DBG_MSG (FMQL_AXI_END_DBG_ERR, "AXI MDIO wait until ready TIMEOUT\n");
            ret = ERROR;
            }
        timeoutUs--;
        (void)vxbUsDelay(1);
        } while (!(data & AXI_ENET_MDIO_MCR_READY_MASK));

    data = AXI_READ_4(pDrvCtrl, AXI_ENET_MDIO_MRD_OFFSET);

    /* read MDIO-MRD register to get PHY register value */

    mdioRegVal = data & 0x0000FFFF; 

    value = (UINT32) (mdioRegVal | 0x8000);

    AXI_WRITE_4(pDrvCtrl, AXI_ENET_MDIO_MWD_OFFSET, value);

    value = (((pDrvCtrl->axiMiiPhyAddr << AXI_ENET_MDIO_MCR_PHYAD_SHIFT)
            & AXI_ENET_MDIO_MCR_PHYAD_MASK)
            | ((0 << AXI_ENET_MDIO_MCR_REGAD_SHIFT) & AXI_ENET_MDIO_MCR_REGAD_MASK)
            | AXI_ENET_MDIO_MCR_INITIATE_MASK | AXI_ENET_MDIO_MCR_OP_WRITE_MASK);

    AXI_WRITE_4(pDrvCtrl, AXI_ENET_MDIO_MCR_OFFSET, value);

    timeoutUs = AXI_PHY_TIMEOUT;
    
    do
        {
        data = AXI_READ_4(pDrvCtrl, AXI_ENET_MDIO_MCR_OFFSET);
        if (timeoutUs == 0)
            {
            ret = ERROR;
            }
        timeoutUs--;
        (void)vxbUsDelay(1);
        } while (!(data & AXI_ENET_MDIO_MCR_READY_MASK));

    return ret;
    }

LOCAL STATUS fmqlAxiEndMacAddrGet
    (
    AXI_DRV_CTRL * pDrvCtrl     /* pointer to end object */
    )
    {
    VXB_DEVICE_ID pDev;

    /* Include an extra byte for the flash reading routine */

    UINT8 readMacAddr[ETHER_ADDR_LEN + 1];
    INT32 chanNo = 0;

    if (pDrvCtrl == NULL)
        {
        return ERROR;
        }

    pDev = pDrvCtrl->axiEthDev;
    chanNo = pDrvCtrl->miiIfUnit;
	int i = 0;	
	/* jc for two eth-port*/
	/*chanNo = pDev->unitNumber;  /*modify by hyf*/

    if (sysNetMacAddrGet (pDev->pName, chanNo, readMacAddr, ETHER_ADDR_LEN)
        != OK)
        {
        if (sysNetMacNVRamAddrGet (pDev->pName, chanNo,
            readMacAddr, ETHER_ADDR_LEN) != OK)
            {
            DBG_LOG_MSG("ReadMacAddr %d is not OK, returning ERROR\n",
                        chanNo, 2,3,4,5,6);
            return (ERROR);
            }
        }

    /*
     * Copy the MAC Address from the little buffer to the driver location,
     * taking care to copy exactly the correct number of characters
     */

    bcopy ((const void *)readMacAddr, (void *)pDrvCtrl->axiEnetMacAddr,
           (size_t)ETHER_ADDR_LEN);

//    DBG_LOG_MSG2("MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
//            pDrvCtrl->axiEnetMacAddr[0], pDrvCtrl->axiEnetMacAddr[1],
//            pDrvCtrl->axiEnetMacAddr[2], pDrvCtrl->axiEnetMacAddr[3],
//            pDrvCtrl->axiEnetMacAddr[4], pDrvCtrl->axiEnetMacAddr[5]);

    return (OK);
    }


/*******************************************************************************
 *
 * fmqlAxiEndStart - start the device
 *
 * This function resets the device to put it into a known state and
 * then configures it for RX and TX operation. The RX and TX configuration
 * registers are initialized, and the address of the RX and TX DMA rings
 * are loaded into the device. Interrupts are then enabled, and the initial
 * link state is configured.
 *
 * Note that this routine also checks to see if an alternate jobQueue
 * has been specified via the vxbParam subsystem. This allows the driver
 * to divert its work to an alternate processing task, such as may be
 * done with TIPC. This means that the jobQueue can be changed while
 * the system is running, but the device must be stopped and restarted
 * for the change to take effect.
 *
 * RETURNS: ERROR if device initialization failed, otherwise OK
 *
 * ERRNO: N/A
 */

LOCAL STATUS fmqlAxiEndStart
    (
    END_OBJ * pEnd
    )
    {
    AXI_DRV_CTRL * pDrvCtrl;
    VXB_DEVICE_ID pDev;
    VXB_INST_PARAM_VALUE val;
    HEND_RX_QUEUE_PARAM * pRxQueue;
    M_BLK_ID pMblk = NULL;
    DMA_DESC * pDesc;
    INT32 i;
    INT32 ret;

    if (pEnd->flags & IFF_UP) return (OK);

    pDrvCtrl = (AXI_DRV_CTRL *) pEnd;
    pDev = pDrvCtrl->axiEthDev;

    (void)semTake (pDrvCtrl->axiDevSem, WAIT_FOREVER);
    END_TX_SEM_TAKE (pEnd, WAIT_FOREVER);

    /*
     * Before we start initialing the device, make sure we can get the MAC
     * address.
     */
    if (fmqlAxiEndMacAddrGet(pDrvCtrl) != OK)
        {
        DBG_LOG_MSG("Please set a valid MAC address for emac%d\n",
            pDrvCtrl->miiIfUnit,2,3,4,5,6);
        return (ERROR);
        }

    /* Initialize job queues */

    pDrvCtrl->axiJobQueue = netJobQueueId;

    /*
     * paramDesc {
     * The rxQueue00 parameter specifies a pointer to a HEND_RX_QUEUE_PARAM
     * structure, which contains, among other things, an ID for the job queue
     * to be used for this instance. }
     */
    if (vxbInstParamByNameGet (pDev, "rxQueue00", VXB_PARAM_POINTER, &val)
            == OK)
        {
        pRxQueue = (HEND_RX_QUEUE_PARAM *) val.pValue;
        if (pRxQueue->jobQueId != NULL)
            pDrvCtrl->axiJobQueue = pRxQueue->jobQueId;
        }

    QJOB_SET_PRI(&pDrvCtrl->axiTxJob, NET_TASK_QJOB_PRI);
    pDrvCtrl->axiTxJob.func = fmqlAxiEndTxHandle;
	
    QJOB_SET_PRI(&pDrvCtrl->axiRxJob, NET_TASK_QJOB_PRI);
    pDrvCtrl->axiRxJob.func = fmqlAxiEndRxHandle;

    (void)vxAtomicSet(&pDrvCtrl->axiRxPending, FALSE);
    (void)vxAtomicSet(&pDrvCtrl->axiTxPending, FALSE);

    ret = fmqlAxiReset(pDev);
    if (ret != OK)
        {
        FMQL_AXI_END_DBG_MSG (FMQL_AXI_END_DBG_ERR, "fmqlAxiReset() failed\n");
        (void)semGive (pDrvCtrl->axiDevSem);
        return ret;
        }
	
    /* New bitstream: reconfigure the MDIO after resetting the Eth ctrl */

    ret = fmqlAxiMdioSetup(pDrvCtrl);
    if (ret != OK)
        {
        FMQL_AXI_END_DBG_MSG (FMQL_AXI_END_DBG_ERR, "fmqlAxiMdioSetup() failed\n");
        (void)semGive (pDrvCtrl->axiDevSem);
        return ret;
        }


    /* Initialize state */

    pDrvCtrl->axiRxIdx 	= 0;
    pDrvCtrl->axiTxStall= FALSE;
    pDrvCtrl->axiTxProd = 0;
    pDrvCtrl->axiTxCons = 0;
    pDrvCtrl->axiTxFree = AXI_TX_DESC_CNT;

    /* Zero the stats counters. */

    pDrvCtrl->axiInUcasts  = 0;
    pDrvCtrl->axiInBcasts  = 0;
    pDrvCtrl->axiInMcasts  = 0;	
    pDrvCtrl->axiInOctets  = 0;
    pDrvCtrl->axiOutUcasts = 0;
    pDrvCtrl->axiOutBcasts = 0;
    pDrvCtrl->axiOutMcasts = 0;
    pDrvCtrl->axiOutOctets = 0;

    /* Rx Config */

    fmqlAxiEndRxConfig(pDrvCtrl);
    /* Enable interrupts */

    (void) vxbIntEnable (pDev, 0, fmqlAxiEndDmaRxInt, pDrvCtrl);
    (void) vxbIntEnable (pDev, 1, fmqlAxiEndDmaTxInt, pDrvCtrl);
    /* Set initial link state */

    pDrvCtrl->axiCurMedia  = IFM_ETHER | IFM_NONE;
    pDrvCtrl->axiCurStatus = IFM_AVALID;

	#if 0
    if (miiBusModeSet(pDrvCtrl->axiMiiBus, IFM_ETHER|IFM_AUTO) != OK)
        {
        (void)semGive (pDrvCtrl->axiDevSem);
        return ERROR;
        }
	#endif
    END_FLAGS_SET(pEnd, (IFF_UP | IFF_RUNNING));

	END_TX_SEM_GIVE (pEnd);
    (void)semGive (pDrvCtrl->axiDevSem);
	/* 
	for test --> poll mode
	*/
#ifdef PL_POLL_MODE
	fmqlAxiEndIoctl(pEnd, EIOCPOLLSTART, NULL);
#endif
	return OK;
    }

/*******************************************************************************
 *
 * fmqlAxiEndStop - stop the device
 *
 * This function undoes the effects of fmqlAxiEndStart(). The device is shut
 * down and all resources are released. Note that the shutdown process
 * pauses to wait for all pending RX, TX and link event jobs that may have
 * been initiated by the interrupt handler to complete. This is done
 * to prevent tNetTask from accessing any data that might be released by
 * this routine.
 *
 * RETURNS: ERROR if device shutdown failed, otherwise OK
 *
 * ERRNO: N/A
 */

LOCAL STATUS fmqlAxiEndStop
    (
    END_OBJ * pEnd
    )
    {
    AXI_DRV_CTRL * pDrvCtrl;
    VXB_DEVICE_ID  pDev;
    int            i;

    if (!(pEnd->flags & IFF_UP))
        return (OK);

    pDrvCtrl = (AXI_DRV_CTRL *)pEnd;
    pDev     = pDrvCtrl->axiEthDev;

    (void)semTake (pDrvCtrl->axiDevSem, WAIT_FOREVER);

    END_FLAGS_CLR (pEnd, (IFF_UP | IFF_RUNNING));

    (void) vxbIntDisable (pDev, 0, fmqlAxiEndDmaRxInt, pDrvCtrl);
    (void) vxbIntDisable (pDev, 1, fmqlAxiEndDmaTxInt, pDrvCtrl);

    /*
     * Wait for all jobs to drain.
     * Note: this must be done before we disable the receiver
     * and transmitter below. If someone tries to reboot us via
     * WDB, this routine may be invoked while the RX handler is
     * still running in tNetTask. If we disable the chip while
     * that function is running, it'll start reading inconsistent
     * status from the chip. We have to wait for that job to
     * terminate first, then we can disable the receiver and
     * transmitter.
     */

    for (i = 0; i < AXI_TIMEOUT; i++)
        {
        if (vxAtomicGet (&pDrvCtrl->axiRxPending) == FALSE &&
            vxAtomicGet (&pDrvCtrl->axiTxPending) == FALSE)
            break;
        (void)taskDelay (1);
        }

    if (i == AXI_TIMEOUT)
        {
        FMQL_AXI_END_DBG_MSG (FMQL_AXI_END_DBG_INFO, "%s%d: timed out waiting for job to complete\n",
                 AXI_END_NAME, pDrvCtrl->unit);
        }

    /*
     * Flush the recycle cache to shake loose any of our
     * mBlks that may be stored there.
     */

    endMcacheFlush ();

    (void)END_TX_SEM_TAKE (pEnd, WAIT_FOREVER);

    for (i = 0; i < AXI_TX_DESC_CNT; i++)
        {
        if (pDrvCtrl->axiTxMblk[i] != NULL)
            {
            netMblkClChainFree (pDrvCtrl->axiTxMblk[i]);
            pDrvCtrl->axiTxMblk[i] = NULL;
            }
        }

    for (i = 0; i <AXI_RX_DESC_CNT; i++)
        {
        if (pDrvCtrl->axiRxMblk[i] != NULL)
            {
            netMblkClChainFree (pDrvCtrl->axiRxMblk[i]);
            pDrvCtrl->axiRxMblk[i] = NULL;
            }
        }

    (void)END_TX_SEM_GIVE (pEnd);

    (void)semGive (pDrvCtrl->axiDevSem);

    return (OK);
    }

/*******************************************************************************
 *
 * fmqlAxiEndDmaRxInt - handle device interrupts
 *
 * This function is invoked whenever the interrupt line is asserted.
 *
 * RETURNS: N/A
 *
 * ERRNO: N/A
 */

LOCAL void fmqlAxiEndDmaRxInt
    (
    AXI_DRV_CTRL * pDrvCtrl
    )
    {
    UINT32 status, cr;

	FMQL_AXI_END_DBG_MSG (FMQL_AXI_END_DBG_INFO,"fmqlAxiEndDmaRxInt!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");

    status = AXIDMA_READ_4(pDrvCtrl, AXI_DMA_RX_SR_OFFSET);

    if (status & (AXI_DMA_IRQ_IOC_MASK | AXI_DMA_IRQ_DELAY_MASK))
        {
#ifdef FMQL_AXI_END_DEBUG
        if (pDrvCtrl->axiPolling == TRUE)
            {
            FMQL_AXI_END_DBG_MSG (FMQL_AXI_END_DBG_INFO,"rx interrupt was not disable in poll modes\n");
            }
#endif        
        AXIDMA_WRITE_4(pDrvCtrl, AXI_DMA_RX_SR_OFFSET, status);

        if (vxAtomic32Cas(&pDrvCtrl->axiRxPending, FALSE, TRUE))
            {
            /* disable RX interrupt */

            AXIDMA_CLRBIT_4(pDrvCtrl, AXI_DMA_RX_CR_OFFSET, 
                            (AXI_DMA_IRQ_IOC_MASK | AXI_DMA_IRQ_DELAY_MASK));
            
            (void)jobQueuePost(pDrvCtrl->axiJobQueue,&pDrvCtrl->axiRxJob);
            }
        
        return;
        }

    if (!(status & AXI_DMA_IRQ_ALL_MASK))
        {
        FMQL_AXI_END_DBG_MSG (FMQL_AXI_END_DBG_INFO, "No interrupts asserted in Rx path status=0x%08x\n",
                 status);
        }

    if (status & AXI_DMA_IRQ_ERROR_MASK)
        {
        FMQL_AXI_END_DBG_MSG (FMQL_AXI_END_DBG_INFO, "DMA Rx error 0x%x\n", status);

        cr = AXIDMA_READ_4(pDrvCtrl, AXI_DMA_TX_CR_OFFSET);

        /* Disable coalesce, delay timer and error interrupts */

        cr &= (~AXI_DMA_IRQ_ALL_MASK);

        /* Finally write to the Tx channel control register */

        AXIDMA_WRITE_4(pDrvCtrl, AXI_DMA_TX_CR_OFFSET, cr);

        cr = AXIDMA_READ_4(pDrvCtrl, AXI_DMA_RX_CR_OFFSET);

        /* Disable coalesce, delay timer and error interrupts */

        cr &= (~AXI_DMA_IRQ_ALL_MASK);

        /* write to the Rx channel control register */

        AXIDMA_WRITE_4(pDrvCtrl, AXI_DMA_RX_CR_OFFSET, cr);

        AXIDMA_WRITE_4(pDrvCtrl, AXI_DMA_RX_SR_OFFSET, status);
        }
    }

/*******************************************************************************
 *
 * fmqlAxiEndDmaTxInt - handle device interrupts
 *
 * This function is invoked whenever the interrupt line is asserted.
 *
 * RETURNS: N/A
 *
 * ERRNO: N/A
 */

LOCAL void fmqlAxiEndDmaTxInt
    (
    AXI_DRV_CTRL * pDrvCtrl
    )
    {
    UINT32 status, cr;

	FMQL_AXI_END_DBG_MSG (FMQL_AXI_END_DBG_INFO,"fmqlAxiEndDmaTxInt~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");

    status = AXIDMA_READ_4(pDrvCtrl, AXI_DMA_TX_SR_OFFSET);

    if (status & (AXI_DMA_IRQ_IOC_MASK | AXI_DMA_IRQ_DELAY_MASK))
        {
#ifdef FMQL_AXI_END_DEBUG
        if (pDrvCtrl->axiPolling == TRUE)
            {
            FMQL_AXI_END_DBG_MSG (FMQL_AXI_END_DBG_INFO,"tx interrupt was not disable in poll modes\n");
            }
#endif

        AXIDMA_WRITE_4(pDrvCtrl, AXI_DMA_TX_SR_OFFSET, status);

        if (vxAtomic32Cas(&pDrvCtrl->axiTxPending, FALSE, TRUE))
            {
            /* disable TX interrupt */

            AXIDMA_CLRBIT_4(pDrvCtrl, AXI_DMA_TX_CR_OFFSET,
                            (AXI_DMA_IRQ_IOC_MASK | AXI_DMA_IRQ_DELAY_MASK));

            (void)jobQueuePost(pDrvCtrl->axiJobQueue,&pDrvCtrl->axiTxJob);
            }

        return;
        }

    if (!(status & AXI_DMA_IRQ_ALL_MASK))
        {
        FMQL_AXI_END_DBG_MSG (FMQL_AXI_END_DBG_INFO, "No interrupts asserted in Tx path\n");
        }

    if (status & AXI_DMA_IRQ_ERROR_MASK)
        {
        FMQL_AXI_END_DBG_MSG (FMQL_AXI_END_DBG_INFO, "DMA Tx error 0x%x\n", status);

        cr = AXIDMA_READ_4(pDrvCtrl, AXI_DMA_TX_CR_OFFSET);

        /* Disable coalesce, delay timer and error interrupts */

        cr &= (~AXI_DMA_IRQ_ALL_MASK);

        /* Write to the Tx channel control register */

        AXIDMA_WRITE_4(pDrvCtrl, AXI_DMA_TX_CR_OFFSET, cr);

        cr = AXIDMA_READ_4(pDrvCtrl, AXI_DMA_RX_CR_OFFSET);

        /* Disable coalesce, delay timer and error interrupts */

        cr &= (~AXI_DMA_IRQ_ALL_MASK);

        /* Write to the Rx channel control register */

        AXIDMA_WRITE_4(pDrvCtrl, AXI_DMA_RX_CR_OFFSET, cr);

        AXIDMA_WRITE_4(pDrvCtrl, AXI_DMA_TX_SR_OFFSET, status);
        }
    }

/*******************************************************************************
 *
 * fmqlAxiEndRxHandle - process received frames
 *
 * This function is scheduled by the ISR to run in the context of tNetTask
 * whenever an RX interrupt is received. It processes packets from the
 * RX DMA ring and encapsulates them into mBlk tuples which are handed up
 * to the MUX.
 *
 * There may be several packets waiting in the ring to be processed.
 * We take care not to process too many packets in a single run through
 * this function so as not to monopolize tNetTask and starve out other
 * jobs waiting in the jobQueue. If we detect that there's still more
 * packets waiting to be processed, we queue ourselves up for another
 * round of processing.
 *
 * RETURNS: N/A
 *
 * ERRNO: N/A
 */

LOCAL void fmqlAxiEndRxHandle
    (
    void * pArg
    )
    {
    QJOB         * pJob;
    AXI_DRV_CTRL * pDrvCtrl;
    M_BLK_ID       pMblk = NULL, pNewMblk = NULL;
    volatile AXI_DESC * pDesc;
    volatile AXI_DESC * pDescLastUpdated = NULL;    
    int            rxLen;
    int            loopCounter = AXI_MAX_RX;

	FMQL_AXI_END_DBG_MSG (FMQL_AXI_END_DBG_INFO,"fmqlAxiEndRxHandle22222222222222222222222222222222222222\n");
    pJob = pArg;
    pDrvCtrl = member_to_object (pJob, AXI_DRV_CTRL, axiRxJob);

    pDesc = &pDrvCtrl->axiRxDescMem[pDrvCtrl->axiRxIdx];

    while (loopCounter && (pDesc->status & AXI_DMA_BD_STS_COMPLETE_MASK))
    {
    /* record last updated descriptor */

    pDescLastUpdated = pDesc;

#ifdef AXI_END_DROP_RX_MULTI_DESC
    if ((pDesc->status & AXI_DMA_BD_CTRL_ALL_MASK) != 
        (AXI_DMA_BD_CTRL_TXSOF_MASK | AXI_DMA_BD_CTRL_TXEOF_MASK))
        {

        /* 
         * Packet is fragmented over multiple descriptors. 
         * This happens when received frame is larger than MTU 
         * (hence larger than allocated Rx buffer size).
         * We can drop this larger that MTU packets.
         */

        pDrvCtrl->axiInDiscards++;

        pDesc->cntrl = pDrvCtrl->maxFrameSize;
        pDesc->status   = 0;

        AXI_INC_DESC(pDrvCtrl->axiRxIdx, AXI_RX_DESC_CNT);
        pDesc = &pDrvCtrl->axiRxDescMem[pDrvCtrl->axiRxIdx];
        continue;
        }
#endif  /* AXI_END_DROP_RX_MULTI_DESC */

    rxLen = pDesc->app4 & 0x0000FFFF;

    pNewMblk = endPoolTupleGet(pDrvCtrl->axiEndObj.pNetPool);

    if (pNewMblk == NULL )
        {
        FMQL_AXI_END_DBG_MSG (FMQL_AXI_END_DBG_ERR, "%s%d: out of mBlks at %d\n",
                 AXI_END_NAME, pDrvCtrl->unit, pDrvCtrl->axiRxIdx);

        pDrvCtrl->axiLastError.errCode = END_ERR_NO_BUF;
        muxError(&pDrvCtrl->axiEndObj, &pDrvCtrl->axiLastError);
        pDrvCtrl->axiInDiscards++;

        pDesc->status = 0;

        AXI_INC_DESC(pDrvCtrl->axiRxIdx, AXI_RX_DESC_CNT);
        pDesc = &pDrvCtrl->axiRxDescMem[pDrvCtrl->axiRxIdx];
        continue;
        }

    pMblk = pDrvCtrl->axiRxMblk[pDrvCtrl->axiRxIdx];

    /* there is a reason to cacheInvalidate the buffer before DMA push 
     * fresh data into it instead of after DMA push data into it.
     * consider a case like this:
     * 
     * - a fresh pNewMblk as been allocated and it will be assigned to an
     *   RX DMA descriptor so that the MAC can use it.
     * - prior to allocating this fresh buffer, it was previously used by 
     *   the TCP/IP stack, and the stack did some writes/stores to it.
     * - these writes/stores are now written to the cache, but have not 
     *   been flushed to RAM yet.
     * - assign this new fresh pNewMblk to RX DMA descriptor.
     * - the CPU is busy doing other things, and it only has a limited 
     *   about of cache.
     * - before the packet arrived into that fresh pNewMblk buffer, there
     *   is a long time for CPU.
     * - during this time, the CPU needs to cache other data, but it all 
     *   the cache lines are in use, the CPU decides that since the RX 
     *   buffer hasn't been touched in a long time, it can evict it from 
     *   the cache so that it can re-use its cache lines, since there are 
     *   writes pending, eviction means the CPU must flush the pending 
     *   writes to RAM also.
     * - before the CPU flush the data from cache into RAM, a new ethernet 
     *   frame arrives, and the MAC DMAs it to the RX buffer in RAM.
     * - then the CPU flushes the modified (and now stale) writes to the 
     *   RX buffer from the cache to RAM.
     *   So this will lead a wrong packet in RAM.
     *
     * The solution is: before DMA push data into buffer, flush the relative
     * cache into RAM first.
     * cacheInvalidate() is called to make two result, one is described as 
     * above-mentioned, the other is when CPU read the data from this 
     * buffer, load it from RAM into cache again.
     */

    (void)cacheInvalidate(DATA_CACHE, pNewMblk->m_data, pNewMblk->m_len);

    pDrvCtrl->axiRxMblk[pDrvCtrl->axiRxIdx] = pNewMblk;
    pNewMblk->m_next = NULL;

    pMblk->m_len = pMblk->m_pkthdr.len = rxLen;
    pMblk->m_flags = M_PKTHDR | M_EXT;

    pDesc->phys = (UINT32)(mtod(pNewMblk, UINT32));
    pDesc->cntrl = pDrvCtrl->maxFrameSize;
    pDesc->status = 0;

    /* Bump stats counters */

    pDrvCtrl->axiInOctets += pMblk->m_len;

    END_CACHE_INVAL_POST_DMA (pMblk->m_data, pMblk->m_len);
    
    /* Give the packet to the stack. */

    END_RCV_RTN_CALL(&pDrvCtrl->axiEndObj, pMblk);

    AXI_INC_DESC(pDrvCtrl->axiRxIdx, AXI_RX_DESC_CNT);
    pDesc = &pDrvCtrl->axiRxDescMem[pDrvCtrl->axiRxIdx];

    loopCounter--;
    }

    if (pDescLastUpdated)
        {
        /*
         * According to S2MM_TAILDESC Register Details from
         * "AXI DMA v7.1 LogiCORE IP Product Guide", the software
         * must not move the Tail Pointer to a location that has
         * not been updated. The software processes and reallocates
         * all completed descriptors (Cmplted = 1), clears the
         * completed bits and then moves the tail pointer.
         * The software must move the pointer to the last descriptor
         * it updated.
         */

        AXIDMA_WRITE_4(pDrvCtrl, AXI_DMA_RX_TDESC_OFFSET, 
                       (UINT32)((UINT32)pDescLastUpdated));
        }

    if (loopCounter == 0)
        {
        (void)jobQueuePost(pDrvCtrl->axiJobQueue, &pDrvCtrl->axiRxJob);
        return;
        }

    (void)vxAtomicSet(&pDrvCtrl->axiRxPending, FALSE);

    /* enable RX interrupt again */

    AXIDMA_SETBIT_4(pDrvCtrl, AXI_DMA_RX_CR_OFFSET, 
                    (AXI_DMA_IRQ_IOC_MASK | AXI_DMA_IRQ_DELAY_MASK));
    }

/*******************************************************************************
 *
 * fmqlAxiEndTxHandle - process TX completion events
 *
 * This function is scheduled by the ISR to run in the context of tNetTask
 * whenever an TX interrupt is received. It runs through all of the
 * TX register pairs and checks the TX status to see how many have
 * completed. For each completed transmission, the associated TX mBlk
 * is released, and the outbound packet stats are updated.
 *
 * In the event that a TX underrun error is detected, the TX FIFO
 * threshold is increased. This will continue until the maximum TX
 * FIFO threshold is reached.
 *
 * If the transmitter has stalled, this routine will also call muxTxRestart()
 * to drain any packets that may be waiting in the protocol send queues,
 *
 * RETURNS: N/A
 *
 * ERRNO: N/A
 */
 
LOCAL void fmqlAxiEndTxHandle
    (
    void * pArg
    )
    {
    QJOB         * pJob;
    AXI_DRV_CTRL * pDrvCtrl;
    BOOL           restart = FALSE;
    volatile AXI_DESC * pDesc;
    M_BLK_ID       pMblk;

	FMQL_AXI_END_DBG_MSG (FMQL_AXI_END_DBG_INFO,"fmqlAxiEndTxHandle111111111111111111111111111111111\n");
    pJob = pArg;
    pDrvCtrl = member_to_object (pJob, AXI_DRV_CTRL, axiTxJob);

    (void)END_TX_SEM_TAKE(&pDrvCtrl->axiEndObj, WAIT_FOREVER);

    while ((pDrvCtrl->axiTxFree < AXI_TX_DESC_CNT))
        {
        pDesc = &pDrvCtrl->axiTxDescMem[pDrvCtrl->axiTxCons];
        if (!(pDesc->status & AXI_DMA_BD_STS_COMPLETE_MASK)) 
            break;

#ifdef FMQL_AXI_END_DEBUG
        /* save data for debug / dump purpose */
        pDesc->app1     = pDesc->cntrl;
        pDesc->app2     = pDesc->app0;
        pDesc->app3     = pDesc->phys;
        pDesc->app4     = pDesc->status;
#endif
        pMblk = (M_BLK_ID) pDesc->app0;
        pDesc->cntrl    = 0; /* GCE - DBG */
        pDesc->phys     = 0;
        pDesc->status   = 0;
        pDesc->app0     = 0;

        if (pMblk != NULL )
            {
            pDrvCtrl->axiOutOctets += pMblk->m_pkthdr.len;
#ifdef AXI_TX_DRV_STAT
            if ((UINT8)pMblk->m_data[0] == 0xFF)
                pDrvCtrl->axiOutBcasts++;
            else if ((UINT8)pMblk->m_data[0] & 0x1)
                pDrvCtrl->axiOutMcasts++;
            else
                pDrvCtrl->axiOutUcasts++;
#endif

            endPoolTupleFree(pMblk);

#ifndef AXI_DBG_USE_APP0_TXMBLK
            pDrvCtrl->axiTxMblk[pDrvCtrl->axiTxCons] = NULL;
#endif
            }

        pDrvCtrl->axiTxFree++;

        AXI_INC_DESC(pDrvCtrl->axiTxCons, AXI_TX_DESC_CNT);

        /*
         * We released at least one descriptor: if the transmit
         * channel is stalled, unstall it.
         */

        if (pDrvCtrl->axiTxStall == TRUE)
            {
            pDrvCtrl->axiTxStall = FALSE;
            restart = TRUE;
            }
        }

    (void)END_TX_SEM_GIVE(&pDrvCtrl->axiEndObj);

    (void)vxAtomicSet(&pDrvCtrl->axiTxPending, FALSE);

    /* enable TX interrupt again */

    AXIDMA_SETBIT_4(pDrvCtrl, AXI_DMA_TX_CR_OFFSET, 
                    (AXI_DMA_IRQ_IOC_MASK | AXI_DMA_IRQ_DELAY_MASK));

    if (restart == TRUE)
        {
        muxTxRestart(pDrvCtrl);
        }
	
	FMQL_AXI_END_DBG_MSG (FMQL_AXI_END_DBG_INFO,"fmqlAxiEndTxHandle end...\n");
    }

/*******************************************************************************
 *
 * fmqlAxiEndSend - transmit a packet
 *
 * This function transmits the packet specified in <pMblk>.
 *
 * RETURNS: OK, ERROR, or END_ERR_BLOCK.
 *
 * ERRNO: N/A
 */

#ifdef FMQL_AXI_END_DEBUG
int g_test_snd_cnt = 0;
#endif

LOCAL int fmqlAxiEndSend
    (
    END_OBJ * pEnd,
    M_BLK_ID pMblk
    )
    {
    FAST AXI_DRV_CTRL * pDrvCtrl;
    M_BLK_ID       pTmp;
    FAST int rval;
	
#ifdef FMQL_AXI_END_DEBUG // add by jc
	g_test_snd_cnt++;
	FMQL_AXI_END_DBG_MSG (FMQL_AXI_END_DBG_INFO, "fmqlAxiEndSend: %d \n", g_test_snd_cnt);
#endif

    pDrvCtrl = (AXI_DRV_CTRL *) pEnd;

    if (pDrvCtrl->axiPolling == TRUE)
        {
        endPoolTupleFree(pMblk);
        return (ERROR);
        }

    (void)END_TX_SEM_TAKE(pEnd, WAIT_FOREVER);

    if (!pDrvCtrl->axiTxFree || !(pDrvCtrl->axiCurStatus & IFM_ACTIVE))
        goto blocked;

    /*
     * First, try to do an in-place transmission, using
     * gather-write DMA.
     */

    rval = fmqlAxiEndEncap(pDrvCtrl, pMblk);

    /*
     * If fmqlAxiEndEncap returns ENOSPC, it means it ran out of TX
     * descriptors and couldn't encapsulate the whole packet fragment
     * chain.  In that case, we need to coalesce everything into a
     * single buffer and try again. If any other error is returned,
     * then something went wrong, and we have to abort the transmission
     * entirely.
     */

    if (rval == ENOSPC)
        {
        if ((pTmp = endPoolTupleGet(pDrvCtrl->axiEndObj.pNetPool)) == NULL)
            goto blocked;

        pTmp->m_len = pTmp->m_pkthdr.len =
            netMblkToBufCopy(pMblk, mtod(pTmp, char *), NULL);
        pTmp->m_flags = pMblk->m_flags;

        /* try transmission again, should succeed this time */

        rval = fmqlAxiEndEncap (pDrvCtrl, pTmp);
        if (rval == OK)
            endPoolTupleFree (pMblk);
        else
            endPoolTupleFree (pTmp);
        }

    if (rval != OK)
        goto blocked;

    (void)END_TX_SEM_GIVE(pEnd);

    return (OK);

blocked: 
    pDrvCtrl->axiTxStall = TRUE;
    (void)END_TX_SEM_GIVE(pEnd);

    return (END_ERR_BLOCK);
    }

/*******************************************************************************
 *
 * fmqlAxiEndEncap - encapsulate an outbound packet in the TX DMA ring
 *
 * This function sets up a descriptor for a packet transmit operation.
 * With the fmql ethrnet controller, the TX DMA ring consists of
 * descriptors that each describe a single packet fragment. We consume
 * as many descriptors as there are mBlks in the outgoing packet, unless
 * the chain is too long. The length is limited by the number of DMA
 * segments we want to allow in a given DMA map. If there are too many
 * segments, this routine will fail, and the caller must coalesce the
 * data into fewer buffers and try again.
 *
 * This routine will also fail if there aren't enough free descriptors
 * available in the ring, in which case the caller must defer the
 * transmission until more descriptors are completed by the chip.
 *
 * RETURNS: ENOSPC if there are too many fragments in the packet, EAGAIN
 * if the DMA ring is full, otherwise OK.
 *
 * ERRNO: N/A
 */

LOCAL int fmqlAxiEndEncap
    (
    AXI_DRV_CTRL * pDrvCtrl,
    M_BLK_ID pMblk
    )
    {
    volatile AXI_DESC * pDesc = NULL;
    UINT32      txIdx;
    int         i = 0, nFrags = 0;
    int         lenTx;
    M_BLK_ID    pCurr;
    
    txIdx = pDrvCtrl->axiTxProd;

    for (pCurr = pMblk; pCurr != NULL; pCurr = pCurr->m_next)
        {
        nFrags ++;
        }

    if (nFrags > pDrvCtrl->axiTxFree)
        {
        return (ENOSPC);
        }

    for (pCurr = pMblk; pCurr != NULL; pCurr = pCurr->m_next)
        {
        lenTx = pCurr->m_len;
        pDesc = &pDrvCtrl->axiTxDescMem[txIdx];

        if (i==0)
            {
            /*
             * First descriptor - Save the mBlk for later and
             * mark DMA desc as first one.
             */

            pDesc->app0     = (UINT32) pMblk;
            pDesc->cntrl    = lenTx | AXI_DMA_BD_CTRL_TXSOF_MASK;
            }
        else
            {
            pDesc->app0     = 0;
            pDesc->cntrl    = lenTx;
            }

        pDesc->phys = (UINT32) (mtod(pCurr, UINT32));

        pDrvCtrl->axiTxFree--;

        /* Sync the buffers. */

        (void)cacheFlush (DATA_CACHE, pCurr->m_data, pCurr->m_len);

        AXI_INC_DESC(pDrvCtrl->axiTxProd, AXI_TX_DESC_CNT);
        txIdx = pDrvCtrl->axiTxProd;
        i++;
        }

    pDesc->cntrl |= AXI_DMA_BD_CTRL_TXEOF_MASK;

    /* Start the transfer */

    AXIDMA_WRITE_4(pDrvCtrl, AXI_DMA_TX_TDESC_OFFSET,
                   (UINT32) ((UINT32) pDesc));

    return (OK);
    }

/*******************************************************************************
 *
 * fmqlAxiEndPollSend - polled mode transmit routine
 *
 * This function is similar to the fmqlAxiEndSend() routine shown above, except
 * it performs transmissions synchronously with interrupts disabled. After
 * the transmission is initiated, the routine will poll the state of the
 * TX status register associated with the current slot until transmission
 * completed. If transmission times out, this routine will return ERROR.
 *
 * RETURNS: OK, EAGAIN, or ERROR
 *
 * ERRNO: N/A
 */

LOCAL STATUS fmqlAxiEndPollSend
    (
    END_OBJ * pEnd,
    M_BLK_ID  pMblk
    )
    {
    AXI_DRV_CTRL      * pDrvCtrl;
    volatile AXI_DESC * pDesc;
    M_BLK_ID            pTmp;
    int                 len, i;

#ifdef FMQL_AXI_END_DEBUG
	g_test_snd_cnt++;
	FMQL_AXI_END_DBG_MSG (FMQL_AXI_END_DBG_INFO,"fmqlAxiEndPollSend: %d \n", g_test_snd_cnt);
#endif

    pDrvCtrl = (AXI_DRV_CTRL *) pEnd;

    if (pDrvCtrl->axiPolling == FALSE)
        {
        FMQL_AXI_END_DBG_MSG (FMQL_AXI_END_DBG_ERR, "poll tx : axiPolling is false\n");
        return (ERROR);
        }
    
    (void)END_TX_SEM_TAKE(pEnd, WAIT_FOREVER);

    pTmp = pDrvCtrl->axiPollBuf;

    len = netMblkToBufCopy(pMblk, mtod(pTmp, char *), NULL );
    pTmp->m_len 	   = len; 
    pTmp->m_pkthdr.len = len;
    pTmp->m_flags 	   = pMblk->m_flags;

    if (fmqlAxiEndEncap(pDrvCtrl, pTmp) != OK)
        {
        FMQL_AXI_END_DBG_MSG (FMQL_AXI_END_DBG_ERR, "poll tx : fmqlAxiEndEncap() failed\n");
        
        (void)END_TX_SEM_GIVE(pEnd);
        return (EAGAIN);
        }

    pDesc = &pDrvCtrl->axiTxDescMem[pDrvCtrl->axiTxCons];
	
#ifdef FMQL_AXI_END_DEBUG
	FMQL_AXI_END_DBG_MSG (FMQL_AXI_END_DBG_INFO, "pDrvCtrl->axiTxCons = %d, pDesc->status = %x\n", pDrvCtrl->axiTxCons, pDesc->status);
	FMQL_AXI_END_DBG_MSG (FMQL_AXI_END_DBG_INFO, "pDesc->next = 0x%.8x\n", pDesc->next);
	FMQL_AXI_END_DBG_MSG (FMQL_AXI_END_DBG_INFO, "pDesc->phys = 0x%.8x\n", pDesc->phys);
	FMQL_AXI_END_DBG_MSG (FMQL_AXI_END_DBG_INFO, "pDesc->cntrl = 0x%.8x\n", pDesc->cntrl);
	FMQL_AXI_END_DBG_MSG (FMQL_AXI_END_DBG_INFO, "pDesc->status = 0x%.8x\n", pDesc->status);
	FMQL_AXI_END_DBG_MSG (FMQL_AXI_END_DBG_INFO, "pDesc->app0 = 0x%.8x\n", pDesc->app0);
	FMQL_AXI_END_DBG_MSG (FMQL_AXI_END_DBG_INFO, "pDesc->app1 = 0x%.8x\n", pDesc->app1);
	FMQL_AXI_END_DBG_MSG (FMQL_AXI_END_DBG_INFO, "pDesc->app2 = 0x%.8x\n", pDesc->app2);
	FMQL_AXI_END_DBG_MSG (FMQL_AXI_END_DBG_INFO, "pDesc->app3 = 0x%.8x\n", pDesc->app3);
	FMQL_AXI_END_DBG_MSG (FMQL_AXI_END_DBG_INFO, "pDesc->app4 = 0x%.8x\n", pDesc->app4);
	FMQL_AXI_END_DBG_MSG (FMQL_AXI_END_DBG_INFO, "pDesc->swIdOffset = 0x%.8x\n", pDesc->swIdOffset);

	FMQL_AXI_END_DBG_MSG (FMQL_AXI_END_DBG_INFO, "DMA Tx SR = 0x%.8x\n", AXIDMA_READ_4(pDrvCtrl,AXI_DMA_TX_SR_OFFSET));
#endif

	/*taskDelay(1);
	for(i = 0; i < 64; i++)
	{
		printf("%.2x ", ((char *)pMblk)[i]);
		if(i% 16 == 15)printf("\n");
	}
	printf("\n");
	taskDelay(1);*/

    for (i = 0; i < AXI_TIMEOUT; i++)
        {
        if (pDesc->status & AXI_DMA_BD_STS_COMPLETE_MASK)
            break;
        }

    if (i == AXI_TIMEOUT)
        {
        pDrvCtrl->axiOutErrors++;
        FMQL_AXI_END_DBG_MSG (FMQL_AXI_END_DBG_ERR, "poll tx : tx dma failed, AXI_TIMEOUT = %d\n",AXI_TIMEOUT);
        
        (void)END_TX_SEM_GIVE(pEnd);
        return (ERROR);
        }

    pDrvCtrl->axiTxMblk[pDrvCtrl->axiTxCons] = NULL;

    pDesc->cntrl    = 0;
    pDesc->phys     = 0;
    pDesc->status   = 0;
    pDesc->app0     = 0;

    pDrvCtrl->axiTxFree++;
    AXI_INC_DESC(pDrvCtrl->axiTxCons, AXI_TX_DESC_CNT);

    pDrvCtrl->axiOutOctets += pMblk->m_pkthdr.len;

    if ((UINT8) pMblk->m_data[0] == 0xFF)
        pDrvCtrl->axiOutBcasts++;
    else if ((UINT8) pMblk->m_data[0] & 0x1)
        pDrvCtrl->axiOutMcasts++;
    else
        pDrvCtrl->axiOutUcasts++;

    (void)END_TX_SEM_GIVE(pEnd);
    return (OK);
    }

/*******************************************************************************
 *
 * fmqlAxiEndPollReceive - polled mode receive routine
 *
 * This function receive a packet in polled mode, with interrupts disabled.
 * It's similar in operation to the fmqlAxiEndRxHandle() routine, except it
 * doesn't process more than one packet at a time and does not load out
 * buffers. Instead, the caller supplied an mBlk tuple into which this
 * function will place the received packet.
 *
 * If no packet is available, this routine will return EAGAIN. If the
 * supplied mBlk is too small to contain the received frame, the routine
 * will return ERROR.
 *
 * RETURNS: OK, EAGAIN, or ERROR
 *
 * ERRNO: N/A
 */
 
#ifdef FMQL_AXI_END_DEBUG
int g_test_rcv_cnt = 0;
#endif

LOCAL int fmqlAxiEndPollReceive
    (
    END_OBJ * pEnd,
    M_BLK_ID  pMblk
    )
    {
    AXI_DRV_CTRL      * pDrvCtrl;
    volatile AXI_DESC * pDesc;
    M_BLK_ID            pPkt = NULL;
    int                 rxLen;

#ifdef FMQL_AXI_END_DEBUG  // add by jc 
	g_test_rcv_cnt++;
	FMQL_AXI_END_DBG_MSG (FMQL_AXI_END_DBG_INFO,"fmqlAxiEndPollReceive: %d \n", g_test_rcv_cnt);
#endif

    pDrvCtrl = (AXI_DRV_CTRL *) pEnd;

    if (pDrvCtrl->axiPolling == FALSE)
        return (ERROR);

    if ((pMblk->m_flags & M_EXT) == 0)
        {
        FMQL_AXI_END_DBG_MSG (FMQL_AXI_END_DBG_ERR, "poll rx : no M_EXT flag\n");
        return (ERROR);
        }  

    if (!(pDrvCtrl->axiRxDescMem[pDrvCtrl->axiRxIdx].status & 
        AXI_DMA_BD_STS_COMPLETE_MASK))
        {
        return (EAGAIN);
        }

    pDesc = &pDrvCtrl->axiRxDescMem[pDrvCtrl->axiRxIdx];
    rxLen = pDesc->app4 & 0x0000FFFF;
    pPkt  = pDrvCtrl->axiRxMblk[pDrvCtrl->axiRxIdx];

    /* Bump stats counters */

    pDrvCtrl->axiInOctets += rxLen;

    pMblk->m_flags     |= M_PKTHDR;
    pMblk->m_len        = rxLen;
    pMblk->m_pkthdr.len = rxLen;

    (void)cacheInvalidate (DATA_CACHE, pPkt->m_data, rxLen);

    /* copy data */

    bcopy (mtod(pPkt, char *), mtod(pMblk, char *), (size_t)rxLen);

    pDesc->status = 0;

    /* Advance to the next descriptor. */

    AXIDMA_WRITE_4(pDrvCtrl, AXI_DMA_RX_TDESC_OFFSET, (UINT32)pDesc);
    AXI_INC_DESC(pDrvCtrl->axiRxIdx, AXI_RX_DESC_CNT);

    return (OK);
    }

/*******************************************************************************
 *
 * fmqlAxiEndPollStart - start polled mode operations
 *
 * RETURNS: OK or ERROR.
 */

LOCAL STATUS fmqlAxiEndPollStart
    (
    AXI_DRV_CTRL * pDrvCtrl
    )
    {
	VXB_DEVICE_ID pDev = pDrvCtrl->axiEthDev;
    END_OBJ  * pEnd = (END_OBJ *)&pDrvCtrl->axiEndObj;
    
    (void)END_TX_SEM_TAKE(pEnd, WAIT_FOREVER);
    
    pDrvCtrl->axiPolling = TRUE;

    /* disable interrupt */
	AXIDMA_CLRBIT_4(pDrvCtrl, AXI_DMA_TX_CR_OFFSET, (AXI_DMA_IRQ_ALL_MASK));

    (void) vxbIntDisable (pDev, 0, fmqlAxiEndDmaRxInt, pDrvCtrl);
    (void) vxbIntDisable (pDev, 1, fmqlAxiEndDmaTxInt, pDrvCtrl);

    while (pDrvCtrl->axiTxFree < AXI_TX_DESC_CNT)
        {
        volatile AXI_DESC * pDesc;
        M_BLK_ID pMblk;

        pDesc = &pDrvCtrl->axiTxDescMem[pDrvCtrl->axiTxCons];

        /* Wait for ownership bit to clear */

        while (!(pDesc->status & AXI_DMA_BD_STS_COMPLETE_MASK))
            {
            ;
            }

        pDesc->cntrl    = 0;
        pDesc->phys     = 0;
        pDesc->status   = 0;
        pDesc->app0     = 0;

        pMblk = pDrvCtrl->axiTxMblk[pDrvCtrl->axiTxCons];

        if (pMblk != NULL)
            {
            endPoolTupleFree (pMblk);
            pDrvCtrl->axiTxMblk[pDrvCtrl->axiTxCons] = NULL;
            }

        pDrvCtrl->axiTxFree++;
        AXI_INC_DESC (pDrvCtrl->axiTxCons, AXI_TX_DESC_CNT);
        }

	FMQL_AXI_END_DBG_MSG (FMQL_AXI_END_DBG_INFO,"fmqlAxiEndPollStart: Use poll mode for test.\n");

    (void)END_TX_SEM_GIVE(pEnd);
    return (OK);
    }

/*******************************************************************************
 *
 * fmqlAxiEndPollStop - stop polled mode operations
 *
 * This function terminates polled mode operation.  The device returns to
 * interrupt mode.
 *
 * The device interrupts are enabled, the current mode flag is switched
 * to indicate interrupt mode and the device is then reconfigured for
 * interrupt operation.
 *
 * RETURNS: OK or ERROR.
 */

LOCAL STATUS fmqlAxiEndPollStop
    (
    AXI_DRV_CTRL * pDrvCtrl
    )
    {
	VXB_DEVICE_ID pDev = pDrvCtrl->axiEthDev;
    
    pDrvCtrl->axiPolling = FALSE;

    /* enable interrupt */

    (void) vxbIntEnable (pDev, 0, fmqlAxiEndDmaRxInt, pDrvCtrl);
    (void) vxbIntEnable (pDev, 1, fmqlAxiEndDmaTxInt, pDrvCtrl);
    
    return (OK);
    }


#if 1

void ifconfig_pl(void)
{
	ipAttach(0, "axienet");
	ifconfig("axienet0 192.168.1.188");
	ifconfig("axienet0 up");

	taskDelay(10);
	ifconfig();		
	
	return;
}

void ifconfig2_pl(char* ip_addr)
{
	char ip_string[64] = {0};
	
	if (ip_addr == NULL)
	{
		ifconfig("axienet0 192.168.1.188");
	}
	else
	{
		sprintf(ip_string, "axienet0  %s", ip_addr);
		ifconfig((char*)(&ip_string[0]));	
	}
	
	ifconfig();

	return;
}

UINT32 pl_mac_read(UINT32 offset)
{
	VXB_DEVICE_ID pDev;
    AXI_DRV_CTRL * pDrvCtrl;
	
	UINT32 addr = 0;
	UINT32 val = 0;
	
    pDev = vxbInstByNameFind("axienet", 0); 
	if (pDev == NULL)
	{
		logMsg("vxbInstByNameFind(\"axienet\") fail! \n", 1,2,3,4,5,6);
		return 0;
	}
	
    pDrvCtrl = pDev->pDrvCtrl;
	
    //void              * axiBar;
    //void              * axiDmaBar;	
	addr = offset + pDrvCtrl->axiBar;
	val = *((UINT32*)addr);
	
	printf("pl_mac_reg(0x%X) = 0x%08X \n", offset, val);
	
	return val;
}

UINT32 pl_mac_read2(UINT32 offset1, UINT32 offset2)
{
	VXB_DEVICE_ID pDev;
    AXI_DRV_CTRL * pDrvCtrl;
	
	UINT32 addr = 0;
	UINT32 val = 0;
	UINT32 offset = 0;
	
    pDev = vxbInstByNameFind("axienet", 0); 
	if (pDev == NULL)
	{
		logMsg("vxbInstByNameFind(\"axienet\") fail! \n", 1,2,3,4,5,6);
		return 0;
	}
	
    pDrvCtrl = pDev->pDrvCtrl;
	
    //void              * axiBar;
    //void              * axiDmaBar;
    printf("\n----mac reg: 0x%X ~ 0x%X ---- \n", offset1, offset2);
	
    for (offset=offset1; offset<=offset2; offset+=4)
	{
		addr = offset + pDrvCtrl->axiBar;
		val = *((UINT32*)addr);
		
		printf("reg(0x%X) = 0x%08X \n", offset, val);
	}	
    printf("----mac reg: 0x%X ~ 0x%X ---- \n\n", offset1, offset2);
	
	return val;
}

UINT32 pl_dma_read(UINT32 offset)
{
	VXB_DEVICE_ID pDev;
    AXI_DRV_CTRL * pDrvCtrl;
	
	UINT32 addr = 0;
	UINT32 val = 0;
	
    pDev = vxbInstByNameFind("axienet", 0); 
	if (pDev == NULL)
	{
		logMsg("vxbInstByNameFind(\"axienet\") fail! \n", 1,2,3,4,5,6);
		return 0;
	}
	
    pDrvCtrl = pDev->pDrvCtrl;
	
    //void              * axiBar;
    //void              * axiDmaBar;	
	addr = offset + pDrvCtrl->axiDmaBar;
	val = *((UINT32*)addr);
	
	printf("pl_dma_reg(0x%X) = 0x%08X \n", offset, val);
	
	return val;
}

UINT32 pl_dma_read2(UINT32 offset1, UINT32 offset2)
{
	VXB_DEVICE_ID pDev;
    AXI_DRV_CTRL * pDrvCtrl;
	
	UINT32 addr = 0;
	UINT32 val = 0;
	UINT32 offset = 0;
	
    pDev = vxbInstByNameFind("axienet", 0); 
	if (pDev == NULL)
	{
		logMsg("vxbInstByNameFind(\"axienet\") fail! \n", 1,2,3,4,5,6);
		return 0;
	}
	
    pDrvCtrl = pDev->pDrvCtrl;
	
    //void              * axiBar;
    //void              * axiDmaBar;	
	
    printf("\n====dma reg: 0x%X ~ 0x%X ==== \n", offset1, offset2);
	
    for (offset=offset1; offset<=offset2; offset+=4)
	{
		addr = offset + pDrvCtrl->axiDmaBar;
		val = *((UINT32*)addr);
		
		printf("reg(0x%X) = 0x%08X \n", offset, val);
	}	
    printf("====dma reg: 0x%X ~ 0x%X ==== \n\n", offset1, offset2);
	
	return val;
}


#if 0   // hp open & enable

UINT32 hpBase = 0;

LOCAL void pl_hp_init(void)
{
	UINT32 virtAddr;
	int hp_chn = 0;
	
	// reset hp
	slcr_write(0x344, 0x0F);
	slcr_write(0x344, 0x00);
	
	// reopen lvl_shift
	slcr_write(0x838, 0x0F);

	// ddr hp clk enable
	slcr_write(0x218, 0x07);

	//
	// reopen hp channel
	//
	virtAddr = pmapGlobalMap (0xE0029000, 0x1000,
							  (MMU_ATTR_SUP_RW | MMU_ATTR_CACHE_OFF | MMU_ATTR_CACHE_GUARDED));
	if (virtAddr == PMAP_FAILED)
	{
		printf ("HP channel init fail! \n");
		return;
	}	
	hpBase = (UINT32)virtAddr;
	hp_chn = 0;
	*((UINT32 *)((hpBase + 0x490) + 0xB0 * hp_chn)) = 0x01;
	hp_chn = 1;
	*((UINT32 *)((hpBase + 0x490) + 0xB0 * hp_chn)) = 0x01;
	hp_chn = 2;
	*((UINT32 *)((hpBase + 0x490) + 0xB0 * hp_chn)) = 0x01;
	hp_chn = 3;
	*((UINT32 *)((hpBase + 0x490) + 0xB0 * hp_chn)) = 0x01;

	return;
}
#endif

#endif

