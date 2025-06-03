/* vxDevC.h - vxDevC driver header */

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
01a, 02Jan20, jc  written.
*/

#ifndef __INC_VX_DEVC_H__
#define __INC_VX_DEVC_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#define  VX_DEVC_CFG_BASE   (0xE0040000)

#define  DEVC_CTRL_0        (0)

/*
devc registers definitions
*/
#define DEVC_REG_SAC_CTRL           (0x08)    /* u32 sac_ctrl;     //0x08       */
#define DEVC_REG_SAC_CFG            (0x0C)    /* u32 sac_cfg;      //0x0c            */
#define DEVC_REG_SAC_INT_STS        (0x10)    /* u32 sac_int_sts;  //0x10         */
#define DEVC_REG_SAC_INT_MASK       (0x14)    /* u32 sac_int_mask; //0x14          */
#define DEVC_REG_SAC_STATUS         (0x18)    /* u32 sac_status;   //0x18           */

#define DEVC_REG_CSU_DMA_SRC_ADR    (0x1C)    /* u32 csu_dma_src_addr;   //0x1c          */
#define DEVC_REG_CSU_DMA_DST_ADR    (0x20)    /* u32 csu_dma_dest_addr;  //0x20       */
#define DEVC_REG_CSU_DMA_SRC_LEN    (0x24)    /* u32 csu_dma_src_len;    //0x24         */
#define DEVC_REG_CSU_DMA_DST_LEN    (0x28)    /* u32 csu_dma_dest_len;   //0x28   */

#define DEVC_REG_CSU_UNLOCK         (0x2C)    /* u32 csu_unlock;         //0x2c          */
#define DEVC_REG_CSU_MUTIBOOT       (0x38)    /* u32 csu_mutiboot;       //0x38 */

#define DEVC_REG_SAC_LOCK           (0x3C)    /* u32 sac_lock;     //0x3c      */
#define DEVC_REG_XADC_STATUS        (0x40)    /* u32 xadc_status;  //0x40*/

#define DEVC_REG_DEV_KEY0           (0x44)    /* u32 dev_key0;  //0x44       */
#define DEVC_REG_DEV_KEY1           (0x48)    /* u32 dev_key1;  //0x48           */
#define DEVC_REG_DEV_KEY2           (0x4C)    /* u32 dev_key2;  //0x4c          */
#define DEVC_REG_DEV_KEY3           (0x50)    /* u32 dev_key3;  //0x50          */
#define DEVC_REG_DEV_KEY4           (0x54)    /* u32 dev_key4;  //0x54       */
#define DEVC_REG_DEV_KEY5           (0x58)    /* u32 dev_key5;  //0x58           */
#define DEVC_REG_DEV_KEY6           (0x5C)    /* u32 dev_key6;  //0x5c           */
#define DEVC_REG_DEV_KEY7           (0x60)    /* u32 dev_key7;  //0x60    */

#define DEVC_REG_MULTH_H0           (0x64)    /* u32 multh_H0;  //0x64       */
#define DEVC_REG_MULTH_H1           (0x68)    /* u32 multh_H1;  //0x68           */
#define DEVC_REG_MULTH_H2           (0x6C)    /* u32 multh_H2;  //0x6c           */
#define DEVC_REG_MULTH_H3           (0x70)    /* u32 multh_H3;  //0x70   */

#define DEVC_REG_MULTH_TAG0         (0x74)    /* u32 multh_tag0;  //0x74       */
#define DEVC_REG_MULTH_TAG1         (0x78)    /* u32 multh_tag1;  //0x78           */
#define DEVC_REG_MULTH_TAG2         (0x7C)    /* u32 multh_tag2;  //0x7c           */
#define DEVC_REG_MULTH_TAG3         (0x80)    /* u32 multh_tag3;  //0x80    */

#define DEVC_REG_KEY_IV_LOAD        (0x84)    /* u32 key_iv_load;   //0x84            */
#define DEVC_REG_KEY_SRC            (0x88)    /* u32 key_src;       //0x88             */
#define DEVC_REG_KEY_IV_CLR         (0x8C)    /* u32 key_iv_clear;  //0x8c     */

