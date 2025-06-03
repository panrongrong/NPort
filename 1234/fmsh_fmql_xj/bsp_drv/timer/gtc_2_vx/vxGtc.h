/* vxSpi.h - vxSpi driver header */

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
01a, 19May20, jc  written.
*/

#ifndef __INC_VX_GTC_H__
#define __INC_VX_GTC_H__



#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define  VX_GTC_CFG_BASE   (0xE0008000)  /* arm generic timer(GTC) */


/*
gtc ctrl reg
*/
#define GTC_REG_CNTCR     0x00
#define GTC_REG_CNTSR     0x04
#define GTC_REG_CNTCV_L   0x08
#define GTC_REG_CNTCV_H   0x0C



/*
GTC ctrl
*/
typedef struct _t_gtc_ctrl_
{
	int  ctrl_x;
	
	UINT16 devId;				/**< Unique ID of device */
	UINT32 cfgBaseAddr;		    /**< APB Base address of the device */
	UINT32 sysClk;				/**< Input clock frequency */
	

	UINT32 devIP_ver;
    UINT32 devIP_type;      /* identify peripheral types:  */

	int status;
} vxT_GTC_CTRL;


typedef struct _t_gtc_
{
	int gtc_x;
	
	vxT_GTC_CTRL * pGtcCtrl;
	
} vxT_GTC;
	

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __INC_VX_GTC_H__ */


