/* vxbAltSocGen5DwI2c.c -  VxBus I2C driver for DesignWare I2C Controller */
/* it support FMSH FMQL PSoc as VxBus I2C driver. */

/*
 * Copyright (c) 2013 Wind River Systems, Inc.
 *
 * The right to copy, distribute, modify or otherwise make use
 * of this software may be licensed only pursuant to the terms
 * of an applicable Wind River license agreement.
 */

/*
modification history
--------------------
26aug19,l_l  move it to support FMQL PSoc.
12dec13,swu  fix vxbAltSocGen5DwI2cUnLock description error. (WIND00447782)
01f,05sep13,xms  fix CHECKED_RETURN error. (WIND00414265)
01e,31jul13,swu  Removed unnecessary APIs. (WIND00428711)
01d,12jul13,swu  fixed problem when read length is larger than 32 (WIND00426017)
01c,08jun13,swu  removed unused semaphore
01b,07jun13,swu  corrected hcnt and lcnt calculation
01a,20mar13,swu  created
*/

/*
DESCRIPTION
This module implements a driver for the DesignWare I2C controller present on 
Altera Cyclone V processors. The controller is capable of acting as a master 
or a slave, and each I2C module instance has one completely independent channel.
Only master mode is supported by this driver.

Access to the I2C controller is required for certain devices such as EEPROMs
and RTCs.

EXTERNAL INTERFACE
The driver provides a vxBus external interface. The only exported routine is the
vxbdwI2cDrvRegister() function, which registers the driver with VxBus. Since the
I2Cs are processor local bus devices, each device instance must be specified in
the hwconf.c file in a BSP. The hwconf entry must specify the following
parameters:

\is
\i <regBase>
Specifies the base address where the controller's CSR registers are mapped
into the host's address space. All register offsets are computed relative
to this address.
\ie

\is
\i <clkFrequency>
Specifies the clock frequency.
\ie

\is
\i <busSpeed>
Specifies the I2C Bus Speed.
The I2C device have 2 speed mode:
standard speed mode:      100kbit/s
Fast     speed mdoe:      400kbit/s
If all the device connected on the bus can be accessed, the busSpeed should
be the minimum speed.

\is
\i <polling>
Specifies the I2C Bus work mode (polling or interrupt).
\ie

\is
\i <i2cDev>
Specifies the device table on I2C Bus.
\ie

\is
\i <i2cDevNum>
Specifies the device table size.
\ie

This driver only support 7 bit addressing, 10 bit addressing is not supported.
\ie
SEE ALSO: vxBus
*/

/* includes */

#include <vxWorks.h>
#include <vxBusLib.h>
#include <string.h>
#include <stdio.h>
#include <semLib.h>
#include <vxbTimerLib.h>
#include <hwif/util/hwMemLib.h>
#include <hwif/vxbus/vxBus.h>
#include <hwif/vxbus/vxbPlbLib.h>
#include <hwif/vxbus/hwConf.h>
#include <hwif/vxbus/vxbI2cLib.h>
#include <spinLockLib.h>

#include "vxbAltSocGen5DwI2c.h"

/* defines */

/* #undef I2C_DBG_ON */
#define I2C_DBG_ON

#ifdef  I2C_DBG_ON
#define I2C_DBG_IRQ         0x00000001
#define I2C_DBG_RW          0x00000002
#define I2C_DBG_ERR         0x00000004
#define I2C_DBG_ALL         0xffffffff
#define I2C_DBG_OFF         0x00000000
LOCAL UINT32 i2cDbgMask = I2C_DBG_ALL;  /* I2C_DBG_OFF; */
IMPORT FUNCPTR _func_logMsg;

#define I2C_DBG(mask, string, a, b, c, d, e, f)         \
    if ((i2cDbgMask & mask) || (mask == I2C_DBG_ALL))   \
        if (_func_logMsg != NULL)                       \
            (* _func_logMsg)(string, a, b, c, d, e, f)
#else
#define I2C_DBG(mask, string, a, b, c, d, e, f)
#endif  /* I2C_DBG_ON */

#ifdef ARMBE8
#    define SWAP32 vxbSwap32
#    define SWAP16 vxbSwap16
#else
#    define SWAP32 
#    define SWAP16
#endif /* ARMBE8 */

#undef CSR_READ_4
#define CSR_READ_4(pDev, addr)                          \
    SWAP32(vxbRead32 (DW_I2C_HANDLE(pDev),              \
        (UINT32 *)((char *)DW_I2C_BAR(pDev) + addr)))

#undef CSR_WRITE_4
#define CSR_WRITE_4(pDev, addr, data)                   \
    vxbWrite32 (DW_I2C_HANDLE(pDev),                    \
        (UINT32 *)((char *)DW_I2C_BAR(pDev) + addr), SWAP32(data))

#undef CSR_READ_2
#define CSR_READ_2(pDev, addr)                          \
    SWAP16(vxbRead16 (DW_I2C_HANDLE(pDev),              \
        (UINT16 *)((char *)DW_I2C_BAR(pDev) + addr)))

#undef CSR_WRITE_2
#define CSR_WRITE_2(pDev, addr, data)                   \
    vxbWrite16 (DW_I2C_HANDLE(pDev),                    \
        (UINT16 *)((char *)DW_I2C_BAR(pDev) + addr), SWAP16(data))

#undef CSR_READ_1
#define CSR_READ_1(pDev, addr)                          \
    vxbRead8 (DW_I2C_HANDLE(pDev),                      \
        (UINT8 *)((char *)DW_I2C_BAR(pDev) + addr))

#undef CSR_WRITE_1
#define CSR_WRITE_1(pDev, addr, data)                   \
    vxbWrite8 (DW_I2C_HANDLE(pDev),                     \
        (UINT8 *)((char *)DW_I2C_BAR(pDev) + addr), data)

/* forward declarations */

void vxbAltSocGen5DwI2cRegister (void);
LOCAL void vxbAltSocGen5DwI2cInstInit (VXB_DEVICE_ID);
LOCAL void vxbAltSocGen5DwI2cInstInit2 (VXB_DEVICE_ID);
LOCAL VXB_I2C_BUS_CTRL * vxbAltSocGen5DwI2cCtrlGet (VXB_DEVICE_ID pDev);
LOCAL void vxbAltSocGen5DwI2cInstConnect (VXB_DEVICE_ID pDev);
LOCAL STATUS vxbAltSocGen5DwI2cInstUnlink (VXB_DEVICE_ID pDev, void * unused);
LOCAL STATUS vxbAltSocGen5DwI2cIsr (VXB_DEVICE_ID pDev);
LOCAL STATUS vxbAltSocGen5DwI2cLock (VXB_DEVICE_ID pDev);
LOCAL STATUS vxbAltSocGen5DwI2cUnLock (VXB_DEVICE_ID pDev);
LOCAL STATUS vxbAltSocGen5DwI2cWaitBusFree (VXB_DEVICE_ID pDev);
LOCAL STATUS vxbAltSocGen5DwI2cWaitStatus (VXB_DEVICE_ID pDev, UINT32 staMsk);
STATUS vxbAltSocGen5DwI2cShow (VXB_DEVICE_ID pDevice, int verbose);

