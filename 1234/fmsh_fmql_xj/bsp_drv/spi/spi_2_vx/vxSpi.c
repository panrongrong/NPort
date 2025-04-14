/* vxSpi.c - fmsh 7020/7045 pSpi driver */

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
01a, 20Dec19, jc  written.
*/

/*
DESCRIPTION
	no vxbus struct
*/

/* includes */

#include <vxWorks.h>
#include <stdio.h>
#include <semLib.h>
#include <sysLib.h>
#include <taskLib.h>
#include <vxBusLib.h>
#include <cacheLib.h>
#include <string.h>
#include <vxbTimerLib.h>

#include "vxSpi.h"

/*
defines 
*/
#if 1

#undef VX_DBG
#undef VX_DBG2

#define SPI_DBG

#ifdef SPI_DBG

#define VX_DBG(string, a, b, c, d, e, f)                    \
        if (_func_logMsg != NULL)                           \
           printf(string, a, b, c, d, e, f)  /* (* _func_logMsg)(string, a, b, c, d, e, f) */
#else
#define VX_DBG(string, a, b, c, d, e, f)
#endif

/* error info log */
#define VX_DBG2(string, a, b, c, d, e, f) printf(string, a, b, c, d, e, f)
#endif

extern void sysUsDelay(int);

/* 
spi_0 : master
*/
static vxT_SPI_CTRL vxSpi_Ctrl_0 = {0};

static vxT_SPI vx_pSpi_0 = {0};
vxT_SPI * g_pSpi0 = (vxT_SPI *)(&vx_pSpi_0.spi_x);

/* 
spi_1 : slave
*/
static vxT_SPI_CTRL vxSpi_Ctrl_1 = {0};

static vxT_SPI vx_pSpi_1 = {0};
vxT_SPI * g_pSpi1 = (vxT_SPI *)(&vx_pSpi_1.spi_x);


#if 1

void spiCtrl_Wr_CfgReg32(vxT_SPI* pSpi, UINT32 offset, UINT32 value)
{
	UINT32 tmp32 = pSpi->pSpiCtrl->cfgBaseAddr;
	FMQL_WRITE_32((tmp32 + offset), value);
	return;
}

UINT32 spiCtrl_Rd_CfgReg32(vxT_SPI* pSpi, UINT32 offset)
{
	UINT32 tmp32 = pSpi->pSpiCtrl->cfgBaseAddr;
	return FMQL_READ_32(tmp32 + offset);
}

void spiCtrl_Reset(vxT_SPI* pSpi, int ctrl_x)
{  
	UINT32 tmp32 = 0;
	
    switch (ctrl_x)
    {
    case SPI_CTRL_0:
        tmp32 = slcr_read(SLCR_SPI0_CTRL) | (SPI0_APB_RST_MASK | SPI0_REF_RST_MASK);
		slcr_write(SLCR_SPI0_CTRL, tmp32);
		delay_ms(1);	
        tmp32 = slcr_read(SLCR_SPI0_CTRL) & (~(SPI0_APB_RST_MASK | SPI0_REF_RST_MASK));
		slcr_write(SLCR_SPI0_CTRL, tmp32);
		break;
		
    case SPI_CTRL_1:
        tmp32 = slcr_read(SLCR_SPI1_CTRL) | (SPI1_APB_RST_MASK | SPI1_REF_RST_MASK);
		slcr_write(SLCR_SPI1_CTRL, tmp32);
		delay_ms(1);	
        tmp32 = slcr_read(SLCR_SPI1_CTRL) & (~(SPI1_APB_RST_MASK | SPI1_REF_RST_MASK));
		slcr_write(SLCR_SPI1_CTRL, tmp32);
		break;
    }
	
	return;
}


void spiCtrl_Disable(vxT_SPI* pSpi)
{
	vxT_SPI_CTRL * pSpiCtrl = pSpi->pSpiCtrl;
	
    spiCtrl_Wr_CfgReg32(pSpi, SPIPS_SSIENR_OFFSET, 0x0);
	return;
}


void spiCtrl_Set_Master(vxT_SPI* pSpi)
{
	vxT_SPI_CTRL * pSpiCtrl = pSpi->pSpiCtrl;
	
    spiCtrl_Wr_CfgReg32(pSpi, SPIPS_MSTR_OFFSET, 0x1);
    pSpiCtrl->isMaster = TRUE;
	
	return;
}

int spiCtrl_Set_Baud_Mst(vxT_SPI* pSpi, UINT32 sck_dv)
{
	vxT_SPI_CTRL * pSpiCtrl = pSpi->pSpiCtrl;
	
    if (pSpiCtrl->isEnable == TRUE || pSpiCtrl->isMaster == FALSE)
    {
        return FMSH_FAILURE;
    }
	
    /*
	Fsclk_out = Fssi_clk/SCKDV
	*/
    spiCtrl_Wr_CfgReg32(pSpi, SPIPS_BAUDR_OFFSET, sck_dv);  
    
    return FMSH_SUCCESS;
}

