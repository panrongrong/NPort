/* vxQspi_norflash.c - fmsh 7020/7045 pQspiFlash driver */

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
01a, 16Dec19, jc  written.
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

#include "vxQspi_Norflash.h"


/*
defines 
*/
#if 1

#undef VX_DBG
#define VX_DBG2

#define QSPI_DBG

#ifdef QSPI_DBG

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
global variables
*/
#if 1  
/* qspi_0 */
static vxT_QSPI_CTRL vxQspi_Ctrl_0 = {0};
static vxT_QSPI_FLASH vxQspi_Flash_0 = {0};

static vxT_QSPI vx_pQspi_0 = {0};
vxT_QSPI * g_pQspi0 = (vxT_QSPI *)(&vx_pQspi_0.qspi_x);

/* qspi_1 */
static vxT_QSPI_CTRL vxQspi_Ctrl_1 = {0};
static vxT_QSPI_FLASH vxQspi_Flash_1 = {0};

static vxT_QSPI vx_pQspi_1 = {0};
vxT_QSPI * g_pQspi1 = (vxT_QSPI *)(&vx_pQspi_1.qspi_x);
#endif


#if 1
static vxT_QSPI_CTRL_PARAMS gQspi_Ctrl_Params = {0};
static vxT_QSPI_DMA_PARAMS  gQspi_Dma_Params =  {0};


/*
ADDR_3_BYTES_MODE:
	1. flash chip size:  <= 16M 
	2. direct_mode
	3. >16M, set segment
	
ADDR_4_BYTES_MODE:
	1. > 16M 
	2. not set segment
*/
void qspiCtrl_params_init(void)
{
	/**/
	/* qspi ctrl params*/
	/**/
    gQspi_Ctrl_Params.hasIntr       =  1;
    gQspi_Ctrl_Params.hasDma        =  0;
    gQspi_Ctrl_Params.hasXIP        =  0;    
    gQspi_Ctrl_Params.txEmptyLvl    =  32;
    gQspi_Ctrl_Params.rxFullLvl     =  480;   
    gQspi_Ctrl_Params.rdMode        =  QOR_CMD;    
	
    gQspi_Ctrl_Params.baudRate      =  4; 
	
    gQspi_Ctrl_Params.sampleDelay   =  2;

	/**/
	/* address mode init*/
	/**/
    /*gQspi_Ctrl_Params.addrBytesMode =  ADDR_4_BYTES_MODE;      // default: 4_bytes_address mode*/
    gQspi_Ctrl_Params.addrBytesMode =  ADDR_3_BYTES_MODE;      /* for test*/
	if (QSPI_DIRECT_MODE)
	{
    	gQspi_Ctrl_Params.addrBytesMode =  ADDR_3_BYTES_MODE;  /* only 3_bytes_address mode in direct_mode		*/
	}

	/**/
	/* dma params*/
	/**/
    gQspi_Dma_Params.burstType  = 5;
    gQspi_Dma_Params.singleType = 2;
    gQspi_Dma_Params.txIf       = 6;
    gQspi_Dma_Params.rxIf       = 7;
    gQspi_Dma_Params.io         = 0;

	return;
}

#endif

#if 1

void qspiCtrl_Wr_CfgReg(vxT_QSPI* pQspi, UINT32 offset, UINT32 value)
{
	UINT32 tmp32 = pQspi->pQspiCtrl->cfgBaseAddr;
	FMQL_WRITE_32((tmp32 + offset), value);
	return;
}

UINT32 qspiCtrl_Rd_CfgReg(vxT_QSPI* pQspi, UINT32 offset)
{
	UINT32 tmp32 = pQspi->pQspiCtrl->cfgBaseAddr;
	return FMQL_READ_32(tmp32 + offset);
}

void qspiCtrl_Wr_Fifo(vxT_QSPI* pQspi, UINT32 offset, UINT32 value)
{
	UINT32 tmp32 = pQspi->pQspiCtrl->fifoBaseAddr;
	FMQL_WRITE_32((tmp32 + offset), value);
	return;
}

UINT32 qspiCtrl_Rd_Fifo(vxT_QSPI* pQspi, UINT32 offset)
{
	UINT32 tmp32 = pQspi->pQspiCtrl->fifoBaseAddr;
	return FMQL_READ_32(tmp32 + offset);
}


void qspiCtrl_Reset(vxT_QSPI* pQspi)
{
	UINT32 tmp32 = 0;
	
    switch (pQspi->pQspiCtrl->devId)
    {
    case QSPI_CTRL_0:
    #if 0
        tmp32 = slcr_read(SLCR_QSPI0_CTRL) | (QSPI_APB_RST_MASK);
		slcr_write(SLCR_QSPI0_CTRL, tmp32);
        tmp32 = slcr_read(SLCR_QSPI0_CTRL) | (QSPI_AHB_RST_MASK);
		slcr_write(SLCR_QSPI0_CTRL, tmp32);
        tmp32 = slcr_read(SLCR_QSPI0_CTRL) | (QSPI_REF_RST_MASK);
		slcr_write(SLCR_QSPI0_CTRL, tmp32);

        tmp32 = slcr_read(SLCR_QSPI0_CTRL) & (~(QSPI_APB_RST_MASK));
		slcr_write(SLCR_QSPI0_CTRL, tmp32);
        tmp32 = slcr_read(SLCR_QSPI0_CTRL) & (~(QSPI_AHB_RST_MASK));
		slcr_write(SLCR_QSPI0_CTRL, tmp32);
        tmp32 = slcr_read(SLCR_QSPI0_CTRL) & (~(QSPI_REF_RST_MASK));
		slcr_write(SLCR_QSPI0_CTRL, tmp32);
	#else
        tmp32 = slcr_read(SLCR_QSPI0_CTRL) | (QSPI_APB_RST_MASK | QSPI_AHB_RST_MASK | QSPI_REF_RST_MASK);
		slcr_write(SLCR_QSPI0_CTRL, tmp32);
		delay_ms(1);	
        tmp32 = slcr_read(SLCR_QSPI0_CTRL) & (~(QSPI_APB_RST_MASK | QSPI_AHB_RST_MASK | QSPI_REF_RST_MASK));
		slcr_write(SLCR_QSPI0_CTRL, tmp32);
	#endif
		break;
		
    case QSPI_CTRL_1:
        tmp32 = slcr_read(SLCR_QSPI1_CTRL) | (QSPI_APB_RST_MASK | QSPI_AHB_RST_MASK | QSPI_REF_RST_MASK);
		slcr_write(SLCR_QSPI1_CTRL, tmp32);
		delay_ms(1);	
        tmp32 = slcr_read(SLCR_QSPI1_CTRL) & (~(QSPI_APB_RST_MASK | QSPI_AHB_RST_MASK | QSPI_REF_RST_MASK));
		slcr_write(SLCR_QSPI1_CTRL, tmp32);
		break;
    }

	pQspi->pQspiFlash->seg_flag = 0;
	
	return;
}

int qspiCtrl_Wait_ExeOk(vxT_QSPI* pQspi, UINT32 offset, UINT32 regMask, UINT8 ok_flag)
{
    UINT32 tmp32 = 0;;
    int timeout = 100;
    
    do
	{
        delay_1us();
		
        tmp32 = qspiCtrl_Rd_CfgReg(pQspi, offset) & regMask; 
		switch (ok_flag)
		{
		case OK_FLAG_1:
			if (tmp32 == regMask)
			{
				return FMSH_SUCCESS;
			} 
			break;
			
		case OK_FLAG_0:
			if (tmp32 == 0x00)
			{
				return FMSH_SUCCESS;
			}			
			break;
		}
		
        timeout--;
    } while (timeout > 0);

	return FMSH_FAILURE;
}

int qspiCtrl_Wait_BusIdle(vxT_QSPI* pQspi)
{
    UINT32 tmp32 = 0;
    int timeout = 100;
    
    do
	{
        delay_1us();
		
		/*
		offset: 0x00 
		------------
		bit31: (RO) Serial Interface and QSPI pipeline is IDLE
		*/
        tmp32 = qspiCtrl_Rd_CfgReg(pQspi, QSPIPS_CFG_OFFSET) & 0x80000000; 
		
        if (tmp32 == 0x80000000)
		{
            return FMSH_SUCCESS;
        }   
		
        timeout--;
    } while (timeout > 0);
		
	return FMSH_FAILURE;
}

int qspiCtrl_CmdExecute(vxT_QSPI* pQspi, UINT32 ctrl_cmd)
{
    int ret;
	UINT32 cmd = 0;
	
    /* 
    Read configuration Register command(0x35) to device (1B) 
	*/
	/*
	Flash Command Control Register:
	
	Offset: 0x90	
	-------------
	Bit     R/W Description Reset
	31:24	R/W 鍛戒护鎿嶄綔绗�	鍛戒护鎿嶄綔绗﹀簲璇ュ湪瑙﹀彂鍓嶈缃畬鎴愩�	8'h00
	23	    R/W 璇绘暟鎹娇鑳� 	濡傛灉bit[31:24]闇�浠庡櫒浠惰澶囦笂璇诲彇鏁版嵁	1'h0
	22:20	R/W 璇绘暟鎹釜鏁� 	鏈�鍙互璇诲彇8byte鏁版嵁(0 瀵瑰簲1byte鈥�7瀵瑰簲8byte)
				濡傛灉Flash Command Control Register bit[2]浣胯兘锛岃瀛楁鏃犳晥銆�
				璇诲彇鏁版嵁澶у皬渚濇嵁Flash Command Control Memory Register 鐨刡it[15:8]	3'h0
	19	    R/W 鍛戒护鍦板潃浣胯兘: 	褰揵it[31:24]闇�浼犺緭鍦板潃鏃讹紝缃�1鈥�1'h0
	18	    R/W Mode 浣嶄娇鑳姐�: 	璇ヤ綅琚疆鈥�鈥濓紝mode浣嶆寜鐓ode Bit Configuration Register锛屽苟鍦ㄥ湴鍧�浼犺緭鍚庤浼犻�鍑哄幓	1'h0
	17:16	R/W 鍦板潃鑷繁鏁帮紝璁剧疆闇�浼犺緭鐨勫瓧鑺備綅瀹�
				2'b00	1byte
				2'b01	2byte
				2'b10	3byte
				2'b11	4byte	2'h00
	15		    鍐欐暟鎹娇鑳�  	褰揵it[31:24]闇�鍐欐暟鎹椂锛岃浣嶇疆鈥�鈥�1'h0
	14:12		鍐欏叆鏁版嵁瀛楄妭鏁�  	鏈�鍙互鍐�byte鏁版嵁(0 瀵瑰簲1byte鈥�7瀵瑰簲8byte)	3'h0
	11:7		Dummy鏃堕挓鏁�:	褰揵it[31:24]闇�Dummy鏃堕挓锛岃缃綅缃�5'h00
	6:3 	RO	Reserved	4'h0
	2		R/W STIG瀛樺偍鍣╞ank浣胯兘 1'h0
	1		RO	STIG鍛戒护姝ｅ湪鎵ц	1'h0
	0		WO	鎵ц璇ュ懡浠�n/a
	*/	
	cmd = ctrl_cmd;

	/**/
	/* set val to cmd_ctrl_reg with no-trig, at first*/
	/*	*/
    cmd &= ~(UINT32)0x01;
    qspiCtrl_Wr_CfgReg(pQspi, QSPIPS_FCCR_OFFSET, cmd);
	
	/*
	wait pipeline idle status
	*/
    ret = qspiCtrl_Wait_BusIdle(pQspi);
    if (ret == FMSH_FAILURE)
	{
        return FMSH_FAILURE;
    }
	
	/**/
	/* trig cmd_ctrl_reg*/
	/*	*/
    cmd = cmd | 0x01;  /* bit0 WO	鎵ц璇ュ懡浠�n/a*/
    qspiCtrl_Wr_CfgReg(pQspi, QSPIPS_FCCR_OFFSET, cmd);
	
	/*
	wait cmd execute ok
	*/
    ret = qspiCtrl_Wait_ExeOk(pQspi, QSPIPS_FCCR_OFFSET, QSPIPS_REG_FCCR_INPROGRESS_MASK, OK_FLAG_0);
    if (ret == FMSH_FAILURE)
	{
        return FMSH_FAILURE;
    }
    
	/*
	wait pipeline idle status
	*/
    ret = qspiCtrl_Wait_BusIdle(pQspi);
    if (ret == FMSH_FAILURE)
	{
        return FMSH_FAILURE;
    }
    
    return FMSH_SUCCESS;
}


void qspiCtrl_Set_BaudRate(vxT_QSPI* pQspi, int baud_rate)
{
    UINT32 cfgReg = 0;
	
    cfgReg = qspiCtrl_Rd_CfgReg(pQspi, QSPIPS_CFG_OFFSET);
	
    cfgReg &= ~QSPIPS_BAUD_MASK;
    cfgReg |= (baud_rate << QSPIPS_BAUD_SHIFT) & QSPIPS_BAUD_MASK;

	/*
	bit22~19:	R/W 涓绘満妯″紡涓嬫尝鐗圭巼鍒嗛(2鍒�2)
	--------
	SPI娉㈢壒鐜�= (涓绘満 reference clock) / BD
	BD= 4'h0  /2
		4'h1  /4
		4'h2  /6 
		鈥�
		4'hf  /32
	*/	
    qspiCtrl_Wr_CfgReg(pQspi, QSPIPS_CFG_OFFSET, cfgReg);

	return;
}

int qspiCtrl_Clr_RcvFifo(vxT_QSPI* pQspi)
{
	vxT_QSPI_CTRL * pQspiCtrl = pQspi->pQspiCtrl;
	
	int dataCnt = 0;	
	UINT8 * pBuf8 = (UINT8 *)pQspiCtrl->fifoBaseAddr;
	
	/*	Read  Watermark */
	dataCnt = qspiCtrl_Rd_CfgReg(pQspi, QSPIPS_SFLR_OFFSET) * 4;
	if (dataCnt > 0)
	{		
		/* 	Read data from SRAM 	*/
		memcpy((UINT8*)pQspiCtrl->rcvBuf, pBuf8, dataCnt);
	}

	return FMSH_SUCCESS;
}


#if 1

/*****************************************************************************
* This function sets pQspi controller address bytes number.
*
* @param
*
* @return
*
* @note		
*
******************************************************************************/
void qspiCtrl_Set_BytesMode(vxT_QSPI* pQspi, int addrSize)
{
    UINT32 cfgReg = 0;

	/*
	offset:0x14 
	-----------
		bit3~0: R/W Number of address bytes
				A value of 0 = 1 byte etc.
	*/
    cfgReg = qspiCtrl_Rd_CfgReg(pQspi, QSPIPS_DSCR_OFFSET);
	
    cfgReg &= ~QSPIPS_DS_ADDRSIZE_MASK;
    cfgReg |= (addrSize & QSPIPS_DS_ADDRSIZE_MASK);
	
    qspiCtrl_Wr_CfgReg(pQspi, QSPIPS_DSCR_OFFSET, cfgReg);

	return;
}

/*
set flash_cmd mode
 & addr 4bytes mode
*/
int qspiCtrl_SetFlashMode(vxT_QSPI* pQspi, UINT8 flash_cmd, int bytes_mode)
{
    UINT32 cfgReg = 0;
    
    if (bytes_mode == ADDR_3_BYTES_MODE)   /* 3 bytes_mode*/
    {
    	switch (flash_cmd)
    	{
    	case READ_CMD: 
			cfgReg = 0x00000003; 
			break;
    	case FAST_READ_CMD: 
			cfgReg = 0x0800000B; 
			break;
    	case DOR_CMD: 
			cfgReg = 0x0801003B; 
			break;
    	case QOR_CMD: 
			cfgReg = 0x0802006B; 
			break;
		
    	case DIOR_CMD:   
        	if (pQspi->pQspiFlash->maker == SPI_MICRON_ID)
            {	
            	cfgReg = 0x041110BB;
			}
        	else
            {	
            	cfgReg = 0x001110BB;   
			}
        	break;
			
    	case QIOR_CMD:
        	if (pQspi->pQspiFlash->maker == SPI_MICRON_ID)
           	{	
           		cfgReg = 0x081220EB;
			}
        	else
            {	
            	cfgReg = 0x041220EB;  
			} 
        	break;
			
    	default: 
			cfgReg = 0x0800000B; 
			break;
		}
		
        /* set program */
        qspiCtrl_Wr_CfgReg(pQspi, QSPIPS_DWIR_OFFSET, 0x00000002);
		
    }
    else if (bytes_mode == ADDR_4_BYTES_MODE)  /* 4 bytes_mode*/
    {
    	switch (flash_cmd)
    	{
    	case READ_CMD: 
			cfgReg = 0x00000013; 		
			break;
    	case FAST_READ_CMD: 
			cfgReg = 0x0800000C; 		
			break;
    	case DOR_CMD: 
			cfgReg = 0x0801003C; 
			break;
    	case QOR_CMD: 
			cfgReg = 0x0802006C; 
			break;
		
    	case DIOR_CMD:   
        	if (pQspi->pQspiFlash->maker == SPI_MICRON_ID)
            {	
            	cfgReg = 0x041110BC;
			}
        	else
            {	
            	cfgReg = 0x001110BC;   
			}
        	break;
			
    	case QIOR_CMD:
        	if (pQspi->pQspiFlash->maker == SPI_MICRON_ID)
           	{	
           		cfgReg = 0x081220EC;
			}
        	else
            {	
            	cfgReg = 0x041220EC;  
			} 
        	break;
			
    	default: 
			cfgReg = 0x0800000C; 
			break;
		}
		
        /*set program */
        qspiCtrl_Wr_CfgReg(pQspi, QSPIPS_DWIR_OFFSET, 0x00000012);
    }   
    else
    {
        return FMSH_FAILURE;
    }

    qspiCtrl_Set_BytesMode(pQspi, (bytes_mode - 1)); 
	
    qspiCtrl_Wr_CfgReg(pQspi, QSPIPS_DRIR_OFFSET, cfgReg);
    
    return FMSH_SUCCESS;
}


/*****************************************************************************
* This function sets pQspi controller read capture delay cycles.
*
* @param
*
* @return
*
* @note		
*
******************************************************************************/
void qspiCtrl_Set_CaptureDelay(vxT_QSPI* pQspi, int cycles)
{
	UINT32 cfgReg = 0;
	/*
	offset-0x10 bit4~1: R/W 
	        Delay the read data capturing logic 
	        by the programmable number of ref_clk cycles
	*/
	cfgReg = qspiCtrl_Rd_CfgReg(pQspi, QSPIPS_RDCR_OFFSET);
	
	cfgReg &= ~QSPIPS_RC_CAPTURE_DELAY_MASK;
	cfgReg |= (cycles << QSPIPS_RC_CAPTURE_DELAY_SHIFT) & QSPIPS_RC_CAPTURE_DELAY_MASK;
	
	qspiCtrl_Wr_CfgReg(pQspi, QSPIPS_RDCR_OFFSET, cfgReg);

	return;
}


void qspiCtrl_Set_3B_AddrMode(vxT_QSPI* pQspi)
{
	vxT_QSPI_CTRL * pQspiCtrl = pQspi->pQspiCtrl;
	int ret = 0;
	
    pQspiCtrl->addr_bytes_mode = ADDR_3_BYTES_MODE;
	
	ret = qspiCtrl_SetFlashMode(pQspi, QOR_CMD, pQspiCtrl->addr_bytes_mode);
    if (ret == FMSH_FAILURE)
    {
		VX_DBG2("ctrl_%d: qspiFlash_SetMode fail! \n", pQspiCtrl->ctrl_x, 2,3,4,5,6);
        return FMSH_FAILURE;
    }

	VX_DBG2("addr_bytes_mode:%d \n", pQspiCtrl->addr_bytes_mode, 2,3,4,5,6);
	return;
}

void qspiCtrl_Set_4B_AddrMode(vxT_QSPI* pQspi)
{
	vxT_QSPI_CTRL * pQspiCtrl = pQspi->pQspiCtrl;
	int ret = 0;
	
    pQspiCtrl->addr_bytes_mode = ADDR_4_BYTES_MODE;
	
	ret = qspiCtrl_SetFlashMode(pQspi, QOR_CMD, pQspiCtrl->addr_bytes_mode);
    if (ret == FMSH_FAILURE)
    {
		VX_DBG2("ctrl_%d: qspiFlash_SetMode fail! \n", pQspiCtrl->ctrl_x, 2,3,4,5,6);
        return FMSH_FAILURE;
    }

	VX_DBG2("addr_bytes_mode:%d \n", pQspiCtrl->addr_bytes_mode, 2,3,4,5,6);
	return;
}

