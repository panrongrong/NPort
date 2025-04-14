/* vxCan.c - fmsh 7020/7045 pCan driver */

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
01a, 24Dec19, jc  written.
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

#include "vxCan.h"

/*
defines 
*/
#if 1

#undef VX_DBG
#undef VX_DBG2

#define CAN_DBG

#ifdef CAN_DBG

#define VX_DBG(string, a, b, c, d, e, f)                    \
        if (_func_logMsg != NULL)                           \
           printf(string, a, b, c, d, e, f)  /* (* _func_logMsg)(string, a, b, c, d, e, f) */
#else
#define VX_DBG(string, a, b, c, d, e, f)
#endif

/* error info log */
#define VX_DBG2(string, a, b, c, d, e, f) printf(string, a, b, c, d, e, f)
#endif



/* 
can_0 : master
*/
static vxT_CAN_CTRL vxCan_Ctrl_0 = {0};

static vxT_CAN vx_pCan_0 = {0};
vxT_CAN * g_pCan0 = (vxT_CAN *)(&vx_pCan_0.can_x);

/* 
can_1 : slave
*/
static vxT_CAN_CTRL vxCan_Ctrl_1 = {0};

static vxT_CAN vx_pCan_1 = {0};
vxT_CAN * g_pCan1 = (vxT_CAN *)(&vx_pCan_1.can_x);


#if 1

UINT32 do_div2(UINT32 n, UINT32 base) 
{					
	UINT32 __base = (base);				
	UINT32 __rem;	
	
	__rem = ((uint64_t)(n)) % __base;			
	(n) = ((uint64_t)(n)) / __base;				
	
    return __rem;							
}

void canCtrl_Wr_CfgReg32(vxT_CAN* pCan, UINT32 offset, UINT32 value)
{
	UINT32 tmp32 = pCan->pCanCtrl->cfgBaseAddr;
	FMQL_WRITE_32((tmp32 + offset), value);
	return;
}

UINT32 canCtrl_Rd_CfgReg32(vxT_CAN* pCan, UINT32 offset)
{
	UINT32 tmp32 = pCan->pCanCtrl->cfgBaseAddr;
	return FMQL_READ_32(tmp32 + offset);
}

int canCtrl_Update_SamplePoint(vxT_CAN* pCan,   			  UINT32 sample_point_nominal, 
                                            UINT32 tseg, UINT32 *tseg1_ptr, UINT32 *tseg2_ptr, UINT32 *pSamplePointErr)
{
	UINT32 sample_point_error, best_SamplePointErr = UINT_MAX;
	UINT32 sample_point, best_sample_point = 0;
	UINT32 tseg1, tseg2;
	
	int i;

	for (i = 0; i <= 1; i++) 
    {
		tseg2 = tseg + CAN_CALC_SYNC_SEG - (sample_point_nominal * (tseg + CAN_CALC_SYNC_SEG)) / 1000 - i;
		tseg2 = clamp(tseg2, pCan->pCanCtrl->BUS_TIME_C.tseg2_min, pCan->pCanCtrl->BUS_TIME_C.tseg2_max);
		tseg1 = tseg - tseg2;
		if (tseg1 > pCan->pCanCtrl->BUS_TIME_C.tseg1_max) 
        {
			tseg1 = pCan->pCanCtrl->BUS_TIME_C.tseg1_max;
			tseg2 = tseg - tseg1;
		}

		sample_point = 1000 * (tseg + CAN_CALC_SYNC_SEG - tseg2) / (tseg + CAN_CALC_SYNC_SEG);
		sample_point_error = abs(sample_point_nominal - sample_point);

		if ((sample_point <= sample_point_nominal) && (sample_point_error < best_SamplePointErr)) 
        {
			best_sample_point = sample_point;
			best_SamplePointErr = sample_point_error;
			*tseg1_ptr = tseg1;
			*tseg2_ptr = tseg2;
		}
	}

	if (pSamplePointErr)
		*pSamplePointErr = best_SamplePointErr;

	return best_sample_point;
}

