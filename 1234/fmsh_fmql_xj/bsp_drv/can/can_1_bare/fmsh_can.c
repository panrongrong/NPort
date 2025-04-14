/******************************************************************************
*
* Copyright (C) 2018 - 2028 FMSH, Inc.  All rights reserved.
*
******************************************************************************/
/******************************************************************************
*
* @file  fmsh_can.c
*
* This file contains all private & pbulic function
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
#include "../../common/fmsh_ps_parameters.h"

#include "fmsh_can_lib.h"

/************************** Constant Definitions *****************************/
#if 0
static const struct can_bittiming_const FCanPs_bittiming_const = 
{
	.tseg1_min = 1,
	.tseg1_max = 16,
	.tseg2_min = 1,
	.tseg2_max = 8,
	.sjw_max = 4,
	.brp_min = 1,
	.brp_max = 64,
	.brp_inc = 1,
};
#else
static const struct can_bittiming_const FCanPs_bittiming_const = 
{
 	"", 1, 16, 1, 8, 4, 1, 64, 1
};
#endif

#define CAN_CALC_MAX_ERROR 50 /* in one-tenth of a percent */
#define CAN_CALC_SYNC_SEG 1
#define UINT_MAX	(~0U)
#define MAX(a,b)                   (((a)>(b))?(a):(b))
#define MIN(a,b)                   (((a)<(b))?(a):(b))
#define clamp(a, min_val, max_val) MIN(MAX((a), (min_val)), (max_val))

/**************************** Type Definitions *******************************/

/***************** Macros (Inline Functions) Definitions *********************/

/************************** Function Prototypes ******************************/

/************************** Variable Definitions *****************************/

/************************** Function Prototypes ******************************/

/*****************************************************************************
*
* @description    Initialize can parameters.
*
* @param    dev is a pointer to the instance of can device.
* @param    id is id code odf can 0 or 1.
* @param    addr is the base address of can device.
* @param    clk is the operate clock of can.
*
* @return   0 if successful, otherwise 1.
*
* @note     None.
*
******************************************************************************/
u8 FCanPs_init(FCanPs_T *dev, FCanPs_Config *cfg)
{
    u8 retval;
    
    retval = 0;
    
    dev->id = cfg->DeviceId;
    dev->btc = FCanPs_bittiming_const;
    dev->base_address = (void*)(cfg->BaseAddress);
    dev->input_clock = cfg->InputClockHz;
    
    return retval;
}

/*****************************************************************************
*
* @description
* This function set can baud rate.
*
* @param    dev is a pointer to the instance of can device.
* @param    baud is baud rate value.
*
* @return   0 if successful, otherwise 1.
*
* @note     None.
*
******************************************************************************/
u8 FCanPs_setBaudRate(FCanPs_T *dev, u32 baud)
{
    struct can_bittiming *bt = &dev->bt;

    bt->bitrate = baud;

    can_calc_bittiming(dev);

    FCanPs_setResetMode(dev, CAN_set); /*enter reset mode*/
    mcan_set_bittiming(dev);
    FCanPs_setResetMode(dev, CAN_clear); /*exit reset mode*/
    
    return 0;
}

/*****************************************************************************
*
* @description
* This function set bit timing
*
* @param    dev is a pointer to the instance of can device.
*
* @return   0 if successful, otherwise 1.
*
* @note     None.
*
******************************************************************************/
static int mcan_set_bittiming(FCanPs_T *dev)
{
    struct can_bittiming *bt = &dev->bt;
    u32 btr0, btr1;
    u32 reg = 0;
    FCanPs_Portmap_T *portmap;

    portmap = (FCanPs_Portmap_T *) dev->base_address; 
    
   	btr0 = ((bt->brp - 1) & 0x3f) | (((bt->sjw - 1) & 0x3) << 6);
	btr1 = ((bt->prop_seg + bt->phase_seg1 - 1) & 0xf) |
		(((bt->phase_seg2 - 1) & 0x7) << 4); 
    
    reg = CAN_INP(portmap->btr0);    
    if(reg != btr0) 
        CAN_OUTP(btr0, portmap->btr0); 
    
    reg = CAN_INP(portmap->btr1);    
    if(reg != btr1) 
        CAN_OUTP(btr1, portmap->btr1); 
    
    return 0;
}

static u32 do_div(u32 n,u32 base) 
{					
	u32 __base = (base);				
	u32 __rem;						
	__rem = ((uint64_t)(n)) % __base;			
	(n) = ((uint64_t)(n)) / __base;				
	
    return __rem;							
}

/*
 * Bit-timing calculation derived from:
 *
 * Code based on LinCAN sources and H8S2638 project
 * Copyright 2004-2006 Pavel Pisa - DCE FELK CVUT cz
 * Copyright 2005      Stanislav Marek
 * email: pisa@cmp.felk.cvut.cz
 *
 * Calculates proper bit-timing parameters for a specified bit-rate
 * and sample-point, which can then be used to set the bit-timing
 * registers of the CAN controller. You can find more information
 * in the header file linux/can/netlink.h.
 */
static int can_update_sample_point(const struct can_bittiming_const *btc,
			  u32 sample_point_nominal, u32 tseg,
			  u32 *tseg1_ptr, u32 *tseg2_ptr,
			  u32 *sample_point_error_ptr)
{
	u32 sample_point_error, best_sample_point_error = UINT_MAX;
	u32 sample_point, best_sample_point = 0;
	u32 tseg1, tseg2;
	int i;

	for (i = 0; i <= 1; i++) 
    {
		tseg2 = tseg + CAN_CALC_SYNC_SEG - (sample_point_nominal * (tseg + CAN_CALC_SYNC_SEG)) / 1000 - i;
		tseg2 = clamp(tseg2, btc->tseg2_min, btc->tseg2_max);
		tseg1 = tseg - tseg2;
		if (tseg1 > btc->tseg1_max) 
        {
			tseg1 = btc->tseg1_max;
			tseg2 = tseg - tseg1;
		}

		sample_point = 1000 * (tseg + CAN_CALC_SYNC_SEG - tseg2) / (tseg + CAN_CALC_SYNC_SEG);
		sample_point_error = abs(sample_point_nominal - sample_point);

		if ((sample_point <= sample_point_nominal) && (sample_point_error < best_sample_point_error)) 
        {
			best_sample_point = sample_point;
			best_sample_point_error = sample_point_error;
			*tseg1_ptr = tseg1;
			*tseg2_ptr = tseg2;
		}
	}

	if (sample_point_error_ptr)
		*sample_point_error_ptr = best_sample_point_error;

	return best_sample_point;
}

/*
 * @description    calculate can bit timing.
 *
 * @para    dev is a can device handle.
 *
 * @return    return 0 if sucessful.
 *
 * @note    see can driver of linux -- dev.c/sja1000.c
 */