void test_set_3b_addrmode(void)
{
	vxT_QSPI* pQspi = g_pQspi0;
	qspiCtrl_Set_3B_AddrMode(pQspi);
	return;
}

void test_set_4b_addrmode(void)
{
	vxT_QSPI* pQspi = g_pQspi0;
	qspiCtrl_Set_4B_AddrMode(pQspi);
	return;
}

#endif

#endif


#if 1
UINT8 qspiFlash_Get_Reg8(vxT_QSPI* pQspi, UINT8 flash_cmd)
{
    UINT32 ctrl_cmd = 0;
    UINT8 value = 0;
    
    /*ctrl_cmd = 0x00800001 | (flash_cmd << 24);*/
    ctrl_cmd = 0x00800000 | ((UINT32)flash_cmd << 24);
	
    /* 
    Read Register command from device (1B) 
	*/	
    qspiCtrl_CmdExecute(pQspi, ctrl_cmd);   
	
    value = qspiCtrl_Rd_CfgReg(pQspi, QSPIPS_FCRDRL_OFFSET) & 0xFF;
    return value;
} 

int qspiFlash_Set_Reg8(vxT_QSPI* pQspi, UINT8 flash_cmd, UINT8 value)
{
    UINT32 ctrl_cmd = 0;
    int ret = 0;
    
    /*ctrl_cmd = 0x00008001 | (flash_cmd << 24);*/
    ctrl_cmd = 0x00008000 | ((UINT32)flash_cmd << 24);
	
    /* 
    Set value to be sent 
	*/
    qspiCtrl_Wr_CfgReg(pQspi, QSPIPS_FCWDRL_OFFSET, (UINT32)value);
    
    ret = qspiFlash_WREN_Cmd(pQspi);
    if (ret == FMSH_FAILURE)
	{
        return FMSH_FAILURE;
    }
    
    /* 
    Send command to device 
	*/
    ret = qspiCtrl_CmdExecute(pQspi, ctrl_cmd);
    if (ret == FMSH_FAILURE)
	{
        return FMSH_FAILURE;
    }
    
    /*ret = qspiFlash_Wait_WIP_Ok(pQspi);*/
	ret = qspiFlash_Wait_FlashReady(pQspi, 1500);
    if (ret == FMSH_FAILURE)
	{
        return FMSH_FAILURE;
    }
    
    return FMSH_SUCCESS;
}

UINT16 qspiFlash_Get_Reg16(vxT_QSPI* pQspi, UINT8 flash_cmd)
{
    int ret;
    UINT32 ctrl_cmd = 0;
    UINT16 tmp16 = 0;
    
    /*ctrl_cmd = 0x00900001 | (flash_cmd << 24);*/
    ctrl_cmd = 0x00900000 | ((UINT32)flash_cmd << 24);
	
    /* 
    Read Register command from device (2B)
    */
    ret = qspiCtrl_CmdExecute(pQspi, ctrl_cmd);  
    if (ret == FMSH_FAILURE)
	{
        return FMSH_FAILURE;	
    }
	
    tmp16 = qspiCtrl_Rd_CfgReg(pQspi, QSPIPS_FCRDRL_OFFSET) & 0xFFFF;
    return tmp16;
}

int qspiFlash_Set_Reg16(vxT_QSPI* pQspi, UINT8 flash_cmd, UINT8 high_val8, UINT8 low_val8)
{
    int ret;
    UINT32 ctrl_cmd = 0;
    
    /*ctrl_cmd = 0x00009001 | (flash_cmd << 24);*/
    ctrl_cmd = 0x00009000 | ((UINT32)flash_cmd << 24);
	
    /* Set value to be sent*/
    qspiCtrl_Wr_CfgReg(pQspi, QSPIPS_FCWDRL_OFFSET, (UINT32)((high_val8 << 8) | low_val8));  
	
    ret = qspiFlash_WREN_Cmd(pQspi);
    if (ret == FMSH_FAILURE)
	{
        return FMSH_FAILURE;
    }
	
    /* Send command to device*/
    ret = qspiCtrl_CmdExecute(pQspi, ctrl_cmd);
    if (ret == FMSH_FAILURE)
	{
        return FMSH_FAILURE;
    }
	
    /*ret = qspiFlash_Wait_WIP_Ok(pQspi);*/
	ret = qspiFlash_Wait_FlashReady(pQspi, 1500);
    if (ret == FMSH_FAILURE)
	{
        return FMSH_FAILURE;
    }
    
    return FMSH_SUCCESS;
}


int qspiFlash_Get_RegStatus1(vxT_QSPI* pQspi, u8  *status)
{
    int ret = 0;
	
    /* 
    Send Read Status1 Register command(0x05) to device 
	*/
    /*ret = qspiCtrl_CmdExecute(pQspi, 0x05800001);*/
    ret = qspiCtrl_CmdExecute(pQspi, 0x05800000);
    
    if (ret == FMSH_FAILURE)
	{
        return FMSH_FAILURE;
    }
    
    *status = qspiCtrl_Rd_CfgReg(pQspi, QSPIPS_FCRDRL_OFFSET) & 0xFF;
    
    return FMSH_SUCCESS;
} 

UINT8 qspiFlash_Get_FlagStatus(vxT_QSPI* pQspi)
{
	UINT8 ret = 0;
	
    /*Send Read Flag Status Register command(0x70) to device*/
    qspiCtrl_CmdExecute(pQspi, 0x70800000);
	
    ret = qspiCtrl_Rd_CfgReg(pQspi, QSPIPS_FCRDRL_OFFSET) & 0xFF;
	return ret;
} 

void qspiFlash_Clr_FlagStatus(vxT_QSPI* pQspi)
{
    qspiCtrl_CmdExecute(pQspi, 0x50000000);
	return;
} 



int qspiFlash_WREN_Cmd(vxT_QSPI* pQspi) 
{
    int ret = 0;
    UINT8 status;
    UINT32 timeout = 1500; 
	
    /* 
    Send WREN(0x06) Command 
	*/
    /*ret = qspiCtrl_CmdExecute(pQspi, 0x06000001);    */
    ret = qspiCtrl_CmdExecute(pQspi, 0x06000000);   
	
    if (ret == FMSH_FAILURE)
	{
        return FMSH_FAILURE;
    }
	
   	do
	{     
        delay_1ms();
		
        /* 
        poll status.wel value 
		*/
         ret = qspiFlash_Get_RegStatus1(pQspi, &status);
         if (ret == FMSH_FAILURE)
         {
        	 goto delay;
         }
         
        if ((status & WEL_MASK) == WEL_MASK)
		{
            return FMSH_SUCCESS;
        }
delay:		
        timeout--;
    } while (timeout > 0);
		
	return FMSH_FAILURE;
}

/*
flash cmd:
----------
SR1: 
	bit0 WIP Write in Progress
*/
int qspiFlash_Wait_WIP_Ok(vxT_QSPI* pQspi, int time_out)
{ 
    UINT8 tmp8;
    int timeout = time_out;
    int ret;
    
    /* 
    Poll Status Register1 
	*/
   	do
	{
        delay_1ms();
		
        ret = qspiFlash_Get_RegStatus1(pQspi, &tmp8);
        if (ret == FMSH_FAILURE)
        {
       	 goto delay;
        }
        
        tmp8 = ~tmp8;
        tmp8 &= BUSY_MASK;
        if(tmp8 == BUSY_MASK) /* 1-Device Busy, 0-Ready Device is in standby */
		{
            return FMSH_SUCCESS;
        }   
delay:		
        timeout--;
    } while (timeout > 0);
	
	return FMSH_FAILURE;
}

int qspiFlash_Wait_FSR_Ok(vxT_QSPI* pQspi, int time_out)
{ 
    UINT8 status;
    int det = 2;
    int cnt = 0;
    
    while (1)
	{
        status = qspiFlash_Get_FlagStatus(pQspi);
        if ((status & 0x80)== 0x80)
		{
            det--;
            if (det == 0)
            {    
            	return FMSH_SUCCESS;
            }
        }    
		
        delay_1ms();
		
        cnt++;
        if (cnt > time_out)
		{
            return FMSH_FAILURE;
        }
    }
}

int qspiFlash_Wait_FlashReady(vxT_QSPI* pQspi, int time_out)
{ 
    int status;
    
    /* Poll Status Register1*/
    status = qspiFlash_Wait_WIP_Ok(pQspi, time_out);
    if (status == FMSH_FAILURE)
    {    
    	return FMSH_FAILURE;
    }

	/**/
    /* Poll Flag Status Register, only for such as: MT25Q512 */
    /**/
    if (pQspi->pQspiFlash->flagStatus & QSPI_FSR)
    {
        status = qspiFlash_Wait_FSR_Ok(pQspi, time_out);
        if (status == FMSH_FAILURE)
		{	 
			return FMSH_FAILURE;
		}
        
        status = qspiFlash_Get_FlagStatus(pQspi);
        if (status & 0x32)
        {
            qspiFlash_Clr_FlagStatus(pQspi);
            return FMSH_FAILURE;
        }
    }
    
    return FMSH_SUCCESS;
}


UINT32 qspiFlash_ReadId(vxT_QSPI* pQspi)
{
	UINT32  tmp32 = 0;
	
    /* 
    Send RDID Command (4 bytes) 
	*/
    qspiCtrl_CmdExecute(pQspi, 0x9FD00001);
	
    /*Get ID */
    tmp32 = qspiCtrl_Rd_CfgReg(pQspi, QSPIPS_FCRDRL_OFFSET);

	return tmp32;
}

void test_qspiflash_rd_id(void)
{
	vxT_QSPI* pQspi = g_pQspi0;
	
	printf("qpsiflash_ID: 0x%08X \n", qspiFlash_ReadId(pQspi));
	
	return;
}

UINT32 qspiFlash_ReadId_2(vxT_QSPI* pQspi, UINT32* buf)
{
	UINT32  tmp32 = 0;	
	UINT32  tmp32_2 = 0;
	
    /* 
    Send RDID Command (4 bytes) 
	*/
    /*qspiCtrl_CmdExecute(pQspi, 0x9FD00001);*/
    qspiCtrl_CmdExecute(pQspi, 0x9FF00000);  /* more bytes*/
	
    /*
    Get ID 
    */
    tmp32 = qspiCtrl_Rd_CfgReg(pQspi, QSPIPS_FCRDRL_OFFSET);	
    tmp32_2 = qspiCtrl_Rd_CfgReg(pQspi, QSPIPS_FCRDRU_OFFSET);

	buf[0] = tmp32;
	buf[1] = tmp32_2;
	
	return tmp32;
}

#if 1
/*
all id data_area data bytes:
*/
void qspiFlash_ReadId_3(vxT_QSPI* pQspi, UINT8* buf)
{
	UINT32 cfgReg = 0;
	UINT32 tmp32 = 0;

	/* read the setup val*/
	tmp32 = qspiCtrl_Rd_CfgReg(pQspi, QSPIPS_DRIR_OFFSET);
	
	/*
	from byte_idx_3 to byte_idx_last
	*/
	cfgReg = 0x9F;
    qspiCtrl_Wr_CfgReg(pQspi, QSPIPS_DRIR_OFFSET, cfgReg);

	/* read data_area of id_area*/
	qspiFlash_Read_Page(pQspi, 0, buf);

	/* replace the val*/
    qspiCtrl_Wr_CfgReg(pQspi, QSPIPS_DRIR_OFFSET, tmp32);
	
	return;
}

void test_qspiflash_rd_ids(void)
{
	UINT32 tmp32 = 0;
	UINT8 pBuf8[0x100] = {0};
	vxT_QSPI* pQspi = g_pQspi0;
	int i = 0, j = 0;
	
	/*
	from byte_idx_3 to byte_idx_last
	*/
	qspiFlash_ReadId_3(pQspi, (UINT8*)(&pBuf8[4]));
	
    /* 
    Get ID: first 4bytes data 
	*/
    tmp32 = qspiFlash_ReadId(pQspi);
	/*printf("tmp32: 0x%08X \n", tmp32);*/
	
	if (pQspi->pQspiCtrl->addr_bytes_mode == ADDR_3_BYTES_MODE)
	{
		/* 
		first 3bytes data 
		*/
		pBuf8[0] = (UINT8)(tmp32  & 0x000000FF);	
		pBuf8[1] = (UINT8)((tmp32 & 0x0000FF00) >> 8);
		pBuf8[2] = (UINT8)((tmp32 & 0x00FF0000) >> 16);
		/*
		from byte_idx_3 to byte_idx_last
		*/
		for (i=4; i<0x100; i++)
		{
			pBuf8[i-1] = pBuf8[i];
		}
	}
	else
	{
		/* 
		first 4bytes data 
		*/
		pBuf8[0] = (UINT8)(tmp32  & 0x000000FF);	
		pBuf8[1] = (UINT8)((tmp32 & 0x0000FF00) >> 8);
		pBuf8[2] = (UINT8)((tmp32 & 0x00FF0000) >> 16);
		pBuf8[3] = (UINT8)((tmp32 & 0xFF000000) >> 24);
	}

	
#if 1
	for (j=0; j<0x100; j++)
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
#endif

	return;
}

/*
-> vxInit_Qspi 0
value = 0 = 0x0
-> 
-> test_qspiflash_rd_ids
0102194D 01805230 81FFFFFF FFFFFFFF  
51525902 00400053 46510027 36000006  
08081002 02030319 02010800 021F0010  
00FD0100 01FFFFFF FFFFFFFF FFFFFFFF  
50524931 33210201 00080001 03000007  
01414C54 32300010 53323546 4C323536  
5341473F 3F495230 8001F084 0885288A  
6475287A 6488040A 0100018C 06960123  
00230090 56060E46 4303130B 0C3B3C6B  
6CBBBCEB EC320300 00000000 00000004  
00020150 00FFFF00 08000800 08040002  
045A01FF FF000800 08000804 01020468  
02FFFF00 08000800 08040202 058502FF  
FF0008FF FFFFFFFF FFFFFF94 01109801  
009A2A05 0846430D 0EBDBEED EE320304  
01020201 03420004 02020401 06420104  
*/
#endif

#if 0
int qspiFlash_ParseInfo(vxT_QSPI* pQspi, UINT32 flashId)
{  
	vxT_QSPI_FLASH * pQspiFlash = pQspi->pQspiFlash;
		
    UINT8 maker = 0, devSize = 0;
	int ret = FMSH_SUCCESS;
	
    maker = flashId & 0xFF;
    devSize = (flashId >> 16) & 0xFF;
    
    /* 
    Deduce flash make   
	*/
	switch (maker)
	{
	case SPI_MICRON_ID:
	case SPI_SPANSION_ID:
	case SPI_WINBOND_ID:
	case SPI_MACRONIX_ID:
	case SPI_ISSI_ID:
	case SPI_FMSH_ID:
		pQspiFlash->maker = maker;
		break;
	
	default:
        pQspiFlash->maker = SPI_WRONG_ID;
        ret = FMSH_FAILURE;
		break;
	}
    
    /* 
    Deduce flash Size 
	*/
    switch (devSize)
    {
   	case QSPIFLASH_SIZE_ID_256K: 
        pQspiFlash->devSize = QSPIFLASH_SIZE_256K;
		break;
    case QSPIFLASH_SIZE_ID_512K: 
        pQspiFlash->devSize = QSPIFLASH_SIZE_512K;
		break;
    case QSPIFLASH_SIZE_ID_1M:
        pQspiFlash->devSize = QSPIFLASH_SIZE_1M;
		break;
    case QSPIFLASH_SIZE_ID_2M: 
        pQspiFlash->devSize = QSPIFLASH_SIZE_2M;
		break;
    case QSPIFLASH_SIZE_ID_4M: 
        pQspiFlash->devSize = QSPIFLASH_SIZE_4M;
		break;
    case QSPIFLASH_SIZE_ID_8M:
        pQspiFlash->devSize = QSPIFLASH_SIZE_8M;
		break;
    case QSPIFLASH_SIZE_ID_16M:
        pQspiFlash->devSize = QSPIFLASH_SIZE_16M;
		break;
    case QSPIFLASH_SIZE_ID_32M:
        pQspiFlash->devSize = QSPIFLASH_SIZE_32M;
		break;
    case QSPIFLASH_SIZE_ID_64M: 
        pQspiFlash->devSize = QSPIFLASH_SIZE_64M;
		break;
    case QSPIFLASH_SIZE_ID_128M:
        pQspiFlash->devSize = QSPIFLASH_SIZE_128M;
		break;
    case QSPIFLASH_SIZE_ID_256M: 
        pQspiFlash->devSize = QSPIFLASH_SIZE_256M;
		break;

    case QSPIFLASH_SIZE_ID_512M:
    case MACRONIX_QSPIFLASH_SIZE_ID_512M: /* ISSI_QSPIFLASH_SIZE_ID_512M */
        pQspiFlash->devSize = QSPIFLASH_SIZE_512M;
		break;

    case QSPIFLASH_SIZE_ID_1G:
    case MACRONIX_QSPIFLASH_SIZE_ID_1G: 
        pQspiFlash->devSize = QSPIFLASH_SIZE_1G;
		break;

	default:
        pQspiFlash->devSize = SPI_WRONG_ID;
        ret = FMSH_FAILURE;
		break;
    }

	pQspiFlash->flash_ID1 = flashId;
	
    return ret;
}

#endif