/* structure to store the driver functions for vxBus */

LOCAL struct drvBusFuncs dwI2cDrvFuncs =
    {
    vxbAltSocGen5DwI2cInstInit,      /* devInstanceInit */
    vxbAltSocGen5DwI2cInstInit2,     /* devInstanceInit2 */
    vxbAltSocGen5DwI2cInstConnect    /* devConnect */
    };

LOCAL device_method_t dwI2cDrv_methods[] =
    {
    DEVMETHOD (vxbI2cControlGet, vxbAltSocGen5DwI2cCtrlGet),  
    DEVMETHOD (busDevShow,       vxbAltSocGen5DwI2cShow),
    DEVMETHOD (vxbDrvUnlink,     vxbAltSocGen5DwI2cInstUnlink),
    DEVMETHOD_END
    };

/* I2C VxBus registration info */
LOCAL struct vxbDevRegInfo dwI2cDrvRegistration =
{
    NULL,                       /* pNext */
    VXB_DEVID_DEVICE,           /* devID */
    VXB_BUSID_PLB,              /* busID = PLB */
    VXB_VER_4_0_0,              /* busVer */
    DW_I2C_DRIVER_NAME,         /* drvName */
    &dwI2cDrvFuncs,             /* pDrvBusFuncs */
    &dwI2cDrv_methods[0],       /* pMethods */
    NULL                        /* devProbe */
};
	
LOCAL struct vxbI2cBusCtrl dwI2cCtrl = 
{
    NULL,                       /* vxbI2cLock  */
    NULL,                       /* vxbI2cStart */
    NULL,                       /* vxbI2cRead  */
    NULL,                       /* vxbI2cWrite */
    NULL,                       /* vxbI2cStop; 		start I2C bus */
    NULL,                       /* vxbI2cUnLock;	I2C unlock by a devcie */
    vxbAltSocGen5DwI2cRead,     /* vxbI2cDevRead;   I2C device read */
    vxbAltSocGen5DwI2cWrite,    /* vxbI2cDevWrite; 	I2C device write */
                                /* vxbI2cDevXfer;	I2C device transfer */
};

/*****************************************************************************
*
* vxbAltSocGen5DwI2cInit - initialize the I2C module
*
* This routine initializes the I2C module
*
* RETURNS: OK, or ERROR if fail
*
* ERRNO: N/A
*/

LOCAL STATUS vxbAltSocGen5DwI2cInit
    (
    VXB_DEVICE_ID   pDev
    )
    {
    I2C_DRV_CTRL * pDrvCtrl;

    UINT16 sclHi = 0;
    UINT16 sclLo = 0;
    UINT32 clkFreq = 0;

    VXB_ASSERT (pDev != NULL, ERROR)
    
    pDrvCtrl = pDev->pDrvCtrl;
    
    /* disable I2C Controller */    
    CSR_WRITE_2 (pDev, I2C_ENABLE, 0x0);

    CSR_WRITE_2 (pDev, I2C_CON, 0x0);

    /* setting SCL clock */    
    clkFreq = pDrvCtrl->clkFrequency / 1000000;
    
    if (pDrvCtrl->busSpeed == I2C_FAST_SPEED)
        {
        sclHi = ((UINT16) ((FS_MIN_SCL_HIGH * clkFreq) / 100));
        sclLo = ((UINT16) ((FS_MIN_SCL_LOW * clkFreq) / 100));
        
        /* check if sclHi and sclLo need round up */
        
        if ((sclHi % 10) != 0)
            sclHi = (sclHi / 10) + 1;
        else             
            sclHi = sclHi / 10;
        
        if ((sclLo % 10) != 0)
            sclLo = (sclLo / 10) + 1;
        else
            sclLo = sclLo / 10;

        CSR_WRITE_2 (pDev, I2C_FS_SCL_HCNT, sclHi);
        CSR_WRITE_2 (pDev, I2C_FS_SCL_LCNT, sclLo);

        CSR_WRITE_2 (pDev, I2C_CON, I2C_CTR_DEFAULT | I2C_CON_MS_FS);
        }
    else if (pDrvCtrl->busSpeed == I2C_STANDARD_SPEED)
        {
        sclHi = ((UINT16) ((SS_MIN_SCL_HIGH * clkFreq) / 100));
        sclLo = ((UINT16) ((SS_MIN_SCL_LOW * clkFreq) / 100));
        
        /* check if sclHi and sclLo need round up */
        
        if ((sclHi % 10) != 0)
            sclHi = (sclHi / 10) + 1;
        else             
            sclHi = sclHi / 10;
        
        if ((sclLo % 10) != 0)
            sclLo = (sclLo / 10) + 1;
        else
            sclLo = sclLo / 10;
            
        CSR_WRITE_2 (pDev, I2C_SS_SCL_HCNT, sclHi);
        CSR_WRITE_2 (pDev, I2C_SS_SCL_LCNT, sclLo);

        CSR_WRITE_2 (pDev, I2C_CON, I2C_CTR_DEFAULT | I2C_CON_MS_SS);
        }

    /* write to the TAR register */

    CSR_WRITE_2 (pDev, I2C_TAR, 0x55 );

    /* clear Interrupt Status Register */

    CSR_WRITE_2 (pDev, I2C_INTR_STAT, 
                 CSR_READ_2 (pDev, I2C_INTR_STAT));

    CSR_READ_2 (pDev, I2C_CLR_INTR);
    CSR_READ_2 (pDev, I2C_CLR_RX_UNDER);
    CSR_READ_2 (pDev, I2C_CLR_RX_OVER);
    CSR_READ_2 (pDev, I2C_CLR_TX_OVER);
    CSR_READ_2 (pDev, I2C_CLR_RD_REQ);
    CSR_READ_2 (pDev, I2C_CLR_TX_ABRT);
    CSR_READ_2 (pDev, I2C_CLR_RX_DONE);
    CSR_READ_2 (pDev, I2C_CLR_ACTIVITY);
    CSR_READ_2 (pDev, I2C_CLR_STOP_DET);
    CSR_READ_2 (pDev, I2C_CLR_START_DET);
    CSR_READ_2 (pDev, I2C_CLR_GEN_CALL);

    if (pDrvCtrl->polling == FALSE)
        {
        CSR_WRITE_2 (pDev, I2C_INTR_MASK, I2C_IRQ_NONE);
        }
    else
        {

        /* disable all bits in IRQ enable register */

        CSR_WRITE_2 (pDev, I2C_INTR_MASK, I2C_IRQ_DEFAULT_MASK & 
                                          (~I2C_INTR_TX_EMPTY));
        }

    return OK;
    }

