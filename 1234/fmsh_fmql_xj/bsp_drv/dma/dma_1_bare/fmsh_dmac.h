/******************************************************************************
*
* Copyright (C) 2018 - 2028 FMSH, Inc.  All rights reserved.
*
******************************************************************************/
/******************************************************************************
*
* @file  fmsh_dmac.h
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
#ifndef _FMSH_DMAC_H_
#define _FMSH_DMAC_H_

/***************************** Include Files *********************************/

#include "../../common/fmsh_common.h"
/*#include "fmsh_common.h"*/

/* Allow C++ to use this header */
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/************************** Constant Definitions *****************************/

/**************************** Type Definitions *******************************/

/*****************************************************************************
* DESCRIPTION
*  This data type is used to describe the DMA controller's channel
*  number. The assigned enumerated value matches the register
*  value that needs to be written to enable the channel.
*  This data type is used by many of the API functions in the driver.
*
*****************************************************************************/
enum FDmaPs_channelNumber {
    Dmac_no_channel   = 0x0000,
    Dmac_channel0     = 0x0101,
    Dmac_channel1     = 0x0202,
    Dmac_channel2     = 0x0404,
    Dmac_channel3     = 0x0808,
    Dmac_channel4     = 0x1010,
    Dmac_channel5     = 0x2020,
    Dmac_channel6     = 0x4040,
    Dmac_channel7     = 0x8080,
    Dmac_all_channels = 0xffff
};

/*****************************************************************************
* DESCRIPTION
*  This data type is used to select the source and/or the
*  destination for a specific DMA channel when using some
*  of the driver's API functions.
*  This data type is used by many of the API functions in the driver.
*
*****************************************************************************/
enum FDmaPs_srcDstSelect {
    Dmac_src     = 0x1,
    Dmac_dst     = 0x2,
    Dmac_src_dst = 0x3
};

/*****************************************************************************
* DESCRIPTION
*  This data type is used to select the bus and/or the channel
*  when using the specified driver API functions to lock DMA
*  transfers. The selection affects which DMA Controller
*  bit field within the CFGx register is accessed.
*
*****************************************************************************/
enum FDmaPs_lockBusCh {
    Dmac_lock_bus     = 0x1,
    Dmac_lock_channel = 0x2,
    Dmac_lock_bus_ch  = 0x4
};

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
enum FDmaPs_swHwHsSelect {
    Dmac_hs_hardware   = 0x0,
    Dmac_hs_software   = 0x1
};

/*****************************************************************************
* DESCRIPTION
*  This data type is used to select the count or interval bit field
*  when using the specified driver API functions to access the
*  scatter or gather registers on the DMA controller.
* NOTES
*  This data type relates directly to the following DMA Controller
*  register(s) / bit field(s): (x = channel number)
*    - SGRx.SGC, SGRx.SGI, DSRx.DSC, DSRx.DSI
*
*****************************************************************************/
enum FDmaPs_scatterGatherParam {
    Dmac_sg_count    = 0x0,
    Dmac_sg_interval = 0x1
};

/*****************************************************************************
* DESCRIPTION
*  This data type is used to select the interrupt type on a specified
*  DMA channel when using the specified driver API functions to access
*  interrupt registers within the DMA Controller.
*
*****************************************************************************/
enum FDmaPs_irq {
    Dmac_irq_none       = 0x00,     /* no interrupts*/
    Dmac_irq_tfr        = 0x01,     /* transfer complete*/
    Dmac_irq_block      = 0x02,     /* block transfer complete*/
    Dmac_irq_srctran    = 0x04,     /* source transaction complete*/
    Dmac_irq_dsttran    = 0x08,     /* destination transaction complete*/
    Dmac_irq_err        = 0x10,     /* error*/
    Dmac_irq_all        = 0x1f      /* all interrupts*/
};

/*****************************************************************************
* DESCRIPTION
*  This data type is used to select which of the software request
*  registers are accessed within the DMA Controller when using the
*  specified driver API functions.
*
*****************************************************************************/
enum FDmaPs_softwareReq {
    Dmac_request        = 0x1, /* ReqSrcReq/ReqDstReq */
    Dmac_single_request = 0x2, /* SglReqSrcReq/SglReqDstReq */
    Dmac_last_request   = 0x4  /* LstReqSrcReq/LstReqDstReq */
};

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
enum FDmaPs_masterNumber {
    Dmac_master1 = 0x0,
    Dmac_master2 = 0x1,
    Dmac_master3 = 0x2,
    Dmac_master4 = 0x3
};

/*****************************************************************************
* DESCRIPTION
*  This data type is used to select the multi block type
*  on the DMA Controller when using the specified driver API
*  functions.
*
*****************************************************************************/
enum FDmaPs_multiBlkType {
    Dmac_no_hardcode   = 0x0, /* transfer type row1 / row5 */
    Dmac_cont_reload   = 0x1, /* transfer type row2 */
    Dmac_reload_cont   = 0x2, /* transfer type row3 */
    Dmac_reload_reload = 0x3, /* transfer type row4 */
    Dmac_cont_llp      = 0x4, /* transfer type row6 */
    Dmac_reload_llp    = 0x5, /* transfer type row7 */
    Dmac_llp_cont      = 0x6, /* transfer type row8 */
    Dmac_llp_reload    = 0x7, /* transfer type row9 */
    Dmac_llp_llp       = 0x8  /* transfer type row10 */
};

