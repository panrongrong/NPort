/******************************************************************************
*
* Copyright (C) 2018 - 2028 FMSH, Inc.  All rights reserved.
*
******************************************************************************/
/******************************************************************************
*
* @file  fmsh_devc.c
*
* This file contains all private & pbulic function for devc(device configure).
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

/***************************** Include Files *********************************/
#include <stdlib.h>

#include "../../common/ps_init.h"
#include "../../common/fmsh_ps_parameters.h"

#include "fmsh_devc_lib.h"

/************************** Constant Definitions *****************************/
void test_load_pl2(u32 loadAddr, u32 size);

/**************************** Type Definitions *******************************/

/***************** Macros (Inline Functions) Definitions *********************/

/*
the following macro performs a 32-bit reads
*/
#define DEVC_IN32P(p)   (UINT32)(*((UINT32 *)(&p)))


/*the following macro performs a 32-bit write*/
#define DEVC_OUT32P(v,p)  *((UINT32 *)(&p)) = (v)     



/************************** Function Prototypes ******************************/

/************************** Variable Definitions *****************************/

/******************************************************************************
*
* This function unlocks CSU module
*
* @param	dev is devc handle.
*
* @return
*		- 0 if successful
*		- 1 if unsuccessful
*
* @note		None
*
******************************************************************************/
u8 FDevcPs_unLockCSU(FDevcPs_T *dev)
{ 
    FMSH_devc_portmap_TypeDef *portmap;
    portmap = (FMSH_devc_portmap_TypeDef *)dev->base_address; 
    
    DEVC_OUTP(0x757bdf0d, portmap->csu_unlock); 
    
    return 0;
}

/******************************************************************************
*
* This function locks CSU module
*
* @param	dev is devc handle.
*
* @return
*		- 0 if successful
*		- 1 if unsuccessful
*
* @note		None
*
******************************************************************************/
u8 FDevcPs_lockCSU(FDevcPs_T *dev)
{
    FMSH_devc_portmap_TypeDef *portmap;
    portmap = (FMSH_devc_portmap_TypeDef *) dev->base_address; 
    
    DEVC_OUTP(0xdf0d767b, portmap->sac_lock); 
    
    return 0;
}

/******************************************************************************
*
* This function unlocks DEVKEY
*
* @param	dev is devc handle.
*
* @return
*		- 0 if successful
*		- 1 if unsuccessful
*
* @note		None
*
******************************************************************************/
u8 FDevcPs_unLockDEVKEY()
{
    FMSH_WriteReg(0xE0040000, EFUSE_AES_KEY_RD_DIS_OFFSET, 0xdeaeb0fd); 
    return 0;
}

/******************************************************************************
*
* This function initializes devc device 
*
* @param	dev is devc handle.
* @param	addr is the base address of CSU.
*
* @return
*		- 0 if successful
*		- 1 not support
*
* @note		None
*
******************************************************************************/
u8 FDevcPs_init(FDevcPs_T *dev, FDevcPs_Config *cfg)
{
    dev->base_address = (void*)(cfg->BaseAddress);
    dev->id =cfg->DeviceId;
   
    return 0;
}

/******************************************************************************
*
* This function initializes Device Configuration module, configure pcap 
* interface work parameters
*
* @param	TransferType.
*		- FMSH_PCAP_READBACK
*		- FMSH_NON_SECURE_PCAP_WRITE
*		- FMSH_PCAP_LOOPBACK
*
* @return
*		- 0 if successful
*		- 1 if unsuccessful
*
* @note		None
*
******************************************************************************/
u8 FDevcPs_fabricInit(FDevcPs_T *dev, u32 TransferType)
{        
    FDevcPs_unLockCSU(dev);
 
    FDevcPs_smap32Swap(dev, smap32_swap_enable);
    FDevcPs_rclk_edge(dev, falling_edge);/*default failing_edge , rising_edge will failed*/
    FDevcPs_wclk_edge(dev, falling_edge);/*default failing_edge*/
    
    FDevcPs_CSI_B_HIGH(dev);
    FDevcPs_RDWR_B_HIGH(dev);

    if(TransferType == FMSH_PCAP_READBACK) 
    {
        FDevcPs_rxDataSwap(dev, byte_swap);
        FDevcPs_txDataSwap(dev, byte_swap); 

        /*default 3 , for xilinx 325t the value must be set 4,*/
        /*for fmsh 325t the value must be set 3 for readback reg, but the value must be set 4 for readback bitstream*/
        FDevcPs_readbackDummyCount(dev, dummy_4);
        
        FDevcPs_downloadMode(dev, DOWNLOAD_BITSTREAM);
        FDevcPs_readFifoThre(dev, readFifoThre_hex_0x40);
    } 
    else if(TransferType ==FMSH_NON_SECURE_PCAP_WRITE)
    {
        FDevcPs_Prog_B(dev);
        FDevcPs_rxDataSwap(dev, byte_swap);        
        FDevcPs_downloadMode(dev, DOWNLOAD_BITSTREAM);
        FDevcPs_writeFifoThre(dev, writeFifoThre_hex_0x80);
    } 
    else if(TransferType == FMSH_PCAP_LOOPBACK) 
    {
        FDevcPs_rxDataSwap(dev, byte_swap);
        FDevcPs_txDataSwap(dev, byte_swap); 
        FDevcPs_readbackDummyCount(dev, dummy_4);/*default 3*/
        FDevcPs_downloadMode(dev, DATA_LOOPBACK);      
    }
    else
        return 1;

    return 0;
}

/******************************************************************************
*
* This function pull down Prog_B, then pull up
*
* @param	dev is devc handle.
*
* @return
*		- 0 if successful
*		- 1 if unsuccessful
*
* @note		None
*
******************************************************************************/
u8 FDevcPs_Prog_B(FDevcPs_T *dev)
{
    u32 reg;

    FMSH_devc_portmap_TypeDef *portmap;
    
    portmap = (FMSH_devc_portmap_TypeDef *) dev->base_address; 
    
    /*Setting PCFG_PROG_B signal to high*/
    
	/*printf("PCFG_PROG_B to high \n");*/
	/*taskDelay(10);*/
	
    reg = DEVC_INP(portmap->sac_cfg); 
   /* if(FMSH_BIT_GET(reg, DEVC_CFG_program_b) != 1) */
    {
        FMSH_BIT_SET(reg, DEVC_CFG_program_b, 1U);  /* sac_cfg:0x0C => bfoDEVC_CFG_program_b -> 31*/
        DEVC_OUTP(reg, portmap->sac_cfg); 
    }	
    
	printf("PCFG_PROG_B to low \n");
	/*taskDelay(1);*/
	
    /*Setting PCFG_PROG_B signal to low*/
    reg = DEVC_INP(portmap->sac_cfg); 
   /* if (FMSH_BIT_GET(reg, DEVC_CFG_program_b) != 0)*/
    {
        FMSH_BIT_SET(reg, DEVC_CFG_program_b, 0);  /* sac_cfg:0x0C => bfoDEVC_CFG_program_b -> 31*/
        DEVC_OUTP(reg, portmap->sac_cfg); 
    }	
    
	/*printf("PCAP_INIT sts for Reset \n");*/
	/*taskDelay(1);*/
	
    /*Polling the PCAP_INIT sts for Reset  */
    reg = DEVC_INP(portmap->sac_status); 
    while (FMSH_BIT_GET(reg, DEVC_STATUS_pcfg_init) != 0)  /*  sac_status:0x18 => bfoDEVC_STATUS_pcfg_init -> 4*/
    {
        reg = DEVC_INP(portmap->sac_status); 
		printf("low->sac_status: 0x%X \n", portmap->sac_status);
    }
	
	/*printf("PCFG_PROG_B to high 2 \n");*/
	/*taskDelay(1);*/
	
    /*Setting PCFG_PROG_B signal to high*/
    reg = DEVC_INP(portmap->sac_cfg); 
   /* if (FMSH_BIT_GET(reg, DEVC_CFG_program_b) != 1) */
    {
        FMSH_BIT_SET(reg, DEVC_CFG_program_b, 1U);   /*  sac_cfg:0x0C => bfoDEVC_CFG_program_b -> 31*/
        DEVC_OUTP(reg, portmap->sac_cfg); 
    }
    
	/*printf("PCAP_INIT sts for set \n");*/
	/*taskDelay(1);*/
	
    /*Polling the PCAP_INIT sts for set  */
    reg = DEVC_INP(portmap->sac_status); 
    while (FMSH_BIT_GET(reg, DEVC_STATUS_pcfg_init) != 1)  /*  sac_status:0x18 => bfoDEVC_STATUS_pcfg_init -> 4*/
    {
        reg = DEVC_INP(portmap->sac_status); 		
		printf("high->sac_status: 0x%X \n", portmap->sac_status);
    }

    return 0;
}

/******************************************************************************
*
* This function pull high CSI_B
*
* @param	dev is devc handle.
*
* @return
*		- 0 if successful
*		- 1 if unsuccessful
*
* @note		None
*
******************************************************************************/
u8 FDevcPs_CSI_B_HIGH(FDevcPs_T *dev)
{
    u32 reg;
    FMSH_devc_portmap_TypeDef *portmap;
    
    portmap = (FMSH_devc_portmap_TypeDef *) dev->base_address; 
    
    /*Setting PCFG_CSI_B signal to HIGH*/
    reg = DEVC_INP(portmap->sac_cfg); 
    /*if(FMSH_BIT_GET(reg, DEVC_CFG_CSI_B) != 1) */
    {
        FMSH_BIT_SET(reg, DEVC_CFG_CSI_B, 1);  /* bfoDEVC_CFG_CSI_B -> 1*/
        DEVC_OUTP(reg, portmap->sac_cfg); 
    }
    
    return 0;
}

/******************************************************************************
*
* This function pull high RDWR_B
*
* @param	dev is devc handle.
*
* @return
*		- 0 if successful
*		- 1 if unsuccessful
*
* @note		None
*
******************************************************************************/
u8 FDevcPs_RDWR_B_HIGH(FDevcPs_T *dev)
{  
    u32 reg;
    FMSH_devc_portmap_TypeDef *portmap;
    
    portmap = (FMSH_devc_portmap_TypeDef *) dev->base_address;         
    /*Setting PCFG_RDWR_B signal to HIGH*/
    reg = DEVC_INP(portmap->sac_cfg); 
    /*if(FMSH_BIT_GET(reg, DEVC_CFG_RDWR_B ) != 1) */
    {
        FMSH_BIT_SET(reg, DEVC_CFG_RDWR_B, 1); /* bfoDEVC_CFG_RDWR_B -> 0*/
        DEVC_OUTP(reg, portmap->sac_cfg); 
    }
    
    return 0;
}

/******************************************************************************
*
* This function pull low CSI_B
*
* @param	dev is devc handle.
*
* @return
*		- 0 if successful
*		- 1 if unsuccessful
*
* @note		None
*
******************************************************************************/
u8 FDevcPs_CSI_B_LOW(FDevcPs_T *dev)
{
    u32 reg;
    FMSH_devc_portmap_TypeDef *portmap;
    
    portmap = (FMSH_devc_portmap_TypeDef *) dev->base_address;         
    /*Setting PCFG_CSI_B signal to LOW*/
    reg = DEVC_INP(portmap->sac_cfg); 
    /*if(FMSH_BIT_GET(reg, DEVC_CFG_CSI_B) != 0)*/
    {
        FMSH_BIT_SET(reg, DEVC_CFG_CSI_B, 0); /* bfoDEVC_CFG_CSI_B -> 1*/
        DEVC_OUTP(reg, portmap->sac_cfg); 
    }
    
    return 0;
}

/******************************************************************************
*
* This function pull low RDWR_B
*
* @param	dev is devc handle.
*
* @return
*		- 0 if successful
*		- 1 if unsuccessful
*
* @note		None
*
******************************************************************************/
u8 FDevcPs_RDWR_B_LOW(FDevcPs_T *dev)
{  
    u32 reg;
    FMSH_devc_portmap_TypeDef *portmap;
    
    portmap = (FMSH_devc_portmap_TypeDef *) dev->base_address; 
    /*Setting PCFG_RDWR_B signal to LOW*/
    reg = DEVC_INP(portmap->sac_cfg); 
    /*if(FMSH_BIT_GET(reg, DEVC_CFG_RDWR_B ) != 0) */
    {
        FMSH_BIT_SET(reg, DEVC_CFG_RDWR_B, 0);  /* bfoDEVC_CFG_RDWR_B -> 0*/
        DEVC_OUTP(reg, portmap->sac_cfg); 
    }
    
    return 0;
}

/******************************************************************************
*
* This function configures KUPKEY
* this key value maybe changed by different bitstream
*
* @param	dev is devc handle.
* @param	p is pointer to KEY buffer.
* @param	len is length of KEY, this should be fix as 8.
*
* @return
*		- 0 if successful
*		- 1 if unsuccessful
*
* @note		None
*
******************************************************************************/
u8 FDevcPs_KUPKEY(FDevcPs_T *dev, u32 *p, u32 len)
{       
    u32 *des;
    FMSH_devc_portmap_TypeDef *portmap;
    
    FMSH_ASSERT(len == 8);
    
    portmap = (FMSH_devc_portmap_TypeDef *)dev->base_address; 
    
    des = malloc(50);
    devc_byte_swap_todes(p, len, des);
    
    DEVC_OUTP(*(des + 0), portmap->KUP7); 
    DEVC_OUTP(*(des + 1), portmap->KUP6);         
    DEVC_OUTP(*(des + 2), portmap->KUP5);         
    DEVC_OUTP(*(des + 3), portmap->KUP4);         
    DEVC_OUTP(*(des + 4), portmap->KUP3);         
    DEVC_OUTP(*(des + 5), portmap->KUP2);         
    DEVC_OUTP(*(des + 6), portmap->KUP1);         
    DEVC_OUTP(*(des + 7), portmap->KUP0);        

    free(des);
    
    return 0;
}

/******************************************************************************
*
* This function configures DEVKEY
* this key value maybe changed by different bitstream
*
* @param	dev is devc handle.
* @param	p is pointer to KEY buffer.
* @param	len is length of KEY, this should be fix as 8.
*
* @return
*		- 0 if successful
*		- 1 if unsuccessful
*
* @note		None
*
******************************************************************************/
u8 FDevcPs_DEVKEY(FDevcPs_T *dev, u32 *p, u32 len)
{ 
    u32 *des;
    FMSH_devc_portmap_TypeDef *portmap;
    
    FMSH_ASSERT(len == 8);
    
    portmap = (FMSH_devc_portmap_TypeDef *)dev->base_address; 
    
    des = malloc(50);
    devc_byte_swap_todes(p, len, des);

    DEVC_OUTP(*(des + 0), portmap->dev_key7); 
    DEVC_OUTP(*(des + 1), portmap->dev_key6);        
    DEVC_OUTP(*(des + 2), portmap->dev_key5);         
    DEVC_OUTP(*(des + 3), portmap->dev_key4);       
    DEVC_OUTP(*(des + 4), portmap->dev_key3);       
    DEVC_OUTP(*(des + 5), portmap->dev_key2);        
    DEVC_OUTP(*(des + 6), portmap->dev_key1); 
    DEVC_OUTP(*(des + 7), portmap->dev_key0); 

    free(des);
    
    return 0;

}

/******************************************************************************
*
* This function configures fixed IV
* this IV value maybe changed by different bitstream
*
* @param	dev is devc handle.
* @param	p is pointer to IV buffer.
* @param	len is length of IV, this should be fix as 4.
*
* @return
*		- 0 if successful
*		- 1 if unsuccessful
*
* @note		None
*
******************************************************************************/
u8 FDevcPs_IV(FDevcPs_T *dev, u32 *p, u32 len)
{
    u32 *des;
    FMSH_devc_portmap_TypeDef *portmap;
            
    FMSH_ASSERT(len == 3);
    
    portmap = (FMSH_devc_portmap_TypeDef *)dev->base_address; 
    
    des = malloc(50);
    devc_byte_swap_todes(p, len, des);
    
    /*set iv_reg             */
    DEVC_OUTP(0x2, portmap->IVUP0); 
    DEVC_OUTP(*(des), portmap->IVUP3); 
    DEVC_OUTP(*(des + 1), portmap->IVUP2); 
    DEVC_OUTP(*(des + 2), portmap->IVUP1); 

    free(des);
    
    return 0;
}