/******************************************************************************
*
* vxbdwI2cDrvRegister - register DW I2c driver
*
* This routine registers the Altera I2c driver with the vxBus subsystem.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

void vxbAltSocGen5DwI2cRegister (void)
{    
    /* call the vxBus routine to register the I2c driver */    
    vxbDevRegister (&dwI2cDrvRegistration);
 }

void vxbAltSocGen5DwI2cRegister2 (void)
{    
    /* call the vxBus routine to register the I2c driver */    
    vxbDevRegister (&dwI2cDrvRegistration);
}

/*******************************************************************************
*
* vxbAltSocGen5DwI2cInstInit - first level initialization routine of I2c device
*
* This routine performs the first level initialization of the I2c device.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

LOCAL void vxbAltSocGen5DwI2cInstInit
    (
    VXB_DEVICE_ID pDev
    )
    {
    I2C_DRV_CTRL * pDrvCtrl;
    struct hcfDevice * pHcf;
    
    VXB_ASSERT (pDev != NULL, ERROR)
    
    /* create controller driver context structure for core */

    pDrvCtrl = (I2C_DRV_CTRL *) hwMemAlloc (sizeof (I2C_DRV_CTRL));
    
    if (pDrvCtrl == NULL)
        return;
    
    /* save instance ID */

    pDev->pDrvCtrl = pDrvCtrl;
    pDrvCtrl->i2cDev = pDev;
    pDrvCtrl->i2cDevSem = NULL;
    pDrvCtrl->polling = TRUE;
    
    /* override intCtlr device reg base from device scratch registers 0 */

    vxbRegMap (pDev, 0, &pDrvCtrl->i2cHandle);

    pHcf = (struct hcfDevice *) hcfDeviceGet (pDev);
    if (pHcf == NULL)
        return;

    /*
     * resourceDesc {
     * The clkFreq resource specifies clock frequency of I2C function clock. }
     */

    if (devResourceGet (pHcf, "clkFreq", HCF_RES_INT,
        (void *) &pDrvCtrl->clkFrequency) != OK)
        {
#ifndef _VXBUS_BASIC_HWMEMLIB
        hwMemFree ((char *) pDrvCtrl);
#endif /* _VXBUS_BASIC_HWMEMLIB */
        pDev->pDrvCtrl = NULL;
        return;
        }

    /*
     * resourceDesc {
     * The busSpeed resource specifies the BUS Speed. }
     */

    if (devResourceGet (pHcf, "busSpeed", HCF_RES_INT,
        (void *) &pDrvCtrl->busSpeed) != OK)
        {
        pDrvCtrl->busSpeed = 100000;
        }
       
    /* reset I2C Controller */
    
    vxbAltSocGen5DwI2cInit (pDev);
    
    /* announce that there's an I2C bus */

    (void) vxbBusAnnounce (pDev, VXB_BUSID_I2C);

    i2cBusAnnounceDevices (pDev);

    return;
    }


/*******************************************************************************
 *
 * vxbAltSocGen5DwI2cInstInit2 - second level initialization routine of I2c modules
 *
 * This routine performs the second level initialization of the I2c modules.
 *
 * This routine is called later during system initialization.  OS features
 * such as memory allocation are available at this time.
 *
 * RETURNS: N/A
 *
 * ERRNO: N/A
 */

LOCAL void vxbAltSocGen5DwI2cInstInit2
    (
    VXB_DEVICE_ID pDev    /* The device */
    )
    {
    
    I2C_DRV_CTRL * pDrvCtrl = NULL;
    struct hcfDevice * pHcf;
    
    VXB_ASSERT (pDev != NULL, ERROR)

    pDrvCtrl = (I2C_DRV_CTRL *) pDev->pDrvCtrl;
  
    pDrvCtrl->dataLength = 0;
    pDrvCtrl->dataBuf = NULL;
  
    pHcf = (struct hcfDevice *) hcfDeviceGet (pDev);
    if (pHcf == NULL)
        return;
        
    /*
      * resourceDesc {
      * The polling resource specifies whether
      * the driver uses polling mode or not.
      * If this property is not explicitly
      * specified, the driver uses interrupt
      * by default. }
      */

     if (devResourceGet (pHcf, "polling", HCF_RES_INT,
         (void *) &pDrvCtrl->polling) != OK)
         {
         pDrvCtrl->polling = FALSE;
         }

     pDrvCtrl->i2cDevSem = semMCreate (
         SEM_Q_PRIORITY | SEM_DELETE_SAFE | SEM_INVERSION_SAFE);

     if (pDrvCtrl->i2cDevSem == SEM_ID_NULL)
         {
         I2C_DBG (I2C_DBG_ERR,
                  "vxbAltSocGen5DwI2cInstInit2:i2cDevSem create failed\n", 
                  0, 0, 0, 0, 0, 0);
         return;
         }
         
    pDrvCtrl->i2cDataSem = semBCreate (SEM_Q_PRIORITY, SEM_EMPTY);
    if (pDrvCtrl->i2cDataSem == SEM_ID_NULL)
    {
        I2C_DBG (I2C_DBG_ERR,
                 "vxbAltSocGen5DwI2cInstInit2:i2cDataSem Ceate failed\n", 
                 0, 0, 0, 0, 0, 0);

        if (semDelete(pDrvCtrl->i2cDevSem) != OK)
            {
            I2C_DBG (I2C_DBG_ERR,
                     "vxbAltSocGen5DwI2cInstInit2:i2cDevSem delete failed\n",
                     0, 0, 0, 0, 0, 0);
            }

        return;
    }
        
     if (pDrvCtrl->polling == FALSE)
     {
         /* enable IRQ Mask */
         CSR_WRITE_2 (pDev, I2C_INTR_MASK, I2C_IRQ_DEFAULT_MASK & 
                                           (~I2C_INTR_TX_EMPTY));

         /* connect and enable interrupt */
         vxbIntConnect (pDev, 0, (VOIDFUNCPTR) vxbAltSocGen5DwI2cIsr, pDev);
         vxbIntEnable (pDev, 0, (VOIDFUNCPTR) vxbAltSocGen5DwI2cIsr, pDev);
     }
     else
     {
         /* disable IRQ Mask */
         CSR_WRITE_2 (pDev, I2C_INTR_MASK, I2C_IRQ_NONE);
     }

     return;
}

