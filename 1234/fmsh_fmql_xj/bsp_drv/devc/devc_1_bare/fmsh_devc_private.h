/******************************************************************************
*
* Copyright (C) 2018 - 2028 FMSH, Inc.  All rights reserved.
*
******************************************************************************/
/******************************************************************************
*
* @file  fmsh_devc_private.h
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

#ifndef _FMSH_DEVC_PRIVATE_H_ /* prevent circular inclusions */
#define _FMSH_DEVC_PRIVATE_H_ /* by using protection macros */

#ifdef __cplusplus
extern "C" {
#endif
 
/***************************** Include Files *********************************/  
        
#include "fmsh_devc_common.h"

/************************** Constant Definitions *****************************/

/**
 * This macro is used to hardcode the APB data accesses, 
 */     
#define DEVC_INP FMSH_IN32_32
#define DEVC_OUTP FMSH_OUT32_32
 
/**   
 * DESCRIPTION
 *  Used in conjunction with fmsh_common_bitops.h to access register
 *  bitfields.  They are defined as bit offset/mask pairs for each gpio
 *  register bitfield.
 * NOTES
 *  bfo is the offset of the bitfield with respect to LSB;
 *  bfw is the width of the bitfield
 */
/*SAC CTRL register  */
#define bfoDEVC_CTRL_encrypted_en             9   
#define bfwDEVC_CTRL_encrypted_en             3       
        
/*SAC CFG register  */
#define bfoDEVC_CFG_RDWR_B              0   
#define bfwDEVC_CFG_RDWR_B              1
#define bfoDEVC_CFG_CSI_B               1  
#define bfwDEVC_CFG_CSI_B               1  
#define bfoDEVC_CFG_WCLK_EDGE           4 
#define bfwDEVC_CFG_WCLK_EDGE           1          
#define bfoDEVC_CFG_RCLK_EDGE           5 
#define bfwDEVC_CFG_RCLK_EDGE           1           
#define bfoDEVC_CFG_WFIFO_TH            6 
#define bfwDEVC_CFG_WFIFO_TH            2          
#define bfoDEVC_CFG_RFIFO_TH            8 
#define bfwDEVC_CFG_RFIFO_TH            2 
#define bfoDEVC_CFG_READBACK_DUMMY_NUM           13 
#define bfwDEVC_CFG_READBACK_DUMMY_NUM            3       
#define bfoDEVC_CFG_smap32_swap_ctrl         16 
#define bfwDEVC_CFG_smap32_swap_ctrl          1         
#define bfoDEVC_CFG_rxfifo_di_data_swap         17 
#define bfwDEVC_CFG_rxfifo_di_data_swap          2       
#define bfoDEVC_CFG_CSU_DMA_SWITCH       20 
#define bfwDEVC_CFG_CSU_DMA_SWITCH        4         
#define bfoDEVC_CFG_txfifo_do_data_swap      24 
#define bfwDEVC_CFG_txfifo_do_data_swap      2                   
#define bfoDEVC_CFG_sec_down_data_byte_swap     28 
#define bfwDEVC_CFG_sec_down_data_byte_swap     2          
#define bfoDEVC_CFG_pcap_mode_en     30 
#define bfwDEVC_CFG_pcap_mode_en     1          
#define bfoDEVC_CFG_program_b     31 
#define bfwDEVC_CFG_program_b     1            
 
/*SAC INT STATUS register  */
#define bfoDEVC_INT_STATUS_pcfg_done_int           2  
#define bfwDEVC_INT_STATUS_pcfg_done_int           1
#define bfoDEVC_INT_STATUS_dp_done_int            12 
#define bfwDEVC_INT_STATUS_dp_done_int            1  
#define bfoDEVC_INT_STATUS_dma_done_int            13 
#define bfwDEVC_INT_STATUS_dma_done_int            1          
        
/*SAC STATUS register  */
#define bfoDEVC_STATUS_pcfg_init            4  
#define bfwDEVC_STATUS_pcfg_init            1
#define bfoDEVC_STATUS_pcfg_done            9  
#define bfwDEVC_STATUS_pcfg_done            1        
#define bfoDEVC_STATUS_dma_busy            31  
#define bfwDEVC_STATUS_dma_busy            1  
        
/*GCM CTRL register  */
#define bfoDEVC_GCM_CTRL_gcm_en            0  
#define bfwDEVC_GCM_CTRL_gcm_en            1          
#define bfoDEVC_GCM_CTRL_alg_sel           1  
#define bfwDEVC_GCM_CTRL_alg_sel           1             
#define bfoDEVC_GCM_CTRL_mode           3  
#define bfwDEVC_GCM_CTRL_mode           2           
#define bfoDEVC_GCM_CTRL_chmode           5  
#define bfwDEVC_GCM_CTRL_chmode           2         
        
/*key src register  */
#define bfoKEY_SRC_key_src            0  
#define bfwKEY_SRC_key_src            4               
       
/*dec flag register  */
#define bfoDEC_FLAG_flag        0  
#define bfwDEC_FLAG_flag        4          

/**
 * offset address
 */
#define EFUSE_AES_KEY_RD_DIS_OFFSET 0xfc4 
      
/*mask define*/
#define FMSH_DEVC_ERROR_FLAGS_MASK	 0x00148040      
    
/**
 * @name Configuration Type1 packet headers masks
 */
#define XDC_TYPE_SHIFT			29
#define XDC_REGISTER_SHIFT		13
#define XDC_OP_SHIFT			27
#define XDC_TYPE_1			1
#define OPCODE_READ			1

/**************************** Type Definitions *******************************/

/**
 * DESCRIPTION
 *  This is the structure used for accessing the gpio memory map.
 */
typedef struct FMSH_devc_portmap
{  
    volatile u32 rsv1[2];  /*0x00 ~ 0x04   */
    volatile u32 sac_ctrl;  /*0x08       */
    volatile u32 sac_cfg;  /*0x0c            */
    volatile u32 sac_int_sts;  /*0x10         */
    volatile u32 sac_int_mask;  /*0x14          */
    volatile u32 sac_status;  /*0x18           */
    volatile u32 csu_dma_src_addr;  /*0x1c          */
    volatile u32 csu_dma_dest_addr;  /*0x20       */
    volatile u32 csu_dma_src_len;  /*0x24         */
    volatile u32 csu_dma_dest_len;  /*0x28          */
    volatile u32 csu_unlock;  /*0x2c          */
    volatile u32 rsv2[2];  /*0x30 ~ 0x34         */
    volatile u32 csu_mutiboot;  /*0x38 */
    volatile u32 sac_lock;  /*0x3c      */
    volatile u32 xadc_status;  /*0x40*/
    volatile u32 dev_key0;  /*0x44       */
    volatile u32 dev_key1;  /*0x48           */
    volatile u32 dev_key2;  /*0x4c          */
    volatile u32 dev_key3;  /*0x50          */
    volatile u32 dev_key4;  /*0x54       */
    volatile u32 dev_key5;  /*0x58           */
    volatile u32 dev_key6;  /*0x5c           */
    volatile u32 dev_key7;  /*0x60       */
    volatile u32 multh_H0;  /*0x64       */
    volatile u32 multh_H1;  /*0x68           */
    volatile u32 multh_H2;  /*0x6c           */
    volatile u32 multh_H3;  /*0x70          */
    volatile u32 multh_tag0;  /*0x74       */
    volatile u32 multh_tag1;  /*0x78           */
    volatile u32 multh_tag2;  /*0x7c           */
    volatile u32 multh_tag3;  /*0x80           */
    volatile u32 key_iv_load;  /*0x84            */
    volatile u32 key_src;  /*0x88             */
    volatile u32 key_iv_clear;  /*0x8c            */
    volatile u32 dec_flag;  /*0x90*/
    volatile u32 puf_cmd;  /*0x94        */
    volatile u32 puf_status;  /*0x98        */
    volatile u32 puf_iv0;  /*0x9c   */
    volatile u32 puf_iv1;  /*0xa0 */
    volatile u32 puf_iv2;  /*0xa4*/
    volatile u32 puf_iv3;  /*0xa8 */
    volatile u32 puf_red_key0;  /*0xac */
    volatile u32 puf_red_key1;  /*0xb0 */
    volatile u32 puf_red_key2;  /*0xb4 */
    volatile u32 puf_red_key3;  /*0xb8 */
    volatile u32 puf_red_key4;  /*0xbc*/
    volatile u32 puf_red_key5;  /*0xc0 */
    volatile u32 puf_red_key6;  /*0xc4 */
    volatile u32 puf_red_key7;  /*0xc8     */
    volatile u32 apu_exc_status;  /*0xcc    */
    volatile u32 apu_exc_addr;  /*0xd0*/
    volatile u32 csu_rst_crtl;  /*0xd4        */
    volatile u32 key_crc;  /*0xd8*/
    volatile u32 bitstream_cnt;  /*0xdc*/
    volatile u32 bitstream_checksum;  /*0xe0*/
    volatile u32 csu_error_code;  /*0xe4*/
    volatile u32 ot_ps_rst_en;  /*0xe8  */
    volatile u32 rsv3[5];  /*0xec ~ 0xfc          */
    volatile u32 gcm_ctrl;  /*0x100*/
    volatile u32 gcm_status;  /*0x104*/
    volatile u32 rsv4[2];  /*0x108 ~ 0x10C     */
    volatile u32 KUP0;  /*0x110   */
    volatile u32 KUP1;  /*0x114   */
    volatile u32 KUP2;  /*0x118   */
    volatile u32 KUP3;  /*0x11c   */
    volatile u32 KUP4;  /*0x120   */
    volatile u32 KUP5;  /*0x124   */
    volatile u32 KUP6;  /*0x128   */
    volatile u32 KUP7;  /*0x12c  */
    volatile u32 IVUP0;  /*0x130   */
    volatile u32 IVUP1;  /*0x134   */
    volatile u32 IVUP2;  /*0x138   */
    volatile u32 IVUP3;  /*0x13c  */
}FMSH_devc_portmap_TypeDef;   

/*download mode*/ 
enum download_mode 
{
    DOWNLOAD_BITSTREAM = 0x0,
    READBACK_BITSTREAM = 0x2, 
    DATA_LOOPBACK = 0x3,     
    SECURE_DOWNLOAD_BITSTREAM = 0x08   
};

/* data swap mode */  
enum data_swap
{
    none_swap = 0x0,
    half_word_swap = 0x1,   
    byte_swap = 0x2,
    bit_swap = 0x3  
};

/* smap32 swap */
enum smap32_swap
{
    smap32_swap_disable = 0x0,
    smap32_swap_enable = 0x1   
};

/* readback dummy number */
enum dummy_num 
{
    dummy_0 = 0x0,
    dummy_1 = 0x1,  
    dummy_2 = 0x2,
    dummy_3 = 0x3,   
    dummy_4 = 0x4,
    dummy_5 = 0x5,   
    dummy_6 = 0x6,
    dummy_7 = 0x7,          
};

/* set read fifo, it is used during in download */
enum readFifoThre 
{
    readFifoThre_hex_0x40 = 0x0,/*default*/
    readFifoThre_hex_0x80 = 0x1,
    readFifoThre_hex_0xc0 = 0x2,
    readFifoThre_hex_0x100 = 0x3
};

/* set write fifo, it is used during in readback */
enum writeFifoThre 
{
    writeFifoThre_hex_0x80 = 0x0,/*default*/
    writeFifoThre_hex_0x60 = 0x1,
    writeFifoThre_hex_0x40 = 0x2,
    writeFifoThre_hex_0x10 = 0x3
};

/* setsmap32 interface clock edge rising or falling */
enum clk_edge 
{
    rising_edge = 0x1,
    falling_edge = 0x0    
};

/* algorithm type */
enum ALG
{
    AES = 0x0,
    SM4 = 0x1,
    ALG_NONE = 0x2
};

/* CSU mode */
enum MODE 
{
    ENCODE = 0x0,
    DCODE = 0x3
};

/* gcm chmod set */
enum CHMOD
{
    ECB = 0x0,
    CTR = 0x2,  /*use for encrypt or decrypt*/
    MULTH = 0x3
};

/* key source */
enum KEYSRC
{
    OTHER = 0x0,
    DEV_KEY = 0x1,
    KUP = 0x2,  
    MULTH_H = 0x3
};

/* decflag set */
enum DECFLAG 
{
    use_opkey = 0xe,  
    no_opkey = 0xa ,
    ivup_kup_wr_en = 0x3 ,
    clear = 0x0      
};

/***************** Macros (Inline Functions) Definitions *********************/  
        
/************************** Variable Definitions *****************************/
        
/************************** Function Prototypes ******************************/   

u8 FDevcPs_unLockDEVKEY();

u8 FDevcPs_Prog_B(FDevcPs_T *dev);
u8 FDevcPs_CSI_B_HIGH(FDevcPs_T *dev);
u8 FDevcPs_RDWR_B_HIGH(FDevcPs_T *dev);
u8 FDevcPs_CSI_B_LOW(FDevcPs_T *dev);
u8 FDevcPs_RDWR_B_LOW(FDevcPs_T *dev);

u8 FDevcPs_secDownDataByteSwap(FDevcPs_T *dev, enum data_swap mode);
u8 FDevcPs_txDataSwap(FDevcPs_T *dev, enum data_swap mode);
u8 FDevcPs_downloadMode(FDevcPs_T *dev, enum download_mode mode);
u8 FDevcPs_rxDataSwap(FDevcPs_T *dev, enum data_swap mode);
u8 FDevcPs_smap32Swap(FDevcPs_T *dev, enum smap32_swap mode);
u8 FDevcPs_readbackDummyCount(FDevcPs_T *dev, enum dummy_num mode);
u8 FDevcPs_readFifoThre(FDevcPs_T *dev, enum readFifoThre mode);
u8 FDevcPs_writeFifoThre(FDevcPs_T *dev, enum writeFifoThre mode);
u8 FDevcPs_rclk_edge(FDevcPs_T *dev, enum clk_edge mode);
u8 FDevcPs_wclk_edge(FDevcPs_T *dev, enum clk_edge mode);

static u8 FDevcPs_AES_CR_AESENDisable(FDevcPs_T *dev);
static u8 FDevcPs_AES_CR_AESENEnable(FDevcPs_T *dev);
static u8 FDevcPs_AES_CR_ALG_SEL(FDevcPs_T *dev, enum ALG mode);
static u8 FDevcPs_AES_CR_MODE(FDevcPs_T *dev, enum MODE mode);
static u8 FDevcPs_AES_CR_CHMOD(FDevcPs_T *dev, enum CHMOD mode);

static u8 FDevcPs_AES_KEY_SOURCE(FDevcPs_T *dev, enum KEYSRC mode);
static u8 FDevcPs_AES_KEY_IV_LOAD(FDevcPs_T *dev);
static u8 FDevcPs_AES_DEC_FLAG(FDevcPs_T *dev, enum DECFLAG mode);

static u8 FDevcPs_CTRL_AESenable(FDevcPs_T *dev);
static u8 FDevcPs_CTRL_AESdisable(FDevcPs_T *dev);

static u8 devc_byte_swap(u32 *srcPtr, u32 len);
static u8 devc_byte_swap_todes(u32 *srcPtr, u32 len, u32 *desPtr);

 u8 FDevcPs_clearPcapStatus(FDevcPs_T *dev);
 u8 FDevcPs_initiateDma(FDevcPs_T *dev, u32 SourcePtr, u32 DestPtr, u32 SrcWordLength, u32 DestWordLength);
 u8 FDevcPs_pollDmaDone(FDevcPs_T *dev, u32 MaxCount);
 u8 FDevcPs_pollDPDone(FDevcPs_T *dev, u32 MaxCount);

static unsigned long FDevcPs_regAddr(u8 Register, u8 OpCode, u8 Size);

u8 FDevcPs_transfer(FDevcPs_T *dev,
                      void *SourcePtr, 
                      u32 SrcWordLength, 
                      void *DestPtr, 
                      u32 DestWordLength, 
                      u32 TransferType);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* end of protection macro */_