int canCtrl_Calc_BusTiming(vxT_CAN* pCan)
{
    UINT32 bitrate;			                    /* current bitrate */
	UINT32 bitrate_error;		                /* difference between current and nominal value */
	UINT32 best_bitrate_error = UINT_MAX;
	
	UINT32 sample_point_error;	                /* difference between current and nominal value */
	UINT32 best_SamplePointErr = UINT_MAX;
	UINT32 sample_point_nominal;	            /* nominal sample point */
	
	UINT32 best_tseg = 0;		                /* current best value for tseg */
	UINT32 best_brp = 0;		                /* current best value for brp */
	
	UINT32 brp, tsegall, tseg, tseg1 = 0, tseg2 = 0;
	UINT64 v64;
    
    struct can_bittiming *bt;
    const struct can_bittiming_const *btc;
        
   	/* 
   	Use CiA recommended sample points 
	*/
	if (pCan->pCanCtrl->BUS_TIME.sample_point)
    {
		sample_point_nominal = pCan->pCanCtrl->BUS_TIME.sample_point;
	} 
    else 
    {
		if (pCan->pCanCtrl->BUS_TIME.bitrate > 800000)
			sample_point_nominal = 750;
		else if (pCan->pCanCtrl->BUS_TIME.bitrate > 500000)
			sample_point_nominal = 800;
		else
			sample_point_nominal = 875;
	}

	/* tseg even = round down, odd = round up */
	for (tseg = (pCan->pCanCtrl->BUS_TIME_C.tseg1_max + pCan->pCanCtrl->BUS_TIME_C.tseg2_max) * 2 + 1;
	     tseg >= (pCan->pCanCtrl->BUS_TIME_C.tseg1_min + pCan->pCanCtrl->BUS_TIME_C.tseg2_min) * 2; tseg--) 
    {
		tsegall = CAN_CALC_SYNC_SEG + tseg / 2;

		/* Compute all possible tseg choices (tseg=tseg1+tseg2) */
		brp = pCan->pCanCtrl->sysClk / (tsegall * pCan->pCanCtrl->BUS_TIME.bitrate) + tseg % 2;

		/* choose brp step which is possible in system */
		brp = (brp / pCan->pCanCtrl->BUS_TIME_C.brp_inc) * pCan->pCanCtrl->BUS_TIME_C.brp_inc;
		if ((brp < pCan->pCanCtrl->BUS_TIME_C.brp_min) || (brp > pCan->pCanCtrl->BUS_TIME_C.brp_max))
			continue;

		bitrate = pCan->pCanCtrl->sysClk / (brp * tsegall);
		bitrate_error = abs(pCan->pCanCtrl->BUS_TIME.bitrate - bitrate);

		/* 
		tseg brp biterror 
		*/
		if (bitrate_error > best_bitrate_error)
			continue;

		/* 
		reset sample point error if we have a better bitrate 
		*/
		if (bitrate_error < best_bitrate_error)
			best_SamplePointErr = UINT_MAX;

		canCtrl_Update_SamplePoint(pCan, sample_point_nominal, tseg / 2, &tseg1, &tseg2, &sample_point_error);
		if (sample_point_error > best_SamplePointErr)
			continue;

		best_SamplePointErr = sample_point_error;
		best_bitrate_error = bitrate_error;
		best_tseg = tseg / 2;
		best_brp = brp;

		if (bitrate_error == 0 && sample_point_error == 0)
			break;
	}
    
	/* real sample point */
	pCan->pCanCtrl->BUS_TIME.sample_point = canCtrl_Update_SamplePoint(pCan, sample_point_nominal, best_tseg,
					  &tseg1, &tseg2, NULL);

	v64 = (u64)best_brp * 1000 * 1000 * 1000;
	
	do_div2(v64, pCan->pCanCtrl->sysClk);
	
	pCan->pCanCtrl->BUS_TIME.tq = (UINT32)v64;
	pCan->pCanCtrl->BUS_TIME.prop_seg = tseg1 / 2;
	pCan->pCanCtrl->BUS_TIME.phase_seg1 = tseg1 - pCan->pCanCtrl->BUS_TIME.prop_seg;
	pCan->pCanCtrl->BUS_TIME.phase_seg2 = tseg2;

	/* 
	check for sjw user settings 
	*/
	if (!pCan->pCanCtrl->BUS_TIME.sjw || !pCan->pCanCtrl->BUS_TIME_C.sjw_max)
    {
		pCan->pCanCtrl->BUS_TIME.sjw = 1;
	}
    else
    {
		/* 
		pCan->pCanCtrl->BUS_TIME.sjw is at least 1 -> sanitize upper bound to sjw_max 
		*/
		if (pCan->pCanCtrl->BUS_TIME.sjw > pCan->pCanCtrl->BUS_TIME_C.sjw_max)
			pCan->pCanCtrl->BUS_TIME.sjw = pCan->pCanCtrl->BUS_TIME_C.sjw_max;
		
		/* 
		pCan->pCanCtrl->BUS_TIME.sjw must not be higher than tseg2 
		*/
		if (tseg2 < pCan->pCanCtrl->BUS_TIME.sjw)
			pCan->pCanCtrl->BUS_TIME.sjw = tseg2;
	}

	pCan->pCanCtrl->BUS_TIME.brp = best_brp;

	/* 
	real bitrate 
	*/
	pCan->pCanCtrl->BUS_TIME.bitrate = pCan->pCanCtrl->sysClk / (pCan->pCanCtrl->BUS_TIME.brp * (CAN_CALC_SYNC_SEG + tseg1 + tseg2));

	return OK;
    
}