/*******************************************************************************
*
* vxbAltSocGen5DwI2cIsr - interrupt service routine
*
* This routine handles interrupts of I2C.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

LOCAL STATUS vxbAltSocGen5DwI2cIsr
    (
    VXB_DEVICE_ID   pDev
    )
    {
    UINT32 status = 0;
    UINT32 enabled = 0;
    UINT32 val = 0;
    I2C_DRV_CTRL * pDrvCtrl;

    UINT8 bytesToRx = CSR_READ_2 (pDev, I2C_RXFLR);

    pDrvCtrl = pDev->pDrvCtrl;

    /* check if i2c controller is enabled */

    enabled = CSR_READ_2 (pDev, I2C_ENABLE);

    if (enabled == 0)
        {
        I2C_DBG(I2C_DBG_IRQ, "Fatal error statue 0x%x \n ", 
                CSR_READ_2 (pDev, I2C_INTR_STAT), 0, 0, 0, 0, 0);

        CSR_WRITE_2 (pDev, I2C_ENABLE, 0);
        return ERROR;
        }

    /* check there is no interrupt */

    status = CSR_READ_2 (pDev, I2C_RAW_INTR_STAT);

    if (! (status & ~I2C_IRQ_ACTIVITY))
        {
        I2C_DBG(I2C_DBG_IRQ, "None IRQ\n", 0, 0, 0, 0, 0, 0);
        return ERROR;
        }

    /* read interrupt status  */

    status = CSR_READ_2 (pDev, I2C_INTR_STAT);

    if (status & I2C_IRQ_TX_EMPTY)
        {
        I2C_DBG(I2C_DBG_IRQ, "I2C_IRQ_TX_EMPTY\n", 0, 0, 0, 0, 0, 0);

        if (pDrvCtrl->dataLength)
            {
            pDrvCtrl->dataLength--;

            if ((pDrvCtrl->dataLength == 0) )
                {
                CSR_WRITE_2 (pDev, I2C_DATA_CMD, *(pDrvCtrl->dataBuf++) | 
                             I2C_DATA_CMD_WR_STOP_BIT);
                
                CSR_WRITE_2 (pDev, I2C_INTR_MASK, 
                             CSR_READ_2(pDev, I2C_INTR_MASK) & 
                             (~I2C_INTR_TX_EMPTY)); 
                }
            else
                CSR_WRITE_2 (pDev, I2C_DATA_CMD, *(pDrvCtrl->dataBuf++));

            }

        }

    /* Reading Process */
    
    if (status & I2C_IRQ_RX_FULL)
        {
        I2C_DBG(I2C_DBG_IRQ, "I2C_IRQ_RX_FULL\n", 
                0, 0, 0, 0, 0, 0);
                  
        bytesToRx = CSR_READ_2 (pDev, I2C_RXFLR);  
        for ( ; (pDrvCtrl->dataLength != 0) && (bytesToRx > 0); bytesToRx--)
            {
            pDrvCtrl->dataLength--;

            *(pDrvCtrl->dataBuf++) = CSR_READ_2 (pDev, I2C_DATA_CMD);

            /* read next byte */

            if (pDrvCtrl->dataLength > 1)
                CSR_WRITE_2 (pDev, I2C_DATA_CMD, 0x100);

            /* generate STOP at last byte */
						
            if (pDrvCtrl->dataLength == 1)
                CSR_WRITE_2 (pDev, I2C_DATA_CMD, I2C_DATA_CMD_RD_STOP_BIT);
                        
            }

        if (pDrvCtrl->dataLength == 0)
            {
            val = CSR_READ_2 (pDev, I2C_INTR_MASK) & 
                            ~(I2C_INTR_TX_EMPTY | I2C_INTR_RX_FULL);
            
            CSR_WRITE_2 (pDev, I2C_INTR_MASK, val);

            /* read completed */

            if (semGive (pDrvCtrl->i2cDataSem) == ERROR)
                {
                I2C_DBG (I2C_DBG_ERR,
                         "semGive error\n", 
                         0, 0, 0, 0, 0, 0);
                } 
            }
        }

    if (status & I2C_IRQ_STOP_DET)
        {
        I2C_DBG(I2C_DBG_IRQ, "I2C_IRQ_STOP_DET\n", 0, 0, 0, 0, 0, 0);

        /* clear interrupt bit */
        
        CSR_READ_2 (pDev, I2C_CLR_STOP_DET);

        }
    
    if (status & I2C_INTR_START_DET)
        {
        I2C_DBG(I2C_DBG_IRQ, "I2C_INTR_START_DET\n", 0, 0, 0, 0, 0, 0);

        /* clear interrupt bit */
            
        CSR_READ_2 (pDev, I2C_CLR_START_DET);
        }
    
    if (status & I2C_IRQ_RX_UNDER)
        {
        I2C_DBG(I2C_DBG_IRQ, "I2C_IRQ_RX_UNDER\n", 0, 0, 0, 0, 0, 0);

        /* clear interrupt bit */
        
        CSR_READ_2 (pDev, I2C_CLR_RX_UNDER);
        }

    if (status & I2C_IRQ_RX_OVER)
        {
        I2C_DBG(I2C_DBG_IRQ, "I2C_IRQ_RX_OVER\n", 0, 0, 0, 0, 0, 0);

        /* clear interrupt bit */
        
        CSR_READ_2 (pDev, I2C_CLR_RX_OVER);
        }
        
    if (status & I2C_IRQ_TX_ABRT)
        {
        I2C_DBG(I2C_DBG_IRQ, "I2C_IRQ_TX_ABRT 0x%x\n", 
                CSR_READ_2 (pDev, I2C_TX_ABRT_SOURCE), 0, 0, 0, 0, 0);
        
        /* clear interrupt bit */
        
        CSR_READ_2 (pDev, I2C_CLR_TX_ABRT);
        }
    
    return OK; 
    }

/*******************************************************************************
*
* vxbAltSocGen5DwI2cCtrlGet - get the callback functions
*
* This routine installs the callback functions to I2C libary.
*
* RETURNS: the address of callback functions
*
* ERRNO: N/A
*/

LOCAL VXB_I2C_BUS_CTRL * vxbAltSocGen5DwI2cCtrlGet
    (
    VXB_DEVICE_ID   pDev
    )
    {
    VXB_ASSERT (pDev != NULL, ERROR)

    return (&dwI2cCtrl);
    }

/*******************************************************************************
*
* vxbAltSocGen5DwI2cDelay - I2C delay.
*
* This routine performs some delay for I2C.
*
* RETURNS: N/A.
*/

LOCAL void vxbAltSocGen5DwI2cDelay(unsigned int delay)
    {
    vxbUsDelay (delay);
    }

/*******************************************************************************
*
* vxbAltSocGen5DwI2cInstConnect - third level initialization
*
* This routine performs the third level initialization of the i2c controller
* driver. Nothing to be done in this routine.
*
* RETURNS: N/A
*
* ERRNO : none
* \NOMANUAL
*/