/*****************************************************************************
* DESCRIPTION
*  This data type is used for selecting the transfer type for a
*  specified DMA channel when using the specified driver API
*  functions. See the DW_ahb_dmac databook for a detailed
*  description on these transfer types.
*
*****************************************************************************/
enum FDmaPs_transferType {
    Dmac_transfer_row1  = 0x1, /* single block or last multi-block */
                               /*  no write back                   */
	
    Dmac_transfer_row2  = 0x2, /* multi-block auto-reload DAR      */
                               /*  contiguous SAR no write back    */
	
    Dmac_transfer_row3  = 0x3, /* multi-block auto reload SAR      */
                               /*  contiguous DAR no write back    */
	
    Dmac_transfer_row4  = 0x4, /* multi-block auto-reload SAR DAR  */
                               /*  no write back                   */
	
    Dmac_transfer_row5  = 0x5, /* single block or last multi-block */
                               /*  with write back                 */
	
    Dmac_transfer_row6  = 0x6, /* multi-block linked list DAR      */
                               /*  contiguous SAR with write back  */
	
    Dmac_transfer_row7  = 0x7, /* multi-block linked list DAR auto */
                               /*  reload SAR  with write back     */
	
    Dmac_transfer_row8  = 0x8, /* multi-block linked list SAR      */
                               /*  contiguous DAR with write back  */
	
    Dmac_transfer_row9  = 0x9, /* multi-block linked list SAR auto */
                               /*  reload DAR with write back      */
	
    Dmac_transfer_row10 = 0xa  /* multi-block linked list SAR DAR  */
                               /*  with write back                 */
};

/*****************************************************************************
* DESCRIPTION
*  This data type is used for selecting the transfer flow device
*  (memory or peripheral device) and for setting the flow control
*  device for the DMA transfer when using the specified driver
*  API functions.
* NOTES
*  This data type relates directly to the following DMA Controller
*  register(s) / bit field(s): (x = channel number)
*    - CTLx.TT_FC
*
*****************************************************************************/
enum FDmaPs_transferFlow {
    Dmac_mem2mem_dma    = 0x0, /* mem to mem - DMAC   flow ctlr */
    Dmac_mem2prf_dma    = 0x1, /* mem to prf - DMAC   flow ctlr */
    Dmac_prf2mem_dma    = 0x2, /* prf to mem - DMAC   flow ctlr */
    Dmac_prf2prf_dma    = 0x3, /* prf to prf - DMAC   flow ctlr */
    
    Dmac_prf2mem_prf    = 0x4, /* prf to mem - periph flow ctlr */
    Dmac_prf2prf_srcprf = 0x5, /* prf to prf - source flow ctlr */
    Dmac_mem2prf_prf    = 0x6, /* mem to prf - periph flow ctlr */
    Dmac_prf2prf_dstprf = 0x7  /* prf to prf - dest   flow ctlr */
};

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
enum FDmaPs_burstTransLength {
    Dmac_msize_1   = 0x0,
    Dmac_msize_4   = 0x1,
    Dmac_msize_8   = 0x2,
    Dmac_msize_16  = 0x3,
    Dmac_msize_32  = 0x4,
    Dmac_msize_64  = 0x5,
    Dmac_msize_128 = 0x6,
    Dmac_msize_256 = 0x7
};

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
enum FDmaPs_addressIncrement {
    Dmac_addr_increment = 0x0,
    Dmac_addr_decrement = 0x1,
    Dmac_addr_nochange  = 0x2
};

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
enum FDmaPs_transferWidth {
    Dmac_trans_width_8   = 0x0,
    Dmac_trans_width_16  = 0x1,
    Dmac_trans_width_32  = 0x2,
    Dmac_trans_width_64  = 0x3,
    Dmac_trans_width_128 = 0x4,
    Dmac_trans_width_256 = 0x5
};

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
enum FDmaPs_hsInterface {
    Dmac_hs_if0  = 0x0,
    Dmac_hs_if1  = 0x1,
    Dmac_hs_if2  = 0x2,
    Dmac_hs_if3  = 0x3,
    Dmac_hs_if4  = 0x4,
    Dmac_hs_if5  = 0x5,
    Dmac_hs_if6  = 0x6,
    Dmac_hs_if7  = 0x7,
    Dmac_hs_if8  = 0x8,
    Dmac_hs_if9  = 0x9,
    Dmac_hs_if10 = 0xa,
    Dmac_hs_if11 = 0xb,
    Dmac_hs_if12 = 0xc,
    Dmac_hs_if13 = 0xd,
    Dmac_hs_if14 = 0xe,
    Dmac_hs_if15 = 0xf
};

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
enum FDmaPs_protLevel {
   Dmac_noncache_nonbuff_nonpriv_opcode = 0x0, /* default prot level */
   Dmac_noncache_nonbuff_nonpriv_data   = 0x1,
   Dmac_noncache_nonbuff_priv_opcode    = 0x2,
   Dmac_noncache_nonbuff_priv_data      = 0x3,
   Dmac_noncache_buff_nonpriv_opcode    = 0x4,
   Dmac_noncache_buff_nonpriv_data      = 0x5,
   Dmac_noncache_buff_priv_opcode       = 0x6,
   Dmac_noncache_buff_priv_data         = 0x7,
   Dmac_cache_nonbuff_nonpriv_opcode    = 0x8,
   Dmac_cache_nonbuff_nonpriv_data      = 0x9,
   Dmac_cache_nonbuff_priv_opcode       = 0xa,
   Dmac_cache_nonbuff_priv_data         = 0xb,
   Dmac_cache_buff_nonpriv_opcode       = 0xc,
   Dmac_cache_buff_nonpriv_data         = 0xd,
   Dmac_cache_buff_priv_opcode          = 0xe,
   Dmac_cache_buff_priv_data            = 0xf
};

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
enum FDmaPs_fifoMode {
    Dmac_fifo_mode_single = 0x0,
    Dmac_fifo_mode_half   = 0x1
};

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
enum FDmaPs_flowCtlMode {
    Dmac_data_prefetch_enabled  = 0x0,
    Dmac_data_prefetch_disabled = 0x1
};

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
enum FDmaPs_polarityLevel {
    Dmac_active_high = 0x0,
    Dmac_active_low  = 0x1
};

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
enum FDmaPs_lockLevel {
    Dmac_lock_level_dma_transfer   = 0x0,
    Dmac_lock_level_block_transfer = 0x1,
    Dmac_lock_level_transaction    = 0x2
};

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
enum FDmaPs_channelPriority {
    Dmac_priority_0 = 0x0,
    Dmac_priority_1 = 0x1,
    Dmac_priority_2 = 0x2,
    Dmac_priority_3 = 0x3,
    Dmac_priority_4 = 0x4,
    Dmac_priority_5 = 0x5,
    Dmac_priority_6 = 0x6,
    Dmac_priority_7 = 0x7
};