/******************************************************************************
*
* This function sets secure download data byteswap
*
* @param	dev is devc handle.
* @param	mode
*              none_swap = 0x0,
*              half_word_swap = 0x1,   
*              byte_swap = 0x2,
*              bit_swap = 0x3  
*
* @return
*		- 0 if successful
*		- 1 if unsuccessful
*
* @note		None
*
******************************************************************************/
u8 FDevcPs_secDownDataByteSwap(FDevcPs_T *dev, enum data_swap mode)
{
    u32 reg; 
    
    FMSH_devc_portmap_TypeDef *portmap;
    
    portmap = (FMSH_devc_portmap_TypeDef *)dev->base_address; 
    
    reg = DEVC_INP(portmap->sac_cfg); 
    if(FMSH_BIT_GET(reg, DEVC_CFG_sec_down_data_byte_swap) != mode) 
    {
        FMSH_BIT_SET(reg, DEVC_CFG_sec_down_data_byte_swap, mode);
        DEVC_OUTP(reg, portmap->sac_cfg); 
    }
               
    return 0;
}

/******************************************************************************
*
* This function sets tx data byte swap.
*
* @param	dev is devc handle.
* @param	mode
*       none_swap = 0x0,
*       half_word_swap = 0x1,   
*       byte_swap = 0x2,
*       bit_swap = 0x3  
*
* @return
*		- 0 if successful
*		- 1 if unsuccessful
*
* @note		None
*
******************************************************************************/
u8 FDevcPs_txDataSwap(FDevcPs_T *dev, enum data_swap mode)
{  
    u32 reg; 
    
    FMSH_devc_portmap_TypeDef *portmap;
    
    portmap = (FMSH_devc_portmap_TypeDef *)dev->base_address; 
    
    reg = DEVC_INP(portmap->sac_cfg); 
    if(FMSH_BIT_GET(reg, DEVC_CFG_txfifo_do_data_swap) != mode) 
    {
        FMSH_BIT_SET(reg, DEVC_CFG_txfifo_do_data_swap, mode); /* bfoDEVC_CFG_txfifo_do_data_swap -> 24*/
        DEVC_OUTP(reg, portmap->sac_cfg); 
    }
    
    return 0;
}

/******************************************************************************
*
* This function sets bitstream download mode.
*
* @param	dev is devc handle.
* @param	mode
*       DOWNLOAD_BITSTREAM = 0x0,
*       READBACK_BITSTREAM = 0x2, 
*       DATA_LOOPBACK = 0x3,     
*       SECURE_DOWNLOAD_BITSTREAM = 0x08  
*
* @return
*		- 0 if successful
*		- 1 if unsuccessful
*
* @note		None
*
******************************************************************************/
u8 FDevcPs_downloadMode(FDevcPs_T *dev, enum download_mode mode)
{  
    u32 reg; 
    
    FMSH_devc_portmap_TypeDef *portmap;
    
    portmap = (FMSH_devc_portmap_TypeDef *)dev->base_address; 
    
    reg = DEVC_INP(portmap->sac_cfg); 
    if(FMSH_BIT_GET(reg, DEVC_CFG_CSU_DMA_SWITCH) != mode) 
    {
        FMSH_BIT_SET(reg, DEVC_CFG_CSU_DMA_SWITCH, mode);  /* bfoDEVC_CFG_CSU_DMA_SWITCH -> 20*/
        DEVC_OUTP(reg, portmap->sac_cfg); 
    }
    
    return 0;
}
/******************************************************************************
*
* This function sets rx data byte swap.
*
* @param	dev is devc handle.
* @param	mode
*       none_swap = 0x0,
*       half_word_swap = 0x1,   
*       byte_swap = 0x2,
*       bit_swap = 0x3  
*
* @return
*		- 0 if successful
*		- 1 if unsuccessful
*
* @note		None
*
******************************************************************************/
u8 FDevcPs_rxDataSwap(FDevcPs_T *dev, enum data_swap mode)
{  
    u32 reg; 
    
    FMSH_devc_portmap_TypeDef *portmap;
    
    portmap = (FMSH_devc_portmap_TypeDef *)dev->base_address; 
    
    reg = DEVC_INP(portmap->sac_cfg); 
    if(FMSH_BIT_GET(reg, DEVC_CFG_rxfifo_di_data_swap) != mode)
    {
        FMSH_BIT_SET(reg, DEVC_CFG_rxfifo_di_data_swap, mode);  /* bfoDEVC_CFG_rxfifo_di_data_swap -> 17*/
        DEVC_OUTP(reg, portmap->sac_cfg); 
    }
    
    return 0;
}

/******************************************************************************
*
* This function sets swap32 data byte swap.
*
* @param	dev is devc handle.
* @param	mode
*        smap32_swap_disable = 0x0,
*        smap32_swap_enable = 0x1  
*
* @return
*		- 0 if successful
*		- 1 if unsuccessful
*
* @note		None
*
******************************************************************************/
u8 FDevcPs_smap32Swap(FDevcPs_T *dev, enum smap32_swap mode)
{  
    u32 reg; 
    
    FMSH_devc_portmap_TypeDef *portmap;
    
    portmap = (FMSH_devc_portmap_TypeDef *)dev->base_address; 
    
    reg = DEVC_INP(portmap->sac_cfg); 
    if(FMSH_BIT_GET(reg, DEVC_CFG_smap32_swap_ctrl) != mode) 
    {
        FMSH_BIT_SET(reg, DEVC_CFG_smap32_swap_ctrl, mode);  /*bfoDEVC_CFG_smap32_swap_ctrl -> 16*/
        DEVC_OUTP(reg, portmap->sac_cfg); 
    }
    
    return 0;
}

/******************************************************************************
*
* This function sets readback dummmy counter, that is where data is sampled 
*
* @param	dev is devc handle.
* @param	mode
*        smap32_swap_disable = 0x0,
*        smap32_swap_enable = 0x1  
*
* @return
*		- 0 if successful
*		- 1 if unsuccessful
*
* @note		None
*
******************************************************************************/
u8 FDevcPs_readbackDummyCount(FDevcPs_T *dev, enum dummy_num mode)
{  
    u32 reg; 
    
    FMSH_devc_portmap_TypeDef *portmap;
    
    portmap = (FMSH_devc_portmap_TypeDef *)dev->base_address; 
    
    reg = DEVC_INP(portmap->sac_cfg); 
    if(FMSH_BIT_GET(reg, DEVC_CFG_READBACK_DUMMY_NUM) != mode) 
    {
        FMSH_BIT_SET(reg, DEVC_CFG_READBACK_DUMMY_NUM, mode); /* bfoDEVC_CFG_READBACK_DUMMY_NUM -> 13*/
        DEVC_OUTP(reg, portmap->sac_cfg); 
    }
    
    return 0;
}

/******************************************************************************
*
* This function sets read FIFO threshold
*
* @param	dev is devc handle.
* @param	mode
*       readFifoThre_hex_0x40 = 0x0,//default
*       readFifoThre_hex_0x80 = 0x1,
*       readFifoThre_hex_0xc0 = 0x2,
*       readFifoThre_hex_0x100 = 0x3
*
* @return
*		- 0 if successful
*		- 1 if unsuccessful
*
* @note		None
*
******************************************************************************/
u8 FDevcPs_readFifoThre(FDevcPs_T *dev, enum readFifoThre mode)
{  
    u32 reg; 
    
    FMSH_devc_portmap_TypeDef *portmap;
    
    portmap = (FMSH_devc_portmap_TypeDef *)dev->base_address; 
    
    reg = DEVC_INP(portmap->sac_cfg); 
    if(FMSH_BIT_GET(reg, DEVC_CFG_RFIFO_TH) != mode) 
    {
        FMSH_BIT_SET(reg, DEVC_CFG_RFIFO_TH, mode);   /* bfoDEVC_CFG_RFIFO_TH -> 8*/
        DEVC_OUTP(reg, portmap->sac_cfg); 
    }
    
    return 0;
}

/******************************************************************************
*
* This function sets write FIFO threshold
*
* @param	dev is devc handle.
* @param	mode
*       writeFifoThre_hex_0x80 = 0x0,//default
*       writeFifoThre_hex_0x60 = 0x1,
*       writeFifoThre_hex_0x40 = 0x2,
*       writeFifoThre_hex_0x10 = 0x3
*
* @return
*		- 0 if successful
*		- 1 if unsuccessful
*
* @note		None
*
******************************************************************************/
u8 FDevcPs_writeFifoThre(FDevcPs_T *dev, enum writeFifoThre mode)
{  
    u32 reg; 
    
    FMSH_devc_portmap_TypeDef *portmap;
    
    portmap = (FMSH_devc_portmap_TypeDef *)dev->base_address; 
    
    reg = DEVC_INP(portmap->sac_cfg); 
    if (FMSH_BIT_GET(reg, DEVC_CFG_WFIFO_TH) != mode) 
    {
        FMSH_BIT_SET(reg, DEVC_CFG_WFIFO_TH, mode);    /* bfoDEVC_CFG_WFIFO_TH -> 6*/
        DEVC_OUTP(reg, portmap->sac_cfg); 
    }
    
    return 0;
}

/******************************************************************************
*
* This function sets read clock edge
*
* @param	dev is devc handle.
* @param	mode
*       rising_edge = 0x1,
*       failing_edge = 0x0   
*
* @return
*		- 0 if successful
*		- 1 if unsuccessful
*
* @note		None
*
******************************************************************************/
u8 FDevcPs_rclk_edge(FDevcPs_T *dev, enum clk_edge mode)
{  
    u32 reg; 
    
    FMSH_devc_portmap_TypeDef *portmap;
    
    portmap = (FMSH_devc_portmap_TypeDef *)dev->base_address; 
    
    reg = DEVC_INP(portmap->sac_cfg); 
    if(FMSH_BIT_GET(reg, DEVC_CFG_RCLK_EDGE) != mode) 
    {
        FMSH_BIT_SET(reg, DEVC_CFG_RCLK_EDGE, mode);   /* bfoDEVC_CFG_RCLK_EDGE -> 5*/
        DEVC_OUTP(reg, portmap->sac_cfg); 
    }
    
    return 0;
}

/******************************************************************************
*
* This function sets write clock edge
*
* @param	dev is devc handle.
* @param	mode
*       rising_edge = 0x1,
*       failing_edge = 0x0   
*
* @return
*		- 0 if successful
*		- 1 if unsuccessful
*
* @note		None
*
******************************************************************************/
u8 FDevcPs_wclk_edge(FDevcPs_T *dev, enum clk_edge mode)
{  
    u32 reg; 
    
    FMSH_devc_portmap_TypeDef *portmap;
    
    portmap = (FMSH_devc_portmap_TypeDef *)dev->base_address; 
    
    reg = DEVC_INP(portmap->sac_cfg); 
    if(FMSH_BIT_GET(reg, DEVC_CFG_WCLK_EDGE) != mode) 
    {
        FMSH_BIT_SET(reg, DEVC_CFG_WCLK_EDGE, mode);  /*  bfoDEVC_CFG_WCLK_EDGE -> 4*/
        DEVC_OUTP(reg, portmap->sac_cfg); 
    }
    
    return 0;
}

/******************************************************************************
*
* This function disable AES
*
* @param	dev is devc handle.
* @param	None 
*
* @return
*		- 0 if successful
*		- 1 if unsuccessful
*
* @note		None
*
******************************************************************************/
static u8 FDevcPs_AES_CR_AESENDisable(FDevcPs_T *dev)
{  
    u32 reg; 
    
    FMSH_devc_portmap_TypeDef *portmap;
    
    portmap = (FMSH_devc_portmap_TypeDef *)dev->base_address; 
    
    reg = DEVC_INP(portmap->gcm_ctrl); 
    if(FMSH_BIT_GET(reg, DEVC_GCM_CTRL_gcm_en) != 0) 
    {
        FMSH_BIT_SET(reg, DEVC_GCM_CTRL_gcm_en, 0);
        DEVC_OUTP(reg, portmap->gcm_ctrl); 
    }
    
    return 0;
}

/******************************************************************************
*
* This function enable AES
*
* @param	dev is devc handle.
* @param	None 
*
* @return
*		- 0 if successful
*		- 1 if unsuccessful
*
* @note		None
*
******************************************************************************/
static u8 FDevcPs_AES_CR_AESENEnable(FDevcPs_T *dev)
{  
    u32 reg; 
    
    FMSH_devc_portmap_TypeDef *portmap;
    
    portmap = (FMSH_devc_portmap_TypeDef *)dev->base_address; 
    
    reg = DEVC_INP(portmap->gcm_ctrl); 
    if(FMSH_BIT_GET(reg, DEVC_GCM_CTRL_gcm_en) != 1) 
    {
        FMSH_BIT_SET(reg, DEVC_GCM_CTRL_gcm_en, 1);
        DEVC_OUTP(reg, portmap->gcm_ctrl); 
    }
    
    return 0;
}

/******************************************************************************
*
* This function selects algorithm
*
* @param	dev is devc handle.
* @param	mode 
*               AES = 0x0,
*               SM4 = 0x1,
*               NONE = 0x2
*
* @return
*		- 0 if successful
*		- 1 if unsuccessful
*
* @note		None
*
******************************************************************************/
static u8 FDevcPs_AES_CR_ALG_SEL(FDevcPs_T *dev, enum ALG mode)
{  
    u32 reg; 
    
    FMSH_devc_portmap_TypeDef *portmap;
    
    portmap = (FMSH_devc_portmap_TypeDef *)dev->base_address; 
    
    reg = DEVC_INP(portmap->gcm_ctrl); 
    if(FMSH_BIT_GET(reg, DEVC_GCM_CTRL_alg_sel) != mode)
    {
        FMSH_BIT_SET(reg, DEVC_GCM_CTRL_alg_sel, mode);
        DEVC_OUTP(reg, portmap->gcm_ctrl); 
    }
    
    return 0;
}

/******************************************************************************
*
* This function selects encrypt or decrypt
*
* @param	dev is devc handle.
* @param	mode 
*               ENCODE = 0x0,
*               DCODE = 0x3 
*
* @return
*		- 0 if successful
*		- 1 if unsuccessful
*
* @note		None
*
******************************************************************************/
static u8 FDevcPs_AES_CR_MODE(FDevcPs_T *dev, enum MODE mode)
{  
    u32 reg; 
    
    FMSH_devc_portmap_TypeDef *portmap;
    
    portmap = (FMSH_devc_portmap_TypeDef *)dev->base_address; 
    
    reg = DEVC_INP(portmap->gcm_ctrl); 
    if(FMSH_BIT_GET(reg, DEVC_GCM_CTRL_mode) != mode) 
    {
        FMSH_BIT_SET(reg, DEVC_GCM_CTRL_mode, mode);
        DEVC_OUTP(reg, portmap->gcm_ctrl); 
    }
    
    return 0;
}

/******************************************************************************
*
* This function selects data stream handle mode
*
* @param	dev is devc handle.
* @param	mode 
*               ECB = 0x0,
*               CTR = 0x2, 
*               MULTH = 0x3
*
* @return
*		- 0 if successful
*		- 1 if unsuccessful
*
* @note		None
*
******************************************************************************/
static u8 FDevcPs_AES_CR_CHMOD(FDevcPs_T *dev, enum CHMOD mode)
{  
    u32 reg; 
    
    FMSH_devc_portmap_TypeDef *portmap;
    
    portmap = (FMSH_devc_portmap_TypeDef *)dev->base_address; 
    
    reg = DEVC_INP(portmap->gcm_ctrl); 
    if(FMSH_BIT_GET(reg, DEVC_GCM_CTRL_chmode) != mode) 
    {
        FMSH_BIT_SET(reg, DEVC_GCM_CTRL_chmode, mode);
        DEVC_OUTP(reg, portmap->gcm_ctrl); 
    }
    
    return 0;
}

/******************************************************************************
*
* This function selects KEY source
*
* @param	dev is devc handle.
* @param	mode 
*              DEV_KEY = 0x1,
*              KUP = 0x2,  
*              MULTH_H = 0x3  
*
* @return
*		- 0 if successful
*		- 1 if unsuccessful
*
* @note		None
*
******************************************************************************/
static u8 FDevcPs_AES_KEY_SOURCE(FDevcPs_T *dev, enum KEYSRC mode)
{  
    u32 reg; 
    
    FMSH_devc_portmap_TypeDef *portmap;
    
    portmap = (FMSH_devc_portmap_TypeDef *)dev->base_address; 
    
    reg = DEVC_INP(portmap->key_src); 
    if(FMSH_BIT_GET(reg, KEY_SRC_key_src ) != mode)
    {
        FMSH_BIT_SET(reg, KEY_SRC_key_src, mode);
        DEVC_OUTP(reg, portmap->key_src); 
    }
    
    return 0;
}