int qspiFlash_ParseInfo_2(vxT_QSPI* pQspi, UINT32 * flashId)
{  
	vxT_QSPI_FLASH * pQspiFlash = pQspi->pQspiFlash;
	
    UINT8 maker = 0, devSize = 0, bigSectMode = 0;
	int ret = FMSH_SUCCESS;
	
    maker = flashId[0] & 0xFF;
    devSize = (flashId[0] >> 16) & 0xFF;
	
    bigSectMode = flashId[1] & 0xFF;  /* judge: 256K or 64K*/

    /* 
    Deduce flash make   
	*/
	switch (maker)
	{
	case SPI_MICRON_ID:
	case SPI_SPANSION_ID:
	case SPI_WINBOND_ID:
	case SPI_MACRONIX_ID:
	case SPI_ISSI_ID:
		
	case SPI_FMSH_ID:
	case SPI_GD_ID:
		pQspiFlash->maker = maker;
		break;
	
	default:
        pQspiFlash->maker = SPI_WRONG_ID;
        ret = FMSH_FAILURE;
		break;
	}
	/*printf("maker: 0x%X \n", pQspiFlash->maker);*/
    
    /* 
    Deduce flash Size 
	*/
    switch (devSize)
    {
   	case QSPIFLASH_SIZE_ID_256K: 
        pQspiFlash->devSize = QSPIFLASH_SIZE_256K;
		break;
    case QSPIFLASH_SIZE_ID_512K: 
        pQspiFlash->devSize = QSPIFLASH_SIZE_512K;
		break;
    case QSPIFLASH_SIZE_ID_1M:
        pQspiFlash->devSize = QSPIFLASH_SIZE_1M;
		break;
    case QSPIFLASH_SIZE_ID_2M: 
        pQspiFlash->devSize = QSPIFLASH_SIZE_2M;
		break;
    case QSPIFLASH_SIZE_ID_4M: 
        pQspiFlash->devSize = QSPIFLASH_SIZE_4M;
		break;
    case QSPIFLASH_SIZE_ID_8M:
        pQspiFlash->devSize = QSPIFLASH_SIZE_8M;
		break;
    case QSPIFLASH_SIZE_ID_16M:
        pQspiFlash->devSize = QSPIFLASH_SIZE_16M;
		break;
    case QSPIFLASH_SIZE_ID_32M:
        pQspiFlash->devSize = QSPIFLASH_SIZE_32M;
		break;
    case QSPIFLASH_SIZE_ID_64M: 
        pQspiFlash->devSize = QSPIFLASH_SIZE_64M;
		break;
    case QSPIFLASH_SIZE_ID_128M:
        pQspiFlash->devSize = QSPIFLASH_SIZE_128M;
		break;
    case QSPIFLASH_SIZE_ID_256M: 
        pQspiFlash->devSize = QSPIFLASH_SIZE_256M;
		break;

    case QSPIFLASH_SIZE_ID_512M:
    case MACRONIX_QSPIFLASH_SIZE_ID_512M: /* ISSI_QSPIFLASH_SIZE_ID_512M = MACRONIX_QSPIFLASH_SIZE_ID_512M */
        pQspiFlash->devSize = QSPIFLASH_SIZE_512M;
		break;

	case QSPIFLASH_SIZE_ID_1G:
    case MACRONIX_QSPIFLASH_SIZE_ID_1G: 
        pQspiFlash->devSize = QSPIFLASH_SIZE_1G;
		break;

	default:
        pQspiFlash->devSize = SPI_WRONG_ID;
        ret = FMSH_FAILURE;
		break;
    }
	/*printf("devSize: 0x%X \n", pQspiFlash->devSize);*/
	
	/*
	00h (Uniform 256-kB sectors)
	01h (4-kB parameter sectors with uniform 64-kB sectors)
	*/
	/*
	switch (bigSectMode)
	{
	case 0x00:
        pQspiFlash->sectSize = QSPIFLASH_SECTOR_256K;
		break;
		
	case 0x01:
        pQspiFlash->sectSize = QSPIFLASH_SECTOR_64K;
		break;
		
	default:
        pQspiFlash->sectSize = SPI_WRONG_ID;
        ret = FMSH_FAILURE;
	break;
	}
	*/
	switch (maker)
    {
    case SPI_MICRON_ID: 
        pQspiFlash->sectSize = QSPIFLASH_SECTOR_64K; /* 0x1 << 16;  // 64K bytes*/
        pQspiFlash->pageSize = QSPIFLASH_PAGE_256;  /* 0x1 << 8;   // 256 bytes*/
        
        if (pQspiFlash->devSize >= QSPIFLASH_SIZE_512M)
        {
			qspiCtrl_Wr_CfgReg(pQspi, QSPIPS_WCCR_OFFSET, 0x022770);
			pQspiFlash->flagStatus |= QSPI_FSR;  /* qspi-flash flag_status */
			
			printf("Micron: N25Q512A \n");
        }
        break;
		
    case SPI_SPANSION_ID:		
		switch (bigSectMode)
		{
		case 0x00:
			pQspiFlash->sectSize = QSPIFLASH_SECTOR_256K;
			break;
			
		case 0x01:
			pQspiFlash->sectSize = QSPIFLASH_SECTOR_64K;
			break;
		}        
        pQspiFlash->pageSize = QSPIFLASH_PAGE_256;
        break;
		
    case SPI_WINBOND_ID: 
        pQspiFlash->sectSize = QSPIFLASH_SECTOR_64K;
        pQspiFlash->pageSize = QSPIFLASH_PAGE_256;
        break;
		
    case SPI_MACRONIX_ID: 
        pQspiFlash->sectSize = QSPIFLASH_SECTOR_64K;
        pQspiFlash->pageSize = QSPIFLASH_PAGE_256;
        break;
		
    case SPI_ISSI_ID: 
        pQspiFlash->sectSize = QSPIFLASH_SECTOR_64K;
        pQspiFlash->pageSize = QSPIFLASH_PAGE_256;
        break;
		
    case SPI_FMSH_ID: 
        pQspiFlash->sectSize = QSPIFLASH_SECTOR_64K;
        pQspiFlash->pageSize = QSPIFLASH_PAGE_256;
        break;
		
    case SPI_GD_ID: 
        pQspiFlash->sectSize = QSPIFLASH_SECTOR_64K;
        pQspiFlash->pageSize = QSPIFLASH_PAGE_256;
        break;
		
    default: 
        pQspiFlash->sectSize = QSPIFLASH_SECTOR_64K;
        pQspiFlash->pageSize = QSPIFLASH_PAGE_256;
        break;
    }
	
	/*printf("sectSize: 0x%X \n", pQspiFlash->sectSize);*/
	/*printf("pageSize: 0x%X \n", pQspiFlash->pageSize);*/
	
	pQspiFlash->flash_ID1 = flashId[0];
	pQspiFlash->flash_ID2 = flashId[1];
	
    return ret;
}

#if 0
int qspiFlash_SectErase_CmdExe(vxT_QSPI* pQspi, UINT32 start_offset)
{
	vxT_QSPI_CTRL * pQspiCtrl = pQspi->pQspiCtrl;

    int ret = 0;
    UINT32 flash_cmd_addr = 0;
    UINT32 ctrl_cmd = 0;
    UINT8 tmp8 = 0;
    int timeout = 0;
    
    /*
    Set Flash Command Address 
	*/
    flash_cmd_addr = start_offset;
    qspiCtrl_Wr_CfgReg(pQspi, QSPIPS_FCAR_OFFSET, flash_cmd_addr);
    
    ret = qspiFlash_WREN_Cmd(pQspi);
    if (ret == FMSH_FAILURE)
	{
        return FMSH_FAILURE;
    }
    
    /*
    Send Erase Command 
	*/
    if (pQspiCtrl->addr_bytes_mode == ADDR_3_BYTES_MODE)
	{	
		ctrl_cmd = 0x000A0001 | ((UINT32)0xD8 << 24);
		/*printf("erase sector:0x%X 3bytes mode! \n", start_offset);*/
    }
    else if (pQspiCtrl->addr_bytes_mode == ADDR_4_BYTES_MODE)
	{	
		ctrl_cmd = 0x000B0001 | ((UINT32)0xDC << 24);
		/*printf("erase sector:0x%X 4bytes mode! \n", start_offset);*/
    }

	/*
	cmd excute
	*/
    ret = qspiCtrl_CmdExecute(pQspi, ctrl_cmd);
    if (ret == FMSH_FAILURE)
	{
        return FMSH_FAILURE;
    }
		
    /* 
    Poll Status Register1 
	*/
	timeout = 5000*2;
   	do
	{
        delay_1ms();
		
        tmp8 = qspiFlash_Get_RegStatus1(pQspi) & BUSY_MASK;		
        if (tmp8 == 0x00) /* 1-Device Busy, 0-Ready Device is in standby */
		{
            return FMSH_SUCCESS;
        }   
		
        timeout--;
    } while (timeout > 0);
	
	return FMSH_FAILURE;
}
#endif

/***************
* pQspi flash reset
*
***************/
int qspiFlash_Reset_Flash(vxT_QSPI* pQspi)
{
	vxT_QSPI_CTRL * pQspiCtrl = pQspi->pQspiCtrl;
	vxT_QSPI_FLASH * pQspiFlash = pQspi->pQspiFlash;
	
    int ret = 0;

    if (pQspiFlash->maker == SPI_SPANSION_ID)
    {
		/* MBR Mode Bit Reset FF*/
        ret = qspiCtrl_CmdExecute(pQspi, 0xFF000001); 
        if (ret != FMSH_SUCCESS) 
        {
            return FMSH_FAILURE;
        }
		
		/* RESET Software Reset F0*/
        ret = qspiCtrl_CmdExecute(pQspi, 0xF0000001); 
        if (ret != FMSH_SUCCESS) 
        {
            return FMSH_FAILURE;
        }
    }
	else  /* all other flash type*/
	{
        ret = qspiCtrl_CmdExecute(pQspi, 0x66000001); 
        if (ret != FMSH_SUCCESS) 
        {
            return FMSH_FAILURE;
        }
		
        ret = qspiCtrl_CmdExecute(pQspi, 0x99000001); 
        if (ret != FMSH_SUCCESS) 
        {
            return FMSH_FAILURE;
        }
	}
    
    delay_1ms();
	
    ret = qspiFlash_Wait_FlashReady(pQspi, 1500);
    if(ret != FMSH_SUCCESS) 
    {
        return FMSH_FAILURE;
    }
	
	pQspi->pQspiFlash->seg_flag = 0;
	
    return FMSH_SUCCESS;
}


/*******************************
* pQspi flash disable protect
*
*******************************/
int qspiFlash_Unlock_Flash(vxT_QSPI* pQspi)
{
    int err = FMSH_SUCCESS;
    UINT8 status = 0, cfg = 0;

	if (pQspi->pQspiFlash->unlock == 1)
	{
		return FMSH_SUCCESS;
	}
	
    switch (pQspi->pQspiFlash->maker)
	{
	case SPI_SPANSION_ID:
        cfg = qspiFlash_Get_Reg8(pQspi, 0x35);     /* SPANSION_RDCR_CMD*/
        err = qspiFlash_Set_Reg16(pQspi, 0x01, cfg, 0x00);    
		break;
		
	case SPI_ISSI_ID:
	case SPI_MACRONIX_ID:
        status = qspiFlash_Get_Reg8(pQspi, 0x05);  /* RDSR1_CMD*/
        status &= 0x40;
        err = qspiFlash_Set_Reg8(pQspi, 0x01, status);   
		break;

	default:
        err = qspiFlash_Set_Reg8(pQspi, 0x01, 0x00);  
		break;		
	}

    if (err == FMSH_FAILURE) 
    {
        return FMSH_FAILURE;
    }
	
	pQspi->pQspiFlash->unlock = 1;  /* unlock ok	*/
    return FMSH_SUCCESS;
}

/*
flash is single-line mode, in normal
------------
4-line mode: FLASH_4_LINE_MODE
1-line mode: FLASH_1_LINE_MODE
*/
int qspiFlash_Get_QuadMode(vxT_QSPI* pQspi)
{
	int err = 0, ret = 0;
	UINT8 status = 0;
	UINT8 status1 = 0, status2 = 0;
	UINT8 cfg = 0; 

    switch (pQspi->pQspiFlash->maker)
	{
		case SPI_SPANSION_ID:
	    {
	        status = qspiFlash_Get_Reg8(pQspi, RDSR1_CMD); /*0x05*/
	        cfg = qspiFlash_Get_Reg8(pQspi, SPANSION_RDCR_CMD); /*0x35*/
	        
	        if ((cfg & SPANSION_QUAD_EN_MASK) == 0)
	        {
				ret = FLASH_1_LINE_MODE;
			}
			else
			{
				ret = FLASH_4_LINE_MODE;
			}
	    }
		break;
		
    case SPI_MICRON_ID:
	    {
			/*no need to operate			*/
			ret = FLASH_4_LINE_MODE;  /* default: 4-line mode*/
	    }    
		break;
	
    case SPI_MACRONIX_ID:
	    {
	        status = qspiFlash_Get_Reg8(pQspi, RDSR1_CMD); /*0x05*/
	        if ((status & MACRONIX_QUAD_EN_MASK) == 0)
	        {
				ret = FLASH_1_LINE_MODE;
			}
			else
			{
				ret = FLASH_4_LINE_MODE;
			}
	    }
		break;
    
    case SPI_ISSI_ID:
	    {   
	        status = qspiFlash_Get_Reg8(pQspi, RDSR1_CMD); /*0x05   */
	        if ((status & ISSI_QUAD_EN_MASK) == 0)
	        {
				ret = FLASH_1_LINE_MODE;
			}
			else
			{
				ret = FLASH_4_LINE_MODE;
			}
	    }
		break;
    
    case SPI_WINBOND_ID:
	    {
	        status1 = qspiFlash_Get_Reg8(pQspi, RDSR1_CMD); /*0x05*/
	        status2 = qspiFlash_Get_Reg8(pQspi, WINBOND_RDSR2_CMD); /*0x35     */
	        if ((status2 & WINBOND_QUAD_EN_MASK) == 0)
	        {
				ret = FLASH_1_LINE_MODE;
			}
			else
			{
				ret = FLASH_4_LINE_MODE;
			}
	    }
		break;
    
    case SPI_FMSH_ID:
	    {
	        status2 = qspiFlash_Get_Reg8(pQspi, FMSH_RDSR2_CMD); /*0x35*/
	        if ((status2 & FMSH_QUAD_EN_MASK) == 0)
	        {
				ret = FLASH_1_LINE_MODE;
			}
			else
			{
				ret = FLASH_4_LINE_MODE;
			}
	    }
		break;
 
    case SPI_GD_ID:
	    {
	        status2 = qspiFlash_Get_Reg8(pQspi, GD_RDSR2_CMD); /*0x35*/
	        if ((status2 & GD_QUAD_EN_MASK) == 0)
	        {
				ret = FLASH_1_LINE_MODE;
			}
			else
			{
				ret = FLASH_4_LINE_MODE;
			}
	    }
		break;
	
    default:
		ret = 0;
		break;
    }

	return ret;
}

void test_rd_quadmode(void)
{
	printf("line-mode: %d \n", qspiFlash_Get_QuadMode(g_pQspi0));
	return;
}

/*
flash is single-line mode, in normal
------------
4-line mode: FLASH_4_LINE_MODE
1-line mode: FLASH_1_LINE_MODE
*/
int qspiFlash_Set_QuadMode(vxT_QSPI* pQspi)
{
	int err = 0, ret = FMSH_SUCCESS;
	UINT8 status = 0;
	UINT8 status1 = 0, status2 = 0;
	UINT8 cfg = 0; 

    switch (pQspi->pQspiFlash->maker)
	{
		case SPI_SPANSION_ID:
	    {
	        status = qspiFlash_Get_Reg8(pQspi, RDSR1_CMD); /*0x05*/
	        cfg = qspiFlash_Get_Reg8(pQspi, SPANSION_RDCR_CMD); /*0x35*/
	        
	        if ((cfg & SPANSION_QUAD_EN_MASK) == 0)
	        {
	            cfg |= (UINT8)SPANSION_QUAD_EN_MASK;
	            err = qspiFlash_Set_Reg16(pQspi, WRR_CMD, cfg, status);  /*0x01*/
	            if (err)
	            {
	                ret = FMSH_FAILURE;
	            }  
	        }	
	    }
		break;
		
    case SPI_MICRON_ID:
	    {
			/*no need to operate, default 4-line mode*/
	    }    
		break;
	
    case SPI_MACRONIX_ID:
	    {
	        status = qspiFlash_Get_Reg8(pQspi, RDSR1_CMD); /*0x05*/
	        if ((status & MACRONIX_QUAD_EN_MASK) == 0)
	        {
	            status |= (UINT8)MACRONIX_QUAD_EN_MASK;
	            err = qspiFlash_Set_Reg8(pQspi, WRR_CMD, status);/*0x01*/
	            if (err)
	            {
	                ret = FMSH_FAILURE;
	            }  
	        }  
	    }
		break;
    
    case SPI_ISSI_ID:
	    {   
	        status = qspiFlash_Get_Reg8(pQspi, RDSR1_CMD); /*0x05   */
	        if ((status & ISSI_QUAD_EN_MASK) == 0)
	        {
	            status |= (UINT8)ISSI_QUAD_EN_MASK;
	            err = qspiFlash_Set_Reg8(pQspi, WRR_CMD, status); /*0x01*/
	            if(err)
	            {
	                ret = FMSH_FAILURE;
	            } 
	        }    
	    }
		break;
    
    case SPI_WINBOND_ID:
	    {
	        status1 = qspiFlash_Get_Reg8(pQspi, RDSR1_CMD); /*0x05*/
	        status2 = qspiFlash_Get_Reg8(pQspi, WINBOND_RDSR2_CMD); /*0x35     */
	        if ((status2 & WINBOND_QUAD_EN_MASK) == 0)
	        {
	            status2 |= (UINT8)WINBOND_QUAD_EN_MASK;
	            err = qspiFlash_Set_Reg16(pQspi, WRR_CMD, status2, status1);/*0x01*/
	            if (err)
	            {
	                ret = FMSH_FAILURE;
	            }  
	        }
	    }
		break;
    
    case SPI_FMSH_ID:
	    {
	        status2 = qspiFlash_Get_Reg8(pQspi, FMSH_RDSR2_CMD); /*0x35*/
	        if ((status2 & FMSH_QUAD_EN_MASK) == 0)
	        {
	            status2 |= (UINT8)FMSH_QUAD_EN_MASK;
	            err = qspiFlash_Set_Reg8(pQspi, FMSH_WRSR2_CMD, status2);/*0x31*/
	            if (err)
	            {
	                ret = FMSH_FAILURE;
	            }
	        } 
	    }
		break;
 
    case SPI_GD_ID:
	    {
	        status2 = qspiFlash_Get_Reg8(pQspi, GD_RDSR2_CMD); /*0x35*/
	        if ((status2 & GD_QUAD_EN_MASK) == 0)
	        {
	            status2 |= (UINT8)GD_QUAD_EN_MASK;
	            err = qspiFlash_Set_Reg8(pQspi, GD_WRSR2_CMD, status2);/*0x31*/
	            if (err)
	            {
	                ret = FMSH_FAILURE;
	            }
	        } 
	    }
		break;
	
    default:
		ret = FMSH_FAILURE;
		break;
    }

	return ret;
}

#endif


#if 1

#define __3_bytes_addr_mode__

/*
3_bytes_address mode:
*/

/*
while 3_bytes_address mode:
---------------------------
1. max one segment: 16M bytes
2. two or four segment for one_flash_chip
3. offset > 16M bytes, must be remap
4. highAddr: 0x00 - 0 ~ 16M
             0x01 - 16 ~ 32M
             0x02 - 32 ~ 48M
             0x03 - 48 ~ 64M
*/
int qspiFlash_Set_Segment(vxT_QSPI* pQspi, UINT8 highAddr)
{
    int err;
    UINT8 extAddr = 0, extAddrRb = 0;    
    int retry = 0;
	
    switch (pQspi->pQspiFlash->maker)
    {
    case SPI_SPANSION_ID:
    case SPI_ISSI_ID:
	    {        
	        /* bit7 is extadd id 0 to enable 3B address */
	        extAddr = highAddr & 0x7F;

			do 
			{
		        /*err = qspiFlash_Set_Reg8(pQspi, 0x17, extAddr); */
		        err = qspiFlash_Set_Reg8(pQspi, 0x17, extAddr); 
				if (err != FMSH_SUCCESS)
				{
					retry++;
				}
				else
				{
					break;
				}				
			} while (retry <= 5);			
	        if (err != FMSH_SUCCESS)
	        {
	        	printf("qspiFlash_Set_Segment->qspiFlash_Set_Reg8 fail! \n");
	            return FMSH_FAILURE;
	        } 
			else
			{
				/*printf("qspiFlash_Set_Segment->qspiFlash_Set_Reg8 (0x%X-%d) ok! \n", highAddr, extAddr);*/
			}
			
	        /* readback & verify */
	        /*extAddrRb = qspiFlash_Get_Reg8(pQspi, 0x16); */
	        extAddrRb = qspiFlash_Get_Reg8(pQspi, 0x16); 
	        if(extAddrRb != extAddr)
	        {
	        	printf("qspiFlash_Set_Segment->qspiFlash_Get_Reg8 fail: 0x%X-0x%X(r) \n", extAddr, extAddrRb);
	            return FMSH_FAILURE;
	        }
			else
			{
				/*printf("qspiFlash_Set_Segment->qspiFlash_Get_Reg8: 0x%X-0x%X(r) ok \n", extAddr, extAddrRb);*/
			}
	    }
		break;
    
    case  SPI_MICRON_ID:
    case  SPI_MACRONIX_ID:
    case  SPI_WINBOND_ID: 
    case  SPI_FMSH_ID:
    case  SPI_GD_ID:
	    {        
	        extAddr = highAddr;	        
	        err = qspiFlash_Set_Reg8(pQspi, 0xC5, extAddr);
	        if (err != 0)
	        {
	        	printf("2-qspiFlash_Set_Segment->qspiFlash_Set_Reg8 fail! \n");
	            return FMSH_FAILURE;
	        }	        
	        /* readback & verify */
	        extAddrRb = qspiFlash_Get_Reg8(pQspi, 0xC8);
	        if(extAddrRb != extAddr)
	        {
	        	printf("2-qspiFlash_Set_Segment->qspiFlash_Get_Reg8 fail: 0x%X-0x%X(r) \n", extAddr, extAddrRb);
	            return FMSH_FAILURE;
	        }   
	    }
		break;

    default:
	    {
	        return FMSH_FAILURE;
	    }
		break;
    }
    
    return FMSH_SUCCESS;
}

UINT8 qspiFlash_Get_Segment(vxT_QSPI* pQspi)
{
    UINT32 ret;
    UINT8 status;
	UINT8 cfg;
	
    switch (pQspi->pQspiFlash->maker)
    {
    case SPI_SPANSION_ID:
    case SPI_ISSI_ID:
        ret = qspiFlash_Get_Reg8(pQspi, 0x16); 
		break;
    
    case  SPI_MICRON_ID:
    case  SPI_MACRONIX_ID:
    case  SPI_WINBOND_ID: 
    case  SPI_FMSH_ID:
    case  SPI_GD_ID:
        ret = qspiFlash_Get_Reg8(pQspi, 0xC8);
		break;

    default:
	    ret = 0;
		break;
    }
    
    return ret;
}