/*****************************************************************************
* DESCRIPTION
*  This data type is used to record the state of the DMA channels
*  source and destination in releation to the transfer.
*  Idle indicates that no software handshaking transfer is under
*  way. single_region indicates that the src/dst is in single
*  transaction region, and will fetch/deliever only one amba
*  word. burst_region indicates indicates that the src/dst is not
*  in single transaction region and perform a burst transfer.
*
*****************************************************************************/
enum FDmaPs_chState {
    Dmac_idle          = 0x0,
    Dmac_single_region = 0x1,
    Dmac_burst_region  = 0x2
};

/*****************************************************************************
* DESCRIPTION
*  This is the data type used for enabling software or hardware
*  handshaking for DMA transfers.  Using software handshaking changes
*  how an interrupt handler processes Rx full and Tx empty interrupts.
*  Any DesignWare peripheral which supports DMA transfers has API
*  function which match those listed below.
*
*****************************************************************************/
enum FMSH_dmaMode {
    FMSH_dma_none,            /* DMA is not being used*/
    FMSH_dma_sw_handshake,    /* DMA using software handshaking*/
    FMSH_dma_hw_handshake     /* DMA using hardware handshaking*/
};

/*****************************************************************************
* DESCRIPTION
*  This typedef contains configuration information for the device.
*
*****************************************************************************/
typedef struct _dmac_config {
    u16 DeviceId;	 /**< Unique ID  of device */
    u32 BaseAddress; /**< Base address of device (IPIF) */
}FDmaPs_Config;

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
typedef struct _dmac_channel_config {
    u32                        sar;
    u32                        dar;
    enum FMSH_state                   ctl_llp_src_en;
    enum FMSH_state                   ctl_llp_dst_en;
    enum FDmaPs_masterNumber      ctl_sms;
    enum FDmaPs_masterNumber      ctl_dms;
    enum FDmaPs_burstTransLength ctl_src_msize;
    enum FDmaPs_burstTransLength ctl_dst_msize;
    enum FDmaPs_addressIncrement  ctl_sinc;
    enum FDmaPs_addressIncrement  ctl_dinc;
    enum FDmaPs_transferWidth     ctl_src_tr_width;
    enum FDmaPs_transferWidth     ctl_dst_tr_width;
    u32                        sstat;
    u32                        dstat;
    u32                        sstatar;
    u32                        dstatar;
    enum FDmaPs_hsInterface       cfg_dst_per;
    enum FDmaPs_hsInterface       cfg_src_per;
    enum FMSH_state                   cfg_ss_upd_en;
    enum FMSH_state                   cfg_ds_upd_en;
    enum FMSH_state                   cfg_reload_src;
    enum FMSH_state                   cfg_reload_dst;
    enum FDmaPs_polarityLevel     cfg_src_hs_pol;
    enum FDmaPs_polarityLevel     cfg_dst_hs_pol;
    enum FDmaPs_swHwHsSelect    cfg_hs_sel_src;
    enum FDmaPs_swHwHsSelect    cfg_hs_sel_dst;

    u32                        llp_loc;
    enum FDmaPs_masterNumber      llp_lms;
    enum FMSH_state                   ctl_done;
    u32                        ctl_block_ts;
    enum FDmaPs_transferFlow      ctl_tt_fc;
    enum FMSH_state                   ctl_dst_scatter_en;
    enum FMSH_state                   ctl_src_gather_en;
    enum FMSH_state                   ctl_int_en;
    enum FDmaPs_protLevel         cfg_protctl;
    enum FDmaPs_fifoMode          cfg_fifo_mode;
    enum FDmaPs_flowCtlMode      cfg_fcmode;
    u32                        cfg_max_abrst;
    enum FMSH_state                   cfg_lock_b;
    enum FMSH_state                   cfg_lock_ch;
    enum FDmaPs_lockLevel         cfg_lock_b_l;
    enum FDmaPs_lockLevel         cfg_lock_ch_l;
    enum FDmaPs_channelPriority   cfg_ch_prior;
    u32                        sgr_sgc;
    u32                        sgr_sgi;
    u32                        dsr_dsc;
    u32                        dsr_dsi;
}FDmaPs_ChannelConfig_T;

