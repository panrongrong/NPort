/* vxbAltSocGen5DwEnd.c - VxBus DesignWare END driver for Altera Soc Gen 5*/

/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 *
 * The right to copy, distribute, modify or otherwise make use
 * of this software may be licensed only pursuant to the terms
 * of an applicable Wind River license agreement.
 */

/*
modification history
--------------------
15Aug19,jc  Init
*/

/*
DESCRIPTION
This module implements a Ethernet MAC Controller (EMAC) network
interface driver. The EMAC is fully compliant with the IEEE 802.3
10Mbps Base-T, 100Mbps Base-T, 1Gbps specifications. Hardware support of
the Media Independent Interface (MII) is built-in in the chip. Jumbo frame
supported with length up to 4088 bytes.

This device is on-board.  No jumpering diagram is necessary.

EXTERNAL SUPPORT REQUIREMENTS

When the device is powered on, the EMAC peripheral is in a disabled state.
Before any EMAC specific initialization can take place, the EMAC needs to be
enabled; otherwise, its registers cannot be written and the reads will all
return a value of zero.

The EMAC module is enabled through the Power and Sleep Controller (PSC).
The BSP is responsible to enable the EMAC module before the driver starts.

Also, this driver requires one external support function from the BSP.
\is
\i sysEnetAddrGet()
\cs
STATUS sysEnetAddrGet(UINT32, UINT8 *)
\ce
This routine queries the BSP to provide the ethernet address for a given EMAC.
\ie

EXTERNAL INTERFACE

The driver provides the standard vxbus external interface
altSocGen5DwRegister(). This function registers the driver with the vxbus
subsystem, and instances will be created as needed.  Since altSocGen5Dw is a
processor local bus device, each device instance must be specified in the
hwconf.c file in a BSP.

Driver-specific resources are specified below (in the driver source file and in
the README file).

An example hwconf entry is shown below:

\cs
const struct hcfResource vxbaltSocGen5DwDevResources[] = {
{ "regBase",   HCF_RES_INT,    { (void *)(EMAC1_BASE_ADRS)} },
{ "descMem",   HCF_RES_INT,    { (void *)(EMAC1_DESC_ADRS)} },
{ "irq",       HCF_RES_INT,    { (void *)(INT_VEC_ETH)} },
{ "phyAddr",   HCF_RES_INT,    { (void *)0 } },
{ "miiIfName", HCF_RES_STRING, { (void *)"emac" } },
{ "miiIfUnit", HCF_RES_INT,    { (void *)0 } }
};
\ce

The DesignWare END devices also support jumbo frames. This driver has
jumbo frame support, which is disabled by default in order to conserve
memory (jumbo frames require the use of a buffer pool with larger clusters).
Jumbo frames can be enabled on a per-interface basis using a parameter
override entry in the hwconf.c file in the BSP. For example, to enable
jumbo frame support for interface emac1, the following entry should be
added to the VXB_INST_PARAM_OVERRIDE table:

    { "emac", 1, "jumboEnable", VXB_PARAM_INT32, {(void *)1} }

SEE ALSO: vxBus, miiBus, ifLib, endLib
\tb "VxWorks Device Driver Developers Guide 6.6 Vol 2, part 6"
\tb "Altera Cyclone V Device Handbook.pdf"
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
#include <vxBusLib.h>
#include <wdLib.h>
#include <etherMultiLib.h>
#include <end.h>

#define END_MACROS

#include <endLib.h>
#include <cacheLib.h>
#include <hwif/vxbus/vxBus.h>
#include <hwif/vxbus/hwConf.h>
#include <hwif/vxbus/vxbPlbLib.h>
#include <hwif/util/vxbParamSys.h>
#include "../h/mii/miiBus.h"
#include "../h/vxbus/vxbAccess.h"
#include "../h/hEnd/hEnd.h"

#include <logLib.h>    /* _func_logMsg */

#include "vxbAltSocGen5DwEnd.h"


#undef ALT_DW_END_DEBUG
/* #define ALT_DW_END_DEBUG */

#ifdef ALT_DW_END_DEBUG
#define DBG_LOG_MSG(x,a,b,c,d,e,f)      \
    do {                                \
        if (_func_logMsg != NULL)       \
        _func_logMsg (x,                \
            (_Vx_usr_arg_t)a,           \
            (_Vx_usr_arg_t)b,           \
            (_Vx_usr_arg_t)c,           \
            (_Vx_usr_arg_t)d,           \
            (_Vx_usr_arg_t)e,           \
            (_Vx_usr_arg_t)f);          \
    } while (FALSE)
#else
#define DBG_LOG_MSG(x,a,b,c,d,e,f) do{}while(0)

#define DBG_LOG_MSG2(x,a,b,c,d,e,f)      \
    do {                                \
        if (_func_logMsg != NULL)       \
        _func_logMsg (x,                \
            (_Vx_usr_arg_t)a,           \
            (_Vx_usr_arg_t)b,           \
            (_Vx_usr_arg_t)c,           \
            (_Vx_usr_arg_t)d,           \
            (_Vx_usr_arg_t)e,           \
            (_Vx_usr_arg_t)f);          \
    } while (FALSE)
    
#endif

/* EMAC Register R/W */
#ifdef ARMBE8
#define SWAP32 vxbSwap32
#else
#define SWAP32
#endif /* ARMBE8 */

#undef EMAC_READ_REG
#define EMAC_READ_REG(device, reg) \
        SWAP32 ((*(volatile UINT32 *)(((UINT32)device->pRegBase[0]) + reg)))

#undef EMAC_WRITE_REG
#define EMAC_WRITE_REG(device, reg, data) \
        (*((volatile UINT32 *)(((UINT32)device->pRegBase[0]) + reg)) = SWAP32 (data))

#undef EMAC_SETBIT_4
#define EMAC_SETBIT_4(device, reg, data)    \
        EMAC_WRITE_REG(device, reg, EMAC_READ_REG(device, reg) | (data))

#undef EMAC_CLRBIT_4
#define EMAC_CLRBIT_4(device, reg, data)    \
        EMAC_WRITE_REG(device, reg, EMAC_READ_REG(device, reg) & (~(data)))

/* forward declarations */

IMPORT FUNCPTR _func_m2PollStatsIfPoll;
IMPORT void     vxbUsDelay (int);

IMPORT STATUS sysNetMacAddrSet (char *ifName, int ifUnit, UINT8 *ifMacAddr,
                                int ifMacAddrLen);

IMPORT STATUS sysNetMacAddrGet (char *ifName, int ifUnit, UINT8 *ifMacAddr,
                                int ifMacAddrLen);

IMPORT STATUS sysNetMacNVRamAddrGet (char *ifName, int ifUnit,
                                     UINT8 * ifMacAddr, int ifMacAddrLen);

/* VxBus methods */

LOCAL void altSocGen5DwInstInit (VXB_DEVICE_ID);
LOCAL void altSocGen5DwInstInit2 (VXB_DEVICE_ID);
LOCAL void altSocGen5DwInstConnect (VXB_DEVICE_ID);
LOCAL STATUS altSocGen5DwInstUnlink (VXB_DEVICE_ID, void *);

/* miiBus methods */

LOCAL STATUS altSocGen5DwPhyRead (VXB_DEVICE_ID, UINT8, UINT8, UINT16 *);
LOCAL STATUS altSocGen5DwPhyWrite (VXB_DEVICE_ID, UINT8, UINT8, UINT16);
LOCAL STATUS altSocGen5DwLinkUpdate (VXB_DEVICE_ID);

/* mux methods */

LOCAL void altSocGen5DwMuxConnect (VXB_DEVICE_ID, void *);
LOCAL struct drvBusFuncs altSocGen5DwFuncs =
    {
    altSocGen5DwInstInit,     /* devInstanceInit  */
    altSocGen5DwInstInit2,    /* devInstanceInit2 */
    altSocGen5DwInstConnect   /* devConnect       */
    };

/* Local functions */

LOCAL STATUS altSocGen5DwDmaInit (VXB_DEVICE_ID);
LOCAL STATUS altSocGen5DwMacInit (VXB_DEVICE_ID);
LOCAL VOID   altSocGen5DwMacStart (VXB_DEVICE_ID);
LOCAL VOID   altSocGen5DwMacStop (VXB_DEVICE_ID);
LOCAL STATUS altSocGen5DwMacAddrGet (EMAC_DRV_CTRL *);
LOCAL UINT32 mdioRegDataMake (UINT32, UINT32, UINT32);
LOCAL UINT32 mdioBusyWait (VXB_DEVICE_ID, UINT32);

LOCAL struct vxbDeviceMethod altSocGen5DwMethods [] =
    {
    DEVMETHOD(miiRead,          altSocGen5DwPhyRead),
    DEVMETHOD(miiWrite,         altSocGen5DwPhyWrite),
    DEVMETHOD(miiMediaUpdate,   altSocGen5DwLinkUpdate),
    DEVMETHOD(muxDevConnect,    altSocGen5DwMuxConnect),
    DEVMETHOD(vxbDrvUnlink,     altSocGen5DwInstUnlink),
    {0, 0}
    };

/* default queue parameters */
LOCAL HEND_RX_QUEUE_PARAM altSocGen5DwRxQueueDefault =
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

LOCAL HEND_TX_QUEUE_PARAM altSocGen5DwTxQueueDefault =
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

LOCAL VXB_PARAMETERS altSocGen5DwParamDefaults [] =
    {
        {
        "rxQueue00", VXB_PARAM_POINTER,
            {
            (void *) &altSocGen5DwRxQueueDefault
            }
        },
        
        {
        "txQueue00", VXB_PARAM_POINTER,
            {
            (void *) &altSocGen5DwTxQueueDefault
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

LOCAL struct vxbPlbRegister altSocGen5DwDevPlbRegistration =
    {
        {
        NULL,               /* pNext */
        VXB_DEVID_DEVICE,   /* devID */
        VXB_BUSID_PLB,      /* busID = PLB */
        VXB_VER_4_0_0,      /* vxbVersion */
        DW_EMAC_NAME,       /* drvName */
        &altSocGen5DwFuncs, /* pDrvBusFuncs */
        NULL,               /* pMethods */
        NULL,               /* devProbe */
        altSocGen5DwParamDefaults /* pParamDefaults */
        },
    };

/* END functions */

LOCAL END_OBJ * altSocGen5DwEndLoad (char *, void *);
LOCAL STATUS    altSocGen5DwEndUnload (END_OBJ *);
LOCAL INT32     altSocGen5DwEndIoctl (END_OBJ *, INT32, caddr_t);
LOCAL STATUS    altSocGen5DwEndMCastAddrAdd (END_OBJ *, char *);
LOCAL STATUS    altSocGen5DwEndMCastAddrDel (END_OBJ *, char *);
LOCAL STATUS    altSocGen5DwEndMCastAddrGet (END_OBJ *, MULTI_TABLE *);
LOCAL void      altSocGen5DwEndHashTblPopulate (EMAC_DRV_CTRL *);
LOCAL STATUS    altSocGen5DwEndStatsDump (EMAC_DRV_CTRL *);
LOCAL void      altSocGen5DwEndRxConfig (EMAC_DRV_CTRL *);
LOCAL STATUS    altSocGen5DwEndStart (END_OBJ *);
LOCAL STATUS    altSocGen5DwEndStop (END_OBJ *);
LOCAL INT32     altSocGen5DwEndSend (END_OBJ *, M_BLK_ID);
LOCAL STATUS    altSocGen5DwEndPollSend (END_OBJ *, M_BLK_ID);
LOCAL INT32     altSocGen5DwEndPollReceive (END_OBJ *, M_BLK_ID);
LOCAL void      altSocGen5DwEndInt (EMAC_DRV_CTRL *);
LOCAL void      altSocGen5DwEndRxHandle (EMAC_DRV_CTRL *);
LOCAL void      altSocGen5DwEndTxHandle (EMAC_DRV_CTRL *);
LOCAL void      altSocGen5DwEndIntHandle (void *);

/* jc */
LOCAL void emacTxCounterUpdate(EMAC_DRV_CTRL * pDrvCtrl, M_BLK_ID pMblk);
LOCAL void emacRxCounterUpdate(EMAC_DRV_CTRL * pDrvCtrl, M_BLK_ID pMblk);  

LOCAL NET_FUNCS altSocGen5DwNetFuncs =
    {
    altSocGen5DwEndStart,           /* start func */
    altSocGen5DwEndStop,            /* stop func */
    altSocGen5DwEndUnload,          /* unload func */
    altSocGen5DwEndIoctl,           /* ioctl func */
    altSocGen5DwEndSend,            /* send func */
    altSocGen5DwEndMCastAddrAdd,    /* multicast add func */
    altSocGen5DwEndMCastAddrDel,    /* multicast delete func */
    altSocGen5DwEndMCastAddrGet,    /* multicast get fun */
    altSocGen5DwEndPollSend,        /* polling send func */
    altSocGen5DwEndPollReceive,     /* polling receive func */
    endEtherAddressForm,            /* put address info into a NET_BUFFER */
    endEtherPacketDataGet,          /* get pointer to data in NET_BUFFER */
    endEtherPacketAddrGet           /* Get packet addresses */
    };

/*******************************************************************************
*
* altSocGen5DwRegister - register with the VxBus subsystem
*
* This routine registers the AlteraDw end driver with VxBus
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

void vxbAltSocGen5DwEndRegister (void)
    {
    vxbDevRegister ((struct vxbDevRegInfo *) &altSocGen5DwDevPlbRegistration);
    }

void vxbAltSocGen5DwEndRegister2 (void)
    {
    vxbDevRegister ((struct vxbDevRegInfo *) &altSocGen5DwDevPlbRegistration);
    }

/*******************************************************************************
*
* altSocGen5DwInstInit - VxBus instInit handler
*
* This function implements the VxBus instInit handler for an altSocGen5Dw
* device instance. The only thing done here is to select a unit number for the
* device.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

LOCAL void altSocGen5DwInstInit
    (
    VXB_DEVICE_ID pDev  /* pointer to device */
    )
    {
    const struct hcfDevice * pHcf;

    /* Always use the unit number allocated to us in the hwconf file */
    pHcf = (struct hcfDevice *) pDev->pBusSpecificDevInfo;

    vxbInstUnitSet (pDev, pHcf->devUnit);

    pDev->pMethods = &altSocGen5DwMethods [0];
    }

/*******************************************************************************
*
* altSocGen5DwInstInit2 - VxBus instInit2 handler
*
* This function implements the VxBus instInit2 handler for an altSocGen5Dw
* device instance. Once we reach this stage of initialization, it's
* safe for us to allocate memory, so we can create our pDrvCtrl
* structure and do some initial hardware setup. The important
* steps we do here are to create a child miiBus instance, connect
* our ISR to our assigned interrupt vector, read the station
* address from NVRAM.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

LOCAL void altSocGen5DwInstInit2
    (
    VXB_DEVICE_ID pDev  /* pointer to device */
    )
    {
    EMAC_DRV_CTRL * pDrvCtrl;

    pDrvCtrl = malloc (sizeof (EMAC_DRV_CTRL));

    if (pDrvCtrl == NULL)
        {
        return;
        }

    bzero ((char *) pDrvCtrl, sizeof (EMAC_DRV_CTRL));

    pDev->pDrvCtrl = pDrvCtrl;
    pDrvCtrl->emacDev = pDev;
    pDrvCtrl->emacBar = pDev->pRegBase [0];
    vxbRegMap (pDev, 0, &pDrvCtrl->emacHandle);

    /* get station address */
    pDrvCtrl->emacDevSem = semMCreate (SEM_Q_PRIORITY |
                                       SEM_DELETE_SAFE | SEM_INVERSION_SAFE);

    /* alloc the memory of ring descriptor */

    if ((pDrvCtrl->descBuf = cacheDmaMalloc(sizeof(DMA_DESC) * EMAC_RX_DESC_CNT + sizeof(DMA_DESC) * EMAC_TX_DESC_CNT + 128)) == NULL)
        {
        DBG_LOG_MSG("Could not allocate descriptor memory\n",
                    1,2,3,4,5,6);
        semDelete (pDrvCtrl->emacDevSem);
        free (pDrvCtrl);
        return;
        }

    /* attach our ISR */
    vxbIntConnect (pDev, 0, altSocGen5DwEndInt, pDrvCtrl);
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

LOCAL void altSocGen5DwInstConnect
    (
    VXB_DEVICE_ID pDev /* pointer to device */
    )
    {
    return;
    }

/*******************************************************************************
*
* altSocGen5DwInstUnlink -  VxBus unlink handler
*
* This function shuts down an altSocGen5Dw device instance in response to an
* unlink event from VxBus. This may occur if our VxBus instance has
* been terminated, or if the altSocGen5Dw driver has been unloaded. When an
* unlink event occurs, we must shut down and unload the END interface
* associated with this device instance and then release all the
* resources allocated during instance creation. We also must destroy
* our child miiBus and PHY devices.
*
* RETURNS: OK if device was successfully destroyed, otherwise ERROR
*
* ERRNO: N/A
*/

LOCAL STATUS altSocGen5DwInstUnlink
    (
    VXB_DEVICE_ID pDev,  /* pointer to device */
    void * unused       /* unused pointer */
    )
    {
    EMAC_DRV_CTRL * pDrvCtrl;
    pDrvCtrl = pDev->pDrvCtrl;

    /*
     * Stop the device and detach from the MUX.
     * Note: it's possible someone might try to delete
     * us after our vxBus instantiation has completed,
     * but before anyone has called our muxConnect method.
     * In this case, there'll be no MUX connection to
     * tear down, so we can skip this step.
     */

    if (pDrvCtrl->emacMuxDevCookie != NULL)
        {
        if (muxDevStop (pDrvCtrl->emacMuxDevCookie) != OK)
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

    if (pDrvCtrl->descBuf != NULL)
        {
        cacheDmaFree (pDrvCtrl->descBuf);
        pDrvCtrl->descBuf = NULL;
        }

    /* destroy our MII bus and child PHYs */

    if (pDrvCtrl->emacMiiBus != NULL)
        {
        miiBusDelete (pDrvCtrl->emacMiiBus);
        semDelete (pDrvCtrl->emacDevSem);
        }

    /* disconnect from the ISR */
    vxbIntDisconnect (pDev, 0, altSocGen5DwEndInt, pDrvCtrl);

    /* destroy the adapter context */
    free (pDrvCtrl);
    pDev->pDrvCtrl = NULL;

    return (OK);
    }

/*******************************************************************************
*
* altSocGen5DwDmaInit - initialize DMA engine
*
* This routine initializes the DMA hardware according to the guidence in
* the Ethernet databook.  See Section I.1 "Initializing DMA" in the Synopsis
* "DesignWare Cores Ethernet MAC Universal Databook", Vesion 3.70a,
* March 2011, page 656.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

LOCAL STATUS altSocGen5DwDmaInit
    (
    VXB_DEVICE_ID pDev /* pointer to device */
    )
    {
    UINT32 timeout = 0;
    UINT32 tmp32 = 0;
    EMAC_DRV_CTRL *pDrvCtrl = (EMAC_DRV_CTRL *)pDev->pDrvCtrl;

    /* initialise the DMA engine */

    /* reset the device */
    EMAC_WRITE_REG(pDev, DMA_BUS_MODE, DMA_RESET_ON);
	
	/* wait for write operation done */  /* jc */
	vxbUsDelay (DW_PTP_INTERVAL);

    timeout = DW_DMA_TIMEOUT_VALUE;
    while (timeout)
    {
        if (!(EMAC_READ_REG(pDev, DMA_BUS_MODE) & DMA_RESET_ON))
            break;

        timeout--;
    }

    if (!timeout)
    {
        DBG_LOG_MSG("DMA software reset failed\n",1,2,3,4,5,6);
        return (ERROR);
    }	

    /* Set up the DMA bus burst length */
    EMAC_SETBIT_4(pDev, DMA_BUS_MODE, (DMA_BUS_MODE_PBL
                        | ((DEFAULT_DMA_PBL << DMA_BUS_MODE_PBL_SHIFT)
                        | (DEFAULT_DMA_PBL << DMA_BUS_MODE_RPBL_SHIFT))));

    EMAC_SETBIT_4(pDev, DMA_AXI_BUS_MODE, DMA_AXI_BUS_MODE_4 |
                                          DMA_AXI_BUS_MODE_8 |
                                          DMA_AXI_BUS_MODE_16 |
                                          DMA_AXI_BUS_MODE_32 );

    /* 
    Receive Descriptor List Address Register
    */
    EMAC_WRITE_REG(pDev, DMA_RX_BASE_ADDR, (UINT32)pDrvCtrl->emacRxDescMem);
	/* jc */
	DBG_LOG_MSG ("pDrvCtrl->emacRxDescMem:0x%08X \n", pDrvCtrl->emacRxDescMem, 2,3,4,5,6);

    /* 
    Transmit Descriptor List Address Register 
	*/
    EMAC_WRITE_REG(pDev, DMA_TX_BASE_ADDR, (UINT32)pDrvCtrl->emacTxDescMem);	
	/* jc */
	DBG_LOG_MSG ("pDrvCtrl->emacTxDescMem:0x%08X \n", pDrvCtrl->emacTxDescMem, 2,3,4,5,6);

    EMAC_SETBIT_4(pDev, DMA_CONTROL, DMA_FLUSH_TX_FIFO);
 
    /* wait for write operation done */    
    vxbUsDelay (DW_PTP_INTERVAL);

    /* do nothing */
    timeout = DW_DMA_TIMEOUT_VALUE;
    while (timeout)
        {
        if (!(EMAC_READ_REG(pDev, DMA_CONTROL) & DMA_FLUSH_TX_FIFO))
            break;

        vxbUsDelay (1);
        timeout--;
        }
    if (timeout == 0)
        {
        DBG_LOG_MSG("altSocGen5DwDmaInit failed\n",1,2,3,4,5,6);
        return (ERROR);
        }

   /* EMAC_SETBIT_4(pDev, DMA_CONTROL, DMA_TX_STR_FWD | DMA_TX_SECOND_FRAME | DMA_RX_STR_FWD );*/
    EMAC_SETBIT_4(pDev, DMA_CONTROL, DMA_TX_STR_FWD | DMA_TX_SECOND_FRAME | DMA_RX_STR_FWD | DMA_FWD_ERR_FRAME);

    EMAC_WRITE_REG(pDev, DMA_ISR_STATUS, EMAC_READ_REG(pDev, DMA_ISR_STATUS));
    EMAC_WRITE_REG(pDev, DMA_INTR_ENA, DMA_INT_ENABLE);

    return (OK);
    }

/*******************************************************************************
*
* altSocGen5DwMacInit - initialize Mac engine
*
* This routine initializes the MAC hardware according to the guidence in
* the Ethernet databook.  See Section I.2 "Initializing MAC" in the Synopsis
* "DesignWare Cores Ethernet MAC Universal Databook", Vesion 3.70a,
* March 2011, page 658.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

LOCAL STATUS altSocGen5DwMacInit
    (
    VXB_DEVICE_ID pDev
    )
{
    UINT32 tmp = 0;
    EMAC_DRV_CTRL *pDrvCtrl = (EMAC_DRV_CTRL *)pDev->pDrvCtrl;
    UINT8 *emac = &pDrvCtrl->emacAddr[0];

    /* Steps 1 and 2 are for TBI, GMII, or RTBA PHY Interfaces only */
    tmp = (emac[5] << 8) + emac[4];
    EMAC_WRITE_REG(pDev, EMAC_ADDR_HIGH, tmp);

    tmp = (emac[3] << 24) + (emac[2] << 16) + (emac[1] << 8) + emac[0];
    EMAC_WRITE_REG(pDev, EMAC_ADDR_LOW, tmp);

    EMAC_WRITE_REG(pDev, EMAC_INT_MASK, EMAC_LPI_INT_MASK
                        | EMAC_TIMESTAMP_INT_MASK
                        | EMAC_PSM_ANE_INT_MASK | EMAC_PCS_LINK_INT_MASK
                        | EMAC_MMC_INTSTAT | EMAC_MMC_CKSM_INTSTAT
                        | EMAC_MMC_RX_INTSTAT | EMAC_MMC_TX_INTSTAT );

    /* mask out MMC interrupts because we don't handle them yet */
    EMAC_WRITE_REG(pDev, MMC_RX_INTR_MASK, 0xFFFFFFFF);
    EMAC_WRITE_REG(pDev, MMC_TX_INTR_MASK, 0xFFFFFFFF);
    EMAC_WRITE_REG(pDev, MMC_RX_IPC_INTR_MASK, 0xFFFFFFFF);

#if 0  /* jc */
    EMAC_SETBIT_4(pDev, EMAC_CONTROL, 
		          EMAC_CONTROL_ACS | EMAC_CONTROL_PS | EMAC_CONTROL_JD |
                  EMAC_CONTROL_BE  | EMAC_CONTROL_DM | EMAC_CONTROL_JE);
#else
    tmp = EMAC_CONTROL_ACS | EMAC_CONTROL_PS  | EMAC_CONTROL_JD  | 
          EMAC_CONTROL_BE  | EMAC_CONTROL_DM  | EMAC_CONTROL_JE;

    /* 
     * Set EMAC_CONTROL_DCRS in EMAC_CONTROL to make the MAC transmitter 
     * ignore the (G)MII CRS signal during frame transmission by default
     */
    if (pDrvCtrl->dcrs == TRUE)
    {
        tmp |= EMAC_CONTROL_DCRS;
    }

    EMAC_SETBIT_4 (pDev, EMAC_CONTROL, tmp);
#endif		

	/* jc*/
	tmp = EMAC_READ_REG(pDev, EMAC_CONTROL);

	/* bit27 TWOKPE IEEE 802.3as Support for 2K Packets*/
	/* bit23 WD Watchdog Disable*/
	/* bit20 JE Jumbo Frame Enable*/
    EMAC_WRITE_REG(pDev, EMAC_CONTROL, (tmp | (1<<27) | (1<<23)) );

    return (OK);
}

/*******************************************************************************
*
* altSocGen5DwMacStart - Start the receive and transmit process
*
* This routine does the final setup of the DMA and MAC hardware
* to enable receive and transmit to occur.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

LOCAL VOID altSocGen5DwMacStart
    (
    VXB_DEVICE_ID pDev
    )
{
    /* enable DMA interrupts */
    EMAC_SETBIT_4(pDev, DMA_CONTROL, DMA_RX_START);

    /* enable MAC */
    EMAC_SETBIT_4(pDev, EMAC_CONTROL, EMAC_CONTROL_TE | EMAC_CONTROL_RE );
}