#define DEVC_REG_DEC_FLAG           (0x90)    /* u32 dec_flag;  //0x90*/

#define DEVC_REG_PUF_CMD            (0x94)    /* u32 puf_cmd;  //0x94        */
#define DEVC_REG_PUF_STATUS         (0x98)    /* u32 puf_status;  //0x98   */

#define DEVC_REG_PUF_IV0            (0x9C)    /* u32 puf_iv0;  //0x9c   */
#define DEVC_REG_PUF_IV1            (0xA0)    /* u32 puf_iv1;  //0xa0 */
#define DEVC_REG_PUF_IV2            (0xA4)    /* u32 puf_iv2;  //0xa4*/
#define DEVC_REG_PUF_IV3            (0xA8)    /* u32 puf_iv3;  //0xa8 */

#define DEVC_REG_PUF_RED_KEY0       (0xAC)    /* u32 puf_red_key0;  //0xac */
#define DEVC_REG_PUF_RED_KEY1       (0xB0)    /* u32 puf_red_key1;  //0xb0 */
#define DEVC_REG_PUF_RED_KEY2       (0xB4)    /* u32 puf_red_key2;  //0xb4 */
#define DEVC_REG_PUF_RED_KEY3       (0xB8)    /* u32 puf_red_key3;  //0xb8 */
#define DEVC_REG_PUF_RED_KEY4       (0xBC)    /* u32 puf_red_key4;  //0xbc*/
#define DEVC_REG_PUF_RED_KEY5       (0xC0)    /* u32 puf_red_key5;  //0xc0 */
#define DEVC_REG_PUF_RED_KEY6       (0xC4)    /* u32 puf_red_key6;  //0xc4 */
#define DEVC_REG_PUF_RED_KEY7       (0xC8)    /* u32 puf_red_key7;  //0xc8    */

#define DEVC_REG_APU_EXC_STATUS     (0xCC)    /* u32 apu_exc_status; //0xcc    */
#define DEVC_REG_APU_EXC_ADR        (0xD0)    /* u32 apu_exc_addr;   //0xd0*/

#define DEVC_REG_CSU_RST_CTRL       (0xD4)    /* u32 csu_rst_crtl;  //0xd4  */
#define DEVC_REG_KEY_CRC            (0xD8)    /* u32 key_crc;       //0xd8*/

#define DEVC_REG_BITSTREAM_CNT       (0xDC)    /* u32 bitstream_cnt;  //0xdc*/
#define DEVC_REG_BITSTREAM_CHKSUM    (0xE0)    /* u32 bitstream_checksum;  //0xe0*/

#define DEVC_REG_CSU_ERR_CODE        (0xE4)    /* u32 csu_error_code;  //0xe4*/
#define DEVC_REG_OT_PS_RST_EN        (0xE8)    /* u32 ot_ps_rst_en;    //0xe8  */

#define DEVC_REG_GCM_CTRL            (0x100)    /* u32 gcm_ctrl;    //0x100*/
#define DEVC_REG_GCM_STATUS          (0x104)    /* u32 gcm_status;  //0x104*/

#define DEVC_REG_KUP0                (0x110)    /* u32 KUP0;  //0x110   */
#define DEVC_REG_KUP1                (0x114)    /* u32 KUP1;  //0x114   */
#define DEVC_REG_KUP2                (0x118)    /* u32 KUP2;  //0x118   */
#define DEVC_REG_KUP3                (0x11C)    /* u32 KUP3;  //0x11c   */
#define DEVC_REG_KUP4                (0x120)    /* u32 KUP4;  //0x120   */
#define DEVC_REG_KUP5                (0x124)    /* u32 KUP5;  //0x124   */
#define DEVC_REG_KUP6                (0x128)    /* u32 KUP6;  //0x128   */
#define DEVC_REG_KUP7                (0x12C)    /* u32 KUP7;  //0x12c  */