/*****************************************************************************
* DESCRIPTION
*  This structure is used when creating Linked List Items.
*
*****************************************************************************/
typedef struct _dmac_lli_item {
    u32  sar;
    u32  dar;
    u32  llp;
    u32  ctl_l;
    u32  ctl_h;
    u32  sstat;
    u32  dstat;
    FMSH_listHead list;
}FDmaPs_LliItem_T;

/*****************************************************************************
* DESCRIPTION
*  This structure comprises the dmac hardware parameters that affect
*  the software driver.  This structure needs to be initialized with
*  the correct values and be pointed to by the (FMSH_device).cc
*  member of the relevant dmac device structure.
*
*****************************************************************************/
typedef struct _dmac_param 
{
  BOOL static_endian_select;  /* static or dynamic cases for endian scheme */
  BOOL encoded_params;   /* include encoded hardware parameters */
  u8   num_master_int;   /* number of AHB master interfaces     */
  u8   num_channels;     /* number of DMA channels              */
  u8   num_hs_int;       /* number of handshaking interfaces    */
  u8   intr_io;          /* individual or combined interrupts   */
  BOOL      mabrst;           /* Max AMBA burst length               */
  BOOL      big_endian;       /* big or little endian 1=big          */
  u16  s_hdata_width;    /* AHB slave data bus width            */
  u16  m_hdata_width[4]; /* AHB master data bus widths          */
                              /*  can be - 32,64,128,256             */
  u16   ch_fifo_depth[8]; /* channels FIFO depths in bytes       */
                              /*  can be - 4,8,16,32,64,128,256            */
  u16  ch_max_mult_size[8]; /* channel max burst transaction    */
                                 /*  sizes can be - 4,8,16,32,64,    */
                                 /*  128,256                         */
  u16  ch_max_blk_size[8];  /* channel max block sizes          */
                                 /*  can be - 3,7,15,31,63,127,255,  */
                                 /*  511, 1023,2047,4095             */
  u8   ch_fc[8];        /* channels flow control hard-codded?   */
                             /*  0=DMA_only, 1=SRC_only, 2=DST_only  */
                             /*  3=ANY                               */
  BOOL      ch_lock_en[8];   /* bus locking for each channel         */
  u16  ch_stw[8];       /* hard-code src channel transfer width */
                             /*  0=NO-HARD_CODE, 8=BYTE, 16=HALFWORD */
                             /*  32=WORD, 64=TWO_WORD, 128=FOUR_WORD */ 
                             /*  256=EIGHT_WORD                      */
  u16  ch_dtw[8];       /* hard-code dst channel transfer width */
                             /*  uses same encoding as the src above */
  BOOL      ch_multi_blk_en[8];  /* Enable multi block transfers     */
  u16  ch_multi_blk_type[8];  /* Multi block transfers types    */
  BOOL      ch_ctl_wb_en[8]; /* Transfer write back enable           */
  BOOL      ch_hc_llp[8];    /* hard-code LLP register               */
                             /*  1=hard-coded, 0=not hard-coded      */
  BOOL      ch_dst_sca_en[8];/* enable scatter feature on dst        */
  BOOL      ch_src_gat_en[8];/* enable gather feature on src         */
  u8   ch_sms[8];       /* hard-code master interface on src    */
                             /*  0=master_1, 1=master_2, 2=master_3, */  
                             /*  3=master_4, 4=no_hard-code          */
  u8   ch_dms[8];       /* hard-code master interface on dst    */
                             /*  same encoding as for src above      */
  u8   ch_lms[8];       /* hard-code master interface on LLP    */
                             /*  same encoding as for src above      */
  BOOL      ch_stat_dst[8];  /* fetch status from dst peripheral     */
                             /*  1=enable feature, 0=disable feature */
  BOOL      ch_stat_src[8];  /* fetch status from src peripheral     */
                             /*  1=enable feature, 0=disable feature */
}FDmaPs_Param_T;