void spiCtrl_Set_Slave(vxT_SPI* pSpi)
{
	vxT_SPI_CTRL * pSpiCtrl = pSpi->pSpiCtrl;
	
    spiCtrl_Wr_CfgReg32(pSpi, SPIPS_MSTR_OFFSET, 0x0);
    pSpiCtrl->isMaster = FALSE;
	
	return;
}

/*
仅当SSI配置从机操作时才有效；该位会禁止/使能ssi_oe_n信号，当SLV_OE=1时，ssi_oe_n信号不会被激活，
所以由ssi_oe_n信号控制的tri-state buffer 在txd输出上永远呈现高阻。
当主设备以广播方式传输时，如果用户不希望从机回应，可以采用该配置。
	0：Slave txd is enabled
	1：Slave txd is disabled
	复位值：0x0
*/
int spiCtrl_Set_enSlvTxd(vxT_SPI* pSpi, BOOL enable)
{
	vxT_SPI_CTRL * pSpiCtrl = pSpi->pSpiCtrl;
	
    UINT32 cfgReg = 0;
    
    if (pSpiCtrl->isEnable == TRUE)
    {
        return FMSH_FAILURE;
    }  
	
    cfgReg = spiCtrl_Rd_CfgReg32(pSpi, SPIPS_CTRLR0_OFFSET);
    if (enable) 
    {
        cfgReg &= ~SPIPS_CTRL0_SLVOE_MASK;  /* 0：Slave txd is enabled */
    }
    else 
    {
        cfgReg |= SPIPS_CTRL0_SLVOE_MASK;   /* 1：Slave txd is disabled */
    }
	
    spiCtrl_Wr_CfgReg32(pSpi, SPIPS_CTRLR0_OFFSET, cfgReg);  
    
    return FMSH_SUCCESS;
}

int spiCtrl_Set_XfrMode(vxT_SPI* pSpi, UINT32 xfr_mode)
{
	vxT_SPI_CTRL * pSpiCtrl = pSpi->pSpiCtrl;
	
    UINT32 cfgReg = 0;
    
    if (pSpiCtrl->isEnable == TRUE)
    {
        return FMSH_FAILURE;
    }   
	
    if (xfr_mode > 3)
    {
        return FMSH_FAILURE;
    }
    
    cfgReg = spiCtrl_Rd_CfgReg32(pSpi, SPIPS_CTRLR0_OFFSET);
	
    cfgReg &= ~SPIPS_CTRL0_TMOD_MASK;
    cfgReg |= (xfr_mode << SPIPS_CTRL0_TMOD_SHIFT);
	
    spiCtrl_Wr_CfgReg32(pSpi, SPIPS_CTRLR0_OFFSET, cfgReg); 
    
    return FMSH_SUCCESS;
}

int spiCtrl_Set_SckMode(vxT_SPI* pSpi, UINT32 sck_mode)
{
	vxT_SPI_CTRL * pSpiCtrl = pSpi->pSpiCtrl;
	
    UINT32 cfgReg = 0;
    
    if(pSpiCtrl->isEnable == TRUE)
    {
        return FMSH_FAILURE;
    }
	
    if (sck_mode > 3)
    {
        return FMSH_FAILURE;
    }
	
    cfgReg = spiCtrl_Rd_CfgReg32(pSpi, SPIPS_CTRLR0_OFFSET);
	
    cfgReg &= ~(SPIPS_CTRL0_SCPH_MASK | SPIPS_CTRL0_SCPOL_MASK);
    cfgReg |= (sck_mode << SPIPS_CTRL0_SCPH_SHIFT);
	
    spiCtrl_Wr_CfgReg32(pSpi, SPIPS_CTRLR0_OFFSET, cfgReg); 
    
    return FMSH_SUCCESS;
}

/*
用来选择传输所用数据帧的长度
bit20~16:	DFS_32, DFS_32字段数值+1为设置的数据帧长度

*/
int spiCtrl_Set_FrmLen(vxT_SPI* pSpi, UINT32 dfs32)
{
	vxT_SPI_CTRL * pSpiCtrl = pSpi->pSpiCtrl;
	
    UINT32 cfgReg = 0;
    
    if (pSpiCtrl->isEnable == TRUE)
    {
        return FMSH_FAILURE;
    }
	
    if ((dfs32 < 4) || (dfs32 > 0x20))
    {
        return FMSH_FAILURE;
    }
    
    cfgReg = spiCtrl_Rd_CfgReg32(pSpi, SPIPS_CTRLR0_OFFSET);
	
    cfgReg &= ~SPIPS_CTRL0_DFS32_MASK;
    cfgReg |= ((dfs32 - 1) << SPIPS_CTRL0_DFS32_SHIFT);
	
    spiCtrl_Wr_CfgReg32(pSpi, SPIPS_CTRLR0_OFFSET, cfgReg); 
    
    pSpiCtrl->frmSize = dfs32;
    
    return FMSH_SUCCESS;
}