/******************************************************************************
*
* This function loads AES's KEY & IV
*
* @param	dev is devc handle.
* @param	None 
*
* @return
*		- 0 if successful
*		- 1 if unsuccessful
*
* @note		None
*
******************************************************************************/
static u8 FDevcPs_AES_KEY_IV_LOAD(FDevcPs_T *dev)
{  
    FMSH_devc_portmap_TypeDef *portmap;
    
    portmap = (FMSH_devc_portmap_TypeDef *)dev->base_address; 
    
    DEVC_OUTP(0x3, portmap->key_iv_load); 
    
    return 0;
}

/******************************************************************************
*
* This function sets DECFLAG
*
* @param	dev is devc handle.
* @param	mode 
*               use_opkey = 0xe,  
*               no_opkey = 0xa ,
*               ivup_kup_wr_en = 0x3 ,
*               clear = 0x0 
* @return              
*		- 0 if successful
*		- 1 if unsuccessful
*
* @note		None
*
******************************************************************************/
static u8 FDevcPs_AES_DEC_FLAG(FDevcPs_T *dev, enum DECFLAG mode)
{  
    u32 reg; 
    
    FMSH_devc_portmap_TypeDef *portmap;
    
    portmap = (FMSH_devc_portmap_TypeDef *)dev->base_address; 
    
    reg = DEVC_INP(portmap->dec_flag); 
    if(FMSH_BIT_GET(reg, DEC_FLAG_flag) != mode)
    {
        FMSH_BIT_SET(reg, DEC_FLAG_flag, mode);
        DEVC_OUTP(reg, portmap->dec_flag); 
    }
    
    return 0;
}

/******************************************************************************
*
* This function enables encrypt moudle (AES/SM4)
*
* @param	dev is devc handle.
* @param	None 
*
* @return              
*		- 0 if successful
*		- 1 if unsuccessful
*
* @note		None
*
******************************************************************************/
static u8 FDevcPs_CTRL_AESenable(FDevcPs_T *dev)
{  
    u32 reg; 
    
    FMSH_devc_portmap_TypeDef *portmap;
    
    portmap = (FMSH_devc_portmap_TypeDef *)dev->base_address; 
    
    reg = DEVC_INP(portmap->sac_ctrl); 
    if(FMSH_BIT_GET(reg, DEVC_CTRL_encrypted_en) != 0x7) 
    {
        FMSH_BIT_SET(reg, DEVC_CTRL_encrypted_en, 0x7);
        DEVC_OUTP(reg, portmap->sac_ctrl); 
    }
    
    return 0;
}

/******************************************************************************
*
* This function disables encrypt moudle (AES/SM4)
*
* @param	dev is devc handle.
*
* @return              
*		- 0 if successful
*		- 1 if unsuccessful
*
* @note		None
*
******************************************************************************/
static u8 FDevcPs_CTRL_AESdisable(FDevcPs_T *dev)
{  
    u32 reg; 
    
    FMSH_devc_portmap_TypeDef *portmap;
    
    portmap = (FMSH_devc_portmap_TypeDef *)dev->base_address; 
    
    reg = DEVC_INP(portmap->sac_ctrl); 
    if(FMSH_BIT_GET(reg, DEVC_CTRL_encrypted_en) != 0x0) 
    {
        FMSH_BIT_SET(reg, DEVC_CTRL_encrypted_en, 0x0);
        DEVC_OUTP(reg, portmap->sac_ctrl); 
    }
    
    return 0;
}

/******************************common data swap*****************************/

/******************************************************************************
*
* This function byte swap data
*
* @param	srcPtr is pointer of data buffer
*               len is length to be swap
*
* @return              
*		- 0 if successful
*		- 1 if unsuccessful
*
* @note		None
*
******************************************************************************/
static u8 devc_byte_swap(u32 *srcPtr, u32 len)
{
    u32 i;
    u32 tmp;

    u32 byte_higher, byte_high, byte_low, byte_lower;

    for(i = 0; i < len; i++)
    {
        tmp = *(srcPtr + i);
        byte_higher = tmp & 0x000000ff;
        byte_high = tmp & 0x0000ff00;  
        byte_low = tmp & 0x00ff0000;
        byte_lower = tmp & 0xff000000;          
        
        *(srcPtr + i) = (byte_higher << 24) | ((byte_high >> 8) << 16) | ((byte_low >> 16) << 8) | ((byte_lower >> 24) << 0);
    }
    return 0;
}

/******************************************************************************
*
* This function byte swap data, swap data save in destination address
*
* @param	srcPtr is pointer of data buffer
*               len is length to be swap
*               desPtr is pointer of data buffer saveing data after swap
*
* @return              
*		- 0 if successful
*		- 1 if unsuccessful
*
* @note		None
*
******************************************************************************/
static u8 devc_byte_swap_todes(u32 *srcPtr, u32 len, u32 *desPtr)
{
    u32 i;
    u32 tmp;

    u32 byte_higher, byte_high, byte_low, byte_lower;

    for(i = 0; i < len; i++)
    {
        tmp = *(srcPtr + i);
        byte_higher = tmp & 0x000000ff;
        byte_high = tmp & 0x0000ff00;  
        byte_low = tmp & 0x00ff0000;
        byte_lower = tmp & 0xff000000;          
        
        *(desPtr + i) = (byte_higher << 24) | ((byte_high >> 8) << 16) | ((byte_low >> 16) << 8) | ((byte_lower >> 24) << 0);
    }
    return 0;
}

/******************************bitstream configure*****************************/

/******************************************************************************
*
*
* This function clears the specified interrupts in the Interrupt Status
* Register.
*
* @param	dev is devc handle.
*
* @return              
*		- 0 if successful
*		- 1 if unsuccessful
*
* @note		None.
*
*****************************************************************************/
u8 FDevcPs_clearPcapStatus(FDevcPs_T *dev)
{  
    u32 reg; 
    
    FMSH_devc_portmap_TypeDef *portmap;
    
    portmap = (FMSH_devc_portmap_TypeDef *)dev->base_address; 
    
    /* Clear it all, so if Boot ROM comes back, it can proceed*/
    /* write 0xffffffff to INT_STS*/
    DEVC_OUT32P(0xffffffff, portmap->sac_int_sts);
    
    /*Get PCAP Interrupt Status Register*/
    reg = DEVC_IN32P(portmap->sac_int_sts);
    if (reg & FMSH_DEVC_ERROR_FLAGS_MASK)
        return 1;

    /*Read the PCAP sts register for DMA sts*/
    reg = DEVC_IN32P(portmap->sac_status);	
    /*If the queue is full(busy), return 1*/
    if(FMSH_BIT_GET(reg, DEVC_STATUS_dma_busy) == 1)   /* bfoDEVC_STATUS_dma_busy -> 31*/
        return 1;
    
    return 0;
}

/******************************************************************************
*
*
* This function initiates the DMA transfer.
*
* @param	dev is devc handle.
* @param	InstancePtr is a pointer to the XDcfg instance.
* @param	SourcePtr contains a pointer to the source memory where the data
*		is to be transferred from.
* @param	SrcWordLength is the number of words (32 bit) to be transferred
*		for the source transfer.
* @param	DestPtr contains a pointer to the destination memory
*		where the data is to be transferred to.
* @param	DestWordLength is the number of words (32 bit) to be transferred
*		for the Destination transfer.
*
* @return              
*		- 0 if successful
*		- 1 if unsuccessful
*
* @note		None.
*
******************************************************************************/
u8 FDevcPs_initiateDma(FDevcPs_T *dev, u32 SourcePtr, u32 DestPtr, u32 SrcWordLength, u32 DestWordLength)
{
    FMSH_devc_portmap_TypeDef *portmap;
    
    portmap = (FMSH_devc_portmap_TypeDef *)dev->base_address; 
        
#ifdef TEST_TIME
    u64 count0, count1;
    double time;
    char s[10];
    global_timer_enable();  
    count0 = *(u64*)(FPS_GTC_BASEADDR + 0x8);
#endif     
    DEVC_OUTP(SourcePtr, portmap->csu_dma_src_addr);
    DEVC_OUTP(DestPtr, portmap->csu_dma_dest_addr);
    DEVC_OUTP(SrcWordLength, portmap->csu_dma_src_len);
    DEVC_OUTP(DestWordLength, portmap->csu_dma_dest_len);   

    /* Poll for the DMA done*/
    /*Poll for the D_P done*/
    FDevcPs_pollDmaDone(dev, MAX_COUNT);
    FDevcPs_pollDPDone(dev, MAX_COUNT);

#ifdef TEST_TIME
    count1 = *(u64*)(FPS_GTC_BASEADDR + 0x8);
    time = (count1 - count0) / 25000.0;/*(count1 - count0) *1000 / 25000000;*/
    sprintf(s, "%f", time);
    uart_logout("dma carry time is /ms");
    uart_logout(s);
#endif   
        
    return 0;
}


/******************************************************************************/
/**
*
* This function initiates the DMA transfer.
*
* @param	devcDev is devc handle.
* @param	InstancePtr is a pointer to the XDcfg instance.
* @param	SourcePtr contains a pointer to the source memory where the data
*		is to be transferred from.
* @param	SrcWordLength is the number of words (32 bit) to be transferred
*		for the source transfer.
* @param	DestPtr contains a pointer to the destination memory
*		where the data is to be transferred to.
* @param	DestWordLength is the number of words (32 bit) to be transferred
*		for the Destination transfer.
*
* @return              
*		- 0 if successful
*		- 1 if unsuccessful
*
* @note		None.
*
****************************************************************************/
u8 FDevcPs_initiateDma2(FDevcPs_T *devcDev, u32 SourcePtr, u32 DestPtr, u32 SrcWordLength, u32 DestWordLength)
{
	int ret = 0;

    FMSH_devc_portmap_TypeDef *portmap;
    
    portmap = (FMSH_devc_portmap_TypeDef *)devcDev->base_address; 
 
#if 0
    u64 count0, count1;
    double time;
    /*global_timer_enable();  */
    count0 = *(u64*)(FPS_GTC_BASEADDR + 0x8);
#endif  
    
    DEVC_OUT32P(SourcePtr, portmap->csu_dma_src_addr);
    DEVC_OUT32P(DestPtr, portmap->csu_dma_dest_addr);
	
    DEVC_OUT32P(SrcWordLength, portmap->csu_dma_src_len);
    DEVC_OUT32P(DestWordLength, portmap->csu_dma_dest_len);   
   
    /* Poll for the DMA done*/
    /*Poll for the D_P done*/
    ret = FDevcPs_pollDmaDone(devcDev, DEVC_MAX_COUNT);
	if (ret == 0)
	{
		printf("FDevcPs_pollDmaDone OK \n");
	}
	else
	{
		printf("FDevcPs_pollDmaDone fail \n");
	}
	
    ret = FDevcPs_pollDPDone(devcDev, DEVC_MAX_COUNT);
	if (ret == 0)
	{
		printf("FDevcPs_pollDPDone OK \n");
	}
	else
	{
		printf("FDevcPs_pollDPDone fail \n");
	}
    
#if 0
    count1 = *(u64*)(FPS_GTC_BASEADDR + 0x8);
    time = (count1 - count0) / (float)( GTC_CLK_FREQ * 1000 );
    LOG_OUT(1,"dma carry time is %fms\r\n",time);
#endif 
    
    return 0;
}

/******************************************************************************
*
*
* This function initiates the DMA transfer for readback.
*
* @param	dev is devc handle.
* @param	InstancePtr is a pointer to the XDcfg instance.
* @param	SourcePtr contains a pointer to the source memory where the data
*		is to be transferred from.
* @param	SrcWordLength is the number of words (32 bit) to be transferred
*		for the source transfer.
* @param	DestPtr contains a pointer to the destination memory
*		where the data is to be transferred to.
* @param	DestWordLength is the number of words (32 bit) to be transferred
*		for the Destination transfer.
*
* @return              
*		- 0 if successful
*		- 1 if unsuccessful
*
* @note		None.
*
******************************************************************************/
u8 FDevcPs_initiateDma_readback(FDevcPs_T *dev, u32 SourcePtr, u32 DestPtr, u32 SrcWordLength, u32 DestWordLength)
{
    FMSH_devc_portmap_TypeDef *portmap;
    
    portmap = (FMSH_devc_portmap_TypeDef *)dev->base_address; 
        
#ifdef TEST_TIME
    u64 count0, count1;
    double time;
    char s[10];
    global_timer_enable();  
    count0 = *(u64*)(FPS_GTC_BASEADDR + 0x8);
#endif     
        
    DEVC_OUTP(SourcePtr, portmap->csu_dma_src_addr);
    DEVC_OUTP(DestPtr, portmap->csu_dma_dest_addr);
    DEVC_OUTP(SrcWordLength, portmap->csu_dma_src_len);
    DEVC_OUTP(DestWordLength, portmap->csu_dma_dest_len);   
    
    FDevcPs_RDWR_B_HIGH(dev);  
    FDevcPs_CSI_B_LOW(dev); 
    delay_ms(50);
    FDevcPs_pollDmaDone(dev, MAX_COUNT);
    FDevcPs_pollDPDone(dev, MAX_COUNT);
        
#ifdef TEST_TIME
    count1 = *(u64*)(FPS_GTC_BASEADDR + 0x8);
    time = (count1 - count0) / 25000.0;/*(count1 - count0) *1000 / 25000000;*/
    sprintf(s, "%f", time);
    uart_logout("dma carry time is /ms");
    uart_logout(s);
#endif   
        
    return 0;
}

/******************************************************************************
*
*
* This function polls DMA done
*
* @param	dev is devc handle.
* @param	maxcount is the max poll times
*
* @return              
*		- 0 if successful
*		- 1 if unsuccessful
*
* @note		None.
*
******************************************************************************/
u8 FDevcPs_pollDmaDone(FDevcPs_T *dev, u32 MaxCount)
{
    FMSH_devc_portmap_TypeDef *portmap;

    int count = MaxCount;
    u32 reg = 0;
    portmap = (FMSH_devc_portmap_TypeDef *)dev->base_address; 

	
    reg = DEVC_INP(portmap->sac_int_sts);
	while (FMSH_BIT_GET(reg, DEVC_INT_STATUS_dma_done_int) != 1)  /* bfoDEVC_INT_STATUS_dma_done_int -> 13*/
    {
        reg = DEVC_INP(portmap->sac_int_sts);
                
		count -= 1;

        if (reg & FMSH_DEVC_ERROR_FLAGS_MASK)
            return 1;

		if (!count)
            return 1;
	}
	
    reg = 0; 
	/* bfoDEVC_INT_STATUS_dma_done_int -> 13*/
    FMSH_BIT_SET(reg, DEVC_INT_STATUS_dma_done_int, 1);  /*clear dma done int status. */
    DEVC_OUTP(reg, portmap->sac_int_sts);
    
    return 0;
}

/******************************************************************************
*
*
* This function polls D_P done
*
* @param	dev is devc handle.
* @param	maxcount is the max poll times
*
* @return              
*		- 0 if successful
*		- 1 if unsuccessful
*
* @note		None.
*
******************************************************************************/
u8 FDevcPs_pollDPDone(FDevcPs_T *dev, u32 MaxCount)
{
    FMSH_devc_portmap_TypeDef *portmap;

    int count = MaxCount;
    u32 reg = 0;
    portmap = (FMSH_devc_portmap_TypeDef *)dev->base_address; 


    reg = DEVC_IN32P(portmap->sac_int_sts);
    while (FMSH_BIT_GET(reg, DEVC_INT_STATUS_dp_done_int) != 1)  /*  bfoDEVC_INT_STATUS_dp_done_int -> 12*/
    {
        reg = DEVC_IN32P(portmap->sac_int_sts);

        count -= 1;

        if (reg & FMSH_DEVC_ERROR_FLAGS_MASK)
            return 1;

        if (!count)
            return 1;
    }

    reg = 0; 
    FMSH_BIT_SET(reg, DEVC_INT_STATUS_dp_done_int, 1);  /*clear dma done int status.*/
    DEVC_OUT32P(reg, portmap->sac_int_sts);

    return 0;
}
/******************************************************************************
*
*
* This function polls FPGA done
*
* @param	dev is devc handle.
* @param	maxcount is the max poll times
*
* @return              
*		- 0 if successful
*		- 1 if unsuccessful
*
* @note		None.
*
******************************************************************************/
u8 FDevcPs_pollFpgaDone(FDevcPs_T *dev, u32 maxcount)
{
    FMSH_devc_portmap_TypeDef *portmap;

    int count = maxcount;
    u32 reg = 0;
    portmap = (FMSH_devc_portmap_TypeDef *)dev->base_address; 

    reg = DEVC_INP(portmap->sac_status);

    while (FMSH_BIT_GET(reg, DEVC_STATUS_pcfg_done) != 1)
    {
        reg = DEVC_INP(portmap->sac_status);
        count -= 1;

        if (count == 0)
            return 1;
    }

#if 1
    u32 Dummy[16]={0};
    u8 i=0;
    Dummy[i++] = 0x20000000;
    Dummy[i++] = 0x20000000;
    Dummy[i++] = 0x20000000;   /*Type 1 Write 1 Word to CMD*/
    Dummy[i++] = 0x20000000;   /*DESYNC Command*/
    Dummy[i++] = 0x20000000;   /*NOOP*/
    Dummy[i++] = 0x20000000;   /*NOOP*/
    Dummy[i++] = 0x20000000;   /*NOOP*/
    Dummy[i++] = 0x20000000;   /*NOOP*/
    Dummy[i++] = 0x20000000;   /*NOOP*/
    Dummy[i++] = 0x20000000;   /*NOOP*/
    Dummy[i++] = 0x20000000;   /*NOOP*/
    Dummy[i++] = 0x20000000;     /*NOOP*/
    FDevcPs_transfer(dev, Dummy, 12, (u8 *)FMSH_DMA_INVALID_ADDRESS, 0, FMSH_NON_SECURE_PCAP_WRITE_DUMMMY);
#endif 

    return 0;
}