/*******************************************************************************
*
* altSocGen5DwMacStop - Stop the receive and transmit process
*
* This routine disable the DMA and MAC hardware.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

LOCAL VOID altSocGen5DwMacStop
    (
    VXB_DEVICE_ID pDev
    )
{
    /* disable MAC */
    EMAC_CLRBIT_4(pDev, EMAC_CONTROL, EMAC_CONTROL_TE | EMAC_CONTROL_RE );

    /* disable DMA interrupts */
    EMAC_CLRBIT_4(pDev, DMA_CONTROL, DMA_RX_START);
}

/*******************************************************************************
*
* altSocGen5DwPhyRead - miiBus miiRead method
*
* This function implements an miiRead() method that allows PHYs on the miiBus
* to access our MII management registers.
*
* RETURNS: ERROR if invalid PHY addr or register is specified, else OK
*
* ERRNO: N/A
*/

LOCAL STATUS altSocGen5DwPhyRead
    (
    VXB_DEVICE_ID pDev, /* pointer to device */
    UINT8 phyAddr,      /* physical address */
    UINT8 regAddr,      /* register address */
    UINT16 * dataVal)   /* data value */
    {
    EMAC_DRV_CTRL * pDrvCtrl = NULL;
    UINT32 tmp = 0;
    UINT32 timeout = 0;

    UINT32 k;
	
    FUNCPTR         miiRead;  /* jc*/

    if (pDev == NULL)
        {
        DBG_LOG_MSG("pDev is NULL, returning ERROR\n",1,2,3,4,5,6);
        return (ERROR);
        }

    pDrvCtrl = pDev->pDrvCtrl;

#if defined(FMSH_VERIFY_BOARD) || defined(GAMC_1_ENABLE)  /* verify_board*/
	if (phyAddr != pDrvCtrl->emacMiiPhyAddr && phyAddr < 32)
	{
		*dataVal = 0xFFFF;
		return (ERROR);
	}

	if (phyAddr > 31)
	{
		phyAddr -= 32;
	}
	
	/*
	 * If we're not the management device (GEM0), then
	 * forward the read request.
	 */
	if (pDrvCtrl->emacMiiDev != pDev && pDrvCtrl->emacMiiPhyRead != NULL)
	{
		miiRead = pDrvCtrl->emacMiiPhyRead;
		phyAddr += 32;
		return (miiRead (pDrvCtrl->emacMiiDev, phyAddr, regAddr, dataVal));
	}	 	
#endif

#ifndef FMSH_VERIFY_BOARD  /* demo_board*/
    if (dataVal == NULL)
        {
        DBG_LOG_MSG("dataVal was NULL, returning ERROR\n",1,2,3,4,5,6);
        return (ERROR);
        }

    if (phyAddr > 31)
        {
        DBG_LOG_MSG("phyAddr: %d is error, returning ERROR\n", phyAddr,
            2,3,4,5,6);
        return (ERROR);
        }
#endif

    /*
     * If we're not the management device (EMAC0), then
     * forward the read request.
     */

    semTake (pDrvCtrl->emacDevSem, WAIT_FOREVER);

    /* Wait for the busy bit to become clear */

    k = mdioBusyWait (pDev, PHY_TIMEOUT_VALUE);
    if (k == PHY_TIMEOUT_VALUE)
        {
        timeout++;
        }

    /* Read the value from the MII register "regAddr" into R5 */
    tmp = mdioRegDataMake (phyAddr, regAddr, EMAC_MII_ADDR_READ);
    EMAC_WRITE_REG(pDev, EMAC_MII_ADDR, tmp);

    /* Wait for the busy bit to become clear */
    k = mdioBusyWait (pDev, PHY_TIMEOUT_VALUE);
    if (k == PHY_TIMEOUT_VALUE)
        {
        timeout++;
        }

    *dataVal = (UINT16) EMAC_READ_REG(pDev, EMAC_MII_DATA);

    semGive (pDrvCtrl->emacDevSem);

/* jc */
/**/
/* for use_example: ps--rgmii-->switch_port, not ps--rgmii-->phy (ref_ruineng)*/
/**/
#if 0 
	switch (regAddr)
	{
	case MII_CTRL_REG:
		*dataVal = 0x1140;
		break;
	case MII_STAT_REG:
		*dataVal = 0x796D;
		break;
	case MII_PHY_ID1_REG:
		*dataVal = 0x0022;
		break;
	case MII_PHY_ID2_REG:
		*dataVal = 0x1637;
		break;
	case MII_AN_ADS_REG:
		*dataVal = 0x0de1;
		break;
	case MII_AN_PRTN_REG:
		*dataVal = 0x4de1;
		break;
	case MII_AN_EXP_REG:
		*dataVal = 0x0017;
		break;
	}
#endif

/*
	DBG_LOG_MSG ("emacPhyRead(phy-%d,reg-%d)=0x%X \n", phyAddr, regAddr, *dataVal, 4,5,6);
*/
    return (OK);
    }

/*******************************************************************************
*
* altSocGen5DwPhyWrite - miiBus miiWrite method
*
* This function implements an miiWrite() method that allows PHYs on the miiBus
* to access our MII management registers.
*
* RETURNS: ERROR if invalid PHY addr or register is specified, else OK
*
* ERRNO: N/A
*/

LOCAL STATUS altSocGen5DwPhyWrite
    (
    VXB_DEVICE_ID pDev, /* pointer to device */
    UINT8 phyAddr,      /* physical address */
    UINT8 regAddr,      /* register address */
    UINT16 dataVal      /* data value */
    )
    {
    UINT32 k = 0;
    UINT32 timeout = 0;
    EMAC_DRV_CTRL * pDrvCtrl;
    UINT32 tmp = 0;
	
    FUNCPTR         miiWrite;  /* jc*/

    if (pDev == NULL)
        {
        DBG_LOG_MSG("altSocGen5DwPhyWrite: pDev is NULL\n",1,2,3,4,5,6);
        return (ERROR);
        }

    pDrvCtrl = pDev->pDrvCtrl;
	
#if defined(FMSH_VERIFY_BOARD) || defined(GAMC_1_ENABLE)  /* verify_board*/
	if (phyAddr != pDrvCtrl->emacMiiPhyAddr && phyAddr < 32)
	{
		return (ERROR);
	}

	if (phyAddr > 31)
	{
		phyAddr -= 32;
	}
	/*
	 * If we're not the management device (GEM0), then
	 * forward the write request.
	 */
	if (pDrvCtrl->emacMiiDev != pDev && pDrvCtrl->emacMiiPhyWrite != NULL)
	{
		miiWrite = pDrvCtrl->emacMiiPhyWrite;
		phyAddr += 32;
		return (miiWrite (pDrvCtrl->emacMiiDev, phyAddr, regAddr, dataVal));
	}
#endif

	
#ifndef FMSH_VERIFY_BOARD  /* demo_board*/
    if (phyAddr != pDrvCtrl->emacMiiPhyAddr && phyAddr < 32)
        {
        DBG_LOG_MSG("altSocGen5DwPhyWrite: phyAddr was invalid\n",1,2,3,4,5,6);
        return (ERROR);
        }

    if (phyAddr > 31)
        {
        phyAddr -= 32;
        }
#endif

    /*
     * If we're not the management device (EMAC0), then
     * forward the write request.
     */
    semTake (pDrvCtrl->emacDevSem, WAIT_FOREVER);

    /* Wait for the busy bit to become clear */
    k = mdioBusyWait (pDev, PHY_TIMEOUT_VALUE);
    if (k == PHY_TIMEOUT_VALUE)
        {
        timeout++;
        }

    EMAC_WRITE_REG(pDev, EMAC_MII_DATA, dataVal);

    tmp = mdioRegDataMake (phyAddr, regAddr, EMAC_MII_ADDR_WRITE);
    EMAC_WRITE_REG(pDev, EMAC_MII_ADDR, tmp);

    semGive(pDrvCtrl->emacDevSem);
	/* jc */
/*
	DBG_LOG_MSG ("emacPhyWrite(phy-%d,reg-%d)=0x%X \n", phyAddr, regAddr, dataVal, 4,5,6);
*/
    return (OK);
}

/**/
/* jc: gmac tx_clk setup by slcr*/
/**/
#define ETH_SPEED_1G_2     (0x03)
#define ETH_SPEED_100M_2   (0x02)
#define ETH_SPEED_10M_2    (0x01)
extern void eth2_clk_gem_set(int mac_x, int speed);

/*****************************************************************************
*
* altSocGen5DwLinkUpdate - miiBus miiLinkUpdate method
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
LOCAL STATUS altSocGen5DwLinkUpdate
    (
    VXB_DEVICE_ID pDev
    )
    {
    EMAC_DRV_CTRL * pDrvCtrl;
    UINT32          oldStatus;
    UINT32          oldMedia;
    UINT32          emacControl;

	int unit = 0;  /* jc*/
	
    if (pDev->pDrvCtrl == NULL)
        return (ERROR);
	
	unit = pDev->unitNumber; /* jc*/

    pDrvCtrl = (EMAC_DRV_CTRL *)pDev->pDrvCtrl;

    semTake (pDrvCtrl->emacDevSem, WAIT_FOREVER);

    if (pDrvCtrl->emacMiiBus == NULL)
        {
        semGive (pDrvCtrl->emacDevSem);
        return (ERROR);
        }

    /*
     * if we are not with flag IFF_UP, which means this port is set to DOWN,
     * nothing need to do.
     */
    if (!(pDrvCtrl->emacEndObj.flags & IFF_UP))
        {
        semGive (pDrvCtrl->emacDevSem);
        return (OK);
        }

    oldStatus = pDrvCtrl->emacCurStatus;
    oldMedia  = pDrvCtrl->emacCurMedia;
    if (miiBusModeGet (pDrvCtrl->emacMiiBus,
        &pDrvCtrl->emacCurMedia, &pDrvCtrl->emacCurStatus) == ERROR)
        {
        semGive (pDrvCtrl->emacDevSem);
        return (ERROR);
        }

    /* if speed & duplex & (link up/down) was not changed, nothing need to do */
    if ((oldStatus == pDrvCtrl->emacCurStatus) &&
        (oldMedia == pDrvCtrl->emacCurMedia))
        {
        semGive (pDrvCtrl->emacDevSem);
        return (OK);
        }

    /* Configure full duplex mode accordingly */
    emacControl = EMAC_READ_REG(pDev, EMAC_CONTROL);
    if (pDrvCtrl->emacCurMedia & IFM_FDX)
        {
        emacControl |= EMAC_CONTROL_DM;
        }
    else
        {
        emacControl &= ~EMAC_CONTROL_DM;
        }

    switch (IFM_SUBTYPE(pDrvCtrl->emacCurMedia))
        {
        case IFM_1000_T:
            emacControl &= ~EMAC_CONTROL_PS;
            pDrvCtrl->emacEndObj.mib2Tbl.ifSpeed = 1000000000;	
			eth2_clk_gem_set(unit, ETH_SPEED_1G_2); /* jc */
            DBG_LOG_MSG2("...gmac_%d-linkspeed: 1000M bps... \n", unit,2,3,4,5,6);
            break;
        case IFM_100_TX:
            emacControl |= (EMAC_CONTROL_FES | EMAC_CONTROL_PS);
            pDrvCtrl->emacEndObj.mib2Tbl.ifSpeed = 100000000;
			eth2_clk_gem_set(unit, ETH_SPEED_100M_2); /* jc */
            DBG_LOG_MSG2("...gmac_%d-linkspeed: 100M bps... \n", unit,2,3,4,5,6);
            break;
        case IFM_10_T:
            emacControl &= ~EMAC_CONTROL_FES;
            emacControl |= EMAC_CONTROL_PS;
            pDrvCtrl->emacEndObj.mib2Tbl.ifSpeed = 10000000;
			eth2_clk_gem_set(unit, ETH_SPEED_10M_2); /* jc */
            DBG_LOG_MSG("...gmac_%d-linkspeed: 10M bps... \n", unit,2,3,4,5,6);
            break;
        default:
            pDrvCtrl->emacEndObj.mib2Tbl.ifSpeed = 0;			
           DBG_LOG_MSG2("...gmac_%d-linkspeed: 0M bps... \n", unit,2,3,4,5,6);
            break;
        }

    if (pDrvCtrl->emacEndObj.pMib2Tbl != NULL)
        pDrvCtrl->emacEndObj.pMib2Tbl->m2Data.mibIfTbl.ifSpeed =
            pDrvCtrl->emacEndObj.mib2Tbl.ifSpeed;

    /* If status went from down to up, announce link up */
    if (pDrvCtrl->emacCurStatus & IFM_ACTIVE && !(oldStatus & IFM_ACTIVE))
    {
        DBG_LOG_MSG("!!!! gmac_%d->LINK UP !!!! Speed: %d %s\n",
            unit, pDrvCtrl->emacEndObj.mib2Tbl.ifSpeed/1000000,
            (UINT32)((pDrvCtrl->emacCurMedia & IFM_FDX)? "FDX":"HDX"), 4,5,6);

        /* enable the MAC transmitter and receiver */
        emacControl |= (EMAC_CONTROL_TE | EMAC_CONTROL_RE);

        /* write all need bits into emac controller */
        EMAC_WRITE_REG(pDev, EMAC_CONTROL, emacControl);

        jobQueueStdPost (pDrvCtrl->emacJobQueue, NET_TASK_QJOB_PRI,
            muxLinkUpNotify, &pDrvCtrl->emacEndObj,
            NULL, NULL, NULL, NULL);
    }
    /* If status went from up to down, announce link down */
    else if (!(pDrvCtrl->emacCurStatus & IFM_ACTIVE) && oldStatus & IFM_ACTIVE)
    {
        DBG_LOG_MSG("!!!! gmac_%d->LINK DOWN !!!! Speed: %d %s\n",
            unit, pDrvCtrl->emacEndObj.mib2Tbl.ifSpeed/1000000,
            (UINT32)((pDrvCtrl->emacCurMedia & IFM_FDX)? "FDX":"HDX"), 4,5,6);

        /* notify stack to stop send packet first */
        jobQueueStdPost (pDrvCtrl->emacJobQueue, NET_TASK_QJOB_PRI,
            muxLinkDownNotify, &pDrvCtrl->emacEndObj,
            NULL, NULL, NULL, NULL);
		
#if 1  /* jc */
        /* disable the transmit DMA */
        emacControl &= ~DMA_TX_START;

        /* disable the MAC receiver */
        emacControl &= ~EMAC_CONTROL_RE;

        /* flush the Tx FIFO for faster empty operation */
        emacControl |= DMA_FLUSH_TX_FIFO;

        /* write value to emac controller */
        EMAC_WRITE_REG(pDev, EMAC_CONTROL, emacControl);

        /* wait for a moment */
        vxbUsDelay(1000);

        /* disable the MAC transmitter */
        EMAC_CLRBIT_4 (pDev, EMAC_CONTROL, EMAC_CONTROL_TE);
#endif
    }
    else /* there must only speed|duplex change */
    {
        /* write value to emac controller */
        EMAC_WRITE_REG(pDev, EMAC_CONTROL, emacControl);
    }

    semGive (pDrvCtrl->emacDevSem);
    return (OK);
}

/*******************************************************************************
*
* altSocGen5DwMuxConnect - muxConnect method handler
*
* This function handles muxConnect() events, which may be triggered
* manually or (more likely) by the bootstrap code. Most VxBus
* initialization occurs before the MUX has been fully initialized,
* so the usual muxDevLoad()/muxDevStart() sequence must be defered
* until the networking subsystem is ready. This routine will ultimately
* trigger a call to altSocGen5DwEndLoad() to create the END interface instance.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

LOCAL void altSocGen5DwMuxConnect
    (
    VXB_DEVICE_ID pDev, /* pointer to device */
    void * unused       /* unused pointer */
    )
    {
    EMAC_DRV_CTRL * pDrvCtrl;
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
            (void *) &pDrvCtrl->emacMiiPhyAddr);

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

    pDrvCtrl->emacMiiDev = miiDev;
    pDrvCtrl->emacMiiPhyRead = vxbDevMethodGet (miiDev,
                                                (UINT32) &miiRead_desc);
    pDrvCtrl->emacMiiPhyWrite = vxbDevMethodGet (miiDev,
                                                 (UINT32) &miiWrite_desc);

    /* create our MII bus */
    miiBusCreate (pDev, &pDrvCtrl->emacMiiBus);
    miiBusMediaListGet (pDrvCtrl->emacMiiBus, &pDrvCtrl->emacMediaList);
    miiBusModeSet (pDrvCtrl->emacMiiBus,
                   pDrvCtrl->emacMediaList->endMediaListDefault);

    /* save the cookie */
    pDrvCtrl->emacMuxDevCookie = muxDevLoad (pDev->unitNumber,
                                 altSocGen5DwEndLoad, "", TRUE, pDev);
    if (pDrvCtrl->emacMuxDevCookie != NULL)
        {
        muxDevStart (pDrvCtrl->emacMuxDevCookie);
        }

    if (_func_m2PollStatsIfPoll != NULL)
        {
        endPollStatsInit (pDrvCtrl->emacMuxDevCookie, _func_m2PollStatsIfPoll);
        }
    }

/*******************************************************************************
*
* altSocGen5DwEndLoad - END driver entry point
*
* This routine initializes the END interface instance associated with this
* device. In traditional END drivers, this function is the only public
* interface, and it's typically invoked by a BSP driver configuration stub.
* With VxBus, the BSP stub code is no longer needed, and this function is now
* invoked automatically whenever this driver's muxConnect() method is called.
*
* For older END drivers, the load string would contain various configuration
* parameters, but with VxBus this use is deprecated. The load string should
* just be an empty string. The second argument should be a pointer to the VxBus
* device instance associated with this device. Like older END drivers, this
* routine will still return the device name if the init string is empty, since
* this behavior is still expected by the MUX. The MUX will invoke this function
* twice: once to obtain the device name, and then again to create the actual
* END_OBJ instance.
*
* When this function is called the second time, it will initialize the END
* object, perform MIB2 setup, allocate a buffer pool, and initialize the
* supported END capabilities. The only special capability we support is
* VLAN_MTU, since we can receive slightly larger than normal frames.
*
* RETURNS: An END object pointer, or NULL on error, or 0 and the name of the
* device if the <loadStr> was empty.
*
* ERRNO: N/A
*/