/*
bit11	SRL:
	0：正常模式操作
	1：测试模式操作, loopback
*/
int spiCtrl_Set_LoopBack(vxT_SPI* pSpi, BOOL enable)
{
	vxT_SPI_CTRL * pSpiCtrl = pSpi->pSpiCtrl;
	
    UINT32 cfgReg = 0;
    
    if (pSpiCtrl->isEnable == TRUE)
    {
        return FMSH_FAILURE;
    }    
	
	/* CTRLR0 bit11: SRL  */
    cfgReg = spiCtrl_Rd_CfgReg32(pSpi, SPIPS_CTRLR0_OFFSET);
	
    if (enable)  
    {
        cfgReg |= SPIPS_CTRL0_SRL_MASK;  
    }
    else  
    {
        cfgReg &= ~SPIPS_CTRL0_SRL_MASK;
    }
	
    spiCtrl_Wr_CfgReg32(pSpi, SPIPS_CTRLR0_OFFSET, cfgReg); 
    
    return FMSH_SUCCESS;
}


/*
接收数据帧的数量
*/
int spiCtrl_Set_RcvFrmNum(vxT_SPI* pSpi, UINT32 dfNum)
{
	vxT_SPI_CTRL * pSpiCtrl = pSpi->pSpiCtrl;
	
    if (pSpiCtrl->isEnable == TRUE || pSpiCtrl->isMaster == FALSE)
    {
        return FMSH_FAILURE;
    }
	
    spiCtrl_Wr_CfgReg32(pSpi, SPIPS_CTRLR1_OFFSET, (dfNum-1));
    
    return FMSH_SUCCESS;
}


/*
发送FIFO阈值；当TX FIFO中数据个数小于或等于阈值时，触发发送FIFO空中断。
*/
int spiCtrl_Set_TxFiFoEmptyLvl(vxT_SPI* pSpi, UINT8 tx_lvl)
{
	vxT_SPI_CTRL * pSpiCtrl = pSpi->pSpiCtrl;
	
    if (pSpiCtrl->isEnable == TRUE)
    {
        return FMSH_FAILURE;
    }    
	
    spiCtrl_Wr_CfgReg32(pSpi, SPIPS_TXFTLR_OFFSET, tx_lvl);
    
    return FMSH_SUCCESS;
}

int spiCtrl_Set_RxFifoFullLvl(vxT_SPI* pSpi, UINT8 rx_lvl)
{
	vxT_SPI_CTRL * pSpiCtrl = pSpi->pSpiCtrl;
	
    if (pSpiCtrl->isEnable == TRUE)
    {
        return FMSH_FAILURE;
    }  
	
    spiCtrl_Wr_CfgReg32(pSpi, SPIPS_RXFTLR_OFFSET, rx_lvl);
    
    return FMSH_SUCCESS;
}


/*
DMA发送数据水位值，控制着发送逻辑何时产生DMA请求；
当TX FIFO中数据小于或等于水位值,且TDMAE=1时，产生dma_tx_req；
*/
void spiCtrl_Set_DMATxLvl(vxT_SPI* pSpi, UINT32 tx_lvl)
{
    spiCtrl_Wr_CfgReg32(pSpi, SPIPS_DMATDLR_OFFSET, tx_lvl);
	return;
}

void spiCtrl_Set_DMARxLvl(vxT_SPI* pSpi, UINT32 rx_lvl)
{
    spiCtrl_Wr_CfgReg32(pSpi, SPIPS_DMARDLR_OFFSET, rx_lvl);
	return;
}

/*
IRQ Mask Register
*/
void spiCtrl_DisIRQ(vxT_SPI* pSpi, UINT32 mask)
{
    UINT32 cfgReg = 0;
    
    cfgReg = spiCtrl_Rd_CfgReg32(pSpi, SPIPS_IMR_OFFSET);
    cfgReg &= ~mask;
	
    spiCtrl_Wr_CfgReg32(pSpi, SPIPS_IMR_OFFSET, cfgReg);
	return;
}

/*

从机选择使能标志，该寄存器中每一位都对应着一个从机。
该寄存器仅在SSI主机模块中有效。该寄存器用来选择与SSI模块连接的外设从机
*/
int spiCtrl_Set_PeerSlave(vxT_SPI* pSpi, UINT32 ctrl_x)
{    
	vxT_SPI_CTRL * pSpiCtrl = pSpi->pSpiCtrl;
	
    if (pSpiCtrl->isMaster == FALSE)
    {
        return FMSH_FAILURE;
    }
	
    if (ctrl_x == 0)
    {
        spiCtrl_Wr_CfgReg32(pSpi, SPIPS_SER_OFFSET, 0x0); 
    }
    else
    {
        spiCtrl_Wr_CfgReg32(pSpi, SPIPS_SER_OFFSET, (0x1 << (ctrl_x - 1))); 
    }
    
    return FMSH_SUCCESS;
}


