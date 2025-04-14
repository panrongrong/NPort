/* usbSynopsysHcdHardwareAccess.c - hardware access routines for Synopsys HCD */

/* Copyright (c) 2009 Wind River Systems, Inc.

   This software includes software licensed to Wind River Systems, Inc.
   by Wipro, Ltd. Wind River licensees may use this software according
   to the terms of their Wind River license agreement(s) applicable to
   this software.
*/

/*
modification history
--------------------
01d,25sep09,m_y  modify the read/write register entry to fit other CAVIUM CPU.
01c,11sep09,m_y  clean warning message.
01b,02sep09,m_y  remove for loop in the flush TX/RX fifo entry.
01a,20jul09,m_y  written.
*/

/*
DESCRIPTION

This file contains the hardware access entries provided by the
Synopsys USB Host Controller Driver.

INCLUDE FILES: usbSynopsysHcdRegisterInfo.h,
               usbSynopsysHcdDataStructures.h,
               usbSynopsysHcdHardwareAccess.h
*/

/* includes */

/*
#include <usbSynopsysHcdRegisterInfo.h>
#include <usbSynopsysHcdDataStructures.h>
#include <usbSynopsysHcdHardwareAccess.h>
*/

#include "usbSynopsysHcdEventHandler.h"
#include "usbSynopsysHcdHardwareAccess.h"
#include "usbSynopsysHcdDataStructures.h"
#include "usbSynopsysHcdInterfaces.h"
#include "usbSynopsysHcdRegisterInfo.h"
#include "usbSynopsysHcdRhEmulation.h"
#include "usbSynopsysHcdUtil.h"

#define USB_SYNOPSYSHCD_FIFO_ALIGN_MASK                     (0xFFFFFFF8)

/*******************************************************************************
*
* usbSynopsysHcdFlushTxFIFO - flush the related TX FIFO
*
* This routine flushs the <TxFIFONum> TX FIFO.
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* \NOMANUAL
*/

void usbSynopsysHcdFlushTxFIFO
    (
    pUSB_SYNOPSYSHCD_DATA pSynopsysHcdData, /* Pointer to HCD block */
    int                   TxFIFONum         /* TX FIFO number to be flushed */
    )
    {
    volatile UINT32 regValue = 0;
    int uTimeOutCount = 10000;

    /* Wait AHB idle  */

    for(;;)
        {
        regValue = USB_SYNOPSYSHCD_READ32_REG(pSynopsysHcdData,
                                              USB_SYNOPSYSHCD_GRSTCTL);
        uTimeOutCount--;
        if ((regValue & USB_SYNOPSYSHCD_GRSTCTL_AHBIDLE)||(uTimeOutCount < 0))
            break;
        }

    /* Flush all periodic/nonperiodic FIFO in usb core */

    regValue = USB_SYNOPSYSHCD_READ32_REG(pSynopsysHcdData,
                                          USB_SYNOPSYSHCD_GRSTCTL);

    /* Set the TxFIFONum */

    regValue &= ~USB_SYNOPSYSHCD_GRSTCTL_TXFNUM;
    regValue |= ((TxFIFONum & 0x1F) << USB_SYNOPSYSHCD_GRSTCTL_TXFNUM_OFFSET);

    /* Set the TXFFLSH bit */

    regValue |= USB_SYNOPSYSHCD_GRSTCTL_TXFFLSH;

    USB_SYNOPSYSHCD_WRITE32_REG(pSynopsysHcdData,
                                USB_SYNOPSYSHCD_GRSTCTL,
                                regValue);

    /* Wait until the TXFFLSH to be cleard by usb core*/
    /* The Datasheet said it will take 8 clock cycle*/

    do
        {
        regValue = USB_SYNOPSYSHCD_READ32_REG(pSynopsysHcdData,
                                              USB_SYNOPSYSHCD_GRSTCTL);
        }while(0 != (regValue & USB_SYNOPSYSHCD_GRSTCTL_TXFFLSH));

    return ;
    }

/*******************************************************************************
*
* usbSynopsysHcdFlushRxFIFO - flush the RX FIFO
*
* This routine flushs the RX FIFO.
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* \NOMANUAL
*/

