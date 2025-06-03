/* vxDma.h - vxDma driver header */

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
01a, 07Jan20, jc  written.
*/

#ifndef __INC_VX_DMA_H__
#define __INC_VX_DMA_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#define  VX_DMA_CFG_BASE   (0xE004B000)

#define  DMA_CTRL_0        (0)

/**/
/* dma registers*/
/**/
/* define DMAC register's offset*/
/* channel registers, x = 0~7*/
#define DMAC_CHX_SAR_L_OFFSET(x)		(0x0000 + 0x0058 * x)
#define DMAC_CHX_SAR_H_OFFSET(x)		(0x0004 + 0x0058 * x)
#define DMAC_CHX_DAR_L_OFFSET(x)		(0x0008 + 0x0058 * x)
#define DMAC_CHX_DAR_H_OFFSET(x)		(0x000C + 0x0058 * x)
#define DMAC_CHX_LLP_L_OFFSET(x)		(0x0010 + 0x0058 * x)
#define DMAC_CHX_LLP_H_OFFSET(x)		(0x0014 + 0x0058 * x)
#define DMAC_CHX_CTL_L_OFFSET(x)		(0x0018 + 0x0058 * x)
#define DMAC_CHX_CTL_H_OFFSET(x)		(0x001C + 0x0058 * x)
#define DMAC_CHX_SSTAT_L_OFFSET(x)		(0x0020 + 0x0058 * x)
#define DMAC_CHX_SSTAT_H_OFFSET(x)		(0x0024 + 0x0058 * x)
#define DMAC_CHX_DSTAT_L_OFFSET(x)		(0x0028 + 0x0058 * x)
#define DMAC_CHX_DSTAT_H_OFFSET(x)		(0x002C + 0x0058 * x)
#define DMAC_CHX_SSTATAR_L_OFFSET(x)	(0x0030 + 0x0058 * x)
#define DMAC_CHX_SSTATAR_H_OFFSET(x)	(0x0034 + 0x0058 * x)
#define DMAC_CHX_DSTATAR_L_OFFSET(x)	(0x0038 + 0x0058 * x)
#define DMAC_CHX_DSTATAR_H_OFFSET(x)	(0x003C + 0x0058 * x)
#define DMAC_CHX_CFG_L_OFFSET(x)		(0x0040 + 0x0058 * x)
#define DMAC_CHX_CFG_H_OFFSET(x)		(0x0044 + 0x0058 * x)
#define DMAC_CHX_SGR_L_OFFSET(x)		(0x0048 + 0x0058 * x)
#define DMAC_CHX_SGR_H_OFFSET(x)		(0x004C + 0x0058 * x)
#define DMAC_CHX_DSR_L_OFFSET(x)		(0x0050 + 0x0058 * x)
#define DMAC_CHX_DSR_H_OFFSET(x)		(0x0054 + 0x0058 * x)

/* interrupt registers*/
#define DMAC_INT_RAW_TFR_L_OFFSET			(0x02C0)
#define DMAC_INT_RAW_TFR_H_OFFSET			(0x02C4)
#define DMAC_INT_RAW_BLOCK_L_OFFSET			(0x02C8)
#define DMAC_INT_RAW_BLOCK_H_OFFSET			(0x02CC)
#define DMAC_INT_RAW_SRCTRAN_L_OFFSET		(0x02D0)
#define DMAC_INT_RAW_SRCTRAN_H_OFFSET		(0x02D4)
#define DMAC_INT_RAW_DSTTRAN_L_OFFSET		(0x02D8)
#define DMAC_INT_RAW_DSTTRAN_H_OFFSET		(0x02DC)
#define DMAC_INT_RAW_ERR_L_OFFSET			(0x02E0)
#define DMAC_INT_RAW_ERR_H_OFFSET			(0x02E4)
#define DMAC_INT_STATUS_TFR_L_OFFSET		(0x02E8)
#define DMAC_INT_STATUS_TFR_H_OFFSET		(0x02EC)
#define DMAC_INT_STATUS_BLOCK_L_OFFSET		(0x02F0)
#define DMAC_INT_STATUS_BLOCK_H_OFFSET		(0x02F4)
#define DMAC_INT_STATUS_SRCTRAN_L_OFFSET	(0x02F8)
#define DMAC_INT_STATUS_SRCTRAN_H_OFFSET	(0x02FC)
#define DMAC_INT_STATUS_DSTTRAN_L_OFFSET	(0x0300)
#define DMAC_INT_STATUS_DSTTRAN_H_OFFSET	(0x0304)
#define DMAC_INT_STATUS_ERR_L_OFFSET		(0x0308)
#define DMAC_INT_STATUS_ERR_H_OFFSET		(0x030C)
#define DMAC_INT_MASK_TFR_L_OFFSET			(0x0310)
#define DMAC_INT_MASK_TFR_H_OFFSET			(0x0314)
#define DMAC_INT_MASK_BLOCK_L_OFFSET		(0x0318)
#define DMAC_INT_MASK_BLOCK_H_OFFSET		(0x031C)
#define DMAC_INT_MASK_SRCTRAN_L_OFFSET		(0x0320)
#define DMAC_INT_MASK_SRCTRAN_H_OFFSET		(0x0324)
#define DMAC_INT_MASK_DSTTRAN_L_OFFSET		(0x0328)
#define DMAC_INT_MASK_DSTTRAN_H_OFFSET		(0x032C)
#define DMAC_INT_MASK_ERR_L_OFFSET			(0x0330)
#define DMAC_INT_MASK_ERR_H_OFFSET			(0x0334)
#define DMAC_INT_CLEAR_TFR_L_OFFSET			(0x0338)
#define DMAC_INT_CLEAR_TFR_H_OFFSET			(0x033C)
#define DMAC_INT_CLEAR_BLOCK_L_OFFSET		(0x0340)
#define DMAC_INT_CLEAR_BLOCK_H_OFFSET		(0x0344)
#define DMAC_INT_CLEAR_SRCTRAN_L_OFFSET		(0x0348)
#define DMAC_INT_CLEAR_SRCTRAN_H_OFFSET		(0x034C)
#define DMAC_INT_CLEAR_DSTTRAN_L_OFFSET		(0x0350)
#define DMAC_INT_CLEAR_DSTTRAN_H_OFFSET		(0x0354)
#define DMAC_INT_CLEAR_ERR_L_OFFSET			(0x0358)
#define DMAC_INT_CLEAR_ERR_H_OFFSET			(0x035C)
#define DMAC_INT_STATUS_INT_L_OFFSET		(0x0360)
#define DMAC_INT_STATUS_INT_H_OFFSET		(0x0364)

/* software handshaking registers*/
#define DMAC_SWHS_REQ_SRC_REG_L_OFFSET		(0x0368)
#define DMAC_SWHS_REQ_SRC_REG_H_OFFSET		(0x036C)
#define DMAC_SWHS_REQ_DST_REG_L_OFFSET		(0x0370)
#define DMAC_SWHS_REQ_DST_REG_H_OFFSET		(0x0374)
#define DMAC_SWHS_SGL_REQ_SRC_REG_L_OFFSET	(0x0378)
#define DMAC_SWHS_SGL_REQ_SRC_REG_H_OFFSET	(0x037C)
#define DMAC_SWHS_SGL_REQ_DST_REG_L_OFFSET	(0x0380)
#define DMAC_SWHS_SGL_REQ_DST_REG_H_OFFSET	(0x0384)
#define DMAC_SWHS_LST_SRC_REG_L_OFFSET		(0x0388)
#define DMAC_SWHS_LST_SRC_REG_H_OFFSET		(0x038C)
#define DMAC_SWHS_LST_DST_REG_L_OFFSET		(0x0390)
#define DMAC_SWHS_LST_DST_REG_H_OFFSET		(0x0394)

