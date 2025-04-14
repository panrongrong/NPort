/* usbPciStub.c - system specific USB controller driver porting routines */

/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 *
 * The right to copy, distribute, modify or otherwise make use
 * of this software may be licensed only pursuant to the terms
 * of an applicable Wind River license agreement.
 */

/*
Modification history
--------------------
01c,15may14,wyy  Change a register bit of USB_DWC_GUSBCFG. (VXW6-81770)
01b,16jul13,ljg  Fix compiler warning (WIND00426520)
01a,10jul13,ljg  Created for USB HCD support
*/

/*
DESCRIPTION

This file defines a skeleton of functions to be used for accessing
USB module regsiters and to support USB stack with some basic hardware
initialization. It is named as "usbPciStub.c" but actually used as
a USB stack porting layer for each BSP supporting USB, either PCI based
or non-PCI based.

The name of each function in this group begins with "usb" to represent
"Device Driver Services."
*/

/* Includes */

#include <vxWorks.h>
#include <usb/usbPlatform.h>          /* Basic definitions */
#include <usb/usbPciLib.h>            /* Our API */

#ifdef INCLUDE_SYNOPSYSHCI
    
#define USB_DWC_GOTGCTL        0x000
#define USB_DWC_GOTGINT        0x004
#define USB_DWC_GAHBCFG        0x008
#define USB_DWC_GUSBCFG        0x00C
#define USB_DWC_GRSTCTL        0x010
#define USB_DWC_GINTSTS        0x014
#define USB_DWC_GINTMSK        0x018
#define USB_DWC_GHWCFG1        0x044
#define USB_DWC_GHWCFG2        0x048
#define USB_DWC_GHWCFG3        0x04c
#define USB_DWC_GHWCFG4        0x050
#define USB_DWC_HPTXFSIZ       0x100

#define USB_DWC_HCFG           0x400
#define USB_DWC_HPRT           0x440
#define USB_DWC_PCGCCTL        0xe00

#define USB_DWC_DPTX_FSIZ_DIPTXF(x)         (0x104 + (x) * 4)    /* 15 => x > 1 */

#define USB_DWC_AHBCFG_BURST_LEN(x)         (x << 1)

#define USB_DWC_GAHBCFG_INT_DMA_BURST_INCR  1

#define USB_DWC_USBCFG_FRC_HST_MODE         (1 << 29)
#define USB_DWC_USBCFG_TERM_SEL_DL_PULSE    (1 << 22)
#define USB_DWC_USBCFG_ULPI_EXT_VBUS_DRV    (1 << 20)
#define USB_DWC_USBCFG_ULPI_CLK_SUS_M       (1 << 19)
#define USB_DWC_USBCFG_ULPI_FSLS            (1 << 17)
#define USB_DWC_USBCFG_HNP_CAP              (1 << 9)
#define USB_DWC_USBCFG_SRP_CAP              (1 << 8)
#define USB_DWC_USBCFG_DDRSEL               (1 << 7)
#define USB_DWC_USBCFG_ULPI_UTMI_SEL        (1 << 4)
#define USB_DWC_USBCFG_PHYIF                (1 << 3)

#define USB_DWC_AHBCFG_DMA_ENA              (1 << 5)
#define USB_DWC_AHBCFG_GLBL_INT_MASK        (1 << 0)

#define USB_DWC_INTMSK_WKP                  (1 << 31)
#define USB_DWC_INTMSK_NEW_SES_DET          (1 << 30)
#define USB_DWC_INTMSK_SES_DISCON_DET       (1 << 29)
#define USB_DWC_INTMSK_CON_ID_STS_CHG       (1 << 28)
#define USB_DWC_INTMSK_USB_SUSP             (1 << 11)
#define USB_DWC_INTMSK_RXFIFO_NOT_EMPT      (1 << 4)
#define USB_DWC_INTMSK_OTG                  (1 << 2)
#define USB_DWC_INTMSK_MODE_MISMTC          (1 << 1)

#define USB_DWC_RSTCTL_AHB_IDLE             (1 << 31)

/*#define USB_DWC_RSTCTL_SFT_RST            (1 << 1)  // origin*/
#define USB_DWC_RSTCTL_SFT_RST              (1 << 0)    /* jc bit0: Core Soft Reset (CSftRst)*/