LOCAL void vxbAltSocGen5DwI2cInstConnect
    (
    VXB_DEVICE_ID   pDev
    )
    {
    VXB_ASSERT (pDev != NULL, ERROR)

    return;
    }

/*******************************************************************************
*
* vxbAltSocGen5DwI2cInstUnlink - VxBus unlink handler
*
* This function shuts down a I2C controller instance in response to an
* an unlink event from VxBus. This may occur if our VxBus instance has
* been terminated, or if the FslI2c driver has been unloaded.
*
* RETURNS: OK if device was successfully destroyed, otherwise ERROR
*
* ERRNO: N/A
*/

LOCAL STATUS vxbAltSocGen5DwI2cInstUnlink
    (
    VXB_DEVICE_ID   pDev,
    void *          unused
    )
    {
    I2C_DRV_CTRL * pDrvCtrl = NULL;

    VXB_ASSERT (pDev != NULL, ERROR)

    pDrvCtrl = (I2C_DRV_CTRL *) pDev->pDrvCtrl;

    if (semTake (pDrvCtrl->i2cDevSem, WAIT_FOREVER) == OK)
        {
        if (semDelete (pDrvCtrl->i2cDevSem) != OK)
            {
            I2C_DBG (I2C_DBG_ERR,
                     "vxbAltSocGen5DwI2cInstUnlink:i2cDevSem delete failed\n", 
                     0, 0, 0, 0, 0, 0);
            return (ERROR);
            }
        }
        
    if (semTake (pDrvCtrl->i2cDataSem, WAIT_FOREVER) == OK)
        {
        if (semDelete (pDrvCtrl->i2cDataSem) != OK)
            {
            I2C_DBG (I2C_DBG_ERR,
                     "vxbAltSocGen5DwI2cInstUnlink:i2cDevSem delete failed\n", 
                     0, 0, 0, 0, 0, 0);
            return (ERROR);
            }
        }
        
#ifndef _VXBUS_BASIC_HWMEMLIB
    hwMemFree ((char *) pDrvCtrl);
#endif /* _VXBUS_BASIC_HWMEMLIB */    
    pDev->pDrvCtrl = NULL;

    return (OK);
    }

/*****************************************************************************
*
* vxbAltSocGen5DwI2cLock - acquire exclusive access to a given I2C controller
*
* This routine is used to gain excludive access to a given I2C controller
*
* This function should be called before any other I2C API routine is
* called, in order to guarantee that no other overlapped access to the
* bus occurs.
*
* RETURNS: OK if bus acquired, otherwise ERROR
*
* ERRNO: N/A
*/

LOCAL STATUS vxbAltSocGen5DwI2cLock
    (
    VXB_DEVICE_ID pDev
    )
    {
    I2C_DRV_CTRL * pDrvCtrl;

    if (pDev == NULL)
        return ERROR;

    pDrvCtrl = (I2C_DRV_CTRL *)pDev->pDrvCtrl;

    /* mutual exclusion begin */

    if (pDrvCtrl->i2cDevSem != NULL)
        semTake (pDrvCtrl->i2cDevSem, WAIT_FOREVER);

    return OK;
    }

/*****************************************************************************
*
* vxbAltSocGen5DwI2cUnLock - relinquish exlusive access to a given I2C 
*                            controller
*
* This function should be called when access to the I2C bus is no longer
* required. The last operation on the bus should be a stop transaction.
*
* RETURNS: OK if bus released, otherwise ERROR
*
* ERRNO: N/A
*/

LOCAL STATUS vxbAltSocGen5DwI2cUnLock
    (
    VXB_DEVICE_ID pDev
    )
    {
    I2C_DRV_CTRL * pDrvCtrl;

    if (pDev == NULL)
        return ERROR;

    pDrvCtrl = pDev->pDrvCtrl;

    if (pDrvCtrl->i2cDevSem != NULL)
        (void) semGive (pDrvCtrl->i2cDevSem);

    return OK;
    }

/*******************************************************************************
*
* vxbAltSocGen5DwI2cWaitStatus - wait for I2C controller to the specify status
*
* This routine waits for I2C controller to the specify status
*
* RETURNS: OK, or ERROR if timeout
*/

LOCAL STATUS vxbAltSocGen5DwI2cWaitStatus
    (
    VXB_DEVICE_ID   pDev,
    UINT32          staMsk
    )
    {
    UINT32 status, timeout = 0;

    VXB_ASSERT (pDev != NULL, ERROR)

    while ((!((status = CSR_READ_2(pDev, I2C_RAW_INTR_STAT)) & staMsk)) && 
           (++timeout < I2C_TIMEOUT))
           vxbAltSocGen5DwI2cDelay (1000);
           
    if (timeout == I2C_TIMEOUT)
        {
        I2C_DBG (I2C_DBG_ERR, "vxbAltSocGen5DwI2cWaitStatus:timeout, \
                 status = 0x%x\n", status, 0, 0, 0, 0, 0);

        return ERROR;
        }

    return OK;
  
    }

/*******************************************************************************
*
* vxbAltSocGen5DwI2cWaitBusFree - wait for I2C bus free
*
* This routine waits for I2C bus free
*
* RETURNS: OK, or ERROR if timeout
*/

LOCAL STATUS vxbAltSocGen5DwI2cWaitBusFree
    (
    VXB_DEVICE_ID   pDev
    )
    {
    UINT32 status, timeout = 0;

    VXB_ASSERT (pDev != NULL, ERROR)

    /* Wait until Activity Status Bit in I2C_STATUS is IDLE = 0 */    
    while (((status = CSR_READ_2(pDev, I2C_STATUS)) & I2C_STATUS_ACTIVITY) && 
           (++timeout < I2C_TIMEOUT))
           vxbAltSocGen5DwI2cDelay (1000);
    
    if (timeout == I2C_TIMEOUT)
        {
        I2C_DBG (I2C_DBG_ERR,
            "vxbAltSocGen5DwI2cWaitBusFree:timeout status 0x%x\n", status, 0, 0, 0, 0, 0);

        return (ERROR);
        }

    return (OK);
    }

/*****************************************************************************
*
* vxbAltSocGen5DwI2cRead - read a sequence of bytes from a device
*
* This function reads <length> bytes from the I2C device and stores them
* in the buffer specified by <pDataBuf>. The read begins at an offset
* specified previously in the transaction. If too many bytes are requested,
* the read will return ERROR, though any successfully read bytes will be
* returned in <pDataBuf>.
*
* RETURNS: OK if read completed successfully, otherwise ERROR
*
* ERRNO: N/A
*
* \NOMANUAL
*
*/

