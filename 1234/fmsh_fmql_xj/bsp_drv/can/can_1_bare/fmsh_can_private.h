/******************************************************************************
*
* Copyright (C) 2018 - 2028 FMSH, Inc.  All rights reserved.
*
******************************************************************************/
/******************************************************************************
*
* @file  fmsh_can_private.h
*
* This file contains private constant & function define
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

#ifndef _FMSH_CAN_PRIVATE_H_
#define _FMSH_CAN_PRIVATE_H_

#ifdef __cplusplus
extern "C" {
#endif
 
/***************************** Include Files *********************************/    
        
#include "fmsh_can_common.h"

/************************** Constant Definitions *****************************/

/**
 * This macro is used to hardcode the APB data accesses, 
 */     
#define CAN_INP FMSH_IN32_32
#define CAN_OUTP FMSH_OUT32_32

/**   
 * DESCRIPTION
 *  Used in conjunction with fmsh_common_bitops.h to access register
 *  bitfields.  They are defined as bit offset/mask pairs for each gpio
 *  register bitfield.
 * NOTES
 *  bfo is the offset of the bitfield with respect to LSB;
 *  bfw is the width of the bitfield
 */
/* cdr register  */
#define bfoCAN_CDR_CDR               0   
#define bfwCAN_CDR_CDR               3

#define bfoCAN_CDR_ClockOff          3  
#define bfwCAN_CDR_ClockOff          1  

/* ocr register  */
#define bfoCAN_OCR_OCMODE            0   
#define bfwCAN_OCR_OCMODE            2

/* mode register  */
#define bfoCAN_MODE_RM            0   
#define bfwCAN_MODE_RM            1

#define bfoCAN_MODE_LOM           1   
#define bfwCAN_MODE_LOM           1  

#define bfoCAN_MODE_STM           2   
#define bfwCAN_MODE_STM           1  

#define bfoCAN_MODE_AFM           3   
#define bfwCAN_MODE_AFM           1  

#define bfoCAN_MODE_SM            4   
#define bfwCAN_MODE_SM            1 

/* sr register  */
#define bfoCAN_SR_RBS           0   
#define bfwCAN_SR_RBS           1

#define bfoCAN_SR_DOS           1   
#define bfwCAN_SR_DOS           1

#define bfoCAN_SR_TBS           2   
#define bfwCAN_SR_TBS           1

#define bfoCAN_SR_TCS           3   
#define bfwCAN_SR_TCS           1   

#define bfoCAN_SR_RS           4   
#define bfwCAN_SR_RS           1   

#define bfoCAN_SR_TS           5   
#define bfwCAN_SR_TS           1   

#define bfoCAN_SR_ES           6   
#define bfwCAN_SR_ES           1  

#define bfoCAN_SR_BS           7   
#define bfwCAN_SR_BS           1  

/* cmr register  */
#define bfoCAN_CMR_TR           0   
#define bfwCAN_CMR_TR           1

#define bfoCAN_CMR_AT           1   
#define bfwCAN_CMR_AT           1

#define bfoCAN_CMR_RRB           2   
#define bfwCAN_CMR_RRB           1   

#define bfoCAN_CMR_CDO           3   
#define bfwCAN_CMR_CDO           1   

#define bfoCAN_CMR_SRR           4   
#define bfwCAN_CMR_SRR           1   

/* ier register  */
#define bfoCAN_IER_RIE           0   
#define bfwCAN_IER_RIE           1    

#define bfoCAN_IER_TIE           1   
#define bfwCAN_IER_TIE           1    

#define bfoCAN_IER_EIE           2   
#define bfwCAN_IER_EIE           1     

#define bfoCAN_IER_DOIE           3   
#define bfwCAN_IER_DOIE           1     

#define bfoCAN_IER_WUIE           4  
#define bfwCAN_IER_WUIE           1    

#define bfoCAN_IER_EPIE           5   
#define bfwCAN_IER_EPIE           1    

#define bfoCAN_IER_ALIE           6   
#define bfwCAN_IER_ALIE           1    

#define bfoCAN_IER_BEIE           7   
#define bfwCAN_IER_BEIE           1  

/* ir register  */
#define bfoCAN_IR_RI           0   
#define bfwCAN_IR_RI           1    

#define bfoCAN_IR_TI           1   
#define bfwCAN_IR_TI           1     

#define bfoCAN_IR_EI           2   
#define bfwCAN_IR_EI           1    

#define bfoCAN_IR_DOI           3   
#define bfwCAN_IR_DOI           1    

#define bfoCAN_IR_WUI           4  
#define bfwCAN_IR_WUI           1     

#define bfoCAN_IR_EPI           5   
#define bfwCAN_IR_EPI           1     

#define bfoCAN_IR_ALI           6   
#define bfwCAN_IR_ALI           1    

#define bfoCAN_IR_BEI           7   
#define bfwCAN_IR_BEI           1   

/* BRT0 register  */
#define bfoCAN_BRT0_BRP          0   
#define bfwCAN_BRT0_BRP          6     

/* BRT1 register  */
#define bfoCAN_BRT1_SEG1          0   
#define bfwCAN_BRT1_SEG1          4    

#define bfoCAN_BRT1_SEG2          4   
#define bfwCAN_BRT1_SEG2          3 
 
/**************************** Type Definitions *******************************/

/**
 * DESCRIPTION
 *  This is the structure used for accessing the gpio memory map.
 */
typedef struct FCanPs_Portmap
{
    volatile u32 mode;  /*0x00*/
    volatile u32 cmr;  /*0x04   */
    volatile u32 sr;  /*0x08*/
    volatile u32 ir;  /*0x0c*/
    volatile u32 ier;  /*0x10   */
    volatile u32 rsv1;  /*0x14    */
    volatile u32 btr0;  /*0x18*/
    volatile u32 btr1;  /* 0x1c  */
    volatile u32 ocr;  /*0x20*/
    volatile u32 rsv2;  /*0x24*/
    volatile u32 rsv3;  /*0x28   */
    volatile u32 alc;  /*0x2c    */
    volatile u32 ecc;  /*0x30*/
    volatile u32 ewlr;  /*0x34   */
    volatile u32 rxerr;  /*0x38*/
    volatile u32 txerr;  /*0x3c*/
    volatile u32 acr0_txrxbuf;  /*0x40 */
    volatile u32 acr1_txrxbuf;  /*0x44 */
    volatile u32 acr2_txrxbuf;  /*0x48*/
    volatile u32 acr3_txrxbuf;  /*0x4c*/
    volatile u32 amr0_txrxbuf;  /*0x50*/
    volatile u32 amr1_txrxbuf;  /*0x54*/
    volatile u32 amr2_txrxbuf;  /*0x58   */
    volatile u32 amr3_txrxbuf;  /*0x5c  */
    volatile u32 txrxbuf[5];  /*0x60~0x70 */
    volatile u32 rmc;  /* 0x74*/
    volatile u32 rbsa;  /*0x78*/
    volatile u32 cdr;  /*0x7c  */
    volatile u32 rxfifo[0x40];  /**/
    volatile u32 txfifo[0x11];  /**/
    volatile u32 rsv4[0x13];  /*     */
}FCanPs_Portmap_T;        

/***************** Macros (Inline Functions) Definitions *********************/  
        
/************************** Variable Definitions *****************************/

/************************** Function Prototypes ******************************/

static int mcan_set_bittiming(FCanPs_T *dev);
static int can_calc_bittiming(FCanPs_T *dev);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* end of protection macro */i