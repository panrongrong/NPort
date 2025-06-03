/* vxQspi_Nand.c - fmsh 7020/7045 pQspiNand driver, just tested for FM25S02A */

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

#include "vxQspi_Nand.h"


/*
defines 
*/
#if 1

#define VX_DBG
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
/* qspi_1: nandflash */
static vxT_QSPI_CTRL vxQspi_Ctrl1 = {0};
static vxT_QSPI_NAND vxQspi_Nand = {0};

static vxT_QSPI vx_pQspi_1 = {0};
vxT_QSPI * g_pQspi1 = (vxT_QSPI *)(&vx_pQspi_1.qspi_x);

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
	
    /* 
    Read configuration Register command(0x35) to device (1B) 
	*/
	/*
	Offset: 0x90	
	-------------
	Bit     R/W Description Reset
	31:24	R/W 命令操作符:	命令操作符应该在触发前设置完成。	8'h00
	23	    R/W 读数据使能: 	如果bit[31:24]需要从器件设备上读取数据	1'h0
	22:20	R/W 读数据个数: 	最多可以读取8byte数据(0 对应1byte… 7对应8byte)
				如果Flash Command Control Register bit[2]使能，该字段无效。
				读取数据大小依据Flash Command Control Memory Register 的bit[15:8]	3'h0
	19	    R/W 命令地址使能: 	当bit[31:24]需要传输地址时，置“1” 1'h0
	18	    R/W Mode 位使能。: 	该位被置“1”，mode位按照Mode Bit Configuration Register，并在地址被传输后被传送出去	1'h0
	17:16	R/W 地址自己数，设置需要传输的字节位宽
				2'b00	1byte
				2'b01	2byte
				2'b10	3byte
				2'b11	4byte	2'h00
	15		    写数据使能:  	当bit[31:24]需要写数据时，该位置“1”	1'h0
	14:12		写入数据字节数:  	最多可以写8byte数据(0 对应1byte… 7对应8byte)	3'h0
	11:7		Dummy时钟数 :	当bit[31:24]需要Dummy时钟，设置位置	5'h00
	6:3 	RO	Reserved	4'h0
	2		R/W STIG存储器bank使能 1'h0
	1		RO	STIG命令正在执行	1'h0
	0		WO	执行该命令 n/a
	*/
    qspiCtrl_Wr_CfgReg(pQspi, QSPIPS_FCCR_OFFSET, ctrl_cmd);

	/*
	wait pipeline idle status
	*/
    ret = qspiCtrl_Wait_BusIdle(pQspi);
    if (ret == FMSH_FAILURE)
	{
        return FMSH_FAILURE;
    }
	
    qspiCtrl_Wr_CfgReg(pQspi, QSPIPS_FCCR_OFFSET, (ctrl_cmd | 1));
	
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
	bit22~19:	R/W 主机模式下波特率分频(2到32)
	--------
	SPI波特率 = (主机 reference clock) / BD
	BD= 4'h0  /2
		4'h1  /4
		4'h2  /6 
		…
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

void qspiCtrl_Stig_Cmd_Exec(vxT_QSPI* pQspi, UINT32 reg) 
{
	int ret = 0;
	
	qspiCtrl_Wr_CfgReg(pQspi, QSPIPS_FCCR_OFFSET, reg);

	ret = qspiCtrl_Wait_BusIdle(pQspi);
	if (ret == FMSH_FAILURE) 
	{
		VX_DBG2("qspiCtrl_Stig_Cmd_Exec:wait idle fail! \r\n", 1,2,3,4,5,6);
	}

	qspiCtrl_Wr_CfgReg(pQspi, QSPIPS_FCCR_OFFSET, (reg | 1));

    ret = qspiCtrl_Wait_ExeOk(pQspi, QSPIPS_FCCR_OFFSET, QSPIPS_REG_FCCR_INPROGRESS_MASK, OK_FLAG_0);
	if (ret == FMSH_FAILURE) 
	{
		VX_DBG2("qspiCtrl_Stig_Cmd_Exec:wait for bit fail! \r\n", 1,2,3,4,5,6);
	}

	ret = qspiCtrl_Wait_BusIdle(pQspi);
	if (ret == FMSH_FAILURE) 
	{
		VX_DBG2("qspiCtrl_Stig_Cmd_Exec:wait idle fail! \r\n", 1,2,3,4,5,6);
	}
	
	return;
}

void qspiCtrl_Stig_Excute(vxT_QSPI* pQspi,
			              UINT8 opcode,
			              UINT32 addr,
			              UINT8 n_addr,
			              UINT8 * tx_buf,
			              UINT8 n_tx,
			              UINT8 * rx_buf,
			              UINT8 n_rx,
			              UINT8 n_dummy_clk) 
{
	UINT32 reg;
	UINT32 data;
	UINT8 status, read_len;	
	/*UINT32 reg_base = pQspi->pQspiCtrl->cfgBaseAddr;*/

	reg = opcode << CQSPI_REG_CMDCTRL_OPCODE_LSB;
	
	/* 
	write addr 
	*/
	if (0 != n_addr)
	{
		reg |= (0x1 << CQSPI_REG_CMDCTRL_ADDR_EN_LSB);
		reg |= ((n_addr - 1) & CQSPI_REG_CMDCTRL_ADD_BYTES_MASK)
		       << CQSPI_REG_CMDCTRL_ADD_BYTES_LSB;

		qspiCtrl_Wr_CfgReg(pQspi, CQSPI_REG_CMDADDRESS, addr);
	}

	/* 
	dummy 
	*/
	reg |= (n_dummy_clk << CQSPI_REG_CMDCTRL_DUM_CYCLE_LSB);

	/* 
	write data 
	*/
	if (NULL != tx_buf) 
	{
		if (!n_tx || n_tx > CQSPI_STIG_DATA_LEN_MAX)
		{
			return ;
		}
		
		reg |= (0x1 << CQSPI_REG_CMDCTRL_WR_EN_LSB);
		reg |= ((n_tx - 1) & CQSPI_REG_CMDCTRL_WR_BYTES_MASK) << CQSPI_REG_CMDCTRL_WR_BYTES_LSB;
		
		if (n_tx > 4)
		{
			data = 0;
			
			memcpy(&data, (tx_buf + 4), (n_tx - 4));
			
			qspiCtrl_Wr_CfgReg(pQspi, CQSPI_REG_CMDWRITEDATAUPPER, data);
			
			n_tx = 4;
		}
		
		data = 0;
		memcpy(&data, tx_buf, n_tx);
		
		qspiCtrl_Wr_CfgReg(pQspi, CQSPI_REG_CMDWRITEDATALOWER, data);
	}

	/*
	read data
	*/
	if (0 != n_rx) 
	{
		reg |= (0x1 << CQSPI_REG_CMDCTRL_RD_EN_LSB);
		/* 0 means 1 byte. */
		reg |= (((n_rx - 1) & CQSPI_REG_CMDCTRL_RD_BYTES_MASK) << CQSPI_REG_CMDCTRL_RD_BYTES_LSB);
	}

	/* exec */
	qspiCtrl_CmdExecute(pQspi, reg);

	if (0 != n_rx) 
	{
		data = qspiCtrl_Rd_CfgReg(pQspi, CQSPI_REG_CMDREADDATALOWER);
		
		/* Put the read value into rx_buf */
		read_len = (n_rx > 4) ? 4 : n_rx;		
		memcpy(rx_buf, &data, read_len);		
		rx_buf += read_len;
		
		if (n_rx > 4) 
		{
			data = qspiCtrl_Rd_CfgReg(pQspi, CQSPI_REG_CMDREADDATAUPPER);
			read_len = n_rx - read_len;
			memcpy(rx_buf, &data, read_len);
		}
	}

	return;
}

void qspiCtrl_Set_Status(vxT_QSPI* pQspi, UINT8 addr, UINT8 data)
{
	UINT32 reg;
	
	qspiCtrl_Wr_CfgReg(pQspi, QSPIPS_FCAR_OFFSET, addr); /* address*/
	qspiCtrl_Wr_CfgReg(pQspi, QSPIPS_FCWDRL_OFFSET, data); /* address*/
	
	reg = (0x1f << 24) | (1 << 19) | (1 << 15);
	
	qspiCtrl_Stig_Cmd_Exec(pQspi, reg);
	return;
}

UINT32 qspiCtrl_Get_Status(vxT_QSPI* pQspi, UINT8 status_reg) 
{
	UINT32 reg, ret = 0;
	UINT8 cmd = 0x0F;
	
	/* init */
	/*FQspiPs_SetFlashMode(qspiPtr, FAST_READ_CMD);  // x1*/
	
	/* read feature */
	qspiCtrl_Wr_CfgReg(pQspi, QSPIPS_FCAR_OFFSET, status_reg); /* address*/

	reg = (cmd << 24) | (1 << 23) | (0 << 20) | (1 << 19) ;
	qspiCtrl_Stig_Cmd_Exec(pQspi, reg);

	ret = qspiCtrl_Rd_CfgReg(pQspi, QSPIPS_FCRDRL_OFFSET);

	return ret;
}

void qspiCtrl_Rd_ALL_Status(vxT_QSPI* pQspi) 
{
	UINT8 reg;
	
	reg = qspiCtrl_Get_Status(pQspi, 0xa0);
	printf("feature A0 : 0x%x\r\n", reg);
	
	reg = qspiCtrl_Get_Status(pQspi, 0xb0);
	printf("feature B0 : 0x%x\r\n", reg);
	
	reg = qspiCtrl_Get_Status(pQspi, 0xc0);
	printf("feature C0 : 0x%x\r\n", reg);
	
	reg = qspiCtrl_Get_Status(pQspi, 0xd0);
	printf("feature D0 : 0x%x\r\n", reg);

	return;
}