/* configuration and channel enable registers*/
#define DMAC_DMA_CFG_REG_L_OFFSET			(0x0398)
#define DMAC_DMA_CFG_REG_H_OFFSET			(0x039C)
#define DMAC_CH_EN_REG_L_OFFSET				(0x03A0)
#define DMAC_CH_EN_REG_H_OFFSET				(0x03A4)

/* miscellaneous registers*/
#define DMAC_DMA_ID_REG_L_OFFSET			(0x03A8)
#define DMAC_DMA_ID_REG_H_OFFSET			(0x03AC)
#define DMAC_DMA_TEST_REG_L_OFFSET			(0x03B0)
#define DMAC_DMA_TEST_REG_H_OFFSET			(0x03B4)
#define DMAC_DMA_COMP_PARAMS_6_L_OFFSET		(0x03C8)
#define DMAC_DMA_COMP_PARAMS_6_H_OFFSET		(0x03CC)
#define DMAC_DMA_COMP_PARAMS_5_L_OFFSET		(0x03D0)
#define DMAC_DMA_COMP_PARAMS_5_H_OFFSET		(0x03D4)
#define DMAC_DMA_COMP_PARAMS_4_L_OFFSET		(0x03D8)
#define DMAC_DMA_COMP_PARAMS_4_H_OFFSET		(0x03DC)
#define DMAC_DMA_COMP_PARAMS_3_L_OFFSET		(0x03E0)
#define DMAC_DMA_COMP_PARAMS_3_H_OFFSET		(0x03E4)
#define DMAC_DMA_COMP_PARAMS_2_L_OFFSET		(0x03E8)
#define DMAC_DMA_COMP_PARAMS_2_H_OFFSET		(0x03EC)
#define DMAC_DMA_COMP_PARAMS_1_L_OFFSET		(0x03F0)
#define DMAC_DMA_COMP_PARAMS_1_H_OFFSET		(0x03F4)
#define DMAC_DMA_COMP_ID_REG_L_OFFSET		(0x03F8)
#define DMAC_DMA_COMP_ID_REG_H_OFFSET		(0x03FC)
/***/

 /* DmaCfgReg registers*/
#define DMAC_DMACFGREG_L_DMA_EN			(0x1 << 0)

/* ChEnReg registers*/
#define DMAC_CHENREG_L_CH_EN(ch)		(0x1 << (ch))
#define DMAC_CHENREG_L_CH_EN_ALL		(0xFF << 0)

#define DMAC_CHENREG_L_CH_EN_WE(ch)		(0x1 << ((ch) + 8))
#define DMAC_CHENREG_L_CH_EN_WE_ALL		(0xFF << 8)

/* Channel registers*/
#define DMAC_SAR_L_SAR              	(0xFFFFFFFF)
#define DMAC_DAR_L_DAR              	(0xFFFFFFFF)

#define DMAC_LLP_L_LMS              	(0x3 << 0)
#define DMAC_LLP_L_LOC              	((unsigned int)0x3FFFFFFF << 2)

#define DMAC_CTL_L_INT_EN           	(0x1 << 0)
#define DMAC_CTL_L_DST_TR_WIDTH     	(0x7 << 1)
#define DMAC_CTL_L_SRC_TR_WIDTH     	(0x7 << 4)
#define DMAC_CTL_L_DINC             	(0x3 << 7)
#define DMAC_CTL_L_SINC             	(0x3 << 9)
#define DMAC_CTL_L_DEST_MSIZE       	(0x7 << 11)
#define DMAC_CTL_L_SRC_MSIZE        	(0x7 << 14)
#define DMAC_CTL_L_SRC_GATHER_EN    	(0x1 << 17)
#define DMAC_CTL_L_DST_SCATTER_EN   	(0x1 << 18)
#define DMAC_CTL_L_TT_FC            	(0x7 << 20)
#define DMAC_CTL_L_DMS              	(0x3 << 23)
#define DMAC_CTL_L_SMS              	(0x3 << 25)
#define DMAC_CTL_L_LLP_DST_EN       	(0x1 << 27)
#define DMAC_CTL_L_LLP_SRC_EN       	(0x1 << 28)

#define DMAC_CTL_H_BLOCK_TS         	(0xFFF << 0)
#define DMAC_CTL_H_DONE             	(0x1 << 12)

#define DMAC_SSTAT_L_SSTAT          (0xFFFFFFFF)

#define DMAC_DSTAT_L_DSTAT          (0xFFFFFFFF)

#define DMAC_SSTATAR_L_SSTATAR      (0xFFFFFFFF)

#define DMAC_DSTATAR_L_DSTATAR      (0xFFFFFFFF)

#define DMAC_CFG_L_CH_PRIOR         (0x7 << 5)
#define DMAC_CFG_L_CH_SUSP          (0x1 << 8)
#define DMAC_CFG_L_FIFO_EMPTY       (0x1 << 9)
#define DMAC_CFG_L_HS_SEL_DST       (0x1 << 10)
#define DMAC_CFG_L_HS_SEL_SRC       (0x1 << 11)
#define DMAC_CFG_L_LOCK_CH_L        (0x3 << 12)
#define DMAC_CFG_L_LOCK_B_L         (0x3 << 14)
#define DMAC_CFG_L_LOCK_CH          (0x1 << 16)
#define DMAC_CFG_L_LOCK_B           (0x1 << 17)
#define DMAC_CFG_L_DST_HS_POL       (0x1 << 18)
#define DMAC_CFG_L_SRC_HS_POL       (0x1 << 19)
#define DMAC_CFG_L_MAX_ABRST        (0x3FF << 20)
#define DMAC_CFG_L_RELOAD_SRC       (0x1 << 30)
#define DMAC_CFG_L_RELOAD_DST       ((unsigned int)0x1 << 31)

#define DMAC_CFG_H_FCMODE           (0x1 << 0)
#define DMAC_CFG_H_FIFO_MODE        (0x1 << 1)
#define DMAC_CFG_H_PROTCTL          (0x7 << 2)
#define DMAC_CFG_H_DS_UPD_EN        (0x1 << 5)
#define DMAC_CFG_H_SS_UPD_EN        (0x1 << 6)
#define DMAC_CFG_H_SRC_PER          (0xF << 7)
#define DMAC_CFG_H_DEST_PER         (0xF << 11)

#define DMAC_SGR_L_SGI              (0xFFFFF << 0)
#define DMAC_SGR_L_SGC              ((unsigned int)0xFFF << 20)

#define DMAC_DSR_L_DSI              (0xFFFFF << 0)
#define DMAC_DSR_L_DSC              ((unsigned int)0xFFF << 20)

/* Interrupt registers*/
#define DMAC_INT_RAW_STAT_CLR(ch)   (0x1 << (ch))
#define DMAC_INT_RAW_STAT_CLR_ALL   (0xFF << 0)

#define DMAC_INT_MASK_L(ch)         (0x1 << (ch))
#define DMAC_INT_MASK_L_ALL         (0xFF << 0)
#define DMAC_INT_MASK_L_WE(ch)      (0x1 << ((ch) + 8))
#define DMAC_INT_MASK_L_WE_ALL      (0xFF << 8)