/*****************************************************************************
* DESCRIPTION
*  This structure is used to pass transfer information into
*  the driver, so that it can be accessed and manipulated by
*  the FDmaPs_irqHandler function.
*
*****************************************************************************/
typedef struct _dmac_instance {
    struct {
        enum FDmaPs_channelNumber ch_num; /* channel number*/
        enum FDmaPs_chState src_state;    /* dmac source state*/
        enum FDmaPs_chState dst_state;    /* dmac destination state*/
        int block_cnt;      /* count of completed blocks*/
        int total_blocks;   /* total number blocks in the transfer*/
        int src_byte_cnt;   /* count of source bytes completed*/
        int dst_byte_cnt;   /* count of destination bytes completed*/
        int src_single_inc; /* src byte increment in single region*/
        int src_burst_inc;  /* src byte increment in burst region*/
        int dst_single_inc; /* dst byte increment in single region*/
        int dst_burst_inc;  /* dst byte increment in burst region*/
        enum FDmaPs_transferType trans_type;  /* transfer type (row)*/
        enum FDmaPs_transferFlow tt_fc;   /* transfer device type flow*/
                                            /* control*/
        FMSH_callback userCallback; /* callback functon for IRQ handler*/
        FMSH_callback userListener; /* listener functon for IRQ handler*/
    } ch[8];
    int ch_order[8]; /* channel order based on priority*/
}FDmaPs_Instance_T;

/*****************************************************************************
* DESCRIPTION
*  This is the primary structure used when dealing with DMA controller.
*  It allows this code to support more than one device of the same type
*  simultaneously.  This structure needs to be initialized with
*  meaningful values before a pointer to it is passed to a driver
*  initialization function.
* PARAMETERS
*  instance        device private data structure pointer
*  os              unused pointer for associating with an OS structure
*  compParam       pointer to structure containing device's
*                  coreConsultant configuration parameters structure
*  compVersion     device version identification number
*  compType        device identification number
*
*****************************************************************************/
typedef struct _dmac_device {
    struct _dmac_config config;
    struct _dmac_instance *instance;
    struct _dmac_param *comp_param;
    uint32_t comp_version;
    enum FMSH_compType comp_type;
    FMSH_listHead list;
}FDmaPs_T;

/***************** Macros (Inline Functions) Definitions *********************/

/*****************************************************************************
* DESCRIPTION
*  This is the data type used for specifying DMA notifier functions.
*  These are needed when software handshaking is used with peripheral
*  DMA transfers in order to inform the DMA controller when data is
*  ready to be sent/received.
*
* @param    
*           pDmac is the pointer to the DMA controller device.
*           channel associated channel number
*           single single or burst transfer?
*           last is this the last block?
*
* @note     NA.
*
*****************************************************************************/
typedef void (*FMSH_dmaNotifierFunc)(FDmaPs_T *pDmac, unsigned
        channel, BOOL single, BOOL last);

/*****************************************************************************
* DESCRIPTION
*  This is the data structure used to store a DMA notifier function
*  and its related arguments.
*
*****************************************************************************/
typedef struct fmsh_dma_config {
    enum FMSH_dmaMode mode;
    FMSH_dmaNotifierFunc notifier;
    FDmaPs_T *dmac;
    unsigned channel;
}FDmaPs_config_T;


/************************** Function Prototypes ******************************/

void FDmaPs_initDev(FDmaPs_T *pDmac,
                        FDmaPs_Instance_T *pInstance,
                        FDmaPs_Param_T *pParam,
                        FDmaPs_Config *pDmaCfg);
void FDmaPs_resetController(void);
int FDmaPs_init(FDmaPs_T *pDmac);
void FDmaPs_enable(FDmaPs_T *pDmac);
int FDmaPs_disable(FDmaPs_T *pDmac);
BOOL FDmaPs_isEnabled(FDmaPs_T *pDmac);
int FDmaPs_enableChannel(FDmaPs_T *pDmac,
    enum FDmaPs_channelNumber ch_num);
int FDmaPs_disableChannel(FDmaPs_T *pDmac,
    enum FDmaPs_channelNumber ch_num);
BOOL FDmaPs_isChannelEnabled(FDmaPs_T *pDmac,
    enum FDmaPs_channelNumber ch_num);
uint8_t FDmaPs_getChannelEnableReg(FDmaPs_T *pDmac);
int FDmaPs_enableChannelIrq(FDmaPs_T *pDmac,
    enum FDmaPs_channelNumber ch_num);
int FDmaPs_disableChannelIrq(FDmaPs_T *pDmac,
    enum FDmaPs_channelNumber ch_num);
BOOL FDmaPs_isChannelIrqEnabled(FDmaPs_T *pDmac,
     enum FDmaPs_channelNumber ch_num);
enum FDmaPs_channelNumber FDmaPs_getFreeChannel(FDmaPs_T *pDmac);
int FDmaPs_suspendChannel(FDmaPs_T *pDmac,
    enum FDmaPs_channelNumber ch_num);
int FDmaPs_resumeChannel(FDmaPs_T *pDmac,
    enum FDmaPs_channelNumber ch_num);
BOOL FDmaPs_isChannelSuspended(FDmaPs_T *pDmac,
     enum FDmaPs_channelNumber ch_num);
int FDmaPs_clearIrq(FDmaPs_T *pDmac,
    enum FDmaPs_channelNumber ch_num,
    enum FDmaPs_irq ch_irq);
int FDmaPs_maskIrq(FDmaPs_T *pDmac,
    enum FDmaPs_channelNumber ch_num,
    enum FDmaPs_irq ch_irq);