int canCtrl_Set_RunMode(vxT_CAN* pCan, int mode_state)
{
	UINT32 tmp32 = 0;

	tmp32 = canCtrl_Rd_CfgReg32(pCan, CAN_REG_MODE);
	if (mode_state == MODE_RST)
	{
		tmp32 |= MODE_RST;   /* bit0 -> 1 */
	}
	else
	{
		tmp32 &= ~MODE_RST;  /* bit0 -> 0 */
	}
	
	canCtrl_Wr_CfgReg32(pCan, CAN_REG_MODE, tmp32);
	
    return OK;
}

int canCtrl_Set_BusTiming(vxT_CAN* pCan)
{
    UINT32 btr0 = 0, btr1 = 0;
    UINT32 tmp32 = 0;
    
   	btr0 = ((pCan->pCanCtrl->BUS_TIME.brp - 1) & 0x3f) | (((pCan->pCanCtrl->BUS_TIME.sjw - 1) & 0x3) << 6);
	btr1 = ((pCan->pCanCtrl->BUS_TIME.prop_seg + pCan->pCanCtrl->BUS_TIME.phase_seg1 - 1) & 0xf) |
		    (((pCan->pCanCtrl->BUS_TIME.phase_seg2 - 1) & 0x7) << 4); 

	tmp32 = canCtrl_Rd_CfgReg32(pCan, CAN_REG_BTR0);
	if (tmp32 != btr0)
	{
		canCtrl_Wr_CfgReg32(pCan, CAN_REG_BTR0, btr0);
	}
	
	tmp32 = canCtrl_Rd_CfgReg32(pCan, CAN_REG_BTR1);
	if (tmp32 != btr1)
	{
		canCtrl_Wr_CfgReg32(pCan, CAN_REG_BTR1, btr1);
	}
	
    return OK;
}


int canCtrl_Set_BaudRate(vxT_CAN* pCan, int baud)
{
    pCan->pCanCtrl->BUS_TIME.bitrate = baud;

    canCtrl_Calc_BusTiming(pCan);

    /* FCanPs_setResetMode(pCan, CAN_set); */ /*enter reset mode*/
	canCtrl_Set_RunMode(pCan, MODE_RST);
	
    canCtrl_Set_BusTiming(pCan);
	
    /* FCanPs_setResetMode(pCan, CAN_clear); */ /*exit reset mode*/
	canCtrl_Set_RunMode(pCan, MODE_RUN);
    
    return OK;
}