STATUS vxbAltSocGen5DwI2cRead
    (
    VXB_DEVICE_ID   pDevice,
    UINT8           devAddr,
    UINT32          startAddr,
    UINT8 *         pDataBuf,
    UINT32          length
    )
{
    UINT16  val = 0;
    UINT32 index = 0;
    UINT32 remain = 0;
    UINT32 transferSize = 0;
        
    I2C_HARDWARE * pI2cDev;
    I2C_DRV_CTRL * pDrvCtrl;
    VXB_DEVICE_ID  pDev;
    
    VXB_ASSERT (pDevice != NULL, ERROR)
    pI2cDev = (I2C_HARDWARE *) pDevice->pBusSpecificDevInfo;
    
    if (length == 0)
        return OK;
        
    pDev = pI2cDev->pCtlr;
    pDrvCtrl = pDev->pDrvCtrl;

    I2C_DBG(I2C_DBG_ERR,"\nvxbAltSocGen5DwI2cRead start 0x%x, length 0x%x ", 
            startAddr, length, 0, 0, 0, 0);
    
    /* acquire lock */
    
    vxbAltSocGen5DwI2cLock (pDev);
    
    if (vxbAltSocGen5DwI2cWaitBusFree (pDev))
        {
        I2C_DBG(I2C_DBG_ERR,"\nI2C is busy, return", 0, 0, 0, 0, 0, 0);
        goto error;
        }
    
    /* disable I2C Controller */    
    CSR_WRITE_2 (pDev, I2C_ENABLE, 0x0);
   
    CSR_WRITE_2 (pDev, I2C_CON, 
                 CSR_READ_2 (pDev, I2C_CON) | I2C_CON_RESTART_EN);

    /* set target address */

    if (pDrvCtrl->slaveAddr != devAddr)
        {
        pDrvCtrl->slaveAddr = devAddr;
        
        val = CSR_READ_2 (pDev, I2C_TAR);
        CSR_WRITE_2 (pDev, I2C_TAR, (val & ~0x3ff) | (devAddr));
        }
    else
        {
        val = CSR_READ_2 (pDev, I2C_TAR);
        CSR_WRITE_2 (pDev, I2C_TAR, (val & ~0x3ff) | (devAddr));
        }
    
    /* enable I2C Controller */
    CSR_WRITE_2 (pDev, I2C_ENABLE, 0x1); 

    /* set device address */    
    if (pI2cDev->flag & I2C_WORDADDR)
        {
        CSR_WRITE_2 (pDev, I2C_DATA_CMD, 
                     (UINT8)((startAddr >> 8) & 0xff));
        CSR_WRITE_2 (pDev, I2C_DATA_CMD, (UINT8)(startAddr & 0xff));
        }
    else
        {
        CSR_WRITE_2 (pDev, I2C_DATA_CMD, (UINT8)(startAddr & 0xff));    
        }

    pDrvCtrl->dataBuf = pDataBuf;
    pDrvCtrl->dataLength = length;

    if (pDrvCtrl->polling == FALSE) /*  irq mode */
    {
        remain = length;
		
        for (;remain;)
        {
            pDrvCtrl->dataLength = remain > I2C_MAX_READ_SIZE ? 
                                   I2C_MAX_READ_SIZE : remain;
            transferSize = pDrvCtrl->dataLength;
            pDrvCtrl->dataBuf = &pDataBuf [length - remain] ;
            remain -= pDrvCtrl->dataLength;

            /* update Receive FIFO Threshold Register */            
            CSR_WRITE_2 (pDev, I2C_RX_TL, pDrvCtrl->dataLength);

            CSR_WRITE_2 (pDev, I2C_INTR_MASK, 
                         (CSR_READ_2 (pDev, I2C_INTR_MASK) | 
                         I2C_INTR_RX_FULL) & (~I2C_INTR_TX_EMPTY)); 

            if (transferSize == 1)
                CSR_WRITE_2 (pDev, I2C_DATA_CMD, I2C_DATA_CMD_RD_STOP_BIT);
            else
                CSR_WRITE_2 (pDev, I2C_DATA_CMD, 0x100);

            /* restore to default value */
            CSR_WRITE_2 (pDev, I2C_RX_TL, 0);
            
            /* waiting reading done */    
            if (semTake (pDrvCtrl->i2cDataSem, SEM_TIMEOUT) == ERROR)
            {
                I2C_DBG (I2C_DBG_ERR,
                         "vxbAltSocGen5DwI2cRead:semTake error\n", 
                         0, 0, 0, 0, 0, 0);

                goto error;
            }
                
        }
    }
    else  /* polling */
    {
        for (index = 0; index < length; index++)
        {
            CSR_WRITE_2 (pDev, I2C_DATA_CMD, 0x100);
            if (vxbAltSocGen5DwI2cWaitStatus (pDev, I2C_RAW_INTR_STAT_RX_FULL) == ERROR)
                {
                I2C_DBG(I2C_DBG_ERR,"vxbAltSocGen5DwI2cWaitStatus error\n", 
                        0, 0, 0, 0, 0, 0);
                goto error;
                }
            else
                pDataBuf [index] =  CSR_READ_2 (pDev, I2C_DATA_CMD) & 0xff;
        }
        
        /* stop Generation */                    
        CSR_WRITE_2 (pDev, I2C_DATA_CMD, 0x300);
    }
    
    if (vxbAltSocGen5DwI2cWaitBusFree (pDev))
    {
        I2C_DBG(I2C_DBG_ERR,"I2C is busy\n", 0, 0, 0, 0, 0, 0);
       
        goto error;
    }
     
    vxbAltSocGen5DwI2cUnLock (pDev);
        
    return OK;
        
error:
    I2C_DBG(I2C_DBG_ERR,"vxbAltSocGen5DwI2cRead failed\n", 0, 0, 0, 0, 0, 0);

    if (semGive (pDrvCtrl->i2cDataSem) == ERROR)
    {
        I2C_DBG (I2C_DBG_ERR,
                 "vxbAltSocGen5DwI2cRead:semGive error\n", 
                 0, 0, 0, 0, 0, 0);
    }
        
    vxbAltSocGen5DwI2cInit (pDev);
        
    vxbAltSocGen5DwI2cUnLock (pDev);
         
    return (ERROR);
}

/*****************************************************************************
*
* vxbAltSocGen5DwI2cWrite - write a sequence of bytes to a device
*
* This function writes <length> bytes from the I2C device read from the
* buffer specified by <pDataBuf>. The write begins at an offset specified
* previously in the transaction. If too many bytes are requested, the write
* will return ERROR.
*
* RETURNS: OK if write completed successfully, otherwise ERROR
*
* ERRNO: N/A
*
* \NOMANUAL
*
*/