/******************************************************************************
*
*
* Generates a Type 1 packet head that reads back the requested Configuration
* register.
*
* @param	Register is the address of the register to be read back.
* @param	OpCode is the read/write operation code.
* @param	Size is the size of the word to be read.
*
* @return	Type 1 packet head to read the specified register
*
* @note		None.
*
*****************************************************************************/
static unsigned long FDevcPs_regAddr(u8 Register, u8 OpCode, u8 Size)
{
	/*
	 * Type 1 Packet Header Format
	 * The head section is always a 32-bit word.
	 *
	 * HeaderType | Opcode | Register Address | Reserved | Word count
	 * [31:29]	[28:27]		[26:13]	     [12:11]     [10:0]
	 * --------------------------------------------------------------
	 *   001 	  xx 	  RRRRRRRRRxxxxx	RR	xxxxxxxxxxx
	 *
	 * ?R? means the bit is not used and reserved for future use.
	 * The reserved bits should be written as 0s.
	 *
	 * Generating the Type 1 packet head which involves sifting of Type 1
	 * Header Mask, Register value and the OpCode which is 01 in this case
	 * as only read operation is to be carried out and then performing OR
	 * operation with the Word Length.
	 */
	return ( ((XDC_TYPE_1 << XDC_TYPE_SHIFT) |
		(Register << XDC_REGISTER_SHIFT) |
		(OpCode << XDC_OP_SHIFT)) | Size);
}

/******************************************************************************
*
*
* This function starts the DMA transfer. This function only starts the
* operation and returns before the operation may be completed.
* If the interrupt is enabled, an interrupt will be generated when the
* operation is completed, otherwise it is necessary to poll the Status register
* to determine when it is completed. It is the responsibility of the caller to
* determine when the operation is completed by handling the generated interrupt
* or polling the Status Register.
*
* @param	dev is devc handle.
* @param	SourcePtr contains a pointer to the source memory where the data
*		is to be transferred from.
* @param	SrcWordLength is the number of words (32 bit) to be transferred
*		for the source transfer.
* @param	DestPtr contains a pointer to the destination memory
*		where the data is to be transferred to.
* @param	DestWordLength is the number of words (32 bit) to be transferred
*		for the Destination transfer.
* @param	TransferType 
*           FMSH_NON_SECURE_PCAP_WRITE		0
*           FMSH_SECURE_PCAP_WRITE			1
*           FMSH_PCAP_READBACK			2
*           FMSH_NON_SECURE_PCAP_WRITE_DUMMMY	4
*
* @return              
*		- 0 if successful
*		- 1 if unsuccessful
*
* @note		None.
*
*****************************************************************************/
u8 FDevcPs_transfer(FDevcPs_T *dev,
                      void *SourcePtr, 
                      u32 SrcWordLength, 
                      void *DestPtr, 
                      u32 DestWordLength, 
                      u32 TransferType)
{ 
    u32 reg = 0;
    FMSH_devc_portmap_TypeDef *portmap;

    portmap = (FMSH_devc_portmap_TypeDef *)dev->base_address; 

    /*check if DMA command queue is full*/
    /*Read the PCAP sts register for DMA sts*/
    reg = DEVC_INP(portmap->sac_status);
    if(FMSH_BIT_GET(reg, DEVC_STATUS_dma_busy) == 1)  /* bfoDEVC_STATUS_dma_busy -> 31*/
        return 1;

    /*Check whether the fabric is in initialized state*/
    reg = DEVC_INP(portmap->sac_status);
    if(FMSH_BIT_GET(reg, DEVC_STATUS_pcfg_init) != 1)  /* bfoDEVC_STATUS_pcfg_init -> 4*/
        return 1;
        
    /*We don't need to check PCFG_INIT to be high for non-encrypted loopback transfers*/
	if ((TransferType == FMSH_SECURE_PCAP_WRITE) ||
		(TransferType == FMSH_NON_SECURE_PCAP_WRITE)) 
    {  
        /*Check for valid source pointer and length*/
        if ((!SourcePtr) || (SrcWordLength == 0))
                return 1;
        
        if (TransferType == FMSH_NON_SECURE_PCAP_WRITE)
                FDevcPs_downloadMode(dev, DOWNLOAD_BITSTREAM);/*set the mode:download bitstream*/

        if (TransferType == FMSH_SECURE_PCAP_WRITE)      
                FDevcPs_downloadMode(dev, SECURE_DOWNLOAD_BITSTREAM);/*set sec_down mode*/

        FDevcPs_RDWR_B_LOW(dev);
        FDevcPs_CSI_B_LOW(dev);

        FDevcPs_initiateDma2(dev, (u32)SourcePtr, (u32)DestPtr, SrcWordLength, DestWordLength);
    }
    else if (TransferType == FMSH_PCAP_READBACK)
    {
        if ((!DestPtr) || (DestWordLength == 0)) 
            return 1;

        /*Send READ Frame command to FPGA*/
        /*set the mode:download */
        FDevcPs_downloadMode(dev, DOWNLOAD_BITSTREAM);/*set the mode:download bitstream*/

        FDevcPs_RDWR_B_LOW(dev);
        FDevcPs_CSI_B_LOW(dev);

        FDevcPs_initiateDma2(dev, (u32)SourcePtr, PCAP_RD_DATA_ADDR, SrcWordLength, 0);
        
        FDevcPs_CSI_B_HIGH(dev);         

        FDevcPs_downloadMode(dev, READBACK_BITSTREAM);/*set the mode:readback bitstream mode*/
        
        /*Initiate the DMA write command.*/
        FDevcPs_initiateDma_readback(dev, PCAP_WR_DATA_ADDR, (u32)DestPtr, 0, DestWordLength);
    }
    else if(TransferType == FMSH_NON_SECURE_PCAP_WRITE_DUMMMY) 
    {
        if ((!SourcePtr) || (SrcWordLength == 0))
            return 1;
        
        FDevcPs_downloadMode(dev, DOWNLOAD_BITSTREAM);/*set the mode:download bitstream*/

        FDevcPs_CSI_B_HIGH(dev);
        FDevcPs_RDWR_B_LOW(dev);
		
        FDevcPs_initiateDma2(dev, (u32)SourcePtr, (u32)DestPtr, SrcWordLength, DestWordLength);
   }

    return 0;
}

/******************************************************************************
*
*
* This function returns the value of the specified configuration register or bitstream.
*
* @param	dev is devc handle.
* @param	DestinationDataPtr contains a pointer to the destination memory
*		    where the data is to be transferred to.
* @param	DestinationLength is the number of words (32 bit) to be transferred
*		    for the Destination transfer.
* @param	addr is the value of the specified configuration
*			register.
* @param	ConfigReg  is a constant which represents the configuration
*			register value to be returned.
*           if(ConfigReg == 0xaa55) radback bitstream
*
* @return              
*		- 0 if successful
*		- 1 if unsuccessful
*
* @note	None.
*
******************************************************************************/
u8 FDevcPs_getConfigdata(FDevcPs_T *dev, 
                           u32 *DestinationDataPtr, 
                           u32 DestinationLength, 
                           u32 addr, 
                           u32 ConfigReg)
{
    u32 len;
    u32 *p;
    u32 *src_addr = NULL;

    u32 reg = 0;
    FMSH_devc_portmap_TypeDef *portmap;

    portmap = (FMSH_devc_portmap_TypeDef *)dev->base_address; 

    src_addr = (u32*)malloc(500);
       
#ifdef FMSH_325T
        p = (u32*)src_addr;
        *p++ = 0x20000000;
        *p++ = 0x20000000;
        *p++ = 0x20000000;   /*Type 1 Write 1 Word to CMD*/
        *p++ = 0x20000000;   /*DESYNC Command*/
        *p++ = 0x20000000;   /*NOOP*/
        *p++ = 0x20000000;   /*NOOP*/
        *p++ = 0x20000000;   /*NOOP*/
        *p++ = 0x20000000;   /*NOOP*/
        *p++ = 0x20000000;   /*NOOP*/
        *p++ = 0x20000000;   /*NOOP*/
        *p++ = 0x20000000;   /*NOOP*/
        *p = 0x20000000;     /*NOOP*/
        len = 12;
        FDevcPs_transfer(dev, (u8*)src_addr, len, (u8 *)FMSH_DMA_INVALID_ADDRESS, 0, FMSH_NON_SECURE_PCAP_WRITE_DUMMMY);
       
        /*for fmsh PL the dummy value must be set 3 for readback reg, but the value must be set 4 for readback bitstream*/
        if(ConfigReg == 0xaa55) 
            FDevcPs_readbackDummyCount(dev, dummy_4);
#endif  
                
    p = (u32*)src_addr;
    
    /*Clear the interrupt sts bits*/
    reg = DEVC_INP(portmap->sac_int_sts); 
    FMSH_BIT_SET(reg, DEVC_INT_STATUS_dp_done_int, 1);
    FMSH_BIT_SET(reg, DEVC_INT_STATUS_dma_done_int, 1);
    FMSH_BIT_SET(reg, DEVC_INT_STATUS_pcfg_done_int, 1);
    DEVC_OUTP(reg, portmap->sac_int_sts); 

	/* Check if DMA command queue is full */
    reg = DEVC_INP(portmap->sac_status); 
    if(FMSH_BIT_GET(reg, DEVC_STATUS_dma_busy) == 1)
            return 1;
        
	/*
	 * Register Readback in non secure mode
	 * Create the data to be written to read back the
	 * Configuration Registers from PL Region.
	 */
    
    if(ConfigReg == 0xaa55) 
    {	
        *p++ = 0xFFFFFFFF; 	/* Dummy Word */
        *p++ = 0xFFFFFFFF; 	/* Dummy Word */
        *p++ = 0xFFFFFFFF; 	/* Dummy Word */
        *p++ = 0xFFFFFFFF; 	/* Dummy Word */
        *p++ = 0xFFFFFFFF; 	/* Dummy Word */
        *p++ = 0xFFFFFFFF; 	/* Dummy Word */
        *p++ = 0xFFFFFFFF; 	/* Dummy Word */
        *p++ = 0xFFFFFFFF; 	/* Dummy Word */
        *p++ = 0xFFFFFFFF; 	/* Dummy Word */
        *p++ = 0xFFFFFFFF; 	/* Dummy Word */
        *p++ = 0xFFFFFFFF; 	/* Dummy Word */
        *p++ = 0xFFFFFFFF; 	/* Dummy Word */
        *p++ = 0x000000BB; 	/* Bus Width Sync Word */
        *p++ = 0x11220044; 	/* Bus Width Detect */
        *p++ = 0xFFFFFFFF; 	/* Dummy Word */
        *p++ = 0xAA995566; 	/* Sync Word */      
        *p++ = 0x02000000; 	/* Type 1 NOOP Word 0 */     
        /*  *++ = 0x30008001;  //Type 1 Write 1 Word to CMD*/
        /*  *++ = 0x0000000B;  //SHUTDOWN Command*/
        /*  *++ = 0x02000000;  //Type 1 NOOP Word 0            */
        /**++ = 0x30008001;  //Type 1 Write 1 Word to CMD    */
        /**++ = 0x00000007;  //RCRC Command*/
        *p++ = 0x20000000;  /*Type 1 NOOP Word 0    */
        *p++ = 0x20000000;  /*Type 1 NOOP Word 0*/
        *p++ = 0x20000000;  /*Type 1 NOOP Word 0*/
        *p++ = 0x20000000;  /*Type 1 NOOP Word 0*/
        *p++ = 0x20000000;  /*Type 1 NOOP Word 0*/
        *p++ = 0x20000000;  /*Type 1 NOOP Word 0     */
        *p++ = 0x30008001;  /*Type 1 Write 1 Word to CMD*/
        *p++ = 0x00000004;  /*RCFG Command*/
        *p++ = 0x20000000;  /*Type 1 NOOP Word 0 */
        *p++ = 0x30002001;  /*Type 1 Write 1 Word to FAR*/
        *p++ = addr; 	/*FAR Address = 00000000     */
        *p++ = 0x28006000;  /*Type 1 Read 0 Words from FDRO     */
        *p++ = 0x48000000 | DestinationLength;  /*Type 2 Read 202 Words from FDRO(for 7K325T)*/
        *p++ = 0x20000000;  /*Type 1 NOOP Word 0*/
        *p++ = 0x20000000;  /*Type 1 31 More NOOPs Word 0     */
        *p++ = 0x20000000;
        *p++ = 0x20000000;
        *p++ = 0x20000000; 
        *p++ = 0x20000000; 
        *p++ = 0x20000000; 
        *p++ = 0x20000000;
        *p++ = 0x20000000; 
        *p++ = 0x20000000;        
        *p++ = 0x20000000; 
        *p++ = 0x20000000;    
        *p++ = 0x20000000; 
        *p++ = 0x20000000; 
        *p++ = 0x20000000; 
        *p++ = 0x20000000; 
        *p++ = 0x20000000; 
        *p++ = 0x20000000;        
        *p++ = 0x20000000; 
        *p++ = 0x20000000;    
        *p++ = 0x20000000; 
        *p++ = 0x20000000; 
        *p++ = 0x20000000; 
        *p++ = 0x20000000; 
        *p++ = 0x20000000; 
        *p++ = 0x20000000;        
        *p++ = 0x20000000; 
        *p++ = 0x20000000;    
        *p++ = 0x20000000; 
        *p++ = 0x20000000; 
        *p++ = 0x20000000; 
        *p = 0x20000000; 

        len = p-src_addr+1;
    }
    else
    {
        *p++ = 0xFFFFFFFF; 	/* Dummy Word */
        *p++ = 0xFFFFFFFF; 	/* Dummy Word */     
        *p++ = 0xFFFFFFFF; 	/* Dummy Word */
        *p++ = 0xFFFFFFFF; 	/* Dummy Word */    
        *p++ = 0xFFFFFFFF; 	/* Dummy Word */
        *p++ = 0xFFFFFFFF; 	/* Dummy Word */     
        *p++ = 0xFFFFFFFF; 	/* Dummy Word */
        *p++ = 0xFFFFFFFF; 	/* Dummy Word */  
        *p++ = 0x000000BB; 	/* Bus Width Sync Word */
        *p++ = 0x11220044; 	/* Bus Width Detect */
        *p++ = 0xFFFFFFFF; 	/* Dummy Word */
        *p++ = 0xFFFFFFFF; 	/* Dummy Word */
        *p++ = 0xAA995566; 	/* Sync Word */
        *p++ = 0x20000000; 	/* Type 1 NOOP Word 0 */ 
        /**++ = 0x30008001; */
        /**++ = 0x00000007; */
        *p++ = FDevcPs_regAddr(addr, OPCODE_READ, 0x1);    /*addr*/
        *p++ = 0x20000000; 
        *p++ = 0x20000000; 
        *p++ = 0x20000000; 
        *p++ = 0x20000000; 
        *p++ = 0x20000000; 
        *p++ = 0x20000000;
        *p++ = 0x20000000; 
        *p++ = 0x20000000; 
        *p++ = 0x20000000;
        *p++ = 0x20000000; 
        *p++ = 0x20000000; 
        *p++ = 0x20000000;
        *p++ = 0x20000000; 
        *p++ = 0x20000000; 
        *p = 0x20000000; 

        len = p-src_addr+1;
    }

    devc_byte_swap((u32*)src_addr, len);
    FDevcPs_transfer(dev, (u8*)src_addr, len, (u8 *)DestinationDataPtr, DestinationLength, FMSH_PCAP_READBACK);

    FDevcPs_CSI_B_HIGH(dev);   

    if( ConfigReg == 0xaa55 )
    {	
        p = (u32*)src_addr;
        *p++ = 0x20000000;/*Type 1 NOOP Word 0        */
        *p++ = 0x20000000;/*Type 1 NOOP Word 0*/
        *p++ = 0x30008001;/*Type 1 Write 1 Word to CMD*/
        *p++ = 0x0000000D;/*START Command*/
        *p++ = 0x20000000;/*Type 1 NOOP Word 0*/
        *p++ = 0x20000000;/*Type 1 Write 1 Word to CMD*/
        *p++ = 0x20000000;/*RCRC Command*/
        *p++ = 0x20000000;/*Type 1 NOOP Word 0*/
        *p++ = 0x20000000;/*Type 1 Write 1 Word to CMD*/
        *p++ = 0x20000000;/*DESYNC Command*/
        *p++ = 0x20000000;/*Type 1 NOOP Word 0*/
        *p = 0x20000000;  /*Type 1 NOOP Word 0*/
        len = 12;    
    } 
    else 
    {
        p = (u32*)src_addr;
        *p++ = 0x20000000;
        *p++ = 0x20000000;
        *p++ = 0x30008001;   /*Type 1 Write 1 Word to CMD*/
        *p++ = 0x0000000D;   /*DESYNC Command*/
        *p++ = 0x20000000;   /*NOOP*/
        *p++ = 0x20000000;   /*NOOP*/
        *p++ = 0x20000000;   /*NOOP*/
        *p++ = 0x20000000;   /*NOOP*/
        *p++ = 0x20000000;   /*NOOP*/
        *p++ = 0x20000000;   /*NOOP*/
        *p++ = 0x20000000;   /*NOOP*/
        *p = 0x20000000;     /*NOOP*/
        len = 12;
    }
    devc_byte_swap((u32*)src_addr, len);
	FDevcPs_transfer(dev, (u8*)src_addr, len, (u8 *)FMSH_DMA_INVALID_ADDRESS, 0, FMSH_NON_SECURE_PCAP_WRITE);

    FDevcPs_CSI_B_HIGH(dev);    
    FDevcPs_RDWR_B_HIGH(dev);   
#ifdef FMSH_325T
        p = (u32*)src_addr;
        *p++ = 0x20000000;
        *p++ = 0x20000000;
        *p++ = 0x20000000;   /*Type 1 Write 1 Word to CMD*/
        *p++ = 0x20000000;   /*DESYNC Command*/
        *p++ = 0x20000000;   /*NOOP*/
        *p++ = 0x20000000;   /*NOOP*/
        *p++ = 0x20000000;   /*NOOP*/
        *p++ = 0x20000000;   /*NOOP*/
        *p++ = 0x20000000;   /*NOOP*/
        *p++ = 0x20000000;   /*NOOP*/
        *p++ = 0x20000000;   /*NOOP*/
        *p = 0x20000000;     /*NOOP*/
        len = 12;
        FDevcPs_transfer(dev, (u8*)src_addr, len, (u8 *)FMSH_DMA_INVALID_ADDRESS, 0, FMSH_NON_SECURE_PCAP_WRITE_DUMMMY);
       
        /*for fmsh PL the dummy value must be set 3 for readback reg, but the value must be set 4 for readback bitstream*/
        if(ConfigReg == 0xaa55) 
          FDevcPs_readbackDummyCount(dev, dummy_4);
#endif  
    free(src_addr);
    src_addr = NULL;
    
    return 0;
}
/******************************************************************************
*
*
* This function starts the DMA transfer. This function only starts the
* operation and returns before the operation may be completed.
* If the interrupt is enabled, an interrupt will be generated when the
* operation is completed, otherwise it is necessary to poll the Status register
* to determine when it is completed. It is the responsibility of the caller to
* determine when the operation is completed by handling the generated interrupt
* or polling the Status Register.
*
* @param	dev is devc handle.
* @param	SourceDataPtr contains a pointer to the source memory where the data
*		is to be transferred from.
* @param	SourceLength is the number of words (32 bit) to be transferred
*		for the source transfer.
* @param	DestinationDataPtr contains a pointer to the destination memory
*		where the data is to be transferred to.
* @param	DestinationLength is the number of words (32 bit) to be transferred
*		for the Destination transfer.
* @param	SecureTransfer 
*           FMSH_NON_SECURE_PCAP_WRITE		0
*           FMSH_SECURE_PCAP_WRITE			1
*           FMSH_PCAP_READBACK			2
*
* @return              
*		- 0 if successful
*		- 1 if unsuccessful
*
* @note		None.
*
*****************************************************************************/
u8 FDevcPs_pcapLoadPartition(FDevcPs_T *dev, 
                               u32 *SourceDataPtr, 
                               u32 *DestinationDataPtr, 
                               u32 SourceLength, 
                               u32 DestinationLength, 
                               u32 SecureTransfer)
{
    u32 sts;
    u32 reg = 0;
    FMSH_devc_portmap_TypeDef *portmap;

    portmap = (FMSH_devc_portmap_TypeDef *)dev->base_address; 

    sts = FDevcPs_clearPcapStatus(dev);
    if(sts == 1)
    return 1;

    sts = FDevcPs_transfer(dev, (u8*)SourceDataPtr,
                           SourceLength,(u8 *)DestinationDataPtr,
                           DestinationLength, SecureTransfer);
    
    if(sts == 1)
        return 1;

    FDevcPs_CSI_B_HIGH(dev);     
    FDevcPs_RDWR_B_HIGH(dev);

    /*Check for errors*/
    reg = DEVC_INP(portmap->sac_int_sts);
    if (reg & FMSH_DEVC_ERROR_FLAGS_MASK)
        return 1;

    return 0;
}