LOCAL END_OBJ *altSocGen5DwEndLoad
    (
    char * loadStr,     /* load string */
    void * pArg         /* pointer to arguments */
    )
    {
    EMAC_DRV_CTRL * pDrvCtrl;
    VXB_DEVICE_ID pDev;
    STATUS ret;
    VXB_INST_PARAM_VALUE val;

    /* make the MUX happy */

    if (loadStr == NULL)
        {
        return NULL;
        }

    if (loadStr[0] == 0)
        {
        bcopy (DW_EMAC_NAME, loadStr, sizeof (DW_EMAC_NAME));
        return NULL;
        }

    pDev = pArg;
    pDrvCtrl = pDev->pDrvCtrl;

    if (END_OBJ_INIT (&pDrvCtrl->emacEndObj, NULL, pDev->pName,
            pDev->unitNumber, &altSocGen5DwNetFuncs, "EMAC VxBus END Driver")
            == ERROR)
        {
        DBG_LOG_MSG("%s%d: END_OBJ_INIT failed\n", pDev->pName,
            pDev->unitNumber,3,4,5,6);
        return (NULL);
        }

    DBG_LOG_MSG("MAC: %x:%x:%x:%x:%x:%x\n", pDrvCtrl->emacAddr[0],
            pDrvCtrl->emacAddr[1], pDrvCtrl->emacAddr[2], pDrvCtrl->emacAddr[3],
            pDrvCtrl->emacAddr[4], pDrvCtrl->emacAddr[5]);

    ret = endM2Init (&pDrvCtrl->emacEndObj, M2_ifType_ethernet_csmacd,
            pDrvCtrl->emacAddr, ETHER_ADDR_LEN, ETHERMTU, 1000000000,
            IFF_NOTRAILERS | IFF_SIMPLEX | IFF_MULTICAST | IFF_BROADCAST);

    /*
     * paramDesc {
     * The jumboEnable parameter specifies whether
     * this instance should support jumbo frames.
     * The default is false. }
     */
#if 0  /* origin*/
	ret = vxbInstParamByNameGet (pDev, "jumboEnable", VXB_PARAM_INT32, &val);

    /* allocate a buffer pool */
    if (ret != OK || val.int32Val == 0)
        {
        pDrvCtrl->emacMaxMtu = EMAC_MTU;
        ret = endPoolCreate (EMAC_TUPLE_CNT, &pDrvCtrl->emacEndObj.pNetPool);
        }
    else
        {
        pDrvCtrl->emacMaxMtu = EMAC_JUMBO_MTU;
        ret = endPoolJumboCreate (EMAC_TUPLE_CNT, &pDrvCtrl->emacEndObj.pNetPool);
        }
	
#else  /* jc for test*/

	pDrvCtrl->emacMaxMtu = EMAC_JUMBO_MTU;
	ret = endPoolJumboCreate (EMAC_TUPLE_CNT, &pDrvCtrl->emacEndObj.pNetPool);
#endif

    if (ret == ERROR)
        {
        DBG_LOG_MSG("%s%d: pool creation failed\n", pDev->pName,
            pDev->unitNumber,3,4,5,6);
        return (NULL);
        }

    pDrvCtrl->emacPollBuf = endPoolTupleGet(pDrvCtrl->emacEndObj.pNetPool);

    /* set up polling stats */
    pDrvCtrl->emacEndStatsConf.ifPollInterval = sysClkRateGet ();
    pDrvCtrl->emacEndStatsConf.ifEndObj = &pDrvCtrl->emacEndObj;
    pDrvCtrl->emacEndStatsConf.ifWatchdog = NULL;
    pDrvCtrl->emacEndStatsConf.ifValidCounters = (END_IFINUCASTPKTS_VALID
            | END_IFINMULTICASTPKTS_VALID | END_IFINBROADCASTPKTS_VALID
            | END_IFINOCTETS_VALID | END_IFINERRORS_VALID
            | END_IFINDISCARDS_VALID | END_IFOUTUCASTPKTS_VALID
            | END_IFOUTMULTICASTPKTS_VALID | END_IFOUTBROADCASTPKTS_VALID
            | END_IFOUTOCTETS_VALID | END_IFOUTERRORS_VALID);

    /* set up capabilities */
    pDrvCtrl->emacCaps.cap_available = IFCAP_VLAN_MTU;
    pDrvCtrl->emacCaps.cap_enabled = IFCAP_VLAN_MTU;

    if (pDrvCtrl->emacMaxMtu == EMAC_JUMBO_MTU)
        {
        pDrvCtrl->emacCaps.cap_available |= IFCAP_JUMBO_MTU;
        pDrvCtrl->emacCaps.cap_enabled |= IFCAP_JUMBO_MTU;
        }

    return (&pDrvCtrl->emacEndObj);
    }

/*******************************************************************************
*
* altSocGen5DwEndUnload - unload END driver instance
*
* This routine undoes the effects of altSocGen5DwEndLoad(). The END object
* is destroyed, our network pool is released, the endM2 structures are
* released, and the polling stats watchdog is terminated.
*
* Note that the END interface instance can't be unloaded if the device is still
* running. The device must be stopped with muxDevStop() first.
*
* RETURNS: ERROR if device is still in the IFF_UP state, otherwise OK
*
* RETURN: ERROR or EALREADY
* ERRNO: N/A
*/

LOCAL STATUS altSocGen5DwEndUnload
    (
    END_OBJ * pEnd      /*pointer to end object to be distroyed */
    )
    {
    EMAC_DRV_CTRL * pDrvCtrl;

    /* We must be stopped before we can be unloaded */
    if (pEnd->flags & IFF_UP)
        {
        DBG_LOG_MSG("altSocGen5DwEndUnload ERROR: "
            "end must be stopped before it was unloaded.\n",1,2,3,4,5,6);
        return (ERROR);
        }

    pDrvCtrl = (EMAC_DRV_CTRL *) pEnd;

    netMblkClChainFree (pDrvCtrl->emacPollBuf);

    /* Relase our buffer pool */
    endPoolDestroy (pDrvCtrl->emacEndObj.pNetPool);

    /* terminate stats polling */
    wdDelete (pDrvCtrl->emacEndStatsConf.ifWatchdog);

    endM2Free (&pDrvCtrl->emacEndObj);

    END_OBJECT_UNLOAD(&pDrvCtrl->emacEndObj);

    /* prevent freeing of pDrvCtrl */

    return (EALREADY);
    }

/*******************************************************************************
*
* altSocGen5BitReverse32 - get 32bit bit reverse value.
*
* This function compute 32bit bit reverse value.
*
* RETURNS: bit reversed value.
*
* ERRNO: N/A
*/

LOCAL UINT32 altSocGen5BitReverse32
    (
    UINT32 input
    )
    {
    UINT32 ix = 0;
    UINT32 output = 0;

    for (; ix < 32; ix++)
        {
        output |= ((input & (1 << ix)) != 0) << (31 - ix);
        }
    return output;
    }

/*******************************************************************************
*
* altSocGen5DwEndHashTblPopulate - populate the multicast hash filter
*
* This function programs the FCC controller's multicast hash filter to receive
* frames sent to the multicast groups specified in the multicast address list
* attached to the END object. If the interface is in IFF_ALLMULTI mode, the
* filter will be programmed to receive all multicast packets by setting all the
* bits in the hash table to one.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

LOCAL void altSocGen5DwEndHashTblPopulate
    (
    EMAC_DRV_CTRL * pDrvCtrl /* pointer to driver controller */
    )
    {
    VXB_DEVICE_ID pDev;
    ETHER_MULTI * mCastNode = NULL;
    INT32 count = 0;
    UINT32 mcFilter [8] ={0};
    INT32  bitNr;
    UINT32 Macff = 0;
    int ix = 0;

    pDev = pDrvCtrl->emacDev;

    if (pDrvCtrl->emacEndObj.flags & IFF_ALLMULTI)
        {
        /* set all multicast mode */
        for (ix = 0; ix < EMAC_HASH_LEN; ix++)
            {
            EMAC_WRITE_REG(pDev, EMAC_HASH_BASE + ix*4, 0xFFFFFFFF);
            vxbUsDelay(1);
            }
        Macff = 0;
        Macff = EMAC_READ_REG(pDev, EMAC_FRAME_FILTER);

        /* enable the receive all multi function */
        Macff |= (UINT32)EMAC_FRAME_FILTER_PM;
        Macff &= ~(UINT32)EMAC_FRAME_FILTER_HMC;
        EMAC_WRITE_REG(pDev, EMAC_FRAME_FILTER, Macff);
        return;
        }

    /* clear out the original filter */
    for (ix = 0; ix < EMAC_HASH_LEN; ix++)
        {
        EMAC_WRITE_REG(pDev, EMAC_HASH_BASE + ix*4, 0);
        vxbUsDelay(1);
        }

    /* repopulate */
    for (mCastNode = (ETHER_MULTI *) lstFirst(&pDrvCtrl->emacEndObj.multiList);
            mCastNode != NULL;
            mCastNode = (ETHER_MULTI *) lstNext(&mCastNode->node))
        {
        /*
         * The upper 8 bits of the calculated CRC are used to
         * index the contens of the hash table.
         */
        bitNr = endEtherCrc32LeGet((const UINT8 *) mCastNode->addr,
                ETHER_ADDR_LEN);
        bitNr = altSocGen5BitReverse32 (~bitNr) >> 24;

        mcFilter[bitNr >> 5] |= 1 << (bitNr & 31);

        DBG_LOG_MSG("Add multicast address [%02X:%02X:%02X:%02X:%02X:%02X]\n",
            mCastNode->addr[0],mCastNode->addr[1],mCastNode->addr[2],
            mCastNode->addr[3],mCastNode->addr[4],mCastNode->addr[5]);

        count++;
        }

    /* set hash table */
    for (ix = 0; ix < EMAC_HASH_LEN; ix++)
        {
        EMAC_WRITE_REG(pDev, EMAC_HASH_BASE + ix*4, mcFilter[ix]);
        vxbUsDelay(1);
        }

    if (count > 0)
        {
        EMAC_SETBIT_4(pDev, EMAC_FRAME_FILTER, EMAC_FRAME_FILTER_HMC);
        }
    else
        {
        /* should not be here */
        EMAC_CLRBIT_4(pDev, EMAC_FRAME_FILTER, EMAC_FRAME_FILTER_HMC);
        EMAC_CLRBIT_4(pDev, EMAC_FRAME_FILTER, EMAC_FRAME_FILTER_PM);
        }
    }

/*******************************************************************************
*
* altSocGen5DwEndMCastAddrAdd - add a multicast address for the device
*
* This routine adds a multicast address to whatever the driver is already
* listening for.  It then resets the address filter.
*
* RETURNS: OK or ERROR.
*
* ERRNO: N/A
*/

LOCAL STATUS altSocGen5DwEndMCastAddrAdd
    (
    END_OBJ * pEnd,     /* pointer to end object */
    char * pAddr        /* pointer to address */
    )
    {
    INT32 retVal;
    EMAC_DRV_CTRL * pDrvCtrl;
    VXB_DEVICE_ID   pDev;

    pDrvCtrl = (EMAC_DRV_CTRL *)pEnd;
    pDev = pDrvCtrl->emacDev;

    semTake (pDrvCtrl->emacDevSem, WAIT_FOREVER);

    retVal = etherMultiAdd (&pEnd->multiList, pAddr);

    if (retVal == ENETRESET)
        {
        pEnd->nMulti++;
        altSocGen5DwEndHashTblPopulate ((EMAC_DRV_CTRL *) pEnd);
        }

    semGive (pDrvCtrl->emacDevSem);
    return (OK);
    }

/*******************************************************************************
*
* altSocGen5DwEndMCastAddrDel - delete a multicast address for the device
*
* This routine removes a multicast address from whatever the driver is
* listening for.  It then resets the address filter.
*
* RETURNS: OK or ERROR.
*
* ERRNO: N/A
*/

LOCAL STATUS altSocGen5DwEndMCastAddrDel
    (
    END_OBJ * pEnd,     /* pointer to end object */
    char * pAddr        /* pointer to address */
    )
    {
    INT32 retVal;
    EMAC_DRV_CTRL * pDrvCtrl;

    pDrvCtrl = (EMAC_DRV_CTRL *)pEnd;
    semTake (pDrvCtrl->emacDevSem, WAIT_FOREVER);

    retVal = etherMultiDel(&pEnd->multiList, pAddr);

    if (retVal == ENETRESET)
        {
        pEnd->nMulti--;
        altSocGen5DwEndHashTblPopulate ((EMAC_DRV_CTRL *) pEnd);
        }

    semGive (pDrvCtrl->emacDevSem);
    return (OK);
    }


/*******************************************************************************
 *
 * altSocGen5DwEndMCastAddrGet - get the multicast address list for the device
 *
 * This routine gets the multicast list of whatever the driver is already
 * listening for.
 *
 * RETURNS: OK or ERROR.
 *
 * ERRNO: N/A
 */

LOCAL STATUS altSocGen5DwEndMCastAddrGet
    (
    END_OBJ * pEnd,     /* pointer to the end object */
    MULTI_TABLE * pTable        /* pointer to table */
    )
    {
    EMAC_DRV_CTRL * pDrvCtrl;
    STATUS state = ERROR;
    pDrvCtrl = (EMAC_DRV_CTRL *)pEnd;

    semTake (pDrvCtrl->emacDevSem, WAIT_FOREVER);
    state = etherMultiGet (&pEnd->multiList, pTable);
    semGive (pDrvCtrl->emacDevSem);

    return state;
    }

/*******************************************************************************
 *
 * altSocGen5DwEndStatsDump - return polled statistics counts
 *
 * This routine is automatically invoked periodically by the polled statistics
 * watchdog.  All stats are available from the MIB registers.
 *
 * RETURNS: always OK
 *
 * ERRNO: N/A
 */

LOCAL STATUS altSocGen5DwEndStatsDump
    (
    EMAC_DRV_CTRL * pDrvCtrl    /* pointer to driver controller */
    )
    {
    END_IFCOUNTERS * pEndStatsCounters;

    pEndStatsCounters = &pDrvCtrl->emacEndStatsCounters;

    pEndStatsCounters->ifInOctets = pDrvCtrl->emacInOctets;
    pDrvCtrl->emacInOctets = 0;

    pEndStatsCounters->ifInUcastPkts = pDrvCtrl->emacInUcasts;
    pDrvCtrl->emacInUcasts = 0;

    pEndStatsCounters->ifInMulticastPkts = pDrvCtrl->emacInMcasts;
    pDrvCtrl->emacInMcasts = 0;

    pEndStatsCounters->ifInBroadcastPkts = pDrvCtrl->emacInBcasts;
    pDrvCtrl->emacInBcasts = 0;

    pEndStatsCounters->ifInErrors = pDrvCtrl->emacInErrors;
    pDrvCtrl->emacInErrors = 0;

    pEndStatsCounters->ifInDiscards = pDrvCtrl->emacInDiscards;
    pDrvCtrl->emacInDiscards = 0;

    pEndStatsCounters->ifOutOctets = pDrvCtrl->emacOutOctets;
    pDrvCtrl->emacOutOctets = 0;

    pEndStatsCounters->ifOutUcastPkts = pDrvCtrl->emacOutUcasts;
    pDrvCtrl->emacOutUcasts = 0;

    pEndStatsCounters->ifOutMulticastPkts = pDrvCtrl->emacOutMcasts;
    pDrvCtrl->emacOutMcasts = 0;

    pEndStatsCounters->ifOutBroadcastPkts = pDrvCtrl->emacOutBcasts;
    pDrvCtrl->emacOutBcasts = 0;

    pEndStatsCounters->ifOutErrors = pDrvCtrl->emacOutErrors;
    pDrvCtrl->emacOutErrors = 0;

    return (OK);
    }

/*******************************************************************************
*
* altSocGen5DwEndIoctl - the driver I/O control routine
*
* This function processes ioctl requests supplied via the muxIoctl() routine.
* In addition to the normal boilerplate END ioctls, this driver supports the
* IFMEDIA ioctls, END capabilities ioctls, and polled stats ioctls.
*
* RETURNS: A command specific response, usually OK or ERROR.
*
* ERRNO: N/A
*/

LOCAL INT32 altSocGen5DwEndIoctl
    (
    END_OBJ * pEnd,     /* pointer to end object */
    INT32 cmd,          /* command */
    caddr_t data        /* data */
    )
    {
    EMAC_DRV_CTRL * pDrvCtrl;
    END_MEDIALIST * mediaList;
    END_CAPABILITIES * hwCaps;
    END_MEDIA * pMedia;
    END_RCVJOBQ_INFO * qinfo;
    UINT32 nQs;
    VXB_DEVICE_ID pDev;
    INT32 value;
    INT32 error = OK;

    pDrvCtrl = (EMAC_DRV_CTRL *) pEnd;
    pDev = pDrvCtrl->emacDev;

    if (cmd != EIOCPOLLSTART && cmd != EIOCPOLLSTOP)
        semTake (pDrvCtrl->emacDevSem, WAIT_FOREVER);

    switch (cmd)
    {
    case EIOCSADDR:
        if (data == NULL)
            error = EINVAL;
        else
            bcopy ((char *) data, (char *) pDrvCtrl->emacAddr, ETHER_ADDR_LEN);

        /*
		(void) sysNetMacAddrSet (pDev->pName, pDrvCtrl->miiIfUnit, pDrvCtrl->emacAddr, ETHER_ADDR_LEN);
		*/
		sysNetMacAddrGet (pDev->pName, pDrvCtrl->miiIfUnit, pDrvCtrl->emacAddr, ETHER_ADDR_LEN);
        altSocGen5DwEndRxConfig (pDrvCtrl);
        break;

    case EIOCGADDR:
        if (data == NULL)
            error = EINVAL;
        else
            bcopy ((char *) pDrvCtrl->emacAddr, (char *) data, ETHER_ADDR_LEN);
        break;

    case EIOCSFLAGS:
        value = (INT32) data;
        if (value < 0)
            {
            value = -value;
            value--;
            END_FLAGS_CLR (pEnd, value);
            }
        else
            END_FLAGS_SET (pEnd, value);

        altSocGen5DwEndRxConfig (pDrvCtrl);
        break;

    case EIOCGFLAGS:
        if (data == NULL)
            error = EINVAL;
        else
            *(long *) data = END_FLAGS_GET (pEnd);
        break;

    case EIOCMULTIADD:
        error = altSocGen5DwEndMCastAddrAdd (pEnd, (char *) data);
        break;

    case EIOCMULTIDEL:
        error = altSocGen5DwEndMCastAddrDel (pEnd, (char *) data);
        break;

    case EIOCMULTIGET:
        error = altSocGen5DwEndMCastAddrGet (pEnd, (MULTI_TABLE *) data);
        break;

    case EIOCPOLLSTART:
        pDrvCtrl->emacPolling = TRUE;
        pDrvCtrl->emacIntMask = EMAC_READ_REG (pDev, EMAC_INT_MASK);
        EMAC_SETBIT_4 (pDev, EMAC_INT_MASK, EMAC_RGMII_SMII_INT_MASK);

        /*
         * We may have been asked to enter polled mode while there are
         * transmissions pending. This is a problem, because the polled transmit
         * routine expects that the TX ring will be empty when it's called. In
         * order to guarantee this, we have to drain the TX ring here. We could
         * also just plain reset and reinitialize the transmitter, but this is
         * faster.
         */
        while (pDrvCtrl->emacTxFree < EMAC_TX_DESC_CNT)
            {
            volatile DMA_DESC * pDesc;
            M_BLK_ID pMblk = NULL;

            pDesc = &pDrvCtrl->emacTxDescMem [pDrvCtrl->emacTxCons];

            /* Wait for ownership bit to clear */
            while (SWAP32(pDesc->desc0) & RDESC0_OWN_BY_DMA);

            pMblk = pDrvCtrl->emacTxMblk [pDrvCtrl->emacTxCons];

            if (pMblk != NULL)
                {
                endPoolTupleFree (pMblk);
                pDrvCtrl->emacTxMblk [pDrvCtrl->emacTxCons] = NULL;
                }

            pDrvCtrl->emacTxFree++;
            EMAC_INC_DESC(pDrvCtrl->emacTxCons, EMAC_TX_DESC_CNT);
            }
        break;

    case EIOCPOLLSTOP:
        pDrvCtrl->emacPolling = FALSE;
        EMAC_CLRBIT_4 (pDev, EMAC_INT_MASK, pDrvCtrl->emacIntMask);
        break;

    case EIOCGMIB2233:
    case EIOCGMIB2:
        error = endM2Ioctl (&pDrvCtrl->emacEndObj, cmd, data);
        break;

    case EIOCGPOLLCONF:
        if (data == NULL)
            error = EINVAL;
        else
            *((END_IFDRVCONF **) data) = &pDrvCtrl->emacEndStatsConf;
        break;

    case EIOCGPOLLSTATS:
        if (data == NULL)
            error = EINVAL;
        else
            {
            error = altSocGen5DwEndStatsDump (pDrvCtrl);
            if (error == OK)
                *((END_IFCOUNTERS **) data) = &pDrvCtrl->emacEndStatsCounters;
            }
        break;

    case EIOCGMEDIALIST:
        if (data == NULL)
            {
            error = EINVAL;
            break;
            }
        if (pDrvCtrl->emacMediaList->endMediaListLen == 0)
            {
            error = ENOTSUP;
            break;
            }

        mediaList = (END_MEDIALIST *) data;
        if (mediaList->endMediaListLen
                < pDrvCtrl->emacMediaList->endMediaListLen)
            {
            mediaList->endMediaListLen =
                    pDrvCtrl->emacMediaList->endMediaListLen;
            error = ENOSPC;
            break;
            }

        bcopy ((char *) pDrvCtrl->emacMediaList,(char *) mediaList,
                sizeof (END_MEDIALIST) +
                (sizeof (UINT32) * pDrvCtrl->emacMediaList->endMediaListLen));
        break;

    case EIOCGIFMEDIA:
        if (data == NULL)
            error = EINVAL;
        else
            {
            pMedia = (END_MEDIA *) data;
            pMedia->endMediaActive = pDrvCtrl->emacCurMedia;
            pMedia->endMediaStatus = pDrvCtrl->emacCurStatus;
            }
        break;

    case EIOCSIFMEDIA:
        if (data == NULL)
            error = EINVAL;
        else
            {
            pMedia = (END_MEDIA *) data;
            miiBusModeSet (pDrvCtrl->emacMiiBus, pMedia->endMediaActive);

            if (ERROR == altSocGen5DwLinkUpdate (pDrvCtrl->emacDev))
                {
                error = ERROR;
                }
            else
                error = OK;
            }
        break;

    case EIOCGIFCAP:
        hwCaps = (END_CAPABILITIES *) data;
        if (hwCaps == NULL)
            {
            error = EINVAL;
            break;
            }
        hwCaps->cap_available = pDrvCtrl->emacCaps.cap_available;
        hwCaps->cap_enabled = pDrvCtrl->emacCaps.cap_enabled;
        break;

        /*
         * The only special capability we support is VLAN_MTU, and
         * it can never be turned off.
         */
    case EIOCSIFCAP:
        error = ENOTSUP;
        break;

    case EIOCGIFMTU:
        if (data == NULL)
            error = EINVAL;
        else
            *(INT32 *) data = pEnd->mib2Tbl.ifMtu;
        break;

    case EIOCSIFMTU:
        value = (INT32) data;
        if (value <= 0 || value > pDrvCtrl->emacMaxMtu)
            {
            error = EINVAL;
            break;
            }
        pEnd->mib2Tbl.ifMtu = value;
        if (pEnd->pMib2Tbl != NULL)
            pEnd->pMib2Tbl->m2Data.mibIfTbl.ifMtu = value;
        break;

    case EIOCGRCVJOBQ:
        if (data == NULL)
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
            qinfo->qIds [0] = pDrvCtrl->emacJobQueue;
        break;

    default:
        error = EINVAL;
        break;
    }

    if (cmd != EIOCPOLLSTART && cmd != EIOCPOLLSTOP)
        semGive (pDrvCtrl->emacDevSem);

    return (error);
}