/*****************************************************************************
*
* @description
* This function set standard frame Acceptance Code Registers
*
* @param    dev is a pointer to the instance of can device.
* @param    id:11 bits
* @param    ptr: 0/1 0 -- remote frame, 1 -- data frame
* @param    byte1: 
* @param    byte2:
* @param    mode: 
*                 single_filter
*                 dual_filter
*
* @return   0 if successful, otherwise 1.
*
* @note     Only read/write in reset mode.
*
******************************************************************************/
int canCtrl_Set_StdACR(vxT_CAN* pCan, UINT32 id, UINT8 ptr, UINT8 byte1, UINT8 byte2, int filter_mode)
{
    if (ptr > 0x1)
        return ERROR;

    if (filter_mode == SINGLE_FILTER)
    {
		canCtrl_Set_RunMode(pCan, MODE_RST);	
		
		canCtrl_Wr_CfgReg32(pCan, CAN_REG_ACR0_TXRXBUF, (id >> 3));		
		canCtrl_Wr_CfgReg32(pCan, CAN_REG_ACR1_TXRXBUF, (((id & 0x07) << 5)|(ptr << 4)));		
		canCtrl_Wr_CfgReg32(pCan, CAN_REG_ACR2_TXRXBUF, byte1);
		canCtrl_Wr_CfgReg32(pCan, CAN_REG_ACR3_TXRXBUF, byte2);		
		
		canCtrl_Set_RunMode(pCan, MODE_RUN);
    }
    else if (filter_mode == DUAL_FILTER) 
    {
		canCtrl_Set_RunMode(pCan, MODE_RST);
		
		canCtrl_Wr_CfgReg32(pCan, CAN_REG_ACR0_TXRXBUF, (id >> 3));
		canCtrl_Wr_CfgReg32(pCan, CAN_REG_ACR1_TXRXBUF, (((id & 0x07) << 5)|(ptr << 4)|(byte1 >> 4)));
		canCtrl_Wr_CfgReg32(pCan, CAN_REG_ACR2_TXRXBUF, (id >> 3));
		canCtrl_Wr_CfgReg32(pCan, CAN_REG_ACR3_TXRXBUF, ((id & 0x07) << 5)|(ptr << 4)|(byte1 & 0x0f));
		
		canCtrl_Set_RunMode(pCan, MODE_RUN);		
    }
    else
    {    
    	return ERROR;
    }

    return OK;   
}

/*****************************************************************************
*
* @description
* This function set Acceptance Mask Registers
*
* @param    dev is a pointer to the instance of can device.
* @param    AMR0~3 
*           0��s in AMR0 �C 3 identify the bits at the corresponding positions in ACR0 �C 3 
*           which must be matched in the message identifier, ��1��s identify the corresponding bits
*           as ��don��t care��.
*
* @return   0 if successful, otherwise 1.
*
* @note     Only read/write in reset mode.
*
******************************************************************************/
int canCtrl_Set_AMR(vxT_CAN* pCan, UINT32 AMR0, UINT32 AMR1, UINT32 AMR2, UINT32 AMR3)
{
	canCtrl_Set_RunMode(pCan, MODE_RST);
	
	canCtrl_Wr_CfgReg32(pCan, CAN_REG_AMR0_TXRXBUF, AMR0);
	canCtrl_Wr_CfgReg32(pCan, CAN_REG_AMR1_TXRXBUF, AMR1);
	canCtrl_Wr_CfgReg32(pCan, CAN_REG_AMR2_TXRXBUF, AMR2);
	canCtrl_Wr_CfgReg32(pCan, CAN_REG_AMR3_TXRXBUF, AMR3);
	
	canCtrl_Set_RunMode(pCan, MODE_RUN);		
    return OK;
}