void spiCtrl_Enable(vxT_SPI* pSpi)
{
	vxT_SPI_CTRL * pSpiCtrl = pSpi->pSpiCtrl;
	
    spiCtrl_Wr_CfgReg32(pSpi, SPIPS_SSIENR_OFFSET, 0x1);
    /*pSpiCtrl->isEnable = TRUE;*/
    
    return;
}

#endif

#if 1

void spiSlcr_Set_SpiClk(void)
{
	slcr_write(IO_PLL_CLKOUT3_DIVISOR, 0xF);  /* 15=> 30M *00 / 15 = 200M ref_clk_spi*/
	slcr_write(SPI_CLK_CTRL, 0xF);
	return;
}

/****************************************************************************/
/**
*
*  This function loop SPI0's outputs to SPI1's inputs,and SPI1's outputs to
*  SPI0's inputs
*
* @param
*  loop_en -- 0:connect SPI inputs according to MIO mapping;1:set the loop
*
* @return
*  Null
*
* @note
*  Null
*
****************************************************************************/
void spiSlcr_Set_SpiLoop(UINT32 loop_en)
{
	UINT32 tmp32 = 0;
	
	/*
	I2C0_LOOP_I2C1	3	rw	0x0 I2C Loopback Control.
					0 = Connect I2C inputs according to MIO mapping.
					1 = Loop I2C 0 outputs to I2C 1 inputs, and I2C 1 outputs
					to I2C 0 inputs.
					
	CAN0_LOOP_CAN1	2	rw	0x0 CAN Loopback Control.
					0 = Connect CAN inputs according to MIO mapping.
					1 = Loop CAN 0 Tx to CAN 1 Rx, and CAN 1 Tx to CAN 0 Rx.
					
	UA0_LOOP_UA1	1	rw	0x0 UART Loopback Control.
					0 = Connect UART inputs according to MIO mapping.
					1 = Loop UART 0 outputs to UART 1 inputs, and UART 1 outputs to UART 0 inputs.
					RXD/TXD cross-connected. RTS/CTS cross-connected.
					DSR, DTR, DCD and RI not used.
					
	SPI0_LOOP_SPI1	0	rw	0x0 SPI Loopback Control.
					0 = Connect SPI inputs according to MIO mapping.
					1 = Loop SPI 0 outputs to SPI 1 inputs, and SPI 1 outputs to SPI 0 inputs.
					The other SPI core will appear on the LS Slave Select.
	*/
	
	if (loop_en == 0)
	{
	    /*FSlcrPs_setBitTo0(FPS_SLCR_BASEADDR, SLCR_MIO_LOOPBACK,0);*/
	    tmp32 = slcr_read(SLCR_MIO_LOOPBACK);
		tmp32 &= ~0x01;
		slcr_write(SLCR_MIO_LOOPBACK, tmp32);
		
	}
	else if (loop_en == 1)
	{
	    /*FSlcrPs_setBitTo1(FPS_SLCR_BASEADDR, SLCR_MIO_LOOPBACK,0);*/
		tmp32 = slcr_read(SLCR_MIO_LOOPBACK);
		tmp32 |= 0x01;
		slcr_write(SLCR_MIO_LOOPBACK, tmp32);
    }

	return;
}

#endif

#if 1
int spiCtrl_Init_Master(vxT_SPI* pSpi)
{
	vxT_SPI_CTRL * pSpiCtrl = pSpi->pSpiCtrl;
	
    int ret = 0;
        
    /* 
    Check whether there is another transfer in progress. Not thread-safe 
	*/
    if (pSpiCtrl->isBusy == TRUE) 
    {
        return FMSH_FAILURE;
    }
	else
	{
		pSpiCtrl->isBusy == TRUE;
	}
    
    /* 
    Disable device 
	*/
    spiCtrl_Disable(pSpi);
	
    /* 
    Select device as Master 
	*/
    spiCtrl_Set_Master(pSpi);
	
    /* 
    CTRL (TMode, CkMode, BaudRate, DFSize, DFNum, isLoopBack) 
	*/
    ret |= spiCtrl_Set_XfrMode(pSpi, SPIPS_TRANSFER_STATE);
    ret |= spiCtrl_Set_SckMode(pSpi, SPI_MODE_3);          /* 3 =>  #define SPI_MODE_3  (SPI_CKPOL | SPI_CKPHA) */

    ret |= spiCtrl_Set_Baud_Mst(pSpi, 200);    /* Fsclk_out = Fssi_clk/SCKDV => 40_000_000 / 200 = 200K bps */

#if 0  /* 1 byte/frm*/
    ret |= spiCtrl_Set_FrmLen(pSpi, 8);       /* frame_len: 8bit-1bytes */
#else  /* 4 bytes/frm*/
    ret |= spiCtrl_Set_FrmLen(pSpi, 32);       /* frame_len: 32bit-4bytes */
#endif
	
    ret |= spiCtrl_Set_LoopBack(pSpi, FALSE);  /* normal work mode, no loop */
	
    ret |= spiCtrl_Set_RcvFrmNum(pSpi, 128);   /* rcv frame num */ 
    
    /* 
    Config Tx/Rx Threshold 
	*/
    ret |= spiCtrl_Set_TxFiFoEmptyLvl(pSpi, 8); /* ->24 */
    ret |= spiCtrl_Set_RxFifoFullLvl(pSpi, 0);  /* -> 8 */
	
    spiCtrl_Set_DMATxLvl(pSpi, 8);
    spiCtrl_Set_DMARxLvl(pSpi, 24);
    
    /* 
    Config IMR : disable all IRQ
	*/
    spiCtrl_DisIRQ(pSpi, SPIPS_INTR_ALL);
    
    /*
    SlaveSelect 
	*/
    ret |= spiCtrl_Set_PeerSlave(pSpi, SPI_CTRL_1);  /*  ???  */
    if (ret == FMSH_FAILURE)
    {
        return FMSH_FAILURE;
    }
	
    /* 
    Enable pSpi
    */
    spiCtrl_Enable(pSpi);
    
    return FMSH_SUCCESS;
}