void qspiCtrl_Rd_ALL_Status2(vxT_QSPI* pQspi) 
{
	UINT8 reg;
	
	reg = qspiCtrl_Get_Status(pQspi, 0xa0);
	printf("feature A0 : 0x%x  \n", reg);
	
	if (reg & 0x02)  /* bit1*/
	{
		printf("  bit1-CMP Protect Bit: 1 \n");
	}
	else
	{
		printf("  bit1-CMP Protect Bit: 0 \n");
	}
	if (reg & 0x04)  /* bit2*/
	{
		printf("  bit2-Top_Bottom Protect Bit: 1 \n");
	}
	else
	{
		printf("  bit2-Top_Bottom Protect Bit: 0 \n");
	}
	
	if (reg & 0x38)  /* bit5~3*/
	{
		printf("  bit5~3-Block Protect Bits: 0x%X \n", ((reg & 0x38) >> 3));
	}
	else
	{
		printf("  bit5~3-Block Protect Bits:0x%X \n", ((reg & 0x38) >> 3));
	}
	if (reg & 0x80)  /* bit7*/
	{
		printf("  bit7-Status Reg Protect: 1 \n");
	}
	else
	{
		printf("  bit7-Status Reg Protect: 0 \n");
	}
	printf("\n");

	
	reg = qspiCtrl_Get_Status(pQspi, 0xb0);
	printf("feature B0 : 0x%x  \n", reg);	
	
	if (reg & 0x01)  /* bit0*/
	{
		printf("  bit0-Quad Enable: 1 \n");
	}
	else
	{
		printf("  bit0-Quad Enable: 0 \n");
	}
	if (reg & 0x10)  /* bit4*/
	{
		printf("  bit4-Enable ECC: 1 \n");
	}
	else
	{
		printf("  bit4-Enable ECC: 0 \n");
	}
	if (reg & 0x40)  /* bit6*/
	{
		printf("  bit6-Enter OPT Mode: 1 \n");
	}
	else
	{
		printf("  bit6-Enter OPT Mode: 0 \n");
	}
	if (reg & 0x80)  /* bit7*/
	{
		printf("  bit7-OPT Data Pages Lock: 1 \n");
	}
	else
	{
		printf("  bit7-OPT Data Pages Lock: 0 \n");
	}	
	printf("\n");
	
	
	reg = qspiCtrl_Get_Status(pQspi, 0xc0);
	printf("feature C0 : 0x%x  \n", reg);
	
	if (reg & 0x01)  /* bit0*/
	{
		printf("  bit0-Operation in Process: 1 \n");
	}
	else
	{
		printf("  bit0-Operation in Process: 0 \n");
	}
	if (reg & 0x02)  /* bit1*/
	{
		printf("  bit1-Write Enable Latch: 1 \n");
	}
	else
	{
		printf("  bit1-Write Enable Latch: 0 \n");
	}
	if (reg & 0x04)  /* bit2*/
	{
		printf("  bit2-Erase Failure: 1 \n");
	}
	else
	{
		printf("  bit2-Erase Failure: 0 \n");
	}
	if (reg & 0x08)  /* bit3*/
	{
		printf("  bit3-Program Failure: 1 \n");
	}
	else
	{
		printf("  bit3-Program Failure: 0 \n");
	}
	if (reg & 0x30)  /* bit5~4*/
	{
		printf("  bit5~4-ECC Status Bits[1:0]: 0x%X \n", ((reg & 0x30) >> 4));
	}
	else
	{
		printf("  bit5~4-ECC Status Bits[1:0]: 0x%X \n", ((reg & 0x30) >> 4));
	}	
	printf("\n");

	
	reg = qspiCtrl_Get_Status(pQspi, 0xd0);
	printf("feature D0 : 0x%x \n", reg);
	
	if (reg & 0x60)  /* bit6~5*/
	{
		printf("  bit6~5-Output Drive Strength: 0x%X \n", ((reg & 0x60) >> 5));
	}
	else
	{
		printf("  bit6~5-Output Drive Strength: 0x%X \n", ((reg & 0x60) >> 5));
	}	
	if (reg & 0x80)  /* bit7*/
	{
		printf("  bit7-Die Select: 1 \n");
	}
	else
	{
		printf("  bit7-Die Select: 0 \n");
	}
	printf("\n");

	return;
}



#endif


#if 1

UINT8 qspiNand_Get_RegStatus1(vxT_QSPI* pQspi)
{
    int ret = 0;
	
    /* 
    Send Read Status1 Register command(0x05) to device 
	*/
    ret = qspiCtrl_CmdExecute(pQspi, 0x05800001);
    if (ret == FMSH_FAILURE)
	{
        return FMSH_FAILURE;
    }
    
    return qspiCtrl_Rd_CfgReg(pQspi, QSPIPS_FCRDRL_OFFSET) & 0xFF;
} 


int qspiNand_WREN_Cmd(vxT_QSPI* pQspi) 
{
    int ret = 0;
    UINT8 status;
    UINT32 timeout = 750; 
	
    /* 
    Send WREN(0x06) Command 
	*/
    ret = qspiCtrl_CmdExecute(pQspi, 0x06000001);    
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
        status = qspiNand_Get_RegStatus1(pQspi);
        if (status & WEL_MASK)
		{
            return FMSH_SUCCESS;
        }
		
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
int qspiNand_Wait_WIP_Ok(vxT_QSPI* pQspi)
{ 
    UINT8 tmp8;
    int timeout = 1500;
    
    /* 
    Poll Status Register1 
	*/
   	do
	{
        delay_1ms();
		
        tmp8 = qspiNand_Get_RegStatus1(pQspi) & BUSY_MASK;		
        if (tmp8 == 0x00) /* 1-Device Busy, 0-Ready Device is in standby */
		{
            return FMSH_SUCCESS;
        }   
		
        timeout--;
    } while (timeout > 0);
    
	return FMSH_FAILURE;
}


UINT32 qspiNand_Read_ID(vxT_QSPI* pQspi, UINT32* nand_id) 
{
	UINT32 flashId,reg;
	int ret;
	UINT8 cmd = 0x9F;
	
	reg = (cmd<<24) | (1<<23) | (1<<20) | (8<<7);
	
	qspiCtrl_Wr_CfgReg(pQspi, QSPIPS_FCCR_OFFSET, reg);

	ret = qspiCtrl_Wait_BusIdle(pQspi);
	if (ret == FMSH_FAILURE) 
	{
		printf("qspiCtrl_Wait_BusIdle fail!  \n");
	}

	qspiCtrl_Wr_CfgReg(pQspi, QSPIPS_FCCR_OFFSET, (reg | 1));

	ret = qspiCtrl_Wait_ExeOk(pQspi, QSPIPS_FCCR_OFFSET, QSPIPS_REG_FCCR_INPROGRESS_MASK, OK_FLAG_0);
	if (ret == FMSH_FAILURE) 
	{
		printf("qspiCtrl_Wait_ExeOk fail!  \n");
	}

	ret = qspiCtrl_Wait_BusIdle(pQspi);
	if (ret == FMSH_FAILURE) 
	{
		printf("qspiCtrl_Wait_BusIdle fail!  \n");
	}

	flashId = qspiCtrl_Rd_CfgReg(pQspi, QSPIPS_FCRDRL_OFFSET);
	*nand_id = flashId;
	
	printf("nand_flash ID : 0x%x \n", flashId);
	return flashId;
}


UINT32 qspiNand_ReadId_Stig(vxT_QSPI* pQspi)
{
    UINT16 id = 0;
	
    qspiCtrl_Stig_Excute(pQspi, 0x9f,
                        NULL,
                        0,
                        NULL,
                        0,
                        (UINT8*)&id,
                        3,
                        8);
	
    printf("stig read ID 0x%x\r\n", id);

	return id;
}

int qspiNand_SectErase_CmdExe(vxT_QSPI* pQspi, UINT32 start_offset)
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
    
    ret = qspiNand_WREN_Cmd(pQspi);
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
		
        tmp8 = qspiNand_Get_RegStatus1(pQspi) & BUSY_MASK;		
        if (tmp8 == 0x00) /* 1-Device Busy, 0-Ready Device is in standby */
		{
            return FMSH_SUCCESS;
        }   
		
        timeout--;
    } while (timeout > 0);
	
	return FMSH_FAILURE;
}


/***************
* pQspi flash reset
*
***************/
void qspiNand_Reset_Flash(vxT_QSPI* pQspi)
{
    qspiCtrl_Stig_Excute(pQspi,0xff,0,0,NULL,0,0,0,0);	
    return;
}

/*
en - 0: disable
     1: enable ecc
*/
void qspiNand_Enable_ECC(vxT_QSPI* pQspi, UINT8 en)
{
    UINT8 status;
	
    qspiCtrl_Stig_Excute(pQspi, 0x0f, 0xb0, 1, NULL, 0, &status, 1, 0);
	
    if (en)
	{
        status |= 0x10;
    }
	else
	{
        status &= ~0x10;
    }
	
    qspiCtrl_Stig_Excute(pQspi, 0x1f, 0xb0, 1, &status, 1, NULL, 0, 0);

	return;
}


#endif


#if 1