/*******************************************************************************
*
* altSocGen5DwEndRxConfig - configure the FCC's RX filter
*
* This is a helper routine used by altSocGen5DwEndIoctl() and
* altSocGen5DwEndStart() to configure the controller's RX filter. The unicast
* address filter is programmed with the currently configured MAC address, and
* the RX configuration is set to allow unicast and broadcast frames to be
* received. If the interface is in IFF_PROMISC mode, the RX_PROMISC bit is set,
* which allows all packets to be received.
*
* The altSocGen5DwEndHashTblPopulate() routine is also called to update the
* multicast filter.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

LOCAL void altSocGen5DwEndRxConfig
    (
    EMAC_DRV_CTRL * pDrvCtrl    /* pointer to driver control */
    )
    {
    VXB_DEVICE_ID pDev;

    pDev = pDrvCtrl->emacDev;

    /* set the MAC address - must write the low first */

    DBG_LOG_MSG("Setting MAC %d :",pDrvCtrl->miiIfUnit,2,3,4,5,6);
    DBG_LOG_MSG("%02X:%02X:%02X:%02X:%02X:%02X\n",
            pDrvCtrl->emacAddr[0], pDrvCtrl->emacAddr[1],
            pDrvCtrl->emacAddr[2], pDrvCtrl->emacAddr[3],
            pDrvCtrl->emacAddr[4], pDrvCtrl->emacAddr[5]);

    EMAC_WRITE_REG(pDev, EMAC_ADDR_LOW,
            (pDrvCtrl->emacAddr[3] << 24) | (pDrvCtrl->emacAddr[2] << 16) |
            (pDrvCtrl->emacAddr[1] << 8) | (pDrvCtrl->emacAddr[0] << 0));

    EMAC_WRITE_REG(pDev, EMAC_ADDR_HIGH,
            (pDrvCtrl->emacAddr[5] << 8) | (pDrvCtrl->emacAddr[4] << 0));
/*  jc
     * According section 6.2.2.15 of <<DesignWare Cores Ethernet MAC Universal
     * Databook>>, Version 3.70a March 2011.
     * If the MAC address registers are configured to be double-synchronized
     * to the (G)MII clock domains, then the synchronization is triggered only 
     * when Bits[31:24] (in little-endian mode) or Bits[7:0] (in big-endian
     * mode) of the MAC Address0 Low Register are written. For proper
     * synchronization updates, the consecutive writes to this Address Low
     * Register should be performed after at least four clock cycles in the
     * destination clock domain.
     * So, we write it twice here to make sure at least four clock cycles.
     */
    EMAC_WRITE_REG (pDev, EMAC_ADDR_LOW,
                    (pDrvCtrl->emacAddr[3] << 24) |
                    (pDrvCtrl->emacAddr[2] << 16) |
                    (pDrvCtrl->emacAddr[1] << 8)  |
                    (pDrvCtrl->emacAddr[0] << 0));

    EMAC_WRITE_REG (pDev, EMAC_ADDR_HIGH,
                    (pDrvCtrl->emacAddr[5] << 8)  |
                    (pDrvCtrl->emacAddr[4] << 0));

    /* Enable promisc mode, if specified */
    if (pDrvCtrl->emacEndObj.flags & IFF_PROMISC)
        {
        EMAC_SETBIT_4(pDev, EMAC_FRAME_FILTER, EMAC_FRAME_FILTER_OFF);
        }
    else
        {
        EMAC_CLRBIT_4(pDev, EMAC_FRAME_FILTER, EMAC_FRAME_FILTER_OFF);
        }

    /* Program the multicast filter */
    altSocGen5DwEndHashTblPopulate(pDrvCtrl);
    }

/*******************************************************************************
*
* altSocGen5DwMacAddrGet - fetch the MAC address for this channel for a
*                          pDrvCtrl strcture
*
* This function accepts a pDrvCtrl structure and expects the channel number for
* that device to already be correct. Based on this channel number, it fetches
* the MAC address for this channel and populates the appropriate fields in the
* data structure.
*
* RETURNS: ERROR if a NULL was passed or the MAC address could not be found
*          OK otherwise.
*
* ERRNO: N/A
*/

LOCAL STATUS altSocGen5DwMacAddrGet
    (
    EMAC_DRV_CTRL * pDrvCtrl     /* pointer to end object */
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

    pDev = pDrvCtrl->emacDev;
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

    bcopy ((const void *)readMacAddr, (void *)pDrvCtrl->emacAddr,
           (size_t)ETHER_ADDR_LEN);

    DBG_LOG_MSG("MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
            pDrvCtrl->emacAddr[0], pDrvCtrl->emacAddr[1],
            pDrvCtrl->emacAddr[2], pDrvCtrl->emacAddr[3],
            pDrvCtrl->emacAddr[4], pDrvCtrl->emacAddr[5]);

    return (OK);
    }

/*******************************************************************************
*
* altSocGen5DwEndStart - start the device
*
* This function resets the device to put it into a known state and then
* configures it for RX and TX operation. The RX and TX configuration registers
* are initialized, and the address of the RX and TX DMA rings are loaded into
* the device. Interrupts are then enabled, and the initial link state is
* configured.
*
* Note that this routine also checks to see if an alternate jobQueue has been
* specified via the vxbParam subsystem. This allows the driver to divert its
* work to an alternate processing task, such as may be done with TIPC. This
* means that the jobQueue can be changed while the system is running, but the
* device must be stopped and restarted for the change to take effect.
*
* RETURNS: ERROR if device initialization failed, otherwise OK
*
* ERRNO: N/A
*/

LOCAL STATUS altSocGen5DwEndStart
    (
    END_OBJ * pEnd      /* pointer to end object */
    )
    {
    EMAC_DRV_CTRL * pDrvCtrl;
    VXB_DEVICE_ID pDev;
    VXB_INST_PARAM_VALUE val;
    HEND_RX_QUEUE_PARAM * pRxQueue;
    M_BLK_ID pMblk = NULL;
    DMA_DESC * pDesc;
    INT32 i;

    if (pEnd->flags & IFF_UP)
        {
        DBG_LOG_MSG("\nInterface was already up, returning\n",1,2,3,4,5,6);
        return (OK);
        }

    pDrvCtrl = (EMAC_DRV_CTRL *) pEnd;
    pDev = pDrvCtrl->emacDev;

    semTake (pDrvCtrl->emacDevSem, WAIT_FOREVER);
    END_TX_SEM_TAKE (pEnd, WAIT_FOREVER);

    /*
     * Before we start initialing the device, make sure we can get the MAC
     * address.
     */
    if (altSocGen5DwMacAddrGet(pDrvCtrl) != OK)
        {
        DBG_LOG_MSG("Please set a valid MAC address for emac%d\n",
            pDrvCtrl->miiIfUnit,2,3,4,5,6);
        return (ERROR);
        }

    /* We now have a good MAC address in our pDrvCtrl structure */

    /* Initialize job queues */
    pDrvCtrl->emacJobQueue = netJobQueueId;

    /* Override the job queue ID if the user supplied an alternate one */

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
            pDrvCtrl->emacJobQueue = pRxQueue->jobQueId;
        }

    QJOB_SET_PRI(&pDrvCtrl->emacIntJob, NET_TASK_QJOB_PRI);
    pDrvCtrl->emacIntJob.func = altSocGen5DwEndIntHandle;

    vxAtomicSet ((atomicVal_t*)&pDrvCtrl->emacRxPending, FALSE);
    vxAtomicSet ((atomicVal_t*)&pDrvCtrl->emacTxPending, FALSE);
    vxAtomicSet ((atomicVal_t*)&pDrvCtrl->emacIntPending, FALSE);

    /* pDrvCtrl->emacRxDescMem = (DMA_DESC *)ROUND_UP (pDrvCtrl->descBuf, 128);	 */
    pDrvCtrl->emacRxDescMem = (DMA_DESC *)ROUND_UP (pDrvCtrl->descBuf, EMAC_RX_DESC_CNT);	
    pDrvCtrl->emacTxDescMem = (DMA_DESC *)(pDrvCtrl->emacRxDescMem + EMAC_RX_DESC_CNT);

    bzero ((char *)pDrvCtrl->emacRxDescMem, sizeof (DMA_DESC) * EMAC_RX_DESC_CNT);
    bzero ((char *)pDrvCtrl->emacTxDescMem, sizeof (DMA_DESC) * EMAC_TX_DESC_CNT);

    /* 
    set up the rx ring 
	*/
    for (i = 0; i < EMAC_RX_DESC_CNT; i++)
    {
        pMblk = endPoolTupleGet (pDrvCtrl->emacEndObj.pNetPool);
        if (pMblk == NULL)
            return (ERROR);
		
        EMAC_BUF_ADJ (pMblk);

        (void)cacheInvalidate (DATA_CACHE, pMblk->m_data, pMblk->m_len);

        pMblk->m_next = NULL;
		pMblk->m_len  = RDESC1_SIZE1_MASK;   /* jc */
        pDrvCtrl->emacRxMblk [i] = pMblk;

        pDesc = (DMA_DESC *)(&pDrvCtrl->emacRxDescMem[i]);

        /* set the length 1 field, with the lowest two bits clear */
        /* pDesc->desc1 = SWAP32((((EMAC_RX_BUFF_SIZE - 1) & DESC_SIZE1_MASK) & (~0x03)) | DESC_RX_CHAIN); */
        pDesc->desc1 = SWAP32((((EMAC_RX_BUFF_SIZE - 1) & RDESC1_SIZE1_MASK) & (~0x03)) | RDESC1_RX_CHAIN);
		
        pDesc->desc2 = SWAP32(VXB_ADDR_LOW32(mtod(pMblk, char *)));
		
        /* descriptor ring segmentation */
        if (i != 0)
            pDesc->desc1 |= SWAP32(RDESC1_RX_INT_DISABLE);
		
		if (i == (EMAC_RX_DESC_CNT - 1))  /* jc */
		{
			/* last descriptor in the list */
			pDesc->desc1 |= SWAP32(RDESC1_RX_END_OF_RING);	/* bit25: Receive End of Ring */
		}		

        /*
         * In chained mode the desc3 points to the next element in the ring.
         * The latest element has to point to the head.
         */
        pDesc->desc3 = SWAP32((UINT32)(&pDrvCtrl->emacRxDescMem[(i+1) % EMAC_RX_DESC_CNT]));
		
		/* Give the descriptor to the DMA */
		pDesc->desc0 = SWAP32(RDESC0_OWN_BY_DMA);
     }

    /* 
    set up tx ring 
	*/
    for (i = 0; i < EMAC_TX_DESC_CNT; i++)
    {
        pDesc = &pDrvCtrl->emacTxDescMem [i];
		
#if 0  /* origin */
        pDesc->desc0 = SWAP32(DESC_TX_CHAIN);

        /* set the length 1 field  */
        pDesc->desc1 = SWAP32(0 & DESC_SIZE1_MASK);
		
#else  /* jc */
		pDesc->desc0 = 0;

        /* set the length 1 field  */
		pDesc->desc1 = SWAP32 (0 & TDESC1_SIZE1_MASK);     /* Bit10~0 TBS1: Transmit Buffer 1 Size */
		pDesc->desc1 |= SWAP32 (TDESC1_TX_CHAIN);  /* jc */		
		if (i == (EMAC_TX_DESC_CNT - 1))  /* jc */
		{
			/* last descriptor in the list */
			pDesc->desc1 |= TDESC1_TX_END_OF_RING;	/* bit25: Transmit End of Ring */
		}
#endif

        /* set the transmit buffer address*/
        pDesc->desc2 = (UINT32)NULL;
        pDesc->desc3 = SWAP32((UINT32)&pDrvCtrl->emacTxDescMem [(i+1) % EMAC_TX_DESC_CNT]);

        /* set TX done buffer to NULL */
        pDrvCtrl->emacTxMblk[i] = NULL;
    }

    /* Initialize state */

    pDrvCtrl->emacRxIdx   = 0;
    pDrvCtrl->emacTxLast  = 0;
    pDrvCtrl->emacTxStall = FALSE;
    pDrvCtrl->emacTxProd  = 0;
    pDrvCtrl->emacTxCons  = 0;
    pDrvCtrl->emacTxFree  = EMAC_TX_DESC_CNT;

    /* Zero the stats counters */

    pDrvCtrl->emacInOctets   = 0;
    pDrvCtrl->emacInUcasts   = 0;
    pDrvCtrl->emacInBcasts   = 0;
    pDrvCtrl->emacInMcasts   = 0;
    pDrvCtrl->emacInErrors   = 0;
    pDrvCtrl->emacInDiscards = 0;
    pDrvCtrl->emacOutUcasts  = 0;
    pDrvCtrl->emacOutBcasts  = 0;
    pDrvCtrl->emacOutMcasts  = 0;
    pDrvCtrl->emacOutOctets  = 0;
    pDrvCtrl->emacOutErrors  = 0;

    /* initialize the DMA engine */
    altSocGen5DwDmaInit (pDev);

    /* initialize the MAC engine */
    altSocGen5DwMacInit (pDev);

    /* setup RX */
    altSocGen5DwEndRxConfig (pDrvCtrl);

    /* set initial link state */
    pDrvCtrl->emacCurMedia = IFM_ETHER | IFM_NONE;
    pDrvCtrl->emacCurStatus = IFM_AVALID;

    miiBusModeSet (pDrvCtrl->emacMiiBus,
                   pDrvCtrl->emacMediaList->endMediaListDefault);

    END_FLAGS_SET (pEnd, (IFF_UP | IFF_RUNNING));

    /* start up the Ethernet driver */
    altSocGen5DwMacStart (pDev);

    /* enable ISR */
    vxbIntEnable (pDev, 0, altSocGen5DwEndInt, pDrvCtrl);

    END_TX_SEM_GIVE (pEnd);
    semGive (pDrvCtrl->emacDevSem);

    return (OK);
}

/*******************************************************************************
*
* altSocGen5DwEndStop - stop the device
*
* This function undoes the effects of altSocGen5DwEndStart(). The device is
* shut down and all resources are released. Note that the shutdown process
* pauses to wait for all pending RX, TX and link event jobs that may have been
* initiated by the interrupt handler to complete. This is done to prevent
* tNetTask from accessing any data that might be released by this routine.
*
* RETURNS: ERROR if device shutdown failed, otherwise OK
*
* ERRNO: N/A
*/

LOCAL STATUS altSocGen5DwEndStop
    (
    END_OBJ * pEnd      /* pointer to end object */
    )
    {
    EMAC_DRV_CTRL * pDrvCtrl;
    VXB_DEVICE_ID pDev;
    INT32 i;

    if (!(pEnd->flags & IFF_UP))
        {
        return (OK);
        }

    pDrvCtrl = (EMAC_DRV_CTRL *) pEnd;
    if (pDrvCtrl == NULL)
        {
        DBG_LOG_MSG("pDrvCtrl is NULL, returning ERROR\n",1,2,3,4,5,6);
        return (ERROR);
        }

    pDev = pDrvCtrl->emacDev;

    if (pDev == NULL)
        {
        DBG_LOG_MSG("pDev is NULL, returning ERROR\n",1,2,3,4,5,6);
        return (ERROR);
        }

    semTake (pDrvCtrl->emacDevSem, WAIT_FOREVER);

    END_FLAGS_CLR (pEnd, (IFF_UP | IFF_RUNNING));

    altSocGen5DwMacStop (pDev);

    vxbIntDisable (pDev, 0, altSocGen5DwEndInt, pDrvCtrl);

    /*
     * Wait for all jobs to drain.
     * Note: this must be done before we disable the receiver and transmitter
     * below. If someone tries to reboot us via WDB, this routine may be invoked
     * while the RX handler is still running in tNetTask. If we disable the chip
     * while that function is running, it'll start reading inconsistent status
     * from the chip. We have to wait for that job to terminate first, then we
     * can disable the receiver and transmitter.
     */

    for (i = 0; i < EMAC_TIMEOUT; i++)
        {
        if (vxAtomicGet ((atomic_t*)&pDrvCtrl->emacRxPending) == FALSE
            && vxAtomicGet ((atomic_t*)&pDrvCtrl->emacTxPending) == FALSE
            && vxAtomicGet ((atomic_t*)&pDrvCtrl->emacIntPending) == FALSE)
            break;
        }

    if (i == EMAC_TIMEOUT)
        {
        DBG_LOG_MSG("%s%d: timed out waiting for job to complete\n",
            pDev->pName, pDev->unitNumber,3,4,5,6);
        }

    /* release resources */

    for (i = 0; i < EMAC_RX_DESC_CNT; i++)
        {
        if (pDrvCtrl->emacRxMblk [i] != NULL)
            {
            netMblkClChainFree(pDrvCtrl->emacRxMblk [i]);
            pDrvCtrl->emacRxMblk [i] = NULL;
            }
        }

    /*
     * Flush the recycle cache to shake loose any of our mBlks that may be
     * stored there.
     */

    endMcacheFlush ();

    END_TX_SEM_TAKE (pEnd, WAIT_FOREVER);

    for (i = 0; i < EMAC_TX_DESC_CNT; i++)
        {
        if (pDrvCtrl->emacTxMblk [i] != NULL)
            {
            netMblkClChainFree (pDrvCtrl->emacTxMblk [i]);
            pDrvCtrl->emacTxMblk [i] = NULL;
            }
        }

    END_TX_SEM_GIVE (pEnd);

    semGive (pDrvCtrl->emacDevSem);
    return (OK);
    }

/*******************************************************************************
*
* altSocGen5DwEndInt - handle new END interrupt
*
* This routine handles new END interrupts.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

LOCAL void altSocGen5DwEndInt
    (
    EMAC_DRV_CTRL * pDrvCtrl    /* pointer to driver controller */
    )
    {
    VXB_DEVICE_ID pDev;

    pDev = pDrvCtrl->emacDev;

    /* not necessary to check return value here */
    (void)vxAtomic32Set ((atomic_t *) &pDrvCtrl->emacIntPending, TRUE);

    /* mask interrupts here */
    EMAC_WRITE_REG (pDev, DMA_INTR_ENA, DMA_INT_DISABLE);
    EMAC_SETBIT_4(pDev, EMAC_INT_MASK, EMAC_RGMII_SMII_INT_MASK);
    jobQueuePost (pDrvCtrl->emacJobQueue, &pDrvCtrl->emacIntJob);
    }

/*******************************************************************************
*
* altSocGen5DwEndIntHandle - process received frames and transmited frames
*
* This function is scheduled by the ISR to run in the context of tNetTask
* whenever an RX/TX interrupt is received.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/
#if 1
LOCAL void altSocGen5DwEndIntHandle
    (
    void * pArg /* pointer to arguments */
    )
    {
    QJOB * pJob;
    EMAC_DRV_CTRL * pDrvCtrl;
    VXB_DEVICE_ID pDev;
    UINT32 status = 0;
    UINT32 tmp = 0;

    pJob = pArg;
    pDrvCtrl = member_to_object (pJob, EMAC_DRV_CTRL, emacIntJob);
    pDev = pDrvCtrl->emacDev;

    status = EMAC_READ_REG(pDev, EMAC_INT_STATUS);
    if ((status & EMAC_INT_STATUS_LINK_MASK) == EMAC_INT_STATUS_LINKCHANGED)
        {
        DBG_LOG_MSG("Link interrupt \n",1,2,3,4,5,6);

        /* status changed, read SGMII register to clear */
        EMAC_READ_REG(pDev, EMAC_GMII_STATUS);
		
		/**/
		/* ps-->rgmii-->switch_port*/
		/* ref-zhimingda: while real-phy not exist, remove the clr function*/
		/**/
        (void) altSocGen5DwLinkUpdate(pDev);
        }

    /* Read and acknowledge interrupts here */
    status = EMAC_READ_REG (pDev, DMA_ISR_STATUS);
    EMAC_WRITE_REG (pDev, DMA_ISR_STATUS, status);

    if (status & DMA_ERROR_BIT1)
    {
        /*
        * reading the PMT control and status register is enough to clear the
        * status bits.
        */
        tmp = EMAC_READ_REG (pDev, EMAC_PMT);
		DBG_LOG_MSG ("encounter error 0x%x \n", tmp, 2,3,4,5,6);
    }

    if (status & DMA_INT_NORMAL)
    {
        /* RX done */
        if ((status & DMA_INT_RX_COMPLETED) && vxAtomicSet ((atomic_t*)&pDrvCtrl->emacRxPending, TRUE) == FALSE)
        {        
            /* update RX watchdog timer */
            EMAC_WRITE_REG (pDev, DMA_RX_WATCHDOG, 0x80);  /* jc */
			
            altSocGen5DwEndRxHandle (pDrvCtrl);
			/* DBG_LOG_MSG("---RX done--- \n", 1,2,3,4,5,6); */
        }
		
		/* TX done at first */
        if ((status & DMA_INT_TX_COMPLETED) && vxAtomicSet ((atomic_t*)&pDrvCtrl->emacTxPending, TRUE) == FALSE)
        {
            altSocGen5DwEndTxHandle (pDrvCtrl);
			/* DBG_LOG_MSG("...TX done... \n", 1,2,3,4,5,6); */
        }
		
    }

    if (status & DMA_INT_ABNORMAL)
    {
        /* Receive buffer unavailable */
        if ((status & DMA_INT_RX_NO_BUFFER) && vxAtomicSet ((atomic_t*)&pDrvCtrl->emacRxPending, TRUE) == FALSE)
        {

            /* To resume processing Receive descriptors, the host should 
             * change the ownership of the descriptor and issue a Receive
             * Poll Demand command.
             */
            altSocGen5DwEndRxHandle (pDrvCtrl);
            EMAC_WRITE_REG (pDev, DMA_RX_POLL_DEMAND, status);
        }
    }

    vxAtomicSet ((atomic_t*)&pDrvCtrl->emacIntPending, FALSE);

    /* unmask the interrupt here */
    EMAC_WRITE_REG (pDev, DMA_INTR_ENA, DMA_INT_ENABLE);

	/**/
	/* ps-->rgmii-->switch_port*/
	/* ref-zhimingda: while real-phy not exist, remove the clr function*/
	/**/
    EMAC_CLRBIT_4 (pDev, EMAC_INT_MASK, EMAC_RGMII_SMII_INT_MASK);
}

#endif

/* jc */
#define END_CACHE_INVAL_POST_DMA(vaddr, len) (void)cacheInvalidate (DATA_CACHE, vaddr, len)