int spiCtrl_Init_Slave(vxT_SPI* pSpi)
{
	vxT_SPI_CTRL * pSpiCtrl = pSpi->pSpiCtrl;
	
    int ret = 0;

    /* 
    Check whether there is another transfer in progress. Not thread-safe 
	*/
    if (pSpiCtrl->isBusy == TRUE) 
    {
        return FMSH_FAILURE;
    }
	else
	{
		pSpiCtrl->isBusy == TRUE;
	}
	
    /* 
    Disable device 
	*/
    spiCtrl_Disable(pSpi);

	/* 
    Select device as Slave 
	*/
    spiCtrl_Set_Slave(pSpi);
	
    /* Config CTRLR0 (TMode, CkMode, DFSize, IsSlaveOut, isLoopBack) */
    ret |= spiCtrl_Set_XfrMode(pSpi, SPIPS_TRANSFER_STATE);
    ret |= spiCtrl_Set_SckMode(pSpi, SPI_MODE_3);          /* 3 =>  #define SPI_MODE_3  (SPI_CKPOL | SPI_CKPHA) */

#if 0  /* 1 byte/frm*/
		ret |= spiCtrl_Set_FrmLen(pSpi, 8); 	  /* frame_len: 8bit-1bytes */
#else  /* 4 bytes/frm*/
		ret |= spiCtrl_Set_FrmLen(pSpi, 32);	   /* frame_len: 32bit-4bytes */
#endif

    ret |= spiCtrl_Set_LoopBack(pSpi, FALSE);  /* normal work mode, no loop */

    ret |= spiCtrl_Set_enSlvTxd(pSpi, TRUE);    

    /* 
    Config Tx/Rx Threshold 
	*/
    ret |= spiCtrl_Set_TxFiFoEmptyLvl(pSpi, 8); /* ->24 */
    ret |= spiCtrl_Set_RxFifoFullLvl(pSpi, 0);  /* -> 8 */
	
    spiCtrl_Set_DMATxLvl(pSpi, 8);
    spiCtrl_Set_DMARxLvl(pSpi, 24);

	
    /* 
    Config IMR : disable IRQ
	*/
    spiCtrl_DisIRQ(pSpi, SPIPS_INTR_ALL);
        
    if (ret == FMSH_FAILURE)
    {
        return FMSH_FAILURE;
    }	
	
    /* 
    Enable pSpi
    */
    spiCtrl_Enable(pSpi);

    return FMSH_SUCCESS;
}

int spiCtrl_SndFifo_Poll(vxT_SPI* pSpi, UINT32 tx_data)
{
	vxT_SPI_CTRL * pSpiCtrl = pSpi->pSpiCtrl;
	
    UINT32 count = 0;
    UINT8 status;
    
    status = spiCtrl_Rd_CfgReg32(pSpi, SPIPS_SR_OFFSET); 
	
    while ((status & SPIPS_SR_TFNF) == 0)   /* loop if TX fifo full */
    {
        sysUsDelay(1);
		
        count++;
        if(count > 10000)
        {
            break;
        }
        status = spiCtrl_Rd_CfgReg32(pSpi, SPIPS_SR_OFFSET); 
    }

    spiCtrl_Wr_CfgReg32(pSpi, SPIPS_DR_OFFSET, tx_data);  
	
	return FMSH_SUCCESS;
}