void usbSynopsysHcdFlushRxFIFO
    (
    pUSB_SYNOPSYSHCD_DATA pSynopsysHcdData /* Pointer to HCD block */
    )
    {
    volatile UINT32 regValue = 0;
    int uTimeOutCount = 10000;

    /* Wait AHB idle  */

    for(;;)
        {
        regValue = USB_SYNOPSYSHCD_READ32_REG(pSynopsysHcdData,
                                              USB_SYNOPSYSHCD_GRSTCTL);
        uTimeOutCount--;
        if ((regValue & USB_SYNOPSYSHCD_GRSTCTL_AHBIDLE)||(uTimeOutCount < 0))
            break;
        }

    /* Set the RXFFLSH bit */

    USB_SYNOPSYSHCD_SETBITS32_REG(pSynopsysHcdData,
                                  USB_SYNOPSYSHCD_GRSTCTL,
                                  USB_SYNOPSYSHCD_GRSTCTL_RXFFLSH);

    /* Wait until the RXFFLSH to be cleard by usb core*/
    /* The Datasheet said it will take 8 clock cycle*/

    do
        {
        regValue = USB_SYNOPSYSHCD_READ32_REG(pSynopsysHcdData,
                                              USB_SYNOPSYSHCD_GRSTCTL);
        }while(0 != (regValue & USB_SYNOPSYSHCD_GRSTCTL_RXFFLSH));

    return ;
    }

/*******************************************************************************
*
* usbSynopsysHCDResetCore - soft reset the USB Core
*
* This routine does soft reset to the USB Core to reinit the statemachine.
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* \NOMANUAL
*/

void usbSynopsysHCDResetCore
    (
    pUSB_SYNOPSYSHCD_DATA pHCDData /* Pointer to HCD block */
    )
    {
	volatile UINT32 uGrstctlReg = 0x0;
    int uTimeOutCount = 10000;

    /* Wait AHB idle */

    for(;;)
        {
        uGrstctlReg = USB_SYNOPSYSHCD_READ32_REG(pHCDData,
                                                 USB_SYNOPSYSHCD_GRSTCTL);
        uTimeOutCount--;
        if ((uGrstctlReg & USB_SYNOPSYSHCD_GRSTCTL_AHBIDLE)||(uTimeOutCount < 0))
            break;
        }

    /* Soft reset USB core */

    uGrstctlReg = USB_SYNOPSYSHCD_READ32_REG(pHCDData, USB_SYNOPSYSHCD_GRSTCTL);
    uGrstctlReg |= USB_SYNOPSYSHCD_GRSTCTL_CSFTRST;
    USB_SYNOPSYSHCD_WRITE32_REG(pHCDData, USB_SYNOPSYSHCD_GRSTCTL, uGrstctlReg);

    /* Wait reset finish */

    do
        {
        uGrstctlReg = USB_SYNOPSYSHCD_READ32_REG(pHCDData,
                                                 USB_SYNOPSYSHCD_GRSTCTL);
    	}while(0 != (uGrstctlReg & USB_SYNOPSYSHCD_GRSTCTL_CSFTRST));

    /* After reset we must wait at lease 3 phy clocks */

    OS_DELAY_MS(100);

}

/*******************************************************************************
*
* usbSynopsysHCDCoreInit - initialize the USB core
*
* This routine initializes the USB Core.
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* \NOMANUAL
*/