/*******************************************************************************
*
* altSocGen5DwEndRxHandle - process received frames
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
#undef DBG_ARP_MSG
#undef DBG_MULTICAST_MSG

LOCAL void altSocGen5DwEndRxHandle
    (
    EMAC_DRV_CTRL * pDrvCtrl
    )
    {
    M_BLK_ID pMblk;
    M_BLK_ID pNewMblk;
    DMA_DESC * pDesc;
    INT32 frameLength = 0;
    INT32 loopCounter = EMAC_RX_DESC_CNT;

    LOCAL M_BLK_ID firstMBlk = NULL;
    LOCAL M_BLK_ID lastMBlk = NULL;
    LOCAL UINT32 pktLen = 0;

	/*logMsg("gmac rx irq! \n", 1,2,3,4,5,6);*/
	
    while (loopCounter)
    {
        pDesc = &pDrvCtrl->emacRxDescMem[pDrvCtrl->emacRxIdx];

        /* if we do not own this descriptor, stop processing */
        if (SWAP32(pDesc->desc0) & RDESC0_OWN_BY_DMA)
        {
            break;
        }

        pNewMblk = endPoolTupleGet (pDrvCtrl->emacEndObj.pNetPool);
        if ((pNewMblk == NULL) ||
            ((SWAP32(pDesc->desc0) & (RDESC0_ERROR | RDESC0_RX_LAST)) == (RDESC0_ERROR | RDESC0_RX_LAST)))
        {
            if (pNewMblk == NULL)
                {
                pDrvCtrl->emacLastError.errCode = END_ERR_NO_BUF;
                muxError(&pDrvCtrl->emacEndObj, &pDrvCtrl->emacLastError);
                /* pDrvCtrl->emacInDiscards++; */ /* jc move to below */
                }
            else
                {
                endPoolTupleFree (pNewMblk);
                pDrvCtrl->emacInErrors++;
                }
			
			pDrvCtrl->emacInDiscards++;  /* move to here */
			
            /* Reset this descriptor's fields */
            pDesc->desc0 = 0;

            /* set the length 1 field, with the lowest two bits clear */
            pDesc->desc1 = SWAP32((((EMAC_RX_BUFF_SIZE-1) & RDESC1_SIZE1_MASK)
                            & (~0x03)) | RDESC1_RX_CHAIN);

            /* Give the descriptor to the DMA */
            pDesc->desc0 |= SWAP32(RDESC0_OWN_BY_DMA);

            EMAC_INC_DESC(pDrvCtrl->emacRxIdx, EMAC_RX_DESC_CNT);
            loopCounter--;
            continue;
        }

        /* swap the mBlks */
        pMblk = pDrvCtrl->emacRxMblk[pDrvCtrl->emacRxIdx];
        pDrvCtrl->emacRxMblk[pDrvCtrl->emacRxIdx] = pNewMblk;

        /* advance to the next descriptor */
        EMAC_INC_DESC(pDrvCtrl->emacRxIdx, EMAC_RX_DESC_CNT);

        loopCounter--;

        /* adjust the data start address in buffer */
        EMAC_BUF_ADJ (pNewMblk);

		/* jc
		 * there is a reason to cacheInvalidate the buffer before DMA push 
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
        (void)cacheInvalidate (DATA_CACHE, pNewMblk->m_data, pNewMblk->m_len);
		
        if ((SWAP32(pDesc->desc0) & (RDESC0_RX_FIRST | RDESC0_RX_LAST)) == (RDESC0_RX_FIRST | RDESC0_RX_LAST))
        {
            /* single frame */
            frameLength = ((SWAP32(pDesc->desc0) & RDESC0_FRAME_LENGTH_MASK) >>
                            RDESC0_FRAME_LENGTH_SHIFT);

            /* Set the mBlk header up with the frame length. */
            pMblk->mBlkHdr.mFlags |= (M_PKTHDR | M_EXT);
            pMblk->mBlkHdr.mLen = frameLength - ETHER_CRC_LEN;
            pMblk->mBlkPktHdr.len = frameLength - ETHER_CRC_LEN;

            cacheInvalidate (DATA_CACHE, pMblk->m_data, pMblk->m_len); /**/

            /* Update statistics for counters */
            pDrvCtrl->emacInOctets += pMblk->m_len;
		    emacRxCounterUpdate (pDrvCtrl, pMblk);
            /*
              
			if (((UINT8) pMblk->m_data[0] == 0xFF) && 
				((UINT8) pMblk->m_data[1] == 0xFF) && 
				((UINT8) pMblk->m_data[2] == 0xFF) && 
				((UINT8) pMblk->m_data[3] == 0xFF) && 
				((UINT8) pMblk->m_data[4] == 0xFF) && 
				((UINT8) pMblk->m_data[5] == 0xFF) )
            {
                pDrvCtrl->emacInBcasts++;
            }
            else if ((UINT8) pMblk->m_data[0] & 0x1)
            {
                pDrvCtrl->emacInMcasts++;
            }
            else
            {
                pDrvCtrl->emacInUcasts++;
            } */ 

            END_CACHE_INVAL_POST_DMA (pMblk->m_data, pMblk->m_len);  /* jc */

            /* Give the frame to the stack. */
            END_RCV_RTN_CALL(&pDrvCtrl->emacEndObj, pMblk);

			
			if (frameLength > 96)
			{	
				/*logMsg("gmac rx: RDESC0_single(%d) \n", frameLength, 2,3,4,5,6);*/
			}
        }
        else if (SWAP32(pDesc->desc0) & RDESC0_RX_FIRST)
        {
            /* first frame */
            firstMBlk = pMblk;
            lastMBlk = pMblk;

            firstMBlk->m_next = NULL;

            /* read buffer size from Receive Buffer 1 Size field */
            pktLen = (SWAP32(pDesc->desc1) & RDESC1_RX_RBS1SZ_MASK) >> RDESC1_RX_RBS1SZ_SHIFT;
            firstMBlk->m_len = pktLen;
            firstMBlk->mBlkHdr.mFlags |= ( M_EXT |M_PKTHDR);

			
			/*logMsg("gmac rx: RDESC0_RX_FIRST(%d) \n", pktLen,2,3,4,5,6);*/
        }
        else if (SWAP32(pDesc->desc0) & RDESC0_RX_LAST)
        {
            /* last frame */
            if ((firstMBlk == NULL) || (lastMBlk == NULL))
            {
                endPoolTupleFree (pMblk);
                firstMBlk = NULL;
                lastMBlk = NULL;
                pktLen = 0;
            }
            else
            {
                /* frame Length is valid when Last Descriptor is set */
                frameLength = ((SWAP32(pDesc->desc0) & RDESC0_FRAME_LENGTH_MASK) >> RDESC0_FRAME_LENGTH_SHIFT);

                /*
                 * If buffer size <= ETHER_CRC_LEN, use the previous pMBlk as
                 * the last pMBlk, and pad remaining FCS field to the tail of
                 * previous pMBlk to keep consistency with other drivers.
                 */
                if ((frameLength - pktLen) <= ETHER_CRC_LEN)
                {
                    bcopy (mtod(pMblk, char *),
                           &lastMBlk->m_data[lastMBlk->mBlkHdr.mLen],
                           frameLength - pktLen);
                    endPoolTupleFree (pMblk);
                }
                else
                {
                    lastMBlk->mBlkHdr.mNext = pMblk;
                    lastMBlk = pMblk;
                    lastMBlk->mBlkHdr.mLen = 0;
                }

                /* update lastMBlk */
                lastMBlk->mBlkHdr.mNext = NULL;
                lastMBlk->mBlkHdr.mLen += frameLength - pktLen - ETHER_CRC_LEN;
                lastMBlk->mBlkHdr.mFlags |= (M_EOR | M_EXT);
				
                /* total length of packet */      
                firstMBlk->mBlkPktHdr.len = frameLength - ETHER_CRC_LEN;

                /* invalidate MBlk chain */
                for (pMblk = firstMBlk; pMblk != NULL;
                     pMblk = pMblk->mBlkHdr.mNext)
                    cacheInvalidate (DATA_CACHE, pMblk->m_data, pMblk->m_len);

                /* Update statistics for counters */
                pDrvCtrl->emacInOctets += firstMBlk->mBlkPktHdr.len;
				emacRxCounterUpdate (pDrvCtrl, firstMBlk); 
				/*
                       
				if (((UINT8) pMblk->m_data[0] == 0xFF) && 
					((UINT8) pMblk->m_data[1] == 0xFF) && 
					((UINT8) pMblk->m_data[2] == 0xFF) && 
					((UINT8) pMblk->m_data[3] == 0xFF) && 
					((UINT8) pMblk->m_data[4] == 0xFF) && 
					((UINT8) pMblk->m_data[5] == 0xFF) )
                {
                    pDrvCtrl->emacInBcasts++;
                }
                else if ((UINT8) firstMBlk->m_data[0] & 0x1)
                {
                    pDrvCtrl->emacInMcasts++;
                }
                else
                {
                    pDrvCtrl->emacInUcasts++;
                }*/

				/* jc */
                /* 2port */
				for (pMblk = firstMBlk; pMblk != NULL; pMblk = pMblk->m_next)
                {
                    END_CACHE_INVAL_POST_DMA (pMblk->m_data, pMblk->m_len);
                }
				/*
				for (pMblk = firstMBlk; pMblk != NULL;  pMblk = pMblk->mBlkHdr.mNext)
                {    
                	cacheInvalidate (DATA_CACHE, pMblk->m_data, pMblk->m_len);
                }	
				*/

                /* give the frame to the stack */
                END_RCV_RTN_CALL(&pDrvCtrl->emacEndObj, firstMBlk);

                /* reset */
                firstMBlk = NULL;
                lastMBlk = NULL;
                pktLen = 0;
            }
			
			/*logMsg("gmac rx: RDESC0_RX_LAST(%d) \n", frameLength,2,3,4,5,6);*/
        }
        else
        {
            /* middle frame*/
            if ((firstMBlk == NULL) || (lastMBlk == NULL))
            {
                endPoolTupleFree (pMblk);
                firstMBlk = NULL;
                lastMBlk = NULL;
                pktLen = 0;
            }
            else
            {
                /* update lastMBlk */
                lastMBlk->mBlkHdr.mNext = pMblk;
                lastMBlk = pMblk;

                lastMBlk->m_next = NULL;
                lastMBlk->m_len = (SWAP32(pDesc->desc1) & RDESC1_RX_RBS1SZ_MASK) >> RDESC1_RX_RBS1SZ_SHIFT;
                lastMBlk->mBlkHdr.mFlags |= (M_EXT);

                /* update packet length */
                pktLen += lastMBlk->m_len;
            }
			
			/*logMsg("gmac rx: RDESC0_RX_middle(%d) \n", lastMBlk->m_len,2,3,4,5,6);*/
        }

        /* set the length 1 field, with the lowest two bits clear */
        pDesc->desc1 = SWAP32((((EMAC_RX_BUFF_SIZE-1) & RDESC1_SIZE1_MASK) &  (~0x03)) | RDESC1_RX_CHAIN);

        pDesc->desc2 = SWAP32((UINT32)pNewMblk->m_data);
		
		/* give the descriptor to the DMA */
		pDesc->desc0 = SWAP32(RDESC0_OWN_BY_DMA);
    }

    cachePipeFlush (); /* */
	
#if defined(DBG_ARP_MSG) || defined(DBG_MULTICAST_MSG) /* add by jc for test 2018-03-07 */
		UINT8 * pBuf8;
		UINT16 * pBuf16;
		UINT32 * pBuf32;
		int i = 0;
#endif

#ifdef DBG_ARP_MSG /* add by jc for test 2018-03-07 */
		/*
		arp:
		<<<<Rx_mLen=60 
		0x9cbb7e0 (tNet0): 00:A0:1E-87:90:78 
		0x9cbb7e0 (tNet0): 00:04:9F-A1:AA:07 
		0x9cbb7e0 (tNet0): 08060001 
		0x9cbb7e0 (tNet0): 08000604 
		0x9cbb7e0 (tNet0): 0002 
		0x9cbb7e0 (tNet0): 00:04:9F-A1:AA:07 
		0x9cbb7e0 (tNet0): 192.168.7.7 
		0x9cbb7e0 (tNet0): 00:A0:1E-87:90:78 
		0x9cbb7e0 (tNet0): 192.168.7.10 
		0x9cbb7e0 (tNet0): 00000000 
		0x9cbb7e0 (tNet0): 00000000 
		0x9cbb7e0 (tNet0): 00000000 
		0x9cbb7e0 (tNet0): 00000000 
		
		udp_tx:
		----------------------------------
		header:
		-------
		| (32bit)dst_mac |
		| (32bit)src_mac |
		| (16bit)type |
		----------------------------------
		udp:
		----
		| (32bit)src_ip |
		| (32bit)dst_ip |
		| 0 | (8bit)17 | (16bit)Udp_len |
		
		0x9cbb7e0 (tNet0): 00:A0:1E-87:90:78 
		0x9cbb7e0 (tNet0): 00:04:9F-A1:AA:07 
		0x9cbb7e0 (tNet0): 0800  (0806-arp; 0800-upd; 0835-rarp)
						   4500 003C
						   918D 0000 
						   4011 59C2
						   C0.A8.7.7
						   C0.A8.7.0A					
						   CB67 1F90  (52071 - 8080)
						   0028 9443					
		0x9cbb7e0 (tNet0): 00010203 
		0x9cbb7e0 (tNet0): 04050607 
		0x9cbb7e0 (tNet0): 08090A0B 
		0x9cbb7e0 (tNet0): 0C0D0E0F 
		0x9cbb7e0 (tNet0): 10111213 
		0x9cbb7e0 (tNet0): 14151617 
		0x9cbb7e0 (tNet0): 18191A1B 
		0x9cbb7e0 (tNet0): 1C1D1E1F
		*/
		
		/* if (pDev->unitNumber != 3) // not front-eth */
		{
			pBuf8 = (UINT32 *)(&pMblk->mBlkHdr.mData[12]);
			if ((pBuf8[0] == 0x08) && (pBuf8[1] == 0x06))  /* (0806-arp; 0800-upd; 0835-rarp) */
			{
				/* logMsg("\n\n<<<< ARP Rx_mLen=%d \n", pMblk->mBlkHdr.mLen, 2,3,4,5,6); */
				logMsg("\n\n---- ARP Rx_mLen=%d \n", pMblk->mBlkHdr.mLen, 2,3,4,5,6);
				
				pBuf8 = (UINT32 *)(&pMblk->mBlkHdr.mData[0]);
				logMsg("%02X:%02X:%02X-%02X:%02X:%02X \n", pBuf8[0], pBuf8[1], pBuf8[2], pBuf8[3], pBuf8[4], pBuf8[5]);
				logMsg("%02X:%02X:%02X-%02X:%02X:%02X \n", pBuf8[6], pBuf8[7], pBuf8[8], pBuf8[9], pBuf8[10], pBuf8[11]);
				
				pBuf32 = (UINT32 *)(&pMblk->mBlkHdr.mData[12]);
				for (i=0; i<2; i++)
				{
					logMsg("%08X \n", pBuf32[i], 2,3,4,5,6);
				}	
				
				pBuf8 = (UINT32 *)(&pMblk->mBlkHdr.mData[20]);
				logMsg("%02X%02X \n", pBuf8[0], pBuf8[1],3,4,5,6);
				
				pBuf8 = (UINT32 *)(&pMblk->mBlkHdr.mData[22]);
				logMsg("%02X:%02X:%02X-%02X:%02X:%02X \n", pBuf8[0], pBuf8[1], pBuf8[2], pBuf8[3], pBuf8[4], pBuf8[5]);
				
				pBuf8 = (UINT32 *)(&pMblk->mBlkHdr.mData[28]);
				logMsg("%d.%d.%d.%d \n", pBuf8[0], pBuf8[1], pBuf8[2], pBuf8[3], 5, 6);
				
				pBuf8 = (UINT32 *)(&pMblk->mBlkHdr.mData[32]);
				logMsg("%02X:%02X:%02X-%02X:%02X:%02X \n", pBuf8[0], pBuf8[1], pBuf8[2], pBuf8[3], pBuf8[4], pBuf8[5]);
				
				pBuf8 = (UINT32 *)(&pMblk->mBlkHdr.mData[38]);
				logMsg("%d.%d.%d.%d \n", pBuf8[0], pBuf8[1], pBuf8[2], pBuf8[3], 5, 6);
				
				pBuf32 = (UINT32 *)(&pMblk->mBlkHdr.mData[42]);
				for (i=0; i<(pMblk->mBlkHdr.mLen-42)/4; i++)
				{
					logMsg("%08X \n", pBuf32[i], 2,3,4,5,6);
				}	
			}
			else if ((pBuf8[0] == 0x08) && (pBuf8[1] == 0x00))	/* (0806-arp; 0800-upd; 0835-rarp) */
			{
				/*
				0x9cbb7e0 (tNet0): 00:A0:1E-87:90:78 
				0x9cbb7e0 (tNet0): 00:04:9F-A1:AA:07 
				0x9cbb7e0 (tNet0): 0800  (0806-arp; 0800-upd; 0835-rarp)
								   4500 003C
								   918D 0000 
								   4011 59C2
								   C0.A8.7.7
								   C0.A8.7.0A							   
								   CB67 1F90  (52071 - 8080)
								   0028 9443							   
				0x9cbb7e0 (tNet0): 00010203 
				0x9cbb7e0 (tNet0): 04050607 
				0x9cbb7e0 (tNet0): 08090A0B 
				0x9cbb7e0 (tNet0): 0C0D0E0F 
				0x9cbb7e0 (tNet0): 10111213 
				0x9cbb7e0 (tNet0): 14151617 
				0x9cbb7e0 (tNet0): 18191A1B 
				0x9cbb7e0 (tNet0): 1C1D1E1F
				*/
				logMsg("\n\n---- UDP Rx_mLen=%d \n", pMblk->mBlkHdr.mLen, 2,3,4,5,6);
				
				pBuf8 = (UINT32 *)(&pMblk->mBlkHdr.mData[0]);
				logMsg("%02X:%02X:%02X-%02X:%02X:%02X \n", pBuf8[0], pBuf8[1], pBuf8[2], pBuf8[3], pBuf8[4], pBuf8[5]);
				logMsg("%02X:%02X:%02X-%02X:%02X:%02X \n", pBuf8[6], pBuf8[7], pBuf8[8], pBuf8[9], pBuf8[10], pBuf8[11]);
				
				pBuf16 = (UINT16 *)(&pMblk->mBlkHdr.mData[12]);
				logMsg("%04X \n", pBuf16[0], 2,3,4,5,6);
				
				pBuf32 = (UINT32 *)(&pMblk->mBlkHdr.mData[14]);
				for (i=0; i<3; i++)
				{
					logMsg("%08X \n", pBuf32[i], 2,3,4,5,6);
				}	
				
				pBuf8 = (UINT8 *)(&pMblk->mBlkHdr.mData[26]);
				logMsg("%d.%d.%d.%d \n", pBuf8[0], pBuf8[1], pBuf8[2], pBuf8[3], 5, 6);
				
				pBuf8 = (UINT8 *)(&pMblk->mBlkHdr.mData[30]);
				logMsg("%d.%d.%d.%d \n", pBuf8[0], pBuf8[1], pBuf8[2], pBuf8[3], 5, 6);
				
				pBuf16 = (UINT16 *)(&pMblk->mBlkHdr.mData[34]);
				logMsg("(port)%d-%d \n", pBuf16[0], pBuf16[1], 3,4,5,6);
				
				pBuf32 = (UINT32 *)(&pMblk->mBlkHdr.mData[38]);
				logMsg("%08X \n", pBuf32[0], 2,3,4,5,6);
				
				/*-------------------------------------------- */
				pBuf32 = (UINT32 *)(&pMblk->mBlkHdr.mData[42]);
				for (i=0; i<(pMblk->mBlkHdr.mLen-42)/4; i++)
				{
					logMsg("%08X \n", pBuf32[i], 2,3,4,5,6);
				}	
			}
			else if ((pBuf8[0] == 0x08) && (pBuf8[1] == 0x00))	/* (0806-arp; 0800-upd/tcp/icmp/igmp; 0835-rarp) */
			{
				/*
				0x9cbb7e0 (tNet0): 00:A0:1E-87:90:78 
				0x9cbb7e0 (tNet0): 00:04:9F-A1:AA:07 
				0x9cbb7e0 (tNet0): 0800  (0806-arp; 0800-upd; 0835-rarp)
								   4500 003C
								   918D 0000 
								   4011 59C2
								   C0.A8.7.7
								   C0.A8.7.0A							   
								   CB67 1F90  (52071 - 8080)
								   0028 9443							   
				0x9cbb7e0 (tNet0): 00010203 
				0x9cbb7e0 (tNet0): 04050607 
				0x9cbb7e0 (tNet0): 08090A0B 
				0x9cbb7e0 (tNet0): 0C0D0E0F 
				0x9cbb7e0 (tNet0): 10111213 
				0x9cbb7e0 (tNet0): 14151617 
				0x9cbb7e0 (tNet0): 18191A1B 
				0x9cbb7e0 (tNet0): 1C1D1E1F
				*/
				pBuf8 = (UINT32 *)(&pMblk->mBlkHdr.mData[23]);
				if (pBuf8[0] == 17)  /* udp: ICMP锛�1锛夈�両GMP锛�2锛夈�乀CP锛�6锛夈�乁DP锛�17锛� */
				{
					logMsg("\n\n---- UDP Rx_mLen=%d \n", pMblk->mBlkHdr.mLen, 2,3,4,5,6);
					
					pBuf8 = (UINT32 *)(&pMblk->mBlkHdr.mData[0]);
					logMsg("%02X:%02X:%02X-%02X:%02X:%02X \n", pBuf8[0], pBuf8[1], pBuf8[2], pBuf8[3], pBuf8[4], pBuf8[5]);
					logMsg("%02X:%02X:%02X-%02X:%02X:%02X \n", pBuf8[6], pBuf8[7], pBuf8[8], pBuf8[9], pBuf8[10], pBuf8[11]);
					
					pBuf16 = (UINT16 *)(&pMblk->mBlkHdr.mData[12]);
					logMsg("%04X \n", pBuf16[0], 2,3,4,5,6);
					
					pBuf32 = (UINT32 *)(&pMblk->mBlkHdr.mData[14]);
					for (i=0; i<3; i++)
					{
						logMsg("%08X \n", pBuf32[i], 2,3,4,5,6);
					}	
					
					pBuf8 = (UINT8 *)(&pMblk->mBlkHdr.mData[26]);
					logMsg("%d.%d.%d.%d \n", pBuf8[0], pBuf8[1], pBuf8[2], pBuf8[3], 5, 6);
					
					pBuf8 = (UINT8 *)(&pMblk->mBlkHdr.mData[30]);
					logMsg("%d.%d.%d.%d \n", pBuf8[0], pBuf8[1], pBuf8[2], pBuf8[3], 5, 6);
					
					pBuf16 = (UINT16 *)(&pMblk->mBlkHdr.mData[34]);
					logMsg("(port)%d-%d \n", pBuf16[0], pBuf16[1], 3,4,5,6);
					
					pBuf32 = (UINT32 *)(&pMblk->mBlkHdr.mData[38]);
					logMsg("%08X \n", pBuf32[0], 2,3,4,5,6);
	
					/*-------------------------------------------- */
					pBuf32 = (UINT32 *)(&pMblk->mBlkHdr.mData[42]);
					for (i=0; i<(pMblk->mBlkHdr.mLen-42)/4; i++)
					{
						logMsg("%08X \n", pBuf32[i], 2,3,4,5,6);
					}	
				}
				else if (pBuf8[0] == 6)  /* tcp: ICMP锛�1锛夈�両GMP锛�2锛夈�乀CP锛�6锛夈�乁DP锛�17锛� */
				{
					logMsg("\n\n---- TCP Rx_mLen=%d \n", pMblk->mBlkHdr.mLen, 2,3,4,5,6);
					
					pBuf8 = (UINT32 *)(&pMblk->mBlkHdr.mData[0]);
					logMsg("%02X:%02X:%02X-%02X:%02X:%02X \n", pBuf8[0], pBuf8[1], pBuf8[2], pBuf8[3], pBuf8[4], pBuf8[5]);
					logMsg("%02X:%02X:%02X-%02X:%02X:%02X \n", pBuf8[6], pBuf8[7], pBuf8[8], pBuf8[9], pBuf8[10], pBuf8[11]);
					
					pBuf16 = (UINT16 *)(&pMblk->mBlkHdr.mData[12]);
					logMsg("%04X \n", pBuf16[0], 2,3,4,5,6);
					
					pBuf32 = (UINT32 *)(&pMblk->mBlkHdr.mData[14]);
					for (i=0; i<3; i++)
					{
						logMsg("%08X \n", pBuf32[i], 2,3,4,5,6);
					}	
					
					pBuf8 = (UINT8 *)(&pMblk->mBlkHdr.mData[26]);
					logMsg("%d.%d.%d.%d \n", pBuf8[0], pBuf8[1], pBuf8[2], pBuf8[3], 5, 6);
					
					pBuf8 = (UINT8 *)(&pMblk->mBlkHdr.mData[30]);
					logMsg("%d.%d.%d.%d \n", pBuf8[0], pBuf8[1], pBuf8[2], pBuf8[3], 5, 6);
					
					pBuf16 = (UINT16 *)(&pMblk->mBlkHdr.mData[34]);
					logMsg("(port)%d-%d \n", pBuf16[0], pBuf16[1], 3,4,5,6);
					
					pBuf32 = (UINT32 *)(&pMblk->mBlkHdr.mData[38]);
					logMsg("%08X \n", pBuf32[0], 2,3,4,5,6);
					
					pBuf32 = (UINT32 *)(&pMblk->mBlkHdr.mData[42]);
					for (i=0; i<(pMblk->mBlkHdr.mLen-42)/4; i++)
					{
						logMsg("%08X \n", pBuf32[i], 2,3,4,5,6);
					}	
				}
			}
			else
			{
			  /* other package			  */
				logMsg("\n\n---- Other Rx_mLen=%d \n", pMblk->mBlkHdr.mLen, 2,3,4,5,6);
				
				pBuf8 = (UINT32 *)(&pMblk->mBlkHdr.mData[0]);
				logMsg("%02X:%02X:%02X-%02X:%02X:%02X \n", pBuf8[0], pBuf8[1], pBuf8[2], pBuf8[3], pBuf8[4], pBuf8[5]);
				logMsg("%02X:%02X:%02X-%02X:%02X:%02X \n", pBuf8[6], pBuf8[7], pBuf8[8], pBuf8[9], pBuf8[10], pBuf8[11]);
				
				pBuf16 = (UINT16 *)(&pMblk->mBlkHdr.mData[12]);
				logMsg("%04X \n", pBuf16[0], 2,3,4,5,6);
				
				pBuf32 = (UINT32 *)(&pMblk->mBlkHdr.mData[14]);
				for (i=0; i<3; i++)
				{
					logMsg("%08X \n", pBuf32[i], 2,3,4,5,6);
				}	
				
				pBuf8 = (UINT8 *)(&pMblk->mBlkHdr.mData[26]);
				logMsg("%d.%d.%d.%d \n", pBuf8[0], pBuf8[1], pBuf8[2], pBuf8[3], 5, 6);
				
				pBuf8 = (UINT8 *)(&pMblk->mBlkHdr.mData[30]);
				logMsg("%d.%d.%d.%d \n", pBuf8[0], pBuf8[1], pBuf8[2], pBuf8[3], 5, 6);
				
				pBuf16 = (UINT16 *)(&pMblk->mBlkHdr.mData[34]);
				logMsg("(port)%d-%d \n", pBuf16[0], pBuf16[1], 3,4,5,6);
				
				pBuf32 = (UINT32 *)(&pMblk->mBlkHdr.mData[38]);
				logMsg("%08X \n", pBuf32[0], 2,3,4,5,6);
				
				/*-------------------------------------------- */
				pBuf32 = (UINT32 *)(&pMblk->mBlkHdr.mData[42]);
				for (i=0; i<(pMblk->mBlkHdr.mLen-42)/4; i++)
				{
					logMsg("%08X \n", pBuf32[i], 2,3,4,5,6);
				}	
			}
		}