STATUS vxbAltSocGen5DwI2cWrite
    (
    VXB_DEVICE_ID   pDevice,
    UINT8           devAddr,
    UINT32          startAddr,
    UINT8 *         pDataBuf,
    UINT32          length
    )
    {
    UINT16 intMask;
    UINT16  val = 0;
    UINT32 index = 0;
    UINT32 timeout = I2C_TIMEOUT;
    
    I2C_HARDWARE * pI2cDev;
    I2C_DRV_CTRL * pDrvCtrl;
    VXB_DEVICE_ID  pDev;
    
    VXB_ASSERT (pDevice != NULL, ERROR)
    pI2cDev = (I2C_HARDWARE *) pDevice->pBusSpecificDevInfo;
    
    pDev = pI2cDev->pCtlr;
    pDrvCtrl = pDev->pDrvCtrl;
    
    I2C_DBG(I2C_DBG_ERR,
            "\nvxbAltSocGen5DwI2cWrite start 0x%x, length 0x%x pDataBuf 0x%x ", 
            startAddr, length, *pDataBuf, 0, 0, 0);
    
    /* acquire lock */
    
    vxbAltSocGen5DwI2cLock (pDev);
   
    if (vxbAltSocGen5DwI2cWaitBusFree (pDev))
        {
        I2C_DBG(I2C_DBG_ERR,"\nI2C is busy, return", 0, 0, 0, 0, 0, 0);
        goto error;
        }
  
    /* disable I2C Controller */

    CSR_WRITE_2 (pDev, I2C_ENABLE, 0x0);
        
    /* set target address */

    if (pDrvCtrl->slaveAddr != devAddr)
        {
        pDrvCtrl->slaveAddr = devAddr;
        
        val = CSR_READ_2 (pDev, I2C_TAR);
        CSR_WRITE_2 (pDev, I2C_TAR, (val & ~0x3ff) | (devAddr));
        }
    else
        {
        val = CSR_READ_2 (pDev, I2C_TAR);
        CSR_WRITE_2 (pDev, I2C_TAR, (val & ~0x3ff) | (devAddr));
        }
    
    /* enable I2C Controller */
    
    CSR_WRITE_2 (pDev, I2C_ENABLE, 0x1); 

    /* set device address */

    if (pI2cDev->flag & I2C_WORDADDR)
        {
        CSR_WRITE_2 (pDev, I2C_DATA_CMD, 
                     (UINT8)((startAddr >> 8) & 0xff));
        CSR_WRITE_2 (pDev, I2C_DATA_CMD, (UINT8)(startAddr & 0xff));
        }
    else
        CSR_WRITE_2 (pDev, I2C_DATA_CMD, (UINT8)(startAddr & 0xff));  

    pDrvCtrl->dataBuf = pDataBuf;
    pDrvCtrl->dataLength = length;
    
    if (pDrvCtrl->polling == FALSE)  /* irq mode */ 
    {       
        /* enable TX Empty, disable Rx Full */
        
        intMask = CSR_READ_2 (pDev, I2C_INTR_MASK);
    
        CSR_WRITE_2 (pDev, I2C_INTR_MASK, 
                     (intMask | I2C_INTR_TX_EMPTY) & (~I2C_INTR_RX_FULL));
    
        while ((pDrvCtrl->dataLength != 0) && (timeout != 0))
            {
            vxbAltSocGen5DwI2cDelay (100);
            timeout--;
            }
        
        if (pDrvCtrl->dataLength != 0)
            goto error;
    }
    else  /* polling mode */
    {
        for (index = 0; index < length; index++)
            {
            
            if (vxbAltSocGen5DwI2cWaitStatus (pDev, I2C_RAW_INTR_STAT_TX_EMPTY) == ERROR)
                {
                I2C_DBG(I2C_DBG_ERR,"vxbAltSocGen5DwI2cWaitStatus error\n", 
                        0, 0, 0, 0, 0, 0);
                goto error;
                }
            if (index == (length - 1))
                CSR_WRITE_2 (pDev, I2C_DATA_CMD, 
                             (pDataBuf [index] & 0xff) | 0x200);
            else
                CSR_WRITE_2 (pDev, I2C_DATA_CMD, pDataBuf [index] & 0xff);
            }
    }

    if (vxbAltSocGen5DwI2cWaitBusFree (pDev))
        {
        I2C_DBG(I2C_DBG_ERR,"I2C is busy \n", 0, 0, 0, 0, 0, 0);
        goto error;
        }
    
    vxbAltSocGen5DwI2cUnLock (pDev);
      
    return OK;

error:

    I2C_DBG(I2C_DBG_ERR,
            "vxbAltSocGen5DwI2cWrite failed\n", 0, 0, 0, 0, 0, 0);

    vxbAltSocGen5DwI2cInit (pDev);
        
    vxbAltSocGen5DwI2cUnLock (pDev);
        
    return ERROR;
    }

/*****************************************************************************
*
* vxbAltSocGen5DwI2cShow - show the valid device address on the bus
*
* This function scans the device address from 0 to 0x7f
* (only 7 bit address supported) on each I2C bus, 
* if we can receipt the ACK signal from the device, means this address is valid
* and have the device connect to the BUS. If no ACK received, go on to the next
* address.
*
* RETURNS: OK if write completed successfully, otherwise ERROR
*
* ERRNO: N/A
*
* \NOMANUAL
*
*/

STATUS vxbAltSocGen5DwI2cShow
    (
    VXB_DEVICE_ID   pDev,
    int             verbose
    )
    {
    UINT8 addrNo, data;
    I2C_DRV_CTRL * pDrvCtrl;

    VXB_ASSERT (pDev != NULL, ERROR)

    pDrvCtrl = pDev->pDrvCtrl;

    printf ("        %s unit %d on %s @ 0x%08x",
        pDev->pName,
        pDev->unitNumber,
        vxbBusTypeString (pDev->busID),
        pDev);
	
    printf (" with busInfo %p\n", pDev->u.pSubordinateBus);

    if (verbose >= 1)
    {
        printf ("           BAR0 @ 0x%08x (memory mapped)\n",
                pDev->pRegBase[0]);
        printf ("           pDrvCtrl @ 0x%08x\n", pDev->pDrvCtrl);
    }

    if (verbose > 1000)
    {
        printf("            clkFrequency:  %dMhz\n", pDrvCtrl->clkFrequency/1000000);
        printf("            busSpeed    :  %dKbps\n",pDrvCtrl->busSpeed/1000);
        printf("            interrupt   :  %s\n",pDrvCtrl->polling?"FALSE":"TRUE");

        printf("\n");
    }
	
    return OK;
 }


#if 1
FUNCPTR fmqlI2cShow  = NULL;