#define USB_PHY_RST_AHB_IDLE_TIMEOUT        (100000*10) /* 100000*/
#define USB_PHY_RST_TIMEOUT                 (10000*10) /* 10000*/

/* jc*/
#undef ARMA9CTX_REGISTER_READ
#undef ARMA9CTX_REGISTER_WRITE
#define ARMA9CTX_REGISTER_READ(reg)   (*(volatile UINT32 *)(reg))
#define ARMA9CTX_REGISTER_WRITE(reg, data) *(volatile UINT32 *)(reg) = (data)

extern void sysMsDelay(int      delay);
extern void sysUsDelay(int delay);


/*******************************************************************************
*
* usbAltSocGen5USBPHYReset - reset the USB PHY for alt_soc_gen5
*
* This routine reset USB PHY for alt_soc_gen5.
*
* RETURNS: N/A.
*
* ERRNO: N/A
*/
void usbAltSocGen5USBPHYReset (void)
{
    UINT32 tempReg;
    UINT32 tempRst = 0;
    int i = 0;

    /* Wait for AHB master IDLE state. */    
    do 
	{
        sysUsDelay(10);
        tempRst = ARMA9CTX_REGISTER_READ(USB_0_BASE_ADRS + USB_DWC_GRSTCTL);

        if (++i > USB_PHY_RST_AHB_IDLE_TIMEOUT)
        {
            logMsg("\n USB PHY reset failed. AHB is busy.\n",1,2,3,4,5,6);
            return;
        }
    } while ((tempRst & (UINT32)USB_DWC_RSTCTL_AHB_IDLE) == 0);

    /* Core Soft Reset */    
    i = 0;
    tempRst |= USB_DWC_RSTCTL_SFT_RST;
    ARMA9CTX_REGISTER_WRITE(USB_0_BASE_ADRS + USB_DWC_GRSTCTL, tempRst);

    do 
	{
        tempRst = ARMA9CTX_REGISTER_READ(USB_0_BASE_ADRS + USB_DWC_GRSTCTL);
        if (++i > USB_PHY_RST_TIMEOUT) 
        {
            logMsg("\n USB PHY reset failed. \n",1,2,3,4,5,6);
            break;
        }
		
        sysUsDelay(1);
    } while ((tempRst & USB_DWC_RSTCTL_SFT_RST) != 0);

    sysMsDelay(100);
    /*taskDelay(100);  // jc*/

	return;
}

/*******************************************************************************
*
* usbAltSocGen5USBIntInit - initialize USB interrupt for alt_soc_gen5
*
* This routine initialize USB interrupt for alt_soc_gen5.
*
* RETURNS: N/A.
*
* ERRNO: N/A
*/

void usbAltSocGen5USBIntInit (void)
{
    UINT32 tempInitmsk = 0;

    /* Clear any pending OTG Interrupts */
    ARMA9CTX_REGISTER_WRITE(USB_0_BASE_ADRS+ USB_DWC_GOTGINT, 0xFFFFFFFF);

    /* Clear any pending interrupts */
    ARMA9CTX_REGISTER_WRITE(USB_0_BASE_ADRS+ USB_DWC_GINTSTS, 0xFFFFFFFF);

    /* Enable the interrupts in the GINTMSK. */
    tempInitmsk |= (UINT32)USB_DWC_INTMSK_MODE_MISMTC;
    tempInitmsk |= (UINT32)USB_DWC_INTMSK_OTG;
    tempInitmsk |= (UINT32)USB_DWC_INTMSK_RXFIFO_NOT_EMPT;
    tempInitmsk |= (UINT32)USB_DWC_INTMSK_USB_SUSP;
    tempInitmsk |= (UINT32)USB_DWC_INTMSK_CON_ID_STS_CHG;
    tempInitmsk |= (UINT32)USB_DWC_INTMSK_SES_DISCON_DET;
    tempInitmsk |= (UINT32)USB_DWC_INTMSK_NEW_SES_DET;
    tempInitmsk |= (UINT32)USB_DWC_INTMSK_WKP;

    ARMA9CTX_REGISTER_WRITE(USB_0_BASE_ADRS+ USB_DWC_GINTMSK, tempInitmsk);

	return;
}