/******************************************************************************
*
*
* This function will handle the AES/SM4-GCM Decryption.
*
* The Multiple key(a.k.a Key Rolling) or Single key
* Encrypted images will have the same format,
* such that it will have the following:
* 
* Secure head -->	Dummy AES/SM4 Key of 32byte +
* 					Block 0 IV of 12byte +
* 					DLC for Block 0 of 4byte +
* 					GCM tag of 16byte(Un-Enc).
* Block N --> Boot Image Data for Block N of n size +
* 			Block N+1 AES key of 32byte +
* 			Block N+1 IV of 12byte +
* 			GCM tag for Block N of 16byte(Un-Enc).
* 
* The Secure head and Block 0 will be decrypted using
* Device key or user provide key.
* If more than 1 blocks are found then the key and IV
* obtained from previous block will be used for decryption
* 
* 
* 1> Read the 1st 64bytes and decrypt 48 bytes using
* 	the selected Device key.
* 2> Decrypt the 0th block using the IV + Size from step 2
* 	and selected device key.
* 3> After decryption we will get decrypted data+KEY+IV+Blk
* 	Size so store the KEY/IV into KUP/IV registers.
* 4> Using Blk size, IV and Next Block Key information
* 	start decrypting the next block.
* 5> if the Current Image size > Total image length,
* 	go to next step 8. Else go back to step 5
* 6> If there are failures, return error code
* 7> If we have reached this step means the decryption is SUCCESS.
*
* @para��alg AES / SM4
* @para��no opkey / use opkey
* @para��bitlen -- bitsteam length
*
* @return   0 if successful, otherwise 1.
*
* @note     None.
*
******************************************************************************/
u8 keyRolling_download(FDevcPs_T *dev, enum ALG alg, enum DECFLAG decflag, u32 *srcPtr, u32 bitlen)
{
    u8 ret;
    u8 block_no;
    u32 head;
    u32 next_len;
    u32 current_totalLen;

    FMSH_devc_portmap_TypeDef *portmap;
    
    portmap = (FMSH_devc_portmap_TypeDef *)dev->base_address; 
    
    FDevcPs_Prog_B(dev);

    FDevcPs_smap32Swap(dev, smap32_swap_enable);
    FDevcPs_secDownDataByteSwap(dev, byte_swap);
    FDevcPs_txDataSwap(dev, byte_swap); 
    FDevcPs_rxDataSwap(dev, byte_swap);
    
    FDevcPs_RDWR_B_LOW(dev);
    FDevcPs_CSI_B_LOW(dev);

    FDevcPs_CTRL_AESenable(dev);
    FDevcPs_downloadMode(dev, SECURE_DOWNLOAD_BITSTREAM);
    
    FDevcPs_AES_CR_AESENDisable(dev);/*GCM_EN = 0*/
    
    FDevcPs_AES_CR_ALG_SEL(dev, alg);
    FDevcPs_AES_CR_MODE(dev, DCODE);
    FDevcPs_AES_CR_CHMOD(dev, CTR);

    FDevcPs_AES_KEY_SOURCE(dev, DEV_KEY);
    FDevcPs_AES_KEY_IV_LOAD(dev);  
    
    FDevcPs_AES_CR_AESENEnable(dev);/*GCM_EN = 1*/
    
    /*set dec flag = secure head,only iv_write_en,no use op_key*/
    FDevcPs_AES_DEC_FLAG(dev, decflag);

    /***********************HEADER********************************/
    head = 0;
    /*head is 12 word*/
    FDevcPs_initiateDma(dev, (u32)srcPtr, PCAP_WR_DATA_ADDR, 12, 12);
    /*set en=0*/
    FDevcPs_AES_CR_AESENDisable(dev);
    
    /***********************BLOCK0 - N********************************/ 
    block_no = 0;
    head += 16 * 4;
    current_totalLen = 16;
    next_len =  DEVC_INP(portmap->IVUP0); 
    
    devc_byte_swap((u32*)&next_len, 1);/*get next block length*/
        
    while((current_totalLen < bitlen) && (next_len != 0)) 
    {
        FDevcPs_AES_DEC_FLAG(dev, clear);                 
        if(block_no == 0) 
        {
            /***********************BLOCK0********************************/

            /*set key_src=DEV 4'b0001*/
            FDevcPs_AES_KEY_SOURCE(dev, DEV_KEY);
            
            DEVC_OUTP(0x2, portmap->IVUP0); 
            /*load the key and iv                         */
            FDevcPs_AES_KEY_IV_LOAD(dev);  
            /*set en=1*/
            FDevcPs_AES_CR_AESENEnable(dev);
            /*set dec flag = blk0*/
            FDevcPs_AES_DEC_FLAG(dev, ivup_kup_wr_en);     
            FDevcPs_initiateDma(dev, (u32)srcPtr + head, PCAP_WR_DATA_ADDR, next_len + 12, next_len + 12);
            /*set en=0*/
            FDevcPs_AES_CR_AESENDisable(dev);        
        }
        else 
        {
            /***********************BLOCK1-N********************************/
            /*set key_src=DEV 4'b0001*/
            FDevcPs_AES_KEY_SOURCE(dev, KUP);   
            
            DEVC_OUTP(0x2, portmap->IVUP0);         
            /*load the key and iv                         */
            FDevcPs_AES_KEY_IV_LOAD(dev);  
            /*set en=1*/
            FDevcPs_AES_CR_AESENEnable(dev);
            /*set dec flag = blk1*/
            FDevcPs_AES_DEC_FLAG(dev, ivup_kup_wr_en);        
            FDevcPs_initiateDma(dev, (u32)srcPtr + head, PCAP_WR_DATA_ADDR, next_len + 12, next_len + 12);
            /*set en=0*/
            FDevcPs_AES_CR_AESENDisable(dev);   
        }
        block_no++;
        
        head += (next_len + 16) * 4;
        current_totalLen += (next_len + 16);
                        
        next_len =  DEVC_INP(portmap->IVUP0);
        devc_byte_swap((u32*)&next_len, 1);/*get next block length*/
    }
    
    FDevcPs_AES_DEC_FLAG(dev, clear);/*clear DEC flag*/
    FDevcPs_CTRL_AESdisable(dev);
    
    ret = FDevcPs_pollFpgaDone(dev, MAX_COUNT);
        
    return ret;
}

/******************************************************************************
*
*
* This function is used to secure download different BITSTREAM
*
* @param   alg_flag�� 0 -- AES 1 -- SM4
* @param   opkey_flag�� 0 -- no opkey 1 -- use opkey
* @param   bitlen: PL BITSTREAM LENGTH
*
* @return   0 if successful, otherwise 1.
*
* @note     None.
*
******************************************************************************/
u8 FDevcPs_keyRollingDownload(FDevcPs_T *dev, u8 alg_flag, u8 opkey_flag, u32 *srcPtr, u32 bitlen)
{
    u8 ret = 0;

    if((alg_flag == 0) && (opkey_flag == 0))
        ret |= keyRolling_download(dev, AES, no_opkey, srcPtr, bitlen);
    else if((alg_flag == 0) && (opkey_flag == 1)) 
        ret |= keyRolling_download(dev, AES, use_opkey, srcPtr, bitlen);
    else if((alg_flag == 1) && (opkey_flag == 0)) 
        ret |= keyRolling_download(dev, SM4, no_opkey, srcPtr, bitlen);
    else if((alg_flag == 1) && (opkey_flag == 1)) 
        ret |= keyRolling_download(dev, SM4, use_opkey, srcPtr, bitlen);
    else
        return 1;
    
    return ret;        
}


#if 1

unsigned long temp_ps_pll_init_data[32]=
{
    /* ARM_PLL_BYPASS_FORCE = 1*/
    EMIT_MASKWRITE(FPS_SLCR_BASEADDR + 0x100, 0x00000008U, 0x00000008U),
    
    /* ARM_PLL_RESET = 1*/
    EMIT_MASKWRITE(FPS_SLCR_BASEADDR + 0x100, 0x00000001U, 0x00000001U),
    
    /* ARM_PLL_FDIV = 13*/
    /* [22:16]*/
    EMIT_MASKWRITE(FPS_SLCR_BASEADDR + 0x100, 0x007f0000U, 0x000c0000U),
    
    /* ARM_PLL_CLKOUT0_DIVISOR = 2*/
    /* [6:0]*/
    EMIT_MASKWRITE(FPS_SLCR_BASEADDR + 0x104, 0x0000007fU, 0x00000002U),
    
    /* ENABLE ARM PLL LATCH*/
    EMIT_WRITE(FPS_SLCR_BASEADDR + 0x180, 0x00000001U), 
    
    /* ARM_PLL_RESET = 0*/
    EMIT_MASKWRITE(FPS_SLCR_BASEADDR + 0x100, 0x00000001U, 0x00000000U),
    
    /* CHECK ARM PLL STATUS*/
    EMIT_MASKPOLL(FPS_SLCR_BASEADDR + 0x170, 0x00000001U),
    
    /* ARM_PLL_BYPASS_FORCE = 0*/
    EMIT_MASKWRITE(FPS_SLCR_BASEADDR + 0x100, 0x00000008U, 0x00000000U),

    EMIT_EXIT(),
};