#endif  /* #ifdef DBG_ARP_MSG */

#ifdef DBG_MULTICAST_MSG
			/* if (pDev->unitNumber != 3) // not front-eth */
			{
				pBuf8 = (UINT32 *)(&pMblk->mBlkHdr.mData[12]);
				if ((pBuf8[0] == 0x08) && (pBuf8[1] == 0x06))  /* (0806-arp; 0800-upd; 0835-rarp) */
				{
					/* do nothing*/
				}
				else if ((pBuf8[0] == 0x08) && (pBuf8[1] == 0x00))	/* (0806-arp; 0800-upd; 0835-rarp) */
				{
					/* do nothing*/
				}
				else if ((pBuf8[0] == 0x08) && (pBuf8[1] == 0x00))	/* (0806-arp; 0800-upd/tcp/icmp/igmp; 0835-rarp) */
				{
					/* do nothing*/
				}
				else
				{
				  /* other package			  */
					logMsg("\n\n---- Other Rx_mLen=%d \n", pMblk->mBlkHdr.mLen, 2,3,4,5,6);
					
					pBuf8 = (UINT32 *)(&pMblk->mBlkHdr.mData[0]);
					logMsg("%02X:%02X:%02X-%02X:%02X:%02X \n", pBuf8[0], pBuf8[1], pBuf8[2], pBuf8[3], pBuf8[4], pBuf8[5]);
					logMsg("%02X:%02X:%02X-%02X:%02X:%02X \n", pBuf8[6], pBuf8[7], pBuf8[8], pBuf8[9], pBuf8[10], pBuf8[11]);
					
					pBuf16 = (UINT16 *)(&pMblk->mBlkHdr.mData[12]);
					logMsg("%04X \n", pBuf16[0], 2,3,4,5,6);
					
					pBuf32 = (UINT32 *)(&pMblk->mBlkHdr.mData[14]);
					for (i=0; i<3; i++)
					{
						logMsg("%08X \n", pBuf32[i], 2,3,4,5,6);
					}	
					
					pBuf8 = (UINT8 *)(&pMblk->mBlkHdr.mData[26]);
					logMsg("%d.%d.%d.%d \n", pBuf8[0], pBuf8[1], pBuf8[2], pBuf8[3], 5, 6);
					
					pBuf8 = (UINT8 *)(&pMblk->mBlkHdr.mData[30]);
					logMsg("%d.%d.%d.%d \n", pBuf8[0], pBuf8[1], pBuf8[2], pBuf8[3], 5, 6);
					
					pBuf16 = (UINT16 *)(&pMblk->mBlkHdr.mData[34]);
					logMsg("(port)%d-%d \n", pBuf16[0], pBuf16[1], 3,4,5,6);
					
					pBuf32 = (UINT32 *)(&pMblk->mBlkHdr.mData[38]);
					logMsg("%08X \n", pBuf32[0], 2,3,4,5,6);
					
					/*-------------------------------------------- */
					pBuf32 = (UINT32 *)(&pMblk->mBlkHdr.mData[42]);
					for (i=0; i<(pMblk->mBlkHdr.mLen-42)/4; i++)
					{
						logMsg("%08X \n", pBuf32[i], 2,3,4,5,6);
					}	
				}
			}
#endif /* #ifdef DBG_MULTICAST_MSG */

    vxAtomicSet((atomic_t*)&pDrvCtrl->emacRxPending, FALSE);
}

/*******************************************************************************
*
* altSocGen5DwEndTxHandle - process TX completion events
*
* This function is scheduled by the ISR to run in the context of tNetTask
* whenever an TX interrupt is received. It runs through all of the TX register
* pairs and checks the TX status to see how many have completed. For each
* completed transmission, the associated TX mBlk is released, and the outbound
* packet stats are updated.
*
* In the event that a TX underrun error is detected, the TX FIFO threshold is
* increased. This will continue until the maximum TX FIFO threshold is reached.
*
* If the transmitter has stalled, this routine will also call muxTxRestart()
* to drain any packets that may be waiting in the protocol send queues.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

LOCAL void altSocGen5DwEndTxHandle
    (
    EMAC_DRV_CTRL * pDrvCtrl
    )
    {
    VXB_DEVICE_ID pDev;
    BOOL restart = FALSE;
    DMA_DESC * pDesc;
    M_BLK_ID pMblk;
    pDev = pDrvCtrl->emacDev;

    END_TX_SEM_TAKE(&pDrvCtrl->emacEndObj, WAIT_FOREVER);

    while (pDrvCtrl->emacTxFree < EMAC_TX_DESC_CNT)
    {
        pDesc = &pDrvCtrl->emacTxDescMem[pDrvCtrl->emacTxCons];

        if (SWAP32(pDesc->desc0) & TDESC0_OWN_BY_DMA)
        {
            break;
        }

        if (SWAP32(pDesc->desc0) & TDESC0_TX_UNDERFLOW)
        {
            pDrvCtrl->emacOutErrors++;
        }

        /* reset tx buffer pointer to NULL */
        pDesc->desc2 = (UINT32)NULL;

        pMblk = pDrvCtrl->emacTxMblk[pDrvCtrl->emacTxCons];
        if (pMblk != NULL)
        {
            pDrvCtrl->emacOutOctets += pMblk->m_pkthdr.len;
			emacTxCounterUpdate (pDrvCtrl, pMblk);
			/*
                      
			if (((UINT8) pMblk->m_data[0] == 0xFF) && 
				((UINT8) pMblk->m_data[1] == 0xFF) && 
				((UINT8) pMblk->m_data[2] == 0xFF) && 
				((UINT8) pMblk->m_data[3] == 0xFF) && 
				((UINT8) pMblk->m_data[4] == 0xFF) && 
				((UINT8) pMblk->m_data[5] == 0xFF) )
            {
                pDrvCtrl->emacOutBcasts++;
            }
            else if ((UINT8) pMblk->m_data[0] & 0x1)
            {
                pDrvCtrl->emacOutMcasts++;
            }
            else
            {
                pDrvCtrl->emacOutUcasts++;
            }*/

            endPoolTupleFree(pMblk);
            pDrvCtrl->emacTxMblk[pDrvCtrl->emacTxCons] = NULL;
        }

        pDrvCtrl->emacTxFree++;

        EMAC_INC_DESC(pDrvCtrl->emacTxCons, EMAC_TX_DESC_CNT);

        /*
         * We released at least one descriptor: if the transmit
         * channel is stalled, unstall it.
         */
        if (pDrvCtrl->emacTxStall == TRUE)
        {
            pDrvCtrl->emacTxStall = FALSE;
            restart = TRUE;
        }
    }

    END_TX_SEM_GIVE (&pDrvCtrl->emacEndObj);

    vxAtomicSet ((atomic_t*)&pDrvCtrl->emacTxPending, FALSE);
	
    if (restart == TRUE)
        muxTxRestart (pDrvCtrl);

	return;
}

/*******************************************************************************
*
* altSocGen5DwEndEncap - encapsulate an outbound packet in the TX DMA ring
*
* This function sets up a descriptor for a packet transmit operation. With the
* EMAC ethernet controller, the TX DMA ring consists of descriptors that each
* describe a single packet fragment.  We consume as many descriptors as there
* are mBlks in the outgoing packet, unless the chain is too long.  The length
* is limited by the number of DMA segments we want to allow in a given DMA map.
* If there are too many segments, this routine will fail, and the caller must
* coalesce the data into fewer buffers and try again.
*
* This routine will also fail if there aren't enough free descriptors available
* in the ring, in which case the caller must defer the transmission until more
* descriptors are completed by the chip.
*
* RETURNS: ENOSPC if there are too many fragments in the packet, EAGAIN
* if the DMA ring is full, otherwise OK.
*
* ERRNO: N/A
*/

LOCAL INT32 altSocGen5DwEndEncap
    (
    EMAC_DRV_CTRL *  pDrvCtrl,
    M_BLK_ID        pMblk
    )
    {
    VXB_DEVICE_ID pDev;
    DMA_DESC * pDesc = NULL, * pFirstDesc, *pLastDesc;
    UINT32 firstIdx, lastIdx = 0;
    UINT32 nFrags;
    M_BLK * pTmp;
	
	/* UINT32 * pBuf32 = NULL; */

    pDev = pDrvCtrl->emacDev;
    firstIdx = pDrvCtrl->emacTxProd;

    if (pDrvCtrl->emacTxMblk [pDrvCtrl->emacTxProd] != NULL)
        return (EAGAIN);

    /*
     * Load the DMA map to build the segment list.
     * This will fail if there are too many segments.
     */
    nFrags = 0;
    for (pTmp = pMblk; pTmp != NULL; pTmp = pTmp->m_next)
        {
        if (pTmp->m_len != 0)
            ++nFrags;
        }

    if (nFrags > pDrvCtrl->emacTxFree || nFrags >= EMAC_MAXFRAG)
        {
        return ENOSPC;
        }

    pFirstDesc = &pDrvCtrl->emacTxDescMem[pDrvCtrl->emacTxProd];

    for (pTmp = pMblk; pTmp != NULL; pTmp = pTmp->m_next)
    {
        pDesc = &pDrvCtrl->emacTxDescMem[pDrvCtrl->emacTxProd];

        /* set up the transmit descriptor */
        /* pDesc->desc0 = SWAP32(DESC_TX_INT_ENABLE | DESC_TX_CHAIN); */
        pDesc->desc0 = 0;
		
        /* set the length 1 field  */
        /* pDesc->desc1 = SWAP32(pTmp->m_len & DESC_SIZE1_MASK); */
        pDesc->desc1 = SWAP32(pTmp->m_len & TDESC1_SIZE1_MASK);
		pDesc->desc1 |= (TDESC1_TX_CHAIN);  /* jc add */

       /* cacheFlush (DATA_CACHE, pTmp->m_data, pTmp->m_len);*/
		
        /* set the transmit buffer */
        pDesc->desc2 = SWAP32((UINT32)pTmp->m_data);

        /* set owner to DMA chip except first one */
        if(pDesc != pFirstDesc)
        {
            pDesc->desc0 |= SWAP32(TDESC0_OWN_BY_DMA);
        }

        cacheFlush (DATA_CACHE, pTmp->m_data, pTmp->m_len); 

        pDrvCtrl->emacTxFree--;
        lastIdx = pDrvCtrl->emacTxProd;
		
        EMAC_INC_DESC (pDrvCtrl->emacTxProd, EMAC_TX_DESC_CNT);
    }

    /* add flag for first & last TxBD */
    pLastDesc = &pDrvCtrl->emacTxDescMem [lastIdx];

    /* Interrupt on completition only for the latest segment */
    /* pLastDesc->desc0  |= SWAP32(DESC_TX_LAST | DESC_TX_INT_ENABLE); */
    pLastDesc->desc1  |= SWAP32 (TDESC1_TX_LAST | TDESC1_TX_INT_ENABLE); 
	
	/* 
	first 
	*/
    /* pFirstDesc->desc0 |= SWAP32(DESC_TX_FIRST); */
    pFirstDesc->desc1 |= SWAP32 (TDESC1_TX_FIRST);  /* Bit29 FS: First Segment */

    /* save the mBlk for later */
    pDrvCtrl->emacTxMblk [lastIdx] = pMblk;

    /* To avoid raise condition */
    pFirstDesc->desc0 |= SWAP32(TDESC0_OWN_BY_DMA);

   cachePipeFlush ();  /* */

    /* If the transmitter is not enabled, enable it */
    EMAC_SETBIT_4(pDev, DMA_CONTROL, DMA_TX_START);
	
	/* DBG_LOG_MSG("pFirstDesc->desc0~3: 0x%08X-0x%08X-0x%08X-0x%08X \n", pFirstDesc->desc0, pFirstDesc->desc1, pFirstDesc->desc2, pFirstDesc->desc3, 5,6); */
	/* pBuf32 = (UINT32 *)(pDrvCtrl->emacBar + 0x1048); */
	/* DBG_LOG_MSG("Reg18~21: 0x%08X-0x%08X-0x%08X-0x%08X \n\n", pBuf32[0], pBuf32[1], pBuf32[2], pBuf32[3], 5,6); */

    /* enable DMA transfer.*/
    EMAC_WRITE_REG (pDev, DMA_TX_POLL_DEMAND, 1);

    return (OK);
}

/*******************************************************************************
*
* altSocGen5DwEndSend - transmit a packet
*
* This function transmits the packet specified in <pMblk>.
*
* RETURNS: OK, ERROR, or END_ERR_BLOCK.
*
* ERRNO: N/A
*/

LOCAL INT32 altSocGen5DwEndSend
    (
    END_OBJ * pEnd,     /* pointer to the end block */
    M_BLK_ID pMblk      /* memory block */
    )
    {
    EMAC_DRV_CTRL * pDrvCtrl;
    /* VXB_DEVICE_ID pDev; */
    M_BLK_ID pTmp;
    INT32 rval;

    pDrvCtrl = (EMAC_DRV_CTRL *) pEnd;

    if (pDrvCtrl->emacPolling == TRUE)
        {
        endPoolTupleFree (pMblk);
        return (ERROR);
        }

    /* pDev = pDrvCtrl->emacDev; */

    END_TX_SEM_TAKE (pEnd, WAIT_FOREVER);

    if (!pDrvCtrl->emacTxFree || !(pDrvCtrl->emacCurStatus & IFM_ACTIVE))
        {
        goto blocked;
        }

    /* first, try to do an in-place transmission, using gather-write DMA */
    rval = altSocGen5DwEndEncap (pDrvCtrl, pMblk);

#ifdef DBG_ARP_MSG /* add by jc for test 2018-03-07 */
		UINT8 * pBuf8;
		UINT16 * pBuf16;
		UINT32 * pBuf32;
		int i = 0;
		
		/* if (pDev->unitNumber != 3)	 */
		{
			pBuf8 = (UINT32 *)(&pMblk->mBlkHdr.mData[12]);
			if ((pBuf8[0] == 0x08) && (pBuf8[1] == 0x06))  /* (0806-arp; 0800-upd; 0835-rarp) */
			{
				/* logMsg("\n\n>>>> ARP Tx_mLen=%d \n", pMblk->mBlkHdr.mLen, 2,3,4,5,6); */
				logMsg("\n\n==== ARP Tx_mLen=%d \n", pMblk->mBlkHdr.mLen, 2,3,4,5,6);
	
				pBuf8 = (UINT32 *)(&pMblk->mBlkHdr.mData[0]);		
				logMsg("%02X:%02X:%02X-%02X:%02X:%02X \n", pBuf8[0], pBuf8[1], pBuf8[2], pBuf8[3], pBuf8[4], pBuf8[5]);
				logMsg("%02X:%02X:%02X-%02X:%02X:%02X \n", pBuf8[6], pBuf8[7], pBuf8[8], pBuf8[9], pBuf8[10], pBuf8[11]);
				
				pBuf32 = (UINT32 *)(&pMblk->mBlkHdr.mData[12]);
				/* for (i=0; i<(pMblk->mBlkHdr.mLen-12)/4; i++) */
				for (i=0; i<2; i++)
				{
					logMsg("%08X \n", pBuf32[i], 2,3,4,5,6);
				}	
				
				pBuf8 = (UINT32 *)(&pMblk->mBlkHdr.mData[20]);
				logMsg("%02X%02X \n", pBuf8[0], pBuf8[1],3,4,5,6);
				
				pBuf8 = (UINT32 *)(&pMblk->mBlkHdr.mData[22]);
				logMsg("%02X:%02X:%02X-%02X:%02X:%02X \n", pBuf8[0], pBuf8[1], pBuf8[2], pBuf8[3], pBuf8[4], pBuf8[5]);
				
				pBuf8 = (UINT32 *)(&pMblk->mBlkHdr.mData[28]);
				logMsg("%d.%d.%d.%d \n", pBuf8[0], pBuf8[1], pBuf8[2], pBuf8[3], 5, 6);
				
				pBuf8 = (UINT32 *)(&pMblk->mBlkHdr.mData[32]);
				logMsg("%02X:%02X:%02X-%02X:%02X:%02X \n", pBuf8[0], pBuf8[1], pBuf8[2], pBuf8[3], pBuf8[4], pBuf8[5]);
				
				pBuf8 = (UINT32 *)(&pMblk->mBlkHdr.mData[38]);
				logMsg("%d.%d.%d.%d \n", pBuf8[0], pBuf8[1], pBuf8[2], pBuf8[3], 5, 6);
				
				pBuf32 = (UINT32 *)(&pMblk->mBlkHdr.mData[42]);
				for (i=0; i<(pMblk->mBlkHdr.mLen-42)/4; i++)
				{
					logMsg("%08X \n", pBuf32[i], 2,3,4,5,6);
				}	
			}
			else if ((pBuf8[0] == 0x08) && (pBuf8[1] == 0x00))	/* (0806-arp; 0800-upd; 0835-rarp) */
			{
				/*
				0x9cbb7e0 (tNet0): 00:A0:1E-87:90:78 
				0x9cbb7e0 (tNet0): 00:04:9F-A1:AA:07 
				0x9cbb7e0 (tNet0): 0800  (0806-arp; 0800-upd; 0835-rarp)
								   4500 003C
								   918D 0000 
								   4011 59C2
								   C0.A8.7.7
								   C0.A8.7.0A							   
								   CB67 1F90  (52071 - 8080)
								   0028 9443							   
				0x9cbb7e0 (tNet0): 00010203 
				0x9cbb7e0 (tNet0): 04050607 
				0x9cbb7e0 (tNet0): 08090A0B 
				0x9cbb7e0 (tNet0): 0C0D0E0F 
				0x9cbb7e0 (tNet0): 10111213 
				0x9cbb7e0 (tNet0): 14151617 
				0x9cbb7e0 (tNet0): 18191A1B 
				0x9cbb7e0 (tNet0): 1C1D1E1F
				*/
				
				pBuf8 = (UINT32 *)(&pMblk->mBlkHdr.mData[23]);
				if (pBuf8[0] == 17)  /* udp: ICMP锛�1锛夈�両GMP锛�2锛夈�乀CP锛�6锛夈�乁DP锛�17锛� */
				{
					logMsg("\n\n==== UDP Tx_mLen=%d \n", pMblk->mBlkHdr.mLen, 2,3,4,5,6);
					
					pBuf8 = (UINT32 *)(&pMblk->mBlkHdr.mData[0]);
					logMsg("%02X:%02X:%02X-%02X:%02X:%02X \n", pBuf8[0], pBuf8[1], pBuf8[2], pBuf8[3], pBuf8[4], pBuf8[5]);
					logMsg("%02X:%02X:%02X-%02X:%02X:%02X \n", pBuf8[6], pBuf8[7], pBuf8[8], pBuf8[9], pBuf8[10], pBuf8[11]);
					
					pBuf16 = (UINT16 *)(&pMblk->mBlkHdr.mData[12]);
					logMsg("%04X \n", pBuf16[0], 2,3,4,5,6);
					
					pBuf32 = (UINT32 *)(&pMblk->mBlkHdr.mData[14]);
					for (i=0; i<3; i++)
					{
						logMsg("%08X \n", pBuf32[i], 2,3,4,5,6);
					}	
					
					pBuf8 = (UINT8 *)(&pMblk->mBlkHdr.mData[26]);
					logMsg("%d.%d.%d.%d \n", pBuf8[0], pBuf8[1], pBuf8[2], pBuf8[3], 5, 6);
					
					pBuf8 = (UINT8 *)(&pMblk->mBlkHdr.mData[30]);
					logMsg("%d.%d.%d.%d \n", pBuf8[0], pBuf8[1], pBuf8[2], pBuf8[3], 5, 6);
					
					pBuf16 = (UINT16 *)(&pMblk->mBlkHdr.mData[34]);
					logMsg("(port)%d-%d \n", pBuf16[0], pBuf16[1], 3,4,5,6);
					
					pBuf32 = (UINT32 *)(&pMblk->mBlkHdr.mData[38]);
					logMsg("%08X \n", pBuf32[0], 2,3,4,5,6);
					
					pBuf32 = (UINT32 *)(&pMblk->mBlkHdr.mData[42]);
					for (i=0; i<(pMblk->mBlkHdr.mLen-42)/4; i++)
					{
						logMsg("%08X \n", pBuf32[i], 2,3,4,5,6);
					}	
				}
				else if (pBuf8[0] == 6)  /*tcp: ICMP锛�1锛夈�両GMP锛�2锛夈�乀CP锛�6锛夈�乁DP锛�17锛� */
				{
					logMsg("\n\n==== TCP Tx_mLen=%d \n", pMblk->mBlkHdr.mLen, 2,3,4,5,6);
					
					pBuf8 = (UINT32 *)(&pMblk->mBlkHdr.mData[0]);
					logMsg("%02X:%02X:%02X-%02X:%02X:%02X \n", pBuf8[0], pBuf8[1], pBuf8[2], pBuf8[3], pBuf8[4], pBuf8[5]);
					logMsg("%02X:%02X:%02X-%02X:%02X:%02X \n", pBuf8[6], pBuf8[7], pBuf8[8], pBuf8[9], pBuf8[10], pBuf8[11]);
					
					pBuf16 = (UINT16 *)(&pMblk->mBlkHdr.mData[12]);
					logMsg("%04X \n", pBuf16[0], 2,3,4,5,6);
					
					pBuf32 = (UINT32 *)(&pMblk->mBlkHdr.mData[14]);
					for (i=0; i<3; i++)
					{
						logMsg("%08X \n", pBuf32[i], 2,3,4,5,6);
					}	
					
					pBuf8 = (UINT8 *)(&pMblk->mBlkHdr.mData[26]);
					logMsg("%d.%d.%d.%d \n", pBuf8[0], pBuf8[1], pBuf8[2], pBuf8[3], 5, 6);
					
					pBuf8 = (UINT8 *)(&pMblk->mBlkHdr.mData[30]);
					logMsg("%d.%d.%d.%d \n", pBuf8[0], pBuf8[1], pBuf8[2], pBuf8[3], 5, 6);
					
					pBuf16 = (UINT16 *)(&pMblk->mBlkHdr.mData[34]);
					logMsg("(port)%d-%d \n", pBuf16[0], pBuf16[1], 3,4,5,6);
					
					pBuf32 = (UINT32 *)(&pMblk->mBlkHdr.mData[38]);
					logMsg("%08X \n", pBuf32[0], 2,3,4,5,6);
					
					pBuf32 = (UINT32 *)(&pMblk->mBlkHdr.mData[42]);
					for (i=0; i<(pMblk->mBlkHdr.mLen-42)/4; i++)
					{
						logMsg("%08X \n", pBuf32[i], 2,3,4,5,6);
					}	
				}
			}
		}
#endif


    /*
     * If altSocGen5DwEndEncap() returns ENOSPC, it means it ran out of TX
     * descriptors and couldn't encapsulate the whole packet fragment chain.
     * In that case, we need to coalesce everything into a single buffer and
     * try again. If any other error is returned, then something went wrong,
     * we have to abort the transmission entirely.
     */
    if (rval == ENOSPC)
    {
        if ((pTmp = endPoolTupleGet (pDrvCtrl->emacEndObj.pNetPool)) == NULL)
            goto blocked;

        pTmp->m_len = pTmp->m_pkthdr.len =
                      netMblkToBufCopy (pMblk, mtod(pTmp, char *), NULL);
        pTmp->m_flags = pMblk->m_flags;

        /* try transmission again, should succeed this time */
        rval = altSocGen5DwEndEncap (pDrvCtrl, pTmp);
        if (rval == OK)
            endPoolTupleFree (pMblk);
        else
            endPoolTupleFree (pTmp);
    }

    if (rval != OK)
        goto blocked;

    END_TX_SEM_GIVE (pEnd);
    return (OK);

blocked:
    pDrvCtrl->emacTxStall = TRUE;
    END_TX_SEM_GIVE (pEnd);

    return (END_ERR_BLOCK);
    }

