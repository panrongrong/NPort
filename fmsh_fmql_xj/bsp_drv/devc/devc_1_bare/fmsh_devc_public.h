/******************************************************************************
*
* Copyright (C) 2018 - 2028 FMSH, Inc.  All rights reserved.
*
******************************************************************************/
/******************************************************************************
*
* @file  fmsh_devc_public.h
*
* This file contains public constant & function define
*
* @note		None.
*
* MODIFICATION HISTORY:
*
*<pre>
* Ver   Who  Date     Changes
* ----- ---- -------- ---------------------------------------------
* 0.01   wfb  11/23/2018  First Release
*</pre>
******************************************************************************/

#ifndef _FMSH_DEVC_PUBLIC_H_		/* prevent circular inclusions */
#define _FMSH_DEVC_PUBLIC_H_		/* by using protection macros */

#ifdef __cplusplus
extern "C" {
#endif
 
/***************************** Include Files *********************************/
#include "../../common/fmsh_common.h"

#include "fmsh_devc_common.h"


/************************** Constant Definitions *****************************/
        
/*FOR FMSH 325T MUST PATCH IT USING PROCISE DEVELOPED BY FMSH LTD BEFORE USE IT*/
#define FMSH_325T /********for FMSH 325T, must open the define********/
/*#define TEST_TIME /*****or output dma carry time, must open the define****/*/

/* max count for waiting */
#define DEVC_MAX_COUNT (0x0FFFFFFF)  /* 0x0FFF_FFFF*/
#define MAX_COUNT      (DEVC_MAX_COUNT)  /* 0x07FFFFFF*/

/* bitstream downloiad destination address */    
#define PCAP_WR_DATA_ADDR  (0xFFFFFFFF)  /* 0xFFFF_FFFF*/
    
/* bitstream readback source address */   
#define PCAP_RD_DATA_ADDR  (0xFFFFFFFF)  /* 0xFFFF_FFFF*/
#define FMSH_DMA_INVALID_ADDRESS         PCAP_RD_DATA_ADDR
    
/* Types of PCAP transfers */
#define FMSH_NON_SECURE_PCAP_WRITE		    0
#define FMSH_SECURE_PCAP_WRITE			    1
#define FMSH_PCAP_READBACK			        2
#define FMSH_PCAP_LOOPBACK			        3
#define FMSH_NON_SECURE_PCAP_WRITE_DUMMMY	4
    
/*
 * Addresses of the Configuration Registers
 */
#define CRC		0	/* Status Register */
#define FAR		1	/* Frame Address Register */
#define FDRI		2	/* FDRI Register */
#define FDRO		3	/* FDRO Register */
#define CMD		4	/* Command Register */
#define CTL0		5	/* Control Register 0 */
#define MASK		6	/* MASK Register */
#define STAT		7	/* Status Register */
#define LOUT		8	/* LOUT Register */
#define COR0		9	/* Configuration Options Register 0 */
#define MFWR		10	/* MFWR Register */
#define CBC		11	/* CBC Register */
#define IDCODE		12	/* IDCODE Register */
#define AXSS		13	/* AXSS Register */
#define COR1		14	/* Configuration Options Register 1 */
#define WBSTAR		16	/* Warm Boot Start Address Register */
#define TIMER		17	/* Watchdog Timer Register */
#define BOOTSTS		22	/* Boot History Status Register */
#define CTL1		24	/* Control Register 1 */

/**************************** Type Definitions *******************************/
    
/***************** Macros (Inline Functions) Definitions *********************/  
        
/************************** Variable Definitions *****************************/

/************************** Function Prototypes ******************************/

u8 FDevcPs_unLockCSU(FDevcPs_T *dev);
u8 FDevcPs_lockCSU(FDevcPs_T *dev);
FDevcPs_Config *FDevcPs_LookupConfig(u16 DeviceId);
u8 FDevcPs_init(FDevcPs_T *dev, FDevcPs_Config *cfg);
u8 FDevcPs_fabricInit(FDevcPs_T *dev, u32 TransferType);

u8 FDevcPs_KUPKEY(FDevcPs_T *dev, u32 *p, u32 len);
u8 FDevcPs_DEVKEY(FDevcPs_T *dev, u32 *p, u32 len);
u8 FDevcPs_IV(FDevcPs_T *dev, u32 *p, u32 len);

u8 FDevcPs_pollFpgaDone(FDevcPs_T *dev, u32 maxcount);

u8 FDevcPs_getConfigdata(FDevcPs_T *dev, 
                           u32 *DestinationDataPtr, 
                           u32 DestinationLength, 
                           u32 addr, 
                           u32 ConfigReg);

u8 FDevcPs_pcapLoadPartition(FDevcPs_T *dev, 
                               u32 *SourceDataPtr, 
                               u32 *DestinationDataPtr, 
                               u32 SourceLength, 
                               u32 DestinationLength, 
                               u32 SecureTransfer);

u8 FDevcPs_keyRollingDownload(FDevcPs_T *dev, u8 alg_flag, u8 opkey_flag, u32 *srcPtr, u32 bitlen);
                           
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* end of protection macro */