#define DMAC_STATUSINT_L_TFR        (0x1 << 0)
#define DMAC_STATUSINT_L_BLOCK      (0x1 << 1)
#define DMAC_STATUSINT_L_SRCTRAN    (0x1 << 2)
#define DMAC_STATUSINT_L_DSTTRAN    (0x1 << 3)
#define DMAC_STATUSINT_L_ERR        (0x1 << 4)

/* SoftWare HandShaking registers*/
#define DMAC_SW_HANDSHAKE_L(ch)     (0x1 << (ch))
#define DMAC_SW_HANDSHAKE_L_WE(ch)  (0x1 << ((ch) + 8))

/* DmaldReg registers*/
#define DMAC_DMALD_L_DMA_ID         (0xFFFFFFFF)

/* DmaTestReg registers*/
#define DMAC_DMATESTREG_L_TEST_SLV_IF (0x1 << 0)

/* DMA_COMP_PARAMS registers*/
#define DMAC_DMACOMPVER_L_DMACOMPVER  (0xFFFFFFFF)

#define DMAC_PARAM_CHX_DTW           (0x7 << 0)
#define DMAC_PARAM_CHX_STW           (0x7 << 3)
#define DMAC_PARAM_CHX_STAT_DST      (0x1 << 6)
#define DMAC_PARAM_CHX_STAT_SRC      (0x1 << 7)
#define DMAC_PARAM_CHX_DST_SCA_EN    (0x1 << 8)
#define DMAC_PARAM_CHX_SRC_GAT_EN    (0x1 << 9)
#define DMAC_PARAM_CHX_LOCK_EN       (0x1 << 10)
#define DMAC_PARAM_CHX_MULTI_BLK_EN  (0x1 << 11)
#define DMAC_PARAM_CHX_CTL_WB_EN     (0x1 << 12)
#define DMAC_PARAM_CHX_HC_LLP        (0x1 << 13)
#define DMAC_PARAM_CHX_FC            (0x3 << 14)
#define DMAC_PARAM_CHX_MAX_MULT_SIZE (0x7 << 16)
#define DMAC_PARAM_CHX_DMS           (0x7 << 19)
#define DMAC_PARAM_CHX_LMS           (0x7 << 22)
#define DMAC_PARAM_CHX_SMS           (0x7 << 25)
#define DMAC_PARAM_CHX_FIFO_DEPTH    (0x7 << 28)

#define DMAC_PARAM_CH0_MULTI_BLK_TYPE  (0xF << 0)
#define DMAC_PARAM_CH1_MULTI_BLK_TYPE  (0xF << 4)
#define DMAC_PARAM_CH2_MULTI_BLK_TYPE  (0xF << 8)
#define DMAC_PARAM_CH3_MULTI_BLK_TYPE  (0xF << 12)
#define DMAC_PARAM_CH4_MULTI_BLK_TYPE  (0xF << 16)
#define DMAC_PARAM_CH5_MULTI_BLK_TYPE  (0xF << 20)
#define DMAC_PARAM_CH6_MULTI_BLK_TYPE  (0xF << 24)
#define DMAC_PARAM_CH7_MULTI_BLK_TYPE  ((unsigned int)0xF << 28)

#define DMAC_PARAM_BIG_ENDIAN        (0x1 << 0)
#define DMAC_PARAM_INTR_IO           (0x3 << 1)
#define DMAC_PARAM_MABRST            (0x1 << 3)
#define DMAC_PARAM_NUM_CHANNELS      (0x7 << 8)
#define DMAC_PARAM_NUM_MASTER_INT    (0x3 << 11)
#define DMAC_PARAM_S_HDATA_WIDTH     (0x3 << 13)
#define DMAC_PARAM_M1_HDATA_WIDTH    (0x3 << 15)
#define DMAC_PARAM_M2_HDATA_WIDTH    (0x3 << 17)
#define DMAC_PARAM_M3_HDATA_WIDTH    (0x3 << 19)
#define DMAC_PARAM_M4_HDATA_WIDTH    (0x3 << 21)
#define DMAC_PARAM_NUM_HS_INT        (0x1F << 23)
#define DMAC_PARAM_ADD_ENCODED_PARAMS (0x1 << 28)
#define DMAC_PARAM_STATIC_ENDIAN_SELECT (0x1 << 29)

#define DMAC_PARAM_CH0_MAX_BLK_SIZE  (0xF << 0)
#define DMAC_PARAM_CH1_MAX_BLK_SIZE  (0xF << 4)
#define DMAC_PARAM_CH2_MAX_BLK_SIZE  (0xF << 8)
#define DMAC_PARAM_CH3_MAX_BLK_SIZE  (0xF << 12)
#define DMAC_PARAM_CH4_MAX_BLK_SIZE  (0xF << 16)
#define DMAC_PARAM_CH5_MAX_BLK_SIZE  (0xF << 20)
#define DMAC_PARAM_CH6_MAX_BLK_SIZE  (0xF << 24)
#define DMAC_PARAM_CH7_MAX_BLK_SIZE  ((unsigned int)0xF << 28)

/* DMA Component ID registers*/
#define DMAC_IDREG_L_DMA_COMP_TYPE     (0xFFFFFFFF)
#define DMAC_IDREG_H_DMA_COMP_VERSION     (0xFFFFFFFF)


/*
FMSH_err = -1,
FMSH_clear = 0,
FMSH_set = 1
*/
#define  DMA_WORK_MODE_NORMAL  (0)   /* FMSH_clear   (0)   // clear test mode and normal work mode*/
#define  DMA_WORK_MODE_TEST    (1)   /* FMSH_set     (1)   // dma test mdoe*/

#define DMA_CHN_NONE   (0x0000)  /* Dmac_no_channel   = 0x0000,*/
#define DMA_CHN_0      (0x0101)  /* Dmac_channel0	  = 0x0101,*/
#define DMA_CHN_1      (0x0202)  /* Dmac_channel1	  = 0x0202,*/
#define DMA_CHN_2      (0x0404)  /* Dmac_channel2	  = 0x0404,*/
#define DMA_CHN_3      (0x0808)  /* Dmac_channel3	  = 0x0808,*/
#define DMA_CHN_4      (0x1010)  /* Dmac_channel4	  = 0x1010,*/
#define DMA_CHN_5      (0x2020)  /* Dmac_channel5	  = 0x2020,*/
#define DMA_CHN_6      (0x4040)  /* Dmac_channel6	  = 0x4040,*/
#define DMA_CHN_7      (0x8080)  /* Dmac_channel7	  = 0x8080,*/
#define DMA_CHN_ALL    (0xFFFF)  /* Dmac_all_channels = 0xffff*/

#define DMA_STATE_IDLE           (0x00)   /* Dmac_idle		   = 0x0,*/
#define DMA_STATE_SINGLE_REGION  (0x01)   /* Dmac_single_region = 0x1,*/
#define DMA_STATE_BURST_REGION   (0x02)   /* Dmac_burst_region  = 0x2*/