/*****************************************************************************
*
* @description
* This function returns the status of Transmit Buffer Status.
*
* @param    dev is a pointer to the instance of can device.
*
* @return   status: 0/1
*
* @note     None.
*
******************************************************************************/
UINT32 canCtrl_Get_TxBufStatus(vxT_CAN* pCan)
{
    UINT32 tmp32 = 0;
	
	/*
	SR.2	TBS	发送缓存区状态	
			1	发送缓存器释放，CPU可以向发送缓存区写消息。
			0	发送缓存区锁定，CPU无法访问发送缓存区，因为消息正在等待传输或正在传输。
	*/
	tmp32 = canCtrl_Rd_CfgReg32(pCan, CAN_REG_SR) & 0x04;

    return tmp32;
}

/*****************************************************************************
*
* @description
* This function sends standard frame format message
*
* @param    dev is a pointer to the instance of can device.
* @param    id is 11 bits
* @param    sbuf is buffer of message data
* @param    len is length of message
* @param    state is a enum frame_type:data_frame or remote_frame
*
* @return   0 if successful, otherwise 1.
*
* @note     None.
*
******************************************************************************/
int canCtrl_Setup_StdFrmTx(vxT_CAN* pCan, UINT32 id, UINT8* pBbuf, int len, int frm_type)
{
    int timeout = 10000;
	int i = 0;

    while ((canCtrl_Get_TxBufStatus(pCan) == 0) && (timeout--));  /* wait tx buffer empty */

	canCtrl_Wr_CfgReg32(pCan, CAN_REG_ACR0_TXRXBUF, ((frm_type << 6) | len));
	canCtrl_Wr_CfgReg32(pCan, CAN_REG_ACR1_TXRXBUF, (((id << 5) & 0xFF00) >> 8));
	canCtrl_Wr_CfgReg32(pCan, CAN_REG_ACR2_TXRXBUF, (((id << 5) | (frm_type << 4)) & 0x00FF));
	
    /* data frame */
    if (frm_type == DATA_FRM) 
    {
		i = 0;
		canCtrl_Wr_CfgReg32(pCan, CAN_REG_ACR3_TXRXBUF, pBbuf[i++]);
		
		canCtrl_Wr_CfgReg32(pCan, CAN_REG_AMR0_TXRXBUF, pBbuf[i++]);
		canCtrl_Wr_CfgReg32(pCan, CAN_REG_AMR1_TXRXBUF, pBbuf[i++]);
		canCtrl_Wr_CfgReg32(pCan, CAN_REG_AMR2_TXRXBUF, pBbuf[i++]);
		canCtrl_Wr_CfgReg32(pCan, CAN_REG_AMR3_TXRXBUF, pBbuf[i++]);
		
		canCtrl_Wr_CfgReg32(pCan, (CAN_REG_TXRXBUF+0x04), pBbuf[i++]);
		canCtrl_Wr_CfgReg32(pCan, (CAN_REG_TXRXBUF+0x08), pBbuf[i++]);
		canCtrl_Wr_CfgReg32(pCan, (CAN_REG_TXRXBUF+0x0C), pBbuf[i++]);		
    }
    
    return OK;
}


/*****************************************************************************
*
* @description
* This function set Transmission Request
*
* @param    dev is a pointer to the instance of can device.
*
* @return   0 if successful, otherwise 1.
*
* @note     None.
*
******************************************************************************/
int canCtrl_Set_TxRequest(vxT_CAN* pCan)
{
    UINT32 tmp32 = 0;
    
	/*
	CMR.0	TR	传输请求	1：发送消息。
	*/
	tmp32 = canCtrl_Rd_CfgReg32(pCan, CAN_REG_CMR) | 0x01;
	canCtrl_Wr_CfgReg32(pCan, CAN_REG_CMR, tmp32); 

    return OK;
}

/*****************************************************************************
*
* @description
* This function returns the status of Transmission Complete Status.
*
* @param    dev is a pointer to the instance of can device.
*
* @return   status: 0/1
*
* @note     None.
*
******************************************************************************/
UINT32 canCtrl_Get_TxOkStatus(vxT_CAN* pCan)
{
    UINT32 tmp32 = 0;
	
	/*
	SR.3	TCS 传输完成状态	
			1	最后请求的传输已成功完成。
			0	最后请求的传输尚未完成。
	*/
	tmp32 = canCtrl_Rd_CfgReg32(pCan, CAN_REG_SR) & 0x08;

    return tmp32;
}


