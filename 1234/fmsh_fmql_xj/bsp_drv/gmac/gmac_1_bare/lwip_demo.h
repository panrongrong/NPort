/******************************************************************************
*
* Copyright (C) 2018 - 2028 FMSH, Inc.  All rights reserved.
*
******************************************************************************/
/*****************************************************************************/
/**
* @file  lwip_demo.h
*
* @note		None.
*
* MODIFICATION HISTORY:
*
*<pre>
* Ver   Who  Date     Changes
* ----- ---- -------- ---------------------------------------------
* 0.01   Hansen Yang  12/24/2018  First Release
*</pre>
******************************************************************************/

#define RCV_POLL 1   /*1 : close irq, use polling to rcv. 0 : open irq*/
#define LWIP_GMAC_ID    FPAR_GMACPS_0_DEVICE_ID /* GMAC 0*/
#define FMQL_LWIP_DEBUG 0