#define DMA_XFER_TYPE1_ROW1    (0x01)    /* Dmac_transfer_row1  = 0x1, /* single block or last multi-block  no write back                   */
#define DMA_XFER_TYPE2_ROW2    (0x02)    /* Dmac_transfer_row2  = 0x2,  multi-block auto-reload DAR       contiguous SAR no write back    */
#define DMA_XFER_TYPE3_ROW3    (0x03)    /* Dmac_transfer_row3  = 0x3, /* multi-block auto reload SAR      contiguous DAR no write back    */
#define DMA_XFER_TYPE4_ROW4    (0x04)    /* Dmac_transfer_row4  = 0x4,  multi-block auto-reload SAR DAR  no write back                   */
#define DMA_XFER_TYPE5_ROW5    (0x05)    /* Dmac_transfer_row5  = 0x5, /* single block or last multi-block with write back                 */
#define DMA_XFER_TYPE6_ROW6    (0x06)    /* Dmac_transfer_row6  = 0x6,  multi-block linked list DAR      contiguous SAR with write back  */
#define DMA_XFER_TYPE7_ROW7    (0x07)    /* Dmac_transfer_row7  = 0x7, /* multi-block linked list DAR auto reload SAR  with write back     */
#define DMA_XFER_TYPE8_ROW8    (0x08)    /* Dmac_transfer_row8  = 0x8,  multi-block linked list SAR      contiguous DAR with write back  */
#define DMA_XFER_TYPE9_ROW9    (0x09)    /* Dmac_transfer_row9  = 0x9, /* multi-block linked list SAR auto reload DAR with write back      */
#define DMA_XFER_TYPE10_ROW10  (0x0A)    /* Dmac_transfer_row10 = 0xa   multi-block linked list SAR DAR  with write back                 */

#define DMA_NO_HARDCODE    (0x00)  /* Dmac_no_hardcode   = 0x0, /* transfer type row1 / row5 */
#define DMA_CONT_RELOAD    (0x01)  /* Dmac_cont_reload   = 0x1,  transfer type row2 */
#define DMA_RELOAD_CONT    (0x02)  /* Dmac_reload_cont   = 0x2, /* transfer type row3 */
#define DMA_RELOAD_RELOAD  (0x03)  /* Dmac_reload_reload = 0x3,  transfer type row4 */
#define DMA_CONT_LLP       (0x04)  /* Dmac_cont_llp	   = 0x4, /* transfer type row6 */
#define DMA_RELOAD_LLP     (0x05)  /* Dmac_reload_llp    = 0x5,  transfer type row7 */
#define DMA_LLP_CONT       (0x06)  /* Dmac_llp_cont	   = 0x6, /* transfer type row8 */
#define DMA_LLP_RELOAD     (0x07)  /* Dmac_llp_reload    = 0x7,  transfer type row9 */
#define DMA_LLP_LLP        (0x08)  /* Dmac_llp_llp	   = 0x8  /* transfer type row10 */


#define DMA_XFER_PATH0_MEM2MEM_DMACTRL      (0x00)    /* Dmac_mem2mem_dma    = 0x0,  mem to mem - DMAC   flow ctlr */
#define DMA_XFER_PATH1_MEM2DEV_DMACTRL      (0x01)    /* Dmac_mem2prf_dma    = 0x1, /* mem to prf - DMAC   flow ctlr */
#define DMA_XFER_PATH2_DEV2MEM_DMACTRL      (0x02)    /* Dmac_prf2mem_dma    = 0x2,  prf to mem - DMAC   flow ctlr */
#define DMA_XFER_PATH3_DEV2DEV_DMACTRL      (0x03)    /* Dmac_prf2prf_dma    = 0x3, /* prf to prf - DMAC   flow ctlr     */
#define DMA_XFER_PATH4_DEV2MEM_DEVCTRL      (0x04)    /* Dmac_prf2mem_prf    = 0x4,  prf to mem - periph flow ctlr */
#define DMA_XFER_PATH5_DEV2DEV_SRCDEVCTRL   (0x05)    /* Dmac_prf2prf_srcprf = 0x5, /* prf to prf - source flow ctlr */
#define DMA_XFER_PATH6_MEM2DEV_DEVCTRL      (0x06)    /* Dmac_mem2prf_prf    = 0x6,  mem to prf - periph flow ctlr */
#define DMA_XFER_PATH7_DEV2DEV_DSTDEVCTRL   (0x07)    /* Dmac_prf2prf_dstprf = 0x7  /* prf to prf - dest   flow ctlr */

#define DMA_IRQ_NONE            (0x00)    /* Dmac_irq_none       = 0x00,      no interrupts*/
#define DMA_IRQ_XFER_DONE       (0x01)    /* Dmac_irq_tfr        = 0x01,      transfer complete*/
#define DMA_IRQ_BLK_XFER_DONE   (0x02)    /* Dmac_irq_block      = 0x02,      block transfer complete*/
#define DMA_IRQ_SRC_XFER_DONE   (0x04)    /* Dmac_irq_srctran    = 0x04,      source transaction complete*/
#define DMA_IRQ_DST_XFER_DONE   (0x08)    /* Dmac_irq_dsttran    = 0x08,      destination transaction complete*/
#define DMA_IRQ_ERR             (0x10)    /* Dmac_irq_err        = 0x10,      error*/
#define DMA_IRQ_ALL             (0x1F)    /* Dmac_irq_all        = 0x1f       all interrupts*/

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
#define DMAC_MAX_CH_MASK  (~(~0 << DMAC_MAX_CHANNELS))

/* i.e. if NUM_CHANNELS = 3 : = 0xf8 */
/*#define DMAC_CH_MASK(chn) (DMAC_MAX_CH_MASK & (DMAC_MAX_CH_MASK << (chn)))*/
#define DMAC_CH_MASK (DMAC_MAX_CH_MASK & (DMAC_MAX_CH_MASK << pParams->num_channels))

/* i.e. if NUM_CHANNELS = 3 : = 0x7 */
#define DMAC_CH_EN_MASK  (DMAC_MAX_CH_MASK & ~(DMAC_CH_MASK))

/* i.e. if NUM_CHANNELS = 3 : = 0x707 */
#define DMAC_CH_ALL_MASK  ((DMAC_CH_EN_MASK) + ((DMAC_CH_EN_MASK) << DMAC_MAX_CHANNELS))

/* get a FDmaPs_channelNumber from a channel index*/
#define DMAC_CH_NUM(ch_idx)  (0x101 << (ch_idx))

/* power of 2 macro*/
#define POW2(pow)  (1 << (pow))

/* Is A greater than or equal to B macro ?*/
#define A_MAXEQ_B(a,b)  ((a) >= (b))


/*****************************************************************************
* DESCRIPTION
*  This data type is used to select the master interface number
*  on the DMA Controller when using the specified driver API
*  functions.
* NOTES
*  This data type relates directly to the following DMA Controller
*  register(s) / bit-field(s): (x = channel number)
*    - CTLx.SMS, CTLx.DMS, LLPx.LMS
*
*****************************************************************************/
#define DMA_MASTER_1   (0x00)    /* Dmac_master1 = 0x0,*/
#define DMA_MASTER_2   (0x01)    /* Dmac_master2 = 0x1,*/
#define DMA_MASTER_3   (0x02)    /* Dmac_master3 = 0x2,*/
#define DMA_MASTER_4   (0x03)    /* Dmac_master4 = 0x3*/

/*****************************************************************************
* DESCRIPTION
*  This data type is used for selecting the burst transfer length
*  on the source and/or destination of a DMA channel when using the
*  specified driver API functions. These transfer length values do
*  not relate to the AMBA HBURST parameter.
* NOTES
*  This data type relates directly to the following DMA Controller
*  register(s) / bit field(s): (x = channel number)
*    - CTLx.SRC_MSIZE, CTLx.DEST_MSIZE
*
*****************************************************************************/
#define DMA_MSIZE_1     (0x00)    /* Dmac_msize_1   = 0x0,*/
#define DMA_MSIZE_4     (0x01)    /* Dmac_msize_4   = 0x1,*/
#define DMA_MSIZE_8     (0x02)    /* Dmac_msize_8   = 0x2,*/
#define DMA_MSIZE_16    (0x03)    /* Dmac_msize_16  = 0x3,*/
#define DMA_MSIZE_32    (0x04)    /* Dmac_msize_32  = 0x4,*/
#define DMA_MSIZE_64    (0x05)    /* Dmac_msize_64  = 0x5,*/
#define DMA_MSIZE_128   (0x06)    /* Dmac_msize_128 = 0x6,*/
#define DMA_MSIZE_256   (0x07)    /* Dmac_msize_256 = 0x7*/

