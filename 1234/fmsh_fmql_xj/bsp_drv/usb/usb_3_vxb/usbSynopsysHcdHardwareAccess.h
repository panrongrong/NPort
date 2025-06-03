/* usbSynopsysHcdHardwareAccess.h - hardware access routines for Synopsys HCD */

/*
 * Copyright (c) 2009, 2013 Wind River Systems, Inc.
 *
 * The right to copy, distribute, modify, or otherwise make use
 * of this software may be licensed only pursuant to the terms
 * of an applicable Wind River license agreement.
 */

/*
modification history
--------------------
01b,10jul13,ljg  add alt_soc_gen5 support
01a,09nov09,m_y  written.
*/

/*
DESCRIPTION

This contains some basic routines which handle the Synopsys USB
host controller hardware.

*/

#ifndef __INCSynopsysHcdHardwareAccessh
#define __INCSynopsysHcdHardwareAccessh

#ifdef __cplusplus
extern "C" {
#endif

#ifdef ARMBE8
#    define SWAP32 vxbSwap32
#else
#    define SWAP32 
#endif /* ARMBE8 */


/* This macro writes the 32-bit value to the reigiter */

#if CPU==MIPSI64R2
#define USB_SYNOPSYSHCD_WRITE32_REG(pSynopsysHcdData, offset, value)            \
    pSynopsysHcdData->pRegWrite32Handle((pSynopsysHcdData->regBase + offset),   \
                                        value)
#else
#define USB_SYNOPSYSHCD_WRITE32_REG(pSynopsysHcdData, offset, value)            \
    vxbWrite32(((pUSB_SYNOPSYSHCD_DATA)(pSynopsysHcdData))->pRegWrite32Handle,  \
        (void *)(((pUSB_SYNOPSYSHCD_DATA)(pSynopsysHcdData))->regBase + offset),\
        SWAP32(value))
#endif

/* This macro gets the 32-bit value from the reigiter */

#if CPU==MIPSI64R2
#define USB_SYNOPSYSHCD_READ32_REG(pSynopsysHcdData, offset)                    \
    pSynopsysHcdData->pRegRead32Handle(pSynopsysHcdData->regBase + offset)
#else
#define USB_SYNOPSYSHCD_READ32_REG(pSynopsysHcdData, offset)                        \
    SWAP32(vxbRead32(((pUSB_SYNOPSYSHCD_DATA)(pSynopsysHcdData))->pRegRead32Handle, \
         (void *)(((pUSB_SYNOPSYSHCD_DATA)(pSynopsysHcdData))->regBase + offset))) 
#endif

#if CPU==MIPSI64R2
/* This macro writes the 64-bit value to the reigiter */

#define USB_SYNOPSYSHCD_WRITE64_REG(pSynopsysHcdData, offset, value)           \
    pSynopsysHcdData->pRegWrite64Uint64Handle(                                 \
                                        (pSynopsysHcdData->regBase + offset),  \
                                        value)

/* This macro gets the 64-bit value from the reigiter */

#define USB_SYNOPSYSHCD_READ64_REG(pSynopsysHcdData, offset)                   \
    pSynopsysHcdData->pRegRead64Uint64Handle(                                  \
                                         pSynopsysHcdData->regBase + offset)
#endif

/* This macro uses mask code to mask the related register value */

#define USB_SYNOPSYSHCD_SETBITS32_REG(pSynopsysHcdData, offset, mask)          \
    {                                                                          \
    UINT32 value = 0x00;                                                       \
    value = USB_SYNOPSYSHCD_READ32_REG(pSynopsysHcdData, offset);              \
    value = value | mask;                                                      \
    USB_SYNOPSYSHCD_WRITE32_REG(pSynopsysHcdData, offset, value);              \
    }

/* This macro uses mask code to un-mask the related register value */

#define USB_SYNOPSYSHCD_CLEARBITS32_REG(pSynopsysHcdData, offset, mask)        \
    {                                                                          \
    UINT32 value = 0x00;                                                       \
    value = USB_SYNOPSYSHCD_READ32_REG(pSynopsysHcdData, offset);              \
    value = value & (~mask);                                                   \
    USB_SYNOPSYSHCD_WRITE32_REG(pSynopsysHcdData, offset, value);              \
    }

#define USB_SYNOPSYSHCD_SETBITS32_HPRT_REG(pSynopsysHcdData, offset, mask)     \
    {                                                                          \
    UINT32 value = 0x00;                                                       \
    value = USB_SYNOPSYSHCD_READ32_REG(pSynopsysHcdData, offset);              \
    value = value & (~(0x3f));                                                 \
    value = value | mask;                                                      \
    USB_SYNOPSYSHCD_WRITE32_REG(pSynopsysHcdData, offset, value);              \
    }

/* This macro uses mask code to un-mask the related register value */

#define USB_SYNOPSYSHCD_CLEARBITS32_HPRT_REG(pSynopsysHcdData, offset, mask)   \
    {                                                                          \
    UINT32 value = 0x00;                                                       \
    value = USB_SYNOPSYSHCD_READ32_REG(pSynopsysHcdData, offset);              \
    value = value & (~(0x3f));                                                 \
    value = value & (~mask);                                                   \
    USB_SYNOPSYSHCD_WRITE32_REG(pSynopsysHcdData, offset, value);              \
    }

/* This macro programes the DMA address */

#if CPU==MIPSI64R2
#define USB_SYNOPSYSHCD_PROGRAM_DMA_ADDRESS(pSynopsysHcdData,offset,address)   \
    {                                                                          \
    UINT64 dmaAddr = 0;                                                        \
    dmaAddr = (pSynopsysHcdData->regBase + offset) & ~(0x10000000);            \
    pSynopsysHcdData->pRegWrite64Uint64Handle(dmaAddr, address);               \
    pSynopsysHcdData->pRegRead64Uint64Handle(dmaAddr);                         \
    }
#else
#define USB_SYNOPSYSHCD_PROGRAM_DMA_ADDRESS(pSynopsysHcdData,offset,address)   \
    {                                                                          \
    USB_SYNOPSYSHCD_WRITE32_REG(pSynopsysHcdData,offset,address);              \
    }
#endif

VOID usbSynopsysHcdFlushTxFIFO
    (
    pUSB_SYNOPSYSHCD_DATA pSynopsysHcdData,
    int TxFIFONum
    );
VOID usbSynopsysHcdFlushRxFIFO
    (
    pUSB_SYNOPSYSHCD_DATA pSynopsysHcdData
    );
VOID usbSynopsysHCDCoreInit
    (
    pUSB_SYNOPSYSHCD_DATA pHCDData
    );
VOID usbSynopsysHCDHostInit
    (
    pUSB_SYNOPSYSHCD_DATA pHCDData
    );

#ifdef __cplusplus
}
#endif

#endif /* __INCSynopsysHcdHardwareAccessh*/

/* End of file */