UINT32 spiCtrl_RcvFifo_Poll(vxT_SPI* pSpi)
{
	vxT_SPI_CTRL * pSpiCtrl = pSpi->pSpiCtrl;
	
    UINT32 count = 0;
    UINT8 status;
    
    status = spiCtrl_Rd_CfgReg32(pSpi, SPIPS_SR_OFFSET); 
    while ((status & SPIPS_SR_RFNE) == 0)  /* loop if RX fifo empty */
    {
        delay_us(1);
        count++;
        if(count > 10000)
        {
            break;
        }        
        status = spiCtrl_Rd_CfgReg32(pSpi, SPIPS_SR_OFFSET); 
    }      

    return spiCtrl_Rd_CfgReg32(pSpi, SPIPS_DR_OFFSET); 
}


int vxInit_Spi (int ctrl_x, int mode_mst_slv)
{
	vxT_SPI * pSpi = NULL;
	vxT_SPI_CTRL * pSpiCtrl = NULL;
	
	int ret = 0;	
	UINT32 tmp32 = 0;

	spiSlcr_Set_SpiClk();
	
#if 1 
	/*
	init the pSpi structure
	*/

	/**/
	/* spi_ctrl select*/
	/**/
	switch (ctrl_x)
	{
	case SPI_CTRL_0:
		pSpi = g_pSpi0;	
		bzero(pSpi, sizeof(vxT_SPI));
		
		pSpi->pSpiCtrl = (vxT_SPI_CTRL *)(&vxSpi_Ctrl_0.ctrl_x);
		bzero(pSpi->pSpiCtrl, sizeof(vxT_SPI_CTRL));

		pSpi->spi_x = SPI_CTRL_0;
		pSpi->pSpiCtrl->devId = SPI_CTRL_0;
		pSpi->pSpiCtrl->cfgBaseAddr = VX_SPI_0_CFG_BASE;
		break;
		
	case SPI_CTRL_1:
		pSpi = g_pSpi1;
		bzero(pSpi, sizeof(vxT_SPI));
	
		pSpi->pSpiCtrl = (vxT_SPI_CTRL *)(&vxSpi_Ctrl_1.ctrl_x);
		bzero(pSpi->pSpiCtrl, sizeof(vxT_SPI_CTRL));
		
		pSpi->spi_x = SPI_CTRL_1;
		pSpi->pSpiCtrl->devId = SPI_CTRL_1;
		pSpi->pSpiCtrl->cfgBaseAddr = VX_SPI_1_CFG_BASE;
		break;
	}
	
	pSpiCtrl = pSpi->pSpiCtrl;
	
	pSpiCtrl->sysClk = FPS_SPI0_CLK_FREQ_HZ;	/* 40M Hz*/
	pSpiCtrl->mode_mst_slv = mode_mst_slv;	    /* 0-master, 1-slave */
	pSpiCtrl->ctrl_x = pSpiCtrl->devId;
#endif	
	
    pSpiCtrl->flag = 0;

    pSpiCtrl->isEnable = FALSE;
    pSpiCtrl->isBusy   = FALSE;
    pSpiCtrl->isMaster = FALSE;   
	
    pSpiCtrl->totalBytes = 0;
    pSpiCtrl->remainBytes = 0;
    pSpiCtrl->requestBytes = 0;
	
    pSpiCtrl->sndBuf = NULL;
    pSpiCtrl->rcvBuf = NULL;

	switch (pSpiCtrl->mode_mst_slv)
	{
	case MODE_SPI_MASTER:
		spiCtrl_Init_Master(pSpi);
		break;
		
	case MODE_SPI_SLAVE:
		spiCtrl_Init_Slave(pSpi);
		break;
	}

	return;
}

/*
3.5.2.1. CTRLR0:
	bit11	SRL	R/W	环回逻辑，仅用于测试。
	内部激活时，将发送移位寄存器的输出连接至接收移位寄存器输入，可用于串行主从机
		0：正常模式操作
		1：测试模式操作
		当SSI配置为从机操作且工作在测试模式时，ss_in_n和ssi_clk信号由外部提供复位值：0x0
*/