int qspiFlash_Set_Seg_0_16M(vxT_QSPI* pQspi)
{
	if (pQspi->pQspiFlash->seg_flag == SEG_FLAG_0_16M)
	{
		return FMSH_SUCCESS;
	}	
	pQspi->pQspiFlash->seg_flag = SEG_FLAG_0_16M;
	
	return qspiFlash_Set_Segment(pQspi, SEG_3B_ADDR_0_16M);
}

int qspiFlash_Set_Seg_16_32M(vxT_QSPI* pQspi)
{
	if (pQspi->pQspiFlash->seg_flag == SEG_FLAG_16_32M)
	{
		return FMSH_SUCCESS;
	}	
	pQspi->pQspiFlash->seg_flag = SEG_FLAG_16_32M;
	
	return qspiFlash_Set_Segment(pQspi, SEG_3B_ADDR_16_32M);
}

int qspiFlash_Set_Seg_32_48M(vxT_QSPI* pQspi)
{
	if (pQspi->pQspiFlash->seg_flag == SEG_FLAG_32_48M)
	{
		/*return FMSH_SUCCESS;*/
	}	
	pQspi->pQspiFlash->seg_flag = SEG_FLAG_32_48M;
	
	return qspiFlash_Set_Segment(pQspi, SEG_3B_ADDR_32_48M);
}

int qspiFlash_Set_Seg_48_64M(vxT_QSPI* pQspi)
{
	if (pQspi->pQspiFlash->seg_flag == SEG_3B_ADDR_48_64M)
	{
		/*return FMSH_SUCCESS;*/
	}	
	pQspi->pQspiFlash->seg_flag = SEG_3B_ADDR_48_64M;
	
	return qspiFlash_Set_Segment(pQspi, SEG_3B_ADDR_48_64M);
}

void test_get_seg(void)
{
	vxT_QSPI* pQspi = g_pQspi0;
	UINT8 ret = 0;
	UINT32 id[2] = {0};
	
	ret = qspiFlash_Get_Segment(pQspi);
    switch (pQspi->pQspiFlash->maker)
    {
    case SPI_SPANSION_ID:
		printf("SPI_SPANSION_ID: ");
		break;
		
    case SPI_ISSI_ID:
		printf("SPI_ISSI_ID: ");
		break;
    
    case  SPI_MICRON_ID:
		printf("SPI_MICRON_ID: ");
		break;
	
    case  SPI_MACRONIX_ID:
		printf("SPI_MACRONIX_ID: ");
		break;
	
    case  SPI_WINBOND_ID: 
		printf("SPI_WINBOND_ID: ");
		break;
	
    case  SPI_FMSH_ID:
		printf("SPI_FMSH_ID: ");
		break;
	
    case  SPI_GD_ID:
		printf("SPI_GD_ID: ");
		break;

    default:
		printf("Unknow_ID!!! \n");
		break;
    }

	/* read id*/
	qspiFlash_ReadId_2(pQspi, id);
	printf("0x%08X-%08X ", id[0], id[1]);
	
	printf("=0x%08X seg-(%d) \n\n", ret, ret&0x03);

	return;
}

void test_set_seg(int seg_idx)
{
	vxT_QSPI* pQspi = g_pQspi0;

	switch (seg_idx)
	{
	case 0:
		qspiFlash_Set_Seg_0_16M(pQspi);
		break;
	case 1:
		qspiFlash_Set_Seg_16_32M(pQspi);
		break;
	case 2:
		qspiFlash_Set_Seg_32_48M(pQspi);
		break;
	case 3:
		qspiFlash_Set_Seg_48_64M(pQspi);
		break;
	}

	return;
}

#if 1
/*
in 3_bytes_addr mode:
1. logic idx: 0,1,2,3 ... (max-1)  [ max  = devSize/sectSize ]
2. phy_idx  : 0,1,2,3 ... (seg-1)  [ seg = 16M/sectSize      ]
3. logic -> phy:
   -------------
   logic: 0,1,2 ... (seg-1), | seg   ... (2*seg-1), | (2*seg) ... (3*seg-1), | (3*seg) ... (4*seg-1)
          -----------------  | -------------------  | ---------------------  | ---------------------
   phy  : 0,1,2 ... (seg-1), | 0,1,2 ...   (seg-1), | 0,1,2   ...   (seg-1), | 0,1,2   ...   (seg-1)
*/
int qspiFlash_SectIdx_LogtoPhy(vxT_QSPI* pQspi, int log_idx)
{
	int ret = 0;
	int seg = 0, phy_idx = 0;

	switch (pQspi->pQspiCtrl->addr_bytes_mode)
	{
	case ADDR_4_BYTES_MODE:		
		{
			phy_idx = log_idx;
		}
		break;
	
	case ADDR_3_BYTES_MODE:
		{
			/**/
			/* 16M / sectSize*/
			/**/
			seg = 0x1000000 / pQspi->pQspiFlash->sectSize;  /* 16M / 64K = 256 sects*/
			/*printf("seg_num: %d (indirect_16M/64K) => ", seg);*/
			
			if ((0 <= log_idx) && (log_idx < seg))                /* first 16M area */
			{
				phy_idx = log_idx;
				ret = qspiFlash_Set_Seg_0_16M(pQspi);
				
				/*printf("sect_log->phy: 0-seg (ret:%d) \n", ret);*/
			}
			else if ((seg <= log_idx) && (log_idx < (2*seg)))    /* second 16M area */
			{
				phy_idx = log_idx - 1*seg;
				ret = qspiFlash_Set_Seg_16_32M(pQspi);
				
				/*printf("bank_map=>sect_log->phy: 1-seg (ret:%d) \n", ret);*/
			}
			else if (((2*seg) <= log_idx) && (log_idx < (3*seg))) /* third 16M area */
			{
				phy_idx = log_idx - 2*seg;
				ret = qspiFlash_Set_Seg_32_48M(pQspi);
				
				/*printf("sect_log->phy: 2-seg (ret:%d) \n", ret);*/
			}
			else if (((3*seg) <= log_idx) && (log_idx < (4*seg))) /* fourth 16M area */
			{
				phy_idx = log_idx - 3*seg;
				ret = qspiFlash_Set_Seg_48_64M(pQspi);
				
				/*printf("sect_log->phy: 3-seg (ret:%d) \n", ret);*/
			}
		}
		break;

	default:			
		phy_idx = 0;  /* error*/
		break;
	}
	
	return phy_idx;
}

int qspiFlash_SectIdx_LogtoPhy_Only(vxT_QSPI* pQspi, int log_idx)
{
	int ret = 0;
	int seg = 0, phy_idx = 0;

	switch (pQspi->pQspiCtrl->addr_bytes_mode)
	{
	case ADDR_4_BYTES_MODE:		
		{
			phy_idx = log_idx;
		}
		break;
	
	case ADDR_3_BYTES_MODE:
		{
			/**/
			/* 16M / sectSize*/
			/**/
			seg = 0x1000000 / pQspi->pQspiFlash->sectSize;  /* 16M / 64K = 256 sects*/
			
			if ((0 <= log_idx) && (log_idx < seg))                /* first 16M area */
			{
				phy_idx = log_idx;
			}
			else if ((seg <= log_idx) && (log_idx < (2*seg)))    /* second 16M area */
			{
				phy_idx = log_idx - 1*seg;
			}
			else if (((2*seg) <= log_idx) && (log_idx < (3*seg))) /* third 16M area */
			{
				phy_idx = log_idx - 2*seg;
			}
			else if (((3*seg) <= log_idx) && (log_idx < (4*seg))) /* fourth 16M area */
			{
				phy_idx = log_idx - 3*seg;
			}
		}
		break;

	default:			
		phy_idx = 0;  /* error*/
		break;
	}
	
	return phy_idx;
}

/*#define TFFS_32M_MODE*/

int qspiFlash_Offset_LogtoPhy(vxT_QSPI* pQspi, int offset_addr)
{
	int ret = 0;
	UINT32 seg = 0, phy_addr = 0;

	switch (pQspi->pQspiCtrl->addr_bytes_mode)
	{
	case ADDR_4_BYTES_MODE:		
		{
			phy_addr = offset_addr;
		}
		break;
	
	case ADDR_3_BYTES_MODE:
		{
			/**/
			/* 16M / sectSize*/
			/**/
			/*seg = 0x1000000 / pQspi->pQspiFlash->sectSize;  // 16M / 64K = 256 sects*/
			seg = 0x1000000;  /* 16M / 64K = 256 sects*/
			/*printf("seg_num: 0x%X (direct_16M) => ", seg);*/
			
			if ((0 <= offset_addr) && (offset_addr < seg))                 /* first 16M area */
			{
				phy_addr = offset_addr;
				ret = qspiFlash_Set_Seg_0_16M(pQspi);
				
				/*printf("sect_log->phy: 0-seg (ret:%d) \n", ret);*/
			}
			else if ((seg <= offset_addr) && (offset_addr < (2*seg)))      /* second 16M area */
			{
				phy_addr = offset_addr - 1*seg;
				ret = qspiFlash_Set_Seg_16_32M(pQspi);
				
				/*printf("sect_log->phy: 1-seg (ret:%d) \n", ret);*/
			}
			else if (((2*seg) <= offset_addr) && (offset_addr < (3*seg)))   /* third 16M area */
			{
				phy_addr = offset_addr - 2*seg;
				ret = qspiFlash_Set_Seg_32_48M(pQspi);
				
				/*printf("sect_log->phy: 2-seg (ret:%d) \n", ret);*/
			}
			else if (((3*seg) <= offset_addr) && (offset_addr < (4*seg)))    /* fourth 16M area */
			{
				phy_addr = offset_addr - 3*seg;
				ret = qspiFlash_Set_Seg_48_64M(pQspi);
				
				/*printf("sect_log->phy: 3-seg (ret:%d) \n", ret);*/
			}
		}
		break;

	default:			
		phy_addr = 0;  /* error*/
		break;
	}

	return phy_addr;
}

int qspiFlash_PageIdx_LogtoPhy(vxT_QSPI* pQspi, int page_idx_log)
{

}

#endif

#define __4_bytes_addr_mode__
/*
4_bytes_address mode:
*/

/*
for Micron FlashChip: N25Q512A
*/
int qspiFlash_Enter_4BAddr(vxT_QSPI* pQspi)
{
    int error;
    UINT32 reg;

    error = qspiFlash_WREN_Cmd(pQspi);
    if (error == FMSH_FAILURE)
	{
        return FMSH_FAILURE;
    }
    
    /*Send Enter 4B Command*/
    reg = (0xB7 << 24);
    error = qspiCtrl_CmdExecute(pQspi, reg);
    if (error == FMSH_FAILURE)
	{
        return FMSH_FAILURE;
    }
	
    return FMSH_SUCCESS;
}

int qspiFlash_Exit_4BAddr(vxT_QSPI* pQspi)
{
    int error;
    UINT32 reg;

    error = qspiFlash_WREN_Cmd(pQspi);
    if (error == FMSH_FAILURE)
	{
        return FMSH_FAILURE;
    }
    
    /*Send Enter 4B Command*/
    reg = (0xE9 << 24);
    error = qspiCtrl_CmdExecute(pQspi, reg);
    if (error == FMSH_FAILURE)
	{
        return FMSH_FAILURE;
    }
	
    return FMSH_SUCCESS;
}

#endif

static void qspiCtrl_Enable(vxT_QSPI* qspi)
{
    UINT32 configReg;
    configReg = qspiCtrl_Rd_CfgReg(qspi, QSPIPS_CFG_OFFSET);
    configReg |= QSPIPS_ENABLE_MASK;
    qspiCtrl_Wr_CfgReg(qspi, QSPIPS_CFG_OFFSET, configReg);
}

static void qspiCtrl_Disable(vxT_QSPI* qspi)
{
    UINT32 configReg;
    configReg = qspiCtrl_Rd_CfgReg(qspi, QSPIPS_CFG_OFFSET);
    configReg &= ~QSPIPS_ENABLE_MASK;
    qspiCtrl_Wr_CfgReg(qspi, QSPIPS_CFG_OFFSET, configReg);
}

static int qspiCtrl_Calibration(vxT_QSPI* qspiPtr)
{
    int i;
    UINT32 id_temp = 0;
    int range_lo = -1, range_hi = -1;
	UINT32 flash_ID[2] = {0};
	vxT_QSPI_FLASH * pQspiFlash = qspiPtr->pQspiFlash;
    
    for(i=0; i<16; i++)
    {
        qspiCtrl_Disable(qspiPtr);
        qspiCtrl_Set_CaptureDelay(qspiPtr, i);
        qspiCtrl_Enable(qspiPtr);
        /*id_temp = FQspiFlash_ReadId(qspiPtr);*/
        qspiFlash_ReadId_2(qspiPtr, (UINT32 *)(&flash_ID[0]));
        /* search for lowest value of delay */
        if((range_lo == -1) && (pQspiFlash->flash_ID1 == flash_ID[0]))
        {
            range_lo = i;
            continue;
        }
        /* search for highest value of delay */
        if((range_lo != -1) && (pQspiFlash->flash_ID1 != flash_ID[0]))
        {
            range_hi = i-1;
            break;
        }
        range_hi = i;
    }
    
    if(range_lo == -1)
    {
		VX_DBG("qspiCtrl_Calibration failed! range_lo %d, range_hi %d\n", range_lo,range_hi, 3,4,5,6);
        return FMSH_FAILURE;
    }
    
    qspiCtrl_Disable(qspiPtr);
    qspiCtrl_Set_CaptureDelay(qspiPtr, (range_hi + range_lo) / 2);
    qspiCtrl_Enable(qspiPtr);
    VX_DBG("qspiCtrl_Calibration: QSPI Read data capture delay calibrate to %i (%i - %i)\n", 
              (range_hi + range_lo) / 2, range_lo, range_hi,4,5,6);
    
    return FMSH_SUCCESS;
}

#if 1

/*
not vxBus 
metal-dirver in vxworks
*/
int vxInit_Qspi (int ctrl_x)
{
	vxT_QSPI * pQspi = NULL;
	vxT_QSPI_CTRL * pQspiCtrl = NULL;
	vxT_QSPI_FLASH * pQspiFlash = NULL;
	
	UINT32 flash_ID[2] = {0};
	int ret = 0;
	u8 status = 0;
	
	UINT32 tmp32 = 0;
   /* int timeout = QSPI_POLL_TIMEOUT;*/
   
    int mode = 0, bdr = 0, sampleDelay = 0;

#if 1 
    switch (ctrl_x)
	{
	case QSPI_CTRL_0:
		pQspi = g_pQspi0;
		break;
	
	case QSPI_CTRL_1:
		pQspi = g_pQspi1;
		break;
    }
	
	if (pQspi->init_flag == QSPI_CTRL_INIT_OK)
	{
		return FMSH_SUCCESS;  /* qspi_ctrl init already*/
	}
	
	/*
	init the pQspi ctrl default params
	*/
	qspiCtrl_params_init();

	/*
	init the pQspi structure
	*/
	/**/
	/* sdmmc_ctrl select*/
	/**/
    switch (ctrl_x)
	{
	case QSPI_CTRL_0:
		/*pQspi = g_pQspi0;*/
		pQspi->qspi_x = QSPI_CTRL_0;
		pQspi->pQspiCtrl = (vxT_QSPI_CTRL *)(&vxQspi_Ctrl_0.ctrl_x);
		pQspi->pQspiFlash = (vxT_QSPI_FLASH *)(&vxQspi_Flash_0.status);

		pQspi->pQspiCtrl->devId = QSPI_CTRL_0;
		pQspi->pQspiCtrl->cfgBaseAddr = VX_QSPI_0_CFG_BASE;
		pQspi->pQspiCtrl->fifoBaseAddr = VX_QSPI_0_FIFO_BASE;
		break;
		
	case QSPI_CTRL_1:
		/*pQspi = g_pQspi1;*/
		pQspi->qspi_x = QSPI_CTRL_1;
		pQspi->pQspiCtrl = (vxT_QSPI_CTRL *)(&vxQspi_Ctrl_1.ctrl_x);
		pQspi->pQspiFlash = (vxT_QSPI_FLASH *)(&vxQspi_Flash_1.status);

		pQspi->pQspiCtrl->devId = QSPI_CTRL_1;
		pQspi->pQspiCtrl->cfgBaseAddr = VX_QSPI_1_CFG_BASE;
		pQspi->pQspiCtrl->fifoBaseAddr = VX_QSPI_1_FIFO_BASE;		
		break;
	}
	
	pQspiCtrl = pQspi->pQspiCtrl;
	pQspiFlash = pQspi->pQspiFlash;
#endif		

	pQspiCtrl->ctrl_x = ctrl_x;

	pQspiCtrl->isBusy = 0; /* no busy */

	pQspiCtrl->sndBuf = NULL;
	pQspiCtrl->rcvBuf = NULL;
	pQspiCtrl->requestBytes = 0;
	pQspiCtrl->remainBytes = 0;
	
	pQspiFlash->maker = 0;				            /**< Flash maker */
	
	/*pQspiFlash->devSize = SZ_32M;	                /** Flash device size in bytes: QSPIFLASH_SIZE_32M -> 8M */
	/*pQspiFlash->sectSize = QSPIFLASH_SECTOR_64K;    * Flash sector size in bytes  */
	/*pQspiFlash->pageSize = 0x100;  /* page size: 256 bytes */

    /* 
    reset controller 
	*/
	qspiCtrl_Reset(pQspi);
	taskDelay(1);

	/* set low baud rate*/
	bdr = 8;
	qspiCtrl_Set_BaudRate(pQspi, QSPIPS_BAUD(bdr));
	
#if 0
	/*
	read flash id
	*/
	flash_ID[0] = qspiFlash_ReadId();
	VX_DBG("flash ID1: 0x%08X \n", flash_ID[0], 2,3,4,5,6);

	/*
	parse flash info: devSize, sectSize
	*/
	ret = qspiFlash_ParseInfo(pQspiFlash, flash_ID[0]);
    if (ret == FMSH_FAILURE)
    {
		VX_DBG2("ctrl_%d: qspiFlash_ParseInfo fail! \n", ctrl_x, 2,3,4,5,6);
        return FMSH_FAILURE;
    } 	
#else
	/*
	read flash id
	*/
	qspiFlash_ReadId_2(pQspi, (UINT32 *)(&flash_ID[0]));
	/* 
	Vendor and Device ID : S25FL256S: 0x4D190201_30528001,  Spansion 
	                       N25Q512A : 0x1020BA20_64230000,  MICRON
	                       W25Q256FV: 0x001940EF_00000000,  Winbond
	*/
	VX_DBG("flash ID1_2: 0x%08X_%08X \n", flash_ID[0],flash_ID[1], 3,4,5,6);


	/*
	parse flash info: devSize, sectSize
	*/	
	ret = qspiFlash_ParseInfo_2(pQspi, (UINT32 *)(&flash_ID[0]));
    if (ret == FMSH_FAILURE)
    {
		VX_DBG2("ctrl_%d: qspiFlash_ParseInfo fail! \n", ctrl_x, 2,3,4,5,6);
        return FMSH_FAILURE;
    }
	else
	{
		VX_DBG("ctrl_%d: qspiFlash_ParseInfo: ", ctrl_x, 2,3,4,5,6);
		VX_DBG("devSize:%d MB, sectSize:%d KB \n", (pQspiFlash->devSize/0x100000), (pQspiFlash->sectSize/1024),3,4,5,6);
	}
#endif

#if 0 /* for test tffs*/
	/*pQspiFlash->devSize = 0x1000000;  16M for test*/
	pQspiFlash->devSize = QSPI_FLASH_MEM_SIZE;  /* for test tffs*/
#endif

	/**/
	/* reset flash device*/
	/**/
	qspiFlash_Reset_Flash(pQspi);

	/**/
	/* unLock Flash*/
	/**/
	/*qspiFlash_Unlock_Flash(pQspi);*/

	/**/
	/* set 4bytes_mode or 3bytes_mode*/
	/**/
	pQspiCtrl->addr_bytes_mode = gQspi_Ctrl_Params.addrBytesMode;	

	/**/
	/* FAST_READ_CMD: 1-line_cmd*/
	/* QOR_CMD: 4-line_cmd*/
	/**/
	ret = qspiCtrl_SetFlashMode(pQspi, QOR_CMD, pQspiCtrl->addr_bytes_mode);
    if (ret == FMSH_FAILURE)
    {
		VX_DBG2("ctrl_%d: qspiFlash_SetMode fail! \n", ctrl_x, 2,3,4,5,6);
        return FMSH_FAILURE;
    }

	VX_DBG("addr_bytes_mode:%d \n", pQspiCtrl->addr_bytes_mode, 2,3,4,5,6);
	
	/*
	bit22~19:	R/W 涓绘満妯″紡涓嬫尝鐗圭巼鍒嗛(2鍒�2)
	--------
	SPI娉㈢壒鐜�= (涓绘満 reference clock) / BD
	BD= 4'h0  /2
		4'h1  /4
		4'h2  /6 
		鈥�
		4'hf  /32
	*/	
	/* 2->100M; */
	/* 8->25M; */
	/* 4->50M	*/
	/* 200MHz/bdr = x MHz*/
	bdr = gQspi_Ctrl_Params.baudRate;   /* 4*/
	qspiCtrl_Set_BaudRate(pQspi, QSPIPS_BAUD(bdr));

	/* set the capture delay autoly */
	
	qspiCtrl_Calibration(pQspi);
	
	/**/
	/*  .sampleDelay = 2,*/
	/**/
	/* sampleDelay = gQspi_Ctrl_Params.sampleDelay;  
	qspiCtrl_Set_CaptureDelay(pQspi, sampleDelay);	*/

	/**/
	/* unlock flash protect*/
	/*	*/
    ret = qspiFlash_Get_RegStatus1(pQspi, &status);
    if (ret == FMSH_FAILURE)
    {
    	printf("read flash reg status failed! \n");
    	
    	return FMSH_FAILURE;
    }
    
    if (status & 0x1c)
    {    
		qspiFlash_Unlock_Flash(pQspi);
    }


	/**/
	/* set flash quad-line mode*/
	/**/
	if (qspiFlash_Get_QuadMode(pQspi) == FLASH_1_LINE_MODE)
	{
		printf("flash interface: 1-line mode! \n");
		
		if (FMSH_SUCCESS == qspiFlash_Set_QuadMode(pQspi))
		{
			printf("flash interface: set 4-line mode ok! \n");
		}
		/*
		*/
	}
	else
	{
		printf("flash interface: 4-line mode! \n");
	}

	/* for Micron: 256M bit flash, and 4_bytes_addr mode*/
	if (pQspi->pQspiFlash->flagStatus & QSPI_FSR)
	{
		if (pQspi->pQspiCtrl->addr_bytes_mode == ADDR_4_BYTES_MODE)
		{
			qspiFlash_Enter_4BAddr(pQspi);
		}
		else  /* 3_bytes_addr mode*/
		{
			qspiFlash_Exit_4BAddr(pQspi);
		}
	}
	/*
	*/
	
	pQspi->sem = semMCreate (SEM_Q_PRIORITY | SEM_DELETE_SAFE | SEM_INVERSION_SAFE);
	
	pQspi->init_flag = QSPI_CTRL_INIT_OK;

	return OK;
}