/*****************************************************************************
* DESCRIPTION
*  This data type is used for selecting the address increment
*  type for the source and/or destination on a DMA channel when using
*  the specified driver API functions.
* NOTES
*  This data type relates directly to the following DMA Controller
*  register(s) / bit-field(s): (x = channel number)
*    - CTLx.SINC, CTLx.DINC
*
*****************************************************************************/
#define DMA_ADDR_MODE_INC     (0x00)    /* Dmac_addr_increment = 0x0,*/
#define DMA_ADDR_MODE_DEC     (0x01)    /* Dmac_addr_decrement = 0x1,*/
#define DMA_ADDR_MODE_NO_CHG  (0x02)    /* Dmac_addr_nochange  = 0x2*/
	

/*****************************************************************************
* DESCRIPTION
*  This data type is used for selecting the transfer width for the
*  source and/or destination on a DMA channel when using the specified
*  driver API functions. This data type maps directly to the AMBA AHB
*  HSIZE parameter.
* NOTES
*  This data type relates directly to the following DMA Controller
*  register(s) / bit field(s): (x = channel number)
*    - CTLx.SRC_TR_WIDTH, CTLx.DST_TR_WIDTH
*
*****************************************************************************/
#define DMA_XFER_WIDTH_8    (0x00)    /* Dmac_trans_width_8   = 0x0,*/
#define DMA_XFER_WIDTH_16   (0x01)    /* Dmac_trans_width_16  = 0x1,*/
#define DMA_XFER_WIDTH_32   (0x02)    /* Dmac_trans_width_32  = 0x2,*/
#define DMA_XFER_WIDTH_64   (0x03)    /* Dmac_trans_width_64  = 0x3,*/
#define DMA_XFER_WIDTH_128  (0x04)    /* Dmac_trans_width_128 = 0x4,*/
#define DMA_XFER_WIDTH_256  (0x05)    /* Dmac_trans_width_256 = 0x5*/


/*****************************************************************************
* DESCRIPTION
*  This data type is used for selecting the handshaking interface
*  number for the source and/or destination on a DMA channel when
*  using the specified driver API functions.
* NOTES
*  This data type relates directly to the following DMA Controller
*  register(s) / bit-field(s): (x = channel number)
*    - CFGx.DEST_PER, CFGx.SRC_PER
*
*****************************************************************************/
#define  DMA_HANDSHAK_IF_0   (0x00)    /* Dmac_hs_if0  = 0x0,*/
#define  DMA_HANDSHAK_IF_1   (0x01)    /* Dmac_hs_if1  = 0x1,*/
#define  DMA_HANDSHAK_IF_2   (0x02)    /* Dmac_hs_if2  = 0x2,*/
#define  DMA_HANDSHAK_IF_3   (0x03)    /* Dmac_hs_if3  = 0x3,*/
#define  DMA_HANDSHAK_IF_4   (0x04)    /* Dmac_hs_if4  = 0x4,*/
#define  DMA_HANDSHAK_IF_5   (0x05)    /* Dmac_hs_if5  = 0x5,*/
#define  DMA_HANDSHAK_IF_6   (0x06)    /* Dmac_hs_if6  = 0x6,*/
#define  DMA_HANDSHAK_IF_7   (0x07)    /* Dmac_hs_if7  = 0x7,*/
#define  DMA_HANDSHAK_IF_8   (0x08)    /* Dmac_hs_if8  = 0x8,*/
#define  DMA_HANDSHAK_IF_9   (0x09)    /* Dmac_hs_if9  = 0x9,*/
#define  DMA_HANDSHAK_IF_10  (0x0A)    /* Dmac_hs_if10 = 0xa,*/
#define  DMA_HANDSHAK_IF_11  (0x0B)    /* Dmac_hs_if11 = 0xb,*/
#define  DMA_HANDSHAK_IF_12  (0x0C)    /* Dmac_hs_if12 = 0xc,*/
#define  DMA_HANDSHAK_IF_13  (0x0D)    /* Dmac_hs_if13 = 0xd,*/
#define  DMA_HANDSHAK_IF_14  (0x0E)    /* Dmac_hs_if14 = 0xe,*/
#define  DMA_HANDSHAK_IF_15  (0x0F)    /* Dmac_hs_if15 = 0xf*/


/*****************************************************************************
* DESCRIPTION
*  This data type is used for selecting the polarity level for the
*  source and/or destination on a DMA channel's handshaking interface
*  when using the specified driver API functions.
* NOTES
*  This data type relates directly to the following DMA Controller
*  register(s) / bit-field(s): (x = channel number)
*    - CFGx.SRC_HS_POL, CFGx.DST_HS_POL
*
*****************************************************************************/
#define  DMA_ACTIVE_HI    (0x00)   /* Dmac_active_high = 0x0,*/
#define  DMA_ACTIVE_LOW   (0x01)   /* Dmac_active_low  = 0x1*/

/*****************************************************************************
* DESCRIPTION
*  This data type is used to select a software or hardware interface
*  when using the specified driver API functions to access the
*  handshaking interface on a specified DMA channel.
* NOTES
*  This data type relates directly to the following DMA Controller
*  register(s) / bit field(s): (x = channel number)
*    - CFGx.HS_SEL_SRC, CFGx.HS_SEL_DST
*
*****************************************************************************/
#define DMA_HARD_IF   (0x00)   /* Dmac_hs_hardware   = 0x0,*/
#define DMA_SOFT_IF   (0x01)   /* Dmac_hs_software   = 0x1*/


/*****************************************************************************
* DESCRIPTION
*  This data type is used for selecting the tranfer protection level
*  on a DMA channel when using the specified driver API functions.
*  This data type maps directly to the AMBA AHB HPROT parameter.
* NOTES
*  This data type relates directly to the following DMA Controller
*  register(s) / bit-field(s): (x = channel number)
*    - CFGx.PROTCTL
*
*****************************************************************************/
#define  DMA_NOCACHE_NOBUF_NOPRIV_OPCODE  (0x00)    /* Dmac_noncache_nonbuff_nonpriv_opcode = 0x0, /* default prot level */
#define  DMA_NOCACHE_NOBUF_NOPRIV_DATA    (0x01)    /* Dmac_noncache_nonbuff_nonpriv_data   = 0x1,*/

#define  DMA_NOCACHE_NOBUF_PRIV_OPCODE  (0x02)    /* Dmac_noncache_nonbuff_priv_opcode    = 0x2,*/
#define  DMA_NOCACHE_NOBUF_PRIV_DATA    (0x03)    /* Dmac_noncache_nonbuff_priv_data      = 0x3,*/

#define  DMA_NOCACHE_BUF_NOPRIV_OPCODE  (0x04)    /* Dmac_noncache_buff_nonpriv_opcode    = 0x4,*/
#define  DMA_NOCACHE_BUF_NOPRIV_DATA    (0x05)    /* Dmac_noncache_buff_nonpriv_data      = 0x5,*/