#define DEVC_REG_IVUP0               (0x130)    /* u32 IVUP0;  //0x130   */
#define DEVC_REG_IVUP1               (0x134)    /* u32 IVUP1;  //0x134   */
#define DEVC_REG_IVUP2               (0x138)    /* u32 IVUP2;  //0x138   */
#define DEVC_REG_IVUP3               (0x13C)    /* u32 IVUP3;  //0x13c  */



/*
mask bit num
*/
#define MASK_1BIT     (0x01)
#define MASK_2BIT     (0x03)
#define MASK_3BIT     (0x07)
#define MASK_4BIT     (0x0F)

/*
SAC CFG register bit definition
*/
/*****************************************************************
program_b				31	rw	0xc0006142	连接至PL_CFG模块的program_b
pcap_mode_en			30			cclk gating signal，当该bit为0时，cclk无效
sec_down_data_byte_swap	29:28			加密位流下载模式下，输出至CFG的data做swap类型控制信号：                                              
								00：无变换                                                       
								01：半字交换                                                   
								10：字节交换                                             
								11：位交换     
JTAG_CHAIN_DIS			27			该bit用于关断PS DAP和PL TAP之间的连接，为1时关断连接（PL JTAG可自回环），默认为0
保留						26			保持为0
txfifo_do_data_swap		25:24			txfifo出来到总线的数据变换控制信号，位宽32bit：                                                             
								00：无变换                                                       
								01：半字交换                                                       
								10：字节交换                                                       
								11：位交换        
CSU_DMA_SWITCH			23:20			"SAC DMA功能模式选择:
								0000: 位流下载
								0001: reseverd
								0010: 位流回读
								0011: loopback数据搬移                                 
								0100: 安全加解密
								0110: sha256运算                                            
								0111: MultH计算                                                   
								1000: 加密位流下载"
保留						19			保留
rxfifo_di_data_swap		18:17			从总线进入rxfifo的数据变换控制信号，位宽32bit：                                          
								00：无变换                                                       
								01：半字交换                                                       
								10：字节交换                                                  
								11：位交换          
smap32_swap_ctrl		16			1：smap32数据按照smap32置换规则进行转换（参考UG470）                                   
									0：不转换
READBACK_DUMMY_NUM		15:13			回读位流时cclk采样周期，默认为3，即csi拉低后第3个cclk上升沿开始采样回读数据
SPNIDEN					12			"Secure Non-Invasive Debug Enable
								0 - Disable
								1 - Enable                                                              
								只有bootrom可以访问，用户不可访问                 可被sac_lock_reg[4]锁定     "
SPIDEN					11			"Secure Invasive Debug Enable
								0 - Disable
								1 - Enable                                                            
								只有bootrom可以访问，用户不可访问                 可被sac_lock_reg[4]锁定     "
NIDEN					10			"Non-Invasive Debug Enable
								0 - Disable
								1 - Enable                                                            
								只有bootrom可以访问，用户不可访问                 可被sac_lock_reg[4]锁定     "
RFIFO_TH				9:8			csu dma读通路fifo threshold：          
								00：0x40                                                           
								01：0x80                                                          
								10：0xc0                                                        
								11：0x100    
WFIFO_TH				7:6			csu dma写通路fifo threshold：                          
								00：0x80                                                          
								01：0x60                                                         
								10：0x40                                                         
								11：0x10   
RCLK_EDGE				5			"pcap Read data active clock edge 
								1 - Rising edge
								0 - Failing edge"
WCLK_EDGE				4			"pcap Write data active clock edge 
								1 - Rising edge
								0 - Failing edge"
DBGEN					3			"Invasive Debug Enable
								0 - Disable
								1 - Enable                                                            
								只有bootrom可以访问，用户不可访问                 可被sac_lock_reg[4]锁定  "
保留						2			保留
pcap2cfg_csi_b_reg		1			连接至PL_CFG模块的csi_b
pcap2cfg_rdwr_b_reg		0			连接至PL_CFG模块的rdwr_b
*****************************************************************/

#define SAC_CFG_PROGRAM_B_MASK           (0x80000000)  /* 31*/