#endif


#if 1

#define __Direct_InDirect_Mode__

/*
erase the whole flash chip
*/
/* need more test*/
int qspiFlash_Erase_Chip(vxT_QSPI* pQspi)
{
    int error;
    UINT32 reg;
	
	UINT8 cmd = 0;
    
    error = qspiFlash_WREN_Cmd(pQspi);
    if (error == FMSH_FAILURE)
	{
        return FMSH_FAILURE;
    }
    
    /*Send Erase Command*/
    cmd = 0xC4; /* 0x60 C7*/
    reg = 0xC40A0000;/*((UINT32)cmd << 24);*/
    qspiCtrl_Wr_CfgReg(pQspi, QSPIPS_FCAR_OFFSET, 0);
	
    error = qspiCtrl_CmdExecute(pQspi, reg);
    if (error == FMSH_FAILURE)
	{
        return FMSH_FAILURE;
    }
    
    error = qspiFlash_Wait_FlashReady(pQspi, 6000000);
    if (error == FMSH_FAILURE)
	{
		return FMSH_FAILURE;
	}

    return FMSH_SUCCESS;
}

/*****************************************************************************
* This function executes SECTOR ERASE.
*
* @param
*       - sect_idx: sector or block idxL: 0,1,2,3....(max-1)
* @return
*		- FMSH_SUCCESS if success.
*		- FMSH_FAILURE if failure.
*
* @note:	
*     default - 64K sector	
*
******************************************************************************/

int qspiFlash_Erase_Sect(vxT_QSPI* pQspi, int sect_idx)
{
	vxT_QSPI_CTRL * pQspiCtrl = pQspi->pQspiCtrl;
	vxT_QSPI_FLASH * pQspiFlash = pQspi->pQspiFlash;

    int ret = FMSH_SUCCESS;
    UINT32 sect_offset = 0, reg = 0;	
	int phy_idx = 0;
	
    QSPI_SEM_TAKE(pQspi->sem);

	phy_idx = qspiFlash_SectIdx_LogtoPhy(pQspi, sect_idx);
	
    /* 
    calculate sector start address to be erased 
	*/
    sect_offset = (phy_idx * pQspiFlash->sectSize);
    qspiCtrl_Wr_CfgReg(pQspi, QSPIPS_FCAR_OFFSET, sect_offset);
    
    ret = qspiFlash_WREN_Cmd(pQspi);
    if (ret == FMSH_FAILURE)
	{
        goto ERASE_END;
    }

	/* Erase Operations */
	/*
	#define SUBSECTOR_ERASE_CMD                  0x20
	#define SUBSECTOR_ERASE_4_BYTE_ADDR_CMD      0x21

	#define SECTOR_ERASE_CMD                     0xD8
	#define SECTOR_ERASE_4_BYTE_ADDR_CMD         0xDC

	#define BULK_ERASE_CMD                       0xC7
	*/
	
    /* Send Erase Command*/
    if (pQspi->pQspiCtrl->addr_bytes_mode == ADDR_4_BYTES_MODE)
    {    
    	reg = 0xDC0B0000; /* 64k*/
    }
	else
    {    
    	reg = 0xD80A0000;  /* 64k*/
    }	
	
    ret = qspiCtrl_CmdExecute(pQspi, reg);
    if (ret == FMSH_FAILURE)
	{
        goto ERASE_END;
    }
    
    ret = qspiFlash_Wait_FlashReady(pQspi, 5000);
    if (ret == FMSH_FAILURE)
	{
        goto ERASE_END;
    }

ERASE_END:	
    QSPI_SEM_GIVE(pQspi->sem);
    
    return ret;
}



#endif

#if 1  /* InDirect mode*/

#define __InDirect_Fifo_Mode__

/*****************************************************************************
* This function read flash using indirect mode
*
* @param
*       - offset is a value where data read 
*       - byteCount is a number of bytes to read
*       - recvBuffer is a point to read data
*
* @return
*		- FMSH_SUCCESS if success.
*		- FMSH_FAILURE if failure.
*
* @note		
*
******************************************************************************/
int qspiCtrl_FastRcv_Setup(vxT_QSPI* pQspi, UINT32 flash_offset, UINT32 byteCnt, UINT8* rcvBuf)
{
	vxT_QSPI_CTRL * pQspiCtrl = pQspi->pQspiCtrl;
		
    UINT32 cfgReg = 0;
    UINT32 rxFullLvl = 480;    
    
    /*
    if (pQspiCtrl->isBusy == TRUE)
    {
        return FMSH_FAILURE;
    }	
    pQspiCtrl->isBusy = TRUE;
	*/
    QSPI_SEM_TAKE(pQspi->sem);
    
    pQspiCtrl->requestBytes = byteCnt;
    pQspiCtrl->remainBytes = byteCnt;
    pQspiCtrl->rcvBuf = rcvBuf;
    
    /* 
    set flash address where read data (address in flash) 
	*/
    qspiCtrl_Wr_CfgReg(pQspi, QSPIPS_IRTSAR_OFFSET, flash_offset);  
	
    /* 
    set number of bytes to be transferred 
	*/
    qspiCtrl_Wr_CfgReg(pQspi, QSPIPS_IRTNBR_OFFSET, pQspiCtrl->requestBytes);  
	
    /* 
    set controller trig adress where read data (adress in controller) 
	*/
    qspiCtrl_Wr_CfgReg(pQspi, QSPIPS_IAATR_OFFSET, pQspiCtrl->fifoBaseAddr);    
	
    /* 
    set Trigger Address Range (2^15) 
	*/
    qspiCtrl_Wr_CfgReg(pQspi, QSPIPS_ITARR_OFFSET, 0x0F);  
	
    /* 
    set WaterMark Register 
	*/
    qspiCtrl_Wr_CfgReg(pQspi, QSPIPS_IWTWR_OFFSET, 0x00); 	  /* tx*/
	rxFullLvl = gQspi_Ctrl_Params.rxFullLvl;
    qspiCtrl_Wr_CfgReg(pQspi, QSPIPS_IRTWR_OFFSET, rxFullLvl);  /* rx*/
	
    /* 
    trigger Indirect Write access
    */
    cfgReg = 0x01;
    qspiCtrl_Wr_CfgReg(pQspi, QSPIPS_IRTCR_OFFSET, cfgReg);  
	
    /*pQspiCtrl->isBusy = FALSE;  /* jc     */
    QSPI_SEM_GIVE(pQspi->sem);
	
    return FMSH_SUCCESS;
}


int qspiCtrl_FastSnd_Setup(vxT_QSPI* pQspi, UINT32 flash_offset, UINT32 byteCnt, UINT8* sndBuf)
{
	vxT_QSPI_CTRL * pQspiCtrl = pQspi->pQspiCtrl;
		
    UINT32 cfgReg = 0;
    UINT32 txEmptyLvl = 32;    
    
    /*
    if (pQspiCtrl->isBusy == TRUE)
    {
        return FMSH_FAILURE;
    }	
    pQspiCtrl->isBusy = TRUE;
	*/	
    QSPI_SEM_TAKE(pQspi->sem);
    
    pQspiCtrl->requestBytes = byteCnt;
    pQspiCtrl->remainBytes = byteCnt;
    pQspiCtrl->sndBuf = sndBuf;
    
    /* 
    set flash address where write data (address in flash) 
	*/
    qspiCtrl_Wr_CfgReg(pQspi, QSPIPS_IWTSAR_OFFSET, flash_offset);  
	
    /* 
    set number of bytes to be transferred 
	*/
    qspiCtrl_Wr_CfgReg(pQspi, QSPIPS_IWTNBR_OFFSET, pQspiCtrl->requestBytes);  
	
    /* 
    set controller trig adress where write data (adress in controller) 
	*/
    qspiCtrl_Wr_CfgReg(pQspi, QSPIPS_IAATR_OFFSET, pQspiCtrl->fifoBaseAddr);    
	
    /* 
    set Trigger Address Range (2^15) 
	*/
    qspiCtrl_Wr_CfgReg(pQspi, QSPIPS_ITARR_OFFSET, 0xf);      
	
    /* 
    set WaterMark Register (between 10~250 if page_size is 256 bytes) 
	*/
    qspiCtrl_Wr_CfgReg(pQspi, QSPIPS_IRTWR_OFFSET, 0x00);   /* rx*/
	txEmptyLvl = gQspi_Ctrl_Params.txEmptyLvl;
    qspiCtrl_Wr_CfgReg(pQspi, QSPIPS_IWTWR_OFFSET, txEmptyLvl);   /* tx*/
	
    /* 
    trigger Indirect Write access 
	*/
    cfgReg = 0x01;
    qspiCtrl_Wr_CfgReg(pQspi, QSPIPS_IWTCR_OFFSET, cfgReg);
    
    /*pQspiCtrl->isBusy = FALSE;  /* jc 	*/
    QSPI_SEM_GIVE(pQspi->sem);
	
    return FMSH_SUCCESS;
}

int qspiFlash_Rx_Data(vxT_QSPI* pQspi, UINT32 offset, UINT32 byteCnt, UINT8* rcvBuf)
{
	vxT_QSPI_CTRL * pQspiCtrl = pQspi->pQspiCtrl;
	vxT_QSPI_FLASH * pQspiFlash = pQspi->pQspiFlash;
	
	int ret = 0;
	UINT32 wdCnt = 0, remainder = 0;
	UINT8 * pBuf8 = NULL;
	UINT32 tmp32 = 0;
	int idx = 0;
	int timeout = 0, j = 0;
	UINT32 cfgReg = 0;
		
	/**/
	/* clear Rcv Fifo*/
	/**/
	qspiCtrl_Clr_RcvFifo(pQspi);

	/*
	setup fastRcv
	*/
	qspiCtrl_FastRcv_Setup(pQspi, offset, byteCnt, rcvBuf);

	do
	{
		/*read progress*/
		cfgReg = qspiCtrl_Rd_CfgReg(pQspi, QSPIPS_IRTCR_OFFSET);
		
		if (cfgReg & 0x4)
		{
			/* Read  Watermark */
			wdCnt = qspiCtrl_Rd_CfgReg(pQspi, QSPIPS_SFLR_OFFSET);
			remainder = pQspiCtrl->requestBytes & 0x3;
			
			/* Read data from SRAM */
			for (idx=0; idx<wdCnt; idx++)
			{
				if (pQspiCtrl->requestBytes >= 4)
				{
					*(UINT32*)(pQspiCtrl->rcvBuf) = *(UINT32*)(pQspiCtrl->fifoBaseAddr);
					
					pQspiCtrl->rcvBuf += 4;
					pQspiCtrl->requestBytes -= 4;  
				}
				else
				{					 
					if (remainder == 0)
					{	
						break;
					}
					
					tmp32 = *(UINT32*)(pQspiCtrl->fifoBaseAddr);
					while (remainder > 0)
					{
						*(UINT8*)(pQspiCtrl->rcvBuf) = tmp32 & 0xff;
						tmp32 = tmp32 >> 8;
						
						pQspiCtrl->rcvBuf++;						
						pQspiCtrl->remainBytes--; 
						remainder--;
					}
					
					break;  /* for()*/
				}
			}/* for ()*/
		}/* if (cfgReg & 0x4)`*/
	} while (cfgReg & 0x4);


	timeout = QSPI_TIMEOUT;
	do 
	{
		ret = qspiCtrl_Wait_ExeOk(pQspi, QSPIPS_IRTCR_OFFSET, QSPIPS_IRTCR_COMPLETE_MASK, OK_FLAG_1);
		if (ret == FMSH_FAILURE)
		{
			sysUsDelay(10);		
		}
		else
		{
			break;
		}
		
		timeout--;
		if (timeout <= 0)
		{
			byteCnt = 0;
		}
	} while (timeout > 0);

	/* 
	clear QSPI_IND_RD_XFER_DONE bit 
	*/
	qspiCtrl_Wr_CfgReg(pQspi, QSPIPS_IRTCR_OFFSET, QSPIPS_IRTCR_COMPLETE_MASK);

	return byteCnt;
}

int qspiFlash_Tx_Data(vxT_QSPI* pQspi, UINT32 offset, UINT32 byteCnt, UINT8* sndBuf)
{
	vxT_QSPI_CTRL * pQspiCtrl = pQspi->pQspiCtrl;
	vxT_QSPI_FLASH * pQspiFlash = pQspi->pQspiFlash;
		
	int ret = 0;
	UINT32 wdCnt = 0, remainder = 0;
	UINT32 tmp32 = 0;
	int idx = 0;
	int timeout = 0, j = 0;	
	UINT32 cfgReg = 0;	
	
	/**/
	/* UnLock flash */
	/**/
	qspiFlash_Unlock_Flash(pQspi);

	/*
	setup fastSnd
	*/
	qspiCtrl_FastSnd_Setup(pQspi, offset, byteCnt, sndBuf);

	/* 
	copy data from sram to flash 
	*/	
	do
	{
		cfgReg = qspiCtrl_Rd_CfgReg(pQspi, QSPIPS_IWTCR_OFFSET);
		
		if (cfgReg & 0x4)
		{
			/*wdCnt = QSPIFLASH_PAGE_256 << 2;*/
			wdCnt = QSPIFLASH_PAGE_256 >> 2;
			remainder = pQspiCtrl->remainBytes & 0x3;
			
			for (idx=0; idx<wdCnt; idx++)
			{
				if (pQspiCtrl->remainBytes >= 4)
				{
					*((UINT32*)(pQspiCtrl->fifoBaseAddr)) = *((UINT32*)(pQspiCtrl->sndBuf));
					
					pQspiCtrl->sndBuf += 4;
					pQspiCtrl->remainBytes -= 4; 
				}
				else
				{
					tmp32 = 0xffffffff;
					
					if (remainder == 0)
					{	
						break; /* for()*/
					}
					
					while (remainder > 0)
					{
						tmp32 = tmp32 << 8;
						tmp32 = tmp32 | *((UINT8*)(pQspiCtrl->sndBuf + remainder - 1));
						
						pQspiCtrl->remainBytes -= 1; 
						remainder -= 1;
					}
					
					*((UINT32*)(pQspiCtrl->fifoBaseAddr)) = tmp32;				
					break;  /* for()*/
				}
			}/* for ()*/
		}/* if (cfgReg & 0x4)*/
	} while (cfgReg & 0x4);

	timeout = QSPI_TIMEOUT;
	do 
	{
		ret = qspiCtrl_Wait_ExeOk(pQspi, QSPIPS_IWTCR_OFFSET, QSPIPS_IWTCR_COMPLETE_MASK, OK_FLAG_1);
		if (ret == FMSH_FAILURE)
		{
			sysUsDelay(10);		
		}
		else
		{
			break;
		}
		
		timeout--;
		if (timeout <= 0)
		{
			byteCnt = 0;
		}
	} while (timeout > 0);

	/* 
	clear QSPI_IND_WR_XFER_DONE bit 
	*/
	qspiCtrl_Wr_CfgReg(pQspi, QSPIPS_IWTCR_OFFSET, QSPIPS_IWTCR_COMPLETE_MASK);

	return byteCnt;
}


#define __qspi_flash_api__

/*
sect_idx: 0,1,2,3 ... ...
*/
int qspiFlash_Read_Sect(vxT_QSPI* pQspi, int sect_idx, UINT8* rcvBuf8)
{
	vxT_QSPI_FLASH * pQspiFlash = pQspi->pQspiFlash;
	
	UINT32 offset = 0;
	UINT32 byteCnt = 0;	
	int phy_idx = 0;	

	phy_idx = qspiFlash_SectIdx_LogtoPhy(pQspi, sect_idx);
	
	offset = phy_idx * pQspiFlash->sectSize;
	byteCnt = pQspiFlash->sectSize;	
	
	return qspiFlash_Rx_Data(pQspi, offset, byteCnt, rcvBuf8);
}

/*
sect_idx: 0,1,2,3 ... ...
*/
int qspiFlash_Write_Sect(vxT_QSPI* pQspi, int sect_idx, UINT8 * sndBuf8)
{
	vxT_QSPI_FLASH * pQspiFlash = pQspi->pQspiFlash;

	UINT32 offset = 0;
	UINT32 byteCnt = 0;
	int phy_idx = 0;	

	phy_idx = qspiFlash_SectIdx_LogtoPhy(pQspi, sect_idx);
	
	offset = phy_idx * pQspiFlash->sectSize;
	byteCnt = pQspiFlash->sectSize;	
	
	return qspiFlash_Tx_Data(pQspi, offset, byteCnt, sndBuf8);
}