/*
not vxBus 
metal-dirver in vxworks
*/
int vxInit_Qspi_Nand(void)
{
	vxT_QSPI * pQspi = NULL;
	vxT_QSPI_CTRL * pQspiCtrl = NULL;
	vxT_QSPI_NAND * pQspiNand = NULL;
	
	UINT32 flash_ID[2] = {0};
	int ret = 0;
	
	UINT32 tmp32 = 0;
   
    int mode = 0, bdr = 0;
	
	int ctrl_x = 1;  /* qspi_1 ->interface-> nandflash*/


#if 1 
	/*
	init the pQspi structure
	*/
	pQspi = g_pQspi1;

	pQspi->qspi_x = QSPI_CTRL_1;
	pQspi->pQspiCtrl = (vxT_QSPI_CTRL *)(&vxQspi_Ctrl1.ctrl_x);
	pQspi->pQspiNand = (vxT_QSPI_NAND *)(&vxQspi_Nand.status);

	pQspi->pQspiCtrl->devId = QSPI_CTRL_1;
	pQspi->pQspiCtrl->cfgBaseAddr = VX_QSPI_1_CFG_BASE;
	pQspi->pQspiCtrl->fifoBaseAddr = VX_QSPI_1_FIFO_BASE;		
	
	pQspiCtrl = pQspi->pQspiCtrl;
	pQspiNand = pQspi->pQspiNand;
#endif	
	
	if (pQspi->init_flag == QSPI_CTRL_INIT_OK)
	{
		return FMSH_SUCCESS;  /* qspi_ctrl init already*/
	}

	pQspiCtrl->ctrl_x = ctrl_x;
	pQspiCtrl->isBusy = 0; /* no busy */

    /* 
    reset controller 
	*/
	qspiCtrl_Reset(pQspi);

	
	/*
	read flash id
	*/
#if 0
	qspiNand_Read_ID(pQspi, (UINT32 *)(&flash_ID[0]));
	/* Vendor and Device ID : FM25S02A:0xe5a1 */
	VX_DBG("flash ID1_2: 0x%08X_%08X \n", flash_ID[0],flash_ID[1], 3,4,5,6);
#else
	/* Vendor and Device ID : FM25S02A:0xe5a1 */
	qspiNand_ReadId_Stig(pQspi);
#endif
	
	/* for test*/
	qspiCtrl_Rd_ALL_Status(pQspi);
	
	/* unlock*/
	qspiCtrl_Set_Status(pQspi, 0xA0, 0);
	
	/* Quad Mode Enable*/
	qspiCtrl_Set_Status(pQspi, 0xB0, 0x11);

	/*
	FM25S02A 3.3V spi nand flash
	----------------------------
	page : 2112 bytes (2048+64)
	block: 64 pages
	plane: 2 * 1024blocks/plane
	device: 2Gb (2048 blocks) => 256M bytes
	*/
#if 0
	pQspiNand->pageSize = 2048 + 64;
	pQspiNand->blkSize = 64 * pQspiNand->pageSize;
	pQspiNand->pages_1_blk = 64;	
#else
	pQspiNand->pageSize    = 2048;
	pQspiNand->spareSize   = 64;	
	pQspiNand->pages_1_blk = 64;
	
	pQspiNand->blkSize = pQspiNand->pages_1_blk * (pQspiNand->pageSize + pQspiNand->spareSize);
#endif

	/**/
	/* reset nandflash*/
	/**/
	qspiNand_Reset_Flash(pQspi);

	pQspi->sem = semMCreate (SEM_Q_PRIORITY | SEM_DELETE_SAFE | SEM_INVERSION_SAFE);	
	pQspi->init_flag = QSPI_CTRL_INIT_OK;

	return OK;
}

#endif


#if 1  /* InDirect mode*/

#define NAND_X1_MODE   (0)   /* mode:0-x1, 1-x4 */

#define __InDirect_Fifo_Mode__

/*
x1 mode: 1-line indirect read setup
*/
int qspiCtrl_FastRcv_Setup_x1(vxT_QSPI* pQspi, UINT32 flash_offset, UINT32 byteCnt, UINT8* rcvBuf)
{
	vxT_QSPI_CTRL * pQspiCtrl = pQspi->pQspiCtrl;
	vxT_QSPI_NAND * pQspiNand = pQspi->pQspiNand;
		
    UINT32 cfgReg = 0;
    UINT32 rxFullLvl = 480;    
    
    QSPI_SEM_TAKE(pQspi->sem);
    
    pQspiCtrl->requestBytes = byteCnt;
    pQspiCtrl->remainBytes = byteCnt;
    pQspiCtrl->rcvBuf = rcvBuf;
	
	/* 
	setup Instruction 
	*/
	cfgReg = (8 << 24) | (0xb);   /* 8 clk dummy*/
	qspiCtrl_Wr_CfgReg(pQspi, QSPIPS_DRIR_OFFSET, cfgReg);

	/* 
	setup device config 
	*/
	/*cfgReg = (2 << 21)| (2048 << 4) | (1);  // 2 byte address*/
	cfgReg = (2 << 21)| (pQspiNand->pageSize << 4) | (1);  /* 2 byte address*/
	qspiCtrl_Wr_CfgReg(pQspi, QSPIPS_DSCR_OFFSET, cfgReg);
	
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
    rxFullLvl = 480;
    qspiCtrl_Wr_CfgReg(pQspi, QSPIPS_IRTWR_OFFSET, rxFullLvl);  /* rx*/
	
    /* 
    trigger Indirect Write access
    */
    cfgReg = 0x01;
    qspiCtrl_Wr_CfgReg(pQspi, QSPIPS_IRTCR_OFFSET, cfgReg);  
	
    QSPI_SEM_GIVE(pQspi->sem);
	
    return FMSH_SUCCESS;
}


/*
x4 mode: 4-line indirect read setup
*/
int qspiCtrl_FastRcv_Setup_x4(vxT_QSPI* pQspi, UINT32 flash_offset, UINT32 byteCnt, UINT8* rcvBuf)
{
	vxT_QSPI_CTRL * pQspiCtrl = pQspi->pQspiCtrl;
		
    UINT32 cfgReg = 0;
    UINT32 rxFullLvl = 480;    
    
    QSPI_SEM_TAKE(pQspi->sem);
    
    pQspiCtrl->requestBytes = byteCnt;
    pQspiCtrl->remainBytes = byteCnt;
    pQspiCtrl->rcvBuf = rcvBuf;
	
	/* 
	setup Instruction 
	*/
	/*reg =        |(8<<24)|(0x3b);   // x1	*/
	/*reg = (1<<16)|(8<<24)|(0x3b);   // x2*/
	cfgReg = (2 << 16) | (8 << 24) | (0x6b);   /* x4 8 clk dummy*/
	qspiCtrl_Wr_CfgReg(pQspi, QSPIPS_DRIR_OFFSET, cfgReg);

	/* 
	setup device config 
	*/
	cfgReg = (2 << 21)| (2048 << 4) | (1);  /* 2 byte address*/
	qspiCtrl_Wr_CfgReg(pQspi, QSPIPS_DSCR_OFFSET, cfgReg);
	
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
    rxFullLvl = 480;
    qspiCtrl_Wr_CfgReg(pQspi, QSPIPS_IRTWR_OFFSET, rxFullLvl);  /* rx*/
	
    /* 
    trigger Indirect Write access
    */
    cfgReg = 0x01;
    qspiCtrl_Wr_CfgReg(pQspi, QSPIPS_IRTCR_OFFSET, cfgReg);  
	
    QSPI_SEM_GIVE(pQspi->sem);
	
    return FMSH_SUCCESS;
}


/*
x1 mode: 1-line indirect write setup
*/
int qspiCtrl_FastSnd_Setup_x1(vxT_QSPI* pQspi, UINT32 flash_offset, UINT32 byteCnt, UINT8* sndBuf)
{
	vxT_QSPI_CTRL * pQspiCtrl = pQspi->pQspiCtrl;
	vxT_QSPI_NAND * pQspiNand = pQspi->pQspiNand;
		
    UINT32 cfgReg = 0;
    UINT32 txEmptyLvl = 64;    
    
    QSPI_SEM_TAKE(pQspi->sem);
    
    pQspiCtrl->requestBytes = byteCnt;
    pQspiCtrl->remainBytes = byteCnt;
    pQspiCtrl->sndBuf = sndBuf;
	
	/* 
	setup Instruction 
	*/
	cfgReg = (0x2);
	qspiCtrl_Wr_CfgReg(pQspi, QSPIPS_DWIR_OFFSET, cfgReg);

	/* 
	setup device config
	*/
	/*cfgReg = (2 << 21) | (2048 << 4) | (1);  // 2 byte address*/
	cfgReg = (2 << 21) | (pQspiNand->pageSize << 4) | (1);  /* 2 byte address*/
	qspiCtrl_Wr_CfgReg(pQspi, QSPIPS_DSCR_OFFSET, cfgReg);
	
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
    txEmptyLvl = 64;
    qspiCtrl_Wr_CfgReg(pQspi, QSPIPS_IWTWR_OFFSET, txEmptyLvl);   /* tx*/
	
    /* 
    trigger Indirect Write access 
	*/
    cfgReg = 0x01;
    qspiCtrl_Wr_CfgReg(pQspi, QSPIPS_IWTCR_OFFSET, cfgReg);
    
    QSPI_SEM_GIVE(pQspi->sem);
	
    return FMSH_SUCCESS;
}