/*****************************************************************************
*
* @description
* This function receives message, read the receive buffer data.
*
* @param    dev is a pointer to the instance of can device.
* @param    rbuf is buffer of message data to readback into
*
* @return   0 if successful, otherwise 1.
*
* @note     None.
*
******************************************************************************/
int canCtrl_Get_RxFrmBuf(vxT_CAN* pCan, UINT8 * rxBuf)
{
	int i = 0;
	
	rxBuf[i++] = (UINT8)canCtrl_Rd_CfgReg32(pCan, CAN_REG_ACR0_TXRXBUF);
	rxBuf[i++] = (UINT8)canCtrl_Rd_CfgReg32(pCan, CAN_REG_ACR1_TXRXBUF);
	rxBuf[i++] = (UINT8)canCtrl_Rd_CfgReg32(pCan, CAN_REG_ACR2_TXRXBUF);
	rxBuf[i++] = (UINT8)canCtrl_Rd_CfgReg32(pCan, CAN_REG_ACR3_TXRXBUF);
	
	rxBuf[i++] = (UINT8)canCtrl_Rd_CfgReg32(pCan, CAN_REG_AMR0_TXRXBUF);
	rxBuf[i++] = (UINT8)canCtrl_Rd_CfgReg32(pCan, CAN_REG_AMR1_TXRXBUF);
	rxBuf[i++] = (UINT8)canCtrl_Rd_CfgReg32(pCan, CAN_REG_AMR2_TXRXBUF);
	rxBuf[i++] = (UINT8)canCtrl_Rd_CfgReg32(pCan, CAN_REG_AMR3_TXRXBUF);
	
	rxBuf[i++] = (UINT8)canCtrl_Rd_CfgReg32(pCan, (CAN_REG_TXRXBUF+0x00));
	rxBuf[i++] = (UINT8)canCtrl_Rd_CfgReg32(pCan, (CAN_REG_TXRXBUF+0x04));
	rxBuf[i++] = (UINT8)canCtrl_Rd_CfgReg32(pCan, (CAN_REG_TXRXBUF+0x08));
	rxBuf[i++] = (UINT8)canCtrl_Rd_CfgReg32(pCan, (CAN_REG_TXRXBUF+0x0C));
	rxBuf[i++] = (UINT8)canCtrl_Rd_CfgReg32(pCan, (CAN_REG_TXRXBUF+0x10));

    return i;
}

/*****************************************************************************
*
* @description
* This function set Release Receive Buffer
*
* @param    dev is a pointer to the instance of can device.
*
* @return   0 if successful, otherwise 1.
*
* @note     None.
*
******************************************************************************/
int canCtrl_ReleaseRxBuf(vxT_CAN* pCan)
{
    UINT32 tmp32 = 0;
	
	/*
	CMR.2	RRB	释放接收缓存区	1：释放接收缓存区 。
	*/
	tmp32 = canCtrl_Rd_CfgReg32(pCan, CAN_REG_CMR) | 0x04;
	canCtrl_Wr_CfgReg32(pCan, CAN_REG_CMR, tmp32);
		
    return OK;
}

#endif

#if 1

void canSlcr_Enable_CanClk(void)
{
	slcr_write(0x304, 0x3); /* CAN1/0_APB_CLKACT: 1-enable     */
	return;
}

void canSlcr_Set_CanMIO(void)
{
	slcr_write(0x768, 0x00001621U);  /* MIO[26] g_CAN0 RX          */
	slcr_write(0x76C, 0x00001620U);  /* MIO[27] g_CAN0 TX     */
	    
	slcr_write(0x7C0, 0x00001620U);  /* MIO[48] g_CAN1 TX          */
	slcr_write(0x7C4, 0x00001621U);  /* MIO[49] g_CAN1 RX   */
	
	return;
}