void usbSynopsysHCDCoreInit
    (
    pUSB_SYNOPSYSHCD_DATA pHCDData /* Pointer to HCD block */
    )
    {
	UINT32 uGahbcfgReg = 0x0;
	UINT32 uGusbcfgReg = 0x0;

	if(NULL == pHCDData)
	    return;

    /* USB Core Initialization */

    /* Reset USB Core Statemachine */
#if 0  /* jc*/
	usbSynopsysHCDResetCore(pHCDData);
#endif

	/* 
	Configure the GAHBCFG register 
	*/
	uGahbcfgReg = USB_SYNOPSYSHCD_READ32_REG(pHCDData, USB_SYNOPSYSHCD_GAHBCFG);

    /* Configure the transfer mode */
    /* Now we always use DMA */
    uGahbcfgReg |= USB_SYNOPSYSHCD_GAHBCFG_DMAEN;  /* bit5*/
    
	uGahbcfgReg &= ~USB_SYNOPSYSHCD_GAHBCFG_HBSTLEN;	
	/* jc*/
	uGahbcfgReg |= (1 << 1);     /* bit1	*/
	/* uGahbcfgReg |= (3 << 1);  // 4:1 Burst Length/Type (HBstLen) => 4’b0011 INCR4*/
	
	uGahbcfgReg |= USB_SYNOPSYSHCD_GAHBCFG_NPTXFEMPLVL;  /* bit8*/
	uGahbcfgReg |= USB_SYNOPSYSHCD_GAHBCFG_PTXFEMPLVL;   /* bit7*/
	
	uGahbcfgReg |= USB_SYNOPSYSHCD_GAHBCFG_GLBLINTRMSK;  /* bit0*/
	
	USB_SYNOPSYSHCD_WRITE32_REG(pHCDData, USB_SYNOPSYSHCD_GAHBCFG, uGahbcfgReg);


	/* 
	Configure the GUSBCFG Register 
	*/
	uGusbcfgReg = USB_SYNOPSYSHCD_READ32_REG(pHCDData, USB_SYNOPSYSHCD_GUSBCFG);
	uGusbcfgReg &= ~USB_SYNOPSYSHCD_GUSBCFG_TOUTCAL;	/* bit2~0*/
	uGusbcfgReg &= ~USB_SYNOPSYSHCD_GUSBCFG_DDRSEL;     /* bit7:0*/
	
	uGusbcfgReg &= ~USB_SYNOPSYSHCD_GUSBCFG_USBTRDTIM;  /* bit13~10	*/
	uGusbcfgReg |=  (0x05 << USB_SYNOPSYSHCD_GUSBCFG_USBTRDTIM_OFFSET);  /* 4’h5: When the MAC interface is 16-bit UTMI+.*/
	
	uGusbcfgReg &= ~USB_SYNOPSYSHCD_GUSBCFG_PHYLPWRCLKSEL; /* bit15*/

	/* jc*/
	uGusbcfgReg &= ~(1 << 17);  /* bit17: ULPI FS/LS Select (ULPIFsLs) => 1’b0: ULPI interface*/
	
	/* jc : disable hnp, srp 	*/
	uGusbcfgReg &= ~USB_SYNOPSYSHCD_GUSBCFG_HNPCAP;  /* bit9*/
	uGusbcfgReg &= ~USB_SYNOPSYSHCD_GUSBCFG_SRPCAP;  /* bit8*/
	
	/* jc*/
	uGusbcfgReg |= (USB_SYNOPSYSHCD_GUSBCFG_ULPI_UTMI_SEL);  /* bit4: ulpi*/
	uGusbcfgReg |= (1 << 29);  /* bit29: Force Host Mode (ForceHstMode) */
	uGusbcfgReg |= (1 << 18);  /* bit18:  1’b1: PHY uses AutoResume feature */
	
	USB_SYNOPSYSHCD_WRITE32_REG(pHCDData, USB_SYNOPSYSHCD_GUSBCFG, uGusbcfgReg);

	return;
}

/*******************************************************************************
*
* usbSynopsysHCDHostInit - initialize the USB to work in host mode
*
* This routine initializes the USB to work in host mode.
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* \NOMANUAL
*/