/*******************************************************************************
*
* usbAltSocGen5USBPHYInit - initialize the USB PHY
*
* This routine initialize the USB PHY.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

void usbAltSocGen5USBPHYInit (void)
    {
    UINT32 i,temp32;
    UINT32 tempReg,usbHprt,hcfg;
    UINT32 ahbcfg = 0;
    UINT32 tempGusbcfg,usbcfg;
    static int usbInitDone = 0;
	
    printf("\n >usbAltSocGen5USBPHYInit \n\n");

    /* Common Initialization */
    tempGusbcfg = ARMA9CTX_REGISTER_READ(USB_0_BASE_ADRS + USB_DWC_GUSBCFG);  /* 0x00C*/
	
    /* Program the ULPI External VBUS bit if needed */    
    tempGusbcfg |= USB_DWC_USBCFG_ULPI_EXT_VBUS_DRV;
   /* tempGusbcfg |= (1 << 21);  // jc 1’b1: PHY uses external VBUS valid comparator.*/

    /* Set external TS Dline pulsing */    
    tempGusbcfg = tempGusbcfg & (~((UINT32) USB_DWC_USBCFG_TERM_SEL_DL_PULSE));
    ARMA9CTX_REGISTER_WRITE(USB_0_BASE_ADRS + USB_DWC_GUSBCFG, tempGusbcfg);

    /* Reset USB Controller */    
    usbAltSocGen5USBPHYReset();

    /* Initialize parameters from Hardware configuration registers. */
    tempGusbcfg = ARMA9CTX_REGISTER_READ(USB_0_BASE_ADRS + USB_DWC_GUSBCFG);
	
#if 0  /* jc: must be disable for vx69*/
    if (!usbInitDone) 
    {
        tempGusbcfg = ARMA9CTX_REGISTER_READ(USB_0_BASE_ADRS + USB_DWC_GUSBCFG);
        usbInitDone = 1;

        if (tempGusbcfg & USB_DWC_USBCFG_ULPI_UTMI_SEL) 
        {
            /* ULPI interface */
            tempGusbcfg |= USB_DWC_USBCFG_PHYIF;

            /* 8bit data bus */			
            tempGusbcfg &= ~((UINT32) USB_DWC_USBCFG_DDRSEL);  
			
			printf("...ULPI: 8bit data bus \n");   /* jc*/
        } 
        
        ARMA9CTX_REGISTER_WRITE(USB_0_BASE_ADRS+ USB_DWC_GUSBCFG, tempGusbcfg);

        /* Reset after setting the PHY parameters */        
        usbAltSocGen5USBPHYReset();
    }
#endif

    tempGusbcfg = ARMA9CTX_REGISTER_READ(USB_0_BASE_ADRS + USB_DWC_GUSBCFG);
    tempGusbcfg &= ~((UINT32) USB_DWC_USBCFG_ULPI_FSLS);
    tempGusbcfg &= ~((UINT32) USB_DWC_USBCFG_ULPI_CLK_SUS_M);
    ARMA9CTX_REGISTER_WRITE(USB_0_BASE_ADRS+ USB_DWC_GUSBCFG, tempGusbcfg);

    /* Program the GAHBCFG Register. */    
    ahbcfg = (ahbcfg & ~USB_DWC_AHBCFG_BURST_LEN(0xf)) \
             | USB_DWC_AHBCFG_BURST_LEN(USB_DWC_GAHBCFG_INT_DMA_BURST_INCR);

    /* enable DMA */    
    ahbcfg |= USB_DWC_AHBCFG_DMA_ENA;

    ARMA9CTX_REGISTER_WRITE(USB_0_BASE_ADRS+ USB_DWC_GAHBCFG, ahbcfg);
    
    /* Program the GUSBCFG register. */    
    tempGusbcfg = ARMA9CTX_REGISTER_READ(USB_0_BASE_ADRS + USB_DWC_GUSBCFG);

#if 0  /* origin*/
		tempGusbcfg |= USB_DWC_USBCFG_HNP_CAP;
		tempGusbcfg |= USB_DWC_USBCFG_SRP_CAP;