int FDmaPs_unmaskIrq(FDmaPs_T *pDmac,
    enum FDmaPs_channelNumber ch_num,
    enum FDmaPs_irq ch_irq);
BOOL FDmaPs_isIrqMasked(FDmaPs_T *pDmac,
    enum FDmaPs_channelNumber ch_num,
    enum FDmaPs_irq ch_irq);
BOOL FDmaPs_isRawIrqActive(FDmaPs_T *pDmac,
    enum FDmaPs_channelNumber ch_num,
    enum FDmaPs_irq ch_irq);
BOOL FDmaPs_isIrqActive(FDmaPs_T *pDmac,
    enum FDmaPs_channelNumber ch_num,
    enum FDmaPs_irq ch_irq);
int FDmaPs_setChannelConfig(FDmaPs_T *pDmac,
    enum FDmaPs_channelNumber ch_num,
    FDmaPs_ChannelConfig_T *ch_config);
int FDmaPs_getChannelConfig(FDmaPs_T *pDmac,
    enum FDmaPs_channelNumber ch_num,
    FDmaPs_ChannelConfig_T *ch_config);
int FDmaPs_setTransferType(FDmaPs_T *pDmac,
    enum FDmaPs_channelNumber ch_num,
    enum FDmaPs_transferType trans_type);
enum FDmaPs_transferType FDmaPs_getTransferType(
    FDmaPs_T *pDmac, enum FDmaPs_channelNumber ch_num);
BOOL FDmaPs_isBlockTransDone(FDmaPs_T *pDmac,
    enum FDmaPs_channelNumber ch_num);
BOOL FDmaPs_isFifoEmpty(FDmaPs_T *pDmac,
    enum FDmaPs_channelNumber ch_num);
void FDmaPs_setTestMode(FDmaPs_T *pDmac,
    enum FMSH_state state);
enum FMSH_state FDmaPs_getTestMode(FDmaPs_T *pDmac);
int FDmaPs_setSoftwareRequest(FDmaPs_T *pDmac,
   enum FDmaPs_channelNumber ch_num,
   enum FDmaPs_srcDstSelect sd_sel,
   enum FDmaPs_softwareReq sw_req, enum FMSH_state state);
enum FMSH_state FDmaPs_getSoftwareRequest(FDmaPs_T *pDmac,
   enum FDmaPs_channelNumber ch_num,
   enum FDmaPs_srcDstSelect sd_sel,
   enum FDmaPs_softwareReq sw_req);
int FDmaPs_setAddress(FDmaPs_T *pDmac,
    enum FDmaPs_channelNumber ch_num,
    enum FDmaPs_srcDstSelect sd_sel,
    u32 address);
u32 FDmaPs_getAddress(FDmaPs_T *pDmac,
    enum FDmaPs_channelNumber ch_num,
    enum FDmaPs_srcDstSelect sd_sel);
int FDmaPs_setBlockTransSize(FDmaPs_T *pDmac,
    enum FDmaPs_channelNumber ch_num, uint16_t block_size);
uint16_t FDmaPs_getBlockTransSize(FDmaPs_T *pDmac,
    enum FDmaPs_channelNumber ch_num);
int FDmaPs_setMstSelect(FDmaPs_T *pDmac,
    enum FDmaPs_channelNumber ch_num,
    enum FDmaPs_srcDstSelect sd_sel,
    enum FDmaPs_masterNumber mst_num);
enum FDmaPs_masterNumber FDmaPs_getMstSelect(FDmaPs_T *pDmac,
    enum FDmaPs_channelNumber ch_num,
    enum FDmaPs_srcDstSelect sd_sel);
int FDmaPs_setMemPeriphFlowCtl(FDmaPs_T *pDmac,
    enum FDmaPs_channelNumber ch_num,
    enum FDmaPs_transferFlow tt_fc);
enum FDmaPs_transferFlow FDmaPs_getMemPeriphFlowCtl(
    FDmaPs_T *pDmac, enum FDmaPs_channelNumber ch_num);
int FDmaPs_setScatterEnable(FDmaPs_T *pDmac,
    enum FDmaPs_channelNumber ch_num, enum FMSH_state state);
enum FMSH_state FDmaPs_getScatterEnable(FDmaPs_T *pDmac,
    enum FDmaPs_channelNumber ch_num);
int FDmaPs_setGatherEnable(FDmaPs_T *pDmac,
   enum FDmaPs_channelNumber ch_num, enum FMSH_state state);
enum FMSH_state FDmaPs_getGatherEnable(FDmaPs_T *pDmac,
    enum FDmaPs_channelNumber ch_num);
int FDmaPs_setBurstTransLength(FDmaPs_T *pDmac,
    enum FDmaPs_channelNumber ch_num,
    enum FDmaPs_srcDstSelect sd_sel,
    enum FDmaPs_burstTransLength length);
enum FDmaPs_burstTransLength FDmaPs_getBurstTransLength(
    FDmaPs_T *pDmac, enum FDmaPs_channelNumber ch_num,
    enum FDmaPs_srcDstSelect sd_sel);
int FDmaPs_setAddressInc(FDmaPs_T *pDmac,
    enum FDmaPs_channelNumber ch_num,
    enum FDmaPs_srcDstSelect sd_sel,
    enum FDmaPs_addressIncrement addr_inc);