/*
page_idx: 0,1,2,3 ... ...
*/
int qspiFlash_Read_Page(vxT_QSPI* pQspi, int page_idx, UINT8* rcvBuf8)
{
	vxT_QSPI_FLASH * pQspiFlash = pQspi->pQspiFlash;
	
	UINT32 offset = 0;
	UINT32 byteCnt = 0;
	
	offset = page_idx * pQspiFlash->pageSize;
	byteCnt = pQspiFlash->pageSize;	
	
	return qspiFlash_Rx_Data(pQspi, offset, byteCnt, rcvBuf8);
	
#if 0	

	/**/
	/* clear Rcv Fifo*/
	/**/
	qspiCtrl_Clr_RcvFifo(pQspi);

	/*
	setup fastRcv
	*/
	qspiCtrl_FastRcv_Setup(pQspi, offset, byteCnt, rcvBuf8);
	
	pBuf8 = (UINT8 *)pQspiCtrl->fifoBaseAddr;
	idx = 0;	
	do 
	{
		/* 	Read  Watermark */
		dataCnt = qspiCtrl_Rd_CfgReg(pQspi, QSPIPS_SFLR_OFFSET) * 4;		
		if (dataCnt == 0)
		{
			delay_1us();
		}
		else
		{		
			/* 
			Read data from SRAM 
			*/
			memcpy((UINT8*)pQspiCtrl->rcvBuf, (UINT8*)(&pBuf8[idx]), dataCnt);
			idx += dataCnt;
			
			pQspiCtrl->rcvBuf += dataCnt;
			pQspiCtrl->remainBytes -= dataCnt;
		}
	} while (pQspiCtrl->remainBytes > 0);
	
	timeout = QSPI_TIMEOUT;
	do 
	{
		ret = qspiCtrl_Wait_ExeOk(pQspi, QSPIPS_IRTCR_OFFSET, QSPIPS_IRTCR_COMPLETE_MASK, OK_FLAG_1);
		if (ret == FMSH_FAILURE)
		{
			sysUsDelay(10);		
		}
		else
		{
			break;
		}
		
		timeout--;
		if (timeout <= 0)
		{
			byteCnt = 0;
		}
	} while (timeout > 0);

	/* 
	clear QSPI_IND_RD_XFER_DONE bit 
	*/
	qspiCtrl_Wr_CfgReg(pQspi, QSPIPS_IRTCR_OFFSET, QSPIPS_IRTCR_COMPLETE_MASK);

	return byteCnt;
#endif
}

/*
page_idx: 0,1,2,3 ... ...
*/
int qspiFlash_Write_Page(vxT_QSPI* pQspi, int page_idx, UINT8* sndBuf8)
{
	vxT_QSPI_FLASH * pQspiFlash = pQspi->pQspiFlash;
		
	UINT32 offset = 0;
	UINT32 byteCnt = 0;
	
	offset = page_idx * pQspiFlash->pageSize;
	byteCnt = pQspiFlash->pageSize;	
	
	return qspiFlash_Tx_Data(pQspi, offset, byteCnt, sndBuf8);
}


int qspiFlash_RcvBytes_Indirect(vxT_QSPI* pQspi, UINT32 addr, int len, UINT8* rcvBuf8)
{
	vxT_QSPI_FLASH * pQspiFlash = pQspi->pQspiFlash;
	int ret = 0;
	UINT32 offset = 0;
	UINT32 byteCnt = 0;
	
	int phy_idx = 0;	
	int log_idx = 0;

	int start_bank = 0, end_bank = 0;
	UINT32 addr1 = 0, addr2 = 0;
	int len1 = 0, len2 = 0;

	if (pQspi->pQspiCtrl->addr_bytes_mode ==  ADDR_4_BYTES_MODE)
	{
		offset = addr;
		byteCnt = len;	
		
		QSPI_SEM_TAKE(pQspi->sem);
		ret = qspiFlash_Rx_Data(pQspi, offset, byteCnt, rcvBuf8);		
		QSPI_SEM_GIVE(pQspi->sem);
		
		if (ret > 0)
		{
			return FMSH_SUCCESS;
		}
		else
		{
			return FMSH_FAILURE;
		}
	}
	else
	{		
		start_bank = addr / SIZE_16M;  /* 16M / bank*/
		end_bank = (addr + len) / SIZE_16M;  /* 16M / bank*/

		/*if (start_bank == end_bank)*/
		{
			/*log_idx = addr / pQspiFlash->sectSize;*/
			/*phy_idx = qspiFlash_SectIdx_LogtoPhy(pQspi, log_idx);				*/
			/*offset = (phy_idx *pQspiFlash->sectSize) + (addr % pQspiFlash->sectSize);			*/
			offset = qspiFlash_Offset_LogtoPhy(pQspi, addr);
			
			byteCnt = len;	

			/*printf("rd sect_idx=>logic:%d->phy:%d \n", log_idx, phy_idx);			*/
			/*printf("wr sect_idx=>logic:0x%X->phy:0x%X(len-%d) \n", addr, offset, len);*/
			
		    QSPI_SEM_TAKE(pQspi->sem);
			ret = qspiFlash_Rx_Data(pQspi, offset, byteCnt, rcvBuf8);		
		    QSPI_SEM_GIVE(pQspi->sem);
		}
/*
		else
		{	
			// len1-area
			addr1 = addr, 
			len1 = ((start_bank + 1) * SIZE_16M) - addr; 
			
			log_idx = addr1 / pQspiFlash->sectSize;
			phy_idx = qspiFlash_SectIdx_LogtoPhy(pQspi, log_idx);			
			offset = (phy_idx * pQspiFlash->sectSize) + (addr1 % pQspiFlash->sectSize);
			byteCnt = len1;
			
		    QSPI_SEM_TAKE(pQspi->sem);			
			ret = qspiFlash_Rx_Data(pQspi, offset, byteCnt, rcvBuf8);			
		    QSPI_SEM_GIVE(pQspi->sem);

			// len2-area
			addr2 = (start_bank + 1) * SIZE_16M;
			len2 = (addr + len) - ((start_bank + 1) * SIZE_16M);
			
			log_idx = addr2 / pQspiFlash->sectSize;
			phy_idx = qspiFlash_SectIdx_LogtoPhy(pQspi, log_idx);			
			offset = (phy_idx * pQspiFlash->sectSize) + (addr2 % pQspiFlash->sectSize);
			byteCnt = len2;
			
			QSPI_SEM_TAKE(pQspi->sem);
			rcvBuf8 += len1;
			ret = qspiFlash_Rx_Data(pQspi, offset, byteCnt, rcvBuf8);			
			QSPI_SEM_GIVE(pQspi->sem);
		}
*/
		if (ret > 0)
		{
			return FMSH_SUCCESS;
		}
		else
		{
			return FMSH_FAILURE;
		}
	}	

}

int qspiFlash_SndBytes_Indirect(vxT_QSPI* pQspi, UINT32 addr, int len, UINT8* sndBuf8)
{
	vxT_QSPI_FLASH * pQspiFlash = pQspi->pQspiFlash;
	int ret = 0;
	UINT32 offset = 0;
	UINT32 byteCnt = 0;
	
	int phy_idx = 0;	
	int log_idx = 0;
	int start_bank = 0, end_bank = 0;
	UINT32 addr1 = 0, addr2 = 0;
	int len1 = 0, len2 = 0;
	
	if (pQspi->pQspiCtrl->addr_bytes_mode ==  ADDR_4_BYTES_MODE)
	{
		offset = addr;
		byteCnt = len;	
		
		QSPI_SEM_TAKE(pQspi->sem);	
		ret = qspiFlash_Tx_Data(pQspi, offset, byteCnt, sndBuf8);		
		QSPI_SEM_GIVE(pQspi->sem);
		
		if (ret > 0)
		{
			return FMSH_SUCCESS;
		}
		else
		{
			return FMSH_FAILURE;
		}
	}
	else
	{
		start_bank = addr / SIZE_16M;  /* 16M / bank*/
		end_bank = (addr + len) / SIZE_16M;  /* 16M / bank*/

		/*if (start_bank == end_bank)*/
		{
			/*log_idx = addr / pQspiFlash->sectSize;*/
			/*phy_idx = qspiFlash_SectIdx_LogtoPhy(pQspi, log_idx);				*/
			/*offset = (phy_idx *pQspiFlash->sectSize) + (addr % pQspiFlash->sectSize);*/
			offset = qspiFlash_Offset_LogtoPhy(pQspi, addr);
			
			byteCnt = len;	
			
			/*printf("wr sect_idx=>logic:%d->phy:%d(len-%d) \n", log_idx, phy_idx, len);*/
			/*printf("wr sect_idx=>logic:0x%X->phy:0x%X(len-%d) \n", addr, offset, len);*/
			
		    QSPI_SEM_TAKE(pQspi->sem);	
			ret = qspiFlash_Tx_Data(pQspi, offset, byteCnt, sndBuf8);		
		    QSPI_SEM_GIVE(pQspi->sem);
		}
/*
		else
		{	
			addr1 = addr, 
			len1 = ((start_bank + 1) * SIZE_16M) - addr; 
			
			log_idx = addr1 / pQspiFlash->sectSize;
			phy_idx = qspiFlash_SectIdx_LogtoPhy(pQspi, log_idx);			
			offset = (phy_idx * pQspiFlash->sectSize) + (addr1 % pQspiFlash->sectSize);
			byteCnt = len1;
			
		    QSPI_SEM_TAKE(pQspi->sem);			
			ret = qspiFlash_Tx_Data(pQspi, offset, byteCnt, sndBuf8);			
		    QSPI_SEM_GIVE(pQspi->sem);

			addr2 = (start_bank + 1) * SIZE_16M;
			len2 = (addr + len) - ((start_bank + 1) * SIZE_16M);
			
			log_idx = addr2 / pQspiFlash->sectSize;
			phy_idx = qspiFlash_SectIdx_LogtoPhy(pQspi, log_idx);			
			offset = (phy_idx * pQspiFlash->sectSize) + (addr2 % pQspiFlash->sectSize);
			byteCnt = len2;
			
			QSPI_SEM_TAKE(pQspi->sem);
			sndBuf8 += len1;
			ret = qspiFlash_Tx_Data(pQspi, offset, byteCnt, sndBuf8);			
			QSPI_SEM_GIVE(pQspi->sem);
		}
*/
		
		if (ret > 0)
		{
			return FMSH_SUCCESS;
		}
		else
		{
			return FMSH_FAILURE;
		}
	}
}


#define __only_qspi_0__
int qspiFlash_ReadBytes_InDirect0(UINT32 addr, int len, UINT8 * rcvBuf8)
{
	if (g_pQspi0->init_flag != QSPI_CTRL_INIT_OK)
	{
		vxInit_Qspi(QSPI_CTRL_0);
	}
	
	if (FMSH_FAILURE == qspiFlash_RcvBytes_Indirect(g_pQspi0, addr, len, rcvBuf8))
	{
		return ERROR;
	}
	else
	{
		return OK;	
	}
}

int qspiFlash_WriteBytes_InDirect0(UINT32 addr, int len, UINT8 * sndBuf8)
{
	if (g_pQspi0->init_flag != QSPI_CTRL_INIT_OK)
	{
		vxInit_Qspi(QSPI_CTRL_0);
	}
	
	if (FMSH_FAILURE == qspiFlash_SndBytes_Indirect(g_pQspi0, addr, len, sndBuf8))
	{
		return ERROR;
	}
	else
	{
		return OK;	
	}
}

int qspiFlash_UnLock_Flash_Ctrl0(void)
{
	if (g_pQspi0->init_flag != QSPI_CTRL_INIT_OK)
	{
		vxInit_Qspi(QSPI_CTRL_0);
	}
	
	return qspiFlash_Unlock_Flash(g_pQspi0);
}

#endif


#if 1   /* direct mode*/

#define __Direct_Sram_Mode__

/*****************************************************************************
* This function write flash using direct mode
*
* @param
*       - offset is a value where data write 
*       - byteCount is a number of bytes to write
*       - sendBuffer is a point to write data
*
* @return
*		- FMSH_SUCCESS if success.
*		- FMSH_FAILURE if failure.
*
* @note		
*
******************************************************************************/
int qspiFlash_SndBytes_Direct_2(vxT_QSPI* pQspi, UINT32 offset_log, UINT32 byteCnt, UINT8* sndBuf)
{
    int status;
    int size, pageSize;
    UINT32 reg1, reg2;
    UINT32 address;
    UINT32 offset;
	
	/* logic_addr to phy_addr*/
	offset = qspiFlash_Offset_LogtoPhy(pQspi, offset_log);
	
	/**/
	/* Wait semphore*/
	/**/
    QSPI_SEM_TAKE(pQspi->sem);

	/**/
    /* disable wel & auto_poll*/
    /**/
    reg1 = qspiCtrl_Rd_CfgReg(pQspi, QSPIPS_WCCR_OFFSET);
    qspiCtrl_Wr_CfgReg(pQspi, QSPIPS_WCCR_OFFSET, reg1 | 0x4000);
	
    reg2 = qspiCtrl_Rd_CfgReg(pQspi, QSPIPS_DWIR_OFFSET);
    qspiCtrl_Wr_CfgReg(pQspi, QSPIPS_DWIR_OFFSET, reg2 | 0x100);    
    
    pageSize = (0x1 << pQspi->pQspiFlash->pageSize);
    
    while (byteCnt > 0)
    {
        status = qspiFlash_WREN_Cmd(pQspi) ;
        if (status)
        {   
        	break;
        }
        
        if ((offset & (pQspi->pQspiFlash->pageSize - 1)) != 0)
            size = pageSize - (offset % pageSize);
        else
            size = pageSize;
        
        if (byteCnt < size)
            size = byteCnt;
        
        address = pQspi->pQspiCtrl->fifoBaseAddr + offset;
		
        /* 
        Write to Flash  
		*/
        memcpy((void*)address, (void*)sndBuf, (size_t)size); 
        
        status = qspiFlash_Wait_FlashReady(pQspi, 1500);
        if (status)
		{	
			break;
		}
        
        byteCnt -= size;
        offset += size;
        sndBuf += size;
    }
    
	/**/
    /* enable wel & auto_poll*/
    /**/
    reg1 = qspiCtrl_Rd_CfgReg(pQspi, QSPIPS_WCCR_OFFSET);
    qspiCtrl_Wr_CfgReg(pQspi, QSPIPS_WCCR_OFFSET, reg1);
	
    reg2 = qspiCtrl_Rd_CfgReg(pQspi, QSPIPS_DWIR_OFFSET);
    qspiCtrl_Wr_CfgReg(pQspi, QSPIPS_DWIR_OFFSET, reg2);  

	/**/
	/* Give semphore*/
	/**/
    QSPI_SEM_GIVE(pQspi->sem);
    
    return status;
    
}

int qspiFlash_SndBytes_Direct(vxT_QSPI* pQspi, UINT32 offset_log, UINT32 byteCnt, UINT8 *sndBuf)
{
	UINT32  remain;
	UINT32  num;
	UINT8* src;
	UINT8* dst;
	int timeout = 0;
	int ret = 0, err = 0;
	
    UINT32 offset;
	
	/* logic_addr to phy_addr*/
	offset = qspiFlash_Offset_LogtoPhy(pQspi, offset_log);
	
    QSPI_SEM_TAKE(pQspi->sem);

	/*src = ppBuf[0];*/
	/*dst = (UINT8 *pDrvCtrl->memBase + uAddr;*/
	/*remain = uBufLen;*/
	src = sndBuf;
	dst = (UINT8 *)(pQspi->pQspiCtrl->fifoBaseAddr + offset);
	remain = byteCnt;

	while (remain > 0)
	{
		if (remain > pQspi->pQspiFlash->pageSize)
		{
			num = (UINT32)pQspi->pQspiFlash->pageSize;
		}
		else
		{
			num = (UINT32)remain;
		}

		memcpy((void *)dst, (void *)src, (size_t)num);

		/* 
		Wait for write operation done 
		*/
		/*err = qspiFlash_Wait_WIP_Ok(pQspi);*/
		err = qspiFlash_Wait_WIP_Ok(pQspi, 1500);
		if (err == FMSH_FAILURE)
		{
			QSPI_SEM_GIVE(pQspi->sem);
			return FMSH_FAILURE;
		}

		src += num;
		dst += num;
		remain -= num;
	}

    QSPI_SEM_GIVE(pQspi->sem);
    
    return FMSH_SUCCESS;
}

/*****************************************************************************
* This function read flash using direct mode
*
* @param
*       - offset is a value where data read 
*       - byteCount is a number of bytes to read
*       - recvBuffer is a point to read data
*
* @return
*		- FMSH_SUCCESS if success.
*		- FMSH_FAILURE if failure.
*
* @note		
*
******************************************************************************/
int qspiFlash_RcvBytes_Direct(vxT_QSPI* pQspi, UINT32 offset_log, UINT32 byteCnt, UINT8 *rcvBuf)
{
    UINT32 address;    
    UINT32 offset;    
    	
    if (byteCnt == 0)
    {
        return FMSH_FAILURE; 
    }    

	/* logic_addr to phy_addr*/
	offset = qspiFlash_Offset_LogtoPhy(pQspi, offset_log);
	
    if (pQspi->pQspiFlash->devSize < (offset + byteCnt))
    {
        return FMSH_FAILURE; 
    }	
	
    QSPI_SEM_TAKE(pQspi->sem);
    
    /* Read from flash */
    address = pQspi->pQspiCtrl->fifoBaseAddr + offset;
    memcpy((UINT8*)rcvBuf, (UINT8*)address, (size_t)byteCnt); 

 
    QSPI_SEM_GIVE(pQspi->sem);
    
    return FMSH_SUCCESS;
}

#define __only_qspi_0__

/*
qpsi-flash control_0
*/
int qspiFlash_ReadBytes_Direct0(int addr, int len, UINT8 * rcvBuf8)
{
	if (g_pQspi0->init_flag != QSPI_CTRL_INIT_OK)
	{
		vxInit_Qspi(QSPI_CTRL_0);
	}
	
	if (FMSH_FAILURE == qspiFlash_RcvBytes_Direct(g_pQspi0, addr, len, rcvBuf8))
	{
		return FMSH_FAILURE;
	}
	else
	{
		return FMSH_SUCCESS;	
	}
}

/*
qpsi-flash control_0
*/
int qspiFlash_WriteBytes_Direct0(int addr, int len, UINT8 * sndBuf8)
{
	if (g_pQspi0->init_flag != QSPI_CTRL_INIT_OK)
	{
		vxInit_Qspi(QSPI_CTRL_0);
	}
	
	if (FMSH_FAILURE == qspiFlash_SndBytes_Direct(g_pQspi0, addr, len, sndBuf8))
	{
		return FMSH_FAILURE;
	}
	else
	{
		return FMSH_SUCCESS;	
	}
}

#endif


#if 1

#define __tffs_norflash__

#define __Direct__

int qspiFlash_SndBytes_Direct_tffs(vxT_QSPI* pQspi, UINT32 offset_log, UINT32 byteCnt, UINT8 *sndBuf)
{
	UINT32  remain;
	UINT32  num;
	UINT8* src;
	UINT8* dst;
	int timeout = 0;
	int ret = 0, err = 0;
	
    UINT32 offset;
	
	/* logic_addr to phy_addr*/
	/*offset = qspiFlash_Offset_LogtoPhy(pQspi, offset_log);	*/
	offset = offset_log;
	
    QSPI_SEM_TAKE(pQspi->sem);

	/*src = ppBuf[0];*/
	/*dst = (UINT8 *pDrvCtrl->memBase + uAddr;*/
	/*remain = uBufLen;*/
	src = sndBuf;
	dst = (UINT8 *)(pQspi->pQspiCtrl->fifoBaseAddr + offset);
	remain = byteCnt;

	while (remain > 0)
	{
		if (remain > pQspi->pQspiFlash->pageSize)
		{
			num = (UINT32)pQspi->pQspiFlash->pageSize;
		}
		else
		{
			num = (UINT32)remain;
		}

		memcpy((void *)dst, (void *)src, (size_t)num);

		/* 
		Wait for write operation done 
		*/
		/*err = qspiFlash_Wait_WIP_Ok(pQspi);*/
		err = qspiFlash_Wait_WIP_Ok(pQspi, 1500);
		if (err == FMSH_FAILURE)
		{
			QSPI_SEM_GIVE(pQspi->sem);
			return FMSH_FAILURE;
		}

		src += num;
		dst += num;
		remain -= num;
	}

    QSPI_SEM_GIVE(pQspi->sem);
    
    return FMSH_SUCCESS;
}

/*****************************************************************************
* This function read flash using direct mode
*
* @param
*       - offset is a value where data read 
*       - byteCount is a number of bytes to read
*       - recvBuffer is a point to read data
*
* @return
*		- FMSH_SUCCESS if success.
*		- FMSH_FAILURE if failure.
*
* @note		
*
******************************************************************************/
int qspiFlash_RcvBytes_Direct_tffs(vxT_QSPI* pQspi, UINT32 offset_log, UINT32 byteCnt, UINT8 *rcvBuf)
{
    UINT32 address;    
    UINT32 offset;    
    	
    if (byteCnt == 0)
    {
        return FMSH_FAILURE; 
    }    

	/* logic_addr to phy_addr*/
	/*offset = qspiFlash_Offset_LogtoPhy(pQspi, offset_log);*/
	offset = offset_log;
	
    if (pQspi->pQspiFlash->devSize < (offset + byteCnt))
    {
        return FMSH_FAILURE; 
    }	
	
    QSPI_SEM_TAKE(pQspi->sem);
    
    /* Read from flash */
    address = pQspi->pQspiCtrl->fifoBaseAddr + offset;
    memcpy((UINT8*)rcvBuf, (UINT8*)address, (size_t)byteCnt); 

 
    QSPI_SEM_GIVE(pQspi->sem);
    
    return FMSH_SUCCESS;
}