#else
		tempGusbcfg &= ~USB_DWC_USBCFG_HNP_CAP;
		tempGusbcfg &= ~USB_DWC_USBCFG_SRP_CAP;
#endif
    ARMA9CTX_REGISTER_WRITE(USB_0_BASE_ADRS+ USB_DWC_GUSBCFG, tempGusbcfg);

    /* Init USB interrupts */
    usbAltSocGen5USBIntInit();

    /* Disable the global interrupt   */    
    ahbcfg = ARMA9CTX_REGISTER_READ(USB_0_BASE_ADRS + USB_DWC_GAHBCFG);
    ahbcfg = ahbcfg & (~USB_DWC_AHBCFG_GLBL_INT_MASK);
    ARMA9CTX_REGISTER_WRITE(USB_0_BASE_ADRS+ USB_DWC_GAHBCFG, ahbcfg);
    
    /* Init USB HCD and set host mode */    
    usbcfg = ARMA9CTX_REGISTER_READ(USB_0_BASE_ADRS + USB_DWC_GUSBCFG);
    usbcfg |= USB_DWC_USBCFG_FRC_HST_MODE;
    ARMA9CTX_REGISTER_WRITE(USB_0_BASE_ADRS+ USB_DWC_GUSBCFG, usbcfg);

    /* Restart USB Phy Clock */    
    ARMA9CTX_REGISTER_WRITE(USB_0_BASE_ADRS+ USB_DWC_PCGCCTL, 0);

    /* Power USB PHY*/
    usbHprt = ARMA9CTX_REGISTER_READ(USB_0_BASE_ADRS + USB_DWC_HPRT);
    usbHprt = usbHprt & (~(0x1 << 2));
    usbHprt = usbHprt & (~(0x1 << 1));
    usbHprt = usbHprt & (~(0x1 << 3));
    usbHprt = usbHprt & (~(0x1 << 4));

    usbHprt = usbHprt | (0x1 << 12);
    
    ARMA9CTX_REGISTER_WRITE(USB_0_BASE_ADRS + USB_DWC_HPRT, usbHprt);

    /* Reset USB PHY */
    usbHprt = ARMA9CTX_REGISTER_READ(USB_0_BASE_ADRS + USB_DWC_HPRT);
    usbHprt = usbHprt & (~(0x1 << 2));
    usbHprt = usbHprt & (~(0x1 << 1));
    usbHprt = usbHprt & (~(0x1 << 3));
    usbHprt = usbHprt & (~(0x1 << 4));

    usbHprt = usbHprt | (0x1 << 8);
	
    ARMA9CTX_REGISTER_WRITE(USB_0_BASE_ADRS + USB_DWC_HPRT, usbHprt);

    sysMsDelay(50);
	/*taskDelay(60); // jc*/

    /* Release PHY from reset */
    usbHprt = ARMA9CTX_REGISTER_READ(USB_0_BASE_ADRS + USB_DWC_HPRT);
    usbHprt = usbHprt & (~(0x1 << 2));
    usbHprt = usbHprt & (~(0x1 << 1));
    usbHprt = usbHprt & (~(0x1 << 3));
    usbHprt = usbHprt & (~(0x1 << 4));

    usbHprt = usbHprt & (~(0x1 << 8));
    
    ARMA9CTX_REGISTER_WRITE(USB_0_BASE_ADRS + USB_DWC_HPRT, usbHprt);

	sysMsDelay(1000);
	/*taskDelay(120); // jc*/
	
    /* High speed PHY running at full speed or high speed USB_DWC_HCFG_30_60_MHZ */
    hcfg = ARMA9CTX_REGISTER_READ(USB_0_BASE_ADRS + USB_DWC_HCFG);
    hcfg = hcfg & (~(0x03));
    ARMA9CTX_REGISTER_WRITE(USB_0_BASE_ADRS+ USB_DWC_HCFG, hcfg);

    printf("\n <usbAltSocGen5USBPHYInitInternal \n\n");

    return;
}

UINT32 usbPciMemioOffset(void)
{
	return 0;
}

#if 1

#define  MISC   0xE0026000