#define SAC_CFG_TXFIFO_DATA_SWAP_SHIFT   (24)  /* bfoDEVC_CFG_txfifo_di_data_swap -> 24*/
#define SAC_CFG_RXFIFO_DATA_SWAP_SHIFT   (17)  /* bfoDEVC_CFG_rxfifo_di_data_swap -> 17*/

#define SAC_CFG_CSU_DMA_SWITCH_SHIFT     (20)  /* CSU_DMA_SWITCH 23:20*/

#define SAC_CFG_SMAP32_SWAP_CTRL_MASK    (0x00010000)   /* smap32_swap_ctrl -> 16*/
#define SAC_CFG_SMAP32_SWAP_CTRL_SHIFT   (16) 

#define SAC_CFG_RDBACK_DUMMY_NUM_SHIFT   (13)          /* READBACK_DUMMY_NUM  15:13*/

#define SAC_CFG_RFIFO_TH_MASK            (0x00000300)  /* bfoDEVC_CFG_RFIFO_TH -> 8 */
#define SAC_CFG_RFIFO_TH_SHIFT           (8)           /* RFIFO_TH	9:8*/
#define SAC_CFG_WFIFO_TH_SHIFT           (6)           /* WFIFO_TH  7:6	*/

#define SAC_CFG_RCLK_EDGE_MASK           (0x00000020)   /* bfoDEVC_CFG_RCLK_EDGE -> 5*/
#define SAC_CFG_RCLK_EDGE_SHIFT          (5)  

#define SAC_CFG_WCLK_EDGE_SHIFT          (4)  /*  bfoDEVC_CFG_WCLK_EDGE -> 4*/

#define SAC_CFG_CSI_B_SHIFT              (1)  /*  bfoDEVC_CFG_CSI_B -> 1*/
#define SAC_CFG_RDWR_B_SHIFT             (0)  /*  */

/*
SAC STATUS register bit definition
*/
#define SAC_STATUS_DMA_BUSY_MASK         (0x80000000)   /* bfoDEVC_STATUS_dma_busy -> 31 dma工作忙状态信号，高有效*/

#define SAC_STATUS_PCFG_INIT_MASK        (0x00000010)   /* pcfg_init 4  来自PL_CFG模块的init_b信号*/

#define SAC_STATUS_PCFG_DONE_MASK        (0x00000200)   /* bfoDEVC_STATUS_pcfg_done -> 9*/
#define SAC_STATUS_PCFG_DONE_SHIFT       (9)

/*
SAC_INT_STS register bit definition
*/
/*mask define*/
#define SAC_INT_STS_ERR_FLAG_MASK	     (0x00148040)    

/*dma_done_int	13   csu dma done触发
  d_p_done_int	12   csu dma 和 pcap都完成后触发*/
#define SAC_INT_STS_DMA_DONE_MASK             (0x00002000)  /* bfoDEVC_INT_STATUS_dma_done_int -> 13*/
#define SAC_INT_STS_DMA_DONE_SHIFT            (13)

#define SAC_INT_STS_DMA_PCAP_DONE_MASK        (0x00001000) 
#define SAC_INT_STS_DMA_PCAP_DONE_SHIFT       (12)


#define READFIFO_THRSH_HEX_40     (0x00) /* readFifoThre_hex_0x40 = 0x0, //default*/
#define READFIFO_THRSH_HEX_80     (0x01) /* readFifoThre_hex_0x80 = 0x1,*/
#define READFIFO_THRSH_HEX_C0     (0x02) /* readFifoThre_hex_0xc0 = 0x2,*/
#define READFIFO_THRSH_HEX_100    (0x03) /* readFifoThre_hex_0x100 = 0x3*/

#define WRITEFIFO_THRSH_HEX_80    (0x00) /* writeFifoThre_hex_0x80 = 0x0, //default*/
#define WRITEFIFO_THRSH_HEX_60    (0x01) /* writeFifoThre_hex_0x60 = 0x1,*/
#define WRITEFIFO_THRSH_HEX_40    (0x02) /* writeFifoThre_hex_0x40 = 0x2,*/
#define WRITEFIFO_THRSH_HEX_10    (0x03) /* writeFifoThre_hex_0x10 = 0x3*/