unsigned long ps_pll_init_data2[] = 
{
    /* UNLOCK_KEY = 0XDF0D767B*/
    EMIT_WRITE(SLCR_REG_BASE + 0x008, 0xDF0D767BU),

    /* CPU_PLL_BYPASS_FORCE = 1*/
    EMIT_MASKWRITE(SLCR_REG_BASE + 0x100, 0x00000008U, 0x00000008U),
    /* CPU_PLL_RESET = 1*/
    EMIT_MASKWRITE(SLCR_REG_BASE + 0x100, 0x00000001U, 0x00000001U),
    /* CPU_PLL_FDIV = 80*/
    /* [22:16]*/
    EMIT_MASKWRITE(SLCR_REG_BASE + 0x100, 0x007f0000U, 0x00500000U),
    /* CPU_PLL_CLKOUT0_DIVISOR = 4*/
    /* [6:0]*/
    EMIT_MASKWRITE(SLCR_REG_BASE + 0x104, 0x0000007fU, 0x00000004U),
    /* CPU_PLL_CLKOUT1_DIVISOR = 47*/
    /* [6:0]*/
    EMIT_MASKWRITE(SLCR_REG_BASE + 0x108, 0x0000007fU, 0x0000002fU),
    /* CPU_PLL_CLKOUT2_DIVISOR = 47*/
    /* [6:0]*/
    EMIT_MASKWRITE(SLCR_REG_BASE + 0x10C, 0x0000007fU, 0x0000002fU),
    /* CPU_PLL_CLKOUT3_DIVISOR = 47*/
    /* [6:0]*/
    EMIT_MASKWRITE(SLCR_REG_BASE + 0x120, 0x0000007fU, 0x0000002fU),
    /* CPU_PLL_CLKOUT4_DIVISOR = 47*/
    /* [6:0]*/
    EMIT_MASKWRITE(SLCR_REG_BASE + 0x124, 0x0000007fU, 0x0000002fU),
    /* CPU_PLL_CLKOUT5_DIVISOR = 47*/
    /* [6:0]*/
    EMIT_MASKWRITE(SLCR_REG_BASE + 0x128, 0x0000007fU, 0x0000002fU),
    /* ENABLE CPU PLL LATCH*/
    EMIT_WRITE(SLCR_REG_BASE + 0x180, 0x00000001U), 
    /* CPU_PLL_RESET = 0*/
    EMIT_MASKWRITE(SLCR_REG_BASE + 0x100, 0x00000001U, 0x00000000U),
    /* CHECK CPU PLL STATUS*/
    EMIT_MASKPOLL(SLCR_REG_BASE + 0x170, 0x00000001U),
    /* CPU_PLL_BYPASS_FORCE = 0*/
    EMIT_MASKWRITE(SLCR_REG_BASE + 0x100, 0x00000008U, 0x00000000U),
    /* DDR_PLL_BYPASS_FORCE = 1*/
    EMIT_MASKWRITE(SLCR_REG_BASE + 0x130, 0x00000008U, 0x00000008U),
    /* DDR_PLL_RESET = 1*/
    EMIT_MASKWRITE(SLCR_REG_BASE + 0x130, 0x00000001U, 0x00000001U),
    /* DDR_PLL_FDIV = 60*/
    /* [22:16]*/
    EMIT_MASKWRITE(SLCR_REG_BASE + 0x130, 0x007f0000U, 0x003c0000U),
    /* DDR_PLL_CLKOUT0_DIVISOR = 47*/
    /* [6:0]*/
    EMIT_MASKWRITE(SLCR_REG_BASE + 0x134, 0x0000007fU, 0x0000002fU),
    /* DDR_PLL_CLKOUT1_DIVISOR = 5*/
    /* [6:0]*/
    EMIT_MASKWRITE(SLCR_REG_BASE + 0x138, 0x0000007fU, 0x00000005U),
    /* DDR_PLL_CLKOUT2_DIVISOR = 47*/
    /* [6:0]*/
    EMIT_MASKWRITE(SLCR_REG_BASE + 0x13C, 0x0000007fU, 0x0000002fU),
    /* DDR_PLL_CLKOUT3_DIVISOR = 47*/
    /* [6:0]*/
    EMIT_MASKWRITE(SLCR_REG_BASE + 0x140, 0x0000007fU, 0x0000002fU),
    /* DDR_PLL_CLKOUT4_DIVISOR = 47*/
    /* [6:0]*/
    EMIT_MASKWRITE(SLCR_REG_BASE + 0x144, 0x0000007fU, 0x0000002fU),
    /* DDR_PLL_CLKOUT5_DIVISOR = 2*/
    /* [6:0]*/
    EMIT_MASKWRITE(SLCR_REG_BASE + 0x148, 0x0000007fU, 0x00000002U),
    /* ENABLE DDR PLL LATCH*/
    EMIT_WRITE(SLCR_REG_BASE + 0x184, 0x00000001U), 
    /* DDR_PLL_RESET = 0*/
    EMIT_MASKWRITE(SLCR_REG_BASE + 0x130, 0x00000001U, 0x00000000U),
    /* CHECK DDR PLL STATUS*/
    EMIT_MASKPOLL(SLCR_REG_BASE + 0x170, 0x00000002U),
    /* DDR_PLL_BYPASS_FORCE = 0*/
    EMIT_MASKWRITE(SLCR_REG_BASE + 0x130, 0x00000008U, 0x00000000U),
    /* IO_PLL_BYPASS_FORCE = 1*/
    EMIT_MASKWRITE(SLCR_REG_BASE + 0x150, 0x00000008U, 0x00000008U),
    /* IO_PLL_RESET = 1*/
    EMIT_MASKWRITE(SLCR_REG_BASE + 0x150, 0x00000001U, 0x00000001U),
    /* IO_PLL_FDIV = 90*/
    /* [22:16]*/
    EMIT_MASKWRITE(SLCR_REG_BASE + 0x150, 0x007f0000U, 0x005a0000U),
    /* IO_PLL_CLKOUT0_DIVISOR = 47*/
    /* [6:0]*/
    EMIT_MASKWRITE(SLCR_REG_BASE + 0x154, 0x0000007fU, 0x0000002fU),
    /* IO_PLL_CLKOUT1_DIVISOR = 47*/
    /* [6:0]*/
    EMIT_MASKWRITE(SLCR_REG_BASE + 0x158, 0x0000007fU, 0x0000002fU),
    /* IO_PLL_CLKOUT2_DIVISOR = 30*/
    /* [6:0]*/
    EMIT_MASKWRITE(SLCR_REG_BASE + 0x15C, 0x0000007fU, 0x0000001eU),
    /* IO_PLL_CLKOUT3_DIVISOR = 47*/
    /* [6:0]*/
    EMIT_MASKWRITE(SLCR_REG_BASE + 0x160, 0x0000007fU, 0x0000002fU),
    /* IO_PLL_CLKOUT4_DIVISOR = 15*/
    /* [6:0]*/
    EMIT_MASKWRITE(SLCR_REG_BASE + 0x164, 0x0000007fU, 0x0000000fU),
    /* IO_PLL_CLKOUT5_DIVISOR = 30*/
    /* [6:0]*/
    EMIT_MASKWRITE(SLCR_REG_BASE + 0x168, 0x0000007fU, 0x0000001eU),
    /* ENABLE IO PLL LATCH*/
    EMIT_WRITE(SLCR_REG_BASE + 0x188, 0x00000001U), 
    /* IO_PLL_RESET = 0*/
    EMIT_MASKWRITE(SLCR_REG_BASE + 0x150, 0x00000001U, 0x00000000U),
    /* CHECK IO PLL STATUS*/
    EMIT_MASKPOLL(SLCR_REG_BASE + 0x170, 0x00000004U),
    /* IO_PLL_BYPASS_FORCE = 0*/
    EMIT_MASKWRITE(SLCR_REG_BASE + 0x150, 0x00000008U, 0x00000000U),

    /* LOCK_KEY = 0XDF0D767B*/
    EMIT_WRITE(SLCR_REG_BASE + 0x004, 0xDF0D767BU),

    EMIT_EXIT(),
};

/*****************************************************************************/
/**
* This function reduce the cpu freq.
*
* @param	
*
* @return
*		- 0 if successful
*		- 1 if unsuccessful

*
* @note		None
*
****************************************************************************/
u8 FDevcPs_adjustFreq(u8* flag)
{
    u32 clk_div=1;
    u8 ret=FMSH_SUCCESS;
    u32 pll_multi=8;
    u32 pll_div=2;
    
    FMSH_WriteReg(FPS_SLCR_BASEADDR, 0x008, 0xDF0D767B);   /* unlock SLCR */
	
    if (FMSH_ReadReg(FPS_SLCR_BASEADDR , 0x20C) & 0x1 == 0x1) 
      clk_div=12;
    else
      clk_div=8;
    
    pll_div = FMSH_ReadReg(FPS_SLCR_BASEADDR , 0x104)&0x7f;
	
    if (APU_FREQ/clk_div > 20000000)  /* 20_000_000 -> 20M Hz*/
    {
		/*pll_multi=35*lk_div*ll_div*000000/PS_CLK_FREQ;*/
		
		pll_multi = (35 * clk_div * pll_div * 1000000) / PS_CLK_FREQ;
		
		/*pll_multi=40*lk_div*ll_div*000000/PS_CLK_FREQ;*/
		
		temp_ps_pll_init_data[11] = pll_multi<<16;
		temp_ps_pll_init_data[15] = pll_div;
		
		/* PLL init */
		ret = ps_config(temp_ps_pll_init_data);
		if (ret != FMSH_SUCCESS) 
			return ret;
		
		*flag=1;
    }
	
    FMSH_WriteReg(FPS_SLCR_BASEADDR,0x004, 0xDF0D767B);/* relock SLCR */
    return ret;
}

u8 FDevcPs_adjustFreq2(u8* flag)
{
	/*fmsh_slcr_unlock();*/
	/* Set IOPLL as CPU clock for download bitfile */
	/*clrsetbits_le32(&slcr_base->arm_clk_ctrl, 0x03, 0x03);*/
	/*fmsh_slcr_lock();*/
	UINT32 tmp32 = 0;

	tmp32 = slcr_read(0x210) | 0x03;
	slcr_write(0x210, tmp32);
	
	*flag = 1;

	return FMSH_SUCCESS;
}

/*****************************************************************************/
/**
* This function recover the cpu freq.
*
* @param	
*
* @return
*		- 0 if successful
*		- 1 if unsuccessful
*
* @note		None
*
****************************************************************************/
u8 FDevcPs_recoverFreq(void)
{ 
    u32 idx;
    u8 ret = FMSH_SUCCESS;
	
    unsigned long pll_data[256] = {0};
	
    for (idx=0; idx<256; idx++)
    {
        pll_data[idx] = ps_pll_init_data2[idx];
		
        /*arm pll force*/
        if (pll_data[idx-3]==( (OPCODE_MASKWRITE << 4 ) | 3 ) &&
              pll_data[idx-2]==SLCR_REG_BASE + 0x100 &&
                  pll_data[idx-1]== 0x00000008U &&
                     pll_data[idx]== 0x00000000U)
              break;  
    }
	
    if (idx==256)
		return 1;
	
    pll_data[idx]= EMIT_EXIT();
	
    /* PLL init */
    ret = ps_config(pll_data);
	
    if (ret != FMSH_SUCCESS) 
  		return ret;
    
    return 0;
}

u8 FDevcPs_recoverFreq2(void)
{
	UINT32 tmp32 = 0;

	tmp32 = (slcr_read(0x210) & (~0x03));
	slcr_write(0x210, tmp32);
	
	return FMSH_SUCCESS;
}

#endif


#if 1



void FmshFsbl_OpenCfgLevelShifter2(void)
{
   FMSH_WriteReg(FPS_SLCR_BASEADDR, 0x008, 0xDF0D767B);/* unlock SLCR*/
   
   FMSH_WriteReg(FPS_SLCR_BASEADDR, 0x83C, 0xF);/*open the cfg-level-shifter*/
   
   FMSH_WriteReg(FPS_SLCR_BASEADDR, 0x004, 0xDF0D767B);/* relock SLCR */
}



u8 FDevcPs_init2(FDevcPs_T *devcDev, u32 addr)
{
    devcDev->base_address = (void*)addr;

    return 0;
}

u8 FDevcPs_devcInitDownload2()
{
    u8 ret=FMSH_SUCCESS;
    FDevcPs_T devcDev;
	
    FDevcPs_init2(&devcDev, 0xE0040000);    
	
    ret = FDevcPs_Prog_B(&devcDev);  /* reset to clear the pl old bit-stream*/

	taskDelay(10);
    ret = FDevcPs_Prog_B(&devcDev);  /* reset to clear the pl old bit-stream*/

	taskDelay(10);
    ret = FDevcPs_Prog_B(&devcDev);  /* reset to clear the pl old bit-stream*/
    
    return ret;
}


/*****************************************************************************/
/**
* This function sets read FIFO threshold
*
* @param	devcDev is devc handle.
* @param	mode
*       readFifoThre_hex_0x40 = 0x0,//default
*       readFifoThre_hex_0x80 = 0x1,
*       readFifoThre_hex_0xc0 = 0x2,
*       readFifoThre_hex_0x100 = 0x3
*
* @return
*		- 0 if successful
*		- 1 if unsuccessful
*
* @note		None
*
****************************************************************************/
u8 FDevcPs_readFifoThre2(FDevcPs_T *devcDev, enum readFifoThre mode)
{  
    u32 reg; 
    
    FMSH_devc_portmap_TypeDef *portmap;
    
    portmap = (FMSH_devc_portmap_TypeDef *)devcDev->base_address; 
    
    reg = DEVC_IN32P(portmap->sac_cfg); 
    if(FMSH_BIT_GET(reg, DEVC_CFG_RFIFO_TH) != mode) 
    {
        FMSH_BIT_SET(reg, DEVC_CFG_RFIFO_TH, mode);   /* bfoDEVC_CFG_RFIFO_TH -> 8*/
        DEVC_OUT32P(reg, portmap->sac_cfg); 
    }
    
    return 0;
}

/*****************************************************************************/
/**
* This function initializes Device Configuration module, configure pcap interface work parameters
*
* @param	TransferType.
*		- FMSH_PCAP_READBACK
*		- FMSH_NON_SECURE_PCAP_WRITE
*		- FMSH_PCAP_LOOPBACK
*
* @return
*		- 0 if successful
*		- 1 if unsuccessful
*
* @note		None
*
****************************************************************************/
u8 FDevcPs_fabricInit2(FDevcPs_T *devcDev, u32 TransferType)
{        
    FDevcPs_unLockCSU(devcDev);

#if 1	
    u32 reg;     
    FMSH_devc_portmap_TypeDef *portmap;    
    portmap = (FMSH_devc_portmap_TypeDef *)devcDev->base_address;    
	
    reg = DEVC_INP(portmap->sac_cfg); 
	printf("portmap->sac_cfg: 0x%08X \n", reg);
    reg = DEVC_INP(portmap->sac_status); 
	printf("portmap->sac_status: 0x%08X \n", reg);
	
	DEVC_OUTP(0xC0057C0A, portmap->sac_cfg);
	delay_1ms();
	
    reg = DEVC_INP(portmap->sac_cfg); 
	printf("wr&rd portmap->sac_cfg: 0x%08X \n", reg);
#endif
	
    FDevcPs_smap32Swap(devcDev, smap32_swap_enable);
	
    FDevcPs_rclk_edge(devcDev, falling_edge);   /* default failing_edge , rising_edge will failed*/
    FDevcPs_wclk_edge(devcDev, falling_edge);   /* default failing_edge*/

	
    FDevcPs_CSI_B_HIGH(devcDev);	
    FDevcPs_RDWR_B_HIGH(devcDev);

	
    if (TransferType == FMSH_PCAP_READBACK) 
    {
        FDevcPs_rxDataSwap(devcDev, byte_swap);
        FDevcPs_txDataSwap(devcDev, byte_swap); 

        /*default 3 , for FMSH 325t the value must be set 4,*/
        /*for fmsh 325t the value must be set 3 for readback reg, but the value must be set 4 for readback bitstream*/
        FDevcPs_readbackDummyCount(devcDev, dummy_4);
        
        FDevcPs_downloadMode(devcDev, DOWNLOAD_BITSTREAM);
		
        FDevcPs_readFifoThre(devcDev, readFifoThre_hex_0x40);
    } 
    else if (TransferType == FMSH_NON_SECURE_PCAP_WRITE)
    {
		/*printf("FMSH_NON_SECURE_PCAP_WRITE \n");*/
		/*taskDelay(10);*/
		
        if (FMSH_ReadReg(FPS_CSU_BASEADDR, 0x18) & 0x200)  /* bit9*/
        {    
			/*printf("FDevcPs_Prog_B \n");*/
			/*taskDelay(10);*/
			
        	FDevcPs_Prog_B(devcDev);  /* reset pl*/
        }
		
        FDevcPs_rxDataSwap(devcDev, byte_swap);   /* for vx, uboot tftp, */
        /*FDevcPs_rxDataSwap(devcDev, none_swap); */
		
        FDevcPs_downloadMode(devcDev, DOWNLOAD_BITSTREAM);
		
        FDevcPs_writeFifoThre(devcDev, writeFifoThre_hex_0x80);
    } 
    else if(TransferType == FMSH_PCAP_LOOPBACK) 
    {
        FDevcPs_rxDataSwap(devcDev, byte_swap);
        FDevcPs_txDataSwap(devcDev, byte_swap); 
        FDevcPs_readbackDummyCount(devcDev, dummy_4);/*default 3*/
        FDevcPs_downloadMode(devcDev, DATA_LOOPBACK);      
    }
    else
        return FMSH_FAILURE;

    return FMSH_SUCCESS;
}