#define MIO_PIN_28                  0x770
#define MIO_PIN_29                  0x774
#define MIO_PIN_30                  0x778
#define MIO_PIN_31                  0x77C
#define MIO_PIN_32                  0x780
#define MIO_PIN_33                  0x784
#define MIO_PIN_34                  0x788
#define MIO_PIN_35                  0x78C
#define MIO_PIN_36                  0x790
#define MIO_PIN_37                  0x794
#define MIO_PIN_38                  0x798
#define MIO_PIN_39                  0x79C

void usb_mio_init(void)
{
	printf("cfg mio of usb0 \n");

	FMSH_WriteReg(MISC, SLCR_UNLOCK, 0xDF0D767B);

	FMSH_WriteReg(MISC,MIO_PIN_28,0x1504);/*5);//28----data[4]   inout*/
	
	FMSH_WriteReg(MISC,MIO_PIN_29,0x1505);/*5);//29----dir  input*/
	FMSH_WriteReg(MISC,MIO_PIN_30,0x1504);/*5);//30----stp  */
	FMSH_WriteReg(MISC,MIO_PIN_31,0x1505);/*5);//31----nxt*/
	
	FMSH_WriteReg(MISC,MIO_PIN_32,0x1504);/*5);//32-35----data[0,1,2,3]*/
	FMSH_WriteReg(MISC,MIO_PIN_33,0x1504);/*5);//33*/
	FMSH_WriteReg(MISC,MIO_PIN_34,0x1504);/*5);//34*/
	FMSH_WriteReg(MISC,MIO_PIN_35,0x1504);/*5);//35*/
	
	FMSH_WriteReg(MISC,MIO_PIN_36,0x1505);/*5);//36----clk*/
	
	FMSH_WriteReg(MISC,MIO_PIN_37,0x1504);/*5);//37-39----data[5,6,7]*/
	FMSH_WriteReg(MISC,MIO_PIN_38,0x1504);/*5);//38*/
	FMSH_WriteReg(MISC,MIO_PIN_39,0x1504);/*5);//39*/
	
	FMSH_WriteReg(MISC, SLCR_LOCK, 0xDF0D767B);

	/*
	printf("cfg usb1 \n");
	
	FMSH_WriteReg(MISC, MIO_PIN_40, 0x1504);
	FMSH_WriteReg(MISC, MIO_PIN_41, 0x1505);
	FMSH_WriteReg(MISC, MIO_PIN_42, 0x1504);
	FMSH_WriteReg(MISC, MIO_PIN_43, 0x1505);
	FMSH_WriteReg(MISC, MIO_PIN_44, 0x1504);
	FMSH_WriteReg(MISC, MIO_PIN_45, 0x1504);
	FMSH_WriteReg(MISC, MIO_PIN_46, 0x1504);
	FMSH_WriteReg(MISC, MIO_PIN_47, 0x1504);
	FMSH_WriteReg(MISC, MIO_PIN_48, 0x1505);
	FMSH_WriteReg(MISC, MIO_PIN_49, 0x1504);
	FMSH_WriteReg(MISC, MIO_PIN_50, 0x1504);
	FMSH_WriteReg(MISC, MIO_PIN_51, 0x1504);
	FMSH_WriteReg(MISC, SLCR_LOCK, 0xDF0D767B);
	*/
	return;
}

#endif


/*
USB_CLK_CTRL	0x31C	reserved	31:4	rw	0x0	Reserved. 写无效，读为0
		USB1_PHY_CLKACT	3	rw	0x1	"USB1 PHY时钟域使能
							0：不使能
							1：使能"
		USB0_PHY_CLKACT	2	rw	0x1	"USB0 PHY时钟域使能
							0：不使能
							1：使能"
		USB1_AHB_CLKACT	1	rw	0x1	"USB1 AHB时钟域使能
							0：不使能
							1：使能"
		USB0_AHB_CLKACT	0	rw	0x1	"USB0 AHB时钟域使能
							0：不使能
							1：使能"
*/
void sysUsbInit(void)
{
	slcr_write(0x31C, 0x05);  /* 使能 usb_0*/
	
	/*usb_mio_init();*/
	
	return;
}



#endif /* End of INCLUDE_SYNOPSYSHCI */