/*******************************************************************************
*
* altSocGen5DwEndPollSend - polled mode transmit routine
*
* This function is similar to the altSocGen5DwEndSend() routine shown above,
* except it performs transmissions synchronously with interrupts disabled.
* After the transmission is initiated, the routine will poll the state of the
* TX status register associated with the current slot until transmission
* completed. If transmission times out, this routine will return ERROR.
*
* RETURNS: OK, EAGAIN, or ERROR
*
* ERRNO: N/A
*/

LOCAL STATUS altSocGen5DwEndPollSend
    (
    END_OBJ * pEnd,     /* pointer to the end object */
    M_BLK_ID  pMblk     /* memory block */
    )
    {
    EMAC_DRV_CTRL * pDrvCtrl;
    /* VXB_DEVICE_ID pDev; */
    DMA_DESC * pDesc;
    M_BLK_ID pTmp;
    INT32 len, i;

    pDrvCtrl = (EMAC_DRV_CTRL *) pEnd;

    if (pDrvCtrl->emacPolling == FALSE)
        return (ERROR);

   /* pDev = pDrvCtrl->emacDev; */
    pTmp = pDrvCtrl->emacPollBuf;

    len = netMblkToBufCopy(pMblk, mtod(pTmp, char *), NULL);
    pTmp->m_len = pTmp->m_pkthdr.len = len;
    pTmp->m_flags = pMblk->m_flags;

    if (altSocGen5DwEndEncap (pDrvCtrl, pTmp) != OK)
        return (EAGAIN);

    pDesc = &pDrvCtrl->emacTxDescMem [pDrvCtrl->emacTxCons];
    for (i = 0; i < EMAC_TIMEOUT; i++)
        {
        if (!(SWAP32(pDesc->desc0) & TDESC0_OWN_BY_DMA))
            break;
        }

    pDrvCtrl->emacTxMblk [pDrvCtrl->emacTxCons] = NULL;

    /* Reset */
    pDrvCtrl->emacTxFree++;
    EMAC_INC_DESC (pDrvCtrl->emacTxCons, EMAC_TX_DESC_CNT);

    if (i == EMAC_TIMEOUT)
        {
        pDrvCtrl->emacOutErrors++;
        return (ERROR);
        }

    pDrvCtrl->emacOutOctets += pMblk->m_pkthdr.len;
	emacTxCounterUpdate (pDrvCtrl, pMblk);
	/*
    
	if (((UINT8) pMblk->m_data[0] == 0xFF) && 
		((UINT8) pMblk->m_data[1] == 0xFF) && 
		((UINT8) pMblk->m_data[2] == 0xFF) && 
		((UINT8) pMblk->m_data[3] == 0xFF) && 
		((UINT8) pMblk->m_data[4] == 0xFF) && 
		((UINT8) pMblk->m_data[5] == 0xFF) )
    {   
    	pDrvCtrl->emacOutBcasts++;
	}
    else if ((UINT8) pMblk->m_data [0] & 0x1)
    {    
    	pDrvCtrl->emacOutMcasts++;
	}
    else
    {    
    	pDrvCtrl->emacOutUcasts++;
	}*/

    return (OK);
}

/*******************************************************************************
*
* altSocGen5DwEndPollReceive - polled mode receive routine
*
* This function receive a packet in polled mode, with interrupts disabled.
* It's similar in operation to the altSocGen5DwEndRxHandle() routine, except it
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

LOCAL INT32 altSocGen5DwEndPollReceive
    (
    END_OBJ * pEnd,     /* pointer to the end object */
    M_BLK_ID  pMblk     /* memory block */
    )
    {
    EMAC_DRV_CTRL * pDrvCtrl;
    /* VXB_DEVICE_ID pDev; */
    DMA_DESC * pDesc;
    M_BLK_ID pPkt;
    INT32 frameLength = 0;
    UINT32 pktLen = 0;
    UINT32 timeout = 0;

    BOOL waitForLastFrame = FALSE;
    BOOL firstFrame = TRUE;

    pDrvCtrl = (EMAC_DRV_CTRL *) pEnd;
    if (pDrvCtrl->emacPolling == FALSE)
        return (ERROR);

    if (!(pMblk->m_flags & M_EXT))
        return (ERROR);

   /* pDev = pDrvCtrl->emacDev; */

    /* adjust the data start address in buffer */
    EMAC_BUF_ADJ (pMblk);
    pMblk->m_len = 0;
    pMblk->mBlkPktHdr.len = 0;

    while (firstFrame || waitForLastFrame)
    {
        firstFrame = FALSE;

        /* check for an incoming packet */
        pDesc = &pDrvCtrl->emacRxDescMem [pDrvCtrl->emacRxIdx];
        pPkt = pDrvCtrl->emacRxMblk[pDrvCtrl->emacRxIdx];

        if (waitForLastFrame)
        {
            /* waiting for the next descriptor */
            timeout = 0;
            while (timeout < EMAC_TIMEOUT)
                {
                if (!(SWAP32(pDesc->desc0) & RDESC0_OWN_BY_DMA))
                    break;
                timeout++;
                }

            /* if no more descriptor, stop processing */
            if (timeout == EMAC_TIMEOUT)
            {
                /* clean */
                pMblk->m_len = 0;
                pMblk->mBlkPktHdr.len = 0;
                break;
            }
        }
        else
        {
            /* if we do not own this descriptor, stop processing */
            if (SWAP32(pDesc->desc0) & RDESC0_OWN_BY_DMA)
                return (EAGAIN);
        }

        if ((SWAP32(pDesc->desc0) & (RDESC0_RX_FIRST | RDESC0_RX_LAST)) ==
                (RDESC0_RX_FIRST | RDESC0_RX_LAST))
        {
            waitForLastFrame = FALSE;

            /* single frame */
            if (SWAP32(pDesc->desc0) & RDESC0_ERROR)
            {
                /* reset */
                pMblk->mBlkHdr.mLen = 0;
                pMblk->mBlkPktHdr.len = 0;
				
                pDrvCtrl->emacInErrors++;				
				pDrvCtrl->emacInDiscards++;  /* jc */
            }
            else
            {
                frameLength = ((SWAP32(pDesc->desc0) & RDESC0_FRAME_LENGTH_MASK)
                    >> RDESC0_FRAME_LENGTH_SHIFT);

                /* set the mBlk header up with the frame length. */
                pMblk->mBlkHdr.mFlags |= (M_PKTHDR | M_EXT);
                pMblk->mBlkHdr.mLen = frameLength - ETHER_CRC_LEN;
                pMblk->mBlkPktHdr.len = frameLength - ETHER_CRC_LEN;

                /* Update statistics for counters */
                pDrvCtrl->emacInOctets += pMblk->m_len;				
			    emacRxCounterUpdate (pDrvCtrl, pMblk);
                /*
				if (((UINT8) pMblk->m_data[0] == 0xFF) && 
					((UINT8) pMblk->m_data[1] == 0xFF) && 
					((UINT8) pMblk->m_data[2] == 0xFF) && 
					((UINT8) pMblk->m_data[3] == 0xFF) && 
					((UINT8) pMblk->m_data[4] == 0xFF) && 
					((UINT8) pMblk->m_data[5] == 0xFF) )
                {
                    pDrvCtrl->emacInBcasts++;
                }
                else if ((UINT8) pMblk->m_data[0] & 0x1)
                {
                    pDrvCtrl->emacInMcasts++;
                }
                else
                {
                    pDrvCtrl->emacInUcasts++;
                }*/
                
                /* sync the DMA buffer */
                cacheInvalidate (DATA_CACHE, pPkt->m_data, pPkt->m_len);
                
				bcopy (mtod(pPkt, char *), mtod(pMblk, char *), pMblk->m_len);
            }
        }
        else if (SWAP32(pDesc->desc0) & RDESC0_RX_FIRST)
        {
            /* first frame */
            waitForLastFrame = TRUE;

            pktLen = (SWAP32(pDesc->desc1) & RDESC1_RX_RBS1SZ_MASK) >>
                      RDESC1_RX_RBS1SZ_SHIFT;

            /* sync the DMA buffer */
            cacheInvalidate (DATA_CACHE, pPkt->m_data, pPkt->m_len);

            bcopy (mtod(pPkt, char *), mtod(pMblk, char *), pktLen);

            pMblk->mBlkHdr.mFlags |= (M_PKTHDR | M_EXT);
            pMblk->m_len = pktLen;
            pMblk->mBlkPktHdr.len = pktLen;
        }
        else if (SWAP32(pDesc->desc0) & RDESC0_RX_LAST)
        {
            /* last frame */
            waitForLastFrame = FALSE;

            if ((pMblk->m_len == 0) || (SWAP32(pDesc->desc0) & RDESC0_ERROR))
            {
                /* reset */
                pMblk->m_len = 0;
                pMblk->mBlkPktHdr.len = 0;

				/* jc */
				pDrvCtrl->emacInErrors++;
				pDrvCtrl->emacInDiscards++;
            }
            else
            {
                frameLength = ((SWAP32(pDesc->desc0) & RDESC0_FRAME_LENGTH_MASK) >>
                               RDESC0_FRAME_LENGTH_SHIFT);

                pktLen = frameLength - pMblk->m_len;

                /* sync the DMA buffer */
                cacheInvalidate (DATA_CACHE, pPkt->m_data, pktLen);

                bcopy (mtod(pPkt, char *), mtod(pMblk, char *) + pMblk->m_len, pktLen);

                pMblk->m_len += pktLen - ETHER_CRC_LEN;
                pMblk->mBlkPktHdr.len += pktLen - ETHER_CRC_LEN;

                /* Update statistics for counters */
               /* pDrvCtrl->emacInOctets += pMblk->m_len;	 */
				pDrvCtrl->emacInOctets += pMblk->mBlkPktHdr.len;  /* jc */
                emacRxCounterUpdate (pDrvCtrl, pMblk);    
				/*
                if (((UINT8) pMblk->m_data[0] == 0xFF) && 
					((UINT8) pMblk->m_data[1] == 0xFF) && 
					((UINT8) pMblk->m_data[2] == 0xFF) && 
					((UINT8) pMblk->m_data[3] == 0xFF) && 
					((UINT8) pMblk->m_data[4] == 0xFF) && 
					((UINT8) pMblk->m_data[5] == 0xFF) )
                {
                    pDrvCtrl->emacInBcasts++;
                }
                else if ((UINT8) pMblk->m_data[0] & 0x1)
                {
                    pDrvCtrl->emacInMcasts++;
                }
                else
                {
                    pDrvCtrl->emacInUcasts++;
                }*/
            }
        }
        else
        {
            /* middle frame*/
            waitForLastFrame = TRUE;
            if (pMblk->m_len == 0)
            {
                /* reset */
                waitForLastFrame = FALSE;
                pMblk->mBlkPktHdr.len = 0;
            }
            else
            {
                pktLen = (SWAP32(pDesc->desc1) & RDESC1_RX_RBS1SZ_MASK) >> RDESC1_RX_RBS1SZ_SHIFT;

                /* sync the DMA buffer */
                cacheInvalidate (DATA_CACHE, pPkt->m_data, pPkt->m_len);

                bcopy (mtod(pPkt, char *), mtod(pMblk, char *) + pMblk->m_len, pktLen);

                pMblk->mBlkHdr.mFlags |= (M_PKTHDR | M_EXT);
                pMblk->m_len += pktLen;
                pMblk->mBlkPktHdr.len += pktLen;
            }
        }

        /* reset this descriptor's fields */
        pDesc->desc0 = 0;

        /* set the length 1 field, with the lowest two bits clear */
        pDesc->desc1 = SWAP32((((EMAC_RX_BUFF_SIZE-1) & RDESC1_SIZE1_MASK) & (~0x03)) | RDESC1_RX_CHAIN);

        /* give the descriptor to the DMA */
        pDesc->desc0 |= SWAP32(RDESC0_OWN_BY_DMA);

        /* advance to the next descriptor */
        EMAC_INC_DESC (pDrvCtrl->emacRxIdx, EMAC_RX_DESC_CNT);
    }

    return (OK);
}

/*******************************************************************************
* jc
* emacTxCounterUpdate - update the statistics counter for TX
*
* This function update the statistics counter for TX include boardcast,
* multicast, unicast.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/
LOCAL void emacTxCounterUpdate(EMAC_DRV_CTRL * pDrvCtrl,M_BLK_ID          pMblk)
{
    if (((UINT8) pMblk->m_data[0] == 0xFF) && 
        ((UINT8) pMblk->m_data[1] == 0xFF) && 
        ((UINT8) pMblk->m_data[2] == 0xFF) && 
        ((UINT8) pMblk->m_data[3] == 0xFF) && 
        ((UINT8) pMblk->m_data[4] == 0xFF) && 
        ((UINT8) pMblk->m_data[5] == 0xFF) )
    {
        pDrvCtrl->emacOutBcasts++;
    }
    else if ((UINT8) pMblk->m_data [0] & 0x1)
    {
        pDrvCtrl->emacOutMcasts++;
    }
    else
    {
        pDrvCtrl->emacOutUcasts++;
    }
}

/*******************************************************************************
* jc
* emacRxCounterUpdate - update the statistics counter for RX
*
* This function update the statistics counter for RX include boardcast,
* multicast, unicast.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/
LOCAL void emacRxCounterUpdate(EMAC_DRV_CTRL * pDrvCtrl, M_BLK_ID pMblk)
{
    if (((UINT8) pMblk->m_data[0] == 0xFF) && 
        ((UINT8) pMblk->m_data[1] == 0xFF) && 
        ((UINT8) pMblk->m_data[2] == 0xFF) && 
        ((UINT8) pMblk->m_data[3] == 0xFF) && 
        ((UINT8) pMblk->m_data[4] == 0xFF) && 
        ((UINT8) pMblk->m_data[5] == 0xFF) )
    {
        pDrvCtrl->emacInBcasts++;
    }
    else if ((UINT8) pMblk->m_data[0] & 0x1)
    {
        pDrvCtrl->emacInMcasts++;
    }
    else
    {
        pDrvCtrl->emacInUcasts++;
    }
}


/******************************************************************************
*
* mdioBusyWait - wait when mido was busy.
*
* This routine reads the status of mdio, if mdio was busy then wait until a max
* value limit was reached.
*
* RETURNS: the times of while was executed.
*
* ERRNO: N/A
*/

LOCAL UINT32 mdioBusyWait
    (
    VXB_DEVICE_ID pDev,
    UINT32 limit
    )
    {
    volatile UINT32 k = 0;

    while ((EMAC_READ_REG (pDev, EMAC_MII_ADDR) & EMAC_MII_ADDR_BUSY)
        && (k < limit))
        {
        k++;
        }

    return (k);
    }

/*******************************************************************************
*
* mdioRegDataMake - construct a register for use with R4 -  GMII Address
* Register.
*
* RETURNS: the register data
*
* ERRNO: N/A
*/

LOCAL UINT32 mdioRegDataMake
    (
    UINT32 phyAddr,      /* The physical address */
    UINT32 gmiiReg,      /* The GMII register to access */
    UINT32 writeMask     /* A mask to indicate reading or writing */
    )
{
    return (UINT32)
      ( ((phyAddr << EMAC_MII_ADDR_PHY_ADDR_SHIFT) & EMAC_MII_ADDR_PHY_ADDR_MASK)
      | ((gmiiReg << EMAC_MII_ADDR_REG_SHIFT) & EMAC_MII_ADDR_REG_MASK)
      | GMII_MII_CLK_RNG_250_300_MHZ /* GMII_MII_CLK_RNG_60_100_MHZ; GMII_MII_CLK_RNG_150_250_MHZ */
      | writeMask
      | EMAC_MII_ADDR_BUSY
      );
 }

	
/*
multi-cast or broad-cast
*/
#if 1

/*
default: multi-cast is disable
-------
enable: 
	0 - disable, 1- enable
*/
UINT32 Gmac_RxFilt_En_MultiCast(int gmac_x, int enable)
{
	VXB_DEVICE_ID pDev;
	
	int offset = 0;
	UINT32 tmp32 = 0;
	
	pDev = vxbInstByNameFind (DW_EMAC_NAME, gmac_x); 
	if (NULL == pDev)
    {   
        printf("Gmac_RxFilt_EnFwAllMultiCast->vxbInstByNameFind: __LINE__=%d fail! \n", __LINE__);
		return ERROR;
    }
	
	offset = EMAC_FRAME_FILTER;
    tmp32 = EMAC_READ_REG(pDev, offset);
	
    if (enable)
    {
        tmp32 |= GMAC_MFF_PM;     /* set bit*/
    }
    else
    {
        tmp32 &= (~GMAC_MFF_PM);  /* clr bit*/
    }

	EMAC_WRITE_REG(pDev, offset, tmp32);

	return tmp32;	
}

/*
default: broad-cast is enable
-------
enable: 
	0 - disable  boradcast, 1- enable boradcast
*/
UINT32 Gmac_RxFilt_Dis_BroadCast(int gmac_x, int enable)
{
	VXB_DEVICE_ID pDev;
	
	int offset = 0;
	UINT32 tmp32 = 0;
	
	pDev = vxbInstByNameFind (DW_EMAC_NAME, gmac_x); 
	if (NULL == pDev)
    {   
        printf("Gmac_RxFilt_EnFwAllMultiCast->vxbInstByNameFind: __LINE__=%d fail! \n", __LINE__);
		return ERROR;
    }
	
	offset = EMAC_FRAME_FILTER;
    tmp32 = EMAC_READ_REG(pDev, offset);
	
    if (enable)
    {
		tmp32 &= (~GMAC_MFF_DBF);  /* clr bit*/
    }
    else
    {
		tmp32 |= GMAC_MFF_DBF;	   /* set bit*/
    }
	
	EMAC_WRITE_REG(pDev, offset, tmp32);
    return tmp32;
}

#endif



#if 1  /* jc for test gamc ctrl_reg*/

UINT32 gmac_reg_show(UINT32 offset)
{
	VXB_DEVICE_ID pDev;
	int unit = 0;
	UINT32 tmp32 = 0;
	
	pDev = vxbInstByNameFind ("gem", unit);  /* ETH_DRV_NAME*/
	if (NULL == pDev)
    {   
        printf("gmac_reg_show->vxbInstByNameFind: __LINE__=%d fail! \n", __LINE__);
		return ERROR;
    }

    /* reset the device */
    tmp32 = EMAC_READ_REG(pDev, offset);  /* DMA_BUS_MODE*/

	printf("gmac_reg<0x%X> = 0x%08X \n", offset, tmp32);

	return tmp32;	
}

