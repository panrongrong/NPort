/* usbCaviumHcdRegisterInfo.h - CAVIUM specific definitions for Synopsys HCD */

/*
 * Copyright (c) 2009 Wind River Systems, Inc.
 *
 * The right to copy, distribute, modify or otherwise make use
 * of this software may be licensed only pursuant to the terms
 * of an applicable Wind River license agreement.
 */

/*
modification history
--------------------
01a,11sep09,m_y  written.
*/

/*
DESCRIPTION

This file contains the register definitions for CAVIUM chips using Synopsys
Dual-Role USB Controller IP.

*/

#ifndef __INCCaviumHcdRegisterInfoh
#define __INCCaviumHcdRegisterInfoh

#ifdef  __cplusplus
extern "C" {
#endif


/* The following register definitions are only specific for CAVIUM chips */

#define USB_CAVIUMHCD_USBN_INT_SUM                    (0x0001180068000000ull)
#define USB_CAVIUMHCD_USBN_INT_ENB                    (0x0001180068000008ull)
#define USB_CAVIUMHCD_USBN_BIST_STATUS                (0x00011800680007F8ull)
#define USB_CAVIUMHCD_USBN_CTL_STATUS                 (0x00016F0000000800ull)
#define USB_CAVIUMHCD_USBN_DMA_TEST                   (0x00016F0000000808ull)
#define USB_CAVIUMHCD_USBN_DMA0_INB_CHN0              (0x00016F0000000818ull)
#define USB_CAVIUMHCD_USBN_DMA0_INB_CHN1              (0x00016F0000000820ull)
#define USB_CAVIUMHCD_USBN_DMA0_INB_CHN2              (0x00016F0000000828ull)
#define USB_CAVIUMHCD_USBN_DMA0_INB_CHN3              (0x00016F0000000830ull)
#define USB_CAVIUMHCD_USBN_DMA0_INB_CHN4              (0x00016F0000000838ull)
#define USB_CAVIUMHCD_USBN_DMA0_INB_CHN5              (0x00016F0000000840ull)
#define USB_CAVIUMHCD_USBN_DMA0_INB_CHN6              (0x00016F0000000848ull)
#define USB_CAVIUMHCD_USBN_DMA0_INB_CHN7              (0x00016F0000000850ull)

#define USB_CAVIUMHCD_USBN_DMA0_OUTB_CHN0             (0x00016F0000000858ull)
#define USB_CAVIUMHCD_USBN_DMA0_OUTB_CHN1             (0x00016F0000000860ull)
#define USB_CAVIUMHCD_USBN_DMA0_OUTB_CHN2             (0x00016F0000000868ull)
#define USB_CAVIUMHCD_USBN_DMA0_OUTB_CHN3             (0x00016F0000000870ull)
#define USB_CAVIUMHCD_USBN_DMA0_OUTB_CHN4             (0x00016F0000000878ull)
#define USB_CAVIUMHCD_USBN_DMA0_OUTB_CHN5             (0x00016F0000000880ull)
#define USB_CAVIUMHCD_USBN_DMA0_OUTB_CHN6             (0x00016F0000000888ull)
#define USB_CAVIUMHCD_USBN_DMA0_OUTB_CHN7             (0x00016F0000000890ull)

#ifdef  __cplusplus
}
#endif

#endif /* __INCCaviumHcdRegisterInfoh */

/* End of file */