enum FDmaPs_addressIncrement FDmaPs_getAddressInc(
    FDmaPs_T *pDmac, enum FDmaPs_channelNumber ch_num,
    enum FDmaPs_srcDstSelect sd_sel);
int FDmaPs_setTransWidth(FDmaPs_T *pDmac,
    enum FDmaPs_channelNumber ch_num,
    enum FDmaPs_srcDstSelect sd_sel,
    enum FDmaPs_transferWidth width);
enum FDmaPs_transferWidth FDmaPs_getTransWidth(
    FDmaPs_T *pDmac, enum FDmaPs_channelNumber ch_num,
    enum FDmaPs_srcDstSelect sd_sel);
int FDmaPs_setHsInterface(FDmaPs_T *pDmac,
    enum FDmaPs_channelNumber ch_num,
    enum FDmaPs_srcDstSelect sd_sel,
    enum FDmaPs_hsInterface hs_inter);
enum FDmaPs_hsInterface FDmaPs_getHsInterface(
    FDmaPs_T *pDmac, enum FDmaPs_channelNumber ch_num,
    enum FDmaPs_srcDstSelect sd_sel);
int FDmaPs_setStatUpdate(FDmaPs_T *pDmac,
    enum FDmaPs_channelNumber ch_num,
    enum FDmaPs_srcDstSelect sd_sel, enum FMSH_state state);
enum FMSH_state FDmaPs_getStatUpdate(FDmaPs_T *pDmac,
    enum FDmaPs_channelNumber ch_num,
    enum FDmaPs_srcDstSelect sd_sel);
int FDmaPs_setProtCtl(FDmaPs_T *pDmac,
    enum FDmaPs_channelNumber ch_num,
    enum FDmaPs_protLevel prot_lvl);
enum FDmaPs_protLevel FDmaPs_getProtCtl(FDmaPs_T *pDmac,
    enum FDmaPs_channelNumber ch_num);
int FDmaPs_setFifoMode(FDmaPs_T *pDmac,
   enum FDmaPs_channelNumber ch_num,
   enum FDmaPs_fifoMode fifo_mode);
enum FDmaPs_fifoMode FDmaPs_getFifoMode(FDmaPs_T *pDmac,
    enum FDmaPs_channelNumber ch_num);
int FDmaPs_setFlowCtlMode(FDmaPs_T *pDmac,
    enum FDmaPs_channelNumber ch_num,
    enum FDmaPs_flowCtlMode fc_mode);
enum  FDmaPs_flowCtlMode FDmaPs_getFlowCtlMode(
    FDmaPs_T *pDmac, enum FDmaPs_channelNumber ch_num);
int FDmaPs_setMaxAmbaBurstLength(FDmaPs_T *pDmac,
    enum FDmaPs_channelNumber ch_num, uint16_t burst_length);
uint16_t FDmaPs_getMaxAmbaBurstLength(FDmaPs_T *pDmac,
    enum FDmaPs_channelNumber ch_num);
int FDmaPs_setHsPolarity(FDmaPs_T *pDmac,
    enum FDmaPs_channelNumber ch_num,
    enum FDmaPs_srcDstSelect sd_sel,
    enum FDmaPs_polarityLevel pol_level);
enum FDmaPs_polarityLevel FDmaPs_getHsPolarity(
    FDmaPs_T *pDmac, enum FDmaPs_channelNumber ch_num,
    enum FDmaPs_srcDstSelect sd_sel);
int FDmaPs_setLockLevel(FDmaPs_T *pDmac,
    enum FDmaPs_channelNumber ch_num,
    enum FDmaPs_lockBusCh bus_ch,
    enum FDmaPs_lockLevel lock_l);
enum  FDmaPs_lockLevel FDmaPs_getLockLevel(
    FDmaPs_T *pDmac, enum FDmaPs_channelNumber ch_num,
    enum FDmaPs_lockBusCh bus_ch);
int FDmaPs_setLockEnable(FDmaPs_T *pDmac,
    enum FDmaPs_channelNumber ch_num,
    enum FDmaPs_lockBusCh bus_ch, enum FMSH_state state);
enum FMSH_state FDmaPs_getLockEnable(FDmaPs_T *pDmac,
    enum FDmaPs_channelNumber ch_num,
    enum FDmaPs_lockBusCh bus_ch);
int FDmaPs_setHandshakingMode(FDmaPs_T *pDmac,
    enum FDmaPs_channelNumber ch_num,
    enum FDmaPs_srcDstSelect sd_sel,
    enum FDmaPs_swHwHsSelect hs_hwsw_sel);
enum FDmaPs_swHwHsSelect FDmaPs_getHandshakingMode(
    FDmaPs_T *pDmac, enum FDmaPs_channelNumber ch_num,
    enum FDmaPs_srcDstSelect sd_sel);
int FDmaPs_setChannelPriority(FDmaPs_T *pDmac,
    enum FDmaPs_channelNumber ch_num,
    enum FDmaPs_channelPriority ch_priority);
enum FDmaPs_channelPriority FDmaPs_getChannelPriority(
    FDmaPs_T *pDmac, enum FDmaPs_channelNumber ch_num);