#define  DMA_NOCACHE_BUF_PRIV_OPCODE  (0x06)    /* Dmac_noncache_buff_priv_opcode       = 0x6,*/
#define  DMA_NOCACHE_BUF_PRIV_DATA    (0x07)    /* Dmac_noncache_buff_priv_data         = 0x7,*/

#define  DMA_CACHE_NOBUF_NOPRIV_OPCODE  (0x08)    /* Dmac_cache_nonbuff_nonpriv_opcode    = 0x8,*/
#define  DMA_CACHE_NOBUF_NOPRIV_DATA    (0x09)    /* Dmac_cache_nonbuff_nonpriv_data      = 0x9,*/

#define  DMA_CACHE_NOBUF_PRIV_OPCODE  (0x0A)    /* Dmac_cache_nonbuff_priv_opcode       = 0xa,*/
#define  DMA_CACHE_NOBUF_PRIV_DATA    (0x0B)    /* Dmac_cache_nonbuff_priv_data         = 0xb,*/

#define  DMA_CACHE_BUF_NOPRIV_OPCODE  (0x0C)    /* Dmac_cache_buff_nonpriv_opcode       = 0xc,*/
#define  DMA_CACHE_BUF_NOPRIV_DATA    (0x0D)    /* Dmac_cache_buff_nonpriv_data         = 0xd,*/

#define  DMA_CACHE_BUF_PRIV_OPCODE  (0x0E)    /* Dmac_cache_buff_priv_opcode          = 0xe,*/
#define  DMA_CACHE_BUF_PRIV_DATA    (0x0F)    /* Dmac_cache_buff_priv_data            = 0xf*/

/*****************************************************************************
* DESCRIPTION
*  This data type is used for selecting the FIFO mode on a DMA
*  channel when using the specified driver API functions.
* NOTES
*  This data type relates directly to the following DMA Controller
*  register(s) / bit field(s): (x = channel number)
*    - CFGx.FIFO_MODE
*
*****************************************************************************/
#define DMA_FIFO_MODE_SINGLE  (0x00)    /* Dmac_fifo_mode_single = 0x0,*/
#define DMA_FIFO_MODE_HALF    (0x01)    /* Dmac_fifo_mode_half   = 0x1*/

/*****************************************************************************
* DESCRIPTION
*  This data type is used for selecting the flow control mode on a
*  DMA channel when using the specified driver API functions.
* NOTES
*  This data type relates directly to the following DMA Controller
*  register(s) / bit-field(s): (x = channel number)
*    - CFGx.FCMODE
*
*****************************************************************************/
#define DMA_DATA_PREFETCH_ENABLE    (0x00)   /* Dmac_data_prefetch_enabled  = 0x0,*/
#define DMA_DATA_PREFETCH_DISABLE   (0x01)   /* Dmac_data_prefetch_disabled = 0x1*/


/*****************************************************************************
* DESCRIPTION
*  This data type is used for selecting the lock level on a DMA
*  channel when using the specified driver API functions.
* NOTES
*  This data type relates directly to the following DMA Controller
*  register(s)/bit field(s): (x = channel number)
*    - CFGx.LOCK_B_L, CFGx.LOCK_CH_L
*
*****************************************************************************/
#define DMA_LOCKLVL_DMA_XFER    (0x00)  /* Dmac_lock_level_dma_transfer   = 0x0,*/
#define DMA_LOCKLVL_BLK_XFER    (0x01)  /* Dmac_lock_level_block_transfer = 0x1,*/
#define DMA_LOCKLVL_XFER        (0x02)  /* Dmac_lock_level_transaction    = 0x2*/


/*****************************************************************************
* DESCRIPTION
*  This data type is used for selecting the priority level of a DMA
*  channel when using the specified driver API functions.
* NOTES
*  This data type relates directly to the following DMA Controller
*  register(s)/bit field(s): (x = channel number)
*    - CFGx.CH_PRIOR
*
*****************************************************************************/
#define  DMA_PRIORITY_0  (0x00)    /* Dmac_priority_0 = 0x0,*/
#define  DMA_PRIORITY_1  (0x01)    /* Dmac_priority_1 = 0x1,*/
#define  DMA_PRIORITY_2  (0x02)    /* Dmac_priority_2 = 0x2,*/
#define  DMA_PRIORITY_3  (0x03)    /* Dmac_priority_3 = 0x3,*/
#define  DMA_PRIORITY_4  (0x04)    /* Dmac_priority_4 = 0x4,*/
#define  DMA_PRIORITY_5  (0x05)    /* Dmac_priority_5 = 0x5,*/
#define  DMA_PRIORITY_6  (0x06)    /* Dmac_priority_6 = 0x6,*/
#define  DMA_PRIORITY_7  (0x07)    /* Dmac_priority_7 = 0x7*/

/*****************************************************************************
* DESCRIPTION
*  This data type is used to select the source and/or the
*  destination for a specific DMA channel when using some
*  of the driver's API functions.
*  This data type is used by many of the API functions in the driver.
*
*****************************************************************************/
#define DMA_SRC      (0x01)  /*   Dmac_src     = 0x1,*/
#define DMA_DST      (0x02)  /*   Dmac_dst     = 0x2,*/
#define DMA_SRC_DST  (0x03)  /*   Dmac_src_dst = 0x3*/



/*****************************************************************************
* DESCRIPTION
*  This structure is used when creating Linked List Items.
*
*****************************************************************************/
typedef struct _dma_lli_item_ 
{
    UINT32  sar;
    UINT32  dar;
    UINT32  llp;
    UINT32  ctl_l;
    UINT32  ctl_h;
    UINT32  sstat;
    UINT32  dstat;
	
    FMSH_listHead list;
	
} vxT_LINKLIST_ITEM; /* FDmaPs_LliItem_T;*/