void usbSynopsysHCDHostInit
    (
    pUSB_SYNOPSYSHCD_DATA pHCDData /* Pointer to HCD block */
    )
    {
	UINT32 uTempRegValue = 0x0;
	UINT32 uGrxfsizReg = 0x0;
	UINT32 uGnptxfsizReg = 0x0;
	UINT32 uHptxfsizReg = 0x0;
	UINT32 uDFIFOSize = 0x0;
	UINT32 uRxFIFOSize = 0x0;
	UINT32 uNpTxFIFOSize = 0x0;
	UINT32 uPTxFIFOSize = 0x0;
	int    i = 0;

	int timeout = 0;

	if(NULL == pHCDData)
	    return;

	/* Init the host side function */

    /* Reset the PHY Clock */
    uTempRegValue = USB_SYNOPSYSHCD_READ32_REG(pHCDData,
                                               USB_SYNOPSYSHCD_PCGCCTL);
	
	uTempRegValue &= ~USB_SYNOPSYSHCD_PCGCCTL_PHYSUSPENDED;
	uTempRegValue &= ~USB_SYNOPSYSHCD_PCGCCTL_RSTPDWNMODULE;
	uTempRegValue &= ~USB_SYNOPSYSHCD_PCGCCTL_PWRCLMP;
	uTempRegValue &= ~USB_SYNOPSYSHCD_PCGCCTL_GATEHCLK;
	uTempRegValue &= ~USB_SYNOPSYSHCD_PCGCCTL_STOPPCLK;
	USB_SYNOPSYSHCD_WRITE32_REG(pHCDData,
                                USB_SYNOPSYSHCD_PCGCCTL,
                                uTempRegValue);

    /*
     * Program the USBC_HCFG register to select full-speed host or
     * high-speed host.
     */
	uTempRegValue = USB_SYNOPSYSHCD_READ32_REG(pHCDData, USB_SYNOPSYSHCD_HCFG);

    /* Support HS/FS/LS speed */
	uTempRegValue &= ~ USB_SYNOPSYSHCD_HCFG_FSLSSUPP;

    /* PHY Clock run at 30/60 MHZ */
    uTempRegValue &= ~ USB_SYNOPSYSHCD_HCFG_FSLPCLKSEL;
	
	USB_SYNOPSYSHCD_WRITE32_REG(pHCDData, USB_SYNOPSYSHCD_HCFG, uTempRegValue);


    /* 
    Set the RX/TX FIFO Size 
	*/

	/* Get the total DFIFO size */
 	uTempRegValue = USB_SYNOPSYSHCD_READ32_REG(pHCDData,
	                                           USB_SYNOPSYSHCD_GHWCFG3);
	
	uDFIFOSize = (uTempRegValue & USB_SYNOPSYSHCD_GHWCFG3_DFIFODEPTH) >>
	             USB_SYNOPSYSHCD_GHWCFG3_DFIFODEPTH_OFFSET;

	/* Program the RX FIFO size */
	uGrxfsizReg = USB_SYNOPSYSHCD_READ32_REG(pHCDData, USB_SYNOPSYSHCD_GRXFSIZ);

	uRxFIFOSize = ((uDFIFOSize * 30) / 100) & USB_SYNOPSYSHCD_FIFO_ALIGN_MASK;

	uGrxfsizReg = (uRxFIFOSize << USB_SYNOPSYSHCD_GRXFSIZ_RXFDEP_OFFSET) & \
                  USB_SYNOPSYSHCD_GRXFSIZ_RXFDEP;
	USB_SYNOPSYSHCD_WRITE32_REG(pHCDData, USB_SYNOPSYSHCD_GRXFSIZ, uGrxfsizReg);



	/* Program the non-periodic TX FIFO size */
	uNpTxFIFOSize = ((uDFIFOSize * 50) / 100) & USB_SYNOPSYSHCD_FIFO_ALIGN_MASK;
	
	uGnptxfsizReg = ((uNpTxFIFOSize << USB_SYNOPSYSHCD_GNPTXFSIZ_NPTXFDEP_OFFSET) & \
                     USB_SYNOPSYSHCD_GNPTXFSIZ_NPTXFDEP) | \
	                (uRxFIFOSize & USB_SYNOPSYSHCD_GNPTXFSIZ_NPTXFSTADDR);
	USB_SYNOPSYSHCD_WRITE32_REG(pHCDData, USB_SYNOPSYSHCD_GNPTXFSIZ, uGnptxfsizReg);



	/* Program the periodic TX FIFO size */
	uPTxFIFOSize = uDFIFOSize - uRxFIFOSize - uNpTxFIFOSize;
	
	uHptxfsizReg = ((uPTxFIFOSize << USB_SYNOPSYSHCD_HPTXFSIZ_PTXFSIZE_OFFSET) &
                     USB_SYNOPSYSHCD_HPTXFSIZ_PTXFSIZE) |
	                ((uRxFIFOSize + uNpTxFIFOSize) &
	                  USB_SYNOPSYSHCD_HPTXFSIZ_PTXFSTADDR);
	
    USB_SYNOPSYSHCD_WRITE32_REG(pHCDData,
                                USB_SYNOPSYSHCD_HPTXFSIZ,
                                uHptxfsizReg);

    /* Flush all FIFOs */
	usbSynopsysHcdFlushTxFIFO(pHCDData, 0x10);
	usbSynopsysHcdFlushRxFIFO(pHCDData);

	pHCDData->hostNumDmaChannels = 4; /* jc for test*/

	/*
	 * Flush all channels and leftover queue to put every
	 * channel into known state
	 */
	for (i = 0 ; i < pHCDData->hostNumDmaChannels; i++)
    {
    	uTempRegValue = USB_SYNOPSYSHCD_READ32_REG(pHCDData,
                                                   USB_SYNOPSYSHCD_HCCHAR(i));
    	uTempRegValue &= ~USB_SYNOPSYSHCD_HCCHAR_CHENA;
    	uTempRegValue |= USB_SYNOPSYSHCD_HCCHAR_CHDIS;
    	uTempRegValue &= ~USB_SYNOPSYSHCD_HCCHAR_EPDIR;
    	USB_SYNOPSYSHCD_WRITE32_REG(pHCDData, USB_SYNOPSYSHCD_HCCHAR(i),
                                    uTempRegValue);
    }

	for(i = 0 ; i < pHCDData->hostNumDmaChannels; i++)
    {
    	uTempRegValue = USB_SYNOPSYSHCD_READ32_REG(pHCDData,
                                                   USB_SYNOPSYSHCD_HCCHAR(i));
    	uTempRegValue |= USB_SYNOPSYSHCD_HCCHAR_CHENA;
    	uTempRegValue |= USB_SYNOPSYSHCD_HCCHAR_CHDIS;
    	uTempRegValue &= ~USB_SYNOPSYSHCD_HCCHAR_EPDIR;
    	USB_SYNOPSYSHCD_WRITE32_REG(pHCDData,
                                    USB_SYNOPSYSHCD_HCCHAR(i),
                                    uTempRegValue);
    	timeout = 5;  /* jc*/
		do
    	{
    		uTempRegValue = USB_SYNOPSYSHCD_READ32_REG(pHCDData,
                                                       USB_SYNOPSYSHCD_HCCHAR(i));

		#if 1  /* jc */
			timeout--;
			OS_DELAY_MS(1);
			
			if (timeout <= 0)
			{
				printf("chn(%d/%d) timeout! \n", i, pHCDData->hostNumDmaChannels);
				break;
			}	
		#endif
    	} while (0 != (uTempRegValue & USB_SYNOPSYSHCD_HCCHAR_CHENA));
    }


    /* Program the port power bit to drive VBUS on the USB */
    uTempRegValue = USB_SYNOPSYSHCD_READ32_REG(pHCDData, USB_SYNOPSYSHCD_HPRT);

    /*
     * Port Enable Bit is R/W1C. So we must make sue we doesn't
     * change the PRTENA bit
     */
    uTempRegValue &= ~USB_SYNOPSYSHCD_HPRT_PRTENA;  
	/* jc*/
    uTempRegValue &= ~USB_SYNOPSYSHCD_HPRT_PRTCONNDET;
    uTempRegValue &= ~USB_SYNOPSYSHCD_HPRT_PRTENCHNG;
    uTempRegValue &= ~USB_SYNOPSYSHCD_HPRT_PRTOVRCURRCHNG;
	
    uTempRegValue |= USB_SYNOPSYSHCD_HPRT_PRTPWR;
    USB_SYNOPSYSHCD_WRITE32_REG(pHCDData, USB_SYNOPSYSHCD_HPRT, uTempRegValue);
	
}

/* End of file */