int spiCtrl_IP_Loop(vxT_SPI* pSpi, UINT32 loop_tx_data)
{
	vxT_SPI_CTRL * pSpiCtrl = pSpi->pSpiCtrl;
	
    int error = 0;
    UINT32 data;
    
    VX_DBG("\n -I- Self IP_Loop Test: \n", 1,2,3,4,5,6);
    
    spiCtrl_Reset(pSpi, pSpiCtrl->ctrl_x);
	
    spiCtrl_Init_Master(pSpi);

    spiCtrl_Disable(pSpi);
    spiCtrl_Set_LoopBack(pSpi, TRUE);	
    spiCtrl_Enable(pSpi); 
    
    /* slave sends first, then master sends */
    spiCtrl_SndFifo_Poll(pSpi, loop_tx_data); /*0xAABBCCDD */
	VX_DBG("<ip_loop>spi_%d: snd: 0x%X \n", pSpiCtrl->ctrl_x, loop_tx_data, 3,4,5,6);
	
    delay_ms(100);
	
    /*data = FMSH_ReadReg(pSpi->config.baseAddress, SPIPS_DR_OFFSET);*/
    data = spiCtrl_RcvFifo_Poll(pSpi);	
    if (data != loop_tx_data) 
    {    
    	error = 0x1;
		printf("Err! <ip_loop>spi_%d: 0x%X(rx) != 0x%X(tx) \n", \
			     pSpiCtrl->ctrl_x, data, loop_tx_data, 4,5,6);
    }
	else
	{
		printf("Ok! <ip_loop>spi_%d: 0x%X(rx) == 0x%X(tx) \n", \
			     pSpiCtrl->ctrl_x, data, loop_tx_data, 4,5,6);
	}
	    
    /* 
    disable loop 
	*/
    spiCtrl_Disable(pSpi);
    spiCtrl_Set_LoopBack(pSpi, FALSE);
    spiCtrl_Enable(pSpi); 
    
    if (error)
    {
        TRACE_OUT(DEBUG_OUT, "-E- IP_loop test Fail! \n\n");
        return FMSH_FAILURE;
    }
    else
    {
        TRACE_OUT(DEBUG_OUT, "-I- IP_loop test Pass! \n\n");
        return FMSH_SUCCESS;
    }

	return 0;
}


/*
MIO_LOOPBACK	0x804	reserved	
--------------------------------
                       31:4	rw	0x0	reserved
                       
		I2C0_LOOP_I2C1	3	rw	0x0	I2C Loopback Control.
						0 = Connect I2C inputs according to MIO mapping.
						1 = Loop I2C 0 outputs to I2C 1 inputs, and I2C 1 outputs
						
		CAN0_LOOP_CAN1	2	rw	0x0	CAN Loopback Control.
						0 = Connect CAN inputs according to MIO mapping.
						1 = Loop CAN 0 Tx to CAN 1 Rx, and CAN 1 Tx to CAN 0 Rx.
						
		UA0_LOOP_UA1	1	rw	0x0	UART Loopback Control.
						0 = Connect UART inputs according to MIO mapping.
						1 = Loop UART 0 outputs to UART 1 inputs, and UART 1 outputs to UART 0 inputs.
							RXD/TXD cross-connected. RTS/CTS cross-connected.
							DSR, DTR, DCD and RI not used.
						
		SPI0_LOOP_SPI1	0	rw	0x0	SPI Loopback Control.
						0 = Connect SPI inputs according to MIO mapping.
						1 = Loop SPI 0 outputs to SPI 1 inputs, and SPI 1 outputs to SPI 0 inputs.
							The other SPI core will appear on the LS Slave Select.
----------------------------------------------------------------------------------
MIO swap:
spi_0(master) --> spi_1(slave)
spi_1         --> spi_0
*/
int spiCtrl_SLCR_Loop(UINT32 loop_tx_data)
{
	vxT_SPI* pSpi0 = g_pSpi0;
	vxT_SPI* pSpi1 = g_pSpi1;
	
    int error = 0;
    UINT32 rx_data0, rx_data1;
    
    TRACE_OUT(DEBUG_OUT, "\r\n-I- SLCR_Loop Test\r\n");
    
    /* 
    enable SLCR loop 
	*/
    spiSlcr_Set_SpiLoop(TRUE);
    
    /* 
    spi0 master, spi1 slaver 
	*/
    /*FSpiPs_Reset(spi0);*/
    /*FSpiPs_Reset(spi1);    */
    /*FSpiPs_Initialize_Master(spi0);*/
    /*FSpiPs_Initialize_Slave(spi1);  */
    if (pSpi0->pSpiCtrl->status != 1)
	{
	    vxInit_Spi(SPI_CTRL_0, MODE_SPI_MASTER);
		pSpi0->pSpiCtrl->status = 1;
	}

	if (pSpi1->pSpiCtrl->status != 1)
    {
    	vxInit_Spi(SPI_CTRL_1, MODE_SPI_SLAVE);
		pSpi1->pSpiCtrl->status = 1;
    }
	

    /* 
    slave sends first, then master sends 
	*/
#if 1 
	spiCtrl_SndFifo_Poll(pSpi1, loop_tx_data);	/* spi1-slave, spi0-master */
	printf("<slcr_loop>spi_%d: snd-0x%X (to mst) \n", pSpi1->pSpiCtrl->ctrl_x, loop_tx_data);
		
	delay_ms(10);	
	printf("<------>\n");	
	
	rx_data0 = spiCtrl_RcvFifo_Poll(pSpi0); 
	printf("<slcr_loop>spi_%d: rcv-0x%X (from slv) \n\n", pSpi0->pSpiCtrl->ctrl_x, rx_data0);
#endif

#if 1
    spiCtrl_SndFifo_Poll(pSpi0, loop_tx_data);  /* spi1-slave, spi0-master */
	printf("<slcr_loop>spi_%d: snd-0x%X (to slv) \n", pSpi0->pSpiCtrl->ctrl_x, loop_tx_data);
	
    delay_ms(10);	
	printf("<------>\n");
	
    rx_data1 = spiCtrl_RcvFifo_Poll(pSpi1); 
	printf("<slcr_loop>spi_%d: rcv-0x%X (from mst) \n\n", pSpi1->pSpiCtrl->ctrl_x, rx_data1);
#endif
	
    /* 
    disable slcr loop 
	*/
    spiSlcr_Set_SpiLoop(FALSE);

	return;
}