/*
qpsi-flash control_0
*/
int qspiFlash_ReadBytes_Direct0_tffs(int addr, int len, UINT8 * rcvBuf8)
{
	if (g_pQspi0->init_flag != QSPI_CTRL_INIT_OK)
	{
		vxInit_Qspi(QSPI_CTRL_0);
	}
	
	if (FMSH_FAILURE == qspiFlash_RcvBytes_Direct_tffs(g_pQspi0, addr, len, rcvBuf8))
	{
		return FMSH_FAILURE;
	}
	else
	{
		return FMSH_SUCCESS;	
	}
}

/*
qpsi-flash control_0
*/
int qspiFlash_WriteBytes_Direct0_tffs(int addr, int len, UINT8 * sndBuf8)
{
	if (g_pQspi0->init_flag != QSPI_CTRL_INIT_OK)
	{
		vxInit_Qspi(QSPI_CTRL_0);
	}
	
	if (FMSH_FAILURE == qspiFlash_SndBytes_Direct_tffs(g_pQspi0, addr, len, sndBuf8))
	{
		return FMSH_FAILURE;
	}
	else
	{
		return FMSH_SUCCESS;	
	}
}

#define __Indirect__
int qspiFlash_RcvBytes_Indirect_tffs(vxT_QSPI* pQspi, UINT32 addr, int len, UINT8* rcvBuf8)
{
	vxT_QSPI_FLASH * pQspiFlash = pQspi->pQspiFlash;
	int ret = 0;
	UINT32 offset = 0;
	UINT32 byteCnt = 0;
	
	int phy_idx = 0;	
	int log_idx = 0;

	int start_bank = 0, end_bank = 0;
	UINT32 addr1 = 0, addr2 = 0;
	int len1 = 0, len2 = 0;

	if (pQspi->pQspiCtrl->addr_bytes_mode ==  ADDR_4_BYTES_MODE)
	{
		offset = addr;
		byteCnt = len;	
		
		QSPI_SEM_TAKE(pQspi->sem);
		ret = qspiFlash_Rx_Data(pQspi, offset, byteCnt, rcvBuf8);		
		QSPI_SEM_GIVE(pQspi->sem);
		
		if (ret > 0)
		{
			return FMSH_SUCCESS;
		}
		else
		{
			return FMSH_FAILURE;
		}
	}
	else
	{		
		start_bank = addr / SIZE_16M;  /* 16M / bank*/
		end_bank = (addr + len) / SIZE_16M;  /* 16M / bank*/

		/*if (start_bank == end_bank)*/
		{
			/*offset = qspiFlash_Offset_LogtoPhy(pQspi, addr);*/
			offset = addr;
			
			byteCnt = len;	

			
		    QSPI_SEM_TAKE(pQspi->sem);
			ret = qspiFlash_Rx_Data(pQspi, offset, byteCnt, rcvBuf8);		
		    QSPI_SEM_GIVE(pQspi->sem);
		}

		if (ret > 0)
		{
			return FMSH_SUCCESS;
		}
		else
		{
			return FMSH_FAILURE;
		}
	}	

}

int qspiFlash_SndBytes_Indirect_tffs(vxT_QSPI* pQspi, UINT32 addr, int len, UINT8* sndBuf8)
{
	vxT_QSPI_FLASH * pQspiFlash = pQspi->pQspiFlash;
	int ret = 0;
	UINT32 offset = 0;
	UINT32 byteCnt = 0;
	
	int phy_idx = 0;	
	int log_idx = 0;
	int start_bank = 0, end_bank = 0;
	UINT32 addr1 = 0, addr2 = 0;
	int len1 = 0, len2 = 0;
	
	if (pQspi->pQspiCtrl->addr_bytes_mode ==  ADDR_4_BYTES_MODE)
	{
		offset = addr;
		byteCnt = len;	
		
		QSPI_SEM_TAKE(pQspi->sem);	
		ret = qspiFlash_Tx_Data(pQspi, offset, byteCnt, sndBuf8);		
		QSPI_SEM_GIVE(pQspi->sem);
		
		if (ret > 0)
		{
			return FMSH_SUCCESS;
		}
		else
		{
			return FMSH_FAILURE;
		}
	}
	else
	{
		start_bank = addr / SIZE_16M;  /* 16M / bank*/
		end_bank = (addr + len) / SIZE_16M;  /* 16M / bank*/

		/*if (start_bank == end_bank)*/
		{
			/*offset = qspiFlash_Offset_LogtoPhy(pQspi, addr);*/
			offset = addr;
			
			byteCnt = len;	
			
			/*printf("wr sect_idx=>logic:%d->phy:%d(len-%d) \n", log_idx, phy_idx, len);*/
			/*printf("wr sect_idx=>logic:0x%X->phy:0x%X(len-%d) \n", addr, offset, len);*/
			
		    QSPI_SEM_TAKE(pQspi->sem);	
			ret = qspiFlash_Tx_Data(pQspi, offset, byteCnt, sndBuf8);		
		    QSPI_SEM_GIVE(pQspi->sem);
		}
		
		if (ret > 0)
		{
			return FMSH_SUCCESS;
		}
		else
		{
			return FMSH_FAILURE;
		}
	}
}

int qspiFlash_ReadBytes_InDirect0_tffs(UINT32 addr, int len, UINT8 * rcvBuf8)
{
	if (g_pQspi0->init_flag != QSPI_CTRL_INIT_OK)
	{
		vxInit_Qspi(QSPI_CTRL_0);
	}
	
	if (FMSH_FAILURE == qspiFlash_RcvBytes_Indirect_tffs(g_pQspi0, addr, len, rcvBuf8))
	{
		return ERROR;
	}
	else
	{
		return OK;	
	}
}

int qspiFlash_WriteBytes_InDirect0_tffs(UINT32 addr, int len, UINT8 * sndBuf8)
{
	if (g_pQspi0->init_flag != QSPI_CTRL_INIT_OK)
	{
		vxInit_Qspi(QSPI_CTRL_0);
	}
	
	if (FMSH_FAILURE == qspiFlash_SndBytes_Indirect_tffs(g_pQspi0, addr, len, sndBuf8))
	{
		return ERROR;
	}
	else
	{
		return OK;	
	}
}

#endif


#if 1

/*******************************************************************************
*
* qspiFlashInit1 - initialize Altera Soc Gen 5 controller.
*
* This function initializes Altera Soc Gen 5 controller stage 1.
*
* RETURNS: N/A.
*
* ERRNO
*/
void qspiFlashInit1 (void)
{
    /* reset pQspi controller and pQspi flash chip */

    /*qspiFlashReset ();*/
    /*qspiFlashResetChip ();*/
    
#if 1	
	UINT32 tmp32 = slcr_read(SLCR_QSPI0_CTRL);
	/* set 1*/
	tmp32 |= (QSPI_APB_RST | QSPI_AHB_RST | QSPI_REF_RST);
	slcr_write(SLCR_QSPI0_CTRL, tmp32);
	
	/* set 0*/
	tmp32 &= ~(QSPI_APB_RST | QSPI_AHB_RST | QSPI_REF_RST);
	slcr_write(SLCR_QSPI0_CTRL, tmp32);
#endif

#if 0
	/*
	reg[00] = 0x80080081 
	reg[04] = 0x0802006B 
	reg[08] = 0x00000002 
	reg[0C] = 0x00000000 
	reg[10] = 0x00000001 
	reg[14] = 0x00000002 
	reg[18] = 0x00000080 
	reg[1C] = 0xE8000000 
	*/
	UINT32 base = 0xE0000000;
	*((UINT32*)(base+0x00)) = 0x80080081;
	*((UINT32*)(base+0x04)) = 0x0802006B; 
	*((UINT32*)(base+0x08)) = 0x00000002; 
	*((UINT32*)(base+0x0C)) = 0x00000000; 
	*((UINT32*)(base+0x10)) = 0x00000001; 
	*((UINT32*)(base+0x14)) = 0x00000002; 
	*((UINT32*)(base+0x18)) = 0x00000080; 
	*((UINT32*)(base+0x1C)) = 0xE8000000; 
#endif

    return;
}

/*******************************************************************************
*
* qspiFlashInit2 - initialize Altera Soc Gen 5 controller.
*
* This function initializes Altera Soc Gen 5 controller stage 2.
*
* RETURNS: N/A.
*
* ERRNO
*/

 void qspiFlashInit2 (void)
{
    /*
    int i = 0;

    for (i = 0; i < QSPI_MAX_DEVICE; i++)
        QspiDev [i].sem = semMCreate (SEM_Q_PRIORITY | SEM_DELETE_SAFE |
                                      SEM_INVERSION_SAFE);
	*/
	return;
}


/*******************************************************************************
*
* qspiFlashSectorErase - erase the specified flash sector
*
* This routine erases the specified sector, all bits in sector are
* set to 1.
*
* RETURNS: OK, or ERROR if timeout or parameter is invalid.
*
* ERRNO: N/A.
*/

STATUS tffs_qspiFlashSectorErase
    (
    UINT32 addr
    )
{
	vxT_QSPI * pQspi = g_pQspi0;
	vxT_QSPI_CTRL * pQspiCtrl = NULL;
	vxT_QSPI_FLASH * pQspiFlash = NULL;
	
	if (pQspi->init_flag != QSPI_CTRL_INIT_OK)
	{
		vxInit_Qspi(QSPI_CTRL_0);
	}
	
	pQspiCtrl = pQspi->pQspiCtrl;
	pQspiFlash = pQspi->pQspiFlash;
	
	int ret = 0;
	int sect_idx = 0;
	
	sect_idx = addr / pQspiFlash->sectSize;

	ret = qspiFlash_Erase_Sect(pQspi, sect_idx);
	return ret;
}
	
STATUS tffs_qspiFlashSectorErase_Idx (UINT32 log_idx)
{
	int ret = 0, phy_idx = 0;
	vxT_QSPI * pQspi = g_pQspi0;
	
	if (pQspi->init_flag != QSPI_CTRL_INIT_OK)
	{
		vxInit_Qspi(QSPI_CTRL_0);
	}	
	
	/*phy_idx = qspiFlash_SectIdx_LogtoPhy(pQspi, log_idx);	*/
	ret = qspiFlash_Erase_Sect(pQspi, log_idx);
	return ret;
}

/*******************************************************************************
*
* qspiFlashRead - read pQspi flash device
*
* This routine fills the read buffer supplied in the parameters.
*
* SEE ALSO: qspiFlashPageWrite()
*
* RETURNS: OK, or ERROR if timeout or parameter is invalid.
*
* ERRNO: N/A
*/

STATUS tffs_qspiFlashRead
    (
    UINT32         offset,         /* The address to read */
    char *      readBuffer,     /* A pointer to a location to read the data */
    int         byteLen         /* The size to read */
    )
{
#if QSPI_DIRECT_MODE
	return qspiFlash_ReadBytes_Direct0_tffs(offset, byteLen, readBuffer);
#else
	return qspiFlash_ReadBytes_InDirect0_tffs(offset, byteLen, readBuffer);	
#endif	
}

/*******************************************************************************
*
* qspiFlashPageWrite - write pQspi flash device
*
* This routine fills the write buffer supplied in the parameters to flash.
* The byte length must not exceed than one page.
*
* SEE ALSO: qspiFlashRead()
*
* RETURNS: OK, or ERROR if timeout or parameter is invalid.
*
* ERRNO: N/A
*/

STATUS tffs_qspiFlashPageWrite
    (
    UINT32         addr,           /* byte offset into flash memory */
    char *      buf,            /* buffer */
    int         byteLen         /* size of bytes */
    )
{	
#if QSPI_DIRECT_MODE
	return qspiFlash_WriteBytes_Direct0_tffs(addr, byteLen, buf);
#else
	return qspiFlash_WriteBytes_InDirect0_tffs(addr, byteLen, buf); /* InDirect*/
#endif
}

#endif



#if  1

UINT8 RcvBuf2[QSPIFLASH_SECTOR_64K] = {0};
UINT8 SndBuf2[QSPIFLASH_SECTOR_64K] = {0};

void test_qspi_rd_sect(int sect_idx)
{
	vxT_QSPI * pQspi = g_pQspi0;
	
	vxT_QSPI_CTRL * pQspiCtrl = pQspi->pQspiCtrl;
	vxT_QSPI_FLASH * pQspiFlash = pQspi->pQspiFlash;

	UINT8 * pBuf8 = (UINT8 *)(&RcvBuf2[0]);
	
	int j = 0, byteCnt = 0;
	int offset = 0;
	
	int phy_idx = 0;
	
	vxInit_Qspi(QSPI_CTRL_0);
	
	byteCnt = pQspiFlash->sectSize;
	offset = sect_idx * pQspiFlash->sectSize;
	
#if  QSPI_DIRECT_MODE
	qspiFlash_RcvBytes_Direct(pQspi, offset, byteCnt, pBuf8);
#else
	qspiFlash_Read_Sect(pQspi, sect_idx, pBuf8);
#endif

	printf("----pQspi(%d)-read(sect-%d):%dKB----\n", pQspiCtrl->ctrl_x, sect_idx, (byteCnt/1024));	
	
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
	
	for (j=(byteCnt-512); j<byteCnt; j++)
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
	printf("------------\n");

	return;
}

int g_test_qspi2 = 0;
void test_qspi_wr_sect(int sect_idx)
{
	vxT_QSPI * pQspi = g_pQspi0;
	
	vxT_QSPI_CTRL * pQspiCtrl = pQspi->pQspiCtrl;
	vxT_QSPI_FLASH * pQspiFlash = pQspi->pQspiFlash;

	UINT8 * pBuf8 = (UINT8 *)(&SndBuf2[0]);
	
	int i = 0, j = 0, byteCnt = 0;
	int offset = 0;
	
	vxInit_Qspi(QSPI_CTRL_0);
	
	byteCnt = pQspiFlash->sectSize;
	offset = sect_idx * pQspiFlash->sectSize;
	
	for (i=0; i<byteCnt; i++)
	{
		pBuf8[i] = g_test_qspi2 + i;
	}
	g_test_qspi2++;

	/* head*/
	*((UINT32*)(&pBuf8[0])) = 0xabcd1234;
	*((UINT32*)(&pBuf8[4])) = sect_idx;
	*((UINT32*)(&pBuf8[8])) = sect_idx;
	*((UINT32*)(&pBuf8[0xC])) = sect_idx;

	/* tail*/
	*((UINT32*)(&pBuf8[byteCnt-0x10])) = sect_idx;
	*((UINT32*)(&pBuf8[byteCnt-0xC])) = sect_idx;
	*((UINT32*)(&pBuf8[byteCnt-8])) = sect_idx;
	*((UINT32*)(&pBuf8[byteCnt-4])) = 0xcdef6789;	
	
	
#if  QSPI_DIRECT_MODE
	qspiFlash_SndBytes_Direct(pQspi, offset, byteCnt, pBuf8);
#else
	qspiFlash_Write_Sect(pQspi, sect_idx, pBuf8);
#endif
	
	printf("----pQspi(%d)-write(sect-%d):%dKB----\n", pQspiCtrl->ctrl_x, sect_idx, (byteCnt/1024));	
	
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
	
	for (j=(byteCnt-512); j<byteCnt; j++)
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
	printf("------------\n");
	
	return;
}

void test_qspi_erase_sect(int sect_idx)
{
	vxT_QSPI * pQspi = g_pQspi0;
	int ret = 0;
	
	vxInit_Qspi(QSPI_CTRL_0);
	
	ret = qspiFlash_Erase_Sect(pQspi, sect_idx);
    if (ret == FMSH_FAILURE)
    {
    	printf("ctrl_%d: qspiflash erase sect: %d fail! \n", pQspi->qspi_x, sect_idx);
    }
	else
	{
		printf("ctrl_%d: qspiflash erase sect: %d ok! \n", pQspi->qspi_x, sect_idx);
	}
	
	return;
}

void test_rd_ctrl_regs(void)
{
	UINT32* pReg = NULL;
	UINT32 base = 0xE0000000;
	int i = 0;

	for (i=0; i<0xb4; i+=4)
	{
		if ((i == 0x48) || (i == 0x4C)
			|| (i == 0x5C) 
			|| (i == 0x84) || (i == 0x88)
			|| (i == 0x98) || (i == 0x9C))
		{
			printf("reg[%02X] = -------- \n", i);
		}
		else
		{
			pReg = (UINT32*)(base + i);
			printf("reg[%02X] = 0x%08X \n", i, *pReg);
		}
	}
	
	pReg = (UINT32*)(base + 0xFC);
	printf("reg[%02X] = 0x%08X \n", i, *pReg);

	return;
}

void test_erase_all_chip(void)
{
	vxT_QSPI * pQspi = g_pQspi0;
	
	vxInit_Qspi (QSPI_CTRL_0);
	
	qspiFlash_Erase_Chip(pQspi);
	return;
}

#endif


/*
for test all sector: read & write & erase
*/
#if 1
void testvx_qspi_erase_sect2(int sect_idx)
{
	vxT_QSPI * pQspi = g_pQspi0;
	int ret = 0;
	
	ret = qspiFlash_Erase_Sect(pQspi, sect_idx);
    if (ret == FMSH_FAILURE)
    {
    	printf("ctrl_%d: qspiflash erase sect: %d fail! \n", pQspi->qspi_x, sect_idx);
    }
	else
	{
		printf("ctrl_%d: qspiflash erase sect: %d ok! \n", pQspi->qspi_x, sect_idx);
	}
	
	return;
}

void testvx_qspi_rd_sect2(int sect_idx)
{
	vxT_QSPI * pQspi = g_pQspi0;
	
	vxT_QSPI_CTRL * pQspiCtrl = pQspi->pQspiCtrl;
	vxT_QSPI_FLASH * pQspiFlash = pQspi->pQspiFlash;

	int ret = 0;
	int j = 0, byteCnt = 0;
	UINT8 * pBuf8 = (UINT8 *)(&RcvBuf2[0]);		

	for (j=0; j<QSPIFLASH_SECTOR_64K; j++)
	{
		RcvBuf2[j] = 0x0;
	}

	byteCnt = pQspiFlash->sectSize;	
	ret = qspiFlash_Read_Sect(pQspi, sect_idx, pBuf8);
	if (ret == FMSH_FAILURE)
	{
		printf("--qpsi read sect(%d) fail!-- \n", sect_idx);
	}
	else
	{
		printf("qpsi read sect(%d) ok! \n", sect_idx);
	}

	return;
}

void testvx_qspi_wr_sect2(int sect_idx)
{
	vxT_QSPI * pQspi = g_pQspi0;
	
	vxT_QSPI_CTRL * pQspiCtrl = pQspi->pQspiCtrl;
	vxT_QSPI_FLASH * pQspiFlash = pQspi->pQspiFlash;

	int ret = 0;
	int i = 0, j = 0, byteCnt = 0;
	UINT8 * pBuf8 = (UINT8 *)(&SndBuf2[0]);	
	
	byteCnt = pQspiFlash->sectSize;
	
	for (i=0; i<byteCnt; i++)
	{
		pBuf8[i] = g_test_qspi2 + i;
	}
	g_test_qspi2++;
	
	ret = qspiFlash_Write_Sect(pQspi, sect_idx, pBuf8);
	if (ret == FMSH_FAILURE)
	{
		printf("--qpsi write sect(%d) fail!-- \n", sect_idx);
	}
	else
	{
		printf("qpsi write sect(%d) ok! \n", sect_idx);
	}
	
	return;
}

/*#define MODE_ONLY_ERASE_RD*/