int FDmaPs_setListMstSelect(FDmaPs_T *pDmac,
    enum FDmaPs_channelNumber ch_num,
    enum FDmaPs_masterNumber mst_num);
enum FDmaPs_masterNumber FDmaPs_getListMstSelect(
    FDmaPs_T *pDmac, enum FDmaPs_channelNumber ch_num);
int FDmaPs_setListPointerAddress(FDmaPs_T *pDmac,
    enum FDmaPs_channelNumber ch_num, u32 address);
u32 FDmaPs_getListPointerAddress(FDmaPs_T *pDmac,
    enum FDmaPs_channelNumber ch_num);
int FDmaPs_setLlpEnable(FDmaPs_T *pDmac,
    enum FDmaPs_channelNumber ch_num,
    enum FDmaPs_srcDstSelect sd_sel,
    enum FMSH_state state);
enum FMSH_state FDmaPs_getLlpEnable(FDmaPs_T *pDmac,
    enum FDmaPs_channelNumber ch_num,
    enum FDmaPs_srcDstSelect sd_sel);
int FDmaPs_setReload(FDmaPs_T *pDmac,
    enum FDmaPs_channelNumber ch_num,
    enum FDmaPs_srcDstSelect sd_sel,
    enum FMSH_state state);
enum FMSH_state FDmaPs_getReload(FDmaPs_T *pDmac,
    enum FDmaPs_channelNumber ch_num,
    enum FDmaPs_srcDstSelect sd_sel);
int FDmaPs_setStatus(FDmaPs_T *pDmac,
    enum FDmaPs_channelNumber ch_num,
    enum FDmaPs_srcDstSelect sd_sel, u32 value);
u32 FDmaPs_getStatus(FDmaPs_T *pDmac,
    enum FDmaPs_channelNumber ch_num,
    enum FDmaPs_srcDstSelect sd_sel);
int FDmaPs_setStatusAddress(FDmaPs_T *pDmac,
    enum FDmaPs_channelNumber ch_num,
    enum FDmaPs_srcDstSelect sd_sel, u32 address);
u32 FDmaPs_getStatusAddress(FDmaPs_T *pDmac,
    enum FDmaPs_channelNumber ch_num,
    enum FDmaPs_srcDstSelect sd_sel);
int FDmaPs_setGatherParam(FDmaPs_T *pDmac,
    enum FDmaPs_channelNumber ch_num,
    enum FDmaPs_scatterGatherParam cnt_int, u32 value);
u32 FDmaPs_getGatherParam(FDmaPs_T *pDmac,
    enum FDmaPs_channelNumber ch_num,
    enum FDmaPs_scatterGatherParam cnt_int);
int FDmaPs_setScatterParam(FDmaPs_T *pDmac,
    enum FDmaPs_channelNumber ch_num,
    enum FDmaPs_scatterGatherParam cnt_int, u32 value);
u32  FDmaPs_getScatterParam(FDmaPs_T *pDmac,
    enum FDmaPs_channelNumber ch_num,
    enum FDmaPs_scatterGatherParam cnt_int);
unsigned FDmaPs_getChannelIndex(enum FDmaPs_channelNumber ch_num);
uint8_t FDmaPs_getNumChannels(FDmaPs_T *pDmac);
int FDmaPs_getChannelFifoDepth(FDmaPs_T *pDmac,
       enum FDmaPs_channelNumber ch_num);
void FDmaPs_addLliItem(FMSH_listHead *lhead,
    FDmaPs_LliItem_T *lli_item,
    FDmaPs_ChannelConfig_T *config);
int FDmaPs_userIrqHandler(FDmaPs_T *pDmac);
int FDmaPs_irqHandler(FDmaPs_T *pDmac);
int FDmaPs_startTransfer(FDmaPs_T *pDmac,
    enum FDmaPs_channelNumber ch_num, int num_blocks,
    FMSH_callback cb_func);
void FDmaPs_sourceReady(FDmaPs_T *pDmac, unsigned ch_index,
        BOOL single, BOOL last);
void FDmaPs_destinationReady(FDmaPs_T *pDmac, unsigned ch_index,
        BOOL single, BOOL last);
void FDmaPs_setSingleRegion(FDmaPs_T *pDmac,
     enum FDmaPs_channelNumber ch_num,
     enum FDmaPs_srcDstSelect sd_sel);
void FDmaPs_nextBlockIsLast(FDmaPs_T *pDmac,
     enum FDmaPs_channelNumber ch_num);
void FDmaPs_setListener(FDmaPs_T *pDmac,
     enum FDmaPs_channelNumber ch_num, FMSH_callback userFunction);
int FDmaPs_getBlockCount(FDmaPs_T *pDmac,
    enum FDmaPs_channelNumber ch_num);
int FDmaPs_getBlockByteCount(FDmaPs_T *pDmac,
    enum FDmaPs_channelNumber ch_num,
    enum FDmaPs_srcDstSelect sd_sel);

/* Static loopup function implemented in fmsh_dmac_sinit.c */
FDmaPs_Config *FDmaPs_LookupConfig(u16 DeviceId);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* #ifndef _FMSH_DMAC_H_ */