#define SWAP32_SWAP_DIS  (0x00)  /* smap32_swap_disable = 0x0,*/
#define SWAP32_SWAP_EN   (0x01)  /* smap32_swap_enable = 0x1   */

#define RISING_EDGE   (1) /* rising_edge = 0x1,*/
#define FALLING_EDGE  (0) /* falling_edge = 0x0	  */

#define HIGH_LVL   (1) /* voltage high level = 0x1,*/
#define LOW_LVL    (0) /* voltage low level = 0x0	  */


#define DATA_SWAP_NONE       (0x00)  /* none_swap = 0x0,*/
#define DATA_SWAP_HALF_WORD  (0x01)  /* half_word_swap = 0x1,	*/
#define DATA_SWAP_BYTE       (0x02)  /* byte_swap = 0x2,*/
#define DATA_SWAP_BIT        (0x03)  /* bit_swap = 0x3	*/


#define DUMMY_NUM_0   (0x00) /* dummy_0 = 0x0,*/
#define DUMMY_NUM_1   (0x01) /* dummy_1 = 0x1,	*/
#define DUMMY_NUM_2   (0x02) /* dummy_2 = 0x2,*/
#define DUMMY_NUM_3   (0x03) /* dummy_3 = 0x3,	 */
#define DUMMY_NUM_4   (0x04) /* dummy_4 = 0x4,*/
#define DUMMY_NUM_5   (0x05) /* dummy_5 = 0x5,	 */
#define DUMMY_NUM_6   (0x06) /* dummy_6 = 0x6,*/
#define DUMMY_NUM_7   (0x07) /* dummy_7 = 0x7,	*/

#define DMA_MODE_DWNLD_BITSTREAM         (0x00)   /* DOWNLOAD_BITSTREAM = 0x0,          */
#define DMA_MODE_RDBACK_BITSTREAM        (0x02)   /* READBACK_BITSTREAM = 0x2,          */
#define DMA_MODE_DATA_LOOPBACK           (0x03)   /*  DATA_LOOPBACK = 0x3,               */
#define DMA_MODE_SECUR_DWNLD_BITSTREAM   (0x08)   /* SECURE_DOWNLOAD_BITSTREAM = 0x08    */


/* Types of PCAP transfers */
#define FMSH_NON_SECURE_PCAP_WRITE		    (0)
#define FMSH_SECURE_PCAP_WRITE			    (1)
#define FMSH_PCAP_READBACK			        (2)
#define FMSH_PCAP_LOOPBACK			        (3)
#define FMSH_NON_SECURE_PCAP_WRITE_DUMMMY	(4)


#define DEVC_MAX_COUNT (0x0FFFFFFF)  /* 0x0FFF_FFFF*/
   
#define PCAP_WR_DATA_ADDR  (0xFFFFFFFF)  /* bitstream downloiad destination address */  
#define PCAP_RD_DATA_ADDR  (0xFFFFFFFF)  /* bitstream readback source address */
#define FMSH_DMA_INVALID_ADDRESS         PCAP_RD_DATA_ADDR


/*
devc ctrl
*/
typedef struct _t_devc_ctrl_
{
	int  ctrl_x;  /* 0, 1 */
	
	UINT32 cfgBaseAddr;		    /**< APB Base address of the device */

	
	UINT32 devIP_ver;
    UINT32 devIP_type;      /* identify peripheral types:  */

	int status;
} vxT_DEVC_CTRL;


typedef struct _t_devc_
{
	int devc_x;
	
	vxT_DEVC_CTRL * pDevcCtrl;
	
} vxT_DEVC;
	


void devcSlcr_En_CfgLvlShift(vxT_DEVC* pDevC);
void devcSlcr_Dis_CfgLvlShift(vxT_DEVC* pDevC);
void devcSlcr_En_UsrLvlShift(vxT_DEVC* pDevC);
void devcSlcr_Dis_UseLvlShift(vxT_DEVC* pDevC);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __INC_VX_DEVC_H__ */