int spiCtrl_X_Loop(UINT32 loop_tx_data)
{
	vxT_SPI* pSpi0 = g_pSpi0;
	vxT_SPI* pSpi1 = g_pSpi1;
	
    int error = 0;
    UINT32 rx_data0, rx_data1;
    
    TRACE_OUT(DEBUG_OUT, "\r\n-I- X_Loop Test\r\n");
        
    /* 
    spi0 master, spi1 slaver 
	*/
    if (pSpi0->pSpiCtrl->status != 1)
	{
	    vxInit_Spi(SPI_CTRL_0, MODE_SPI_MASTER);
		pSpi0->pSpiCtrl->status = 1;
	}

	if (pSpi1->pSpiCtrl->status != 1)
    {
    	vxInit_Spi(SPI_CTRL_1, MODE_SPI_SLAVE);
		pSpi1->pSpiCtrl->status = 1;
    }
	

    /* 
    slave sends first, then master sends 
	*/
#if 1 
	spiCtrl_SndFifo_Poll(pSpi1, loop_tx_data);	/* spi1-slave, spi0-master */
	printf("<slcr_loop>spi_%d: snd-0x%X (to mst) \n", pSpi1->pSpiCtrl->ctrl_x, loop_tx_data);
		
	delay_ms(10);	
	printf("<------>\n");	
	
	rx_data0 = spiCtrl_RcvFifo_Poll(pSpi0); 
	printf("<slcr_loop>spi_%d: rcv-0x%X (from slv) \n\n", pSpi0->pSpiCtrl->ctrl_x, rx_data0);
#endif

#if 1
    spiCtrl_SndFifo_Poll(pSpi0, loop_tx_data);  /* spi1-slave, spi0-master */
	printf("<slcr_loop>spi_%d: snd-0x%X (to slv) \n", pSpi0->pSpiCtrl->ctrl_x, loop_tx_data);
	
    delay_ms(10);	
	printf("<------>\n");
	
    rx_data1 = spiCtrl_RcvFifo_Poll(pSpi1); 
	printf("<slcr_loop>spi_%d: rcv-0x%X (from mst) \n\n", pSpi1->pSpiCtrl->ctrl_x, rx_data1);
#endif

	
		
	
    /* 
    disable slcr loop 
	*/
    spiSlcr_Set_SpiLoop(FALSE);

	return;
}

#endif


#if 1

int g_test_spi2 = 0;

/* 
IP loop
*/
void test_spi_loop1(int spi_x, UINT32 tx_data)
{
	vxT_SPI * pSpi = NULL;
	UINT32 tx_loop_data = 0;

	switch (spi_x)
	{
	case SPI_CTRL_0:
		pSpi = g_pSpi0;
		break;
	case SPI_CTRL_1:
		pSpi = g_pSpi1;
		break;
	}
	
	if (pSpi->pSpiCtrl->status != 1)
	{
		vxInit_Spi(spi_x, MODE_SPI_MASTER);
		pSpi->pSpiCtrl->status = 1;	/* init ok*/
	}

	if (tx_data == 0)
	{
		tx_loop_data = g_test_spi2;
		g_test_spi2++;		
	}
	else
	{
		tx_loop_data = tx_data;
	}

	switch (spi_x)
	{
	case SPI_CTRL_0:
		spiCtrl_IP_Loop(pSpi, tx_loop_data);
		break;
	case SPI_CTRL_1:
		spiCtrl_IP_Loop(pSpi, tx_loop_data);
		break;
	}
	
	return;
}

/* 
SLCR loop
*/
void test_spi_loop2( UINT32 tx_data)
{
	UINT32 tx_loop_data = 0;
	
	if (tx_data == 0)
	{
		tx_loop_data = g_test_spi2;
		g_test_spi2++;		
	}
	else
	{
		tx_loop_data = tx_data;
	}
	
	spiCtrl_SLCR_Loop(tx_loop_data);

	return;
}

/* 
X_loop
*/
void test_spi_loop3( UINT32 tx_data)
{
	UINT32 tx_loop_data = 0;
	
	if (tx_data == 0)
	{
		tx_loop_data = g_test_spi2;
		g_test_spi2++;		
	}
	else
	{
		tx_loop_data = tx_data;
	}
	
	spiCtrl_X_Loop(tx_loop_data);

	return;
}

#endif