/*****************************************************************************
* DESCRIPTION
*  This structure is used to set configuration parameters for
*  a channel in the DMA Controller. All of these configuration
*  parameters must be programmed into the DMA controller before
*  enabling the channel. The members of this structure map directly
*  to a channel's register/bit field within the DMAC device. 
* NOTES
*  To initialize the structure, the user should call the
*  FDmaPs_getChannelConfig() API function after reset of the DMA
*  controller. This sets the FDmaPs_ChannelConfig_T structure members
*  to the DMA controllers reset values. This allows the user to
*  change only the structure members that need to be different from
*  the default values and then call the FDmaPs_setChannelConfig()
*  function to set up the DMA channel transfer.
*
*****************************************************************************/
typedef struct _dma_chn_cfg_ 
{
    UINT32                        sar;
    UINT32                        dar;
	
    int ctl_llp_src_en; /* enum FMSH_state*/
    int ctl_llp_dst_en; /* enum FMSH_state*/
    
    int ctl_sms;  /* enum FDmaPs_masterNumber*/
    int ctl_dms;  /* enum FDmaPs_masterNumber*/
	
    int ctl_src_msize;  /* enum FDmaPs_burstTransLength*/
    int ctl_dst_msize;  /* enum FDmaPs_burstTransLength*/
	
    int ctl_sinc;  /* enum FDmaPs_addressIncrement*/
    int ctl_dinc;  /* enum FDmaPs_addressIncrement*/
	
    int ctl_src_tr_width;  /*  enum FDmaPs_transferWidth*/
    int ctl_dst_tr_width;  /* enum FDmaPs_transferWidth*/
	
    UINT32                        sstat;
    UINT32                        dstat;
	
    UINT32                        sstatar;
    UINT32                        dstatar;
	
    int cfg_dst_per;  /* enum FDmaPs_hsInterface*/
    int cfg_src_per;  /* enum FDmaPs_hsInterface       */
	
    int cfg_ss_upd_en;  /* enum FMSH_state                   */
    int cfg_ds_upd_en;  /* enum FMSH_state                   */
	
    int cfg_reload_src;  /* enum FMSH_state                  */
    int cfg_reload_dst;  /* enum FMSH_state                  */
	
    int cfg_src_hs_pol;  /* enum FDmaPs_polarityLevel     */
    int cfg_dst_hs_pol;  /* enum FDmaPs_polarityLevel    */
	
    int cfg_hs_sel_src;  /* enum FDmaPs_swHwHsSelect    */
    int cfg_hs_sel_dst;  /* enum FDmaPs_swHwHsSelect    */

    UINT32                        llp_loc;
	
    int llp_lms;   /* enum FDmaPs_masterNumber      */
    int ctl_done;  /* enum FMSH_state               */
	
    UINT32                        ctl_block_ts;
	
    int ctl_xfer_path;  /* enum FDmaPs_transferFlow   */
    
    int ctl_dst_scatter_en;  /* enum FMSH_state                   */
    int ctl_src_gather_en;   /* enum FMSH_state                   */
    int ctl_int_en;          /* enum FMSH_state                   */
	
    int cfg_protctl;    /* enum FDmaPs_protLevel         */
    int cfg_fifo_mode;  /* enum FDmaPs_fifoMode          */
    int cfg_fcmode;     /* enum FDmaPs_flowCtlMode      */
	
    UINT32                        cfg_max_abrst;
	
    int cfg_lock_b;   /* enum FMSH_state                  */
    int cfg_lock_ch;  /* enum FMSH_state             */
     
    int cfg_lock_b_l;    /* enum FDmaPs_lockLevel         */
    int cfg_lock_ch_l;   /* enum FDmaPs_lockLevel        */
	
    int cfg_ch_prior;    /* enum FDmaPs_channelPriority   */
	
    UINT32                        sgr_sgc;
    UINT32                        sgr_sgi;
	
    UINT32                        dsr_dsc;
    UINT32                        dsr_dsi;
	
}vxT_DMA_CHNCFG;


/*****************************************************************************
* DESCRIPTION
*  This is the structure used for accessing the dmac register portmap.
*
*****************************************************************************/
typedef struct _dma_port_map_ 
{

  /* Channel registers                                    */
  /* The offset address for each of the channel registers */
  /*  is shown for channel 0. For other channel numbers   */
  /*  use the following equation.                         */
  /*                                                      */
  /*    offset = (channel_num * 0x058) + channel_0 offset */
  /*                                                      */
  struct 
  {
      volatile UINT32 sar_l;     /* Source Address Reg      (0x000) */
      volatile UINT32 sar_h;
      volatile UINT32 dar_l;     /* Destination Address Reg (0x008) */
      volatile UINT32 dar_h;
      volatile UINT32 llp_l;     /* Linked List Pointer Reg (0x010) */
      volatile UINT32 llp_h;  
      volatile UINT32 ctl_l;     /* Control Reg             (0x018) */
      volatile UINT32 ctl_h;
      volatile UINT32 sstat_l;   /* Source Status Reg       (0x020) */
      volatile UINT32 sstat_h;
      volatile UINT32 dstat_l;   /* Destination Status Reg  (0x028) */
      volatile UINT32 dstat_h;   
      volatile UINT32 sstatar_l; /* Source Status Addr Reg  (0x030) */
      volatile UINT32 sstatar_h; 
      volatile UINT32 dstatar_l; /* Dest Status Addr Reg    (0x038) */
      volatile UINT32 dstatar_h; 
      volatile UINT32 cfg_l;     /* Configuration Reg       (0x040) */
      volatile UINT32 cfg_h;
      volatile UINT32 sgr_l;     /* Source Gather Reg       (0x048) */
      volatile UINT32 sgr_h; 
      volatile UINT32 dsr_l;     /* Destination Scatter Reg (0x050) */
      volatile UINT32 dsr_h;
  } CHN[8];
  
  /* Interrupt Raw Status Registers */
  volatile UINT32 raw_tfr_l;     /* Raw Status for IntTfr   (0x2c0) */
  volatile UINT32 raw_tfr_h;   
  volatile UINT32 raw_block_l;   /* Raw Status for IntBlock (0x2c8) */
  volatile UINT32 raw_block_h;
  volatile UINT32 raw_srctran_l; /* Raw Status IntSrcTran   (0x2d0) */
  volatile UINT32 raw_srctran_h; 
  volatile UINT32 raw_dsttran_l; /* Raw Status IntDstTran   (0x2d8) */
  volatile UINT32 raw_dsttran_h;
  volatile UINT32 raw_err_l;     /* Raw Status for IntErr   (0x2e0) */
  volatile UINT32 raw_err_h;

  /* Interrupt Status Registers */
  volatile UINT32 status_tfr_l;    /* Status for IntTfr     (0x2e8) */
  volatile UINT32 status_tfr_h;
  volatile UINT32 status_block_l;  /* Status for IntBlock   (0x2f0) */
  volatile UINT32 status_block_h;
  volatile UINT32 status_srctran_l;/* Status for IntSrcTran (0x2f8) */
  volatile UINT32 status_srctran_h;
  volatile UINT32 status_dsttran_l;/* Status for IntDstTran (0x300) */
  volatile UINT32 status_dsttran_h;
  volatile UINT32 status_err_l;    /* Status for IntErr     (0x308) */
  volatile UINT32 status_err_h;

  /* Interrupt Mask Registers */
  volatile UINT32 mask_tfr_l;      /* Mask for IntTfr       (0x310) */
  volatile UINT32 mask_tfr_h;
  volatile UINT32 mask_block_l;    /* Mask for IntBlock     (0x318) */
  volatile UINT32 mask_block_h;
  volatile UINT32 mask_srctran_l;  /* Mask for IntSrcTran   (0x320) */
  volatile UINT32 mask_srctran_h;
  volatile UINT32 mask_dsttran_l;  /* Mask for IntDstTran   (0x328) */
  volatile UINT32 mask_dsttran_h;
  volatile UINT32 mask_err_l;      /* Mask for IntErr       (0x330) */
  volatile UINT32 mask_err_h;

  /* Interrupt Clear Registers */
  volatile UINT32 clear_tfr_l;     /* Clear for IntTfr      (0x338) */
  volatile UINT32 clear_tfr_h;
  volatile UINT32 clear_block_l;   /* Clear for IntBlock    (0x340) */
  volatile UINT32 clear_block_h;
  volatile UINT32 clear_srctran_l; /* Clear for IntSrcTran  (0x348) */
  volatile UINT32 clear_srctran_h;
  volatile UINT32 clear_dsttran_l; /* Clear for IntDstTran  (0x350) */
  volatile UINT32 clear_dsttran_h;
  volatile UINT32 clear_err_l;     /* Clear for IntErr      (0x358) */
  volatile UINT32 clear_err_h;
  volatile UINT32 status_int_l;    /* Combined Intr Status  (0x360) */
  volatile UINT32 status_int_h;

  /* Software Handshaking Registers */
  volatile UINT32 req_src_reg_l; /* Src Sw Transaction Req  (0x368) */
  volatile UINT32 req_src_reg_h;   
  volatile UINT32 req_dst_reg_l; /* Dest Sw Transaction Req (0x370) */
  volatile UINT32 req_dst_reg_h;
  volatile UINT32 sgl_rq_src_reg_l; /* Sgl Src Transac Req  (0x378) */
  volatile UINT32 sgl_rq_src_reg_h;
  volatile UINT32 sgl_rq_dst_reg_l; /* Sgl Dest Transac Req (0x380) */
  volatile UINT32 sgl_rq_dst_reg_h;
  volatile UINT32 lst_src_reg_l;   /* Last Src Transac Req  (0x388) */
  volatile UINT32 lst_src_reg_h;
  volatile UINT32 lst_dst_reg_l;   /* Last Dest Transac Req (0x390) */
  volatile UINT32 lst_dst_reg_h;

  /* Misc Registers */
  volatile UINT32 dma_cfg_reg_l; /* Configuration Register  (0x398) */
  volatile UINT32 dma_cfg_reg_h;
  volatile UINT32 ch_en_reg_l;   /* Channel Enable Register (0x3a0) */
  volatile UINT32 ch_en_reg_h;
  volatile UINT32 dma_id_reg_l;    /* ID Register           (0x3a8) */
  volatile UINT32 dma_id_reg_h;
  volatile UINT32 dma_test_reg_l;  /* Test Register         (0x3b0) */
  volatile UINT32 dma_test_reg_h;
  volatile UINT32 old_version_id_l;/* legacy support        (0x3b8) */
  volatile UINT32 old_version_id_h;
  volatile UINT32 reserved_low;    /* reserved              (0x3c0) */
  volatile UINT32 reserved_high;
  volatile UINT32 dma_comp_params_6_l;/* hardware params    (0x3c8) */
  volatile UINT32 dma_comp_params_6_h;
  volatile UINT32 dma_comp_params_5_l;/* hardware params    (0x3d0) */
  volatile UINT32 dma_comp_params_5_h;
  volatile UINT32 dma_comp_params_4_l;/* hardware params    (0x3d8) */
  volatile UINT32 dma_comp_params_4_h;
  volatile UINT32 dma_comp_params_3_l;/* hardware params    (0x3e0) */
  volatile UINT32 dma_comp_params_3_h;
  volatile UINT32 dma_comp_params_2_l;/* hardware params    (0x3e8) */
  volatile UINT32 dma_comp_params_2_h;
  volatile UINT32 dma_comp_params_1_l;/* hardware params    (0x3f0) */
  volatile UINT32 dma_comp_params_1_h;
  volatile UINT32 dma_version_id_l;/* Version ID Register   (0x3f8) */
  volatile UINT32 dma_version_id_h;
  
}vxT_DMA_PORTMAP;