/******************************************************************************/
/**
*
* This function polls FPGA done
*
* @param	devcDev is devc handle.
* @param	maxcount is the max poll times
*
* @return              
*		- 0 if successful
*		- 1 if unsuccessful
*
* @note		None.
*
****************************************************************************/
u8 FDevcPs_pollFpgaDone2(FDevcPs_T *devcDev, u32 maxcount)
{
    FMSH_devc_portmap_TypeDef *portmap;

    /*int count = maxcount;*/
    int count = 0xFFF;
	
    u32 reg = 0;
    portmap = (FMSH_devc_portmap_TypeDef *)devcDev->base_address; 
    u8 i=0;

	#if 1
	    reg = DEVC_IN32P(portmap->sac_status);
	    while (FMSH_BIT_GET(reg, DEVC_STATUS_pcfg_done) != 1)  /* sac_status:0x18 => bfoDEVC_STATUS_pcfg_done -> 9*/
	    {
	        reg = DEVC_IN32P(portmap->sac_status);
	        count -= 1;

			/*printf("no DEVC_STATUS_pcfg_done,count(0x%X) \n", count);*/
			
	        if (count == 0)
	        {    
				printf("err FDevcPs_pollFpgaDone2: timeout=0x%X, count=0x%X \n", (0xFFF-count), count);
	        	return FMSH_FAILURE;
	        }
	    }
		printf("FDevcPs_pollFpgaDone2: timeout=0x%X, count=0x%X \n", (0xFFF-count), count);
		/*taskDelay(180);*/
		
	#else	
		taskDelay(120);	
	#endif
       
#if 1
    u32 Dummy[16]={0};

    Dummy[i++] = 0x20000000;
    Dummy[i++] = 0x20000000;
    Dummy[i++] = 0x20000000;   /*Type 1 Write 1 Word to CMD*/
    Dummy[i++] = 0x20000000;   /*DESYNC Command*/
    Dummy[i++] = 0x20000000;   /*NOOP*/
    Dummy[i++] = 0x20000000;   /*NOOP*/
    Dummy[i++] = 0x20000000;   /*NOOP*/
    Dummy[i++] = 0x20000000;   /*NOOP*/
    Dummy[i++] = 0x20000000;   /*NOOP*/
    Dummy[i++] = 0x20000000;   /*NOOP*/
    Dummy[i++] = 0x20000000;   /*NOOP*/
    Dummy[i++] = 0x20000000;   /*NOOP*/
    
    FDevcPs_transfer(devcDev, Dummy, 12, (u8 *)FMSH_DMA_INVALID_ADDRESS, 0, FMSH_NON_SECURE_PCAP_WRITE_DUMMMY); 
#endif


    FMSH_WriteReg(FPS_SLCR_BASEADDR, 0x008, 0xDF0D767B);/* unlock SLCR */

    FMSH_WriteReg(FPS_SLCR_BASEADDR , 0x838, 0xF);
    
    /*enable HP interface*/
    reg=FMSH_ReadReg(DDR_UMC_REG_BASE,0x490);
    if(0x1==reg)
        FMSH_WriteReg(DDR_UMC_REG_BASE , 0x490, 0x1); 
	
    reg=FMSH_ReadReg(DDR_UMC_REG_BASE,0x540);
    if(0x1==reg)
        FMSH_WriteReg(DDR_UMC_REG_BASE , 0x540, 0x1); 
	
    reg=FMSH_ReadReg(DDR_UMC_REG_BASE,0x5f0);
    if(0x1==reg)
        FMSH_WriteReg(DDR_UMC_REG_BASE , 0x5f0, 0x1); 
	
    reg=FMSH_ReadReg(DDR_UMC_REG_BASE,0x6a0);
    if(0x1==reg)
        FMSH_WriteReg(DDR_UMC_REG_BASE , 0x6a0, 0x1); 

	
    FMSH_WriteReg(FPS_SLCR_BASEADDR,0x004, 0xDF0D767B);/* relock SLCR */
        
    return FMSH_SUCCESS;
}

u8 FDevcPs_transfer2(FDevcPs_T *devcDev,
                      u32 SourceAddr, 
                      u32 SrcWordLength, 
                      u32 DestAddr, 
                      u32 DestWordLength, 
                      u32 TransferType)
{ 
    u32 reg = 0;
    FMSH_devc_portmap_TypeDef *portmap;

    portmap = (FMSH_devc_portmap_TypeDef *)devcDev->base_address; 

    /*check if DMA command queue is full*/
    /*Read the PCAP sts register for DMA sts*/
    reg = DEVC_IN32P(portmap->sac_status);
    if(FMSH_BIT_GET(reg, DEVC_STATUS_dma_busy) == 1)  /* bfoDEVC_STATUS_dma_busy -> 31*/
        return FMSH_FAILURE;

    /*Check whether the fabric is in initialized state*/
    reg = DEVC_IN32P(portmap->sac_status);
    if(FMSH_BIT_GET(reg, DEVC_STATUS_pcfg_init) != 1) /* bfoDEVC_STATUS_pcfg_init -> 4*/
        return FMSH_FAILURE;
        
    /* We don't need to check PCFG_INIT to be high for non-encrypted loopback transfers*/
	if ((TransferType == FMSH_SECURE_PCAP_WRITE) ||
		(TransferType == FMSH_NON_SECURE_PCAP_WRITE)) 
    {  
        /*Check for valid source pointer and length*/
        if ((!SourceAddr) || (SrcWordLength == 0))
                return FMSH_FAILURE;
        
        if (TransferType == FMSH_NON_SECURE_PCAP_WRITE)
                FDevcPs_downloadMode(devcDev, DOWNLOAD_BITSTREAM);/*set the mode:download bitstream*/

        if (TransferType == FMSH_SECURE_PCAP_WRITE)      
                FDevcPs_downloadMode(devcDev, SECURE_DOWNLOAD_BITSTREAM);/*set sec_down mode*/

        FDevcPs_RDWR_B_LOW(devcDev);
        FDevcPs_CSI_B_LOW(devcDev);

		/*portmap->sac_cfg: 0xC0007C4A */
#if 1	
		u32 reg;	 
		FMSH_devc_portmap_TypeDef *portmap;    
		portmap = (FMSH_devc_portmap_TypeDef *)devcDev->base_address;	  
		reg = DEVC_INP(portmap->sac_cfg); 
		printf("2->portmap->sac_cfg: 0x%08X \n", reg);
		
		reg = DEVC_INP(portmap->sac_status); 
		printf("2->portmap->sac_status: 0x%08X \n", reg);
#endif
		
        FDevcPs_initiateDma2(devcDev, (u32)SourceAddr, (u32)DestAddr, SrcWordLength, DestWordLength);
    }
    else if (TransferType == FMSH_PCAP_READBACK)
    {
        if ((!DestAddr) || (DestWordLength == 0)) 
            return FMSH_FAILURE;

        /*Send READ Frame command to FPGA*/
        /*set the mode:download */
        FDevcPs_downloadMode(devcDev, DOWNLOAD_BITSTREAM);/*set the mode:download bitstream*/

        FDevcPs_RDWR_B_LOW(devcDev);
        FDevcPs_CSI_B_LOW(devcDev);

        FDevcPs_initiateDma2(devcDev, (u32)SourceAddr, PCAP_RD_DATA_ADDR, SrcWordLength, 0);
        
        FDevcPs_CSI_B_HIGH(devcDev);         

        FDevcPs_downloadMode(devcDev, READBACK_BITSTREAM);  /* set the mode:readback bitstream mode*/
        
        /*Initiate the DMA write command.*/
        FDevcPs_initiateDma_readback(devcDev, PCAP_WR_DATA_ADDR, (u32)DestAddr, 0, DestWordLength);
    }
    else if (TransferType == FMSH_NON_SECURE_PCAP_WRITE_DUMMMY) 
    {
        if ((!SourceAddr) || (SrcWordLength == 0))
            return FMSH_FAILURE;
        
        FDevcPs_downloadMode(devcDev, DOWNLOAD_BITSTREAM);/*set the mode:download bitstream*/

        FDevcPs_CSI_B_HIGH(devcDev);
        FDevcPs_RDWR_B_LOW(devcDev);
		
        FDevcPs_initiateDma2(devcDev, (u32)SourceAddr, (u32)DestAddr, SrcWordLength, DestWordLength);
   }

    return FMSH_SUCCESS;
}



/****************************************************************************/
/**
*
* This function starts the DMA transfer. This function only starts the
* operation and returns before the operation may be completed.
* If the interrupt is enabled, an interrupt will be generated when the
* operation is completed, otherwise it is necessary to poll the Status register
* to determine when it is completed. It is the responsibility of the caller to
* determine when the operation is completed by handling the generated interrupt
* or polling the Status Register.
*
* @param	devcDev is devc handle.
* @param	SourceDataPtr contains a pointer to the source memory where the data
*		is to be transferred from.
* @param	SourceLength is the number of words (32 bit) to be transferred
*		for the source transfer.
* @param	DestinationDataPtr contains a pointer to the destination memory
*		where the data is to be transferred to.
* @param	DestinationLength is the number of words (32 bit) to be transferred
*		for the Destination transfer.
* @param	SecureTransfer 
*           FMSH_NON_SECURE_PCAP_WRITE		0
*           FMSH_SECURE_PCAP_WRITE			1
*           FMSH_PCAP_READBACK			2
*
* @return              
*		- 0 if successful
*		- 1 if unsuccessful
*
* @note		None.
*
*****************************************************************************/
u8 FDevcPs_pcapLoadPartition2(FDevcPs_T *devcDev, 
                               u32 SourceDataAddr, 
                               u32 DestinationDataAddr, 
                               u32 SourceLength, 
                               u32 DestinationLength, 
                               u32 SecureTransfer)
{
    u32 sts;
    u32 reg = 0;
    FMSH_devc_portmap_TypeDef *portmap;

    portmap = (FMSH_devc_portmap_TypeDef *)devcDev->base_address; 

#if 1
    sts = FDevcPs_clearPcapStatus(devcDev);
    if (FMSH_SUCCESS != sts)
        return FMSH_FAILURE;
#endif

#if 1
	u32 Dummy[16]={0};
	int i = 0;
	
	Dummy[i++] = 0x20000000;
	Dummy[i++] = 0x20000000;
	Dummy[i++] = 0x20000000;   /*Type 1 Write 1 Word to CMD*/
	Dummy[i++] = 0x20000000;   /*DESYNC Command*/
	Dummy[i++] = 0x20000000;   /*NOOP*/
	Dummy[i++] = 0x20000000;   /*NOOP*/
	Dummy[i++] = 0x20000000;   /*NOOP*/
	Dummy[i++] = 0x20000000;   /*NOOP*/
	Dummy[i++] = 0x20000000;   /*NOOP*/
	Dummy[i++] = 0x20000000;   /*NOOP*/
	Dummy[i++] = 0x20000000;   /*NOOP*/
	Dummy[i++] = 0x20000000;   /*NOOP*/
	
	FDevcPs_transfer2(devcDev, Dummy, 12, (u8 *)FMSH_DMA_INVALID_ADDRESS, 0, FMSH_NON_SECURE_PCAP_WRITE_DUMMMY); 
#endif


    sts = FDevcPs_transfer2(devcDev, SourceDataAddr,
						    SourceLength,
						    DestinationDataAddr,
						    DestinationLength, SecureTransfer);
    if (FMSH_SUCCESS != sts)
    {    
    	return FMSH_FAILURE;
    }
	else
	{
		/*printf("FDevcPs_transfer(0x%08X - len: 0x%08X) ok \n\n", SourceDataAddr, SourceLength);*/
	}

    FDevcPs_CSI_B_HIGH(devcDev);     
    FDevcPs_RDWR_B_HIGH(devcDev);
	
	printf("FDevcPs_transfer(0x%08X - len: 0x%08X) ok \n\n", SourceDataAddr, SourceLength);

    /*Check for errors*/
	reg = DEVC_IN32P(portmap->sac_int_sts);
    if (reg & FMSH_DEVC_ERROR_FLAGS_MASK)  /* 0x00148040*/
    {    
		printf("Check sac_int_sts for errors-> fail! \n\n");
    	return FMSH_FAILURE;
    }
	else
	{
		printf("Check sac_int_sts for errors-> OK! \n\n");
	}
/*    
*/

    return FMSH_SUCCESS;
}

u8 FDevcPs_download_non_secure2_pre(u32 srcAddress,u32 len)
{
    u8 ret=FMSH_SUCCESS;
	
    FDevcPs_T devcDev;
    /*u8 *rcPtr;*/
    /*srcPtr = (u8*srcAddress;*/
    u32 srcAddr;
	
    srcAddr = (u32)srcAddress;

	/*
	0xE004_0000～0xE004_0FFF	 4KB	CSU(DEVC)	AHB_SUBSYS
	*/
    FDevcPs_init2(&devcDev, 0xE0040000);
	
    /*printf(" start: FDevcPs_readFifoThre()... \n");*/
    /*FDevcPs_readFifoThre2(&devcDev, (enum readFifoThre)0);  //  readFifoThre_hex_0x40 = 0x0,//default*/
    FDevcPs_readFifoThre2(&devcDev, (enum readFifoThre)readFifoThre_hex_0x40);  /*  readFifoThre_hex_0x40 = 0x0,//default*/
    printf(" end: FDevcPs_readFifoThre()!!! \n\n");

	/*taskDelay(10);*/
	
   /* printf(" start: FDevcPs_fabricInit()... \n");*/
	/*taskDelay(10);*/
    FDevcPs_fabricInit2(&devcDev, FMSH_NON_SECURE_PCAP_WRITE);
	/*taskDelay(10);*/
    printf(" end: FDevcPs_fabricInit()!!! \n\n");
	
	/*taskDelay(20);     */
     {
     	printf("srcAddr: 0x%08X, len-0x%X \n", srcAddr, len);
		
        ret = FDevcPs_pcapLoadPartition2(&devcDev, (u32)srcAddr, (u32)PCAP_WR_DATA_ADDR, 
                                        len, len, DOWNLOAD_BITSTREAM);
        if (FMSH_SUCCESS != ret)
        {
            return ret;
        }
		else
		{		
			printf(" end: FDevcPs_pcapLoadPartition()!!! \n");
		}
     }
    	
    return FMSH_SUCCESS;
}

int g_first_load_fail = 0;

u8 FDevcPs_download_non_secure2(u32 srcAddress,u32 len)
{
    u8 ret=FMSH_SUCCESS;
	
    FDevcPs_T devcDev;
    /*u8 *rcPtr;*/
    /*srcPtr = (u8*srcAddress;*/
    u32 srcAddr;
	
    srcAddr = (u32)srcAddress;

	/*
	0xE004_0000～0xE004_0FFF	 4KB	CSU(DEVC)	AHB_SUBSYS
	*/
    FDevcPs_init2(&devcDev, 0xE0040000);
	
    /*printf(" start: FDevcPs_readFifoThre()... \n");*/
    /*FDevcPs_readFifoThre2(&devcDev, (enum readFifoThre)0);  //  readFifoThre_hex_0x40 = 0x0,//default*/
    FDevcPs_readFifoThre2(&devcDev, (enum readFifoThre)readFifoThre_hex_0x40);  /*  readFifoThre_hex_0x40 = 0x0,//default*/
    printf(" end: FDevcPs_readFifoThre()!!! \n\n");

	/*taskDelay(10);*/
	
   /* printf(" start: FDevcPs_fabricInit()... \n");*/
	/*taskDelay(10);*/
    FDevcPs_fabricInit2(&devcDev, FMSH_NON_SECURE_PCAP_WRITE);
	/*taskDelay(10);*/
    printf(" end: FDevcPs_fabricInit()!!! \n\n");
	
	/*taskDelay(20);*/
	int try_cnt = 5;
     do 
     {
     	printf("srcAddr: 0x%08X, len-0x%X \n", srcAddr, len);
		
        ret = FDevcPs_pcapLoadPartition2(&devcDev, (u32)srcAddr, (u32)PCAP_WR_DATA_ADDR, 
                                        len, len, DOWNLOAD_BITSTREAM);
        if (FMSH_SUCCESS != ret)
        {
			printf("err end: FDevcPs_pcapLoadPartition()!!! \n\n\n");
            return ret;
        }
		else
		{		
			printf(" end: FDevcPs_pcapLoadPartition():%d! \n", try_cnt);
		}		
    
	    ret = FDevcPs_pollFpgaDone2(&devcDev, DEVC_MAX_COUNT); 
	    if (FMSH_SUCCESS != ret)
	    {
		   /*g_first_load_fail = 1;*/
	       /*return ret;*/
	       try_cnt--;
		   
		   if (try_cnt == 0)
		   	{
		   		return ret;
		   	}
	    }
		else
		{		
			try_cnt = 0;
			printf(" end: FDevcPs_pollFpgaDone()!!! \n");
		}		
     }while (try_cnt > 0);
	
    return FMSH_SUCCESS;
}