/*
x4 mode: 4-line indirect write setup
*/
int qspiCtrl_FastSnd_Setup_x4(vxT_QSPI* pQspi, UINT32 flash_offset, UINT32 byteCnt, UINT8* sndBuf)
{
	vxT_QSPI_CTRL * pQspiCtrl = pQspi->pQspiCtrl;
	vxT_QSPI_NAND * pQspiNand = pQspi->pQspiNand;
		
    UINT32 cfgReg = 0;
    UINT32 txEmptyLvl = 64;    
	
    QSPI_SEM_TAKE(pQspi->sem);
    
    pQspiCtrl->requestBytes = byteCnt;
    pQspiCtrl->remainBytes = byteCnt;
    pQspiCtrl->sndBuf = sndBuf;
	
	/* 
	setup Instruction 
	*/
	cfgReg = (0x32) | (2 << 16);  /* load x4*/
	qspiCtrl_Wr_CfgReg(pQspi, QSPIPS_DWIR_OFFSET, cfgReg);

	/* 
	setup device config
	*/
	/*cfgReg = (17 << 16) | (2048 << 4) | (1);  // 2 byte address*/
	cfgReg = (17 << 16) | (pQspiNand->pageSize << 4) | (1);  /* 2 byte address*/
	qspiCtrl_Wr_CfgReg(pQspi, QSPIPS_DSCR_OFFSET, cfgReg);
	
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
    txEmptyLvl = 64;
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

#define __qspi_nandflash_api__

#if 1
/*
FM25S02A 3.3V spi nand flash
----------------------------
page : 2112 bytes (2048+64)
block: 64 pages
plane: 2 * 1024blocks/plane
device: 2Gb (2048 blocks) => 256M bytes

blk_idx: 0, 1, 2, 3, ... ... 2047 
page_idx: 0, 1, 2, 3, ... ...
---------
page_idx -> blk_idx
    0    ->    0
	64	 ->    1
	128	 ->    2
	...
*/
int qspiNand_Erase_Blk(vxT_QSPI* pQspi, int blk_idx)
{
	vxT_QSPI_CTRL * pQspiCtrl = pQspi->pQspiCtrl;
	vxT_QSPI_NAND * pQspiNand = pQspi->pQspiNand;

    int ret = 0;
	UINT32 reg = 0;
	int timeout = 0;
	int page_idx = 0;
	
    QSPI_SEM_TAKE(pQspi->sem);	

	/* 
	write enable
	*/
	reg = 0x6 << 24;
	qspiCtrl_Stig_Cmd_Exec(pQspi, reg);

	/* 
	erase 
	*/
	page_idx = blk_idx * pQspiNand->pages_1_blk;
	qspiCtrl_Wr_CfgReg(pQspi, QSPIPS_FCAR_OFFSET, page_idx); /* address	*/
	
	reg = (0xd8 << 24) | (1 << 19) | (2 << 16); /* 3 byte addr*/
	qspiCtrl_Stig_Cmd_Exec(pQspi, reg);

	/* 
	get status 
	*/	
	timeout = 0;
	do 
	{
		reg = qspiCtrl_Get_Status(pQspi, 0xc0);
		
		timeout++;
		if (timeout > 20000)
		{
			QSPI_SEM_GIVE(pQspi->sem);
			return FMSH_FAILURE;	/* read page fail*/
		}
	} while ((reg & 1) == 1);
    
    QSPI_SEM_GIVE(pQspi->sem);
    
    return FMSH_SUCCESS;
}

#endif

#if 1
void qspiNand_Rd_PreReady(vxT_QSPI* pQspi, UINT32 page_idx)
{
	UINT32 reg = 0;
	int timeout = 0;
	UINT8 bdr = 0;
	
	/* 
	page read to cache 
	*/
	qspiCtrl_Wr_CfgReg(pQspi, QSPIPS_FCAR_OFFSET, page_idx); /* page address*/
	
	reg = (0x13 << 24) | (1 << 19) | (2 << 16);   /* 3 byte address*/
	qspiCtrl_Stig_Cmd_Exec(pQspi, reg);

	/* 
	GET FEATURE command to read the status 
	*/
	timeout = 0;
	do 
	{
		reg = qspiCtrl_Get_Status(pQspi, 0xc0);

		timeout++;
		if (timeout > 20000)
		{
			return -1;  /* read page fail*/
		}
	} while ((reg & 1) == 1);

	/* 
	READ FROM CACHE 
	*/
	bdr = 4; 
	
	if (bdr==2)
	{
		qspiCtrl_Wr_CfgReg(pQspi, QSPIPS_RDCR_OFFSET, 0x9);
	} 
	else if(bdr==4) 
	{
		qspiCtrl_Wr_CfgReg(pQspi, QSPIPS_RDCR_OFFSET, 0x5);
	} 
	else 
	{
		qspiCtrl_Wr_CfgReg(pQspi, QSPIPS_RDCR_OFFSET, 0x1);
	}
	
	qspiCtrl_Set_BaudRate(pQspi, QSPIPS_BAUD(bdr));

	return;
}


int qspiNand_Rd_Data(vxT_QSPI* pQspi, int page_idx, UINT8* rcvBuf8, int byte_cnt)
{
	vxT_QSPI_CTRL * pQspiCtrl = pQspi->pQspiCtrl;
	
	int ret = 0, idx = 0, timeout = 0;
	UINT32 tmp32 = 0;
	UINT32 byteCnt = 0, wdCnt = 0, remainder = 0;

	
	pQspiCtrl->rcvBuf = rcvBuf8;
	byteCnt = byte_cnt;

	do 
	{
		/*	Read  Watermark */
		wdCnt = qspiCtrl_Rd_CfgReg(pQspi, QSPIPS_SFLR_OFFSET);			
		remainder = pQspiCtrl->requestBytes & 0x3;
		
		for (idx=0; idx<wdCnt; idx++)
		{
			if (pQspiCtrl->requestBytes >= 4)
			{
				*((UINT32*)(pQspiCtrl->rcvBuf)) = *((UINT32*)(pQspiCtrl->fifoBaseAddr));
				
				pQspiCtrl->rcvBuf += 4;
				pQspiCtrl->requestBytes -= 4;  
				pQspiCtrl->remainBytes -= 4;
			}
			else
			{				
				if (remainder == 0)
				{	
					break; /* for()*/
				}
				
				tmp32 = *((UINT32*)(pQspiCtrl->fifoBaseAddr));
				while (remainder > 0)
				{
					*pQspiCtrl->rcvBuf = tmp32 & 0xff;
					tmp32 = tmp32 >> 8;
					
					pQspiCtrl->rcvBuf += 1;
					pQspiCtrl->remainBytes -= 1; 
					remainder--;
				}				
				break; /* for()*/
			}
		}

		if (pQspiCtrl->remainBytes <= 0)
		{
			break;	/* while()*/
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
}

/*
only page data area: 2048 bytes
*/
int qspiNand_Read_Page(vxT_QSPI* pQspi, int page_idx, UINT8* rcvBuf8)
{
	vxT_QSPI_NAND * pQspiNand = pQspi->pQspiNand;
	
	int ret = 0;
	UINT32 byteCnt = 0;

	/* */
	/* read mode setup etc.*/
	/**/
	qspiNand_Rd_PreReady(pQspi, page_idx);
	
	/* 
	read 
	*/
	byteCnt = pQspiNand->pageSize;
	
#if NAND_X1_MODE
	qspiCtrl_FastRcv_Setup_x1(pQspi, 0, byteCnt, rcvBuf8);
#else
	qspiCtrl_FastRcv_Setup_x4(pQspi, 0, byteCnt, rcvBuf8);
#endif	

	/**/
	/*  indirect read to sram from nand*/
	/**/
	ret = qspiNand_Rd_Data(pQspi, page_idx, rcvBuf8, byteCnt);	

	return ret;
}


/*
page data area + spare area: 2048 + 64 bytes
*/
int qspiNand_Read_Page_Ext(vxT_QSPI* pQspi, int page_idx, UINT8* rcvBuf8)
{
	vxT_QSPI_NAND * pQspiNand = pQspi->pQspiNand;
	
	int ret = 0;
	UINT32 byteCnt = 0;
	
	/* */
	/* read mode setup etc.*/
	/**/
	qspiNand_Rd_PreReady(pQspi, page_idx);
	
	/* 
	read 
	*/
	byteCnt = pQspiNand->pageSize + pQspiNand->spareSize;  /* page + ext*/
	
#if NAND_X1_MODE
	qspiCtrl_FastRcv_Setup_x1(pQspi, 0, byteCnt, rcvBuf8);
#else
	qspiCtrl_FastRcv_Setup_x4(pQspi, 0, byteCnt, rcvBuf8);
#endif	

	/**/
	/*  indirect read to sram from nand*/
	/**/
	ret = qspiNand_Rd_Data(pQspi, page_idx, rcvBuf8, byteCnt);	

	return ret;
}

/*
spare area: 64 bytes
*/
int qspiNand_Read_Page_Spare(vxT_QSPI* pQspi, int page_idx, UINT8* rcvBuf8)
{
	vxT_QSPI_NAND * pQspiNand = pQspi->pQspiNand;
	
	int ret = 0;
	UINT32 byteCnt = 0, flash_offset = 0;
	
	/* */
	/* read mode setup etc.*/
	/**/
	qspiNand_Rd_PreReady(pQspi, page_idx);
	
	/* 
	read 
	*/
	byteCnt = pQspiNand->spareSize;  /* spare: 64 bytes*/
	flash_offset = pQspiNand->pageSize;
	qspiCtrl_FastRcv_Setup_x4(pQspi, flash_offset, byteCnt, rcvBuf8);

	/**/
	/*  indirect read to sram from nand*/
	/**/
	ret = qspiNand_Rd_Data(pQspi, page_idx, rcvBuf8, byteCnt);	
	return ret;
}

#endif

#if 1

void qspiNand_Wr_PreReady(vxT_QSPI* pQspi)
{
	int bdr = 0;
	
	/* Quad mode enable*/
	qspiCtrl_Set_Status(pQspi, 0xb0, 0x11);
	
	/* 
	send to cache 
	*/
	bdr = 32;  /* val more bigger, baudrate more littler*/
	if (bdr == 2) 
	{
		qspiCtrl_Wr_CfgReg(pQspi, QSPIPS_RDCR_OFFSET, 0x9);
	} 
	else if (bdr == 4) 
	{
		qspiCtrl_Wr_CfgReg(pQspi, QSPIPS_RDCR_OFFSET, 0x5);
	} 
	else 
	{
		qspiCtrl_Wr_CfgReg(pQspi, QSPIPS_RDCR_OFFSET, 0x1);
	}
	
	qspiCtrl_Set_BaudRate(pQspi, QSPIPS_BAUD(bdr));	
	qspiCtrl_Wr_CfgReg(pQspi, QSPIPS_WCCR_OFFSET, (0x1<<14));

	return;
}

int qspiNand_Wr_Data(vxT_QSPI* pQspi, int page_idx, UINT8* sndBuf8, int byte_cnt)
{
	vxT_QSPI_CTRL * pQspiCtrl = pQspi->pQspiCtrl;
	vxT_QSPI_NAND * pQspiNand = pQspi->pQspiNand;
		
	int ret = 0;
	UINT32 byteCnt = 0, wdCnt = 0, remainder = 0;
	int idx = 0;
	int timeout = 0;	
	UINT32 reg = 0, tmp32 = 0;
	UINT8 bdr = 0;
	
	UINT32 dataCnt = 0;
	UINT32 flash_offset = 0;
	
	pQspiCtrl->sndBuf = sndBuf8;
	byteCnt = byte_cnt;

	/* 
	copy data from sram to flash 
	*/	
	do
	{
		wdCnt = byteCnt >> 2;
		
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
				
				break;	/* for()*/
			}
		}

		if (pQspiCtrl->remainBytes <= 0)
		{
			break;	/* while()*/
		}
	} while (pQspiCtrl->remainBytes > 0);

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

#if 1
	qspiCtrl_Wait_BusIdle(pQspi);
	
	timeout = 0;
	do 
	{
		reg = qspiCtrl_Get_Status(pQspi, 0xc0);
		
		timeout++;
		if (timeout > 200000)
		{
			byteCnt = 0;  /* write page fail!*/
			break;
		}
	} while ((reg & 1) == 1);
#endif	
	
	/* 
	write enable 
	*/
	reg = 0x6 << 24;
	qspiCtrl_Stig_Cmd_Exec(pQspi, reg);

	/* 
	prog exe 
	*/	
	qspiCtrl_Wr_CfgReg(pQspi, QSPIPS_FCAR_OFFSET, page_idx); /* address: page idx: 0,1,2 ...*/
	reg = (0x10 << 24) | (1 << 19) | (2 << 16); /* 3 byte addr*/
	qspiCtrl_Stig_Cmd_Exec(pQspi, reg);

	/* wait status */
	timeout = 0;
	do 
	{
		reg = qspiCtrl_Get_Status(pQspi, 0xc0);
		delay_1us();
		
		timeout++;
		if (timeout > 200000)
		{
			byteCnt = 0;  /* write page fail!*/
			break;
		}
	} while ((reg & 1) == 1);

	return byteCnt;
}

/*
page data area: 2048
*/
int qspiNand_Wr_Page_x4(vxT_QSPI* pQspi, int page_idx, UINT8* sndBuf8)
{
	vxT_QSPI_CTRL * pQspiCtrl = pQspi->pQspiCtrl;
	vxT_QSPI_NAND * pQspiNand = pQspi->pQspiNand;
		
	int ret = 0;
	UINT32 byteCnt = 0;	

	/* */
	/* write mode setup etc.*/
	/**/
	qspiNand_Wr_PreReady(pQspi);

	/**/
	/* setup indirect snd x4*/
	/**/
	qspiCtrl_FastSnd_Setup_x4(pQspi, 0x0, pQspiNand->pageSize, sndBuf8);

	/**/
	/*  indirect write sram to nand*/
	/**/
	ret = qspiNand_Wr_Data(pQspi, page_idx, sndBuf8, byteCnt);	
	return ret;
}


/*
page data area: 2048
*/
int qspiNand_Wr_Page_x1(vxT_QSPI* pQspi, int page_idx, UINT8* sndBuf8)
{
	vxT_QSPI_CTRL * pQspiCtrl = pQspi->pQspiCtrl;
	vxT_QSPI_NAND * pQspiNand = pQspi->pQspiNand;
		
	int ret = 0;
	UINT32 byteCnt = 0, wdCnt = 0, remainder = 0;
	int idx = 0;
	int timeout = 0;	
	UINT32 reg = 0, tmp32 = 0;
	UINT8 bdr = 0;
	
	UINT32 dataCnt = 0;
	UINT8 * pBuf8 = NULL;

	UINT32 flash_offset = 0;

	/* */
	/* write mode setup etc.*/
	/**/
	qspiNand_Wr_PreReady(pQspi);

	/**/
	/* setup indirect snd x1*/
	/**/
	qspiCtrl_FastSnd_Setup_x1(pQspi, 0x0, pQspiNand->pageSize, sndBuf8);

	/**/
	/*  indirect write sram to nand*/
	/**/
	ret = qspiNand_Wr_Data(pQspi, page_idx, sndBuf8, byteCnt);	
	return ret;
}


/*
x1 or x4 mode, page data area: 2048 bytes
*/
int qspiNand_Write_Page(vxT_QSPI* pQspi, int page_idx, UINT8* sndBuf8)
{
#if NAND_X1_MODE
	return qspiNand_Wr_Page_x1(pQspi, page_idx, sndBuf8);
#else
	return qspiNand_Wr_Page_x4(pQspi, page_idx, sndBuf8);
#endif
}


/*
only x4 mode, spare area: 64 bytes
*/
int qspiNand_Write_Spare(vxT_QSPI* pQspi, int page_idx, UINT8* sndBuf8)
{
	vxT_QSPI_CTRL * pQspiCtrl = pQspi->pQspiCtrl;
	vxT_QSPI_NAND * pQspiNand = pQspi->pQspiNand;
		
	int ret = 0;
	UINT32 byteCnt = 0;	
	UINT32 flash_offset = 0;

	/* */
	/* write mode setup etc.*/
	/**/
	qspiNand_Wr_PreReady(pQspi);
	
	/**/
	/* setup snd */
	/**/
	byteCnt = pQspiNand->spareSize;
	flash_offset = pQspiNand->pageSize;
	qspiCtrl_FastSnd_Setup_x4(pQspi, flash_offset, byteCnt, sndBuf8);

	/**/
	/*  indirect write sram to nand*/
	/**/
	ret = qspiNand_Wr_Data(pQspi, page_idx, sndBuf8, byteCnt);	
	return ret;
}


#define __512_BYTES_MODE__

int qspiNand_Wr_Page_0_0p5k(vxT_QSPI* pQspi, int page_idx, UINT8* sndBuf8)
{
	vxT_QSPI_CTRL * pQspiCtrl = pQspi->pQspiCtrl;
	vxT_QSPI_NAND * pQspiNand = pQspi->pQspiNand;
		
	int ret = 0;
	UINT32 byteCnt = 0;	
	UINT32 flash_offset = 0;

	/* */
	/* write mode setup etc.*/
	/**/
	qspiNand_Wr_PreReady(pQspi);

	/**/
	/* setup indirect snd */
	/**/
	byteCnt = pQspiNand->pageSize / 4;
	flash_offset = 0;       /* 0 ~ 0.5K area */ 	
	
	qspiCtrl_FastSnd_Setup_x4(pQspi, flash_offset, byteCnt, sndBuf8);

	/**/
	/*  indirect write sram to nand*/
	/**/
	ret = qspiNand_Wr_Data(pQspi, page_idx, sndBuf8, byteCnt);	
	return ret;
}

int qspiNand_Wr_Page_0p5_1k(vxT_QSPI* pQspi, int page_idx, UINT8* sndBuf8)
{
	vxT_QSPI_CTRL * pQspiCtrl = pQspi->pQspiCtrl;
	vxT_QSPI_NAND * pQspiNand = pQspi->pQspiNand;
		
	int ret = 0;
	UINT32 byteCnt = 0;	
	UINT32 flash_offset = 0;

	/* */
	/* write mode setup etc.*/
	/**/
	qspiNand_Wr_PreReady(pQspi);

	/**/
	/* setup indirect snd */
	/**/
	byteCnt = pQspiNand->pageSize / 4;
	flash_offset = 512;       /* 0.5 ~ 1K area */ 	
	
	qspiCtrl_FastSnd_Setup_x4(pQspi, flash_offset, byteCnt, sndBuf8);

	/**/
	/*  indirect write sram to nand*/
	/**/
	ret = qspiNand_Wr_Data(pQspi, page_idx, sndBuf8, byteCnt);	
	return ret;
}

int qspiNand_Wr_Page_1_1p5k(vxT_QSPI* pQspi, int page_idx, UINT8* sndBuf8)
{
	vxT_QSPI_CTRL * pQspiCtrl = pQspi->pQspiCtrl;
	vxT_QSPI_NAND * pQspiNand = pQspi->pQspiNand;
		
	int ret = 0;
	UINT32 byteCnt = 0;	
	UINT32 flash_offset = 0;

	/* */
	/* write mode setup etc.*/
	/**/
	qspiNand_Wr_PreReady(pQspi);

	/**/
	/* setup indirect snd */
	/**/
	byteCnt = pQspiNand->pageSize / 4;
	flash_offset = 1024;       /* 1 ~ 1.5K area */ 	
	
	qspiCtrl_FastSnd_Setup_x4(pQspi, flash_offset, byteCnt, sndBuf8);

	/**/
	/*  indirect write sram to nand*/
	/**/
	ret = qspiNand_Wr_Data(pQspi, page_idx, sndBuf8, byteCnt);	
	return ret;
}

int qspiNand_Wr_Page_1p5_2k(vxT_QSPI* pQspi, int page_idx, UINT8* sndBuf8)
{
	vxT_QSPI_CTRL * pQspiCtrl = pQspi->pQspiCtrl;
	vxT_QSPI_NAND * pQspiNand = pQspi->pQspiNand;
		
	int ret = 0;
	UINT32 byteCnt = 0;	
	UINT32 flash_offset = 0;

	/* */
	/* write mode setup etc.*/
	/**/
	qspiNand_Wr_PreReady(pQspi);

	/**/
	/* setup indirect snd */
	/**/
	byteCnt = pQspiNand->pageSize / 4;
	flash_offset = 1024 + 512;       /* 1.5 ~ 2K area */ 	
	
	qspiCtrl_FastSnd_Setup_x4(pQspi, flash_offset, byteCnt, sndBuf8);

	/**/
	/*  indirect write sram to nand*/
	/**/
	ret = qspiNand_Wr_Data(pQspi, page_idx, sndBuf8, byteCnt);	
	return ret;
}

int qspiNand_Write_Page2(vxT_QSPI* pQspi, int page_idx, UINT8* sndBuf8)
{
	/* part 1: 0 ~ 512*/
	qspiNand_Wr_Page_0_0p5k(pQspi, page_idx, (UINT8*)(&sndBuf8[0]));
	
	/* part 2; 512 ~ 1k	*/
	qspiNand_Wr_Page_0p5_1k(pQspi, page_idx, (UINT8*)(&sndBuf8[512]));
	
	/* part 3: 1k ~ 1.5k*/
	qspiNand_Wr_Page_1_1p5k(pQspi, page_idx, (UINT8*)(&sndBuf8[1024]));
	
	/* part 4: 1.5k ~ 2k*/
	qspiNand_Wr_Page_1p5_2k(pQspi, page_idx, (UINT8*)(&sndBuf8[1024+512]));

	return;
}
#endif


#if 1

/*
// FM25S02A-spi_nand
// -----------------
// 2plane * 1024blocks * 64pages
// 1-bad block, 0-good block
*/
static UINT8 g_BadBlock[2048] = {0};

/*
check first spare area location(800H) for non-FFh data 
on the page_0 and page_1
=> spare area, fisrt_byte == 0xFF ? ok : bad
*/
void qspiNand_Scan_Badblk(vxT_QSPI* pQspi)
{
	UINT8 page1[2048+64] = {0};
	UINT8 page2[2048+64] = {0};
	int total_pages = 0;
	int idx = 0, blk_idx;
	int first_spare = 0;

	/* clear*/
	for (idx=0; idx<2048; idx++)
	{
		g_BadBlock[idx] = 0;
	}

	printf("\n");
	printf("=======scan the nand flash======= \n");
	printf("        FM25S02A-spi_nand         \n");
	printf("==2plane * 1024blocks * 64pages== \n\n");
	
	/**/
	/* FM25S02A-spi_nand*/
	/**/
	/* 2 plane *1024 blocks *64 pages*/
	total_pages = 2 * 1024 * 64;

#if 0	/* read the whole page+spare area*/

	first_spare = 0x800;
	for (idx=0; idx<total_pages; idx+=64)
	{
		qspiNand_Read_Page_Ext(pQspi, idx, &page1[0]);
		qspiNand_Read_Page_Ext(pQspi, idx+1, &page2[0]);

		if ((page1[first_spare] != 0xFF) || (page2[first_spare] != 0xFF))
		{
			printf(" -block_idx:%d is bad[0x%X_0x%X]! \n", (idx/64), page1[first_spare], page2[first_spare]);
			
			blk_idx = idx / 64;
			g_BadBlock[blk_idx] = 1;
		}
	}
#else  /* only read spare area*/

	first_spare = 0x00;
	for (idx=0; idx<total_pages; idx+=64)
	{
		qspiNand_Read_Page_Spare(pQspi, idx, &page1[0]);
		qspiNand_Read_Page_Spare(pQspi, idx+1, &page2[0]);
		
		if ((page1[first_spare] != 0xFF) || (page2[first_spare] != 0xFF))
		{
			printf(" -block_idx:%d is bad[0x%X_0x%X]! \n", (idx/64), page1[first_spare], page2[first_spare]);
			
			blk_idx = idx / 64;
			g_BadBlock[blk_idx] = 1;
		}
	}
#endif	
	
	printf("=========scan end !!!!=========== \n");		
	return;
}
#endif

#endif


#if  1

void printf_log(int start, int end, UINT8* pBuf8)
{
	int j = 0;
	
	for (j=start; j<end; j++)
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

}

UINT8 RcvBuf2[QSPIFLASH_SECTOR_64K] = {0};
UINT8 SndBuf2[QSPIFLASH_SECTOR_64K] = {0};

int g_test_qspi2 = 0;

/*
only page data area: 2048 bytes
*/
void test_nand_rd_page(int page_idx)
{
	vxT_QSPI * pQspi = g_pQspi1;
	
	vxT_QSPI_CTRL * pQspiCtrl = pQspi->pQspiCtrl;
	vxT_QSPI_NAND * pQspiNand = pQspi->pQspiNand;

	UINT8 * pBuf8 = (UINT8 *)(&RcvBuf2[0]);
	
	int j = 0, byteCnt = 0;
	int offset = 0;
	
	vxInit_Qspi_Nand();
	
	byteCnt = pQspiNand->pageSize;
	offset = page_idx * pQspiNand->pageSize;
	
	qspiNand_Read_Page(pQspi, page_idx, pBuf8);

	printf("----pQspi(%d)-read(page-%d):%dKB----\n", pQspiCtrl->ctrl_x, page_idx, (byteCnt/1024));	
	

	printf_log(0, 512, pBuf8);	
	printf(" ... \n");
	
	printf_log(512, 1024, pBuf8);	
	printf(" ... ... \n");

	printf_log(1024, (1024+512), pBuf8);
	printf(" ... ... ... \n");
	
	printf_log((1024+512), 2048, pBuf8);
	printf("\n------------\n");

	return;
}


/*
page data area & spare area: 2048 bytes + 64 bytes
*/
void test_nand_rd_page_ext(int page_idx)
{
	vxT_QSPI * pQspi = g_pQspi1;
	
	vxT_QSPI_CTRL * pQspiCtrl = pQspi->pQspiCtrl;
	vxT_QSPI_NAND * pQspiNand = pQspi->pQspiNand;

	UINT8 * pBuf8 = (UINT8 *)(&RcvBuf2[0]);
	
	int j = 0, byteCnt = 0;
	int offset = 0;
	
	vxInit_Qspi_Nand();
	
	byteCnt = pQspiNand->pageSize + 64;
	offset = page_idx * pQspiNand->pageSize;
	
	qspiNand_Read_Page_Ext(pQspi, page_idx, pBuf8);

	printf("----pQspi(%d)-read(page-%d):%dKB----\n", pQspiCtrl->ctrl_x, page_idx, (byteCnt/1024));	
	
	printf_log(0, 512, pBuf8);	
	printf(" ... \n\n");
	
	printf_log(512, 1024, pBuf8);	
	printf(" ... ... \n\n");

	printf_log(1024, (1024+512), pBuf8);
	printf(" ... ... ... \n\n");
	
	printf_log((1024+512), 2048, pBuf8);
	printf(" ... ... ... ... \n\n");

	/* spare area*/
	printf_log(2048, (2048+64), pBuf8);
	
	printf("\n------------\n");

	return;
}


/*
only page data area: 2048 bytes
*/
void test_nand_wr_page(int page_idx)
{
	vxT_QSPI * pQspi = g_pQspi1;
	
	vxT_QSPI_CTRL * pQspiCtrl = pQspi->pQspiCtrl;
	vxT_QSPI_NAND * pQspiNand = pQspi->pQspiNand;

	UINT8 * pBuf8 = (UINT8 *)(&SndBuf2[0]);
	
	int i = 0, j = 0, byteCnt = 0;
	int offset = 0;
	
	vxInit_Qspi_Nand();
	
	byteCnt = pQspiNand->pageSize;
	offset = page_idx * pQspiNand->pageSize;
	
	for (i=0; i<byteCnt; i++)
	{
		pBuf8[i] = g_test_qspi2 + i;
	}
	g_test_qspi2++;

	/* head*/
	*((UINT32*)(&pBuf8[0])) = 0xcdab3412;  /*0xabcd1234*/
	*((UINT32*)(&pBuf8[4])) = page_idx;
	*((UINT32*)(&pBuf8[8])) = page_idx;
	*((UINT32*)(&pBuf8[0xC])) = page_idx;

	/* tail*/
	*((UINT32*)(&pBuf8[byteCnt-0x10])) = page_idx;
	*((UINT32*)(&pBuf8[byteCnt-0xC])) = page_idx;
	*((UINT32*)(&pBuf8[byteCnt-8])) = page_idx;
	*((UINT32*)(&pBuf8[byteCnt-4])) = 0xefcd8967;	/* 0xcdef6789*/
	
	if (0 != qspiNand_Write_Page(pQspi, page_idx, pBuf8))
	{
		printf("----pQspi(%d)-write(page-%d):%dKB----\n", pQspiCtrl->ctrl_x, page_idx, (byteCnt/1024));	
		printf("qspiNand_Write_Page \n");
		
		printf_log(0, 64, pBuf8);
		printf(" ... ... ... ... \n\n");	

		printf_log((byteCnt-64), byteCnt, pBuf8);		
		printf("------------\n");
	}
	else
	{	
		printf("qspiNand_Write_Page fail:page_idx-(%d)! \n", page_idx);
	}
	
	return;
}

void test_nand_wr_page2(int page_idx)
{
	vxT_QSPI * pQspi = g_pQspi1;
	
	vxT_QSPI_CTRL * pQspiCtrl = pQspi->pQspiCtrl;
	vxT_QSPI_NAND * pQspiNand = pQspi->pQspiNand;

	UINT8 * pBuf8 = (UINT8 *)(&SndBuf2[0]);
	
	int i = 0, j = 0, byteCnt = 0;
	int offset = 0;
	
	vxInit_Qspi_Nand();
	
	byteCnt = pQspiNand->pageSize;
	offset = page_idx * pQspiNand->pageSize;
	
	for (i=0; i<byteCnt; i++)
	{
		pBuf8[i] = g_test_qspi2 + i;
	}
	g_test_qspi2++;

	/* head*/
	*((UINT32*)(&pBuf8[0])) = 0xcdab3412;  /*0xabcd1234*/
	*((UINT32*)(&pBuf8[4])) = page_idx;
	*((UINT32*)(&pBuf8[8])) = page_idx;
	*((UINT32*)(&pBuf8[0xC])) = page_idx;

	/* tail*/
	*((UINT32*)(&pBuf8[byteCnt-0x10])) = page_idx;
	*((UINT32*)(&pBuf8[byteCnt-0xC])) = page_idx;
	*((UINT32*)(&pBuf8[byteCnt-8])) = page_idx;
	*((UINT32*)(&pBuf8[byteCnt-4])) = 0xefcd8967;	/* 0xcdef6789*/
	
	if (0 != qspiNand_Write_Page2(pQspi, page_idx, pBuf8))
	{
		printf("----pQspi(%d)-write(page-%d):%dKB----\n", pQspiCtrl->ctrl_x, page_idx, (byteCnt/1024));	
		printf("qspiNand_Write_Page2 \n");
		
		printf_log(0, 64, pBuf8);
		printf(" ... ... ... ... \n\n");	

		printf_log((byteCnt-64), byteCnt, pBuf8);		
		printf("------------\n");
	}
	else
	{	
		printf("qspiNand_Write_Page2 fail:page_idx-(%d)! \n", page_idx);
	}
	
	return;
}

/*
only spare area: 64 bytes
*/
void test_nand_wr_page_spare(int page_idx)
{
	vxT_QSPI * pQspi = g_pQspi1;
	
	vxT_QSPI_CTRL * pQspiCtrl = pQspi->pQspiCtrl;
	vxT_QSPI_NAND * pQspiNand = pQspi->pQspiNand;

	UINT8 * pBuf8 = (UINT8 *)(&SndBuf2[0]);
	
	int i = 0, j = 0, byteCnt = 0;
	
	vxInit_Qspi_Nand();
	
	byteCnt = pQspiNand->spareSize;
	
	for (i=0; i<byteCnt; i++)
	{
		pBuf8[i] = g_test_qspi2 + i;
	}
	g_test_qspi2++;

	/* head*/
	*((UINT32*)(&pBuf8[0])) = 0xcdab3412;  /*0xabcd1234*/
	*((UINT32*)(&pBuf8[4])) = page_idx;
	*((UINT32*)(&pBuf8[8])) = page_idx;
	*((UINT32*)(&pBuf8[0xC])) = page_idx;

	/* tail*/
	*((UINT32*)(&pBuf8[byteCnt-0x10])) = page_idx;
	*((UINT32*)(&pBuf8[byteCnt-0xC])) = page_idx;
	*((UINT32*)(&pBuf8[byteCnt-8])) = page_idx;
	*((UINT32*)(&pBuf8[byteCnt-4])) = 0xefcd8967;	/* 0xcdef6789*/
	
	if (0 != qspiNand_Write_Spare(pQspi, page_idx, pBuf8))
	{
		printf("----pQspi(%d)-write(page_spare-%d):%d----\n", pQspiCtrl->ctrl_x, page_idx, (byteCnt));	
		
		printf_log(0, 64, pBuf8);	
		printf("------------\n");
	}
	else
	{	
		printf("nand write page_sapre fail:page_idx-(%d)! \n", page_idx);
	}
	
	return;
}



void test_nand_wr_page_ecc(int page_idx)
{
	vxT_QSPI * pQspi = g_pQspi1;
	
	vxT_QSPI_CTRL * pQspiCtrl = pQspi->pQspiCtrl;
	vxT_QSPI_NAND * pQspiNand = pQspi->pQspiNand;

	UINT8 * pBuf8 = (UINT8 *)(&SndBuf2[0]);
	
	int i = 0, j = 0, byteCnt = 0;
	int offset = 0;

	/**/
	/* init */
	/**/
	vxInit_Qspi_Nand();

	
	byteCnt = pQspiNand->pageSize;
	offset = page_idx * pQspiNand->pageSize;
	
	for (i=0; i<byteCnt; i++)
	{
		pBuf8[i] = g_test_qspi2 + i;
	}
	g_test_qspi2++;

	/* head*/
	*((UINT32*)(&pBuf8[0])) = 0xcdab3412;  /*0xabcd1234*/
	*((UINT32*)(&pBuf8[4])) = page_idx;
	*((UINT32*)(&pBuf8[8])) = page_idx;
	*((UINT32*)(&pBuf8[0xC])) = page_idx;

	/* tail*/
	*((UINT32*)(&pBuf8[byteCnt-0x10])) = page_idx;
	*((UINT32*)(&pBuf8[byteCnt-0xC])) = page_idx;
	*((UINT32*)(&pBuf8[byteCnt-8])) = page_idx;
	*((UINT32*)(&pBuf8[byteCnt-4])) = 0xefcd8967;	/* 0xcdef6789*/
	
	/* */
	/* enable ecc*/
	/**/
	qspiNand_Enable_ECC(pQspi, 1);
	
	/* */
	/* write one page*/
	/**/
	if (0 != qspiNand_Write_Page(pQspi, page_idx, pBuf8))
	{
		printf("----pQspi(%d)-write(page-%d):%dKB----\n", pQspiCtrl->ctrl_x, page_idx, (byteCnt/1024));	
		
		for (j=0; j<64; j++)
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
		
		for (j=(byteCnt-64); j<byteCnt; j++)
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
	}
	else
	{	
		printf("nand write page fail:page_idx-(%d)! \n", page_idx);
	}
	
	return;
}

/*
blk_idx: 0, 1, 2, 3, ... ... 2047 
page_idx: 0, 1, 2, 3, ... ...
---------
page_idx -> blk_idx
    0    ->    0
	64	 ->    1
	128	 ->    2
	...
*/
void test_nand_erase_blk(int blk_idx)
{
	vxT_QSPI * pQspi = g_pQspi1;
	int ret = 0;
	
	vxInit_Qspi_Nand();
	
	ret = qspiNand_Erase_Blk(pQspi, blk_idx);
    if (ret == FMSH_FAILURE)
    {
    	printf("ctrl_%d: qspiflash erase sect: %d fail! \n", pQspi->qspi_x, blk_idx);
    }
	else
	{
		printf("ctrl_%d: qspiflash erase sect: %d ok! \n", pQspi->qspi_x, blk_idx);
	}
	
	return;
}


void test_nand_wr_page_p1(int page_idx)
{
	vxT_QSPI * pQspi = g_pQspi1;
	
	vxT_QSPI_CTRL * pQspiCtrl = pQspi->pQspiCtrl;
	vxT_QSPI_NAND * pQspiNand = pQspi->pQspiNand;

	UINT8 * pBuf8 = (UINT8 *)(&SndBuf2[0]);
	
	int i = 0, j = 0, byteCnt = 0;
	
	vxInit_Qspi_Nand();
	
	byteCnt = pQspiNand->pageSize / 4;
	
	for (i=0; i<byteCnt; i++)
	{
		pBuf8[i] = g_test_qspi2 + i;
	}
	g_test_qspi2++;

	/* head*/
	*((UINT32*)(&pBuf8[0])) = 0xcdab3412;  /*0xabcd1234*/
	*((UINT32*)(&pBuf8[4])) = 0x11111111;
	*((UINT32*)(&pBuf8[8])) = page_idx;
	*((UINT32*)(&pBuf8[0xC])) = page_idx;
	

	/* tail*/
	*((UINT32*)(&pBuf8[byteCnt-0x10])) = page_idx;
	*((UINT32*)(&pBuf8[byteCnt-0xC])) = page_idx;
	*((UINT32*)(&pBuf8[byteCnt-8])) = 0x11111111;
	*((UINT32*)(&pBuf8[byteCnt-4])) = 0xefcd8967;	/* 0xcdef6789*/
	
	if (0 != qspiNand_Wr_Page_0_0p5k(pQspi, page_idx, pBuf8))
	{
		printf("----pQspi(%d)-write(page-%d):%dB----\n", pQspiCtrl->ctrl_x, page_idx, (byteCnt));	
		printf("qspiNand_Wr_Page_0_0p5k \n");
		
		printf_log(0, 64, pBuf8);
		printf(" ... ... ... ... \n\n");
		
		printf_log((byteCnt-64), byteCnt, pBuf8);		
		printf("------------\n");
	}
	else
	{	
		printf("qspiNand_Wr_Page_0_0p5k fail:page_idx-(%d)! \n", page_idx);
	}
	
	return;
}

void test_nand_wr_page_p2(int page_idx)
{
	vxT_QSPI * pQspi = g_pQspi1;
	
	vxT_QSPI_CTRL * pQspiCtrl = pQspi->pQspiCtrl;
	vxT_QSPI_NAND * pQspiNand = pQspi->pQspiNand;

	UINT8 * pBuf8 = (UINT8 *)(&SndBuf2[0]);
	
	int i = 0, j = 0, byteCnt = 0;
	
	vxInit_Qspi_Nand();
	
	byteCnt = pQspiNand->pageSize / 4;
	
	for (i=0; i<byteCnt; i++)
	{
		pBuf8[i] = g_test_qspi2 + i;
	}
	g_test_qspi2++;

	/* head*/
	*((UINT32*)(&pBuf8[0])) = 0xcdab3412;  /*0xabcd1234*/
	*((UINT32*)(&pBuf8[4])) = 0x22222222;
	*((UINT32*)(&pBuf8[8])) = page_idx;
	*((UINT32*)(&pBuf8[0xC])) = page_idx;

	/* tail*/
	*((UINT32*)(&pBuf8[byteCnt-0x10])) = page_idx;
	*((UINT32*)(&pBuf8[byteCnt-0xC])) = page_idx;
	*((UINT32*)(&pBuf8[byteCnt-8])) = 0x22222222;
	*((UINT32*)(&pBuf8[byteCnt-4])) = 0xefcd8967;	/* 0xcdef6789*/
	
	if (0 != qspiNand_Wr_Page_0p5_1k(pQspi, page_idx, pBuf8))
	{
		printf("----pQspi(%d)-write(page-%d):%dB----\n", pQspiCtrl->ctrl_x, page_idx, (byteCnt));	
		printf("qspiNand_Wr_Page_0p5_1k \n");
		
		printf_log(0, 64, pBuf8);
		printf(" ... ... ... ... \n\n");
		
		printf_log((byteCnt-64), byteCnt, pBuf8);		
		printf("------------\n");
	}
	else
	{	
		printf("qspiNand_Wr_Page_0_0p5k fail:page_idx-(%d)! \n", page_idx);
	}
	
	return;
}

void test_nand_wr_page_p3(int page_idx)
{
	vxT_QSPI * pQspi = g_pQspi1;
	
	vxT_QSPI_CTRL * pQspiCtrl = pQspi->pQspiCtrl;
	vxT_QSPI_NAND * pQspiNand = pQspi->pQspiNand;

	UINT8 * pBuf8 = (UINT8 *)(&SndBuf2[0]);
	
	int i = 0, j = 0, byteCnt = 0;
	
	vxInit_Qspi_Nand();
	
	byteCnt = pQspiNand->pageSize / 4;
	
	for (i=0; i<byteCnt; i++)
	{
		pBuf8[i] = g_test_qspi2 + i;
	}
	g_test_qspi2++;

	/* head*/
	*((UINT32*)(&pBuf8[0])) = 0xcdab3412;  /*0xabcd1234*/
	*((UINT32*)(&pBuf8[4])) = 0x33333333;
	*((UINT32*)(&pBuf8[8])) = page_idx;
	*((UINT32*)(&pBuf8[0xC])) = page_idx;

	/* tail*/
	*((UINT32*)(&pBuf8[byteCnt-0x10])) = page_idx;
	*((UINT32*)(&pBuf8[byteCnt-0xC])) = page_idx;
	*((UINT32*)(&pBuf8[byteCnt-8])) = 0x33333333;
	*((UINT32*)(&pBuf8[byteCnt-4])) = 0xefcd8967;	/* 0xcdef6789*/
	
	if (0 != qspiNand_Wr_Page_1_1p5k(pQspi, page_idx, pBuf8))
	{
		printf("----pQspi(%d)-write(page-%d):%dB----\n", pQspiCtrl->ctrl_x, page_idx, (byteCnt));	
		printf("qspiNand_Wr_Page_1p5_2k(1_1p5k) \n");
		
		printf_log(0, 64, pBuf8);
		printf(" ... ... ... ... \n\n");
		
		printf_log((byteCnt-64), byteCnt, pBuf8);		
		printf("------------\n");
	}
	else
	{	
		printf("qspiNand_Wr_Page_0_0p5k fail:page_idx-(%d)! \n", page_idx);
	}
	
	return;
}

void test_nand_wr_page_p4(int page_idx)
{
	vxT_QSPI * pQspi = g_pQspi1;
	
	vxT_QSPI_CTRL * pQspiCtrl = pQspi->pQspiCtrl;
	vxT_QSPI_NAND * pQspiNand = pQspi->pQspiNand;

	UINT8 * pBuf8 = (UINT8 *)(&SndBuf2[0]);
	
	int i = 0, j = 0, byteCnt = 0;
	
	vxInit_Qspi_Nand();
	
	byteCnt = pQspiNand->pageSize / 4;
	
	for (i=0; i<byteCnt; i++)
	{
		pBuf8[i] = g_test_qspi2 + i;
	}
	g_test_qspi2++;

	/* head*/
	*((UINT32*)(&pBuf8[0])) = 0xcdab3412;  /*0xabcd1234*/
	*((UINT32*)(&pBuf8[4])) = 0x44444444;
	*((UINT32*)(&pBuf8[8])) = page_idx;
	*((UINT32*)(&pBuf8[0xC])) = page_idx;

	/* tail*/
	*((UINT32*)(&pBuf8[byteCnt-0x10])) = page_idx;
	*((UINT32*)(&pBuf8[byteCnt-0xC])) = page_idx;
	*((UINT32*)(&pBuf8[byteCnt-8])) = 0x44444444;
	*((UINT32*)(&pBuf8[byteCnt-4])) = 0xefcd8967;	/* 0xcdef6789*/
	
	if (0 != qspiNand_Wr_Page_1p5_2k(pQspi, page_idx, pBuf8))
	{
		printf("----pQspi(%d)-write(page-%d):%dB----\n", pQspiCtrl->ctrl_x, page_idx, (byteCnt));	
		printf("qspiNand_Wr_Page_1p5_2k \n");
			
		printf_log(0, 64, pBuf8);
		printf(" ... ... ... ... \n\n");
		
		printf_log((byteCnt-64), byteCnt, pBuf8);		
		printf("------------\n");
	}
	else
	{	
		printf("qspiNand_Wr_Page_1p5_2k fail:page_idx-(%d)! \n", page_idx);
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

void test_read_all_status(void)
{
	qspiCtrl_Rd_ALL_Status2(g_pQspi1);
	return;
}

void test_read_nand_id(void)
{
	printf("Spi_Nand(FM25S02A<0xe5a1>): 0x%X \n", qspiNand_ReadId_Stig(g_pQspi1));
	return;
}

void test_scan_nand(void)
{
	vxInit_Qspi_Nand();

	qspiNand_Scan_Badblk(g_pQspi1);
	return;
}

#endif

#if 1

#define __tffs_norflash__

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
#if 1	
	UINT32 tmp32 = slcr_read(SLCR_QSPI0_CTRL);
	/* set 1*/
	tmp32 |= (QSPI_APB_RST | QSPI_AHB_RST | QSPI_REF_RST);
	slcr_write(SLCR_QSPI0_CTRL, tmp32);
	
	/* set 0*/
	tmp32 &= ~(QSPI_APB_RST | QSPI_AHB_RST | QSPI_REF_RST);
	slcr_write(SLCR_QSPI0_CTRL, tmp32);
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
	return;
}

#endif


/*
test the all pages & block of nand flash
*/
#if 1
void testvx_nand_erase_blk2(int blk_idx)
{
	vxT_QSPI * pQspi = g_pQspi1;
	int ret = 0;
	
	ret = qspiNand_Erase_Blk(pQspi, blk_idx);
    if (ret == FMSH_FAILURE)
    {
    	printf("--ctrl_%d: qspiNand erase blk: %d fail!-- \n", pQspi->qspi_x, blk_idx);
    }
	else
	{
		printf("--ctrl_%d: qspiNand erase blk: %d ok!-- \n", pQspi->qspi_x, blk_idx);
	}
	
	return;
}

void testvx_nand_rd_page2(int page_idx)
{
	vxT_QSPI * pQspi = g_pQspi1;
	
	vxT_QSPI_CTRL * pQspiCtrl = pQspi->pQspiCtrl;
	vxT_QSPI_NAND * pQspiNand = pQspi->pQspiNand;

	int ret = 0;
	int j = 0, byteCnt = 0;
	UINT8 * pBuf8 = (UINT8 *)(&RcvBuf2[0]);		

	for (j=0; j<QSPIFLASH_SECTOR_64K; j++)
	{
		RcvBuf2[j] = 0x0;
	}

	byteCnt = pQspiNand->pageSize;	
	ret = qspiNand_Read_Page(pQspi, page_idx, pBuf8);
	if (ret == 0)
	{
		printf("--qpsi read page(%d) fail!-- \n", page_idx);
	}
	else
	{
		printf("qpsi read page(%d) ok! \n", page_idx);
	}

	return;
}

void testvx_nand_wr_page2(int page_idx)
{
	vxT_QSPI * pQspi = g_pQspi1;
	
	vxT_QSPI_CTRL * pQspiCtrl = pQspi->pQspiCtrl;
	vxT_QSPI_NAND * pQspiNand = pQspi->pQspiNand;

	int ret = 0;
	int i = 0, j = 0, byteCnt = 0;
	UINT8 * pBuf8 = (UINT8 *)(&SndBuf2[0]);	
	
	byteCnt = pQspiNand->pageSize;
	
	for (i=0; i<byteCnt; i++)
	{
		pBuf8[i] = g_test_qspi2 + i;
	}
	g_test_qspi2++;
	
	ret = qspiNand_Write_Page(pQspi, page_idx, pBuf8);
	if (ret == 0)
	{
		printf("--qpsi write page(%d) fail!-- \n", page_idx);
	}
	else
	{
		printf("qpsi write page(%d) ok! \n", page_idx);
	}
	
	return;
}

#define MODE_ONLY_ERASE_RD   (0)  /* 1-erase&read&compare, 0-erase&write&read&compare*/

void testvx_nand_rdwr_pages(int from_page, int to_page)
{
	vxT_QSPI * pQspi = g_pQspi1;
	
	int idx = 0, j = 0;
	int sect_diff = 0;
	int blk_idx = 0;

	/**/
	/* init*/
	/**/
	vxInit_Qspi_Nand();

	/**/
	/* scan bad block*/
	/**/
	qspiNand_Scan_Badblk(pQspi);

	for (idx = from_page; idx <= to_page; idx++)
	{	
		blk_idx = idx / 64;
		if (g_BadBlock[blk_idx] == 1)
		{
			idx += 64; /* jump the next good block*/
			continue;
		}

		if ((idx % 64) == 0)  /* 64 pages/block*/
		{
			blk_idx = idx / 64;
			testvx_nand_erase_blk2(blk_idx);
			taskDelay(10);
		}

	#if MODE_ONLY_ERASE_RD
		/* do nothing*/
		taskDelay(1);
	#else
		testvx_nand_wr_page2(idx);
		taskDelay(10);
	#endif
		
		testvx_nand_rd_page2(idx);
		taskDelay(10);

		sect_diff = 0;
		for (j=0; j<pQspi->pQspiNand->pageSize; j++)
		{
		#if MODE_ONLY_ERASE_RD
			if (RcvBuf2[j] != 0xFF)
		#else
			if (RcvBuf2[j] != SndBuf2[j])
		#endif
			{
				sect_diff = 1;
				break;
			}
		}

		if (sect_diff == 1)
		{
			printf("------page(%d) erase & wr & rd err:[%d][0x%X-0x%X]!------- \n\n", \
				    idx, j, RcvBuf2[j], SndBuf2[j]);
		}
		else
		{
		
		#if MODE_ONLY_ERASE_RD
			printf("==page(%d) erase & ... & rd & compare OK!== \n\n", idx);  /* only erase & read*/
		#else
			printf("==page(%d) erase & wr & rd & compare OK!== \n\n", idx);
		#endif
		}
	}

	printf("\n\n testvx nand pages:[%d -> %d] finish! \n\n\n", from_page, to_page);
	
	return;
}


#endif