/*****************************************************************************
* DESCRIPTION
*  This structure comprises the dmac hardware parameters that affect
*  the software driver.  This structure needs to be initialized with
*  the correct values and be pointed to by the (FMSH_device).cc
*  member of the relevant dmac device structure.
*
*****************************************************************************/
typedef struct _dma_param_ 
{
	int header;

	BOOL    static_endian_select;  /* static or dynamic cases for endian scheme */
	BOOL    encoded_params;   /* include encoded hardware parameters */

	UINT8   num_master_int;   /* number of AHB master interfaces     */
	UINT8   num_channels;     /* number of DMA channels              */
	UINT8   num_hs_int;       /* number of handshaking interfaces    */
	UINT8   intr_io;          /* individual or combined interrupts   */

	BOOL    mabrst;           /* Max AMBA burst length               */
	BOOL    big_endian;       /* big or little endian 1=big          */

	UINT16  s_hdata_width;    /* AHB slave data bus width            */
	UINT16  m_hdata_width[4]; /* AHB master data bus widths          */  
	                          /*  can be - 32,64,128,256             */

	UINT16  ch_fifo_depth[8]; /* channels FIFO depths in bytes       */
	                          /*  can be - 4,8,16,32,64,128,256            */

	UINT16  ch_max_mult_size[8]; /* channel max burst transaction    */
	                             /*  sizes can be - 4,8,16,32,64,    */
	                             /*  128,256                         */
								 
	UINT16  ch_max_blk_size[8];  /* channel max block sizes          */
	                             /*  can be - 3,7,15,31,63,127,255,  */
	                             /*  511, 1023,2047,4095             */
								 
	UINT8   ch_fc[8];        /* channels flow control hard-codded?   */
	                         /*  0=DMA_only, 1=SRC_only, 2=DST_only  */
	                         /*  3=ANY                               */
							 
	BOOL    ch_lock_en[8];   /* bus locking for each channel         */
							 
	UINT16  ch_stw[8];       /* hard-code src channel transfer width */
	                         /*  0=NO-HARD_CODE, 8=BYTE, 16=HALFWORD */
	                         /*  32=WORD, 64=TWO_WORD, 128=FOUR_WORD */ 
	                         /*  256=EIGHT_WORD                      */
							 
	UINT16  ch_dtw[8];       /* hard-code dst channel transfer width */
	                         /*  uses same encoding as the src above */

	UINT16  ch_multi_blk_type[8];  /* Multi block transfers types    */
							 
	BOOL    ch_multi_blk_en[8];  /* Enable multi block transfers     */  
	BOOL    ch_ctl_wb_en[8]; /* Transfer write back enable           */
	BOOL    ch_hc_llp[8];    /* hard-code LLP register               */
	                         /*  1=hard-coded, 0=not hard-coded      */

	BOOL    ch_dst_sca_en[8];/* enable scatter feature on dst        */
	BOOL    ch_src_gat_en[8];/* enable gather feature on src         */

	UINT8   ch_sms[8];       /* hard-code master interface on src    */
	                         /*  0=master_1, 1=master_2, 2=master_3, */  
	                         /*  3=master_4, 4=no_hard-code          */
							 
	UINT8   ch_dms[8];       /* hard-code master interface on dst    */
	                         /*  same encoding as for src above      */

	UINT8   ch_lms[8];       /* hard-code master interface on LLP    */
	                         /*  same encoding as for src above      */

	BOOL    ch_stat_dst[8];  /* fetch status from dst peripheral     */
	                         /*  1=enable feature, 0=disable feature */

	BOOL    ch_stat_src[8];  /* fetch status from src peripheral     */
	                         /*  1=enable feature, 0=disable feature */

}vxT_DMA_PARAMS;


/*
devc ctrl
*/
typedef struct _t_dma_ctrl_
{
	int  ctrl_x;  /* 0, 1 */
	
	UINT32 cfgBaseAddr;		    /**< APB Base address of the device */
	
    struct 
	{
        UINT32 chn_x;
		
        UINT32 src_state;
        UINT32 dst_state;
		
        int block_cnt;      /* count of completed blocks*/
        int total_blocks;   /* total number blocks in the transfer*/
        
        int src_byte_cnt;   /* count of source bytes completed*/
        int dst_byte_cnt;   /* count of destination bytes completed*/
        
        int src_single_inc; /* src byte increment in single region*/
        int src_burst_inc;  /* src byte increment in burst region*/
        
        int dst_single_inc; /* dst byte increment in single region*/
        int dst_burst_inc;  /* dst byte increment in burst region*/
        
		int xfer_type;  /* trans_type // transfer type (row)*/
		int xfer_path;  /* tt_fc;       // transfer device type flow*/
                                        /* control*/
    } CHN[8];
	
    int ch_order[8]; /* channel order based on priority*/

	vxT_DMA_PARAMS DmaParams;
	
	UINT32 devIP_ver;
    UINT32 devIP_type;      /* identify peripheral types:  */

	int status;
} vxT_DMA_CTRL;


typedef struct _t_dma_
{
	int dma_x;	
	
	vxT_DMA_CTRL * pDmaCtrl;

	int init_flag;
} vxT_DMA;
	



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __INC_VX_DMA_H__ */