void test_i2c()
{
	VXB_DEVICE_ID i2cDev;	
	int bus_id = 0;
	
	bus_id = 0;
	i2cDev = vxbInstByNameFind ("dwI2c", bus_id);
	if (NULL == i2cDev)
	{	
		return  ;
	}

	fmqlI2cShow  = vxbDevMethodGet (i2cDev, DEVMETHOD_CALL(busDevShow));
	fmqlI2cShow(i2cDev, 0xFFFF);
	
	bus_id = 1;
	i2cDev = vxbInstByNameFind ("dwI2c", bus_id);
	if (NULL == i2cDev)
	{	
		return  ;
	}

	fmqlI2cShow  = vxbDevMethodGet (i2cDev, DEVMETHOD_CALL(busDevShow));
	fmqlI2cShow(i2cDev, 0xFFFF);
	
	return;
}

#define EEPROM_I2C_ADDR   (0x50)

int i2c_read_eeprom(UINT8* pDataBuf, UINT32 len, UINT32	startAddr)
{
	VXB_DEVICE_ID i2cDev;	
	int bus_id = 0;
	UINT8 devAddr = 0;
	VXB_I2C_BUS_CTRL *  pI2cCtrl;
	
	devAddr = EEPROM_I2C_ADDR;  /* i2c_1: eeprom; i2c_addr: 0x50 */
	bus_id = 1;
	
	i2cDev = vxbInstByNameFind ("dwI2c", bus_id);
	if (NULL == i2cDev)
	{	
		return ERROR;
	}
	
	pI2cCtrl = vxbAltSocGen5DwI2cCtrlGet(i2cDev);	
	/* return vxbAltSocGen5DwI2cRead(i2cDev, devAddr, startAddr, pDataBuf, len); */
	return pI2cCtrl->vxbI2cDevRead(i2cDev, devAddr, startAddr, pDataBuf, len);
}

int i2c_write_eeprom(UINT8* pDataBuf, UINT32 len, UINT32 startAddr)
{
	VXB_DEVICE_ID i2cDev;	
	int bus_id = 0;
	UINT8 devAddr = 0;
	VXB_I2C_BUS_CTRL *  pI2cCtrl;
	
	devAddr = EEPROM_I2C_ADDR;  /* i2c_1: eeprom; i2c_addr: 0x50*/
	bus_id = 1;
	
	i2cDev = vxbInstByNameFind ("dwI2c", bus_id);
	if (NULL == i2cDev)
	{	
		return ERROR;
	}
	
	pI2cCtrl = vxbAltSocGen5DwI2cCtrlGet(i2cDev);	
	/* return vxbAltSocGen5DwI2cWrite(i2cDev, devAddr, startAddr, pDataBuf, len); */
	return pI2cCtrl->vxbI2cDevWrite(i2cDev, devAddr, startAddr, pDataBuf, len);
}



void CONSOLE_LOG(UINT8* buf, int len)
{
	int i = 0;
	
	for (i=0; i<len; i++)
	{
		printf("%02X", buf[i]);
		
		if ((i+1)%4 == 0)
		{
			printf(" ");
		}
		
		if ((i+1)%16 == 0)
		{
			printf("\n");
		}
	}
	
	if (i%16 != 0)
	{
		printf("\n");
	}

	return;
}

void test_i2c_2(int offset, int len)
{
	UINT8 buf[1024] = {0};
	int i = 0;
	
	i2c_read_eeprom((UINT8*)(&buf[0]), len, offset);
	printf("----read eeprom---- \n");
	CONSOLE_LOG((UINT8*)(&buf[0]), len);

	printf("\n----write eeprom---- \n");
	for (i=0; i<1024; i++)
	{
		buf[i] = i;
	}	
	i2c_write_eeprom((UINT8*)(&buf[0]), len, offset);
	CONSOLE_LOG((UINT8*)(&buf[0]), len);
	
	memset((UINT8*)(&buf[0]), 0, 1024);
	
	printf("\n----wr&rd eeprom---- \n");
	i2c_read_eeprom((UINT8*)(&buf[0]), len, offset);	
	CONSOLE_LOG((UINT8*)(&buf[0]), len);

	return;
}

#endif

#if 1

#define DEV_I2C_ADDR   (0x39)  /* 0x72>>1 == 0x39*/

int i2c_read_slave(UINT8* pDataBuf, UINT32 len, UINT32	startAddr)
{
	VXB_DEVICE_ID i2cDev;	
	int bus_id = 0;
	UINT8 devAddr = 0;
	VXB_I2C_BUS_CTRL *  pI2cCtrl;
	
	devAddr = DEV_I2C_ADDR;  /* i2c_0: hdmi-sii9135; i2c_addr: 0x39 */
	bus_id = 0;
	
	i2cDev = vxbInstByNameFind ("dwI2c", bus_id);
	if (NULL == i2cDev)
	{	
		printf("open dwI2c fail! \n");
		return ERROR;
	}
	
	pI2cCtrl = vxbAltSocGen5DwI2cCtrlGet(i2cDev);	
	return pI2cCtrl->vxbI2cDevRead(i2cDev, devAddr, startAddr, pDataBuf, len);
}

int i2c_write_slave(UINT8* pDataBuf, UINT32 len, UINT32 startAddr)
{
	VXB_DEVICE_ID i2cDev;	
	int bus_id = 0;
	UINT8 devAddr = 0;
	VXB_I2C_BUS_CTRL *  pI2cCtrl;
	
	devAddr = DEV_I2C_ADDR;  /* i2c_0: hdmi-sii9135; i2c_addr: 0x39 */
	bus_id = 0;
	
	i2cDev = vxbInstByNameFind ("dwI2c", bus_id);
	if (NULL == i2cDev)
	{	
		printf("open dwI2c fail! \n");
		return ERROR;
	}
	
	pI2cCtrl = vxbAltSocGen5DwI2cCtrlGet(i2cDev);	
	return pI2cCtrl->vxbI2cDevWrite(i2cDev, devAddr, startAddr, pDataBuf, len);
}

int i2c0_read_reg32(UINT8* pDataBuf, UINT32	reg_offset)
{
	return i2c_read_slave(pDataBuf, 4, reg_offset);
}

int i2c0_write_reg32(UINT8* pDataBuf, UINT32	reg_offset)
{
	return i2c_write_slave(pDataBuf, 4, reg_offset);
}


void test_i2c_0_hdmi9134(void)
{
	UINT32 tmp32 = 0;
	UINT32 reg = 0;

	reg = 0x00;	
	i2c0_read_reg32(&tmp32, reg);
	printf("id=> read-reg(0x%X) = 0x%08X \n", reg, tmp32);
	
	reg = 0x08;	
	i2c0_read_reg32(&tmp32, reg);
	printf("ro=> read-reg(0x%X) = 0x%08X \n", reg, tmp32);

	tmp32 = 0x35;
	i2c0_write_reg32(&tmp32, reg);
	taskDelay(1);
	tmp32 = 0;
	i2c0_read_reg32(&tmp32, reg);
	printf("w&r=> read-reg(0x%X) = 0x%08X \n", reg, tmp32);
	
	return;
}

#endif

