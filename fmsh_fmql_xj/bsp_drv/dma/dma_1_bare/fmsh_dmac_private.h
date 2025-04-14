/******************************************************************************
*
* Copyright (C) 2018 - 2028 FMSH, Inc.  All rights reserved.
*
******************************************************************************/
/******************************************************************************
*
* @file  fmsh_dmac_private.h
*
* This file contains
*
* @note		None.
*
* MODIFICATION HISTORY:
*
*<pre>
* Ver   Who  Date     Changes
* ----- ---- -------- ---------------------------------------------
* 0.01   yl  12/20/2018  First Release
*</pre>
******************************************************************************/
#ifndef _FMSH_DMAC_PRIVATE_H_
#define _FMSH_DMAC_PRIVATE_H_

/***************************** Include Files *********************************/
#include "../../common/fmsh_common.h"
/*#include "fmsh_common.h"*/

/************************** Constant Definitions *****************************/

/* allow C++ to use this header */
#ifdef __cplusplus
extern "C" {
#endif

/* Common requirements (preconditions) for all dmac driver functions*/
#define DMAC_COMMON_REQUIREMENTS(p)             \
do {                                            \
    FMSH_REQUIRE(p != NULL);                      \
    FMSH_REQUIRE(p->instance != NULL);            \
    FMSH_REQUIRE(p->comp_param != NULL);          \
    FMSH_REQUIRE(p->comp_type == FMSH_ahb_dmac);    \
} while(0)

/* Macro definitions for DMA controller limits*/
#define DMAC_MAX_CHANNELS    8
#define DMAC_MAX_INTERRUPTS  5

/* Macro definitions for the flow control mode of the DMA Controller*/
#define DMAC_DMA_FC_ONLY     0x0
#define DMAC_SRC_FC_ONLY     0x1
#define DMAC_DST_FC_ONLY     0x2
#define DMAC_ANY_FC          0x3

/* Macro definitions for channel masks*/

/* i.e if DMAC_MAX_CHANNELS = 8 : = 0xff */
#define DMAC_MAX_CH_MASK  \
     (~(~0 << DMAC_MAX_CHANNELS))

/* i.e. if NUM_CHANNELS = 3 : = 0xf8 */
#define DMAC_CH_MASK \
     (DMAC_MAX_CH_MASK & (DMAC_MAX_CH_MASK << param->num_channels))

/* i.e. if NUM_CHANNELS = 3 : = 0x7 */
#define DMAC_CH_EN_MASK \
     (DMAC_MAX_CH_MASK & ~(DMAC_CH_MASK))

/* i.e. if NUM_CHANNELS = 3 : = 0x707 */
#define DMAC_CH_ALL_MASK \
     ((DMAC_CH_EN_MASK) + ((DMAC_CH_EN_MASK) << DMAC_MAX_CHANNELS))

/* get a FDmaPs_channelNumber from a channel index*/
#define DMAC_CH_NUM(ch_idx)     (0x101 << (ch_idx))

/* power of 2 macro*/
#define POW2(pow) \
     (1 << (pow))

/* Is A greater than or equal to B macro ?*/
#define A_MAXEQ_B(a,b) \
  ((a) >= (b))

/**************************** Type Definitions *******************************/

/*****************************************************************************
* DESCRIPTION
*  This is the structure used for accessing the dmac register portmap.
*
*****************************************************************************/
typedef struct _dmac_portmap {

  /* Channel registers                                    */
  /* The offset address for each of the channel registers */
  /*  is shown for channel 0. For other channel numbers   */
  /*  use the following equation.                         */
  /*                                                      */
  /*    offset = (channel_num * 0x058) + channel_0 offset */
  /*                                                      */
  struct {
      volatile u32 sar_l;     /* Source Address Reg      (0x000) */
      volatile u32 sar_h;
      volatile u32 dar_l;     /* Destination Address Reg (0x008) */
      volatile u32 dar_h;
      volatile u32 llp_l;     /* Linked List Pointer Reg (0x010) */
      volatile u32 llp_h;  
      volatile u32 ctl_l;     /* Control Reg             (0x018) */
      volatile u32 ctl_h;
      volatile u32 sstat_l;   /* Source Status Reg       (0x020) */
      volatile u32 sstat_h;
      volatile u32 dstat_l;   /* Destination Status Reg  (0x028) */
      volatile u32 dstat_h;   
      volatile u32 sstatar_l; /* Source Status Addr Reg  (0x030) */
      volatile u32 sstatar_h; 
      volatile u32 dstatar_l; /* Dest Status Addr Reg    (0x038) */
      volatile u32 dstatar_h; 
      volatile u32 cfg_l;     /* Configuration Reg       (0x040) */
      volatile u32 cfg_h;
      volatile u32 sgr_l;     /* Source Gather Reg       (0x048) */
      volatile u32 sgr_h; 
      volatile u32 dsr_l;     /* Destination Scatter Reg (0x050) */
      volatile u32 dsr_h;
  } ch[8];

  /* Interrupt Raw Status Registers */
  volatile u32 raw_tfr_l;     /* Raw Status for IntTfr   (0x2c0) */
  volatile u32 raw_tfr_h;   
  volatile u32 raw_block_l;   /* Raw Status for IntBlock (0x2c8) */
  volatile u32 raw_block_h;
  volatile u32 raw_srctran_l; /* Raw Status IntSrcTran   (0x2d0) */
  volatile u32 raw_srctran_h; 
  volatile u32 raw_dsttran_l; /* Raw Status IntDstTran   (0x2d8) */
  volatile u32 raw_dsttran_h;
  volatile u32 raw_err_l;     /* Raw Status for IntErr   (0x2e0) */
  volatile u32 raw_err_h;

  /* Interrupt Status Registers */
  volatile u32 status_tfr_l;    /* Status for IntTfr     (0x2e8) */
  volatile u32 status_tfr_h;
  volatile u32 status_block_l;  /* Status for IntBlock   (0x2f0) */
  volatile u32 status_block_h;
  volatile u32 status_srctran_l;/* Status for IntSrcTran (0x2f8) */
  volatile u32 status_srctran_h;
  volatile u32 status_dsttran_l;/* Status for IntDstTran (0x300) */
  volatile u32 status_dsttran_h;
  volatile u32 status_err_l;    /* Status for IntErr     (0x308) */
  volatile u32 status_err_h;

  /* Interrupt Mask Registers */
  volatile u32 mask_tfr_l;      /* Mask for IntTfr       (0x310) */
  volatile u32 mask_tfr_h;
  volatile u32 mask_block_l;    /* Mask for IntBlock     (0x318) */
  volatile u32 mask_block_h;
  volatile u32 mask_srctran_l;  /* Mask for IntSrcTran   (0x320) */
  volatile u32 mask_srctran_h;
  volatile u32 mask_dsttran_l;  /* Mask for IntDstTran   (0x328) */
  volatile u32 mask_dsttran_h;
  volatile u32 mask_err_l;      /* Mask for IntErr       (0x330) */
  volatile u32 mask_err_h;

  /* Interrupt Clear Registers */
  volatile u32 clear_tfr_l;     /* Clear for IntTfr      (0x338) */
  volatile u32 clear_tfr_h;
  volatile u32 clear_block_l;   /* Clear for IntBlock    (0x340) */
  volatile u32 clear_block_h;
  volatile u32 clear_srctran_l; /* Clear for IntSrcTran  (0x348) */
  volatile u32 clear_srctran_h;
  volatile u32 clear_dsttran_l; /* Clear for IntDstTran  (0x350) */
  volatile u32 clear_dsttran_h;
  volatile u32 clear_err_l;     /* Clear for IntErr      (0x358) */
  volatile u32 clear_err_h;
  volatile u32 status_int_l;    /* Combined Intr Status  (0x360) */
  volatile u32 status_int_h;

  /* Software Handshaking Registers */
  volatile u32 req_src_reg_l; /* Src Sw Transaction Req  (0x368) */
  volatile u32 req_src_reg_h;   
  volatile u32 req_dst_reg_l; /* Dest Sw Transaction Req (0x370) */
  volatile u32 req_dst_reg_h;
  volatile u32 sgl_rq_src_reg_l; /* Sgl Src Transac Req  (0x378) */
  volatile u32 sgl_rq_src_reg_h;
  volatile u32 sgl_rq_dst_reg_l; /* Sgl Dest Transac Req (0x380) */
  volatile u32 sgl_rq_dst_reg_h;
  volatile u32 lst_src_reg_l;   /* Last Src Transac Req  (0x388) */
  volatile u32 lst_src_reg_h;
  volatile u32 lst_dst_reg_l;   /* Last Dest Transac Req (0x390) */
  volatile u32 lst_dst_reg_h;

  /* Misc Registers */
  volatile u32 dma_cfg_reg_l; /* Configuration Register  (0x398) */
  volatile u32 dma_cfg_reg_h;
  volatile u32 ch_en_reg_l;   /* Channel Enable Register (0x3a0) */
  volatile u32 ch_en_reg_h;
  volatile u32 dma_id_reg_l;    /* ID Register           (0x3a8) */
  volatile u32 dma_id_reg_h;
  volatile u32 dma_test_reg_l;  /* Test Register         (0x3b0) */
  volatile u32 dma_test_reg_h;
  volatile u32 old_version_id_l;/* legacy support        (0x3b8) */
  volatile u32 old_version_id_h;
  volatile u32 reserved_low;    /* reserved              (0x3c0) */
  volatile u32 reserved_high;
  volatile u32 dma_comp_params_6_l;/* hardware params    (0x3c8) */
  volatile u32 dma_comp_params_6_h;
  volatile u32 dma_comp_params_5_l;/* hardware params    (0x3d0) */
  volatile u32 dma_comp_params_5_h;
  volatile u32 dma_comp_params_4_l;/* hardware params    (0x3d8) */
  volatile u32 dma_comp_params_4_h;
  volatile u32 dma_comp_params_3_l;/* hardware params    (0x3e0) */
  volatile u32 dma_comp_params_3_h;
  volatile u32 dma_comp_params_2_l;/* hardware params    (0x3e8) */
  volatile u32 dma_comp_params_2_h;
  volatile u32 dma_comp_params_1_l;/* hardware params    (0x3f0) */
  volatile u32 dma_comp_params_1_h;
  volatile u32 dma_version_id_l;/* Version ID Register   (0x3f8) */
  volatile u32 dma_version_id_h;
}FDmaPs_PortMap_T;

/***************** Macros (Inline Functions) Definitions *********************/

/************************** Function Prototypes ******************************/

void FDmaPs_resetInstance(FDmaPs_T *pDmac);
int FDmaPs_autoCompParams(FDmaPs_T *pDmac);
int FDmaPs_checkChannelBusy(FDmaPs_T *pDmac,
        enum FDmaPs_channelNumber ch_num);
int FDmaPs_checkChannelRange(FDmaPs_T *pDmac,
        enum FDmaPs_channelNumber ch_num);
void FDmaPs_setChannelPriorityOrder(FDmaPs_T *pDmac);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* #ifndef _FMSH_DMAC_PRIVATE_H_ */