UINT32 gmac0_read(UINT32 offset)
{
	VXB_DEVICE_ID pDev;
	int unit = 0;
	UINT32 tmp32 = 0;
	
	pDev = vxbInstByNameFind ("gem", unit);  /* ETH_DRV_NAME*/
	if (NULL == pDev)
    {   
        printf("gmac_reg_show->vxbInstByNameFind: __LINE__=%d fail! \n", __LINE__);
		return ERROR;
    }

    tmp32 = EMAC_READ_REG(pDev, offset); 
	return tmp32;	
}

UINT32 gmac1_read(UINT32 offset)
{
	VXB_DEVICE_ID pDev;
	int unit = 1;
	UINT32 tmp32 = 0;
	
	pDev = vxbInstByNameFind ("gem", unit);  /* ETH_DRV_NAME*/
	if (NULL == pDev)
    {   
        printf("gmac_reg_show->vxbInstByNameFind: __LINE__=%d fail! \n", __LINE__);
		return ERROR;
    }

    tmp32 = EMAC_READ_REG(pDev, offset); 
	return tmp32;	
}

UINT32 gmac0_write(UINT32 offset, UINT32 val)
{
	VXB_DEVICE_ID pDev;
	int unit = 0;
	UINT32 tmp32 = 0;
	
	pDev = vxbInstByNameFind ("gem", unit);  /* ETH_DRV_NAME*/
	if (NULL == pDev)
    {   
        printf("gmac_reg_show->vxbInstByNameFind: __LINE__=%d fail! \n", __LINE__);
		return ERROR;
    }

    /* reset the device */
    EMAC_WRITE_REG(pDev, offset, val);
    tmp32 = EMAC_READ_REG(pDev, offset);

	/*printf("gmac_reg<0x%X> = 0x%08X \n", offset, tmp32);*/

	return tmp32;	
}

#endif




#if 1

void gmac0_info_show(void)
{
	UINT32 tmp32 = 0;
	
	/*
	BOOTMODE	0x404	reserved	31:7	rw	0x0	Reserved. Writes are ignored, read data is zero.
			"VMODE_MIO	(persistant reg)"	8:7	rw	0x0	"
												VMODE[1]: bank 1 VMODE cfg
												VMODE[0]: bank 0 VMODE cfg	MIO VMODE实际控制寄存器。
														0: 2.5/3.3v
														1: 1.8v"
			VMODE	6:5	ro	0x0	"
								VMODE[1]: bank 1 VMODE cfg
								VMODE[0]: bank 0 VMODE cfg
								Boot Mode Strap Pin采样值，只读
								0: 2.5/3.3v
								1: 1.8v"
								
			PLL_BYPASS	4	ro	0x0	Boot mode pins are sampled when Power-on Reset deasserts. 
								The logic levels are stored in this register. 
								The PLL_BYPASS pin sets the initial operating mode 
								of all three PLL clocks (ARM, IO and DDR):
							0: PLLs are enabled and their outputs are routed to the clock generators
							1: PLLs are disabled and bypassed
							
			BOOT_MODE	3:0	ro	0x0	Boot mode pins are sampled when Power-on Reset deasserts. 
			                        The logic levels are stored in this register. The interpretation of these boot mode values 
			                        are explained in the boot mode section of the Zynq Technical Reference Manual.
	*/	
	printf("\n");
	
	tmp32 = slcr_read(0x404);
	printf("slcr-0x404: 0x%X \n", tmp32);
	
	/* bit8:7*/
	if (tmp32 & 0x100)  /* bit8*/
	{
		printf(" VMODE_MIO[1]: bank 1 => 1: 1.8v \n");
	}
	else
	{
		printf(" VMODE_MIO[1]: bank 1 => 0: 2.5/3.3v \n");
	}	
	if (tmp32 & 0x80)  /* bit7*/
	{
		printf(" VMODE_MIO[0]: bank 0 => 1: 1.8v \n");
	}
	else
	{
		printf(" VMODE_MIO[0]: bank 0 => 0: 2.5/3.3v \n");
	}

	/* bit6:5*/
	if (tmp32 & 0x40)	/* bit6*/
	{
		printf(" Boot Mode Strap Pin, VMODE[1]: bank 1 => 1: 1.8v \n");
	}
	else
	{
		printf(" Boot Mode Strap Pin, VMODE[1]: bank 1 => 0: 2.5/3.3v \n");
	}	
	if (tmp32 & 0x20)  /* bit5*/
	{
		printf(" Boot Mode Strap Pin, VMODE[0]: bank 0 => 1: 1.8v \n");
	}
	else
	{
		printf(" Boot Mode Strap Pin, VMODE[0]: bank 0 => 0: 2.5/3.3v \n");
	}
	printf("\n");
	
	/*
	GEM0_CLK_CTRL	0x268	reserved	31:15	rw	0x0 Reserved. 写无效，读为0
			GEM0_TX_DIVISOR 14:9	rw	0x32	GMAC0 Tx 时钟分频系数。1/10/50
			GEM0_TX_SRCSEL	8:6 rw	0x0 "GMAC0 Tx时钟源选择：
										0x0：IO PLL
										0x1：ARM PLL
										1xx:   EMIO"
			GEM0_RX_SRCSEL	5	rw	0x0 "GMAC0 Rx时钟源选择：
										0：MIO
										1：EMIO"
			GEM0_AHB_CLKACT 4	rw	0x1 "GMAC0 AHB时钟使能
										0：不使能
										1：使能"
			GEM0_AXI_CLKACT 3	rw	0x1 "GMAC0 AXI时钟使能
										0：不使能
										1：使能"
			reserved	2	rw	0x0 Reserved. 写无效，读为0
			GEM0_RX_CLKACT	1	rw	0x1 "GMAC0 Rx时钟使能
										0：不使能
										1：使能"
			GEM0_TX_CLKACT	0	rw	0x1 "GMAC0 Tx时钟使能
										0：不使能
										1：使能"
	*/
	tmp32 = slcr_read(0x268);
	printf("slcr-0x268: 0x%X \n", tmp32);
	
	printf(" GEM0_TX_DIVISOR: %d \n", ((tmp32 & 0x7E00) >> 9));

	if (((tmp32 & 0x1C0) >> 6) == 0x0)
	{
		printf(" GEM0_TX_SRCSEL: 0x0-IO PLL \n");
	}
	else if (((tmp32 & 0x1C0) >> 6) == 0x1)
	{
		printf(" GEM0_TX_SRCSEL: 0x1-ARM PLL \n");
	}
	else
	{
		printf(" GEM0_TX_SRCSEL: 1xx- EMIO \n");
	}
	printf("\n");

	/*
	GMAC_CRTL	0x414	reserved	31:7	rw	0x0 Reserved. Writes are ignored, read data is zero.
			GMAC_PHY_INTF_SEL2	6:4 rw	0x0 "PHY Interface Select
											■ 000: GMII or MII 
											■ 001: RGMII 
											■ 010: SGMII 
											■ 011: TBI 
											■ 100: RMII 
											■ 101: RTBI 
											■ 110: SMII 
											■ 111: RevMII  
											Active State: N/A"
			reserved	3	rw	0x0 Reserved. Writes are ignored, read data is zero.
			GMAC_PHY_INTF_SEL1	2:0 rw	0x0 "PHY Interface Select
											■ 000: GMII or MII 
											■ 001: RGMII 
											■ 010: SGMII 
											■ 011: TBI 
											■ 100: RMII 
											■ 101: RTBI 
											■ 110: SMII 
											■ 111: RevMII  
											Active State: N/A"
	*/
	tmp32 = slcr_read(0x414);
	printf("slcr-0x414: 0x%X \n", tmp32);
	switch ((tmp32 & 0x70) >> 4)
	{
	case 0:
		printf(" GMAC_PHY_INTF_SEL2-> 000: GMII or MII \n");
		break;
	case 1:
		printf(" GMAC_PHY_INTF_SEL2-> 001: RGMII \n");
		break;
	case 2:
		printf(" GMAC_PHY_INTF_SEL2-> 010: SGMII \n");
		break;
	case 4:
		printf(" GMAC_PHY_INTF_SEL2-> 100: RMII \n");
		break;
	}
	switch ((tmp32 & 0x07))
	{
	case 0:
		printf(" GMAC_PHY_INTF_SEL1-> 000: GMII or MII \n");
		break;
	case 1:
		printf(" GMAC_PHY_INTF_SEL1-> 001: RGMII \n");
		break;
	case 2:
		printf(" GMAC_PHY_INTF_SEL1-> 010: SGMII \n");
		break;
	case 4:
		printf(" GMAC_PHY_INTF_SEL1-> 100: RMII \n");
		break;
	}
	printf("\n");

	/*
	0x1058: Register 22 (HW Feature Register)
		30:28 ACTPHYIF Active or selected PHY interface
				When you have multiple PHY interfaces in your configuration, this field
				indicates the sampled value of phy_intf_sel_i during reset de-assertion.
				■ 0000: GMII or MII
				■ 0001: RGMII
				■ 0010: SGMII
				■ 0011: TBI
				■ 0100: RMII
				■ 0101: RTBI
				■ 0110: SMII
				■ 0111: RevMII
				■ All Others: Reserved
	*/
	tmp32 = gmac0_read(0x1058);
	printf("gmac0-0x1058: 0x%X \n", tmp32);
	
	switch ((tmp32 & 0x70000000) >> 28)
	{
	case 0:
		printf(" ACTPHYIF-> 0000: GMII or MII \n");
		break;
	case 1:
		printf(" ACTPHYIF-> 0001: RGMII \n");
		break;
	case 2:
		printf(" ACTPHYIF-> 0010: SGMII \n");
		break;
	case 4:
		printf(" ACTPHYIF-> 0100: RMII \n");
		break;
	}
	printf("\n");

	/*
	0x00D8: Register 54 (SGMII/RGMII/SMII Status Register)

	3 LNKSTS Link Status
		When set, this bit indicates that the link is up between the local PHY
		and the remote PHY. When cleared, this bit indicates that the link is
		down between the local PHY and the remote PHY.
		0 RO
	2:1 LNKSPEED Link Speed
		This bit indicates the current speed of the link:
		■ 00: 2.5 MHz
		■ 01: 25 MHz
		■ 10: 125 MHz
	0 LNKMOD Link Mode
		This bit indicates the current mode of operation of the link:
		■ 1’b0: Half-duplex mode
		■ 1’b1: Full-duplex mode
	*/
	tmp32 = gmac0_read(0x00D8);
	printf("gmac0-0x00D8: 0x%X (SGMII/RGMII/SMII Status Register) \n", tmp32);

	if (tmp32 & 0x08)
	{
		printf(" SGMII/RGMII/SMII->Link Status up! \n");
	}
	else
	{
		printf(" SGMII/RGMII/SMII->Link Status down! \n");
	}	
	switch ((tmp32 & 0x06) >> 1)
	{
	case 0:
		printf(" SGMII/RGMII/SMII->Link Speed 00: 2.5 MHz \n");
		break;
	case 1:
		printf(" SGMII/RGMII/SMII->Link Speed 01: 25 MHz \n");
		break;
	case 2:
		printf(" SGMII/RGMII/SMII->Link Speed 10: 125 MHz \n");
		break;
	}
	if (tmp32 & 0x01)
	{
		printf(" SGMII/RGMII/SMII->Link Mode 1: Full-duplex mode \n");
	}
	else
	{
		printf(" SGMII/RGMII/SMII->Link Mode 0: Half-duplex mode \n");
	}	
	printf("\n");

	
	/*
	0x00C4: Register 49 (AN Status Register)
	
	5 ANC Auto-Negotiation Complete
		When set, this bit indicates that the auto-negotiation process
		is complete.
		This bit is cleared when auto-negotiation is reinitiated.
	2 LS Link Status
		When set, this bit indicates that the link is up between the
		MAC and the TBI, RTBI, or SGMII interface. When cleared,
		this bit indicates that the link is down between the MAC and
		the TBI, RTBI, or SGMII interface.
	*/	
#if 0  /* while sgmii*/
	tmp32 = gmac0_read(0x00C4);
	printf("gmac0-0x00C4: 0x%X (AN Status Register) \n", tmp32);
	if (tmp32 & 0x20)
	{
		printf(" ANC Auto-Negotiation Complete! \n");
	}
	else
	{
		printf(" ANC Auto-Negotiation reinitiated \n");
	}	
	if (tmp32 & 0x04)
	{
		printf(" Link Status UP! \n");
	}
	else
	{
		printf(" Link Status DOWN! \n");
	}	
	printf("\n");
#endif
	
	return;
}

void gmac1_info_show(void)
{
	UINT32 tmp32 = 0;
	
	/*
	BOOTMODE	0x404	reserved	31:7	rw	0x0	Reserved. Writes are ignored, read data is zero.
			"VMODE_MIO	(persistant reg)"	8:7	rw	0x0	"
												VMODE[1]: bank 1 VMODE cfg
												VMODE[0]: bank 0 VMODE cfg	MIO VMODE实际控制寄存器。
														0: 2.5/3.3v
														1: 1.8v"
			VMODE	6:5	ro	0x0	"
								VMODE[1]: bank 1 VMODE cfg
								VMODE[0]: bank 0 VMODE cfg
								Boot Mode Strap Pin采样值，只读
								0: 2.5/3.3v
								1: 1.8v"
								
			PLL_BYPASS	4	ro	0x0	Boot mode pins are sampled when Power-on Reset deasserts. 
								The logic levels are stored in this register. 
								The PLL_BYPASS pin sets the initial operating mode 
								of all three PLL clocks (ARM, IO and DDR):
							0: PLLs are enabled and their outputs are routed to the clock generators
							1: PLLs are disabled and bypassed
							
			BOOT_MODE	3:0	ro	0x0	Boot mode pins are sampled when Power-on Reset deasserts. 
			                        The logic levels are stored in this register. The interpretation of these boot mode values 
			                        are explained in the boot mode section of the Zynq Technical Reference Manual.
	*/	
	printf("\n");
	
	tmp32 = slcr_read(0x404);
	printf("slcr-0x404: 0x%X \n", tmp32);
	
	/* bit8:7*/
	if (tmp32 & 0x100)  /* bit8*/
	{
		printf(" VMODE_MIO[1]: bank 1 => 1: 1.8v \n");
	}
	else
	{
		printf(" VMODE_MIO[1]: bank 1 => 0: 2.5/3.3v \n");
	}	
	if (tmp32 & 0x80)  /* bit7*/
	{
		printf(" VMODE_MIO[0]: bank 0 => 1: 1.8v \n");
	}
	else
	{
		printf(" VMODE_MIO[0]: bank 0 => 0: 2.5/3.3v \n");
	}

	/* bit6:5*/
	if (tmp32 & 0x40)	/* bit6*/
	{
		printf(" Boot Mode Strap Pin, VMODE[1]: bank 1 => 1: 1.8v \n");
	}
	else
	{
		printf(" Boot Mode Strap Pin, VMODE[1]: bank 1 => 0: 2.5/3.3v \n");
	}	
	if (tmp32 & 0x20)  /* bit5*/
	{
		printf(" Boot Mode Strap Pin, VMODE[0]: bank 0 => 1: 1.8v \n");
	}
	else
	{
		printf(" Boot Mode Strap Pin, VMODE[0]: bank 0 => 0: 2.5/3.3v \n");
	}
	printf("\n");
	
	/*
	GEM1_CLK_CTRL	0x26C	reserved	31:15	rw	0x0 Reserved. 写无效，读为0
			GEM1_TX_DIVISOR 14:9	rw	0x32	GMAC0 Tx 时钟分频系数。1/10/50
			GEM1_TX_SRCSEL	8:6 rw	0x0 "GMAC0 Tx时钟源选择：
										0x0：IO PLL
										0x1：ARM PLL
										1xx:   EMIO"
			GEM1_RX_SRCSEL	5	rw	0x0 "GMAC0 Rx时钟源选择：
										0：MIO
										1：EMIO"
			GEM1_AHB_CLKACT 4	rw	0x1 "GMAC0 AHB时钟使能
										0：不使能
										1：使能"
			GEM1_AXI_CLKACT 3	rw	0x1 "GMAC0 AXI时钟使能
										0：不使能
										1：使能"
			reserved	2	rw	0x0 Reserved. 写无效，读为0
			GEM1_RX_CLKACT	1	rw	0x1 "GMAC0 Rx时钟使能
										0：不使能
										1：使能"
			GEM1_TX_CLKACT	0	rw	0x1 "GMAC0 Tx时钟使能
										0：不使能
										1：使能"
	*/
	tmp32 = slcr_read(0x26C);
	printf("slcr-0x26C: 0x%X \n", tmp32);
	
	printf(" GEM1_TX_DIVISOR: %d \n", ((tmp32 & 0x7E00) >> 9));

	if (((tmp32 & 0x1C0) >> 6) == 0x0)
	{
		printf(" GEM1_TX_SRCSEL: 0x0-IO PLL \n");
	}
	else if (((tmp32 & 0x1C0) >> 6) == 0x1)
	{
		printf(" GEM1_TX_SRCSEL: 0x1-ARM PLL \n");
	}
	else
	{
		printf(" GEM1_TX_SRCSEL: 1xx- EMIO \n");
	}
	printf("\n");

	/*
	GMAC_CRTL	0x414	reserved	31:7	rw	0x0 Reserved. Writes are ignored, read data is zero.
			GMAC_PHY_INTF_SEL2	6:4 rw	0x0 "PHY Interface Select
											■ 000: GMII or MII 
											■ 001: RGMII 
											■ 010: SGMII 
											■ 011: TBI 
											■ 100: RMII 
											■ 101: RTBI 
											■ 110: SMII 
											■ 111: RevMII  
											Active State: N/A"
			reserved	3	rw	0x0 Reserved. Writes are ignored, read data is zero.
			GMAC_PHY_INTF_SEL1	2:0 rw	0x0 "PHY Interface Select
											■ 000: GMII or MII 
											■ 001: RGMII 
											■ 010: SGMII 
											■ 011: TBI 
											■ 100: RMII 
											■ 101: RTBI 
											■ 110: SMII 
											■ 111: RevMII  
											Active State: N/A"
	*/
	tmp32 = slcr_read(0x414);
	printf("slcr-0x414: 0x%X \n", tmp32);
	switch ((tmp32 & 0x70) >> 4)
	{
	case 0:
		printf(" GMAC_PHY_INTF_SEL2-> 000: GMII or MII \n");
		break;
	case 1:
		printf(" GMAC_PHY_INTF_SEL2-> 001: RGMII \n");
		break;
	case 2:
		printf(" GMAC_PHY_INTF_SEL2-> 010: SGMII \n");
		break;
	case 4:
		printf(" GMAC_PHY_INTF_SEL2-> 100: RMII \n");
		break;
	}
	switch ((tmp32 & 0x07))
	{
	case 0:
		printf(" GMAC_PHY_INTF_SEL1-> 000: GMII or MII \n");
		break;
	case 1:
		printf(" GMAC_PHY_INTF_SEL1-> 001: RGMII \n");
		break;
	case 2:
		printf(" GMAC_PHY_INTF_SEL1-> 010: SGMII \n");
		break;
	case 4:
		printf(" GMAC_PHY_INTF_SEL1-> 100: RMII \n");
		break;
	}
	printf("\n");

	/*
	0x1058: Register 22 (HW Feature Register)
		30:28 ACTPHYIF Active or selected PHY interface
				When you have multiple PHY interfaces in your configuration, this field
				indicates the sampled value of phy_intf_sel_i during reset de-assertion.
				■ 0000: GMII or MII
				■ 0001: RGMII
				■ 0010: SGMII
				■ 0011: TBI
				■ 0100: RMII
				■ 0101: RTBI
				■ 0110: SMII
				■ 0111: RevMII
				■ All Others: Reserved
	*/
	tmp32 = gmac1_read(0x1058);
	printf("gmac1-0x1058: 0x%X \n", tmp32);
	
	switch ((tmp32 & 0x70000000) >> 28)
	{
	case 0:
		printf(" ACTPHYIF-> 0000: GMII or MII \n");
		break;
	case 1:
		printf(" ACTPHYIF-> 0001: RGMII \n");
		break;
	case 2:
		printf(" ACTPHYIF-> 0010: SGMII \n");
		break;
	case 4:
		printf(" ACTPHYIF-> 0100: RMII \n");
		break;
	}
	printf("\n");

	/*
	0x00D8: Register 54 (SGMII/RGMII/SMII Status Register)

	3 LNKSTS Link Status
		When set, this bit indicates that the link is up between the local PHY
		and the remote PHY. When cleared, this bit indicates that the link is
		down between the local PHY and the remote PHY.
		0 RO
	2:1 LNKSPEED Link Speed
		This bit indicates the current speed of the link:
		■ 00: 2.5 MHz
		■ 01: 25 MHz
		■ 10: 125 MHz
	0 LNKMOD Link Mode
		This bit indicates the current mode of operation of the link:
		■ 1’b0: Half-duplex mode
		■ 1’b1: Full-duplex mode
	*/
	tmp32 = gmac1_read(0x00D8);
	printf("gmac1-0x00D8: 0x%X (SGMII/RGMII/SMII Status Register) \n", tmp32);

	if (tmp32 & 0x08)
	{
		printf(" SGMII/RGMII/SMII->Link Status up! \n");
	}
	else
	{
		printf(" SGMII/RGMII/SMII->Link Status down! \n");
	}	
	switch ((tmp32 & 0x06) >> 1)
	{
	case 0:
		printf(" SGMII/RGMII/SMII->Link Speed 00: 2.5 MHz \n");
		break;
	case 1:
		printf(" SGMII/RGMII/SMII->Link Speed 01: 25 MHz \n");
		break;
	case 2:
		printf(" SGMII/RGMII/SMII->Link Speed 10: 125 MHz \n");
		break;
	}
	if (tmp32 & 0x01)
	{
		printf(" SGMII/RGMII/SMII->Link Mode 1: Full-duplex mode \n");
	}
	else
	{
		printf(" SGMII/RGMII/SMII->Link Mode 0: Half-duplex mode \n");
	}	
	printf("\n");

	
	/*
	0x00C4: Register 49 (AN Status Register)
	
	5 ANC Auto-Negotiation Complete
		When set, this bit indicates that the auto-negotiation process
		is complete.
		This bit is cleared when auto-negotiation is reinitiated.
	2 LS Link Status
		When set, this bit indicates that the link is up between the
		MAC and the TBI, RTBI, or SGMII interface. When cleared,
		this bit indicates that the link is down between the MAC and
		the TBI, RTBI, or SGMII interface.
	*/	
#if 0  /* while sgmii*/
	tmp32 = gmac0_read(0x00C4);
	printf("gmac0-0x00C4: 0x%X (AN Status Register) \n", tmp32);
	if (tmp32 & 0x20)
	{
		printf(" ANC Auto-Negotiation Complete! \n");
	}
	else
	{
		printf(" ANC Auto-Negotiation reinitiated \n");
	}	
	if (tmp32 & 0x04)
	{
		printf(" Link Status UP! \n");
	}
	else
	{
		printf(" Link Status DOWN! \n");
	}	
	printf("\n");
#endif
	
	return;
}

#endif