void test_load_pl_pre(u32 loadAddr, u32 size)
{
	u8 freqFlag=0;
	u32 Status = 0;
	
	slcr_write(0x838, 0x0);
	/*slcr_write(0x83C, 0x0);*/
	taskDelay(1);
	
	FmshFsbl_OpenCfgLevelShifter2();  /* cdg_level_shift 打开*/
	taskDelay(10);


	/*Status = FDevcPs_devcInitDownload2();*/
	/*taskDelay(10);	*/
	Status = FDevcPs_devcInitDownload2();
	if (Status != FMSH_SUCCESS)
	{
		printf("\nFDevcPs_devcInitDownload2: clear old bitstream fail! \n\n");
		return;
	}
	else
	{
		printf("\nFDevcPs_devcInitDownload2: clear old bitstream OK! \n\n");
	}
	
	Status = FDevcPs_adjustFreq(&freqFlag);  /* 降频率*/
	/*Status = FDevcPs_adjustFreq(&freqFlag);  // 降频率*/
	/*Status = FDevcPs_adjustFreq2(&freqFlag);  // 降频率*/
	if (Status != FMSH_SUCCESS)
	{
		printf("FDevcPs_adjustFreq: reduce & down freq fail! \n\n");
		return;
	}
	else
	{
		clk_cpu_show();
		printf(" down cpu freq: freqFlag=%d \n\n", freqFlag);
	}
	
	/*taskDelay(20);*/
	
	Status= FDevcPs_download_non_secure2_pre(loadAddr,size);
	
	/*taskDelay(20);*/
/*	
	if (freqFlag != 0)
	{  
		FDevcPs_recoverFreq();  // 升频率
		//FDevcPs_recoverFreq2();  // 升频率
		printf("FDevcPs_recoverFreq: add & up freq! \n\n");
	}
	else
	{
		clk_cpu_show();
		printf(" up cpu freq: \n\n");
	}
*/
	return;
}

void test_load_pl(u32 loadAddr, u32 size)
{
	u8 freqFlag=0;
	u32 Status = 0;
	u32 loadAddr_old = loadAddr;
	u32 size_old = size;
	
	slcr_write(0x838, 0x0);
	/*slcr_write(0x83C, 0x0);*/
	taskDelay(1);
	
	
	FmshFsbl_OpenCfgLevelShifter2();  /* cdg_level_shift 打开*/
	taskDelay(10);


	/*Status = FDevcPs_devcInitDownload2();*/
	/*taskDelay(10);	*/
#if 0
	Status = FDevcPs_devcInitDownload2();
	if (Status != FMSH_SUCCESS)
	{
		printf("\nFDevcPs_devcInitDownload2: clear old bitstream fail! \n\n");
		return;
	}
	else
	{
		printf("\nFDevcPs_devcInitDownload2: clear old bitstream OK! \n\n");
	}

	taskDelay(30);
#endif
	
	Status = FDevcPs_adjustFreq(&freqFlag);  /* 降频率*/
	/*Status = FDevcPs_adjustFreq(&freqFlag);  // 降频率*/
	/*Status = FDevcPs_adjustFreq2(&freqFlag);  // 降频率*/
	if (Status != FMSH_SUCCESS)
	{
		printf("FDevcPs_adjustFreq: reduce & down freq fail! \n\n");
		return;
	}
	else
	{
		clk_cpu_show();
		printf(" down cpu freq: freqFlag=%d \n\n", freqFlag);
	}
	
	/*taskDelay(20);*/
	
	Status= FDevcPs_download_non_secure2(loadAddr,size);
		
	/*taskDelay(20);*/
	
	if (freqFlag != 0)
	{  
		FDevcPs_recoverFreq();  /* 升频率*/
		/*FDevcPs_recoverFreq2();  // 升频率*/
		printf("FDevcPs_recoverFreq: add & up freq! \n\n");
	}
	else
	{
		clk_cpu_show();
		printf(" up cpu freq: \n\n");
	}
	
	if (g_first_load_fail == 1)
	{
		taskDelay(60);
		test_load_pl2(loadAddr_old, size_old);
		
		g_first_load_fail = 0;
	}

	return;
}

void test_load_pl2(u32 loadAddr, u32 size)
{
	u8 freqFlag=0;
	u32 Status = 0;
	
	slcr_write(0x838, 0x0);
	/*slcr_write(0x83C, 0x0);*/
	taskDelay(1);
	
	
	FmshFsbl_OpenCfgLevelShifter2();  /* cdg_level_shift 打开*/
	taskDelay(10);


	/*Status = FDevcPs_devcInitDownload2();*/
	/*taskDelay(10);	*/
	Status = FDevcPs_devcInitDownload2();
	if (Status != FMSH_SUCCESS)
	{
		printf("\nFDevcPs_devcInitDownload2: clear old bitstream fail! \n\n");
		return;
	}
	else
	{
		printf("\nFDevcPs_devcInitDownload2: clear old bitstream OK! \n\n");
	}

	taskDelay(30);
	
	Status = FDevcPs_adjustFreq(&freqFlag);  /* 降频率*/
	/*Status = FDevcPs_adjustFreq(&freqFlag);  // 降频率*/
	/*Status = FDevcPs_adjustFreq2(&freqFlag);  // 降频率*/
	if (Status != FMSH_SUCCESS)
	{
		printf("FDevcPs_adjustFreq: reduce & down freq fail! \n\n");
		return;
	}
	else
	{
		clk_cpu_show();
		printf(" down cpu freq: freqFlag=%d \n\n", freqFlag);
	}
	
	/*taskDelay(20);*/
	
	Status= FDevcPs_download_non_secure2(loadAddr,size);
	
	/*taskDelay(20);*/
	
	if (freqFlag != 0)
	{  
		FDevcPs_recoverFreq();  /* 升频率*/
		/*FDevcPs_recoverFreq2();  // 升频率*/
		printf("FDevcPs_recoverFreq: add & up freq! \n\n");
	}
	else
	{
		clk_cpu_show();
		printf(" up cpu freq: \n\n");
	}

	return;
}

void test_cpu_freq_down(void)
{
	u8 freqFlag=0;
	u32 Status = 0;
	
	FmshFsbl_OpenCfgLevelShifter2();  /* cdg_level_shift 打开*/
	
	Status = FDevcPs_adjustFreq(&freqFlag);  /* 降频率	*/
	if (Status != FMSH_SUCCESS)
	{
		printf("FDevcPs_adjustFreq: reduce & down freq fail! \n\n");
		return;
	}
	else
	{
		printf("FDevcPs_adjustFreq: reduce & down freq OK! \n");		
		clk_cpu_show();
	}
	
	taskDelay(120);
	
	if (freqFlag != 0)
	{  
		FDevcPs_recoverFreq();  /* 升频率*/
		
		printf("FDevcPs_recoverFreq: add & up freq! \n\n");
		clk_cpu_show();
	}
	else
	{
		printf("no FDevcPs_recoverFreq, for FDevcPs_adjustFreq fail! \n\n");
	}

	return;
}

void test_cpu_freq_down2(void)
{
	u8 freqFlag=0;
	u32 Status = 0;
	
	FmshFsbl_OpenCfgLevelShifter2();  /* cdg_level_shift 打开*/
	
	Status = FDevcPs_adjustFreq(&freqFlag);  /* 降频率*/
	if (Status != FMSH_SUCCESS)
	{
		printf("FDevcPs_adjustFreq: reduce & down freq fail! \n\n");
		return;
	}
	else
	{
		printf("FDevcPs_adjustFreq: reduce & down freq OK! \n\n");
	}
	
	clk_cpu_show();
	
	return;
}

void test_cpu_freq_up2(void)
{
	FmshFsbl_OpenCfgLevelShifter2();  /* cdg_level_shift 打开*/

	FDevcPs_recoverFreq();  /* 升频率*/
	printf("FDevcPs_recoverFreq: add & up freq! \n\n");
	
	clk_cpu_show();
	return;
}
#endif

#if 0  /* qspi_0 norflash must be exist*/

#if 0
UINT32 swap_hex32(UINT32 input)
{
#if 0
	UINT32 tmp32 = 0;

	tmp32 = (input & 0x000000FF) << 24;
	tmp32 |= (input & 0x0000FF00) << 8;
	tmp32 |= (input & 0x00FF0000) >> 8;
	tmp32 |= (input & 0xFF000000) >> 24;

	return tmp32;
#else
	return input;
#endif	
}
#endif

#define PL_BIT_FILE_SRC_DDR_ADR   (0x30000000)

/*static UINT8 pl_bit_file[0x10100] = {0};   // 64K + 256*/

int g_vxInit_Qspi_ok = 0;


void test_rd_qspi_bit(void)
{	
	vxT_QSPI * pQspi = g_pQspi0;
	
	vxT_QSPI_CTRL * pQspiCtrl = NULL;
	vxT_QSPI_FLASH * pQspiFlash = NULL;
	
	
	UINT8 * pBuf8 = NULL;
	UINT32 * pBuf32 = NULL;
	
	int j = 0, byteCnt = 0;
	int sect_idx = 0, sect_size = 0;
	
	int i = 0, base_idx = 0;
	
	/*int bit_file_size = 0xCB44C0;  // 0xE00000 14M  Bytes transferred = 13321503 (cb451f hex)*/
	int bit_file_size = 0xCB44BF;  /* 0xE00000 14M  Bytes transferred = 13321503 (cb451f hex)*/
	int bit_file_size2 = 0xcb451f;  /* 0xE00000 14M  Bytes transferred = 13321503 (cb451f hex)*/

#if 0
	pBuf8 = (UINT8 *)(&pl_bit_file[0]);
#else

	#if 0
	/*pBuf8 = (UINT8 *(malloc(0x1000000));  // 16M	*/
	pBuf8 = (UINT8 *)(PL_BIT_FILE_SRC_DDR_ADR);  /* ddr phy_addr: 0x3000_0000*/
	pBuf32 = (UINT32*)(&pBuf8[0]);

	if (g_vxInit_Qspi_ok == 0)
	{
		vxInit_Qspi(0);
		
	
		pQspiCtrl = pQspi->pQspiCtrl;
		pQspiFlash = pQspi->pQspiFlash;

		byteCnt = pQspiFlash->sectSize;	
		sect_size = pQspiFlash->sectSize;	

		printf("sector size: %d \n", pQspiFlash->sectSize);

		/*for (i=0; i<(bit_file_size/sect_size); i++)*/
		for (i=0; i<(0xE00000/sect_size); i++)
		{
			qspiFlash_Read_Sect(pQspi, i, (UINT8*)(&pBuf8[i*sect_size]));
		}

		printf("----qspi(%d)-read(%d):%dKB----\n", pQspiCtrl->ctrl_x, sect_idx, (byteCnt/1024));	
		
		g_vxInit_Qspi_ok = 1;
	}

	#else
	
	pBuf8 = (UINT8 *)(0x20000000);  /* ddr-phy 0x20000000*/
	pBuf32 = (UINT32*)(&pBuf8[0]);
	
	printf("\n--------uboot_tftp-------\n\n");	
	
	#endif
#endif

#if 1

#if 1
	/*=========================================*/
	for (j=0; j<512; j++)
	{
		printf("%02X", pBuf8[j]);
		
		if ((j+1)%4 == 0)
		{
			printf(" ");
		}
		
		if ((j+1)%16 == 0)
		{
			printf(" \n");
		}
		
		if ((j+1)%256 == 0)
		{
			printf(" \n");
		}
	}

	printf(" ... ... ... ... \n\n");
	
	/*for (j=(byteCnt-512); j<byteCnt; j++)*/
	for (j=(bit_file_size-512); j<bit_file_size; j++)
	{
		printf("%02X", pBuf8[j]);
		
		if ((j+1)%4 == 0)
		{
			printf(" ");
		}
		
		if ((j+1)%16 == 0)
		{
			printf(" \n");
		}
		
		if ((j+1)%256 == 0)
		{
			printf(" \n");
		}
	}
	printf("\n ------------\n");
	
	/*=========================================*/
#endif

	/**/
	/* find the head of data area*/
	/**/
	for (i=0; i<256; i++)
	{
		for (j=0; j<32; j++)
		{
			if (pBuf8[i+j] != 0xFF)
			{
				break;  /* for (j=0; j<32; j++)*/
			}
		}

		if (j >= 32)
		{
			base_idx = i;
			break;  /* for (i=0; i<256; i++)*/
		}
	}

	printf("base_idx=%d \n\n", base_idx);

	/**/
	/* move data to idx_0*/
	/**/
	/*for (i=0; i<byteCnt; i++)*/
	#if 0
		for (i=0; i<bit_file_size; i++)
		{
			pBuf8[i] = pBuf8[base_idx + i];
		}
	#else
	
		for (i=0; i<bit_file_size2; i++)
		{
			pBuf8[i] = pBuf8[base_idx + i];
		}
		
		/*extern UINT32 swap_hex32(UINT32 tmp);*/
		
		for (i=0; i<bit_file_size/4; i++)
		{
			pBuf32[i] = swap_hex32_2(pBuf32[i]);
		}
		/*
		*/
	#endif

	
#if 1
	/*=========================================*/
	for (j=0; j<512; j++)
	{
		printf("%02X", pBuf8[j]);
		
		if ((j+1)%4 == 0)
		{
			printf(" ");
		}
		
		if ((j+1)%16 == 0)
		{
			printf(" \n");
		}
		
		if ((j+1)%256 == 0)
		{
			printf(" \n");
		}
	}

	printf(" ... ... ... ... \n\n");
	
	/*for (j=(byteCnt-512); j<byteCnt; j++)*/
	for (j=(bit_file_size-512); j<bit_file_size; j++)
	{
		printf("%02X", pBuf8[j]);
		
		if ((j+1)%4 == 0)
		{
			printf(" ");
		}
		
		if ((j+1)%16 == 0)
		{
			printf(" \n");
		}
		
		if ((j+1)%256 == 0)
		{
			printf(" \n");
		}
	}
	printf("\n-----byte seq-------\n\n");
	/*=========================================*/
#endif

	printf("-------word seq-----\n");
	for (j=0; j<128; j++)
	{
		printf("%08X ", pBuf32[j]);
		
		if ((j+1)%4 == 0)
		{
			printf(" \n");
		}
		
		if ((j+1)%128 == 0)
		{
			printf(" \n");
		}
	}
	printf("======word seq======\n");	
	for (j=(bit_file_size/4)-128; j<(bit_file_size/4); j++)
	{
		printf("%08X ", pBuf32[j]);
		
		if ((j+1)%4 == 0)
		{
			printf(" \n");
		}
		
		if ((j+1)%128 == 0)
		{
			printf(" \n");
		}
	}

	taskDelay(60);
	
#endif

	printf("&pBuf32[0]: 0x%08X len:0x%08X \n\n", &pBuf32[0], (bit_file_size/4));
	
	/*test_load_pl_pre((UINT32)(&pBuf32[0]), (bit_file_size/4));*/
	/*taskDelay(30);*/
	
	test_load_pl((UINT32)(&pBuf32[0]), (bit_file_size/4));
	
	/*
	bitstream_checksum	0x0e0	bitstream_checksum
	csu_error_code_reg	0x0e4	保留
			csu_error_code_reg
	*/
	printf("\n\n");
	printf("bitstream_checksum: 0x%08X \n", *((UINT32*)(0xE0040000+0xE0)));	
	printf("csu_error_code_reg: 0x%08X \n", *((UINT32*)(0xE0040000+0xE4)));
	printf("\n\n");

	return;
}


#endif

void devc_show(int offset)
{
	UINT32 addr = 0xE0040000 + offset;

	printf("devc_reg(0x%X)= 0x%08X \n", addr, *((UINT32*)addr));
	return;
}

void devc_show2(void)
{
	int offset = 0;
	
	UINT32 addr = 0xE0040000;

	for (offset=0x08; offset<=0x2C; offset+=4)
	{
		addr = 0xE0040000 + offset;
		printf("devc_reg(0x%X)= 0x%08X \n", addr, *((UINT32*)addr));
	}
	
	return;
}