static int can_calc_bittiming(FCanPs_T *dev)
{
    u32 bitrate;			/* current bitrate */
	u32 bitrate_error;		/* difference between current and nominal value */
	u32 best_bitrate_error = UINT_MAX;
	u32 sample_point_error;	/* difference between current and nominal value */
	u32 best_sample_point_error = UINT_MAX;
	u32 sample_point_nominal;	/* nominal sample point */
	u32 best_tseg = 0;		/* current best value for tseg */
	u32 best_brp = 0;		/* current best value for brp */
	u32 brp, tsegall, tseg, tseg1 = 0, tseg2 = 0;
	u64 v64;
    
    struct can_bittiming *bt;
    const struct can_bittiming_const *btc;
    
    bt = &dev->bt;
    btc = &dev->btc;
        
   	/* Use CiA recommended sample points */
	if (bt->sample_point)
    {
		sample_point_nominal = bt->sample_point;
	} 
    else 
    {
		if (bt->bitrate > 800000)
			sample_point_nominal = 750;
		else if (bt->bitrate > 500000)
			sample_point_nominal = 800;
		else
			sample_point_nominal = 875;
	}

	/* tseg even = round down, odd = round up */
	for (tseg = (btc->tseg1_max + btc->tseg2_max) * 2 + 1;
	     tseg >= (btc->tseg1_min + btc->tseg2_min) * 2; tseg--) 
    {
		tsegall = CAN_CALC_SYNC_SEG + tseg / 2;

		/* Compute all possible tseg choices (tseg=tseg1+tseg2) */
		brp = dev->input_clock / (tsegall * bt->bitrate) + tseg % 2;

		/* choose brp step which is possible in system */
		brp = (brp / btc->brp_inc) * btc->brp_inc;
		if ((brp < btc->brp_min) || (brp > btc->brp_max))
			continue;

		bitrate = dev->input_clock / (brp * tsegall);
		bitrate_error = abs(bt->bitrate - bitrate);

		/* tseg brp biterror */
		if (bitrate_error > best_bitrate_error)
			continue;

		/* reset sample point error if we have a better bitrate */
		if (bitrate_error < best_bitrate_error)
			best_sample_point_error = UINT_MAX;

		can_update_sample_point(btc, sample_point_nominal, tseg / 2, &tseg1, &tseg2, &sample_point_error);
		if (sample_point_error > best_sample_point_error)
			continue;

		best_sample_point_error = sample_point_error;
		best_bitrate_error = bitrate_error;
		best_tseg = tseg / 2;
		best_brp = brp;

		if (bitrate_error == 0 && sample_point_error == 0)
			break;
	}
    
	/* real sample point */
	bt->sample_point = can_update_sample_point(btc, sample_point_nominal, best_tseg,
					  &tseg1, &tseg2, NULL);

	v64 = (u64)best_brp * 1000 * 1000 * 1000;
	do_div(v64, dev->input_clock);
	bt->tq = (u32)v64;
	bt->prop_seg = tseg1 / 2;
	bt->phase_seg1 = tseg1 - bt->prop_seg;
	bt->phase_seg2 = tseg2;

	/* check for sjw user settings */
	if (!bt->sjw || !btc->sjw_max)
    {
		bt->sjw = 1;
	}
    else
    {
		/* bt->sjw is at least 1 -> sanitize upper bound to sjw_max */
		if (bt->sjw > btc->sjw_max)
			bt->sjw = btc->sjw_max;
		/* bt->sjw must not be higher than tseg2 */
		if (tseg2 < bt->sjw)
			bt->sjw = tseg2;
	}

	bt->brp = best_brp;

	/* real bitrate */
	bt->bitrate = dev->input_clock / (bt->brp * (CAN_CALC_SYNC_SEG + tseg1 + tseg2));

	return 0;
    
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
u8 FCanPs_setStdACR(FCanPs_T *dev, u32 id, u8 ptr, u8 byte1, u8 byte2, enum filter_mode mode)
{
    FCanPs_Portmap_T *portmap;
    portmap = (FCanPs_Portmap_T *) dev->base_address;
    if(ptr > 0x1)
        return 1;

    if(mode == single_filter)
    {
        FCanPs_setResetMode(dev, CAN_set);
		
        CAN_OUTP(id >> 3, portmap->acr0_txrxbuf);
		
        CAN_OUTP(((id & 0x07) << 5)|(ptr << 4), portmap->acr1_txrxbuf);  
		
        CAN_OUTP(byte1, portmap->acr2_txrxbuf);
		
        CAN_OUTP(byte2, portmap->acr3_txrxbuf); 
		
        FCanPs_setResetMode(dev, CAN_clear);
    }
    else if(mode == dual_filter) 
    {
        FCanPs_setResetMode(dev, CAN_set);
		
        CAN_OUTP(id >> 3, portmap->acr0_txrxbuf);
		
        CAN_OUTP(((id & 0x07) << 5)|(ptr << 4)|(byte1 >> 4), portmap->acr1_txrxbuf);   
		
        CAN_OUTP(id >> 3, portmap->acr2_txrxbuf);
		
        CAN_OUTP(((id & 0x07) << 5)|(ptr << 4)|(byte1 & 0x0f), portmap->acr3_txrxbuf);   
		
        FCanPs_setResetMode(dev, CAN_clear);
    }
    else
        return 1;

    return 0;   
}

/*****************************************************************************
*
* @description
* This function set extended frame Acceptance Code Registers
*
* @param    dev is a pointer to the instance of can device.
* @param    id: 29 bits
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
u8 FCanPs_setEffACR(FCanPs_T *dev, u32 id, u8 ptr, u8 byte1, u8 byte2, enum filter_mode mode)
{
    FCanPs_Portmap_T *portmap;
    portmap = (FCanPs_Portmap_T *) dev->base_address;
    
    if (ptr > 0x1) 
        return 1;
    
    if (mode == single_filter)
    {
        FCanPs_setResetMode(dev, CAN_set);
        CAN_OUTP(id >> 21, portmap->acr0_txrxbuf);
        CAN_OUTP(id >> 13, portmap->acr1_txrxbuf);
        CAN_OUTP(id >> 5, portmap->acr2_txrxbuf);             
        CAN_OUTP(((id & 0x1f)<< 3)|(ptr << 2), portmap->acr3_txrxbuf);  
        FCanPs_setResetMode(dev, CAN_clear);
    }
    else if (mode == dual_filter) 
    {
        FCanPs_setResetMode(dev, CAN_set);
        CAN_OUTP(id >> 21, portmap->acr0_txrxbuf);
        CAN_OUTP(id >> 13, portmap->acr1_txrxbuf);
        CAN_OUTP(id >> 21, portmap->acr2_txrxbuf);
        CAN_OUTP(id >> 13, portmap->acr3_txrxbuf);
        FCanPs_setResetMode(dev, CAN_clear);
    }
    else
        return 1;
    
    return 0;        
}

/*****************************************************************************
*
* @description
* This function set Acceptance Mask Registers
*
* @param    dev is a pointer to the instance of can device.
* @param    AMR0~3 
*           0¡¯s in AMR0 ¨C 3 identify the bits at the corresponding positions in ACR0 ¨C 3 
*           which must be matched in the message identifier, ¡®1¡¯s identify the corresponding bits
*           as ¡®don¡¯t care¡¯.
*
* @return   0 if successful, otherwise 1.
*
* @note     Only read/write in reset mode.
*
******************************************************************************/
u8 FCanPs_setAMR(FCanPs_T *dev,u32 AMR0,u32 AMR1,u32 AMR2,u32 AMR3)
{
    FCanPs_Portmap_T *portmap;
    portmap = (FCanPs_Portmap_T *) dev->base_address;

    FCanPs_setResetMode(dev, CAN_set);
    CAN_OUTP(AMR0, portmap->amr0_txrxbuf);
    CAN_OUTP(AMR1, portmap->amr1_txrxbuf);       
    CAN_OUTP(AMR2, portmap->amr2_txrxbuf);
    CAN_OUTP(AMR3, portmap->amr3_txrxbuf); 
    FCanPs_setResetMode(dev, CAN_clear);

    return 0;
}

/*****************************************************************************
*
* @description
* This function set Sleep Mode
*
* @param    dev is a pointer to the instance of can device.
* @param    state:     
*           CAN_err = -1,
*           CAN_clear = 0,
*           CAN_set = 1
*
* @return   0 if successful, otherwise 1.
*
* @note     None.
*
******************************************************************************/
u8 FCanPs_setSleepMode(FCanPs_T *dev, enum FCanPs_state state)
{
    u32 reg = 0;
    FCanPs_Portmap_T *portmap;
    
    portmap = (FCanPs_Portmap_T *) dev->base_address; 

    reg = CAN_INP(portmap->mode);
    if(FMSH_BIT_GET(reg, CAN_MODE_SM) != state) 
    {
        FMSH_BIT_SET(reg, CAN_MODE_SM, state);
        CAN_OUTP(reg, portmap->mode); 
    }

    return 0;
}

/*****************************************************************************
*
* @description
* This function set Listen Only Mode
*
* @param    dev is a pointer to the instance of can device.
* @param    state:     
*           CAN_err = -1,
*           CAN_clear = 0,
*           CAN_set = 1
*
* @return   0 if successful, otherwise 1.
*
* @note     None.
*
******************************************************************************/
u8 FCanPs_setListenOnlyMode(FCanPs_T *dev, enum FCanPs_state state)
{
    u32 reg = 0;
    FCanPs_Portmap_T *portmap;
    
    portmap = (FCanPs_Portmap_T *) dev->base_address; 
    reg = CAN_INP(portmap->mode);
    if(FMSH_BIT_GET(reg, CAN_MODE_LOM) != state)
    {
        FMSH_BIT_SET(reg, CAN_MODE_LOM, state);
        CAN_OUTP(reg, portmap->mode); 
    }

    return 0;
}

/*****************************************************************************
*
* @description
* This function set Reset Mode
*
* @param    dev is a pointer to the instance of can device.
* @param    state:     
*           CAN_err = -1,
*           CAN_clear = 0,
*           CAN_set = 1
*
* @return   0 if successful, otherwise 1.
*
* @note     None.
*
******************************************************************************/
u8 FCanPs_setResetMode(FCanPs_T *dev, enum FCanPs_state state)
{
    u32 reg = 0;
    FCanPs_Portmap_T *portmap;
    
    portmap = (FCanPs_Portmap_T *) dev->base_address; 
    reg = CAN_INP(portmap->mode);
    if(FMSH_BIT_GET(reg, CAN_MODE_RM) != state)
    {
        FMSH_BIT_SET(reg, CAN_MODE_RM, state);
        CAN_OUTP(reg, portmap->mode); 
    }

    return 0;
}

/*****************************************************************************
*
* @description
* This function set Self Test Mode
*
* @param    dev is a pointer to the instance of can device.
* @param    state:     
*           CAN_err = -1,
*           CAN_clear = 0,
*           CAN_set = 1
*
* @return   0 if successful, otherwise 1.
*
* @note     None.
*
******************************************************************************/
u8 FCanPs_setSelfTestMode(FCanPs_T *dev, enum FCanPs_state state)
{
    u32 reg = 0;
    
    FCanPs_Portmap_T *portmap;
    portmap = (FCanPs_Portmap_T *) dev->base_address; 
    reg = CAN_INP(portmap->mode);
    if (FMSH_BIT_GET(reg, CAN_MODE_STM) != state) 
    {
        FMSH_BIT_SET(reg, CAN_MODE_STM, state);
        CAN_OUTP(reg, portmap->mode); 
    }

    return 0;
}

/*****************************************************************************
*
* @description
* This function set Acceptance Mask Mode
*
* @param    dev is a pointer to the instance of can device.
* @param    mode:     
*                single_filter
*                dual_filter
*
* @return   0 if successful, otherwise 1.
*
* @note     None.
*
******************************************************************************/
u8 FCanPs_setReceiveFilterMode(FCanPs_T *dev, enum filter_mode mode)
{
    u32 reg = 0;
    FCanPs_Portmap_T *portmap;
    portmap = (FCanPs_Portmap_T *) dev->base_address; 
    
    reg = CAN_INP(portmap->mode);
    if(FMSH_BIT_GET(reg, CAN_MODE_AFM) != mode) 
    {
        FMSH_BIT_SET(reg, CAN_MODE_AFM, mode);
        CAN_OUTP(reg, portmap->mode); 
    }

    return 0;
}

/*****************************************************************************
*
* @description
* This function returns the status of Receive Buffer Status.
*
* @param    dev is a pointer to the instance of can device.
*
* @return   status: 0/1
*
* @note     None.
*
******************************************************************************/
u32 FCanPs_getReceiveBufferStatus(FCanPs_T *dev)
{
    u32 reg = 0;
    
    FCanPs_Portmap_T *portmap;
    portmap = (FCanPs_Portmap_T *) dev->base_address; 
    reg = CAN_INP(portmap->sr);
    reg = FMSH_BIT_GET(reg, CAN_SR_RBS);

    return reg;
}

/*****************************************************************************
*
* @description
* This function returns the status of Data Overrun Status.
*
* @param    dev is a pointer to the instance of can device.
*
* @return   status: 0/1
*
* @note     None.
*
******************************************************************************/
u32 FCanPs_getDataOverrunStatus(FCanPs_T *dev)
{
    u32 reg = 0;
    FCanPs_Portmap_T *portmap;
    
    portmap = (FCanPs_Portmap_T *) dev->base_address; 
    reg = CAN_INP(portmap->sr);
    reg = FMSH_BIT_GET(reg, CAN_SR_DOS);

    return reg;
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
u32 FCanPs_getTransmitBufferStatus(FCanPs_T *dev)
{
    u32 reg = 0;
    FCanPs_Portmap_T *portmap;

    portmap = (FCanPs_Portmap_T *) dev->base_address; 
    reg = CAN_INP(portmap->sr);
    reg = FMSH_BIT_GET(reg, CAN_SR_TBS);

    return reg;
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
u32 FCanPs_getTransmissionCompleteStatus(FCanPs_T *dev)
{
    u32 reg = 0;
    FCanPs_Portmap_T *portmap;
    
    portmap = (FCanPs_Portmap_T *) dev->base_address; 
    reg = CAN_INP(portmap->sr);
    reg = FMSH_BIT_GET(reg, CAN_SR_TCS);

    return reg;
}

/*****************************************************************************
*
* @description
* This function returns the status of Receive Status
*
* @param    dev is a pointer to the instance of can device.
*
* @return   status: 0/1
*
* @note     None.
*
******************************************************************************/
u32 FCanPs_getReceiveStatus(FCanPs_T *dev)
{
    u32 reg = 0;
    FCanPs_Portmap_T *portmap;
    
    portmap = (FCanPs_Portmap_T *) dev->base_address; 
    reg = CAN_INP(portmap->sr);
    reg = FMSH_BIT_GET(reg, CAN_SR_RS);

    return reg;
}

/*****************************************************************************
*
* @description
* This function returns the status of Transmit Status
*
* @param    dev is a pointer to the instance of can device.
*
* @return   status: 0/1
*
* @note     None.
*
******************************************************************************/
u32 FCanPs_getTtansmitStatus(FCanPs_T *dev)
{
    u32 reg = 0;
    FCanPs_Portmap_T *portmap;
    
    portmap = (FCanPs_Portmap_T *)dev->base_address; 
    
    reg = CAN_INP(portmap->sr);
    reg = FMSH_BIT_GET(reg, CAN_SR_TS);

    return reg;
}

/*****************************************************************************
*
* @description
* This function returns the status of Error Status
*
* @param    dev is a pointer to the instance of can device.
*
* @return   status: 0/1
*
* @note     None.
*
******************************************************************************/
u32 FCanPs_getErrorStatus(FCanPs_T *dev)
{
    u32 reg = 0;
    FCanPs_Portmap_T *portmap;
    
    portmap = (FCanPs_Portmap_T *) dev->base_address; 
    
    reg = CAN_INP(portmap->sr);
    reg = FMSH_BIT_GET(reg, CAN_SR_ES);

    return reg;
}

/*****************************************************************************
*
* @description
* This function returns the status of Bus Status
*
* @param    dev is a pointer to the instance of can device.
*
* @return   status: 0/1
*
* @note     None.
*
******************************************************************************/
u32 FCanPs_getBusStatus(FCanPs_T *dev)
{
    u32 reg = 0;
    FCanPs_Portmap_T *portmap;
    
    portmap = (FCanPs_Portmap_T *) dev->base_address; 
    
    reg = CAN_INP(portmap->sr);
    reg = FMSH_BIT_GET(reg, CAN_SR_BS);

    return reg;
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
u8 FCanPs_transmissionRequest(FCanPs_T *dev)
{
    u32 reg = 0;
    FCanPs_Portmap_T *portmap;
    
    portmap = (FCanPs_Portmap_T *) dev->base_address; 
    
    FMSH_BIT_SET(reg, CAN_CMR_TR, CAN_set);
    CAN_OUTP(reg, portmap->cmr); 

    return 0;
}

/*****************************************************************************
*
* @description
* This function set Abort Transmission
*
* @param    dev is a pointer to the instance of can device.
*
* @return   0 if successful, otherwise 1.
*
* @note     None.
*
******************************************************************************/
u8 FCanPs_abortTransmission(FCanPs_T *dev)
{
    u32 reg = 0;
    FCanPs_Portmap_T *portmap;
    
    portmap = (FCanPs_Portmap_T *) dev->base_address; 
    
    FMSH_BIT_SET(reg, CAN_CMR_AT, CAN_set);
    CAN_OUTP(reg, portmap->cmr); 
    
    return 0;
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
u8 FCanPs_releaseReceiveBuffer(FCanPs_T *dev)
{
    u32 reg = 0;
    FCanPs_Portmap_T *portmap;
    
    portmap = (FCanPs_Portmap_T *) dev->base_address; 
    
    FMSH_BIT_SET(reg, CAN_CMR_RRB, CAN_set);
    CAN_OUTP(reg, portmap->cmr); 
    
    return 0;
}

/*****************************************************************************
*
* @description
* This function clears data overrun.
*
* @param    dev is a pointer to the instance of can device.
*
* @return   0 if successful, otherwise 1.
*
* @note     None.
*
******************************************************************************/
u8 FCanPs_clearDataOverrun(FCanPs_T *dev)
{
    u32 reg = 0;
    FCanPs_Portmap_T *portmap;
    
    portmap = (FCanPs_Portmap_T *) dev->base_address;
    
    FMSH_BIT_SET(reg, CAN_CMR_CDO, CAN_set);
    CAN_OUTP(reg, portmap->cmr); 
    
    return 0;
}

/*****************************************************************************
*
* @description
* This function set self reception request
*
* @param    dev is a pointer to the instance of can device.
*
* @return   0 if successful, otherwise 1.
*
* @note     None.
*
******************************************************************************/
u8 FCanPs_selfReceptionRequest(FCanPs_T *dev)
{
    u32 reg = 0;
    FCanPs_Portmap_T *portmap;
    
    portmap = (FCanPs_Portmap_T *) dev->base_address; 
    
    FMSH_BIT_SET(reg, CAN_CMR_SRR, CAN_set);
    CAN_OUTP(reg, portmap->cmr); 
    
    return 0;
}

/*****************************************************************************
*
* @description
* This function set Receive Interrupt
*
* @param    dev is a pointer to the instance of can device.
* @param    state:     
*           CAN_err = -1,
*           CAN_clear = 0,
*           CAN_set = 1
*
* @return   0 if successful, otherwise 1.
*
* @note     None.
*
******************************************************************************/
u8 FCanPs_setReceiveInterrupt(FCanPs_T *dev, enum FCanPs_state state)
{
    u32 reg = 0;
    FCanPs_Portmap_T *portmap;
    portmap = (FCanPs_Portmap_T *) dev->base_address; 
    reg = CAN_INP(portmap->ier);
    if(FMSH_BIT_GET(reg, CAN_IER_RIE) != state)
    {
        FMSH_BIT_SET(reg, CAN_IER_RIE, state);
        CAN_OUTP(reg, portmap->ier); 
    }
    
    return 0;
}

/*****************************************************************************
*
* @description
* This function set Transmit Interrupt
*
* @param    dev is a pointer to the instance of can device.
* @param    state:     
*           CAN_err = -1,
*           CAN_clear = 0,
*           CAN_set = 1
*
* @return   0 if successful, otherwise 1.
*
* @note     None.
*
******************************************************************************/
u8 FCanPs_setTransmitInterrupt(FCanPs_T *dev, enum FCanPs_state state)
{
    u32 reg = 0;
    FCanPs_Portmap_T *portmap;
    
    portmap = (FCanPs_Portmap_T *) dev->base_address; 
    
    reg = CAN_INP(portmap->ier);
    if(FMSH_BIT_GET(reg, CAN_IER_TIE) != state) 
    {
        FMSH_BIT_SET(reg, CAN_IER_TIE, state);
        CAN_OUTP(reg, portmap->ier); 
    }
    
    return 0;
}

/*****************************************************************************
*
* @description
* This function set Error Warning Interrupt
*
* @param    dev is a pointer to the instance of can device.
* @param    state:     
*           CAN_err = -1,
*           CAN_clear = 0,
*           CAN_set = 1
*
* @return   0 if successful, otherwise 1.
*
* @note     None.
*
******************************************************************************/
u8 FCanPs_setErrorWarningInterrupt(FCanPs_T *dev, enum FCanPs_state state)
{
    u32 reg = 0;
    FCanPs_Portmap_T *portmap;
    portmap = (FCanPs_Portmap_T *) dev->base_address; 
    reg = CAN_INP(portmap->ier);
    
    if(FMSH_BIT_GET(reg, CAN_IER_EIE) != state) 
    {
        FMSH_BIT_SET(reg, CAN_IER_EIE, state);
        CAN_OUTP(reg, portmap->ier); 
    }
    
    return 0;
}

/*****************************************************************************
*
* @description
* This function set Data Overrun Interrupt
*
* @param    dev is a pointer to the instance of can device.
* @param    state:     
*           CAN_err = -1,
*           CAN_clear = 0,
*           CAN_set = 1
*
* @return   0 if successful, otherwise 1.
*
* @note     None.
*
******************************************************************************/
u8 FCanPs_setDataOverrunInterrupt(FCanPs_T *dev, enum FCanPs_state state)
{
    u32 reg = 0;
    FCanPs_Portmap_T *portmap;
    portmap = (FCanPs_Portmap_T *) dev->base_address; 

    reg = CAN_INP(portmap->ier);

    if(FMSH_BIT_GET(reg, CAN_IER_DOIE) != state) 
    {
        FMSH_BIT_SET(reg, CAN_IER_DOIE, state);
        CAN_OUTP(reg, portmap->ier); 
    }

    return 0;
}

/*****************************************************************************
*
* @description
* This function set Wake-Up Interrupt
*
* @param    dev is a pointer to the instance of can device.
* @param    state:     
*           CAN_err = -1,
*           CAN_clear = 0,
*           CAN_set = 1
*
* @return   0 if successful, otherwise 1.
*
* @note     None.
*
******************************************************************************/
u8 FCanPs_setWakeUpInterrupt(FCanPs_T *dev, enum FCanPs_state state)
{
    u32 reg = 0;
    FCanPs_Portmap_T *portmap;
    portmap = (FCanPs_Portmap_T *) dev->base_address; 
    
    reg = CAN_INP(portmap->ier);
    if(FMSH_BIT_GET(reg, CAN_IER_WUIE) != state) 
    {
        FMSH_BIT_SET(reg, CAN_IER_WUIE, state);
        CAN_OUTP(reg, portmap->ier); 
    }
    
    return 0;
}

/*****************************************************************************
*
* @description
* This function set Error Passive Interrupt
*
* @param    dev is a pointer to the instance of can device.
* @param    state:     
*           CAN_err = -1,
*           CAN_clear = 0,
*           CAN_set = 1
*
* @return   0 if successful, otherwise 1.
*
* @note     None.
*
******************************************************************************/
u8 FCanPs_setErrorPassiveInterrupt(FCanPs_T *dev, enum FCanPs_state state)
{
    u32 reg = 0;
    FCanPs_Portmap_T *portmap;
    
    portmap = (FCanPs_Portmap_T *) dev->base_address; 
    
    reg = CAN_INP(portmap->ier);
    if(FMSH_BIT_GET(reg, CAN_IER_EPIE) != state) 
    {
        FMSH_BIT_SET(reg, CAN_IER_EPIE, state);
        CAN_OUTP(reg, portmap->ier); 
    }
    
    return 0;
}

/*****************************************************************************
*
* @description
* This function set Arbitration Loss Interrupt
*
* @param    dev is a pointer to the instance of can device.
* @param    state:     
*           CAN_err = -1,
*           CAN_clear = 0,
*           CAN_set = 1
*
* @return   0 if successful, otherwise 1.
*
* @note     None.
*
******************************************************************************/
u8 FCanPs_setArbitrationLostInterrupt(FCanPs_T *dev, enum FCanPs_state state)
{
    u32 reg = 0;
    FCanPs_Portmap_T *portmap;
    
    portmap = (FCanPs_Portmap_T *) dev->base_address; 
    
    reg = CAN_INP(portmap->ier);
    if(FMSH_BIT_GET(reg, CAN_IER_ALIE) != state)
    {
        FMSH_BIT_SET(reg, CAN_IER_ALIE, state);
        CAN_OUTP(reg, portmap->ier); 
    }
    
    return 0;
}

/*****************************************************************************
*
* @description
* This function set Bus Error Interrupt
*
* @param    dev is a pointer to the instance of can device.
* @param    state:     
*           CAN_err = -1,
*           CAN_clear = 0,
*           CAN_set = 1
*
* @return   0 if successful, otherwise 1.
*
* @note     None.
*
******************************************************************************/
u8 FCanPs_setBusErrorInterrupt(FCanPs_T *dev, enum FCanPs_state state)
{
    u32 reg = 0;
    FCanPs_Portmap_T *portmap;
    
    portmap = (FCanPs_Portmap_T *) dev->base_address; 
    
    reg = CAN_INP(portmap->ier);
    if(FMSH_BIT_GET(reg, CAN_IER_BEIE) != state) 
    {
        FMSH_BIT_SET(reg, CAN_IER_BEIE, state);
        CAN_OUTP(reg, portmap->ier); 
    }
    
    return 0;
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
u8 FCanPs_standardFrameTransmit(FCanPs_T *dev, u32 id,u8 *sbuf,u8 len,enum frame_type state)
{
    u8 *p;
    u32 timeout0;	
    FCanPs_Portmap_T *portmap;
	
    p = sbuf;
    portmap = (FCanPs_Portmap_T *) dev->base_address; 
    timeout0 = 10000;

    while ((FCanPs_getTransmitBufferStatus(dev) == 0) && (timeout0--));  /*wait tx buffer empty*/

    CAN_OUTP((state<<6)|len, portmap->acr0_txrxbuf); 
    CAN_OUTP(((id<<5)&0xff00)>>8, portmap->acr1_txrxbuf);     
    CAN_OUTP(((id<<5)|(state<<4))&0x00ff, portmap->acr2_txrxbuf);
	
    /*data frame*/
    if (state == data_frame) 
    {
        CAN_OUTP(*p++, portmap->acr3_txrxbuf);
		
        CAN_OUTP(*p++, portmap->amr0_txrxbuf);
        CAN_OUTP(*p++, portmap->amr1_txrxbuf);
        CAN_OUTP(*p++, portmap->amr2_txrxbuf);
        CAN_OUTP(*p++, portmap->amr3_txrxbuf);
		
        CAN_OUTP(*p++, portmap->txrxbuf[0]);
        CAN_OUTP(*p++, portmap->txrxbuf[1]);
        CAN_OUTP(*p++, portmap->txrxbuf[2]);
    }
    
    return 0;
}

/*****************************************************************************
*
* @description
* This function sends extended frame format message.
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
u8 FCanPs_extendedFrameTransmit(FCanPs_T *dev, u32 id,u8 *sbuf,u8 len,enum frame_type state)
{
    u8 *p;
    p = sbuf;
    u32 timeout0;
    timeout0 = 10000;

    FCanPs_Portmap_T *portmap;
    portmap = (FCanPs_Portmap_T *) dev->base_address; 

    while((FCanPs_getTransmitBufferStatus(dev) == 0) && (timeout0--));/*wait tx buffer empty*/

    CAN_OUTP((state<<6)|len|(1<<7), portmap->acr0_txrxbuf); 
    CAN_OUTP(((id<<3)&0xff000000)>> 24, portmap->acr1_txrxbuf);     
    CAN_OUTP(((id<<3)&0x00ff0000)>> 16, portmap->acr2_txrxbuf);
    CAN_OUTP(((id<<3)&0x0000ff00)>> 8, portmap->acr3_txrxbuf);
    CAN_OUTP((((id<<3)|(state<<2))&0x000000ff), portmap->amr0_txrxbuf);    
    if(state == data_frame) 
    {
        CAN_OUTP(*p++, portmap->amr1_txrxbuf);
        CAN_OUTP(*p++, portmap->amr2_txrxbuf);
        CAN_OUTP(*p++, portmap->amr3_txrxbuf);
        CAN_OUTP(*p++, portmap->txrxbuf[0]);
        CAN_OUTP(*p++, portmap->txrxbuf[1]);
        CAN_OUTP(*p++, portmap->txrxbuf[2]);
        CAN_OUTP(*p++, portmap->txrxbuf[3]);
        CAN_OUTP(*p++, portmap->txrxbuf[4]);
    }
    
    return 0;
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
u8 FCanPs_frameReceive(FCanPs_T *dev, u8 *rbuf)
{
    u8 *p, frame;
    p = rbuf;
   
    FCanPs_Portmap_T *portmap;
    portmap = (FCanPs_Portmap_T *) dev->base_address; 

    frame = CAN_INP(portmap->acr0_txrxbuf);
	
    *p++ = frame;
	
    *p++ = CAN_INP(portmap->acr1_txrxbuf);
    *p++ = CAN_INP(portmap->acr2_txrxbuf);  
    *p++ = CAN_INP(portmap->acr3_txrxbuf);
    *p++ = CAN_INP(portmap->amr0_txrxbuf);
    *p++ = CAN_INP(portmap->amr1_txrxbuf);
    *p++ = CAN_INP(portmap->amr2_txrxbuf);                
    *p++ = CAN_INP(portmap->amr3_txrxbuf);
	
    *p++ = CAN_INP(portmap->txrxbuf[0]);   
    *p++ = CAN_INP(portmap->txrxbuf[1]);  
    *p++ = CAN_INP(portmap->txrxbuf[2]);  
    *p++ = CAN_INP(portmap->txrxbuf[3]);  
    *p++ = CAN_INP(portmap->txrxbuf[4]);  

    return 0;
}

/*****************************************************************************
*
* @description
* This function returns interrupt register value.
*
* @param    dev is a pointer to the instance of can device.
*
* @return   value of inerrupt register.
*
* @note     None.
*
******************************************************************************/
u32 FCanPs_getInterrupt(FCanPs_T *dev)
{
    u32 reg = 0;
    FCanPs_Portmap_T *portmap;

    portmap = (FCanPs_Portmap_T *) dev->base_address; 
    reg = CAN_INP(portmap->ir);

    return reg;
}


#if 1  /* polling mode*/

#define CAN_BUAD_1MHZ   1000000
#define CAN_BUAD_500KHZ  500000
#define CAN_BUAD_250KHZ  250000
#define CAN_BUAD_100KHZ  100000
#define CAN_BUAD_50KHZ    50000
#define CAN_BUAD_25KHZ    25000

static FCanPs_T g_CAN0 = {0}; 
FCanPs_T * pCAN0 = (FCanPs_T *)(&g_CAN0);

static FCanPs_T g_CAN1 = {0};  
FCanPs_T * pCAN1 = (FCanPs_T *)(&g_CAN1);

u8 g_can0rbuf[50] = {0};
u8 g_can1rbuf[50] = {0};

u8 can_init(void)
{
    u8 ret=0;     
	
    /*Initialize UARTs device & instance*/
    FCanPs_Config* Config=NULL;
	
    Config= FCanPs_LookupConfig(FPAR_CANPS_0_DEVICE_ID);
    if(Config==NULL)
      return FMSH_FAILURE;
	
    ret=FCanPs_init(pCAN0, Config);
    if(ret!=FMSH_SUCCESS)
      return ret;
    
    Config= FCanPs_LookupConfig(FPAR_CANPS_1_DEVICE_ID);
    if(Config==NULL)
      return FMSH_FAILURE;
	
    ret=FCanPs_init(pCAN1, Config);
    if(ret!=FMSH_SUCCESS)
      return ret;

    return FMSH_SUCCESS;
}

/*
can0 <--> can1, loopback by slcr
*/
u8 test_can_loop0_slcr(FCanPs_T* dev1, FCanPs_T* dev2)
{
#if 0
    u32 i;
    u32 tr_val[11];
    u8 buf[8];
    u32 timeout = 0x10000;
    u8 intrFlag;
    u32 addrSrc;
    
    if (dev1->id == 0)
            addrSrc = FPS_CAN0_BASEADDR;
    else if (dev1->id == 1)
            addrSrc = FPS_CAN1_BASEADDR; 
    
    for(i = 0; i < 8; i++)
            buf[i] = 0x55 + i;
    
    can_init();
    can_defaultParaSet(dev1);
    can_defaultParaSet(dev2);
    FSlcrPS_setCanLoop(1);
    
    FCanPs_setResetMode(dev2, CAN_set);
    FCanPs_setReceiveInterrupt(dev2, CAN_set);
    FCanPs_setResetMode(dev2,  CAN_clear);
    
    intrFlag = 0;
    FCanPs_standardFrameTransmit(dev1, 0x55, buf, 8, data_frame);
    FCanPs_transmissionRequest(dev1);
    timeout = 0x10000;
    while((FCanPs_getTransmissionCompleteStatus(dev1) == 0)&&(timeout--));
    if(timeout == 0)
            return 1;
    if(dev2->id == 0)
            intrFlag = g_can0_recv_intr_flag;
    else if(dev2->id == 1)
            intrFlag = g_can1_recv_intr_flag;
    if(intrFlag == 1) {
            intrFlag = 0;
            
            for(i = 0; i < 11; i++) {
                    tr_val[i] = FMSH_ReadReg(addrSrc, 0x180 + 4 * i);
            }
            for(i = 0; i < 11; i++) { 
                    if(dev2->id == 0) {
                            if(tr_val[i] != g_can0rbuf[i])
                            return 1;
                    }
                    else if(dev2->id == 1) {
                            if(tr_val[i] != g_can1rbuf[i])
                            return 1;
                    }
            }
    }
    else
            return 1;
    
    intrFlag = 0;
    FCanPs_standardFrameTransmit(dev1, 0x55, buf, 8, remote_frame);
    FCanPs_transmissionRequest(dev1);
	
    timeout = 0x10000;
    while((FCanPs_getTransmissionCompleteStatus(dev1) == 0)&&(timeout--));
	
    if(timeout == 0)
            return 1;
	
    if(dev2->id == 0)
            intrFlag = g_can0_recv_intr_flag;
    else if(dev2->id == 1)
            intrFlag = g_can1_recv_intr_flag;
	
    if(intrFlag == 1) {
            intrFlag = 0;
            
            for(i = 0; i < 11; i++) {
                    tr_val[i] = FMSH_ReadReg(addrSrc, 0x180 + 4 * i);
            }
            for(i = 0; i < 3; i++) {
                    if(dev2->id == 0) {
                            if(tr_val[i] != g_can0rbuf[i])
                            return 1;
                    }
                    else if(dev2->id == 1) {
                            if(tr_val[i] != g_can1rbuf[i])
                            return 1;
                    }
            }
    }
    else
            return 1;
    
    intrFlag = 0;
    FCanPs_extendedFrameTransmit(dev1, 0x55, buf, 8, data_frame);
    FCanPs_transmissionRequest(dev1);
    timeout = 0x10000;
    while((FCanPs_getTransmissionCompleteStatus(dev1) == 0)&&(timeout--));
    if(timeout == 0)
            return 1;
    if(dev2->id == 0)
            intrFlag = g_can0_recv_intr_flag;
    else if(dev2->id == 1)
            intrFlag = g_can1_recv_intr_flag;
    if(intrFlag == 1) {
            intrFlag = 0;
            
            for(i = 0; i < 11; i++) {
                    tr_val[i] = FMSH_ReadReg(addrSrc, 0x180 + 4 * i);/*read tx buffer data*/
            }
            for(i = 0; i < 11; i++) {
                    if(dev2->id == 0) {
                            if(tr_val[i] != g_can0rbuf[i])/*read rx buffer data, compare*/
                            return 1;
                    }
                    else if(dev2->id == 1) {
                            if(tr_val[i] != g_can1rbuf[i])/*read rx buffer data, compare*/
                            return 1;
                    }
            }
    }
    else
            return 1;
    
    intrFlag = 0;
    FCanPs_extendedFrameTransmit(dev1, 0x55, buf, 8, remote_frame);
    FCanPs_transmissionRequest(dev1);
    timeout = 0x10000;
    while((FCanPs_getTransmissionCompleteStatus(dev1) == 0)&&(timeout--));
    if(timeout == 0)
            return 1;
    if(dev2->id == 0)
            intrFlag = g_can0_recv_intr_flag;
    else if(dev2->id == 1)
            intrFlag = g_can1_recv_intr_flag;
    if(intrFlag == 1) {
            intrFlag = 0;
            
            for(i = 0; i < 11; i++) {
                    tr_val[i] = FMSH_ReadReg(addrSrc, 0x180 + 4 * i);
            }
            for(i = 0; i < 3; i++) {
                    if(dev2->id == 0) {
                            if(tr_val[i] != g_can0rbuf[i])
                            return 1;
                    }
                    else if(dev2->id == 1) {
                            if(tr_val[i] != g_can1rbuf[i])
                            return 1;
                    }
            }
    }
    else
            return 1;  
    
    FSlcrPS_setCanLoop(0);
#endif

    return 0;
}

int g_test_base_can = 0;
u8 test_can_loop1_slcr(void)
{
	u32 i;
	u8 ret = FMSH_SUCCESS;
	u32 tr_val[11];
	
	u8 buf[8];
	u32 timeout = 0x10000;

	for (i=0; i<8; i++)
	{        
		buf[i] = g_test_base_can + i;
	}
	g_test_base_can++;

#if 1
	/**/
	/* MIO init*/
	/**/
	FMSH_WriteReg(FPS_SLCR_BASEADDR, 0x008, 0xDF0D767BU); /*unlock     */
	
	FMSH_WriteReg(FPS_SLCR_BASEADDR, 0x304, 0x3); /* CAN1/0_APB_CLKACT: 1-enable    */
	
	FMSH_WriteReg(FPS_SLCR_BASEADDR, 0x768, 0x00001621U);/* MIO[26] g_CAN0 RX         */
	FMSH_WriteReg(FPS_SLCR_BASEADDR, 0x76C, 0x00001620U);/* MIO[27] g_CAN0 TX    */
	FMSH_WriteReg(FPS_SLCR_BASEADDR, 0x004, 0xDF0D767BU);/*lock    */
	    
	FMSH_WriteReg(FPS_SLCR_BASEADDR, 0x008, 0xDF0D767BU);/*unlock       */
	FMSH_WriteReg(FPS_SLCR_BASEADDR, 0x7C0, 0x00001620U);/* MIO[48] g_CAN1 TX         */
	FMSH_WriteReg(FPS_SLCR_BASEADDR, 0x7C4, 0x00001621U);/* MIO[49] g_CAN1 RX  */
	FMSH_WriteReg(FPS_SLCR_BASEADDR, 0x004, 0xDF0D767BU);/*lock              */
#endif

	/* can_0*/
	FCanPs_Config* Config = NULL;
	Config = FCanPs_LookupConfig(FPAR_CANPS_0_DEVICE_ID);
	if (Config==NULL)
	    return FMSH_FAILURE;
	
	ret = FCanPs_init(pCAN0, Config);
	if (ret != FMSH_SUCCESS)
	    return ret;

	FCanPs_setBaudRate(pCAN0, CAN_BUAD_1MHZ);
	FCanPs_setStdACR(pCAN0, 0x55, 1, 0x66, 0x66, single_filter);
	FCanPs_setAMR(pCAN0, 0xff, 0xff, 0xff, 0xff); 

	/* can_1*/
	Config = FCanPs_LookupConfig(FPAR_CANPS_1_DEVICE_ID);
	if (Config == NULL)
	  return FMSH_FAILURE;
	
	ret = FCanPs_init(pCAN1, Config);
	if (ret != FMSH_SUCCESS)
	  return ret;

	FCanPs_setBaudRate(pCAN1, CAN_BUAD_1MHZ);
	FCanPs_setStdACR(pCAN1, 0x55, 1, 0x66, 0x66, single_filter);
	FCanPs_setAMR(pCAN1, 0xff, 0xff, 0xff, 0xff);    

/*
	can_setHanlder(pCAN1, CAN1_INT_ID, (FMSH_InterruptHandler)CAN1_interrupt_hanlder);
	FCanPs_setReceiveInterrupt(pCAN1, CAN_set);
*/
	FSlcrPS_setCanLoop(1);  /* slcr_loop*/

	/**/
	/* can0_tx ...*/
	/**/
	/*g_can_recv_intr_flag = 0;*/
	FCanPs_standardFrameTransmit(pCAN0, 0x55, buf, 8, data_frame);
	FCanPs_transmissionRequest(pCAN0);

	timeout = 0x10000;
	while((FCanPs_getTransmissionCompleteStatus(pCAN0) == 0)&&(timeout--));

	if (timeout == 0)
	        return 1;

	/**/
	/* can1_rx ...*/
	/**/
	
    FCanPs_Portmap_T *portmap;	
    portmap = (FCanPs_Portmap_T *) pCAN1->base_address; 
	printf("\n\n rx_cnt: %d bytes \n", CAN_INP(portmap->rmc));
	
	FCanPs_frameReceive(pCAN1, g_can1rbuf);
	FCanPs_releaseReceiveBuffer(pCAN1);

	/* read tx_buf for check*/
    for(i=0; i<11; i++)
    {
       tr_val[i] = FMSH_ReadReg(FPS_CAN0_BASEADDR, (0x180 + 4 * i));
    }
	
	printf("can_tx: \n");
    for(i = 0; i < 11; i++)
    {
       printf("%02X ", tr_val[i]);
    }
	printf("\n");
	
	printf("can_rx: \n");
    for(i = 0; i < 11; i++)
    {
       printf("%02X ", g_can1rbuf[i]);
    }
	printf("\n");
	

	FSlcrPS_setCanLoop(0);        

	return ret;
}

#endif

/*
can rx irq mode
*/
#if 1

u8 g_can0_recv_intr_flag = 0;
u8 g_can1_recv_intr_flag = 0;

// jc
u8 g_can0_rx_irq_init_ok = 0;
u8 g_can1_rx_irq_init_ok = 0;

u32 g_can_recv_counter = 0;

static u8 err_counter = 0;

/****************************************************************************/
/**
*
* This function is used to handle g_CAN0 intrrrupt
*
* @param    None.
*
* @return   None.
*
* @note     None.
*
****************************************************************************/
void CAN0_interrupt_hanlder (void)
{
    u8 reg;
    
    g_can0_recv_intr_flag = 1;
    
    reg = FCanPs_getInterrupt(&g_CAN0);/*intr sts reg is read only,all bits will be reset except intr recv bit*/

    if (FMSH_BIT_GET(reg, CAN_IR_RI) == 1) 
    {  
    	/*recv buf full*/
        FCanPs_frameReceive(&g_CAN0, g_can0rbuf);
        FCanPs_releaseReceiveBuffer(&g_CAN0);
		
        err_counter++;
    }

	if (FMSH_BIT_GET(reg, CAN_IR_TI) == 1) 
    {  /**/
        err_counter++;
    }
	
    if (FMSH_BIT_GET(reg, CAN_IR_EI) == 1)
    {  /*err warnning*/
        err_counter++;
    }    
	
    if (FMSH_BIT_GET(reg, CAN_IR_DOI) == 1) 
    {  /*overrun*/
        err_counter++;
    }   
	
    if (FMSH_BIT_GET(reg, CAN_IR_WUI) == 1)
    {  /*wakeup*/
        err_counter++;
    }   
	
    if (FMSH_BIT_GET(reg, CAN_IR_EPI) == 1) 
    {  /*err passive*/
        err_counter++;
    }   
	
    if (FMSH_BIT_GET(reg, CAN_IR_ALI) == 1) 
    {  /*alc lost*/
        err_counter++;
    }   
	
    if (FMSH_BIT_GET(reg, CAN_IR_BEI) == 1) 
    {  /*bus err*/
        err_counter++;
    }   

	return;
}

/****************************************************************************/
/**
*
* This function is used to handle g_CAN1 intrrrupt
*
* @param  None.
*  
* @return   None.
*
* @note     None.
*
****************************************************************************/
void CAN1_interrupt_hanlder (void)
{
    u8 reg;

    g_can1_recv_intr_flag = 1;
            
    reg = FCanPs_getInterrupt(&g_CAN1);/*intr sts reg is read only,all bits will be reset except intr recv bit*/

    if (FMSH_BIT_GET(reg, CAN_IR_RI) == 1) 
    { 
    	/*recv buf full*/
        FCanPs_frameReceive(&g_CAN1, g_can1rbuf);
        FCanPs_releaseReceiveBuffer(&g_CAN1);
		
        err_counter++;
        g_can_recv_counter++;
    }
	
    if (FMSH_BIT_GET(reg, CAN_IR_TI) == 1) 
    {  /**/
        err_counter++;
    }
	
    if (FMSH_BIT_GET(reg, CAN_IR_EI) == 1) 
    {  /*err warnning*/
        err_counter++;
    }   
	
    if (FMSH_BIT_GET(reg, CAN_IR_DOI) == 1)
    {  /*overrun*/
        err_counter++;
    }   
	
    if (FMSH_BIT_GET(reg, CAN_IR_WUI) == 1) 
    {  /*wakeup*/
        err_counter++;
    }  
	
    if (FMSH_BIT_GET(reg, CAN_IR_EPI) == 1) 
    {  /*err passive*/
        err_counter++;
    }  
	
    if (FMSH_BIT_GET(reg, CAN_IR_ALI) == 1)
    {  /*alc lost*/
        err_counter++;
    }   
	
    if (FMSH_BIT_GET(reg, CAN_IR_BEI) == 1)
    {  /*bus err*/
        err_counter++;
    }   

	return;
}


#define CAN0_INT_ID		56U
#define CAN1_INT_ID		77U

extern void vx_FGicPs_Enable(UINT32 Int_Id);

/****************************************************************************/
/**
*
* This function is used to set default patameters of g_CAN0/1
*
* @param    dev is a pointer to the instance of g_CAN0/1
*
* @return   0 if successful, otherwise 1.
*
* @note    FCanPs_setBaudRate can be changed by can clock.
*
****************************************************************************/
u8 can_defaultParaSet(FCanPs_T* dev)
{     
    g_can_recv_counter = 0;
	
    can_init();
    
    if (dev->id ==0) 
    {
        FSlcrPs_ipSetRst(SLCR_CAN0_CTRL, CAN0_APB_RST);
        FSlcrPs_ipReleaseRst(SLCR_CAN0_CTRL, CAN0_APB_RST);
    }
    else if(dev->id == 1)
    {
        FSlcrPs_ipSetRst(SLCR_CAN1_CTRL, CAN1_APB_RST);
        FSlcrPs_ipReleaseRst(SLCR_CAN1_CTRL, CAN1_APB_RST);
    }
    else
        return 1;

    FCanPs_setBaudRate(dev, 1000000);/*10MHZ apb clk -- 1Mbps*/
    
    FCanPs_setStdACR(dev, 0x55, 1, 0x66, 0x66, single_filter);
    FCanPs_setAMR(dev, 0xff, 0xff, 0xff, 0xff);
    
    if (dev->id == 0)
    {    
    	/*can_setHanlder(dev, CAN0_INT_ID, (FMSH_InterruptHandler)CAN0_interrupt_hanlder);		*/
    	if (g_can0_rx_irq_init_ok == 0)
    	{
			intConnect(INUM_TO_IVEC(INT_VEC_CAN_INT0), CAN0_interrupt_hanlder, NULL);
			intEnable(INT_VEC_CAN_INT0);
			
			vx_FGicPs_Enable(CAN0_INT_ID);	
			g_can0_rx_irq_init_ok = 1;
    	}		
    }
    else if (dev->id == 1)
    {    
    	/*can_setHanlder(dev, CAN1_INT_ID, (FMSH_InterruptHandler)CAN0_interrupt_hanlder);*/
    	if (g_can1_rx_irq_init_ok == 0)
    	{
			intConnect(INUM_TO_IVEC(INT_VEC_CAN_INT1), CAN1_interrupt_hanlder, NULL);
			intEnable(INT_VEC_CAN_INT1);
			
			vx_FGicPs_Enable(CAN1_INT_ID);	
			g_can1_rx_irq_init_ok = 1;
		}
    }
    else
    {    
    	return 1;
    }
       
    return 0;
}

/****************************************************************************/
/**
*
* This function communicte messages between dev1 & dev2.
* dev1 & dev2 inner loopback by set SLCR register
*
* @param    dev1 & dev2 is a pointer to the instance of can device: CAN0/1.
*
* @return   0 if successful, otherwise 1.
*
* @note     None.
*
****************************************************************************/
int g_can_test_base = 0;
u8 can_can02Can1DataTrans(FCanPs_T* dev1, FCanPs_T* dev2)
{
	u32 i;
	u32 tr_val[11];
	u8 buf[8];
	u32 timeout = 0x10000;
	u8 intrFlag;
	u32 addrSrc;

	if (dev1->id == 0)
	{    
		addrSrc = FPS_CAN0_BASEADDR;
	}
	else if (dev1->id == 1)
	{    
		addrSrc = FPS_CAN1_BASEADDR;
	}

	for (i = 0; i < 8; i++)
	{     
		/*buf[i] = 0x55 + i;*/
		buf[i] = g_can_test_base + i;
	}
	g_can_test_base++;

	can_init();

	can_defaultParaSet(dev1);
	can_defaultParaSet(dev2);

	/**/
	/* enable slcr loop mode*/
	/**/
	FSlcrPS_setCanLoop(1);

	/**/
	/* can_1 rx_irq*/
	/**/
    FCanPs_setResetMode(dev2, CAN_set);
    FCanPs_setReceiveInterrupt(dev2, CAN_set);
    FCanPs_setResetMode(dev2,  CAN_clear);
	
	/**/
	/* can_0 rx_irq*/
	/**/
    FCanPs_setResetMode(dev1, CAN_set);
    FCanPs_setReceiveInterrupt(dev1, CAN_set);
    FCanPs_setResetMode(dev1,  CAN_clear);

	

#if 1  /* data_frame*/

    intrFlag = 0;
    FCanPs_standardFrameTransmit(dev1, 0x55, buf, 8, data_frame);
    FCanPs_transmissionRequest(dev1);

	printf("can_%d->tx: ", dev1->id);
	for (i = 0; i < 8; i++)
	{     
		printf("%02X ", buf[i]);
	}
	printf("\n");

	/**/
	/* waiting for tx finish*/
	/**/
    timeout = 0x10000;
    while ((FCanPs_getTransmissionCompleteStatus(dev1) == 0)&&(timeout--));
	
    if (timeout == 0)
    {        
    	return 1;
    }

	/**/
	/* get the irq flag*/
	/**/
    if (dev2->id == 0)
    {        
    	intrFlag = g_can0_recv_intr_flag;
	}
    else if (dev2->id == 1)
    {        
    	intrFlag = g_can1_recv_intr_flag;
	}

	/*	*/
	/* read data by irq notify flag*/
	/**/
    if (intrFlag == 1) 
	{
		intrFlag = 0;

		for (i = 0; i < 11; i++) 
		{
			tr_val[i] = FMSH_ReadReg(addrSrc, 0x180 + 4 * i);
		}
		
		printf("can_%d->rx(%d-%d): ", dev2->id, g_can0_recv_intr_flag, g_can1_recv_intr_flag);
		g_can0_recv_intr_flag = 0;
		g_can1_recv_intr_flag = 0;
		for (i = 0; i < 11; i++)
		{	  
			printf("%02X ", tr_val[i]);
		}
		printf("\n\n");
		
		
		for (i = 0; i < 11; i++) 
		{ 
	        if (dev2->id == 0) 
			{
                if (tr_val[i] != g_can0rbuf[i])
                return 1;
	        }
	        else if (dev2->id == 1) 
			{
                if (tr_val[i] != g_can1rbuf[i])
                return 1;
	        }
		}
    }
    else
    {         
    	return 1;
    }
#endif

		
#if 1  /* remote_frame*/

    intrFlag = 0;
    FCanPs_standardFrameTransmit(dev1, 0x55, buf, 8, remote_frame);
    FCanPs_transmissionRequest(dev1);

	/*/////////////////////////////////////*/
	/**/
	printf("can_%d->tx: ", dev1->id);
	for (i = 0; i < 8; i++)
	{     
		printf("%02X ", buf[i]);
	}
	printf("\n");
	/**/
	/*/////////////////////////////////////*/
	
    timeout = 0x10000;
    while ((FCanPs_getTransmissionCompleteStatus(dev1) == 0)&&(timeout--));
	
    if (timeout == 0)
    {        
    	return 1;
    }
	
    if (dev2->id == 0)
    {        
    	intrFlag = g_can0_recv_intr_flag;		
	}
    else if(dev2->id == 1)
    {        
    	intrFlag = g_can1_recv_intr_flag;
	}
	
    if (intrFlag == 1) 
	{
		intrFlag = 0;

		for (i=0; i<11; i++) 
		{
			tr_val[i] = FMSH_ReadReg(addrSrc, 0x180 + 4 * i);
		}

		/*////////////////////////////////////*/
		/**/
		printf("can_%d->rx(%d-%d): ", dev2->id, g_can0_recv_intr_flag, g_can1_recv_intr_flag);
		g_can0_recv_intr_flag = 0;
		g_can1_recv_intr_flag = 0;
		for (i = 0; i < 11; i++)
		{	  
			printf("%02X ", tr_val[i]);
		}
		printf("\n\n");
		/**/
		/*////////////////////////////////////*/
		
		for (i=0; i<3; i++) 
		{
			if (dev2->id == 0) 
			{
				if(tr_val[i] != g_can0rbuf[i])
				return 1;
			}
			else if (dev2->id == 1)
			{
				if(tr_val[i] != g_can1rbuf[i])
				return 1;
			}
		}
		
    }
    else
    {     
    	return 1;
    }
#endif


#if 1  /* data_frame*/

    intrFlag = 0;
    FCanPs_extendedFrameTransmit(dev1, 0x55, buf, 8, data_frame);
    FCanPs_transmissionRequest(dev1);
	
	/*/////////////////////////////////////*/
	/**/
	printf("can_%d->tx: ", dev1->id);
	for (i = 0; i < 8; i++)
	{     
		printf("%02X ", buf[i]);
	}
	printf("\n");
	/**/
	/*/////////////////////////////////////*/
	
    timeout = 0x10000;
    while ((FCanPs_getTransmissionCompleteStatus(dev1) == 0)&&(timeout--));
	
    if (timeout == 0)
    {       
    	return 1;
	}
	
    if (dev2->id == 0)
    {        
		intrFlag = g_can0_recv_intr_flag;
	}
    else if (dev2->id == 1)
    {        
    	intrFlag = g_can1_recv_intr_flag;
	}
	
    if (intrFlag == 1) 
	{
		intrFlag = 0;

		for (i = 0; i < 11; i++) 
		{
			tr_val[i] = FMSH_ReadReg(addrSrc, 0x180 + 4 * i);/*read tx buffer data*/
		}
		
		/*////////////////////////////////////*/
		/**/
		printf("can_%d->rx(%d-%d): ", dev2->id, g_can0_recv_intr_flag, g_can1_recv_intr_flag);
		g_can0_recv_intr_flag = 0;
		g_can1_recv_intr_flag = 0;
		for (i = 0; i < 11; i++)
		{	  
			printf("%02X ", tr_val[i]);
		}
		printf("\n\n");
		/**/
		/*////////////////////////////////////*/
		
		for (i = 0; i < 11; i++) 
		{
			if (dev2->id == 0)
			{
				if(tr_val[i] != g_can0rbuf[i])/*read rx buffer data, compare*/
				return 1;
			}
			else if (dev2->id == 1)
			{
				if(tr_val[i] != g_can1rbuf[i])/*read rx buffer data, compare*/
				return 1;
			}
		}
    }
    else
    {        
    	return 1;
    }
#endif


#if 1  /* remote_frame*/

    intrFlag = 0;
    FCanPs_extendedFrameTransmit(dev1, 0x55, buf, 8, remote_frame);
    FCanPs_transmissionRequest(dev1);
	
	/*/////////////////////////////////////*/
	/**/
	printf("can_%d->tx: ", dev1->id);
	for (i = 0; i < 8; i++)
	{     
		printf("%02X ", buf[i]);
	}
	printf("\n");
	/**/
	/*/////////////////////////////////////*/
	
    timeout = 0x10000;
    while ((FCanPs_getTransmissionCompleteStatus(dev1) == 0)&&(timeout--));
	
    if (timeout == 0)
    {        
    	return 1;
    }
	
    if (dev2->id == 0)
    {        
    	intrFlag = g_can0_recv_intr_flag;
	}
    else if (dev2->id == 1)
    {        
    	intrFlag = g_can1_recv_intr_flag;
	}
	
    if (intrFlag == 1) 
	{
		intrFlag = 0;

		for (i = 0; i < 11; i++) 
		{
			tr_val[i] = FMSH_ReadReg(addrSrc, 0x180 + 4 * i);
		}
		
		/*////////////////////////////////////*/
		/**/
		printf("can_%d->rx(%d-%d): ", dev2->id, g_can0_recv_intr_flag, g_can1_recv_intr_flag);
		g_can0_recv_intr_flag = 0;
		g_can1_recv_intr_flag = 0;
		
		for (i = 0; i < 11; i++)
		{	  
			printf("%02X ", tr_val[i]);
		}
		printf("\n\n");
		/**/
		/*////////////////////////////////////*/
		
		for (i = 0; i < 3; i++) 
		{
			if (dev2->id == 0) 
			{
				if (tr_val[i] != g_can0rbuf[i])
					return 1;
			}
			else if (dev2->id == 1) 
			{
				if (tr_val[i] != g_can1rbuf[i])
				return 1;
			}
		}
    }
    else
   	{          
   		return 1;  
    }        
#endif

	/**/
	/* disable slcr loop mode*/
	/**/
	FSlcrPS_setCanLoop(0);        
    return 0;
}

/****************************************************************************/
/**
*
* This function communicte messages between g_CAN0 & g_CAN1.
*
* @param    None.
*
* @return   0 if successful, otherwise 1.
*
* @note     None.
*
****************************************************************************/
/*
can0 --> tx --> can1(rx) or can1 --> tx --> can0(rx), by slcr loop
*/
u8 can02can1DataLoop(void)
{
    u8 ret = 0;
	
#if 1
	/**/
	/* MIO init or fsbl setup*/
	/**/
	FMSH_WriteReg(FPS_SLCR_BASEADDR, 0x008, 0xDF0D767BU); /*unlock	   */
	
	FMSH_WriteReg(FPS_SLCR_BASEADDR, 0x304, 0x3); /* CAN1/0_APB_CLKACT: 1-enable	*/
	
	FMSH_WriteReg(FPS_SLCR_BASEADDR, 0x768, 0x00001621U);/* MIO[26] g_CAN0 RX		  */
	FMSH_WriteReg(FPS_SLCR_BASEADDR, 0x76C, 0x00001620U);/* MIO[27] g_CAN0 TX	 */
	FMSH_WriteReg(FPS_SLCR_BASEADDR, 0x004, 0xDF0D767BU);/*lock    */
		
	FMSH_WriteReg(FPS_SLCR_BASEADDR, 0x008, 0xDF0D767BU);/*unlock		*/
	FMSH_WriteReg(FPS_SLCR_BASEADDR, 0x7C0, 0x00001620U);/* MIO[48] g_CAN1 TX		  */
	FMSH_WriteReg(FPS_SLCR_BASEADDR, 0x7C4, 0x00001621U);/* MIO[49] g_CAN1 RX  */
	FMSH_WriteReg(FPS_SLCR_BASEADDR, 0x004, 0xDF0D767BU);/*lock 			 */
#endif

	/**/
	/* can0-->tx-->can1(rx)*/
	/**/
	printf("\n can0-->tx-->can1(rx): \n\n");
    ret |= can_can02Can1DataTrans(&g_CAN0, &g_CAN1);
	
	/**/
	/* can1-->tx-->can0(rx)*/
	/**/
	printf("\n can1-->tx-->can0(rx): \n\n");
    ret |= can_can02Can1DataTrans(&g_CAN1, &g_CAN0);

    return ret;
}

int test_can_loop_slcr(void)
{
	return can02can1DataLoop();
}


/*
log:
====

-> test_can_loop_slcr

 can0-->tx-->can1(rx): 
 
vxbIntConnect(0xbb0870,56,0x21dd70,0x0)
vxbIntCtlrMatch(): intCtlr 0xbac770
vxbIntConnect(0xbb0870,77,0x21de7c,0x0)
vxbIntCtlrMatch(): intCtlr 0xbac770
can_0->tx: 00 01 02 03 04 05 06 07 
can_1->rx(0-1): 08 0A A0 00 01 02 03 04 05 06 07 

can_0->tx: 00 01 02 03 04 05 06 07 
can_1->rx(0-1): 48 0A B0 00 01 02 03 04 05 06 07 

can_0->tx: 00 01 02 03 04 05 06 07 
can_1->rx(0-1): 88 00 00 02 A8 00 01 02 03 04 05 

can_0->tx: 00 01 02 03 04 05 06 07 
can_1->rx(0-1): C8 00 00 02 AC 00 01 02 03 04 05 


 can1-->tx-->can0(rx): 
 
can_1->tx: 01 02 03 04 05 06 07 08 
can_0->rx(1-0): 08 0A A0 01 02 03 04 05 06 07 08 

can_1->tx: 01 02 03 04 05 06 07 08 
can_0->rx(1-0): 48 0A B0 01 02 03 04 05 06 07 08 

can_1->tx: 01 02 03 04 05 06 07 08 
can_0->rx(1-0): 88 00 00 02 A8 01 02 03 04 05 06 

can_1->tx: 01 02 03 04 05 06 07 08 
can_0->rx(1-0): C8 00 00 02 AC 01 02 03 04 05 06 

value = 0 = 0x0
-> test_can_loop_slcr

 can0-->tx-->can1(rx): 
 
can_0->tx: 02 03 04 05 06 07 08 09 
can_1->rx(0-1): 08 0A A0 02 03 04 05 06 07 08 09 

can_0->tx: 02 03 04 05 06 07 08 09 
can_1->rx(0-1): 48 0A B0 02 03 04 05 06 07 08 09 

can_0->tx: 02 03 04 05 06 07 08 09 
can_1->rx(0-1): 88 00 00 02 A8 02 03 04 05 06 07 

can_0->tx: 02 03 04 05 06 07 08 09 
can_1->rx(0-1): C8 00 00 02 AC 02 03 04 05 06 07 


 can1-->tx-->can0(rx): 
 
can_1->tx: 03 04 05 06 07 08 09 0A 
can_0->rx(1-0): 08 0A A0 03 04 05 06 07 08 09 0A 

can_1->tx: 03 04 05 06 07 08 09 0A 
can_0->rx(1-0): 48 0A B0 03 04 05 06 07 08 09 0A 

can_1->tx: 03 04 05 06 07 08 09 0A 
can_0->rx(1-0): 88 00 00 02 A8 03 04 05 06 07 08 

can_1->tx: 03 04 05 06 07 08 09 0A 
can_0->rx(1-0): C8 00 00 02 AC 03 04 05 06 07 08 

value = 0 = 0x0
-> 
*/
#endif