/****************************************************************************/
/**
*
*  This function loop CAN0's outputs to CAN1's inputs,and CAN1's outputs to
*  CAN0's inputs
*
* @param
*  loop_en -- 0:connect CAN inputs according to MIO mapping;1:set the loop
*
* @return
*  Null
*
* @note
*  Null
*
****************************************************************************/
void canSlcr_Set_CanLoop(int loop_en)
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
	    tmp32 = slcr_read(SLCR_MIO_LOOPBACK);
		tmp32 &= ~0x04;
		slcr_write(SLCR_MIO_LOOPBACK, tmp32);
	}
	else if (loop_en == 1)
	{
		tmp32 = slcr_read(SLCR_MIO_LOOPBACK);
		tmp32 |= 0x04;
		slcr_write(SLCR_MIO_LOOPBACK, tmp32);
    }

	return;
}

#endif

#if 1


int vxInit_Can(int ctrl_x)
{
	vxT_CAN * pCan = NULL;
	vxT_CAN_CTRL * pCanCtrl = NULL;
	
	int ret = 0;	
	UINT32 tmp32 = 0;

	/* */
	/* enable can_clk of slcr */
	/* */
	canSlcr_Enable_CanClk();
	
	/* */
	/* setup can_MIO of slcr */
	/* */
	canSlcr_Set_CanMIO();
	
#if 1 
	/*
	init the pCan structure
	*/

	/* */
	/* spi_ctrl select */
	/* */
	switch (ctrl_x)
	{
	case CAN_CTRL_0:
		pCan = g_pCan0;	
		bzero(pCan, sizeof(vxT_CAN));
		
		pCan->pCanCtrl = (vxT_CAN_CTRL *)(&vxCan_Ctrl_0.ctrl_x);
		bzero(pCan->pCanCtrl, sizeof(vxT_CAN_CTRL));

		pCan->can_x = CAN_CTRL_0;
		pCan->pCanCtrl->devId = CAN_CTRL_0;
		pCan->pCanCtrl->cfgBaseAddr = VX_CAN_0_CFG_BASE;
		pCan->pCanCtrl->sysClk = FPAR_CANPS_0_CAN_CLK_FREQ_HZ;
		break;
		
	case CAN_CTRL_1:
		pCan = g_pCan1;
		bzero(pCan, sizeof(vxT_CAN));
	
		pCan->pCanCtrl = (vxT_CAN_CTRL *)(&vxCan_Ctrl_1.ctrl_x);
		bzero(pCan->pCanCtrl, sizeof(vxT_CAN_CTRL));
		
		pCan->can_x = CAN_CTRL_1;
		pCan->pCanCtrl->devId = CAN_CTRL_1;
		pCan->pCanCtrl->cfgBaseAddr = VX_CAN_1_CFG_BASE;
		pCan->pCanCtrl->sysClk = FPAR_CANPS_1_CAN_CLK_FREQ_HZ;
		break;
	}
	
	pCanCtrl = pCan->pCanCtrl;	
	
	pCanCtrl->ctrl_x = pCanCtrl->devId;

	sprintf((char*)(&pCanCtrl->BUS_TIME_C.name[0]), "%s", "bus_timing_c");
	pCanCtrl->BUS_TIME_C.tseg1_min = 1;
	pCanCtrl->BUS_TIME_C.tseg1_max = 16;
	pCanCtrl->BUS_TIME_C.tseg2_min = 1;
	pCanCtrl->BUS_TIME_C.tseg2_max = 8;
	pCanCtrl->BUS_TIME_C.sjw_max = 4;
	pCanCtrl->BUS_TIME_C.brp_min = 1;
	pCanCtrl->BUS_TIME_C.brp_max = 64;
	pCanCtrl->BUS_TIME_C.brp_inc = 1;
#endif	
	
	canCtrl_Set_BaudRate(pCan, CAN_BUAD_1MHZ);

	canCtrl_Set_StdACR(pCan, 0x55, 1, 0x66, 0x66, SINGLE_FILTER);

	canCtrl_Set_AMR(pCan, 0xff, 0xff, 0xff, 0xff);
		
	return;
}