void testvx_qspi_rdwr_sects(int from_sect, int to_sect)
{
	vxT_QSPI * pQspi = g_pQspi0;
	
	int idx = 0, j = 0;
	int sect_diff = 0;
	int phy_idx = 0;

	vxInit_Qspi(QSPI_CTRL_0);

	for (idx = from_sect; idx <= to_sect; idx++)
	{
		/*phy_idx = qspiFlash_SectIdx_LogtoPhy(pQspi, idx);*/
		
		testvx_qspi_erase_sect2(idx);
		taskDelay(10);

	#ifndef MODE_ONLY_ERASE_RD
		testvx_qspi_wr_sect2(phy_idx);
		taskDelay(10);
	#endif
		
		testvx_qspi_rd_sect2(phy_idx);
		taskDelay(10);

		sect_diff = 0;
		for (j=0; j<pQspi->pQspiFlash->sectSize; j++)
		{
		#ifndef MODE_ONLY_ERASE_RD
			if (RcvBuf2[j] != SndBuf2[j])
		#else
			if (RcvBuf2[j] != 0xFF)
		#endif
			{
				sect_diff = 1;
				break;
			}
		}

		if (sect_diff == 1)
		{
			printf("------sect(%d) erase & wr & rd err:[%d][0x%X-0x%X]!------- \n\n", \
				    idx, j, RcvBuf2[j], SndBuf2[j]);
		}
		else
		{		
		#ifndef MODE_ONLY_ERASE_RD
			printf("====sect(%d) erase & wr & rd OK!==== \n\n", idx);
		#else
			printf("====sect(%d) erase & ... & rd OK!==== \n\n", idx);  /* only erase & read*/
		#endif
		}
	}

	printf("\n\n testvx sectors:[%d -> %d] finish! \n\n\n", from_sect, to_sect);
	
	return;
}



void testvx_qspi_all_chip(void)
{
	vxT_QSPI * pQspi = g_pQspi0;
	
	int idx = 0, j = 0;
	int sect_diff = 0;
	
	int from_sect, to_sect;
	int sectSize = 0, devSize = 0;
	int phy_idx = 0;

	vxInit_Qspi(QSPI_CTRL_0);

	sectSize = pQspi->pQspiFlash->sectSize;
	devSize = pQspi->pQspiFlash->devSize;
	
	from_sect = 0;
	to_sect = devSize / sectSize;  /* max sect num*/

	for (idx = from_sect; idx < to_sect; idx++)
	{
		phy_idx = qspiFlash_SectIdx_LogtoPhy_Only(pQspi, idx);
		
		testvx_qspi_erase_sect2(idx);
		taskDelay(10);

	#ifndef MODE_ONLY_ERASE_RD
		testvx_qspi_wr_sect2(idx);
		taskDelay(10);
	#endif
		
		testvx_qspi_rd_sect2(idx);
		taskDelay(10);

		sect_diff = 0;
		for (j=0; j<pQspi->pQspiFlash->sectSize; j++)
		{
		#ifndef MODE_ONLY_ERASE_RD
			if (RcvBuf2[j] != SndBuf2[j])
		#else
			if (RcvBuf2[j] != 0xFF)
		#endif
			{
				sect_diff = 1;
				break;
			}
		}

		if (sect_diff == 1)
		{
			printf("------sect(%d) erase & wr & rd err:[%d][0x%X-0x%X]!------- \n\n", \
				    idx, j, RcvBuf2[j], SndBuf2[j]);
		}
		else
		{
		
		#ifndef MODE_ONLY_ERASE_RD
			printf("====sect(%d-%d) erase & wr & rd OK!==== \n\n", idx, phy_idx);
		#else
			printf("====sect(%d-%d) erase & ... & rd OK!==== \n\n", idx, phy_idx);  /* only erase & read*/
		#endif
		
			if (((idx+1) % 10) == 0)
			{
				printf("... ... test: %.1f %% finished ... ... \n\n", ((float)(idx*sectSize*1.0)/(float)(devSize*1.0))*100.0);
			}
		}
	}

	printf("... ... test: 100 %% finished(sects:%d, chip_size:0x%X) ... ... \n\n", idx, devSize);
	printf("\n\n testvx all chip sectors:[%d -> %d] finish! \n\n\n", from_sect, to_sect);
	
	return;
}

#endif


#if 1
/*
uboot env: 
	offset - 0xE0_0000
	
qspiflash_ctrl_0:  
	base_addr - 0xE000_0000 
*/
void test_qspi_rd_uboot_env(void)
{
	vxT_QSPI * pQspi = g_pQspi0;	
	
	vxT_QSPI_CTRL * pQspiCtrl = pQspi->pQspiCtrl;
	vxT_QSPI_FLASH * pQspiFlash = pQspi->pQspiFlash;

	UINT8 * pBuf8 = (UINT8 *)(&RcvBuf2[0]);
	
	int j = 0, byteCnt = 0;
	int offset = 0;
	int sect_idx = 0;
	int k = 0;

	/**/
	/* int qspi flash ctrl*/
	/**/
	vxInit_Qspi(QSPI_CTRL_0);
	
	pQspiCtrl = pQspi->pQspiCtrl;
	pQspiFlash = pQspi->pQspiFlash;
		
	sect_idx = 0xE00000 / pQspiFlash->sectSize;
	
	byteCnt = pQspiFlash->sectSize;
	offset = sect_idx * pQspiFlash->sectSize;

	
#if  QSPI_DIRECT_MODE
	qspiFlash_RcvBytes_Direct(pQspi, offset, byteCnt, pBuf8);
#else
	qspiFlash_Read_Sect(pQspi, sect_idx, pBuf8);
#endif

	printf("uboot env base:0xE000_0000 offset: 0xE0_0000 \n");
	printf("-------------------------------------------- \n\n");

	for (k=0, j=0; j<0x900; j++)
	{
		if (pBuf8[j] == 0) /* 0 == \n */
		{
			printf("%s \n", (char*)(&pBuf8[k]));
			
			k = j + 1;  /* new line*/
		}

		if ((pBuf8[j]==0) && (pBuf8[j+1]==0) && (pBuf8[j+2]==0))
		{	
			break;
		}
	}

	return;
}


char g_uboot_ipAddr[16] = {0};

int qspi_get_ipaddr(void)
{
	vxT_QSPI * pQspi = g_pQspi0;	
	
	vxT_QSPI_CTRL * pQspiCtrl = pQspi->pQspiCtrl;
	vxT_QSPI_FLASH * pQspiFlash = pQspi->pQspiFlash;

	UINT8 * pBuf8 = (UINT8 *)(&RcvBuf2[0]);
	
	int j = 0, byteCnt = 0;
	int offset = 0;
	int sect_idx = 0;
	int k = 0;

	/**/
	/* int qspi flash ctrl*/
	/**/
	vxInit_Qspi(QSPI_CTRL_0);
	
	pQspiCtrl = pQspi->pQspiCtrl;
	pQspiFlash = pQspi->pQspiFlash;
		
	sect_idx = 0xE00000 / pQspiFlash->sectSize;
	
	byteCnt = pQspiFlash->sectSize;
	offset = sect_idx * pQspiFlash->sectSize;
	
#if  QSPI_DIRECT_MODE
	qspiFlash_RcvBytes_Direct(pQspi, offset, byteCnt, pBuf8);
#else
	qspiFlash_Read_Sect(pQspi, sect_idx, pBuf8);
#endif

	for (j=0; j<0x900; j++)
	{
		if ((pBuf8[j]=='i') && (pBuf8[j+1]=='p') && (pBuf8[j+2]=='a') \
			&& (pBuf8[j+3]=='d') && (pBuf8[j+4]=='d') && (pBuf8[j+5]=='r') \
			&& (pBuf8[j+6]=='='))
		{
			for (k=0; k<16; k++)
			{
				g_uboot_ipAddr[k] = pBuf8[j+7+k];
				if (g_uboot_ipAddr[k] == 0)
				{
					goto ip_end;
				}
			}		
		}
	}

ip_end:
	printf("%s \n", (char*)(&g_uboot_ipAddr[0]));

	return;
}

/**/
/* erase sector: 0xE0, uboot env place(saveenv) */
/**/
void qspi_erase_ipaddr(void)
{
	int sect_idx = 0xE00000 / 0x10000;
	test_qspi_erase_sect(sect_idx);
	return;
}

#endif


/*
winbond: w25q256fv, for test fatFS
4K sector: read & write & erase
*/
#ifdef ENABLE_FATFS_QSPI

int qspiFlash_Erase_Sect_4K(vxT_QSPI* pQspi, int sect_idx)
{
	vxT_QSPI_CTRL * pQspiCtrl = pQspi->pQspiCtrl;
	vxT_QSPI_FLASH * pQspiFlash = pQspi->pQspiFlash;

    int ret = FMSH_SUCCESS;
    UINT32 sect_offset = 0, reg = 0;	
	int phy_idx = 0;
	
    QSPI_SEM_TAKE(pQspi->sem);

	phy_idx = qspiFlash_SectIdx_LogtoPhy(pQspi, sect_idx);
	
    /* 
    calculate sector start address to be erased 
	*/
    sect_offset = (phy_idx * pQspiFlash->sectSize);
    qspiCtrl_Wr_CfgReg(pQspi, QSPIPS_FCAR_OFFSET, sect_offset);
    
    ret = qspiFlash_WREN_Cmd(pQspi);
    if (ret == FMSH_FAILURE)
	{
        goto ERASE_END;
    }

	/* Erase Operations */
	/*
	#define SUBSECTOR_ERASE_CMD                  0x20
	#define SUBSECTOR_ERASE_4_BYTE_ADDR_CMD      0x21

	#define SECTOR_ERASE_CMD                     0xD8
	#define SECTOR_ERASE_4_BYTE_ADDR_CMD         0xDC

	#define BULK_ERASE_CMD                       0xC7
	*/
	
    /* Send Erase Command*/
    if (pQspi->pQspiCtrl->addr_bytes_mode == ADDR_4_BYTES_MODE)
    {    
		reg = 0x210B0000;  /* 4k*/
    }
	else
    {    
		reg = 0x200A0000;  /* 4k*/
    }	
	
    ret = qspiCtrl_CmdExecute(pQspi, reg);
    if (ret == FMSH_FAILURE)
	{
        goto ERASE_END;
    }
    
    ret = qspiFlash_Wait_FlashReady(pQspi, 5000);
    if (ret == FMSH_FAILURE)
	{
        goto ERASE_END;
    }

ERASE_END:	
    QSPI_SEM_GIVE(pQspi->sem);
    
    return ret;
}

void test_qspi_rd_sect_4K(int sect_idx)
{
	vxT_QSPI * pQspi = g_pQspi0;
	
	vxT_QSPI_CTRL * pQspiCtrl = pQspi->pQspiCtrl;
	vxT_QSPI_FLASH * pQspiFlash = pQspi->pQspiFlash;

	UINT8 * pBuf8 = (UINT8 *)(&RcvBuf2[0]);
	
	int j = 0, byteCnt = 0;
	int offset = 0;
	
	int phy_idx = 0;
	
	vxInit_Qspi(QSPI_CTRL_0);

	pQspiFlash->sectSize = 0x1000;  /* 4K*/
	
	byteCnt = pQspiFlash->sectSize;
	offset = sect_idx * pQspiFlash->sectSize;
	
	qspiFlash_RcvBytes_Direct(pQspi, offset, byteCnt, pBuf8);

	printf("----pQspi(%d)-read(sect-%d):%dKB----\n", pQspiCtrl->ctrl_x, sect_idx, (byteCnt/1024));	
	
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
	
	for (j=(byteCnt-512); j<byteCnt; j++)
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
	printf("------------\n");

	return;
}

void test_qspi_wr_sect_4K(int sect_idx)
{
	vxT_QSPI * pQspi = g_pQspi0;
	
	vxT_QSPI_CTRL * pQspiCtrl = pQspi->pQspiCtrl;
	vxT_QSPI_FLASH * pQspiFlash = pQspi->pQspiFlash;

	UINT8 * pBuf8 = (UINT8 *)(&SndBuf2[0]);
	
	int i = 0, j = 0, byteCnt = 0;
	int offset = 0;
	
	vxInit_Qspi(QSPI_CTRL_0);

	pQspiFlash->sectSize = 0x1000;  /* 4K*/
	
	byteCnt = pQspiFlash->sectSize;
	offset = sect_idx * pQspiFlash->sectSize;
	
	for (i=0; i<byteCnt; i++)
	{
		pBuf8[i] = g_test_qspi2 + i;
	}
	g_test_qspi2++;

	/* head*/
	*((UINT32*)(&pBuf8[0])) = 0xabcd1234;
	*((UINT32*)(&pBuf8[4])) = sect_idx;
	*((UINT32*)(&pBuf8[8])) = sect_idx;
	*((UINT32*)(&pBuf8[0xC])) = sect_idx;

	/* tail*/
	*((UINT32*)(&pBuf8[byteCnt-0x10])) = sect_idx;
	*((UINT32*)(&pBuf8[byteCnt-0xC])) = sect_idx;
	*((UINT32*)(&pBuf8[byteCnt-8])) = sect_idx;
	*((UINT32*)(&pBuf8[byteCnt-4])) = 0xcdef6789;	
	
	
	qspiFlash_SndBytes_Direct(pQspi, offset, byteCnt, pBuf8);
	
	printf("----pQspi(%d)-write(sect-%d):%dKB----\n", pQspiCtrl->ctrl_x, sect_idx, (byteCnt/1024));	
	
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
	
	for (j=(byteCnt-512); j<byteCnt; j++)
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
	printf("------------\n");
	
	return;
}

void test_qspi_erase_sect_4K(int sect_idx)
{
	vxT_QSPI * pQspi = g_pQspi0;
	int ret = 0;
	
	vxInit_Qspi(QSPI_CTRL_0);
	
	ret = qspiFlash_Erase_Sect_4K(pQspi, sect_idx);
    if (ret == FMSH_FAILURE)
    {
    	printf("ctrl_%d: qspiflash erase sect_4K: %d fail! \n", pQspi->qspi_x, sect_idx);
    }
	else
	{
		printf("ctrl_%d: qspiflash erase sect_4K: %d ok! \n", pQspi->qspi_x, sect_idx);
	}
	
	return;
}


/*
qspi flash chip: 
	some are only 64K erase sect, and used as 4K size space of 64K
	some are 4k erase sect, and used as whole 4k size space of 4K
----------------
4K : W25Q256FV
64K: S25FL256S ...
*/

#if 0  /* for 4K sector chip*/
#undef ERASE_SECT_64K   
#else  /* for 64K sector chip*/
#define ERASE_SECT_64K   
#endif

void fatFS_W25Qxx_Read_Sect_4K(UINT8 * buff, UINT64  sector)
{
	vxT_QSPI * pQspi = g_pQspi0;
	vxT_QSPI_FLASH * pQspiFlash = pQspi->pQspiFlash;

	UINT8* pBuf8 = (UINT8*)(buff);
	int sect_idx = sector;
	
	int byteCnt = 0, offset = 0;
		
	vxInit_Qspi(QSPI_CTRL_0);
	
#ifdef ERASE_SECT_64K
	pQspiFlash->sectSize = 0x10000;  /* 64K*/
	
	byteCnt = 0x1000;  /* 4K;*/
	offset = sect_idx * pQspiFlash->sectSize;
#else
	pQspiFlash->sectSize = 0x1000;  /* 4K*/
	
	byteCnt = pQspiFlash->sectSize;
	offset = sect_idx * pQspiFlash->sectSize;
#endif

	
	qspiFlash_RcvBytes_Direct(pQspi, offset, byteCnt, pBuf8);

	return;
}
									
void fatFS_W25Qxx_Write_Sect_4K(UINT8 * buff, UINT64  sector)
{
	vxT_QSPI * pQspi = g_pQspi0;
	vxT_QSPI_FLASH * pQspiFlash = pQspi->pQspiFlash;

	UINT8* pBuf8 = (UINT8*)(buff);
	int sect_idx = sector;
	
	int byteCnt = 0, offset = 0;
		
	vxInit_Qspi(QSPI_CTRL_0);

#ifdef ERASE_SECT_64K
	pQspiFlash->sectSize = 0x10000;  /* 64K*/
	
	byteCnt = 0x1000;	/* 4K*/
	offset = sect_idx * pQspiFlash->sectSize;
#else
	pQspiFlash->sectSize = 0x1000;	/* 4K*/
	
	byteCnt = pQspiFlash->sectSize;
	offset = sect_idx * pQspiFlash->sectSize;
#endif
	
	
	qspiFlash_SndBytes_Direct(pQspi, offset, byteCnt, pBuf8);

	return;
}


void fatFS_W25Qxx_Erase_Sect_4K(int sect_idx)
{
	vxT_QSPI * pQspi = g_pQspi0;
	
	vxInit_Qspi(QSPI_CTRL_0);

#ifdef ERASE_SECT_64K
	pQspi->pQspiFlash->sectSize = 0x10000;  /* 64K*/
	qspiFlash_Erase_Sect(pQspi, sect_idx);
	
#else
	pQspi->pQspiFlash->sectSize =  0x1000;  /* 4K*/
	qspiFlash_Erase_Sect_4K(pQspi, sect_idx);
#endif

	return;
}
									
#endif

#if 1

void test_qspi_wr_file(char * filename)
{
	float speed = 0.0;
	UINT32 all_size = 0;
	int tick_start = 0, tick_end = 0;
	int fd = 0;
	char file_name[64] = {0};	
	int file_size = 0x400000 + 64;  /* max: 2M*/
	
	UINT8* pBuf8 = (UINT8*)malloc(file_size);
	int i = 0;
	
	for (i=0; i<file_size; i++)
	{
		pBuf8[i] = i;
	}
	
	/**/
	/* write*/
	/*	*/
	tick_start = tickGet();
	
	sprintf(file_name, "/tffs0/%s", filename);
	
	/*
	fd = open(file_name, O_RDWR, 0);
	if (fd == -1)
	{
		logMsg("error: create file(%s) failed \r\n", file_name,0,0,0,0,0);
		goto ERR;
	}
	*/
	fd = creat(file_name, O_RDWR); 
	if (fd == -1)
	{
		logMsg("error: creat file(%s) failed\r\n", file_name, 0,0,0,0,0);
		goto ERR;
	}
	
	file_size = 0x400000; /* 2M 	*/
	all_size = write(fd, pBuf8, file_size); 
	
	if (all_size != file_size)
	{
		logMsg("error: write %d bytes data\r\n", all_size,0,0,0,0,0);
		close(fd);
		goto ERR;
	}
	tick_end = tickGet();	
	
	taskDelay(10);
	speed = (float)(all_size * 1.0) / ((float)(tick_end - tick_start)*1.0 / sysClkRateGet());
	logMsg("write %d bytes data of file(%s) \r\n", all_size, file_name, 0,0,0,0);

	taskDelay(10);
	printf("test vx69_qspi_dosfs write_speed: %.2f bytes/s \n", speed, 0,0,0,0,0);
	close(fd);

ERR:	
	free(pBuf8);	
	return;
}

void test_qspi_rd_file(char* filename)
{
	float speed = 0.0;
	UINT32 all_size = 0;
	int tick_start = 0, tick_end = 0;
	int fd = 0, i = 0;
	char file_name[64] = {0};	
	int file_size = 0x400000 + 64;  /* max: 4M*/
	
	UINT8* pBuf8 = (UINT8*)malloc(file_size);

	tick_start = tickGet();

	sprintf(file_name, "/tffs0/%s", filename);
	
	/* read*/
	fd = open(file_name, O_RDWR, 0);
	if(fd == -1)
	{
		logMsg("error: open file(%s) failed \r\n", file_name, 0,0,0,0,0);
		goto ERR;
	}
	
	all_size = read(fd, pBuf8, file_size);	
	tick_end = tickGet();
	
	for (i=0; i<64; i++)
	{
		printf("%02X", pBuf8[i]);
		
		if ((i+1)%8 == 0)
		{
			printf(" ");
		}
		
		if ((i+1)%32 == 0)
		{
			printf("\n");
		}
	}
	printf("---------\n");
	for (i=(all_size-64); i<all_size; i++)
	{
		printf("%02X", pBuf8[i]);
		
		if ((i+1)%8 == 0)
		{
			printf(" ");
		}
		
		if ((i+1)%32 == 0)
		{
			printf("\n");
		}
	}

	for (i=0; i<all_size; i++)
	{
		if (pBuf8[i] != (i & 0xFF))
		{
			printf("write source data err:< idx-%d, err-0x%02X,ok-0x%02X > \n", i, pBuf8[i], i&0xFF);
			break;
		}
	}
	
	if (i == all_size)
	{
		printf("write & read compare ok & no-err! \n");
	}
	
	taskDelay(10);
	speed = (float)(all_size * 1.0) / ((float)(tick_end - tick_start)*1.0 / sysClkRateGet());
	logMsg("read %d bytes data of file(%s) \r\n", all_size, file_name, 3,4,5,6);


	taskDelay(10);		
	printf("test vx69_qspi_dosfs read_speed: %.2f bytes/s \n", speed, 2,3,4,5,6);
	close(fd);

ERR:	
	free(pBuf8);	
	return;
}

#endif