int canCtrl_SndFrm_Poll(vxT_CAN* pCan, UINT32 id, UINT8* pData, int len)
{
	int timeout = 0x10000;
	
	canCtrl_Setup_StdFrmTx(pCan, id, pData, len, DATA_FRM);
	
	canCtrl_Set_TxRequest(pCan);

	/*
	SR.3	TCS 传输完成状态	1	最后请求的传输已成功完成。
				0	最后请求的传输尚未完成。
	*/
	while ((canCtrl_Get_TxOkStatus(pCan) == 0) && (timeout--));

	if (timeout <= 0)
	{
		return ERROR;
	}
	else
	{
		return OK;
	}
}

int canCtrl_RcvFrm_Poll(vxT_CAN* pCan, UINT8 * pRxBuf)
{
	int ret = 0;
	
	ret = canCtrl_Get_RxFrmBuf(pCan, pRxBuf);
	
	canCtrl_ReleaseRxBuf(pCan);
	        
	return ret;
}


int canCtrl_SLCR_Loop(UINT8 * tx_frm, UINT8 * rx_buf)
{
	vxT_CAN* pCan0 = g_pCan0;
	vxT_CAN* pCan1 = g_pCan1;
	
    int error = 0;

	int id = 0x55;	
	int len = 8;
    
    TRACE_OUT(DEBUG_OUT, "\r\n-I- SLCR_Loop Test\r\n");
	
    /* 
    enable SLCR loop 
	*/
    canSlcr_Set_CanLoop(TRUE);
    
    /* 
    spi0 master, spi1 slaver 
	*/
    if (pCan0->pCanCtrl->status != 1)
	{
	    vxInit_Can(CAN_CTRL_0);
		pCan0->pCanCtrl->status = 1;
	}

	if (pCan1->pCanCtrl->status != 1)
    {
    	vxInit_Can(CAN_CTRL_1);
		pCan1->pCanCtrl->status = 1;
    }	


#if 1 
	canCtrl_SndFrm_Poll(pCan0, id, tx_frm, len);	/* spi1-slave, spi0-master */
	printf("<slcr_loop>can_%d: snd \n", pCan0->pCanCtrl->ctrl_x);
		
	delay_ms(10);	
	printf("<------>\n");	
	
	canCtrl_RcvFrm_Poll(pCan1, rx_buf); 
	printf("<slcr_loop>can_%d: rcv \n\n", pCan1->pCanCtrl->ctrl_x);
#endif
	
    /* 
    disable slcr loop 
	*/
    canSlcr_Set_CanLoop(FALSE);

	return;
}


#endif


#if 1

int g_test_can2 = 0;


/* 
can SLCR loop: can0_tx  -> can1_rx
*/
void test_can_loop2(void)
{
	int i = 0;
	
	UINT8 tx_buf[16] = {0};
	UINT8 rx_buf[16] = {0};
	
	for (i=0; i<8; i++)
	{        
		tx_buf[i] = g_test_can2 + i;
	}
	g_test_can2++;

	
	canCtrl_SLCR_Loop(tx_buf, rx_buf);
	
    for(i=0; i<11; i++)
    {
       /*tx_buf[i] = canCtrl_Rd_CfgReg32((0x180 + 4 * i)); */
		tx_buf[i] = canCtrl_Rd_CfgReg32(g_pCan0, (CAN_REG_TXFIFO + 4 * i));
    }
	
	printf("\ncan_tx: \n");
    for(i = 0; i < 11; i++)
    {
       printf("%02X ", tx_buf[i]);
    }
	printf("\n");
	
	printf("can_rx: \n");
    for(i = 0; i < 11; i++)
    {
       printf("%02X ", rx_buf[i]);
    }
	printf("\n\n");

	
	return;
}


/* 
X_loop
*/
void test_can_loop3( UINT32 tx_data)
{
	UINT32 tx_loop_data = 0;
	
	if (tx_data == 0)
	{
		tx_loop_data = g_test_can2;
		g_test_can2++;		
	}
	else
	{
		tx_loop_data = tx_data;
	}
	
	/* canCtrl_X_Loop(tx_loop_data); */

	return;
}

#endif




