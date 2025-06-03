/* vxSdmmc.c - fmsh 7020/7045 sdmmc driver */

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
01a, 22Nov19, jc  written.
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

#include "vxSdmmc.h"


#ifdef ENABLE_FATFS_SDMMC
#include "./vxFatFS2/ff.h"
#include "./vxFatFS2/diskio.h"
#endif

/*
defines 
*/
#if 1
#undef SDMMC_DBG
#ifdef SDMMC_DBG

#define VX_DBG(string, a, b, c, d, e, f)                    \
        if (_func_logMsg != NULL)                           \
           printf(string, a, b, c, d, e, f)         /* (* _func_logMsg)(string, a, b, c, d, e, f) */
#else
#define VX_DBG(string, a, b, c, d, e, f)
#endif

/* error info log */
#define VX_DBG2(string, a, b, c, d, e, f) printf(string, a, b, c, d, e, f)
#endif



#undef  FMQL_SDMMC_0_BASE
#undef  FMQL_SDMMC_1_BASE
#define  FMQL_SDMMC_0_BASE     (VX_SDMMC_0_BASE)
#define  FMQL_SDMMC_1_BASE     (VX_SDMMC_1_BASE)


static int gSdmmc_x = SDMMC_CTRL_0;
static UINT32 gSdmmc_base = FMQL_SDMMC_0_BASE;

/* sdmmc_0 */
static vxT_SDMMC_CTRL vxSdmmc_Ctrl_0 = {0};
static vxT_SDMMC_CARD vxSdmmc_Card_0 = {0};
static vxT_SDMMC_CMD  vxSdmmc_Cmd_0  = {0};

static vxT_SDMMC vxSdmmc_0 = {0};
vxT_SDMMC * g_pSdmmc0 = (vxT_SDMMC *)(&vxSdmmc_0.sdmmc_x);

/* sdmmc_1 */
static vxT_SDMMC_CTRL vxSdmmc_Ctrl_1 = {0};
static vxT_SDMMC_CARD vxSdmmc_Card_1 = {0};
static vxT_SDMMC_CMD  vxSdmmc_Cmd_1  = {0};

static vxT_SDMMC vxSdmmc_1 = {0};
vxT_SDMMC * g_pSdmmc1 = (vxT_SDMMC *)(&vxSdmmc_1.sdmmc_x);


static const UINT32 tran_exp[] = 
{
    100000,         1000000,                10000000,       100000000,
    0,              0,              0,              0
};

static const UINT8 tran_mant[] =
{
    0,      10,     12,     13,     15,     20,     25,     30,
    35,     40,     45,     50,     55,     60,     70,     80,
};

static const UINT32 taac_exp[] = 
{
	1,	10,	100,	1000,	10000,	100000,	1000000, 10000000,
};

static const UINT32 taac_mant[] = 
{
	0,	10,	12,	13,	15,	20,	26,	30,
	35,	40,	45,	52,	55,	60,	70,	80,
};




/* 
forward declarations 
*/



/* 
locals 
*/



/* 
externs 
*/


/*
functions
*/
int UNSTUFF_BITS(u32 *rsp_buf, unsigned int start, unsigned int size)
{
	const unsigned int __mask = (1 << (size)) - 1;
	const int __off = /*3 - */((start) / 32);	 
	const int __shft = (start) & 31;	 
	unsigned int __res;  
	__res = rsp_buf[__off] >> __shft; 
	if ((size) + __shft >= 32)	 
		__res |= rsp_buf[__off+1] << (32 - __shft);	
	return (__res & __mask);
}


#if 1

void sdmmcCtrl_WrReg32(vxT_SDMMC* pSdmmc, UINT32 offset, UINT32 value)
{
	UINT32 tmp32 = pSdmmc->pSdmmcCtrl->baseAddr;
	FMQL_WRITE_32((tmp32 + offset), value);
	return;
}

UINT32 sdmmcCtrl_RdReg32(vxT_SDMMC* pSdmmc, UINT32 offset)
{
	UINT32 tmp32 = pSdmmc->pSdmmcCtrl->baseAddr;
	return FMQL_READ_32(tmp32 + offset);
}

/*****************************************************************************
*
* @description
* This function is used to check if a command has been accepted by the card.
*
* @param
*           pSdmmc is the pointer to a sd card device.
*
* @return
*           Returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note     NA.
*
*****************************************************************************/
static int sdmmcCtrl_wait_StartCmd_Ok(vxT_SDMMC* pSdmmc)
{
    int timeout = SDMMC_POLL_TIMEOUT;
    UINT32 tmp32 = 0;
    
    do
	{
        delay_1us();
		
		tmp32 = sdmmcCtrl_RdReg32(pSdmmc, SDMMC_CMD_VAL) & START_CMD;
		
        timeout--;
        if (timeout <= 0)
        {
            return (-FMSH_ETIME);
        }
    } while (tmp32);

    return 0;
}


int sdmmcCtrl_Get_BusWidth    (vxT_SDMMC* pSdmmc)
{
	UINT32       buswidth;
	/*
	31:16 card_width 0 One bit per card indicates if card is 8-bit:
		0 – Non 8-bit mode
		1 – 8-bit mode
		Bit[31] corresponds to card[15]; bit[16] corresponds to card[0].
		
	15:0 card_width 0 One bit per card indicates if card is 1-bit or 4-bit:
		0 – 1-bit mode
		1 – 4-bit mode
		Bit[15] corresponds to card[15], bit[0] corresponds to card[0].
		Only NUM_CARDS*2 number of bits are implemented.
	*/ 
	buswidth = sdmmcCtrl_RdReg32 (pSdmmc, SDMMC_CTYPE);

	if (buswidth & 0x10000)
	{
		return 8; /* 8 bit mode*/
	}
	else if (buswidth & 0x01)
	{
		return 4; /* 4 bit mode*/
	}
	else if ((buswidth & 0x01) == 0x00)
	{
		return 1; /* 1 bit mode*/
	}
	else
	{
		return 0; /* unknow bit mode*/
	}
}

#endif


#if 1

int sdmmcCard_decode_CSD(vxT_SDMMC* pSdmmc)
{
	vxT_SDMMC_CARD * pCard = pSdmmc->pSdmmcCard;
	
	UINT32 e, m;
	UINT32 * pResp;	
	
	pResp = pCard->raw_csd;

	if (pCard->card_type == SD)
	{
		pCard->info_csd.csd_struct = UNSTUFF_BITS(pResp, 126, 2);

		m = UNSTUFF_BITS(pResp, 115, 4);
		e = UNSTUFF_BITS(pResp, 112, 3);		
		pCard->info_csd.taac_ns	 = (taac_exp[e] * taac_mant[m] + 9) / 10;
		pCard->info_csd.taac_clks	 = UNSTUFF_BITS(pResp, 104, 8) * 100;
		
		m = UNSTUFF_BITS(pResp, 99, 4);
		e = UNSTUFF_BITS(pResp, 96, 3);
		pCard->info_csd.max_dtr	  = tran_exp[e] * tran_mant[m]/10;
		pCard->info_csd.cmdclass	  = UNSTUFF_BITS(pResp, 84, 12);
		
		if (pCard->highCapacity == 0)
		{
			e = UNSTUFF_BITS(pResp, 47, 3);
	/*		m = UNSTUFF_BITS(pResp, 62, 12);*/
			m = UNSTUFF_BITS(pResp, 62, 2) | (UNSTUFF_BITS(pResp, 64, 10) << 2);
			pCard->info_csd.blockNR	  = (1 + m) << (e + 2);
		}
		else if(pCard->highCapacity == 1)
		{
			e = UNSTUFF_BITS(pResp, 48, 22);
			pCard->info_csd.blockNR = (e + 1)<<10;
		}
		
		pCard->info_csd.read_blkbits = UNSTUFF_BITS(pResp, 80, 4);
		pCard->info_csd.read_partial = UNSTUFF_BITS(pResp, 79, 1);
		pCard->info_csd.write_misalign = UNSTUFF_BITS(pResp, 78, 1);
		pCard->info_csd.read_misalign = UNSTUFF_BITS(pResp, 77, 1);
		pCard->info_csd.r2w_factor = UNSTUFF_BITS(pResp, 26, 3);
		pCard->info_csd.write_blkbits = UNSTUFF_BITS(pResp, 22, 4);
		pCard->info_csd.write_partial = UNSTUFF_BITS(pResp, 21, 1);
		
	}
	else  /* mmc / eMMC */
	{
		pCard->info_csd.csd_struct = UNSTUFF_BITS(pResp, 126, 2);
		if (pCard->info_csd.csd_struct == 0)
		{
			return -FMSH_EINVAL;
		}

		pCard->info_csd.mmca_vsn	 = UNSTUFF_BITS(pResp, 122, 4);
		
		m = UNSTUFF_BITS(pResp, 115, 4);
		e = UNSTUFF_BITS(pResp, 112, 3);
		pCard->info_csd.taac_ns	 = (taac_exp[e] * taac_mant[m] + 9) / 10;
		pCard->info_csd.taac_clks	 = UNSTUFF_BITS(pResp, 104, 8) * 100;

		m = UNSTUFF_BITS(pResp, 99, 4);
		e = UNSTUFF_BITS(pResp, 96, 3);
		pCard->info_csd.max_dtr	  = tran_exp[e] * tran_mant[m]/10;		
		pCard->info_csd.cmdclass	  = UNSTUFF_BITS(pResp, 84, 12);

		e = UNSTUFF_BITS(pResp, 47, 3);
		m = UNSTUFF_BITS(pResp, 62, 2) | (UNSTUFF_BITS(pResp, 64, 10) << 2);
		pCard->info_csd.blockNR	  = (1 + m) << (e + 2);
		
		pCard->info_csd.read_blkbits = UNSTUFF_BITS(pResp, 80, 4);
		pCard->info_csd.read_partial = UNSTUFF_BITS(pResp, 79, 1);
		pCard->info_csd.write_misalign = UNSTUFF_BITS(pResp, 78, 1);
		pCard->info_csd.read_misalign = UNSTUFF_BITS(pResp, 77, 1);
		pCard->info_csd.r2w_factor = UNSTUFF_BITS(pResp, 26, 3);
		pCard->info_csd.write_blkbits = UNSTUFF_BITS(pResp, 22, 4);
		pCard->info_csd.write_partial = UNSTUFF_BITS(pResp, 21, 1);
	}
	
	return 0;
}


int sdmmcCard_decode_CSD_MMC(vxT_SDMMC* pSdmmc)
{
	vxT_SDMMC_CARD * pCard = pSdmmc->pSdmmcCard;
		
	UINT32 e, m;
	UINT32 * pResp;	
	
	pResp = pCard->raw_csd;
	
	/* mmc / eMMC */
	{
		pCard->info_csd.csd_struct = UNSTUFF_BITS(pResp, 126, 2);
		if (pCard->info_csd.csd_struct == 0)
		{
			return -FMSH_EINVAL;
		}

		pCard->info_csd.mmca_vsn	 = UNSTUFF_BITS(pResp, 122, 4);
		
		m = UNSTUFF_BITS(pResp, 115, 4);
		e = UNSTUFF_BITS(pResp, 112, 3);
		pCard->info_csd.taac_ns	 = (taac_exp[e] * taac_mant[m] + 9) / 10;
		pCard->info_csd.taac_clks	 = UNSTUFF_BITS(pResp, 104, 8) * 100;

		m = UNSTUFF_BITS(pResp, 99, 4);
		e = UNSTUFF_BITS(pResp, 96, 3);
		pCard->info_csd.max_dtr	  = tran_exp[e] * tran_mant[m]/10;		
		pCard->info_csd.cmdclass	  = UNSTUFF_BITS(pResp, 84, 12);

		e = UNSTUFF_BITS(pResp, 47, 3);
		m = UNSTUFF_BITS(pResp, 62, 2) | (UNSTUFF_BITS(pResp, 64, 10) << 2);
		pCard->info_csd.blockNR	  = (1 + m) << (e + 2);
		
		pCard->info_csd.read_blkbits = UNSTUFF_BITS(pResp, 80, 4);
		pCard->info_csd.read_partial = UNSTUFF_BITS(pResp, 79, 1);
		pCard->info_csd.write_misalign = UNSTUFF_BITS(pResp, 78, 1);
		pCard->info_csd.read_misalign = UNSTUFF_BITS(pResp, 77, 1);
		pCard->info_csd.r2w_factor = UNSTUFF_BITS(pResp, 26, 3);
		pCard->info_csd.write_blkbits = UNSTUFF_BITS(pResp, 22, 4);
		pCard->info_csd.write_partial = UNSTUFF_BITS(pResp, 21, 1);
	}
	
	return 0;
}

int sdmmcCard_decode_CID(vxT_SDMMC* pSdmmc)
{
	vxT_SDMMC_CARD * pSdmmcCard = pSdmmc->pSdmmcCard;
	vxT_SDMMC_CARD * pCard = pSdmmc->pSdmmcCard;
	
	UINT32 * pResp;
	
	pResp = pCard->raw_cid;
	
	if (pCard->card_type == SD)
	{
		/*
		 * SD doesn't currently have a version field so we will
		 * have to assume we can parse this.
		 */
		pSdmmcCard->info_cid.manfid		= UNSTUFF_BITS(pResp, 120, 8);
		pSdmmcCard->info_cid.oemid			= UNSTUFF_BITS(pResp, 104, 16);
		pSdmmcCard->info_cid.prod_name[0]		= UNSTUFF_BITS(pResp, 96, 8);
		pSdmmcCard->info_cid.prod_name[1]		= UNSTUFF_BITS(pResp, 88, 8);
		pSdmmcCard->info_cid.prod_name[2]		= UNSTUFF_BITS(pResp, 80, 8);
		pSdmmcCard->info_cid.prod_name[3]		= UNSTUFF_BITS(pResp, 72, 8);
		pSdmmcCard->info_cid.prod_name[4]		= UNSTUFF_BITS(pResp, 64, 8);
		pSdmmcCard->info_cid.hwrev			= UNSTUFF_BITS(pResp, 60, 4);
		pSdmmcCard->info_cid.fwrev			= UNSTUFF_BITS(pResp, 56, 4);
		pSdmmcCard->info_cid.serial		= UNSTUFF_BITS(pResp, 24, 32);
		pSdmmcCard->info_cid.year			= UNSTUFF_BITS(pResp, 12, 8);
		pSdmmcCard->info_cid.month			= UNSTUFF_BITS(pResp, 8, 4);

		pSdmmcCard->info_cid.year += 2000; /* SD cards year offset */
	}
	else   /* mmc / eMMC */
	{
		/*
		 * The selection of the format here is based upon published
		 * specs from sandisk and from what people have reported.
		 */
		switch (pSdmmcCard->info_csd.mmca_vsn)
		{
			case CSD_SPEC_VER_0: /* MMC v1.0 - v1.2 */
			case CSD_SPEC_VER_1: /* MMC v1.4 */
				pSdmmcCard->info_cid.manfid	= UNSTUFF_BITS(pResp, 104, 24);
				pSdmmcCard->info_cid.prod_name[0]	= UNSTUFF_BITS(pResp, 96, 8);
				pSdmmcCard->info_cid.prod_name[1]	= UNSTUFF_BITS(pResp, 88, 8);
				pSdmmcCard->info_cid.prod_name[2]	= UNSTUFF_BITS(pResp, 80, 8);
				pSdmmcCard->info_cid.prod_name[3]	= UNSTUFF_BITS(pResp, 72, 8);
				pSdmmcCard->info_cid.prod_name[4]	= UNSTUFF_BITS(pResp, 64, 8);
				pSdmmcCard->info_cid.prod_name[5]	= UNSTUFF_BITS(pResp, 56, 8);
				pSdmmcCard->info_cid.prod_name[6]	= UNSTUFF_BITS(pResp, 48, 8);
				pSdmmcCard->info_cid.hwrev		= UNSTUFF_BITS(pResp, 44, 4);
				pSdmmcCard->info_cid.fwrev		= UNSTUFF_BITS(pResp, 40, 4);
				pSdmmcCard->info_cid.serial	= UNSTUFF_BITS(pResp, 16, 24);
				pSdmmcCard->info_cid.month		= UNSTUFF_BITS(pResp, 12, 4);
				pSdmmcCard->info_cid.year		= UNSTUFF_BITS(pResp, 8, 4) + 1997;
				break;

			case CSD_SPEC_VER_2: /* MMC v2.0 - v2.2 */
			case CSD_SPEC_VER_3: /* MMC v3.1 - v3.3 */
			case CSD_SPEC_VER_4: /* MMC v4 */
				pSdmmcCard->info_cid.manfid	= UNSTUFF_BITS(pResp, 120, 8);
				pSdmmcCard->info_cid.oemid		= UNSTUFF_BITS(pResp, 104, 16);
				pSdmmcCard->info_cid.prod_name[0]	= UNSTUFF_BITS(pResp, 96, 8);
				pSdmmcCard->info_cid.prod_name[1]	= UNSTUFF_BITS(pResp, 88, 8);
				pSdmmcCard->info_cid.prod_name[2]	= UNSTUFF_BITS(pResp, 80, 8);
				pSdmmcCard->info_cid.prod_name[3]	= UNSTUFF_BITS(pResp, 72, 8);
				pSdmmcCard->info_cid.prod_name[4]	= UNSTUFF_BITS(pResp, 64, 8);
				pSdmmcCard->info_cid.prod_name[5]	= UNSTUFF_BITS(pResp, 56, 8);
				pSdmmcCard->info_cid.serial	= UNSTUFF_BITS(pResp, 16, 32);
				pSdmmcCard->info_cid.month		= UNSTUFF_BITS(pResp, 12, 4);
				pSdmmcCard->info_cid.year		= UNSTUFF_BITS(pResp, 8, 4) + 1997;
				break;

			default:
				return -FMSH_EINVAL;
		}
	}
	
	return 0;
}

int sdmmcCard_decode_CID_MMC(vxT_SDMMC* pSdmmc)
{
	vxT_SDMMC_CARD * pSdmmcCard = pSdmmc->pSdmmcCard;
	vxT_SDMMC_CARD * pCard = pSdmmcCard;
	
	UINT32 * pResp;
	
	pResp = pCard->raw_cid;
	
	/* mmc / eMMC */
	{
		/*
		 * The selection of the format here is based upon published
		 * specs from sandisk and from what people have reported.
		 */
		switch (pSdmmcCard->info_csd.mmca_vsn)
		{
			case CSD_SPEC_VER_0: /* MMC v1.0 - v1.2 */
			case CSD_SPEC_VER_1: /* MMC v1.4 */
				pSdmmcCard->info_cid.manfid	= UNSTUFF_BITS(pResp, 104, 24);
				pSdmmcCard->info_cid.prod_name[0]	= UNSTUFF_BITS(pResp, 96, 8);
				pSdmmcCard->info_cid.prod_name[1]	= UNSTUFF_BITS(pResp, 88, 8);
				pSdmmcCard->info_cid.prod_name[2]	= UNSTUFF_BITS(pResp, 80, 8);
				pSdmmcCard->info_cid.prod_name[3]	= UNSTUFF_BITS(pResp, 72, 8);
				pSdmmcCard->info_cid.prod_name[4]	= UNSTUFF_BITS(pResp, 64, 8);
				pSdmmcCard->info_cid.prod_name[5]	= UNSTUFF_BITS(pResp, 56, 8);
				pSdmmcCard->info_cid.prod_name[6]	= UNSTUFF_BITS(pResp, 48, 8);
				pSdmmcCard->info_cid.hwrev		= UNSTUFF_BITS(pResp, 44, 4);
				pSdmmcCard->info_cid.fwrev		= UNSTUFF_BITS(pResp, 40, 4);
				pSdmmcCard->info_cid.serial	= UNSTUFF_BITS(pResp, 16, 24);
				pSdmmcCard->info_cid.month		= UNSTUFF_BITS(pResp, 12, 4);
				pSdmmcCard->info_cid.year		= UNSTUFF_BITS(pResp, 8, 4) + 1997;
				break;

			case CSD_SPEC_VER_2: /* MMC v2.0 - v2.2 */
			case CSD_SPEC_VER_3: /* MMC v3.1 - v3.3 */
			case CSD_SPEC_VER_4: /* MMC v4 */
				pSdmmcCard->info_cid.manfid	= UNSTUFF_BITS(pResp, 120, 8);
				pSdmmcCard->info_cid.oemid		= UNSTUFF_BITS(pResp, 104, 16);
				pSdmmcCard->info_cid.prod_name[0]	= UNSTUFF_BITS(pResp, 96, 8);
				pSdmmcCard->info_cid.prod_name[1]	= UNSTUFF_BITS(pResp, 88, 8);
				pSdmmcCard->info_cid.prod_name[2]	= UNSTUFF_BITS(pResp, 80, 8);
				pSdmmcCard->info_cid.prod_name[3]	= UNSTUFF_BITS(pResp, 72, 8);
				pSdmmcCard->info_cid.prod_name[4]	= UNSTUFF_BITS(pResp, 64, 8);
				pSdmmcCard->info_cid.prod_name[5]	= UNSTUFF_BITS(pResp, 56, 8);
				pSdmmcCard->info_cid.serial	= UNSTUFF_BITS(pResp, 16, 32);
				pSdmmcCard->info_cid.month		= UNSTUFF_BITS(pResp, 12, 4);
				pSdmmcCard->info_cid.year		= UNSTUFF_BITS(pResp, 8, 4) + 1997;
				break;

			default:
				return -FMSH_EINVAL;
		}
	}
	
	return 0;
}

UINT32 sdmmcCard_Get_CardSize_KB(vxT_SDMMC * pSdmmc)
{
	vxT_SDMMC_CARD * pSdmmcCard = pSdmmc->pSdmmcCard;
	
    if (pSdmmcCard->capacity == 0xFFFFFFFF)
    {
        return pSdmmcCard->info_csd.blockNR / 2;
    }
    
    return pSdmmcCard->capacity / 1024;
}

/*****************************************************************************
*
* @description
* This function sets the bus width to the card.
*
* @param
*           pSdmmc is the pointer to a sd card device.
*           bus_width is bus width.
*
* @return
*           Returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note     NA.
*
*****************************************************************************/
int sdmmcCard_Set_BusWidth(vxT_SDMMC* pSdmmc, UINT32 bus_width)
{
	vxT_SDMMC_CARD * pSdmmcCard = pSdmmc->pSdmmcCard;
	
    int ret;
    int retry = 5;
    
    if (pSdmmcCard->card_type == SD)
    {
        while (retry)
        {
            ret = sdmmc_sdACMD6(pSdmmc, bus_width);
            if (ret < 0)
            {
                if (ret == (-FMSH_ENODEV))
                {
					VX_DBG2("SD ACMD6 failed! \n", 1,2,3,4,5,6);
                    return ret;
                }
                
                retry--;				
				VX_DBG("retry to SD set buswidth \n", 1,2,3,4,5,6);
            }
            else
            {
                break;
            }
        }
		
        if (ret < 0)
        {
            VX_DBG2("ACMD6 failed! \n", 1,2,3,4,5,6);
            return ret;
        }
    }
    else
    {
        if (pSdmmcCard->info_csd.mmca_vsn >= CSD_SPEC_VER_4)
        {
            while (retry)
            {
                ret = sdmmc_cmd6_mmc(pSdmmc, MMC_4_BIT_BUS_ARG);
                if (ret < 0)
                {
                    if (ret == (-FMSH_ENODEV))
                    {
                    
						VX_DBG2("MMC ACMD6 failed! \n", 1,2,3,4,5,6);
                        return ret;
                    }
                    
                    retry--;					
					VX_DBG("retry to MMC set buswidth \n", 1,2,3,4,5,6);
                }
                else
                {
                    break;
                }
            }
			
            if (ret < 0)
            {
				VX_DBG2("ACMD6_mmc failed! \n", 1,2,3,4,5,6);
                return ret;
            }
        }
    }
	
    switch (bus_width)
    {       
    case SD_BUS_WIDTH_4:
        sdmmcCtrl_WrReg32(pSdmmc, SDMMC_CTYPE, 0x1);
    	break;  
	
    case SD_BUS_WIDTH_1:
        sdmmcCtrl_WrReg32(pSdmmc, SDMMC_CTYPE, 0);
    	break;  
	
    default:
        sdmmcCtrl_WrReg32(pSdmmc, SDMMC_CTYPE, 0);
    	break;
    }   

    return ret;
}

int sdmmcCard_Set_BusWidth_MMC(vxT_SDMMC* pSdmmc, UINT32 bus_width)
{
	vxT_SDMMC_CARD * pSdmmcCard = pSdmmc->pSdmmcCard;
	
    int ret;
    int retry = 5;
	
    if (pSdmmcCard->info_csd.mmca_vsn >= CSD_SPEC_VER_4)
    {
        while (retry)
        {
            ret = sdmmc_cmd6_mmc(pSdmmc, MMC_4_BIT_BUS_ARG);
            if (ret < 0)
            {
                if (ret == (-FMSH_ENODEV))
                {
                
					VX_DBG2("MMC ACMD6 failed! \n", 1,2,3,4,5,6);
                    return ret;
                }
                
                retry--;					
				VX_DBG("retry to MMC set buswidth \n", 1,2,3,4,5,6);
            }
            else
            {
                break;
            }
        }
		
        if (ret < 0)
        {
			VX_DBG2("ACMD6_mmc fail! \n", 1,2,3,4,5,6);
            return ret;
        }
		else
		{
			VX_DBG2("ACMD6_mmc ok! \n", 1,2,3,4,5,6);
		}
    }
	
    /*pSdmmcCard->setBuswidth(pSdmmc, bus_width);*/
    switch (bus_width)
    {       
    case SD_BUS_WIDTH_4:
        sdmmcCtrl_WrReg32(pSdmmc, SDMMC_CTYPE, 0x1);
    	break;  
	
    case SD_BUS_WIDTH_1:
        sdmmcCtrl_WrReg32(pSdmmc, SDMMC_CTYPE, 0);
    	break;  
	
    default:
        sdmmcCtrl_WrReg32(pSdmmc, SDMMC_CTYPE, 0);
    	break;
    }   

    return ret;
}


int sdmmcCard_Get_Info_CSD(vxT_SDMMC* pSdmmc) 
{
	vxT_SDMMC_CARD * pSdmmcCard = pSdmmc->pSdmmcCard;

	UINT32 * pResp;
	
	vxT_SDMMC_CARD * pCard = pSdmmcCard;
	
	pResp = pCard->raw_csd;
	
	/* read/write block length */
	pCard->read_blk_len	= 1 << pCard->info_csd.read_blkbits;
	pCard->write_blk_len = 1 << pCard->info_csd.write_blkbits;

	if (pCard->read_blk_len != pCard->write_blk_len)
	{
		return (-FMSH_EINVAL);
	}

	/* partial block read/write I/O support */
	if (pCard->info_csd.read_partial) 
		pCard->status |= MMC_READ_PART;
	
	if (pCard->info_csd.write_partial) 
		pCard->status |= MMC_WRITE_PART;

	/* calculate total card size in bytes */
    if (pCard->highCapacity == 1)
		pCard->capacity = 0xFFFFFFFF; /* SDHC/SDXC may larger than 32bits*/
	else
		pCard->capacity = pCard->info_csd.blockNR * pCard->read_blk_len;

	return 0;
}

/*****************************************************************************
*
* @description
* This function sets the block length to the card.
*
* @param
*           pSdmmc is the pointer to a sd card device.
*           len is block length in bytes.
*
* @return
*           Returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note     NA.
*
*****************************************************************************/
int sdmmcCard_Set_BlockSize(vxT_SDMMC* pSdmmc, int block_size)
{
    int ret = 0;
    int retry = 100;

    while (retry)
    {
        ret = sdmmc_cmd16(pSdmmc, block_size);
        if(ret < 0)
        {
            delay_us(10);
            retry--;
        }
        else
        {
            break;
        }
    }
    
    if (ret < 0)
    {    
    	VX_DBG2("Cmd 16 error, set block_size(%d) fail! \n", block_size, 2,3,4,5,6);
    }
	
    return ret;
}

/*****************************************************************************
*
* @description
* This function gets bus speed supported by card.
*
* @param
*           pSdmmc is the pointer to a sd card device.
*           pBuf is buffer to store function group support data 
*                   returned by card.
*
* @return
*           Returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note     NA.
*
*****************************************************************************/
int sdmmcCard_Get_BusSpeed(vxT_SDMMC* pSdmmc, UINT8 * pBuf)
{
    int ret;
    UINT32 arg;

    arg = SD_SWITCH_CMD_HS_GET;
    ret = sdmmc_cmd6_sd(pSdmmc, arg, pBuf);

    return ret;
}


int sdmmcCard_Get_Info_SCR(vxT_SDMMC* pSdmmc)
{
	vxT_SDMMC_CARD * pSdmmcCard = pSdmmc->pSdmmcCard;
	
    int ret;
	int i = 0;
    UINT32 scr_struct;

    memset(pSdmmcCard->raw_scr, 0, (8*sizeof(UINT8)));
	
    ret = sdmmc_sdACMD51(pSdmmc, (UINT8*)(&pSdmmcCard->raw_scr[0]));
    if (ret < 0)
    {
        return ret;
    }

    VX_DBG("scr is: 0x", 1,2,3,4,5,6);
	for (i=0; i<=7; i++)
	{
	    VX_DBG("%02X_", pSdmmcCard->raw_scr[i], 2,3,4,5,6);
	}
    VX_DBG(" \n", 1,2,3,4,5,6);

    scr_struct = (pSdmmcCard->raw_scr[0] >> 4) & 0xF;
    if (scr_struct != 0)
    {
        return -FMSH_EINVAL;
    }
	
    pSdmmcCard->info_scr.sd_spec   = pSdmmcCard->raw_scr[0] & 0xF;
    pSdmmcCard->info_scr.bus_width = pSdmmcCard->raw_scr[1] & 0xF;
    pSdmmcCard->info_scr.sd_spec3  =(pSdmmcCard->raw_scr[2] >> 7) & 0x1;
    
    return ret;
}

/*****************************************************************************
*
* @description
* This function sends command 8 to get the EXT_CSD register
* as a block of data, only used for MMC card 4.0.
*
* @param
*           pSdmmc is the pointer to a sd card device.
*
* @return
*           Returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note     NA.
*
*****************************************************************************/
int sdmmcCard_Get_ExtCsd_MMC(vxT_SDMMC* pSdmmc)
{
	vxT_SDMMC_CARD * pSdmmcCard = pSdmmc->pSdmmcCard;
	
    int ret;
	int retry = 5;
	
    if (pSdmmcCard->info_csd.mmca_vsn >= CSD_SPEC_VER_4)
    {
        /* get EXT CSD info */
        /*retry = 5;*/
        retry = 50;  /* about 8 times is ok*/
        
        while (retry)
        {
            memset(pSdmmcCard->ext_csd, 0, (512*sizeof(UINT8)));
			
            ret = sdmmc_cmd8_mmc(pSdmmc, pSdmmcCard->ext_csd);
            if (ret < 0)
            {
                retry--;
                if (retry <= 0)
                {    
                	printf("pSdmmcCard->ext_csd: 0x%X \n", pSdmmcCard->ext_csd);
                	return ret;
                }
            }
			else
            {
            	break;
			}
        }
    }

    return 0;
}

#endif

#if 1

int sdmmcSet_Clk_DataRate(vxT_SDMMC* pSdmmc, UINT32 rate)
{
	vxT_SDMMC_CTRL * pSdmmcCtrl = pSdmmc->pSdmmcCtrl;
	int ret = 0;
	UINT32 tmp32 = 0;
	
	int clk_div = 0, div_cnt = 0, sd_clk = 0;
	int timeout = 0;
	int ctrl_x = 0;
	
	/*rate = SDMMC_CLK_400_KHZ;*/
	ctrl_x = pSdmmcCtrl->ctrl_x;	
	sd_clk = pSdmmcCtrl->sysClkHz;   /* 100M Hz*/

	if (rate >= sd_clk)
	{
		clk_div = 0;
	}
	else
	{
		if (rate != 0)
		{
			/*clk_div = (sd_clk / (2 *rate) + 1);*/
			for (div_cnt=1; div_cnt<=SDMMC_MAX_DIV_CNT; div_cnt++)
			{
				if ((sd_clk / (2 * div_cnt) <= rate))
				{
					clk_div = div_cnt;
					break;
				}
			}
		}
	}

	/* yulei for test*/
	if (rate > SDMMC_CLK_400_KHZ)
	{
		clk_div += 1;
	}

	/**/
	/* disable clock */
	/**/
	sdmmcCtrl_WrReg32(pSdmmc, SDMMC_CLKENA, 0);
	sdmmcCtrl_WrReg32(pSdmmc, SDMMC_CLKSRC, 0);
	
	tmp32 = START_CMD + UPDATE_CLOCK_REG_ONLY + WAIT_PRVDATA_COMPLETE;
	sdmmcCtrl_WrReg32(pSdmmc, SDMMC_CMD_VAL, tmp32);
	ret = sdmmcCtrl_wait_StartCmd_Ok(pSdmmc);
	if (ret < 0)
	{
		VX_DBG2("sdmmc ctrl_%d: disable SDMMC_CLKENA fail, exit! \n", ctrl_x, 2,3,4,5,6);
		return (-FMSH_ETIME);
	}
	
	/**/
	/* set clock */
	/**/
	sdmmcCtrl_WrReg32(pSdmmc, SDMMC_CLKDIV, clk_div);
	tmp32 = START_CMD + UPDATE_CLOCK_REG_ONLY + WAIT_PRVDATA_COMPLETE;
	sdmmcCtrl_WrReg32(pSdmmc, SDMMC_CMD_VAL, tmp32);	
	ret = sdmmcCtrl_wait_StartCmd_Ok(pSdmmc);
	if (ret < 0)
	{
		VX_DBG2("sdmmc ctrl_%d: set SDMMC_CLKSRC fail, exit! \n", ctrl_x, 2,3,4,5,6);
		return (-FMSH_ETIME);
	}

	/**/
	/* enable clock */
	/**/
	sdmmcCtrl_WrReg32(pSdmmc, SDMMC_CLKENA, 0xFFFF);
	tmp32 = START_CMD + UPDATE_CLOCK_REG_ONLY + WAIT_PRVDATA_COMPLETE;
	sdmmcCtrl_WrReg32(pSdmmc, SDMMC_CMD_VAL, tmp32);
	ret = sdmmcCtrl_wait_StartCmd_Ok(pSdmmc);
	if (ret < 0)
	{
		VX_DBG2("sdmmc ctrl_%d: enable SDMMC_CLKENA fail, exit! \n", ctrl_x, 2,3,4,5,6);
		return (-FMSH_ETIME);
	}
	
    delay_us(10);

	return 0;
}












/*****************************************************************************
*
* @description
* This function to set high speed in card and host.
* Changes clock in host accordingly.
*
* @param
*           pSdmmc is the pointer to a sd card device.
*
* @return
*           Returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note     NA.
*
*****************************************************************************/
int sdmmcChg_BusSpeed(vxT_SDMMC* pSdmmc)
{
	vxT_SDMMC_CTRL * pSdmmcCtrl = pSdmmc->pSdmmcCtrl;
	vxT_SDMMC_CARD * pSdmmcCard = pSdmmc->pSdmmcCard;
		
    int ret;
    UINT32 arg;	
    UINT8 readBuf[64];
    
    if (pSdmmcCard->card_type == SD)
    {
        arg = SD_SWITCH_CMD_HS_SET;
        ret = sdmmc_cmd6_sd(pSdmmc, arg, readBuf);
        if (ret < 0)
        {
            return ret;
        }
        
		ret = sdmmcSet_Clk_DataRate(pSdmmc, SDMMC_DATA_SPEED);  /* SDMMC_CLK_25_MHZ */
        if (ret < 0)
        {
            return ret;
        }
    }
    else if ((pSdmmcCard->card_type == MMC) || (pSdmmcCard->card_type == EMMC))
    {
        arg = MMC_HIGH_SPEED_ARG;
        ret = sdmmc_cmd6_mmc(pSdmmc, arg);
        if (ret < 0)
        {
            return ret;
        }
        
		ret = sdmmcSet_Clk_DataRate(pSdmmc, SDMMC_DATA_SPEED);   /*  SDMMC_CLK_25_MHZ */
        if (ret < 0)
        {
            return ret;
        }
    }

    return ret;
}


int sdmmcChg_BusSpeed_MMC(vxT_SDMMC* pSdmmc)
{
	vxT_SDMMC_CTRL * pSdmmcCtrl = pSdmmc->pSdmmcCtrl;
	vxT_SDMMC_CARD * pSdmmcCard = pSdmmc->pSdmmcCard;
		
    int ret;
    UINT32 arg;	
    UINT8 readBuf[64];
    
    arg = MMC_HIGH_SPEED_ARG;
    ret = sdmmc_cmd6_mmc(pSdmmc, arg);
	
    if (ret < 0)
    {
        return ret;
    }
    
	ret = sdmmcSet_Clk_DataRate(pSdmmc, SDMMC_DATA_SPEED); /* SDMMC_CLK_52_MHZ, SDMMC_CLK_25_MHZ */
    if (ret < 0)
    {
        return ret;
    }

    return ret;
}


#endif


#define __ENUM__

#if 1
/*****************************************************************************
*
* @description
* This function runs the MMC card and emmc enumeration sequence.
* This function runs after the initialization and identification procedure.
* It gets all necessary information from the card.
*
* @param
*           pSdmmc is the pointer to a sd card device.
*
* @return
*           Returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note     NA.
*
*****************************************************************************/
int sdmmc_Enumerate_MMC(vxT_SDMMC * pSdmmc)
{
	vxT_SDMMC_CTRL * pSdmmcCtrl = pSdmmc->pSdmmcCtrl;
	vxT_SDMMC_CARD * pSdmmcCard = pSdmmc->pSdmmcCard;
	vxT_SDMMC_CMD  * pSdmmcCmd  = pSdmmc->pSdmmcCmd;
	
    int ret;
    int retry = 10000;
	
    
    pSdmmcCard->highCapacity = 0;

    /* 
    SEND_OP_COND 
	*/
    ret = sdmmc_cmd1(pSdmmc);
    if(ret == -FMSH_ENODEV)
    {
        return ret;
    }
    delay_ms(1);
	
    pSdmmcCard->ocr = pSdmmcCmd->rsp_buf[0];
	
    while (!(pSdmmcCard->ocr & MMC_CARD_BUSY))
    {
        ret = sdmmc_cmd1(pSdmmc);
        if(ret == -FMSH_ENODEV)
        {
            return ret;
        }
		
        pSdmmcCard->ocr = pSdmmcCmd->rsp_buf[0];
		
        retry--;
        if (retry == 0)
        {
            ret = -FMSH_ETIME;
            return ret;
        }
		
        delay_ms(1);
    }
	
	VX_DBG("sdmmc ctrl_%d: CMD1 response success, Card is unbusy, retry[%d]! \n", pSdmmcCtrl->ctrl_x, retry, 3,4,5,6);
	VX_DBG("--ctrl(%d)cmd1-- \n\n", pSdmmcCtrl->ctrl_x, 2,3,4,5,6);
	
    /* 
    ALL_SEND_CID 
	*/
    ret = sdmmc_cmd2(pSdmmc);
    if (ret < 0) 
    {
        return ret;
    }
	VX_DBG("--ctrl(%d)cmd2-- \n\n", pSdmmcCtrl->ctrl_x, 2,3,4,5,6);
	
    memcpy(pSdmmcCard->raw_cid, pSdmmcCmd->rsp_buf, (4*sizeof(UINT32)));

    /* 
    SET_RELATIVE_ADDR, Stand-by State 
	*/
    /*pInstance->rca = 2; /* slot id begins at 0 */
    
    ret = sdmmc_cmd3(pSdmmc);
    if (ret < 0)
    {
        return ret;
    }
	VX_DBG("mmc cmd3->g_pSdmmcCard->rca: 0x%X \n", pSdmmcCard->rca, 2,3,4,5,6);
	VX_DBG("--ctrl(%d)cmd3-- \n\n", pSdmmcCtrl->ctrl_x, 2,3,4,5,6);
	
    if (!(pSdmmcCmd->rsp_buf[0] & STATE_IDENT))
    {
		VX_DBG2("sdmmc ctrl_%d: CMD3 response failed\! \n", pSdmmcCtrl->ctrl_x, 2,3,4,5,6);
        return -FMSH_ENODEV;
    }

    /* 
    SEND_CSD, Stand-by State 
	*/
    ret = sdmmc_cmd9(pSdmmc);
    if (ret < 0)
    {
        return ret;
    }
	
    VX_DBG("csd[0] is 0x%08x\r\n", pSdmmcCmd->rsp_buf[0], 2,3,4,5,6);
    VX_DBG("csd[1] is 0x%08x\r\n", pSdmmcCmd->rsp_buf[1], 2,3,4,5,6);
    VX_DBG("csd[2] is 0x%08x\r\n", pSdmmcCmd->rsp_buf[2], 2,3,4,5,6);
    VX_DBG("csd[3] is 0x%08x\r\n", pSdmmcCmd->rsp_buf[3], 2,3,4,5,6);
	
    VX_DBG("g_pSdmmcCard->rca:0x%08X \n", pSdmmcCard->rca, 2,3,4,5,6);
	VX_DBG("--ctrl(%d)cmd9-- \n\n", pSdmmcCtrl->ctrl_x, 2,3,4,5,6);
	
    memcpy(pSdmmcCard->raw_csd, pSdmmcCmd->rsp_buf, (4*sizeof(UINT32)));
    
    ret = sdmmcCard_decode_CSD_MMC(pSdmmc);
    if (ret < 0)
    {
		VX_DBG2("sdmmc ctrl_%d: unrecognised CSD structure version! \n", pSdmmcCtrl->ctrl_x, 2,3,4,5,6);
        return ret;
    }
    
    ret = sdmmcCard_decode_CID_MMC(pSdmmc);
    if (ret < 0)
    {
		VX_DBG2("sdmmc ctrl_%d: card has unknown MMCA version! \n", pSdmmcCtrl->ctrl_x, 2,3,4,5,6);
    }
    
    ret = sdmmcCard_Get_Info_CSD(pSdmmc);
    if (ret < 0)
    {
		VX_DBG2("sdmmc ctrl_%d: read_blk_len and write_blk_len are not equal! \n", pSdmmcCtrl->ctrl_x, 2,3,4,5,6);
    }
	
	VX_DBG("sdmmc ctrl_%d: -I- MMC Card OK, size: %d MB! \n", \
	       pSdmmcCtrl->ctrl_x, sdmmcCard_Get_CardSize_KB(pSdmmc)/1024, 3,4,5,6);

    return 0;
}


/*****************************************************************************
*
* @description
* This function runs the SD card enumeration sequence.
* This function runs after the initialization and identification procedure.
* It gets all necessary information from the card.
*
* @param
*           pSdmmc is the pointer to a sd card device.
*
* @return
*           Returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note     NA.
*
*****************************************************************************/
int sdmmc_Enumerate_SD(vxT_SDMMC* pSdmmc)
{
	vxT_SDMMC_CTRL * pSdmmcCtrl = pSdmmc->pSdmmcCtrl;
	vxT_SDMMC_CARD * pSdmmcCard = pSdmmc->pSdmmcCard;
	vxT_SDMMC_CMD * pSdmmcCmd = pSdmmc->pSdmmcCmd;    
	
    int ret;
    int retry = 20000;	
    
    ret = sdmmc_appCmd41(pSdmmc);
    if(ret < 0)
    {
        VX_DBG2("ACMD41 failed: %d\r\n", ret, 2,3,4,5,6);
        return ret;
    }
    
    pSdmmcCard->ocr = pSdmmcCmd->rsp_buf[0];
	
    while (!(pSdmmcCard->ocr & MMC_CARD_BUSY))
    {
        ret = sdmmc_sdACMD41(pSdmmc);
        if(ret < 0)
        {
            VX_DBG2("ACMD41 failed: %d\r\n", ret, 2,3,4,5,6);
            return ret;
        }
		
        pSdmmcCard->ocr = pSdmmcCmd->rsp_buf[0];
		
        retry--;
        if (retry == 0)
        {
            ret = (-FMSH_ETIME);
            return ret;
        }
    }
	
    VX_DBG("ACMD41 response success, Card is unbusy, retry[%d]! \n", retry, 2,3,4,5,6);
    
    if(pSdmmcCard->ocr & 0x40000000)
    {
        VX_DBG("high capacity sd card \n", 1,2,3,4,5,6);
        pSdmmcCard->highCapacity = 1;
    }
    else
    {
        VX_DBG("normal capacity sd card \n", 1,2,3,4,5,6);
        pSdmmcCard->highCapacity = 0;
    }
	
    /* 
    ALL_SEND_CID 
	*/
    ret = sdmmc_cmd2(pSdmmc);
    if (ret < 0)
    {
		VX_DBG2("sdmmc_cmd2 failed: %d \n", ret, 2,3,4,5,6);
        return ret;
    }
		
    VX_DBG("cid[0] is 0x%08x\r\n", pSdmmcCmd->rsp_buf[0], 2,3,4,5,6);
    VX_DBG("cid[1] is 0x%08x\r\n", pSdmmcCmd->rsp_buf[1], 2,3,4,5,6);
    VX_DBG("cid[2] is 0x%08x\r\n", pSdmmcCmd->rsp_buf[2], 2,3,4,5,6);
    VX_DBG("cid[3] is 0x%08x\r\n", pSdmmcCmd->rsp_buf[3], 2,3,4,5,6);
	
    memcpy(pSdmmcCard->raw_cid, pSdmmcCmd->rsp_buf, (4*sizeof(UINT32)));
    
    /*
    SET_RELATIVE_ADDR, Stand-by State 
    */
    pSdmmcCard->rca = 1 ; /* slot id begins at 0 */
	
    ret = sdmmc_cmd3(pSdmmc);
    if (ret < 0)
    {
		VX_DBG2("sdmmc_cmd3 failed: %d \n", ret, 2,3,4,5,6);
        return ret;
    }
    pSdmmcCard->rca = pSdmmcCmd->rsp_buf[0] >> 16;
	
	VX_DBG("sd cmd3->g_pSdmmcCard->rca: 0x%X \n", pSdmmcCard->rca, 2,3,4,5,6);
	
    if (!(pSdmmcCmd->rsp_buf[0] & STATE_IDENT))
    {
		VX_DBG2("sdmmc_cmd3-STATE_IDENT: 0x%08X\r\n", pSdmmcCmd->rsp_buf[0], 2,3,4,5,6);
        return (-FMSH_ENODEV);
    }
	
    /* 
    SEND_CSD, Stand-by State 
	*/
    ret = sdmmc_cmd9(pSdmmc);
    if (ret < 0)
    {
		VX_DBG2("sdmmc_cmd9 failed: %d \n", ret, 2,3,4,5,6);
        return ret;
    }
	
    VX_DBG("\ncsd[0] is 0x%08x\r\n", pSdmmcCmd->rsp_buf[0], 2,3,4,5,6);
    VX_DBG("csd[1] is 0x%08x\r\n", pSdmmcCmd->rsp_buf[1], 2,3,4,5,6);
    VX_DBG("csd[2] is 0x%08x\r\n", pSdmmcCmd->rsp_buf[2], 2,3,4,5,6);
    VX_DBG("csd[3] is 0x%08x\r\n", pSdmmcCmd->rsp_buf[3], 2,3,4,5,6);
	
    memcpy(pSdmmcCard->raw_csd, pSdmmcCmd->rsp_buf, (4*sizeof(UINT32)));
	
    ret = sdmmcCard_decode_CSD(pSdmmc);
    if (ret < 0)
    {
		VX_DBG2("sdmmc ctrl_%d: unrecognised CSD structure version! \n", pSdmmcCtrl->ctrl_x, 2,3,4,5,6);
        return ret;
    }
    
    ret = sdmmcCard_decode_CID(pSdmmc);
    if (ret < 0)
    {
		VX_DBG2("sdmmc ctrl_%d: card has unknown MMCA version! \n", pSdmmcCtrl->ctrl_x, 2,3,4,5,6);
    }
    
    ret = sdmmcCard_Get_Info_CSD(pSdmmc);
    if (ret < 0)
    {
		VX_DBG2("sdmmc ctrl_%d: read_blk_len and write_blk_len are not equal! \n", pSdmmcCtrl->ctrl_x, 2,3,4,5,6);
		return ret;
    }

	
	VX_DBG("sdmmc ctrl_%d: -I- SD Card OK, size: %d MB! \n", \
	       pSdmmcCtrl->ctrl_x, sdmmcCard_Get_CardSize_KB(pSdmmc)/1024, 3,4,5,6);
	
    return 0;    
}

#endif


#if 1


#if 1
/*******************************************************************************
*
* vx_sdmmc_init - init sdmmc ctrl and sd_card(device)
*
*
* RETURNS: N/A
*
* ERRNO: N/A
*/
int vxInit_Sdmmc(int ctrl_x, int fifo_dma_mode)
{
	vxT_SDMMC * pSdmmc = NULL;
	
	vxT_SDMMC_CTRL * pSdmmcCtrl = NULL;
	vxT_SDMMC_CARD * pSdmmcCard = NULL;
	vxT_SDMMC_CMD  * pSdmmcCmd  = NULL;
	
	vxT_DMA_CTRL * pSdmmcDMA  = NULL;
	
	int ret = 0;
	
	UINT32 tmp32 = 0;
    int timeout = SDMMC_POLL_TIMEOUT;
	int dma_msize = 0;
	int rx_wmark = 0, tx_wmark = 0;
	
    UINT8 cmd6_RdBuf[64] = { 0U };

#if 1
	/**/
	/* sdmmc_ctrl select*/
	/**/
	switch (ctrl_x)
	{
	case SDMMC_CTRL_0:
		pSdmmc = g_pSdmmc0;
		pSdmmc->pSdmmcCtrl = (vxT_SDMMC_CTRL*)(&vxSdmmc_Ctrl_0.ctrl_x);
		pSdmmc->pSdmmcCard = (vxT_SDMMC_CARD*)(&vxSdmmc_Card_0.status);
		pSdmmc->pSdmmcCmd  = (vxT_SDMMC_CMD*)(&vxSdmmc_Cmd_0.cmd_arg);
		pSdmmc->sdmmc_x = SDMMC_CTRL_0;
		break;
		
	case SDMMC_CTRL_1:
		pSdmmc = g_pSdmmc1;
		pSdmmc->pSdmmcCtrl = (vxT_SDMMC_CTRL*)(&vxSdmmc_Ctrl_1.ctrl_x);
		pSdmmc->pSdmmcCard = (vxT_SDMMC_CARD*)(&vxSdmmc_Card_1.status);
		pSdmmc->pSdmmcCmd  = (vxT_SDMMC_CMD*)(&vxSdmmc_Cmd_1.cmd_arg);
		pSdmmc->sdmmc_x = SDMMC_CTRL_1;
		break;
	}
	
	if (pSdmmc->init_flag == 1)
	{
		return 0;  /* init already*/
	}
	
	pSdmmcCtrl = pSdmmc->pSdmmcCtrl;
	pSdmmcCard = pSdmmc->pSdmmcCard;
	pSdmmcCmd  = pSdmmc->pSdmmcCmd;
	
	/*pSdmmc->pSdmmcDMA = g_pDma;*/
	/*pSdmmcDMA = pSdmmc->pSdmmcDMA;*/
	
	switch (ctrl_x)
	{
	case SDMMC_CTRL_0:
		pSdmmcCtrl->baseAddr = FMQL_SDMMC_0_BASE;	/* Base address of the device */
		break;
		
	case SDMMC_CTRL_1:
		pSdmmcCtrl->baseAddr = FMQL_SDMMC_1_BASE;	/* Base address of the device */
		break;
	}	
#endif

	/**/
	/* config init*/
	/**/
	pSdmmcCtrl->ctrl_x = ctrl_x;
	
	pSdmmcCtrl->sysClkHz = PS_SDIO_CLK_FREQ_HZ;		/* 100M Hz Input to sdmmc_ctrl clock frequency: sysclk of bus */

#if 1 /* emmc*/
	pSdmmcCtrl->isCardDetect = 0;	                /* has Card Detect ? */
	pSdmmcCtrl->isWrProtect = 0;	                /* has Write Protect ? */
#else  /* sd*/
	pSdmmcCtrl->isCardDetect = 0;	                /* has Card Detect ? */
	pSdmmcCtrl->isWrProtect = 0;	                /* has Write Protect ? */
#endif


	pSdmmcCtrl->devIP_ver = 0x1000;                 /* 1.0.0.0 */
    pSdmmcCtrl->devIP_type = FMSH_DEVIP_APB_SDMMC;  /* identify peripheral types: FMSH_DEVIP_APB_SDMMC */

	pSdmmcCtrl->xferMode = fifo_dma_mode; /* MODE_XFER_FIFO, MODE_XFER_DMA; */

	/**/
	/* dma pre_init*/
	/**/
	if (fifo_dma_mode == MODE_XFER_DMA)
	{
		vxInit_Dma_1();
		
		pSdmmc->pSdmmcDMA = g_pDma;
		pSdmmcDMA = pSdmmc->pSdmmcDMA;
	}

	
	/*///////////////////////////////////*/
	/**/
	/* sdmmc_ctrl init*/
	/**/
	/*///////////////////////////////////*/
	
	/**/
	/* reset ctrl, fifo,*/
	/*	*/
	/* reset ctrl, fifo, dma*/
	/**/
    tmp32 = sdmmcCtrl_RdReg32(pSdmmc, SDMMC_CTRL);
    tmp32 |= (CONTROLLER_RESET | FIFO_RESET | DMA_RESET);
	sdmmcCtrl_WrReg32(pSdmmc, SDMMC_CTRL, tmp32);

    timeout = SDMMC_POLL_TIMEOUT;
    do
	{
        delay_1us();
		
        tmp32 = sdmmcCtrl_RdReg32(pSdmmc, SDMMC_CTRL);
        tmp32 &= (CONTROLLER_RESET | FIFO_RESET | DMA_RESET);
		
		timeout--;
		if (timeout <= 0)
		{
			VX_DBG2("sdmmc ctrl_%d reset ctrl_fifo_dma fail, exit! \n", ctrl_x, 2,3,4,5,6);
			return (-FMSH_ETIME);
		}
    } while (tmp32);

	/*
	RX_WMark = (FIFO_DEPTH / 2) - 1
	TX_WMark =  FIFO_DEPTH / 2
	*/	
	/**/
	/* set dma & fifo*/
	/*	*/
	dma_msize = 2; /* 010 - 8*/
    tmp32 = sdmmcCtrl_RdReg32(pSdmmc, SDMMC_FIFOTH) | (dma_msize << DW_DMA_MULTI_TRAN_SIZE_OFFSET);
    sdmmcCtrl_WrReg32(pSdmmc, SDMMC_FIFOTH, tmp32);
	
	/* MSize = 8，RX_WMark = 7, TX_WMark = 8*/
    tmp32 = sdmmcCtrl_RdReg32(pSdmmc, SDMMC_FIFOTH) & (~(RX_WMARK_MASK << RX_WMARK_OFFSET));
	rx_wmark = 7;
    tmp32 |= (rx_wmark << RX_WMARK_OFFSET);
    sdmmcCtrl_WrReg32(pSdmmc, SDMMC_FIFOTH, tmp32);
	/**/
    tmp32 = sdmmcCtrl_RdReg32(pSdmmc, SDMMC_FIFOTH) & (~(TX_WMARK_MASK << TX_WMARK_OFFSET));
	tx_wmark = 8;
    tmp32 |= (tx_wmark << TX_WMARK_OFFSET);
    sdmmcCtrl_WrReg32(pSdmmc, SDMMC_FIFOTH, tmp32);

	/**/
	/* clear irq*/
	/*	*/
    sdmmcCtrl_WrReg32(pSdmmc, SDMMC_RINTSTS, 0xFFFFFFFF);
	/* enable irq*/
    tmp32 = sdmmcCtrl_RdReg32(pSdmmc, SDMMC_CTRL) | INT_ENABLE;
    sdmmcCtrl_WrReg32(pSdmmc, SDMMC_CTRL, tmp32);
    /* mask interrupt: enable CD*/
    tmp32 = INT_MASK_CD;              /* 0-屏蔽中断; 1-使能中断 */
    sdmmcCtrl_WrReg32(pSdmmc, SDMMC_INTMASK, tmp32);

	/**/
	/* dma_mode*/
	/**/
	if (fifo_dma_mode == MODE_XFER_DMA)
	{		
		/* enble DMA*/
	    tmp32 = sdmmcCtrl_RdReg32(pSdmmc, SDMMC_CTRL);
		tmp32 |= DMA_ENABLE;
	    sdmmcCtrl_WrReg32(pSdmmc, SDMMC_CTRL, tmp32);

		/**/
		/* init dma*/
		/**/
		vxInit_Dma_2();
		
		/*FDmaPs_enable(pInstance->pDma);*/
		/**/
		/* enable dma_ctrl*/
		/**/
		/**/
		dmaCtrl_Enable_Ctrl(pSdmmc->pSdmmcDMA);
	}

	/**/
    /* clear int*/
	/**/
    sdmmcCtrl_WrReg32(pSdmmc, SDMMC_RINTSTS, sdmmcCtrl_RdReg32(pSdmmc, SDMMC_RINTSTS));

#if 1  /* card detect*/
	if (pSdmmcCtrl->isCardDetect == 1)
	{	
    	tmp32 = sdmmcCtrl_RdReg32(pSdmmc, SDMMC_CDETECT) & CARD_DETECT;
        if (tmp32 != CARD_INSERT)
        {
			VX_DBG2("sdmmc ctrl_%d: No card insert, exit! \n", ctrl_x, 2,3,4,5,6);
			return FMSH_FAILURE;
        }
		else
		{
			VX_DBG("sdmmc ctrl_%d: Card insert! \n", ctrl_x, 2,3,4,5,6);
		}
	}
	
    pSdmmcCard->status = 0;
	pSdmmcCard->rca = 0;
#endif

	/**/
	/* powerup*/
	/**/
#if 1
    /*sdmmcCtrl_WrReg32(pSdmmc, SDMMC_PWREN, 0xFFFF);*/
    sdmmcCtrl_WrReg32(pSdmmc, SDMMC_PWREN, 0xFF);
    delay_us(100);
#endif

	/**/
	/* setclock-400K*/
	/**/
	ret = sdmmcSet_Clk_DataRate(pSdmmc, SDMMC_CLK_400_KHZ);
	if (ret < 0)
	{
		VX_DBG2("sdmmc ctrl_%d: sdmmcSet_Clk_DataRate-400K fail, exit(%d)! \n", ctrl_x, ret,3,4,5,6);
		return ret;
	}
	else
	{
		VX_DBG("sdmmc ctrl_%d: sdmmcSet_Clk_DataRate-400K OK! \n", ctrl_x, 2,3,4,5,6);
	}
	
	delay_us(10);

#if 1	

    /* 
    go IDLE state 
	*/
    ret = sdmmc_cmd0(pSdmmc);
    if (ret < 0)
    {
		VX_DBG2("sdmmc ctrl_%d: sdmmc_cmd0 fail, exit(%d)! \n", ctrl_x, ret,3,4,5,6);
        return ret;
    }
    pSdmmcCard->card_ver = SDMMC_CARD_VER_1_0;
	
	delay_us(100);

	/*
	check whether ver2.0
	*/
    ret = sdmmc_cmd8_sd(pSdmmc);
    if (ret < 0)
    {
        pSdmmcCard->card_ver = SDMMC_CARD_VER_1_0;
		VX_DBG2("sdmmc ctrl_%d: sdmmc_cmd8_sd fail, exit(%d)! \n", ctrl_x, ret,3,4,5,6);
    }
    else
    {
		VX_DBG("sdmmc ctrl_%d: sdmmc_cmd8_get response! \n", ctrl_x, 2,3,4,5,6);

        if((pSdmmcCmd->rsp_buf[0] & 0xFF) == (SD_CMD8_VOL_PATTERN & 0xFF))
        {
            pSdmmcCard->card_ver = SDMMC_CARD_VER_2_0;
			VX_DBG("sdmmc ctrl_%d: card ver2.0 0x%X \n", ctrl_x, pSdmmcCmd->rsp_buf[0], 3,4,5,6);
        }
        else 
        {
            pSdmmcCard->card_ver = SDMMC_CARD_VER_1_0;
			VX_DBG("sdmmc ctrl_%d: card ver1.0 0x%X \n", ctrl_x, pSdmmcCmd->rsp_buf[0], 3,4,5,6);
        }
    }

	/* 
	 check whether it's a MMC or a SD card. 
	*/
	
    /* APP_CMD */
    ret = sdmmc_cmd55(pSdmmc);
    if (ret < 0)
    {
        if (ret == (-FMSH_ENODEV))
        {
            return ret;
        }
        
		VX_DBG("sdmmc ctrl_%d: MultiMedia Card Identification: \n", ctrl_x, 2,3,4,5,6);
		
    	tmp32 = sdmmcCtrl_RdReg32(pSdmmc, SDMMC_CDETECT) & CARD_DETECT;
        if (tmp32 == CARD_INSERT)
        {
        	pSdmmcCard->card_type = MMC;
        }
        else
        {
        	pSdmmcCard->card_type = EMMC;
        }		
    }
    else
    {
        pSdmmcCard->card_type = SD;
		VX_DBG("sdmmc ctrl_%d: SD Memory Card Identification: \n", ctrl_x,2,3,4,5,6);		
    }

	/**/
	/* enumerate the sdmmc*/
	/**/
	switch (pSdmmcCard->card_type)
	{
	case MMC:
	case EMMC:
        ret = sdmmc_Enumerate_MMC(pSdmmc);
		if (ret < 0)
		{
			VX_DBG2("sdmmc ctrl_%d: sdmmc_Enumerate_MMC fail,exit(%d)! \n", ctrl_x, ret, 3,4,5,6);
			return ret;
		}
		break;

	case SD:
        ret = sdmmc_Enumerate_SD(pSdmmc);
		if (ret < 0)
		{
			VX_DBG2("sdmmc ctrl_%d: sdmmc_Enumerate_SD fail,exit(%d)! \n", ctrl_x, ret, 3,4,5,6);
			return ret;
		}
		break;
	}
#endif

#if 1

	/**/
    /* switch to TRAN mode to Select the current SD/MMC*/
    /* SELECT CARD & set card state from Stand-by to Transfer*/
    /**/
    ret = sdmmc_cmd7(pSdmmc);
    if (ret < 0)
    {
		VX_DBG2("sdmmc ctrl_%d: sdmmc_cmd7 fail,exit(%d)! \n", ctrl_x, ret, 3,4,5,6);
        return ret;
    }
    if (pSdmmcCmd->rsp_buf[0] & (R1_CC_ERR | R1_ERR))
    {
		VX_DBG2("sdmmc ctrl_%d: CMD7 response fail,exit(%d)! \n", ctrl_x, ret, 3,4,5,6);
        return (-FMSH_EIO);
    }

    /* 
    select card bus width 
	*/
	if (pSdmmcCard->card_type == SD)
	{
		ret = sdmmcCard_Set_BusWidth(pSdmmc, SD_BUS_WIDTH_4);
		if (ret < 0)
		{
			VX_DBG2("sdmmc ctrl_%d: sdmmcCard_Set_BusWidth fail,exit(%d)! \n", ctrl_x, ret, 3,4,5,6);
			return ret;
		}
	}
	else  /* mmc / eMMC*/
	{
	    ret = sdmmcCard_Set_BusWidth_MMC(pSdmmc, SD_BUS_WIDTH_4);
	    if (ret < 0)
	    {
			VX_DBG2("sdmmc ctrl_%d: sdmmcCard_Set_BusWidth_MMC fail,exit(%d)! \n", ctrl_x, ret, 3,4,5,6);
	        return ret;
	    }

		delay_ms(10);
		
        ret = sdmmcCard_Get_ExtCsd_MMC(pSdmmc);
        if (ret < 0)
        {
            VX_DBG2("sdmmc ctrl_%d: Get MMC Ext_CSD info failed! \n", ctrl_x,2,3,4,5,6);
            return ret;
        }
    }
	
    /* 
    Set high clock rate for the normal data transfer 
	*/
	ret = sdmmcSet_Clk_DataRate(pSdmmc, pSdmmcCard->info_csd.max_dtr);
    if (ret < 0)
    {
		VX_DBG2("sdmmc ctrl_%d: Switch the clock to %dHz for data transfer \n", ctrl_x, pSdmmcCard->info_csd.max_dtr, 3,4,5,6);
        return ret;
    }
	else
	{
		VX_DBG2("sdmmcSet_Clk_DataRate(%d) OK! \n", pSdmmcCard->info_csd.max_dtr, 2,3,4,5,6);
	}
	
#endif

#if 1
	/*pSdmmcCtrl->xferMode = MODE_XFER_FIFO;*/

	if (pSdmmcCard->card_type == SD)
    {
        /* get sd card SCR info*/
        ret = sdmmcCard_Get_Info_SCR(pSdmmc);
        if (ret < 0)
        {
            VX_DBG2("SD Get SD's SCR info error! \n", 1,2,3,4,5,6);
            return ret;
        }

        if (pSdmmcCard->info_scr.sd_spec != 0)
        {
            ret = sdmmcCard_Get_BusSpeed(pSdmmc, cmd6_RdBuf);
            if (ret < 0)
            {
                return ret;
            }

            if ((cmd6_RdBuf[13] & HIGH_SPEED_SUPPORT) != 0)
            {
                ret = sdmmcChg_BusSpeed(pSdmmc);
                if (ret < 0)
                {
                    VX_DBG2("Failed to change SD to high speed mode! \n", 1,2,3,4,5,6);
                    return ret;
                }
                else
                { 
                	VX_DBG2("Change SD to high speed mode succeed! \n", 1,2,3,4,5,6);
                }
            }
        }
    }
    else if ((pSdmmcCard->card_type == MMC) || (pSdmmcCard->card_type == EMMC))
    {
        if ((pSdmmcCard->ext_csd[MMC_EXT_CSD_CARD_TYPE] & EXT_CSD_DEVICE_TYPE_HIGH_SPEED) != 0)
        {
			ret = sdmmcChg_BusSpeed_MMC(pSdmmc);
            if (ret < 0)
            {
				VX_DBG2("Failed to change MMC to high speed mode! \n", 1,2,3,4,5,6);
                return ret;
            }
            else
            {
            	VX_DBG2("Change MMC to high speed mode succeed! \n", 1,2,3,4,5,6);
            }
        }
        else
        {
			VX_DBG2("MMC doesn't support high speed mode! \n", 1,2,3,4,5,6);
        }
        /*
		*/
    }
#endif

    if (pSdmmcCard->highCapacity == 0)
    {
    	sdmmcCard_Set_BlockSize(pSdmmc, SDMMC_BLOCK_SIZE);
		VX_DBG("sdmmc ctrl_%d: sdmmcCard_Set_BlockSize(%d) OK! \n", ctrl_x, SDMMC_BLOCK_SIZE, 3,4,5,6);
    }
	
	if (fifo_dma_mode == MODE_XFER_DMA)
	{
		pSdmmcCtrl->xferMode = MODE_XFER_DMA;
	}
	else
	{
		pSdmmcCtrl->xferMode = MODE_XFER_FIFO;
	}

	return FMSH_SUCCESS;
}


int vxInit_Sdmmc_uboot(int ctrl_x, int fifo_dma_mode)
{
	vxT_SDMMC * pSdmmc = NULL;
	
	vxT_SDMMC_CTRL * pSdmmcCtrl = NULL;
	vxT_SDMMC_CARD * pSdmmcCard = NULL;
	vxT_SDMMC_CMD  * pSdmmcCmd  = NULL;
	
	vxT_DMA_CTRL * pSdmmcDMA  = NULL;
	
	int ret = 0;
	
	UINT32 tmp32 = 0;
    int timeout = SDMMC_POLL_TIMEOUT;
	int dma_msize = 0;
	int rx_wmark = 0, tx_wmark = 0;
	
    UINT8 cmd6_RdBuf[64] = { 0U };

#if 1
	/**/
	/* sdmmc_ctrl select*/
	/**/
	switch (ctrl_x)
	{
	case SDMMC_CTRL_0:
		pSdmmc = g_pSdmmc0;
		pSdmmc->pSdmmcCtrl = (vxT_SDMMC_CTRL*)(&vxSdmmc_Ctrl_0.ctrl_x);
		pSdmmc->pSdmmcCard = (vxT_SDMMC_CARD*)(&vxSdmmc_Card_0.status);
		pSdmmc->pSdmmcCmd  = (vxT_SDMMC_CMD*)(&vxSdmmc_Cmd_0.cmd_arg);
		pSdmmc->sdmmc_x = SDMMC_CTRL_0;
		break;
		
	case SDMMC_CTRL_1:
		pSdmmc = g_pSdmmc1;
		pSdmmc->pSdmmcCtrl = (vxT_SDMMC_CTRL*)(&vxSdmmc_Ctrl_1.ctrl_x);
		pSdmmc->pSdmmcCard = (vxT_SDMMC_CARD*)(&vxSdmmc_Card_1.status);
		pSdmmc->pSdmmcCmd  = (vxT_SDMMC_CMD*)(&vxSdmmc_Cmd_1.cmd_arg);
		pSdmmc->sdmmc_x = SDMMC_CTRL_1;
		break;
	}
	
	if (pSdmmc->init_flag == 1)
	{
		return 0;  /* init already*/
	}
	
	pSdmmcCtrl = pSdmmc->pSdmmcCtrl;
	pSdmmcCard = pSdmmc->pSdmmcCard;
	pSdmmcCmd  = pSdmmc->pSdmmcCmd;
	
	/*pSdmmc->pSdmmcDMA = g_pDma;*/
	/*pSdmmcDMA = pSdmmc->pSdmmcDMA;*/
	
	switch (ctrl_x)
	{
	case SDMMC_CTRL_0:
		pSdmmcCtrl->baseAddr = FMQL_SDMMC_0_BASE;	/* Base address of the device */
		break;
		
	case SDMMC_CTRL_1:
		pSdmmcCtrl->baseAddr = FMQL_SDMMC_1_BASE;	/* Base address of the device */
		break;
	}	
#endif

	/**/
	/* config init*/
	/**/
	pSdmmcCtrl->ctrl_x = ctrl_x;
	
	pSdmmcCtrl->sysClkHz = PS_SDIO_CLK_FREQ_HZ;		/* 100M Hz Input to sdmmc_ctrl clock frequency: sysclk of bus */
	pSdmmcCtrl->isCardDetect = 1;	                /* has Card Detect ? */
	pSdmmcCtrl->isWrProtect = 1;	                /* has Write Protect ? */
	
	pSdmmcCtrl->devIP_ver = 0x1000;                 /* 1.0.0.0 */
    pSdmmcCtrl->devIP_type = FMSH_DEVIP_APB_SDMMC;  /* identify peripheral types: FMSH_DEVIP_APB_SDMMC */

	pSdmmcCtrl->xferMode = fifo_dma_mode; /* MODE_XFER_FIFO, MODE_XFER_DMA; */

	/**/
	/* dma pre_init*/
	/**/
	if (fifo_dma_mode == MODE_XFER_DMA)
	{
		vxInit_Dma_1();
		
		pSdmmc->pSdmmcDMA = g_pDma;
		pSdmmcDMA = pSdmmc->pSdmmcDMA;
	}

	
	/*///////////////////////////////////*/
	/**/
	/* sdmmc_ctrl init*/
	/**/
	/*///////////////////////////////////*/
	
	/**/
	/* powerup*/
	/**/
#if 1
	/*sdmmcCtrl_WrReg32(pSdmmc, SDMMC_PWREN, 0xFFFF);*/
	/*sdmmcCtrl_WrReg32(pSdmmc, SDMMC_PWREN, 0xFF);*/
	sdmmcCtrl_WrReg32(pSdmmc, SDMMC_PWREN, 0x01);  /* regf_uboot*/
	delay_us(100);
#endif

	/**/
	/* reset ctrl, fifo,*/
	/*	*/
	/* reset ctrl, fifo, dma*/
	/**/
    tmp32 = sdmmcCtrl_RdReg32(pSdmmc, SDMMC_CTRL);
    tmp32 |= (CONTROLLER_RESET | FIFO_RESET | DMA_RESET);
	sdmmcCtrl_WrReg32(pSdmmc, SDMMC_CTRL, tmp32);

    timeout = SDMMC_POLL_TIMEOUT;
    do
	{
        delay_1us();
		
        tmp32 = sdmmcCtrl_RdReg32(pSdmmc, SDMMC_CTRL);
        tmp32 &= (CONTROLLER_RESET | FIFO_RESET | DMA_RESET);
		
		timeout--;
		if (timeout <= 0)
		{
			VX_DBG2("sdmmc ctrl_%d reset ctrl_fifo_dma fail, exit! \n", ctrl_x, 2,3,4,5,6);
			return (-FMSH_ETIME);
		}
    } while (tmp32);

	/**/
	/* setclock-400K*/
	/**/
	ret = sdmmcSet_Clk_DataRate(pSdmmc, SDMMC_CLK_400_KHZ);
	if (ret < 0)
	{
		VX_DBG2("sdmmc ctrl_%d: sdmmcSet_Clk_DataRate-400K fail, exit(%d)! \n", ctrl_x, ret,3,4,5,6);
		return ret;
	}
	else
	{
		VX_DBG("sdmmc ctrl_%d: sdmmcSet_Clk_DataRate-400K OK! \n", ctrl_x, 2,3,4,5,6);
	}
	
	delay_us(10);

	/**/
	/* clear irq*/
	/*	*/
    sdmmcCtrl_WrReg32(pSdmmc, SDMMC_RINTSTS, 0xFFFFFFFF);
    sdmmcCtrl_WrReg32(pSdmmc, SDMMC_INTMASK, 0);

    sdmmcCtrl_WrReg32(pSdmmc, SDMMC_TMOUT, 0xFFFFFFFF);
	
    sdmmcCtrl_WrReg32(pSdmmc, SDMMC_IDINTEN, 0);
    sdmmcCtrl_WrReg32(pSdmmc, SDMMC_BMOD, 1);
	
	/*
	RX_WMark = (FIFO_DEPTH / 2) - 1
	TX_WMark =  FIFO_DEPTH / 2
	*/	
	/**/
	/* set dma & fifo*/
	/*	*/
#if 0
	dma_msize = 2; /* 010 - 8*/
    tmp32 = sdmmcCtrl_RdReg32(pSdmmc, SDMMC_FIFOTH) | (dma_msize << DW_DMA_MULTI_TRAN_SIZE_OFFSET);
    sdmmcCtrl_WrReg32(pSdmmc, SDMMC_FIFOTH, tmp32);
	
	/* MSize = 8，RX_WMark = 7, TX_WMark = 8*/
    tmp32 = sdmmcCtrl_RdReg32(pSdmmc, SDMMC_FIFOTH) & (~(RX_WMARK_MASK << RX_WMARK_OFFSET));
	rx_wmark = 7;
    tmp32 |= (rx_wmark << RX_WMARK_OFFSET);
    sdmmcCtrl_WrReg32(pSdmmc, SDMMC_FIFOTH, tmp32);
	
	/**/
    tmp32 = sdmmcCtrl_RdReg32(pSdmmc, SDMMC_FIFOTH) & (~(TX_WMARK_MASK << TX_WMARK_OFFSET));
	tx_wmark = 8;
    tmp32 |= (tx_wmark << TX_WMARK_OFFSET);
    sdmmcCtrl_WrReg32(pSdmmc, SDMMC_FIFOTH, tmp32);
#else

	UINT32 fifo_size, fifoth_val;

	tmp32 = sdmmcCtrl_RdReg32(pSdmmc, SDMMC_FIFOTH);
	fifo_size = ((fifo_size & RX_WMARK_MASK) >> RX_WMARK_SHIFT) + 1;
	fifoth_val = MSIZE(0x2) | RX_WMARK(fifo_size / 2 - 1) |	TX_WMARK(fifo_size / 2);
    sdmmcCtrl_WrReg32(pSdmmc, SDMMC_FIFOTH, fifoth_val);
#endif
	
	/* enable irq*/
    tmp32 = sdmmcCtrl_RdReg32(pSdmmc, SDMMC_CTRL) | INT_ENABLE;
    sdmmcCtrl_WrReg32(pSdmmc, SDMMC_CTRL, tmp32);

	
    /* mask interrupt: enable CD*/
    tmp32 = INT_MASK_CD;              /* 0-屏蔽中断; 1-使能中断 */
    sdmmcCtrl_WrReg32(pSdmmc, SDMMC_INTMASK, tmp32);

	/**/
	/* dma_mode*/
	/**/
	if (fifo_dma_mode == MODE_XFER_DMA)
	{		
		/* enble DMA*/
	    tmp32 = sdmmcCtrl_RdReg32(pSdmmc, SDMMC_CTRL);
		tmp32 |= DMA_ENABLE;
	    sdmmcCtrl_WrReg32(pSdmmc, SDMMC_CTRL, tmp32);

		/**/
		/* init dma*/
		/**/
		vxInit_Dma_2();
		
		/*FDmaPs_enable(pInstance->pDma);*/
		/**/
		/* enable dma_ctrl*/
		/**/
		/**/
		dmaCtrl_Enable_Ctrl(pSdmmc->pSdmmcDMA);
	}

	/**/
    /* clear int*/
	/**/
    sdmmcCtrl_WrReg32(pSdmmc, SDMMC_RINTSTS, sdmmcCtrl_RdReg32(pSdmmc, SDMMC_RINTSTS));

#if 1  /* card detect*/
	if (pSdmmcCtrl->isCardDetect == 1)
	{	
    	tmp32 = sdmmcCtrl_RdReg32(pSdmmc, SDMMC_CDETECT) & CARD_DETECT;
        if (tmp32 != CARD_INSERT)
        {
			VX_DBG2("sdmmc ctrl_%d: No card insert, exit! \n", ctrl_x, 2,3,4,5,6);
			return FMSH_FAILURE;
        }
		else
		{
			VX_DBG("sdmmc ctrl_%d: Card insert! \n", ctrl_x, 2,3,4,5,6);
		}
	}
	
    pSdmmcCard->status = 0;
	pSdmmcCard->rca = 0;
#endif



#if 1	

    /* 
    go IDLE state 
	*/
    ret = sdmmc_cmd0(pSdmmc);
    if (ret < 0)
    {
		VX_DBG2("sdmmc ctrl_%d: sdmmc_cmd0 fail, exit(%d)! \n", ctrl_x, ret,3,4,5,6);
        return ret;
    }
    pSdmmcCard->card_ver = SDMMC_CARD_VER_1_0;
	
	delay_us(100);

	/*
	check whether ver2.0
	*/
    ret = sdmmc_cmd8_sd(pSdmmc);
    /*ret = sdmmc_cmd8_mmc(pSdmmc);*/
    if (ret < 0)
    {
        pSdmmcCard->card_ver = SDMMC_CARD_VER_1_0;
		VX_DBG2("sdmmc ctrl_%d: sdmmc_cmd8_sd fail, exit(%d)! \n", ctrl_x, ret,3,4,5,6);
    }
    else
    {
		VX_DBG("sdmmc ctrl_%d: sdmmc_cmd8_get response! \n", ctrl_x, 2,3,4,5,6);

        if((pSdmmcCmd->rsp_buf[0] & 0xFF) == (SD_CMD8_VOL_PATTERN & 0xFF))
        {
            pSdmmcCard->card_ver = SDMMC_CARD_VER_2_0;
			VX_DBG("sdmmc ctrl_%d: card ver2.0 0x%X \n", ctrl_x, pSdmmcCmd->rsp_buf[0], 3,4,5,6);
        }
        else 
        {
            pSdmmcCard->card_ver = SDMMC_CARD_VER_1_0;
			VX_DBG("sdmmc ctrl_%d: card ver1.0 0x%X \n", ctrl_x, pSdmmcCmd->rsp_buf[0], 3,4,5,6);
        }
    }

	/* 
	 check whether it's a MMC or a SD card. 
	*/
	
    /* APP_CMD */
    ret = sdmmc_cmd55(pSdmmc);
    if (ret < 0)
    {
        if (ret == (-FMSH_ENODEV))
        {
            return ret;
        }
        
		VX_DBG("sdmmc ctrl_%d: MultiMedia Card Identification: \n", ctrl_x, 2,3,4,5,6);
		
    	tmp32 = sdmmcCtrl_RdReg32(pSdmmc, SDMMC_CDETECT) & CARD_DETECT;
        if (tmp32 == CARD_INSERT)
        {
        	pSdmmcCard->card_type = MMC;
        }
        else
        {
        	pSdmmcCard->card_type = EMMC;
        }		
    }
    else
    {
        pSdmmcCard->card_type = SD;
		VX_DBG("sdmmc ctrl_%d: SD Memory Card Identification: \n", ctrl_x,2,3,4,5,6);		
    }

	/**/
	/* enumerate the sdmmc*/
	/**/
	switch (pSdmmcCard->card_type)
	{
	case MMC:
	case EMMC:
        ret = sdmmc_Enumerate_MMC(pSdmmc);
		if (ret < 0)
		{
			VX_DBG2("mmc ctrl_%d: sdmmc_Enumerate_MMC fail,exit(%d)! \n", ctrl_x, ret, 3,4,5,6);
			return ret;
		}
		break;

	case SD:
        ret = sdmmc_Enumerate_SD(pSdmmc);
		if (ret < 0)
		{
			VX_DBG2("sd ctrl_%d: sdmmc_Enumerate_SD fail,exit(%d)! \n", ctrl_x, ret, 3,4,5,6);
			return ret;
		}
		break;
	}
#endif

#if 1

	/**/
    /* switch to TRAN mode to Select the current SD/MMC*/
    /* SELECT CARD & set card state from Stand-by to Transfer*/
    /**/
    ret = sdmmc_cmd7(pSdmmc);
    if (ret < 0)
    {
		VX_DBG2("sdmmc ctrl_%d: sdmmc_cmd7 fail,exit(%d)! \n", ctrl_x, ret, 3,4,5,6);
        return ret;
    }
    if (pSdmmcCmd->rsp_buf[0] & (R1_CC_ERR | R1_ERR))
    {
		VX_DBG2("sdmmc ctrl_%d: CMD7 response fail,exit(%d)! \n", ctrl_x, ret, 3,4,5,6);
        return (-FMSH_EIO);
    }


    /* 
    select card bus width 
	*/
#if 1  /* move to back*/
	if (pSdmmcCard->card_type == SD)
	{
		ret = sdmmcCard_Set_BusWidth(pSdmmc, SD_BUS_WIDTH_4);
		if (ret < 0)
		{
			VX_DBG2("sdmmc ctrl_%d: sdmmcCard_Set_BusWidth fail,exit(%d)! \n", ctrl_x, ret, 3,4,5,6);
			return ret;
		}
	}
	else  /* mmc / eMMC*/
	{
	    ret = sdmmcCard_Set_BusWidth_MMC(pSdmmc, SD_BUS_WIDTH_4);
	    if (ret < 0)
	    {
			VX_DBG2("sdmmc ctrl_%d: sdmmcCard_Set_BusWidth_MMC fail,exit(%d)! \n", ctrl_x, ret, 3,4,5,6);
	        return ret;
	    }

		delay_ms(10);
		
        ret = sdmmcCard_Get_ExtCsd_MMC(pSdmmc);
        if (ret < 0)
        {
            VX_DBG2("sdmmc ctrl_%d: Get MMC Ext_CSD info failed! \n", ctrl_x,2,3,4,5,6);
            return ret;
        }
    }
#endif

    /* 
    Set high clock rate for the normal data transfer 
	*/
	ret = sdmmcSet_Clk_DataRate(pSdmmc, pSdmmcCard->info_csd.max_dtr);
    if (ret < 0)
    {
		VX_DBG2("sdmmc ctrl_%d: Switch the clock to %dHz for data transfer \n", ctrl_x, pSdmmcCard->info_csd.max_dtr, 3,4,5,6);
        return ret;
    }
	else
	{
		VX_DBG2("sdmmcSet_Clk_DataRate(%d) OK! \n", pSdmmcCard->info_csd.max_dtr, 2,3,4,5,6);
	}
#endif


#if 0  /* move to here*/
	if (pSdmmcCard->card_type == SD)
	{
		ret = sdmmcCard_Set_BusWidth(pSdmmc, SD_BUS_WIDTH_4);
		if (ret < 0)
		{
			VX_DBG2("sd ctrl_%d: sdmmcCard_Set_BusWidth fail,exit(%d)! \n", ctrl_x, ret, 3,4,5,6);
			return ret;
		}
	}
	else  /* mmc / eMMC*/
	{
	    ret = sdmmcCard_Set_BusWidth_MMC(pSdmmc, SD_BUS_WIDTH_4);
	    if (ret < 0)
	    {
			VX_DBG2("mmc ctrl_%d: sdmmcCard_Set_BusWidth_MMC fail,exit(%d)! \n", ctrl_x, ret, 3,4,5,6);
	        return ret;
	    }

		delay_ms(10);
		
        ret = sdmmcCard_Get_ExtCsd_MMC(pSdmmc);
        if (ret < 0)
        {
            VX_DBG2("mmc ctrl_%d: Get MMC Ext_CSD info failed! \n", ctrl_x,2,3,4,5,6);
            return ret;
        }
    }
#endif


#if 1
	/*pSdmmcCtrl->xferMode = MODE_XFER_FIFO;*/

	if (pSdmmcCard->card_type == SD)
    {
        /* get sd card SCR info*/
        ret = sdmmcCard_Get_Info_SCR(pSdmmc);
        if (ret < 0)
        {
            VX_DBG2("SD Get SD's SCR info error! \n", 1,2,3,4,5,6);
            return ret;
        }

        if (pSdmmcCard->info_scr.sd_spec != 0)
        {
            ret = sdmmcCard_Get_BusSpeed(pSdmmc, cmd6_RdBuf);
            if (ret < 0)
            {
                return ret;
            }

            if ((cmd6_RdBuf[13] & HIGH_SPEED_SUPPORT) != 0)
            {
                ret = sdmmcChg_BusSpeed(pSdmmc);
                if (ret < 0)
                {
                    VX_DBG2("Failed to change SD to high speed mode! \n", 1,2,3,4,5,6);
                    return ret;
                }
                else
                { 
                	VX_DBG2("Change SD to high speed mode succeed! \n", 1,2,3,4,5,6);
                }
            }
        }
    }
    else if ((pSdmmcCard->card_type == MMC) || (pSdmmcCard->card_type == EMMC))
    {
        if ((pSdmmcCard->ext_csd[MMC_EXT_CSD_CARD_TYPE] & EXT_CSD_DEVICE_TYPE_HIGH_SPEED) != 0)
        {
			ret = sdmmcChg_BusSpeed_MMC(pSdmmc);
            if (ret < 0)
            {
				VX_DBG2("Failed to change MMC to high speed mode! \n", 1,2,3,4,5,6);
                return ret;
            }
            else
            {
            	VX_DBG2("Change MMC to high speed mode succeed! \n", 1,2,3,4,5,6);
            }
        }
        else
        {
			VX_DBG2("MMC doesn't support high speed mode! \n", 1,2,3,4,5,6);
        }
    }
#endif

    if (pSdmmcCard->highCapacity == 0)
    {
    	sdmmcCard_Set_BlockSize(pSdmmc, SDMMC_BLOCK_SIZE);
		VX_DBG("sdmmc ctrl_%d: sdmmcCard_Set_BlockSize(%d) OK! \n", ctrl_x, SDMMC_BLOCK_SIZE, 3,4,5,6);
    }
	
	if (fifo_dma_mode == MODE_XFER_DMA)
	{
		pSdmmcCtrl->xferMode = MODE_XFER_DMA;
	}
	else
	{
		pSdmmcCtrl->xferMode = MODE_XFER_FIFO;
	}

	return FMSH_SUCCESS;
}


#endif

/*
int rd_wr_flag: 
	0 - write
	1 - read
*/
int vxSdmmc_WrRd_OneSector(vxT_SDMMC* pSdmmc, UINT8 * pBuf, UINT32 addr_offset, UINT32 len, int wr_rd_flag)
{
	vxT_SDMMC_CARD *pSdmmcCard = pSdmmc->pSdmmcCard;
    vxT_SDMMC_CMD *pCmd = pSdmmc->pSdmmcCmd;
		
    int ret;

    if (((pSdmmcCard->card_type == MMC)  || (pSdmmcCard->card_type == EMMC)) \
		 && (len > 0x200))
    {
        ret = sdmmc_cmd23(pSdmmc, len);
        if (ret)
        {
            VX_DBG2("vxSdmmc_WrRd_OneSector: send cmd23 error! \n", 1,2,3,4,5,6);
            return ret;
        }
    }
		 
	if (wr_rd_flag == SDMMC_WRITE_MODE)  /* write*/
	{
	    pCmd->cmd_idx = (len > 512) ? CMD_WRITE_MULTIPLE_BLOCK : CMD_WRITE_BLOCK;       /* 25 : 24*/
    	pCmd->rsp_flag = CMD_RESP_FLAG_WDATA;  /* 8*/
	}
	else /* read*/
	{
		pCmd->cmd_idx = (len > 512) ? CMD_READ_MULTIPLE_BLOCK : CMD_READ_SINGLE_BLOCK;  /* 18 : 17*/
	    pCmd->rsp_flag = CMD_RESP_FLAG_RDATA;	 /* 4*/
	}

    if (pSdmmcCard->highCapacity == 0)  
    {    
    	pCmd->cmd_arg = addr_offset << 9; /* block size: 512Bytes*/
    }
    else
    {    
    	pCmd->cmd_arg = addr_offset;      /* unit: block number  //hc, xc*/
    }
	
    pCmd->rsp_type = CMD_RESP_TYPE_R1;	
	
    pCmd->txRxBuf = pBuf;
    pCmd->data_len = len;	
    pCmd->block_size = 512;
	
	ret = sdmmc_sndCmd(pSdmmc, pCmd);
    if (ret < 0)
    {
		VX_DBG2("vxSdmmc_WrRd_OneSector: send CMD%d fail! \n", pCmd->cmd_idx,2,3,4,5,6);
        return ret;
    }	
	
	/* 
	check card-state. if card-state != StandBy, return BUSY 
	*/
    do
    {
        ret = sdmmc_cmd13(pSdmmc);
    } while (pCmd->rsp_buf[0] != 0x900);

    return len;
}

/*
UINT32 addr_offset: sector_idx
*/
int vxSdmmc_Wr_Sector(vxT_SDMMC* pSdmmc, UINT8* pBuf, UINT32 addr_offset, UINT32 len)
{
	return vxSdmmc_WrRd_OneSector(pSdmmc, pBuf, addr_offset, len, SDMMC_WRITE_MODE);
}


/*
UINT32 addr_offset: sector_idx
*/
int vxSdmmc_Rd_Sector(vxT_SDMMC* pSdmmc, UINT8* pBuf, UINT32 addr_offset, UINT32 len)
{
	return vxSdmmc_WrRd_OneSector(pSdmmc, pBuf, addr_offset, len, SDMMC_READ_MODE);
}



int vxInit_Sdmmc_2(int ctrl_x, int fifo_dma_mode)
{
	vxT_SDMMC * pSdmmc = NULL;
	
	vxT_SDMMC_CTRL * pSdmmcCtrl = NULL;
	vxT_SDMMC_CARD * pSdmmcCard = NULL;
	vxT_SDMMC_CMD  * pSdmmcCmd  = NULL;
	
	vxT_DMA_CTRL   * pSdmmcDMA  = NULL;
	
	int ret = 0;
	
	UINT32 tmp32 = 0;
    int timeout = SDMMC_POLL_TIMEOUT;
	int dma_msize = 0;
	int rx_wmark = 0, tx_wmark = 0;
	
    UINT8 cmd6_RdBuf[64] = { 0U };
	
#if 1
	/**/
	/* sdmmc_ctrl select*/
	/**/
    switch (ctrl_x)
	{
	case SDMMC_CTRL_0:
		pSdmmc = g_pSdmmc0;
		pSdmmc->pSdmmcCtrl = (vxT_SDMMC_CTRL*)(&vxSdmmc_Ctrl_0.ctrl_x);
		pSdmmc->pSdmmcCard = (vxT_SDMMC_CARD*)(&vxSdmmc_Card_0.status);
		pSdmmc->pSdmmcCmd  = (vxT_SDMMC_CMD*)(&vxSdmmc_Cmd_0.cmd_arg);
		pSdmmc->sdmmc_x = SDMMC_CTRL_0;
		break;
		
	case SDMMC_CTRL_1:
		pSdmmc = g_pSdmmc1;
		pSdmmc->pSdmmcCtrl = (vxT_SDMMC_CTRL*)(&vxSdmmc_Ctrl_1.ctrl_x);
		pSdmmc->pSdmmcCard = (vxT_SDMMC_CARD*)(&vxSdmmc_Card_1.status);
		pSdmmc->pSdmmcCmd  = (vxT_SDMMC_CMD*)(&vxSdmmc_Cmd_1.cmd_arg);
		pSdmmc->sdmmc_x = SDMMC_CTRL_1;
		break;
	}
	
	if (pSdmmc->init_flag == 1)
	{
		return 0;  /* init already*/
	}
	
	pSdmmcCtrl = pSdmmc->pSdmmcCtrl;
	pSdmmcCard = pSdmmc->pSdmmcCard;
	pSdmmcCmd  = pSdmmc->pSdmmcCmd;
	
	/*pSdmmc->pSdmmcDMA = g_pDma;*/
	/*pSdmmcDMA = pSdmmc->pSdmmcDMA;*/
	
    switch (ctrl_x)
	{
	case SDMMC_CTRL_0:
		pSdmmcCtrl->baseAddr = FMQL_SDMMC_0_BASE; 	/* Base address of the device */
		break;
		
	case SDMMC_CTRL_1:
		pSdmmcCtrl->baseAddr = FMQL_SDMMC_1_BASE; 	/* Base address of the device */
		break;
	}	
#endif

	/**/
	/* config init*/
	/**/
	pSdmmcCtrl->ctrl_x = ctrl_x;
	
	pSdmmcCtrl->sysClkHz = PS_SDIO_CLK_FREQ_HZ;		/* 100M Hz Input to sdmmc_ctrl clock frequency: sysclk of bus */
	pSdmmcCtrl->isCardDetect = 1;	                /* has Card Detect ? */
	pSdmmcCtrl->isWrProtect = 1;	                /* has Write Protect ? */
	
	pSdmmcCtrl->devIP_ver = 0x1000;                 /* 1.0.0.0 */
    pSdmmcCtrl->devIP_type = FMSH_DEVIP_APB_SDMMC;  /* identify peripheral types: FMSH_DEVIP_APB_SDMMC */

	pSdmmcCtrl->xferMode = MODE_XFER_FIFO;   /* default for init, init finish, can switch to DMA*/
	
	
	/**/
	/* dma pre_init*/
	/**/
	if (fifo_dma_mode == MODE_XFER_DMA)
	{
		vxInit_Dma_1();
		
		pSdmmc->pSdmmcDMA = g_pDma;
		pSdmmcDMA = pSdmmc->pSdmmcDMA;
	}
	
	/*///////////////////////////////////*/
	/**/
	/* sdmmc_ctrl init*/
	/**/
	/*///////////////////////////////////*/
	/**/
	/* reset ctrl*/
	/*	*/
    tmp32 = sdmmcCtrl_RdReg32(pSdmmc, SDMMC_CTRL) | CONTROLLER_RESET;
    sdmmcCtrl_WrReg32(pSdmmc, SDMMC_CTRL, tmp32);

    timeout = SDMMC_POLL_TIMEOUT;
    do
	{
        delay_1us();
        tmp32 = sdmmcCtrl_RdReg32(pSdmmc, SDMMC_CTRL) & CONTROLLER_RESET;
		
		timeout--;
		if (timeout <= 0)
		{
			VX_DBG2("sdmmc ctrl_%d reset ctrl fail, exit! \n", ctrl_x, 2,3,4,5,6);
			return (-FMSH_ETIME);
		}
    } while (tmp32);
	
	/**/
	/* reset fifo*/
	/**/
    tmp32 = sdmmcCtrl_RdReg32(pSdmmc, SDMMC_CTRL) | FIFO_RESET;
    sdmmcCtrl_WrReg32(pSdmmc, SDMMC_CTRL, tmp32);

    timeout = SDMMC_POLL_TIMEOUT;
    do
	{
        delay_1us();
		tmp32 = sdmmcCtrl_RdReg32(pSdmmc, SDMMC_CTRL) & FIFO_RESET;
		
		timeout--;
		if (timeout <= 0)
		{
			VX_DBG2("sdmmc ctrl_%d reset fifo fail, exit! \n", ctrl_x, 2,3,4,5,6);
			return (-FMSH_ETIME);
		}
    } while (tmp32);



	/*
	RX_WMark = (FIFO_DEPTH / 2) - 1
	TX_WMark =  FIFO_DEPTH / 2
	*/
	/**/
	/* set dma & fifo*/
	/*	*/
	dma_msize = 2; /* 010 - 8*/
    tmp32 = sdmmcCtrl_RdReg32(pSdmmc, SDMMC_FIFOTH) | (dma_msize << DW_DMA_MULTI_TRAN_SIZE_OFFSET);
    sdmmcCtrl_WrReg32(pSdmmc, SDMMC_FIFOTH, tmp32);
	
	/* MSize = 8，RX_WMark = 7, TX_WMark = 8*/
    tmp32 = sdmmcCtrl_RdReg32(pSdmmc, SDMMC_FIFOTH) & (~(RX_WMARK_MASK << RX_WMARK_OFFSET));
	rx_wmark = 7;
    tmp32 |= (rx_wmark << RX_WMARK_OFFSET);
    sdmmcCtrl_WrReg32(pSdmmc, SDMMC_FIFOTH, tmp32);
	/**/
    tmp32 = sdmmcCtrl_RdReg32(pSdmmc, SDMMC_FIFOTH) & (~(TX_WMARK_MASK << TX_WMARK_OFFSET));
	tx_wmark = 8;
    tmp32 |= (tx_wmark << TX_WMARK_OFFSET);
    sdmmcCtrl_WrReg32(pSdmmc, SDMMC_FIFOTH, tmp32);

	/**/
	/* clear irq*/
	/*	*/
    sdmmcCtrl_WrReg32(pSdmmc, SDMMC_RINTSTS, 0xFFFFFFFF);
	
	/* enable irq*/
    tmp32 = sdmmcCtrl_RdReg32(pSdmmc, SDMMC_CTRL) | 0x10; /*INT_ENABLE*/
    sdmmcCtrl_WrReg32(pSdmmc, SDMMC_CTRL, tmp32);
	
    /* mask interrupt: enable CD*/
    tmp32 = INT_MASK_CD;              /* 0-屏蔽中断; 1-使能中断 */
    sdmmcCtrl_WrReg32(pSdmmc, SDMMC_INTMASK, tmp32);

	/**/
	/* dma_mode*/
	/**/
	if (fifo_dma_mode == MODE_XFER_DMA)
	{		
		/*pSdmmcCtrl->xferMode = MODE_XFER_DMA;*/

		/**/
		/* enble DMA of SDMMC_Ctrl*/
		/**/
	    tmp32 = sdmmcCtrl_RdReg32(pSdmmc, SDMMC_CTRL);
		tmp32 |= DMA_ENABLE;
	    sdmmcCtrl_WrReg32(pSdmmc, SDMMC_CTRL, tmp32);

		/**/
		/* init dma*/
		/**/
		vxInit_Dma_2();
		
		/**/
		/* enable dma_ctrl*/
		/**/
		/**/
		dmaCtrl_Enable_Ctrl(pSdmmc->pSdmmcDMA);
	}

	/**/
    /* clear int*/
	/**/
	tmp32 = sdmmcCtrl_RdReg32(pSdmmc, SDMMC_RINTSTS);
    sdmmcCtrl_WrReg32(pSdmmc, SDMMC_RINTSTS, tmp32);

#if 1  /* card detect*/
	if (pSdmmcCtrl->isCardDetect == 1)
	{	
    	tmp32 = sdmmcCtrl_RdReg32(pSdmmc, SDMMC_CDETECT) & CARD_DETECT;
        if (tmp32 != CARD_INSERT)
        {
			VX_DBG2("sdmmc ctrl_%d: No card insert, exit! \n", ctrl_x, 2,3,4,5,6);
			return FMSH_FAILURE;
        }
		else
		{
			VX_DBG("sdmmc ctrl_%d: Card insert! \n", ctrl_x, 2,3,4,5,6);
		}
	}
	
    pSdmmcCard->status = 0;
	pSdmmcCard->rca = 0;
#endif

	/**/
	/* powerup*/
	/**/
#if 1
    /*sdmmcCtrl_WrReg32(pSdmmc, SDMMC_PWREN, 0xFFFF);*/
    sdmmcCtrl_WrReg32(pSdmmc, SDMMC_PWREN, 0xFF);
    delay_us(100);
#endif

	/**/
	/* setclock-400K*/
	/**/
	ret = sdmmcSet_Clk_DataRate(pSdmmc, SDMMC_CLK_400_KHZ);
	if (ret < 0)
	{
		VX_DBG2("sdmmc ctrl_%d: sdmmcSet_Clk_DataRate-400K fail, exit(%d)! \n", ctrl_x, ret,3,4,5,6);
		return ret;
	}
	else
	{
		VX_DBG("sdmmc ctrl_%d: sdmmcSet_Clk_DataRate-400K OK! \n", ctrl_x, 2,3,4,5,6);
	}
	
	delay_us(10);

#if 1	
    /* 
    go IDLE state 
	*/
    ret = sdmmc_cmd0(pSdmmc);
    if (ret < 0)
    {
		VX_DBG2("sdmmc ctrl_%d: sdmmc_cmd0 fail, exit(%d)! \n", ctrl_x, ret,3,4,5,6);
        return ret;
    }
    pSdmmcCard->card_ver = SDMMC_CARD_VER_1_0;
	
	delay_us(100);

	/*
	check whether ver2.0
	*/
    ret = sdmmc_cmd8_sd(pSdmmc);
    if (ret < 0)
    {
        pSdmmcCard->card_ver = SDMMC_CARD_VER_1_0;
		VX_DBG2("sdmmc ctrl_%d: sdmmc_cmd8_sd no response(%d)! \n", ctrl_x, ret,3,4,5,6);
    }
    else
    {
		VX_DBG("sdmmc ctrl_%d: sdmmc_cmd8_get response! \n", ctrl_x, 2,3,4,5,6);

        if((pSdmmcCmd->rsp_buf[0] & 0xFF) == (SD_CMD8_VOL_PATTERN & 0xFF))
        {
            pSdmmcCard->card_ver = SDMMC_CARD_VER_2_0;
			VX_DBG("sdmmc ctrl_%d: card ver2.0 0x%X \n", ctrl_x, pSdmmcCmd->rsp_buf[0], 3,4,5,6);
        }
        else 
        {
            pSdmmcCard->card_ver = SDMMC_CARD_VER_1_0;
			VX_DBG("sdmmc ctrl_%d: card ver1.0 0x%X \n", ctrl_x, pSdmmcCmd->rsp_buf[0], 3,4,5,6);
        }
    }

	/* 
	 check whether it's a MMC or a SD card. 
	*/
	
    /* APP_CMD */	
	VX_DBG("sdmmc ctrl_%d:sdmmc_cmd55->g_pSdmmcCard->rca(0x%X) \n", ctrl_x, pSdmmcCard->rca,3,4,5,6);
	
    ret = sdmmc_cmd55(pSdmmc);	
    if (ret < 0)
    {
        if (ret == (-FMSH_ENODEV))
        {
            return ret;
        }
        
		VX_DBG("sdmmc ctrl_%d: MultiMedia Card Identification: \n", ctrl_x, 2,3,4,5,6);
		
    	tmp32 = sdmmcCtrl_RdReg32(pSdmmc, SDMMC_CDETECT) & CARD_DETECT;		
        if (tmp32 == CARD_INSERT)
        {    
        	pSdmmcCard->card_type = MMC;
        }
        else
        {    
        	pSdmmcCard->card_type = EMMC;
        }		
    }
    else
    {
        pSdmmcCard->card_type = SD;
		VX_DBG("sdmmc ctrl_%d: SD Memory Card Identification: \n", ctrl_x,2,3,4,5,6);		
    }

	/**/
	/* enumerate the sdmmc*/
	/**/
	switch (pSdmmcCard->card_type)
	{
	case MMC:
	case EMMC:
        ret = sdmmc_Enumerate_MMC(pSdmmc);
		if (ret < 0)
		{
			VX_DBG2("sdmmc ctrl_%d: sdmmc_Enumerate_MMC fail,exit(%d)! \n", ctrl_x, ret, 3,4,5,6);
			return ret;
		}
		break;

	case SD:
        ret = sdmmc_Enumerate_SD(pSdmmc);
		if (ret < 0)
		{
			VX_DBG2("sdmmc ctrl_%d: sdmmc_Enumerate_SD fail,exit(%d)! \n", ctrl_x, ret, 3,4,5,6);
			return ret;
		}
		break;
	}
#endif


#if 1
	/**/
    /* switch to TRAN mode to Select the current SD/MMC*/
    /* SELECT CARD & set card state from Stand-by to Transfer*/
    /**/
    ret = sdmmc_cmd7(pSdmmc);
    if (ret < 0)
    {
		VX_DBG2("sdmmc ctrl_%d: sdmmc_cmd7 fail,exit(%d)! \n", ctrl_x, ret, 3,4,5,6);
        return ret;
    }
    if (pSdmmcCmd->rsp_buf[0] & (R1_CC_ERR | R1_ERR))
    {
		VX_DBG2("sdmmc ctrl_%d: CMD7 response fail,exit(%d)! \n", ctrl_x, ret, 3,4,5,6);
        return (-FMSH_EIO);
    }

    /* 
    select card bus width 
	*/
	if (pSdmmcCard->card_type == SD)
	{
		ret = sdmmcCard_Set_BusWidth(pSdmmc, SD_BUS_WIDTH_4);
		if (ret < 0)
		{
			VX_DBG2("sdmmc ctrl_%d: sdmmcCard_Set_BusWidth fail,exit(%d)! \n", ctrl_x, ret, 3,4,5,6);
			return ret;
		}
	}
	else  /* mmc / eMMC*/
	{
	    ret = sdmmcCard_Set_BusWidth_MMC(pSdmmc, SD_BUS_WIDTH_4);
	    if (ret < 0)
	    {
			VX_DBG2("sdmmc ctrl_%d: sdmmcCard_Set_BusWidth_MMC fail,exit(%d)! \n", ctrl_x, ret, 3,4,5,6);
	        return ret;
	    }

		delay_ms(10);
		
		ret = sdmmcCard_Get_ExtCsd_MMC(pSdmmc);
		if (ret < 0)
		{
			VX_DBG2("sdmmc ctrl_%d: Get MMC Ext_CSD info failed! \n", ctrl_x,2,3,4,5,6);
			return ret;
		}		
	}
		
    /* 
    Set high clock rate for the normal data transfer 
	*/
	ret = sdmmcSet_Clk_DataRate(pSdmmc, pSdmmcCard->info_csd.max_dtr);
    if (ret < 0)
    {
		VX_DBG2("sdmmc ctrl_%d: Switch the clock to %dHz for data transfer fail! \n", ctrl_x, pSdmmcCard->info_csd.max_dtr, 3,4,5,6);
        return ret;
    }
	else
	{
		VX_DBG("sdmmc ctrl_%d: Switch the clock to %dHz for data transfer OK! \n", ctrl_x, pSdmmcCard->info_csd.max_dtr, 3,4,5,6);
	}
#endif

#if 1
	/*pSdmmcCtrl->xferMode = MODE_XFER_FIFO;*/

	if (pSdmmcCard->card_type == SD)
    {
    	
        /* get sd card SCR info*/
        ret = sdmmcCard_Get_Info_SCR(pSdmmc);
        if (ret < 0)
        {
            VX_DBG2("SD Get SD's SCR info error! \n", 1,2,3,4,5,6);
            return ret;
        }

        if (pSdmmcCard->info_scr.sd_spec != 0)
        {
            ret = sdmmcCard_Get_BusSpeed(pSdmmc, cmd6_RdBuf);
            if (ret < 0)
            {
                return ret;
            }

            if ((cmd6_RdBuf[13] & HIGH_SPEED_SUPPORT) != 0)
            {
                ret = sdmmcChg_BusSpeed(pSdmmc);
                if (ret < 0)
                {
                    VX_DBG2("Failed to change SD to high speed mode! \n", 1,2,3,4,5,6);
                    return ret;
                }
                else
                { 
                	VX_DBG2("Change SD to high speed mode succeed! \n", 1,2,3,4,5,6);
                }
            }
        }
    }
    else if ((pSdmmcCard->card_type == MMC) || (pSdmmcCard->card_type == EMMC))
    {
        if ((pSdmmcCard->ext_csd[MMC_EXT_CSD_CARD_TYPE] & EXT_CSD_DEVICE_TYPE_HIGH_SPEED) != 0)
        {
			ret = sdmmcChg_BusSpeed_MMC(pSdmmc);
            if (ret < 0)
            {
				VX_DBG2("Failed to change MMC to high speed mode! \n", 1,2,3,4,5,6);
                return ret;
            }
            else
            {
            	VX_DBG2("Change MMC to high speed mode succeed! \n", 1,2,3,4,5,6);
            }
        }
        else
        {
			VX_DBG2("MMC doesn't support high speed mode! \n", 1,2,3,4,5,6);
        }
    }
#endif

    if (pSdmmcCard->highCapacity == 0)
    {    
    	sdmmcCard_Set_BlockSize(pSdmmc, SDMMC_BLOCK_SIZE);
		VX_DBG("sdmmc ctrl_%d: sdmmcCard_Set_BlockSize(%d) OK! \n", ctrl_x, SDMMC_BLOCK_SIZE, 3,4,5,6);
    }

	if (fifo_dma_mode == MODE_XFER_DMA)
	{
		pSdmmcCtrl->xferMode = MODE_XFER_DMA;
	}
	else
	{
		pSdmmcCtrl->xferMode = MODE_XFER_FIFO;
	}

	return FMSH_SUCCESS;
}


/*
UINT32 addr_offset: sector_idx

for fatFS, only return status
*/
int vxSdmmc_WrSector_Poll_fatFS(vxT_SDMMC* pSdmmc, UINT8 * pBuf, UINT32 addr_offset, UINT32 len)
{
	return vxSdmmc_WrRd_OneSector(pSdmmc, pBuf, addr_offset, len, SDMMC_WRITE_MODE);
}

/*
UINT32 addr_offset: sector_idx
*/
int vxSdmmc_RdSector_Poll_fatFS(vxT_SDMMC* pSdmmc, UINT8 * pBuf, UINT32 addr_offset, UINT32 len)
{
	return vxSdmmc_WrRd_OneSector(pSdmmc, pBuf, addr_offset, len, SDMMC_READ_MODE);
}


int vxInit_Sdmmc_Fifo(int ctrl_x)
{
	return vxInit_Sdmmc_2(ctrl_x, MODE_XFER_FIFO);
}

int vxInit_Sdmmc_Dma(int ctrl_x)
{
	if (ctrl_x == SDMMC_CTRL_1)
	{
		printf("SDMMC CTRL 1 only support fifo_mode, no dma_mode! \n");
		return vxInit_Sdmmc_2(ctrl_x, MODE_XFER_FIFO);
	}
	else
	{
		return vxInit_Sdmmc_2(ctrl_x, MODE_XFER_DMA);
	}
}

#endif



/*
test code & sample 
*/
#if 1

#if 0
static UINT8 g_buf[1024] = {0};
#else
/*UINT8*g_buf = (UINT8*FPS_AHB_SRAM_BASEADDR;*/
UINT8* g_buf = (UINT8*)0x20000000;
#endif

int g_test_sd = 0;

void test_sd_rd_sect(int ctrl_x, int sector)
{
	vxT_SDMMC * pSdmmc = NULL;	
	vxT_SDMMC_CARD *pSdmmcCard = NULL;
	
	/*UINT8 buf[1024] = {0};*/
	UINT8* buf = (UINT8*)(&g_buf[0]);
	
	int j = 0;

	/**/
	/* init sdmmc ctrl*/
	/**/
	/*vxInit_Sdmmc_Dma(ctrl_x);*/
	/*vxInit_Sdmmc_Fifo(ctrl_x);*/

	
	switch (ctrl_x)
	{
	case SDMMC_CTRL_0:
		pSdmmc = g_pSdmmc0; 
		break;
	
	case SDMMC_CTRL_1:
		pSdmmc = g_pSdmmc1; 
		break;
	}	
	pSdmmcCard = pSdmmc->pSdmmcCard;

	memset(buf, 0, 512);
		
	vxSdmmc_Rd_Sector(pSdmmc, (UINT8 *)(&buf[0]), sector, 512);

	printf("----sdmmc(%d)_%s-read(%d)----\n", pSdmmc->sdmmc_x, \
		  ((pSdmmc->pSdmmcCtrl->xferMode == MODE_XFER_DMA) ? "DMA" : "FIFO"), sector);	
	
	for (j=0; j<SDMMC_BLOCK_SIZE; j++)
	{
		printf("%02X", buf[j]);
		
		if ((j+1)%4 == 0)
		{
			printf(" ");
		}
		
		if ((j+1)%16 == 0)
		{
			printf(" \n");
		}
	}
	printf("------------\n");

	return;
}



void test_sd_wr_sect(int ctrl_x, int sector)
{
	vxT_SDMMC * pSdmmc = NULL;	
	vxT_SDMMC_CARD *pSdmmcCard = NULL;
	
	/*UINT8 buf[1024] = {0};	*/
	UINT8* buf = (UINT8*)(&g_buf[0]);
	
	int j = 0, byteCnt;
	
	/**/
	/* init sdmmc ctrl*/
	/**/
	/*vxInit_Sdmmc_Dma(ctrl_x);*/
	/*vxInit_Sdmmc_Fifo(ctrl_x);*/

	
	switch (ctrl_x)
	{
	case SDMMC_CTRL_0:
		pSdmmc = g_pSdmmc0; 
		break;
	
	case SDMMC_CTRL_1:
		pSdmmc = g_pSdmmc1; 
		break;
	}
	pSdmmcCard = pSdmmc->pSdmmcCard;
	

	for (j=0; j<SDMMC_BLOCK_SIZE; j++)
	{
		buf[j] = j + g_test_sd;
	}
	g_test_sd++;
		
	/* head*/
	*((UINT32*)(&buf[0])) = 0xabcd1234;
	*((UINT32*)(&buf[4])) = sector;

	/* tail*/
	byteCnt = 512;
	*((UINT32*)(&buf[byteCnt-8])) = sector;
	*((UINT32*)(&buf[byteCnt-4])) = 0xcdef6789;
	
	vxSdmmc_Wr_Sector(pSdmmc, (UINT8 *)(&buf[0]), sector, 512);
	
	printf("----sdmcc(%d)-write(%d)----\n", pSdmmc->sdmmc_x, sector);	

	for (j=0; j<SDMMC_BLOCK_SIZE; j++)
	{
		printf("%02X", buf[j]);
		
		if ((j+1)%4 == 0)
		{
			printf(" ");
		}
		
		if ((j+1)%16 == 0)
		{
			printf(" \n");
		}
	}
	printf("------------\n");

	return;
}

void test_mmc_w_r_sects(int from_sect, int to_sect)
{
	float speed = 0.0;
	UINT32 all_size = 0;
	int tick_start = 0, tick_end = 0;
	
	vxT_SDMMC * pSdmmc = NULL;	
	vxT_SDMMC_CARD *pSdmmcCard = NULL;
	
	int sect = 0;	
	
	/*int ctrl_x = SDMMC_CTRL_0;     // sd_ctrl_1: sd_card*/
	int ctrl_x = SDMMC_CTRL_1;   /* sd_ctrl_0: sd_card*/
	
	int sect_nums = (0x800000 / SDMMC_BLOCK_SIZE);  /* 8M*/
	int len = 0;
	
	UINT8 pBuf8[1024] = {0};  /*(UINT8*malloc(0x1000000);  // 16M*/
	UINT8 pBuf8_2[1024] = {0}; 
	int i = 0, j = 0;	
	int byteCnt = 512;

	/**/
 	/* init sdmmc*/
	/**/
	vxInit_Sdmmc_Fifo(ctrl_x);
	/*vxInit_Sdmmc_Dma(ctrl_x);*/
	
	switch (ctrl_x)
	{
	case SDMMC_CTRL_0:
		pSdmmc = g_pSdmmc0; 
		break;
	
	case SDMMC_CTRL_1:
		pSdmmc = g_pSdmmc1; 
		break;
	}	
	pSdmmcCard = pSdmmc->pSdmmcCard;

	for (sect=from_sect; sect<=to_sect; sect++)
	{
	
		for (j=0; j<SDMMC_BLOCK_SIZE; j++)
		{
			pBuf8[j] = j + g_test_sd;
		}
		g_test_sd++;
			
		/* head*/
		*((UINT32*)(&pBuf8[0])) = 0xabcd1234;
		*((UINT32*)(&pBuf8[4])) = sect;
		
		/* tail*/
		byteCnt = 512;
		*((UINT32*)(&pBuf8[byteCnt-8])) = sect;
		*((UINT32*)(&pBuf8[byteCnt-4])) = 0xcdef6789;
		
		/**/
		/* first read one sector*/
		/**/
		/*vxSdmmc_Rd_Sector(pSdmmc, (UINT8 *(&pBuf8[0]), sect, 512);*/

		/**/
		/* second write one sector*/
		/**/
		vxSdmmc_Wr_Sector(pSdmmc, (UINT8 *)(&pBuf8[0]), sect, 512);
		
		/**/
		/* third: read one sector again*/
		/**/
		vxSdmmc_Rd_Sector(pSdmmc, (UINT8 *)(&pBuf8_2[0]), sect, 512);
		
		/**/
		/* forth: compare*/
		/**/
		for (i=0; i<512; i++)
		{
			if (pBuf8[i] != pBuf8_2[i])
			{
				printf("compare err: idx-%d, wr-0x%02X<->rd-0x%02X \n", i, pBuf8[i], pBuf8_2[i]);
				break;
			}
		}

		if (i == 512)
		{
			printf(">>>wr&rd sector(%d) ok,no-err!>>> \n", sect);
		}
	}
	
	return;
}
#endif


#define __test_speed__
#if 1

/*
sd_ctrl_1: sd_card
*/
int file_idx = 0;
void test_sd_speed(void)
{
	float speed = 0.0;
	UINT32 all_size = 0;
	int tick_start = 0, tick_end = 0;
	
	vxT_SDMMC * pSdmmc = NULL;	
	vxT_SDMMC_CARD *pSdmmcCard = NULL;
	
	int sect = 0;	
	
	/*int ctrl_x = SDMMC_CTRL_0;     // sd_ctrl_1: sd_card*/
	int ctrl_x = SDMMC_CTRL_1;   /* sd_ctrl_0: sd_card*/
	
	int sect_nums = (0x800000 / SDMMC_BLOCK_SIZE);  /* 8M*/
	int len = 0;

	/**/
 	/* init sdmmc*/
	/**/
	vxInit_Sdmmc_Fifo(ctrl_x);
	/*vxInit_Sdmmc_Dma(ctrl_x);*/
	
	switch (ctrl_x)
	{
	case SDMMC_CTRL_0:
		pSdmmc = g_pSdmmc0; 
		break;
	
	case SDMMC_CTRL_1:
		pSdmmc = g_pSdmmc1; 
		break;
	}	
	pSdmmcCard = pSdmmc->pSdmmcCard;

	UINT8* pBuf8 = (UINT8*)malloc(0x1000000);  /* 16M*/

	/**/
	/* read*/
	/**/
	tick_start = tickGet();

#if 1
	len = SDMMC_BLOCK_SIZE;
	for (sect=0; sect<sect_nums; sect++)
	{
		vxSdmmc_Rd_Sector(pSdmmc, (UINT8 *)(&pBuf8[sect*512]), sect, len);
	}
#else
	len = sect_nums * 512;
	vxSdmmc_Rd_Sector(pSdmmc, (UINT8 *)(&pBuf8[0]), sect, len);
#endif
	
	tick_end = tickGet();	

	all_size = sect * SDMMC_BLOCK_SIZE;		
	speed = (float)(all_size * 1.0) / ((float)(tick_end - tick_start)*1.0 / sysClkRateGet());
	
	logMsg("read %d bytes data of bare-metal \r\n", all_size, 0,0,0,0,0);	
	printf("data: %d_bit mode, speed:%d Hz \n", sdmmcCtrl_Get_BusWidth(pSdmmc), sysMshcClkFreq_Get(ctrl_x));   /* sd_ctrl_0: sd_card*/

	printf("test sd_bare read_speed: %.2f bytes/s \n\n", speed);


	/**/
	/* write*/
	/*	*/
#if 1
	tick_start = tickGet();

#if 1
	len = SDMMC_BLOCK_SIZE;
	for (sect=0; sect<sect_nums; sect++)
	{
		vxSdmmc_Wr_Sector(pSdmmc, (UINT8 *)(&pBuf8[sect*512]), sect, len);
	}
#else
	len = sect_nums * SDMMC_BLOCK_SIZE;
	vxSdmmc_Wr_Sector(pSdmmc, (UINT8 *)(&pBuf8[0]), sect, len);
#endif
	
	tick_end = tickGet();	
	
	all_size = sect * SDMMC_BLOCK_SIZE;		
	speed = (float)(all_size * 1.0) / ((float)(tick_end - tick_start)*1.0 / sysClkRateGet());
	
	logMsg("write %d bytes data of bare-metal \r\n", all_size, 0,0,0,0,0);
	printf("data: %d_bit mode, speed:%d Hz \n", sdmmcCtrl_Get_BusWidth(pSdmmc), sysMshcClkFreq_Get(ctrl_x));   /* sd_ctrl_1: sd_card*/

	printf("test sd_bare write_speed: %.2f bytes/s \n", speed);
#endif

ERR:	
	free(pBuf8);	
	return;
}

/*
sd_ctrl_0: emmc
*/
void test_mmc_speed(void)
{
	float speed = 0.0;
	UINT32 all_size = 0;
	int tick_start = 0, tick_end = 0;
	
	vxT_SDMMC * pSdmmc = NULL;	
	vxT_SDMMC_CARD *pSdmmcCard = NULL;
	
	int sect = 0;	
	int ctrl_x = SDMMC_CTRL_0;   /* sd_ctrl_0: emmc chip	*/
	int sect_nums = (0x400000 / SDMMC_BLOCK_SIZE);  /* 4M*/
	
	int len = 0;
	
	/**/
 	/* init sdmmc*/
	/**/
	vxInit_Sdmmc_Fifo(ctrl_x);
	/*vxInit_Sdmmc_Dma(ctrl_x);*/
	
	switch (ctrl_x)
	{
	case SDMMC_CTRL_0:
		pSdmmc = g_pSdmmc0; 
		break;
	
	case SDMMC_CTRL_1:
		pSdmmc = g_pSdmmc1; 
		break;
	}	
	pSdmmcCard = pSdmmc->pSdmmcCard;

	UINT8* pBuf8 = (UINT8*)malloc(0x1000000);  /* 16M*/

	/**/
	/* read*/
	/**/
	tick_start = tickGet();
#if 1
	len = SDMMC_BLOCK_SIZE;
	for (sect=0; sect<sect_nums; sect++)
	{
		vxSdmmc_Rd_Sector(pSdmmc, (UINT8*)(&pBuf8[sect*512]), sect, len);
	}
#else
	len = sect_nums * SDMMC_BLOCK_SIZE;
	vxSdmmc_Rd_Sector(pSdmmc, (UINT8 *)(&pBuf8[0]), sect, len);
#endif

	all_size = sect * SDMMC_BLOCK_SIZE;	
	tick_end = tickGet();	
	
	speed = (float)(all_size * 1.0) / ((float)(tick_end - tick_start)*1.0 / sysClkRateGet());
	printf("read %d bytes data of bare-metal \r\n", all_size, 0,0,0,0,0);
	
	printf("data: %d_bit mode, speed:%d Hz \n", sdmmcCtrl_Get_BusWidth(pSdmmc), sysMshcClkFreq_Get(ctrl_x));   /* sd_ctrl_0: sd_card*/

	printf("test mmc_bare read_speed: %.2f bytes/s \n\n", speed);

#if 1	
	/**/
	/* write*/
	/*	*/
	tick_start = tickGet();
#if 1
	len = SDMMC_BLOCK_SIZE;
	for (sect=0; sect<sect_nums; sect++)
	{
		vxSdmmc_Wr_Sector(pSdmmc, (UINT8*)(&pBuf8[sect*512]), sect, len);
	}
#else
	len = sect_nums * SDMMC_BLOCK_SIZE;
	vxSdmmc_Wr_Sector(pSdmmc, (UINT8 *)(&pBuf8[0]), sect, len);
#endif


	tick_end = tickGet();	
	
	all_size = sect * SDMMC_BLOCK_SIZE;			
	speed = (float)(all_size * 1.0) / ((float)(tick_end - tick_start)*1.0 / sysClkRateGet());

	printf("write %d bytes data of bare-metal \r\n", all_size, 0,0,0,0,0);
	printf("data: %d_bit mode, speed:%d Hz \n", sdmmcCtrl_Get_BusWidth(pSdmmc), sysMshcClkFreq_Get(ctrl_x));   /* sd_ctrl_0: sd_card*/

	printf("test mmc_bare write_speed: %.2f bytes/s \n", speed);
#endif

ERR:	
	free(pBuf8);	
	return;
}

/*
-> test_mmc_speed
0x15841b0 (tShell0): read 6009968 bytes data of file(/mmc0:0/zImage) 
test mmc_dosfs read speed: 32781644.00 bytes/s 
0x15841b0 (tShell0): write 8388608 bytes data of file(/mmc0:0/test_3) 
data: 4_bit mode, speed:50000000 Hz 
test mmc_dosfs write_speed: 7626007.50 bytes/s 
value = 0 = 0x0
-> 
-> 
-> 
-> 
-> test_sd_speed
0x15841b0 (tShell0): read 6009968 bytes data of file(/sd0:0/zImage) 
test sd_dosfs read_speed: 30049840.00 bytes/s 
0x15841b0 (tShell0): write 8388608 bytes data of file(/sd0:0/test_4) 
data: 4_bit mode, speed:50000000 Hz 
test sd_dosfs write_speed: 14380471.00 bytes/s 
value = 0 = 0x0
-> 

*/
#endif


#define __fatFS__

#ifdef ENABLE_FATFS_SDMMC

FIL fil;		/* File object */
FATFS fatfs;

static int g_sdmmc_fs_mount = 0;


#if 1 /* fatFS_v2*/

/*BYTE Buff[4096];	/* Working buffer */

/*FATFS FatFs[2];		 Filesystem object for each logical drive */
/*FIL File[2];		/* File object */

DIR Dir;			/* Directory object */
FILINFO Finfo;
DWORD AccSize;				/* Work register for fs command */
WORD AccFiles, AccDirs;

/*BYTE RtcOk;				/* RTC is available */
/*volatile UINT Timer;	 Performance timer (100Hz increment) */

FRESULT scan_files (char* path)
{
	DIR dirs;
	FRESULT fr;
	int i;

	fr = f_opendir(&dirs, path);
	
	if (fr == FR_OK) 
	{
		while (((fr = f_readdir(&dirs, &Finfo)) == FR_OK) && Finfo.fname[0]) 
		{
			if (Finfo.fattrib & AM_DIR) 
			{
				AccDirs++;
				i = strlen(path);
				
				path[i] = '/'; strcpy(path+i+1, Finfo.fname);
				
				fr = scan_files(path);
				
				path[i] = 0;
				
				if (fr != FR_OK) 
					break;
			} 
			else 
			{
				printf("%s/%s\n", path, Finfo.fname);
				
				AccFiles++;
				AccSize += Finfo.fsize;
			}
		}
	}

	return fr;
}

#endif

#if 1

void format1_fs(void)
{
	/* Initialize a brand-new disk drive mapped to physical drive 0 */
	
	BYTE work[FF_MAX_SS];		  /* Working buffer */
	
	/*LBA_t plist[] = {0x10000000, 100};  */
	DWORD plist[] = {0x10000000, 100};  
				 /* Divide the drive into two partitions */
				 /* {0x10000000, 100}; 256M sectors for 1st partition and left all for 2nd partition */
				 /* {20, 20, 20, 0}; 20% for 3 partitions each and remaing space is left not allocated */

	f_fdisk(0, plist, work);					/* Divide physical drive 0 */

	f_mkfs("0:", FM_FAT32, work, sizeof work); /* Create FAT volume on the logical drive 0 */
	/*f_mkfs("0:", FM_FAT32, 0, work, sizeof(work)); /* Create FAT volume on the logical drive 1 */
	return;
}

void sd_fs_mount(void)
{
	FRESULT res_sd;
	UINT8 work[FF_MAX_SS*4] = {0};   /* Work area (larger is better for process time) */

	if (g_sdmmc_fs_mount == 0)
	{
		g_sdmmc_fs_mount = 1;

		res_sd = f_mount(&fatfs, "0:/", 0);
	
		/* 
		Format and create file system if no file in SD card 
		*/
		if (res_sd == FR_NO_FILESYSTEM)
		{
			TRACE_OUT(DEBUG_OUT, "> No file in SD card, will format first...\r\n");
#if 0
			
			/* Format */
			res_sd = f_mkfs("0:", FM_FAT32, 0, work, sizeof(work));
#endif
			if (res_sd == FR_OK)
			{
				TRACE_OUT(DEBUG_OUT, "> SD card format succeed.\r\n");
				
				/* cancel mount after format */
				res_sd = f_mount(NULL, "0:", 0);
				
				/* remount */
				res_sd = f_mount(&fatfs, "0:", 0);
			}
			else
			{
				TRACE_OUT(DEBUG_OUT, "> SD card format failed.\r\n");
				return FMSH_FAILURE;
			}
		}
		else 
		if (res_sd != FR_OK)
		{
			TRACE_OUT(DEBUG_OUT, "> SD card mount file system failed.(%d)\r\n", res_sd);
			return FMSH_FAILURE;
		}
		else
		{
			TRACE_OUT(DEBUG_OUT, "> File system mount succeed, begin to write/read test.\r\n");
		}
	}

	return;
}


/*
cmd example:
------------
sdls2 "0:"
sdls2 "0:/"
*/
void sdls2(char * pPathName)
{
	/*FRESULT res_sd;*/
	/*UINT8 work[FF_MAX_SS*]; /* Work area (larger is better for process time) */
	
    int isDir = 0;
    int i = 0;
	
    FRESULT fr;     /* Return value */
    DIR dj;         /* Directory object */
    FILINFO fno;    /* File information */	

	/*
	mount sd FatFS
	*/
	sd_fs_mount();

	/*
	judge dir ?
	*/
	i = 0;
	do
	{
		if ((pPathName[i] == '/') || (pPathName[i] == ':'))
		{
			isDir = 1;
			break;
		}

		i++;
	} while (pPathName[i] != '\0');

	if (isDir == 1)
	{
		scan_files(pPathName);
	}
	else
	{	
	   /* fr = f_findfirst(&dj, &fno, "0:", "**);  /* Start to search for photo files */
	    fr = f_findfirst(&dj, &fno, "0:", pPathName);  /* Start to search for photo files */
		if (fr != FR_OK)
		{
			printf("empty:%d! \n", fr);
			/*f_closedir(&dj);*/
			return;
		}

	    while ((fr == FR_OK) && fno.fname[0])           /* Repeat while an item is found */
		{         
	        printf("%s \n", fno.fname);               /* Print the object name */
	        fr = f_findnext(&dj, &fno);               /* Search for next item */
	    }

	    /*f_closedir(&dj);*/
	}

	return;
}

void test_wr_sd_file(char* file_name)
{
	FRESULT res_sd;
	UINT bw;
	
	int i;
	u8 *src, *dst; 
	int dataLen;
	u8 fileName[64] = {0};

	src = (u8*)FPS_AHB_SRAM_BASEADDR;
	dst = (u8*)(FPS_AHB_SRAM_BASEADDR + 0x10000);
	dataLen = 512;

	for(i=0; i<dataLen; i++)
    {
    	*src = i + 'a';
		src++;
    }
	
	src = (u8*)FPS_AHB_SRAM_BASEADDR;	
	sprintf(src, "%s", file_name);

	memset(dst, 0, dataLen * sizeof(u8));
	
	/*
	--------------------- Write test -----------------------
	*/
	
	/*
	mount sd FatFS
	*/	
	sd_fs_mount();
	
	/* 
	Open file or create file if no file in SD card 
	*/
	sprintf((char*)(&fileName[0]), "0:%s", file_name);
	
	/*res_sd = f_open(&fil, "0:test.txt", FA_CREATE_ALWAYS|FA_WRITE);*/
	res_sd = f_open(&fil, (char*)(&fileName[0]), (FA_CREATE_ALWAYS|FA_WRITE));	
	if (res_sd == FR_OK)
	{
		printf("> Open/Create file: %s succeed! \r\n", (char*)(&fileName[0]));
		
		/* 
		Write data to the file 
		*/
		res_sd = f_write(&fil, (void*)src, dataLen, &bw);
		
		if (res_sd == FR_OK)
		{
			printf("> File(%s) write succeed, the write data is: %d \r\n", (char*)(&fileName[0]), bw);
		}
		else
		{
			printf("> File(%s) write failed: (%d) \r\n", (char*)(&fileName[0]), res_sd);
		}
		
		/* close the file */
		res_sd = f_close(&fil);
		if (res_sd != FR_OK)
		{
			return;
		}
	}
	else
	{
		printf("> File(%s) create failed: (%d) \r\n", (char*)(&fileName[0]), res_sd);
	}

	return;
}

void test_rd_sd_file(char* file_name)
{
	FRESULT res_sd;
	UINT bw;
	
	int i;
	u8 *src, *dst; 
	int dataLen;
	u8 fileName[64] = {0};

	src = (u8*)FPS_AHB_SRAM_BASEADDR;
	dst = (u8*)(FPS_AHB_SRAM_BASEADDR + 0x10000);
	dataLen = 512;
	
	src = (u8*)FPS_AHB_SRAM_BASEADDR;	
	sprintf(src, "%s", file_name);

	memset(dst, 0, dataLen * sizeof(u8));

	/*
	------------------ Read test --------------------------
	*/	
	
	/*
	mount sd FatFS
	*/	
	sd_fs_mount();
	
	sprintf((char*)(&fileName[0]), "0:%s", file_name);	
	res_sd = f_open(&fil, (char*)(&fileName[0]), (FA_OPEN_EXISTING|FA_READ));
	if (res_sd == FR_OK)
	{
		printf("> open file(%s) succeed. \r\n", (char*)(&fileName[0]));
		
		res_sd = f_read(&fil, (void*)dst, dataLen, &bw);
		
		if (res_sd == FR_OK)
		{
			printf("> File(%s) read succeed, the read data is: %d\r\n", (char*)(&fileName[0]), bw);
		}
		else
		{
			printf("> File(%s) read failed: (%d)\n", (char*)(&fileName[0]), res_sd);
		}
	}
	else
	{
		printf("> File(%s) open failed: (%d) \r\n", (char*)(&fileName[0]), res_sd);
	}

	for (i=0; i<dataLen; i++)
	{
		printf("%02X", dst[i]);

		if ((i+1)%8 == 0)
		{
			printf(" ");
		}
		
		if ((i+1)%32 == 0)
		{
			printf("\n");
		}
	}
	
	/* 
	close the file 
	*/
	res_sd = f_close(&fil);
	if (res_sd != FR_OK)
	{
		return;
	}

	return;
}

#endif

#if 1

/*
cmd example:
------------
sdls "0:"
sdls "0:/"
sdls "0:/sd_45"
*/

/*
-> sdls "0:/"
 -att-      -time-       -size-     -name-   
 ------------------------------------------- 
 D-HS- 2020/04/13 21:58         0  SYSTEM~1
 ----A 2010/01/01 00:00       512  TEST1.TXT
 ----A 2010/01/01 00:00       512  TEST2.TXT
 D---- 2020/04/26 15:59         0  ZCU102
 D---- 2020/04/26 16:18         0  ZC706_~1
 D---- 2020/05/22 14:54         0  706
 D---- 2020/05/22 14:53         0  SD_45
 ----A 2020/01/09 13:43     16294  SYSTEM~1.DTB
 ----A 2020/03/12 16:13       174  UENV.TXT
 ----A 2018/11/20 13:55  22924246  U-ROOT~1
 ----A 2020/05/15 19:26   4162936  ZIMAGE
 ----A 2018/10/10 09:19   8407412  DWC_GM~1.PDF
 ----A 2020/05/27 10:18    228914  GMA_DMA.PDF
 ----A 2020/05/27 10:30    445713  BINDER~1.PDF
 ----A 2019/12/23 11:19    748672  BOOT.BIN
 ----A 2019/12/23 11:23  13321503  PL_TOP.BIT
 
  11 File(s),  50256888 bytes total
   5 Dir(s),    7517056KiB free
value = 21 = 0x15
*/

BYTE Buff[4096];	/* Working buffer */

void sdls(char* pPathName)
{
	char line[120];
	char *ptr, *ptr2;
	long p1, p2, p3;
	LBA_t lba;
	FRESULT fr;
	BYTE b1, *bp;
	UINT s1, s2, cnt, blen = sizeof Buff;
	DWORD ofs, sect = 0;
	static const char* const fst[] = {"", "FAT12", "FAT16", "FAT32", "exFAT"};
	FATFS *fs;

	/*
	mount sd FatFS
	*/
	sd_fs_mount();
	
	/*while (*tr == ' ') */
	/*		ptr++;*/
	ptr = pPathName;
					
	fr = f_opendir(&Dir, ptr);
	if (fr) 
	{
		printf("f_opendir fail(%d) \n", fr); 
		printf("shell cmd: sdls \"0:/\" or sdls \"0:\" \n");
		return;
	}
	
	p1 = s1 = s2 = 0;
	
	printf(" -att-      -time-       -size-     -name-   \n");
	printf(" ------------------------------------------- \n");
		  /* -att- ------time------ --size--- ---name--- 
		 	 D-HS- 2020/04/13 21:58         0  SYSTEM~1
		 	 */
	for (;;) 
	{
		fr = f_readdir(&Dir, &Finfo);
		
		if ((fr != FR_OK) || !Finfo.fname[0]) 
		{	
			break;
		}
		
		if (Finfo.fattrib & AM_DIR) 
		{
			s2++;
		} 
		else 
		{
			s1++; 
			p1 += Finfo.fsize;
		}		
		
		printf(" %c%c%c%c%c %u/%02u/%02u %02u:%02u %9lu  %s\n", 
					(Finfo.fattrib & AM_DIR) ? 'D' : '-',
					(Finfo.fattrib & AM_RDO) ? 'R' : '-',
					(Finfo.fattrib & AM_HID) ? 'H' : '-',
					(Finfo.fattrib & AM_SYS) ? 'S' : '-',
					(Finfo.fattrib & AM_ARC) ? 'A' : '-',
					(Finfo.fdate >> 9) + 1980, (Finfo.fdate >> 5) & 15, Finfo.fdate & 31,
					(Finfo.ftime >> 11), (Finfo.ftime >> 5) & 63,
					(DWORD)Finfo.fsize, 
					Finfo.fname);
	}
	
	printf(" \n");
	
	if (fr == FR_OK) 
	{
		printf("%4u File(s),%10lu bytes total\n%4u Dir(s)", s1, p1, s2);
		
		if (f_getfree(ptr, (DWORD*)&p1, &fs) == FR_OK) 
		{
			printf(", %10luKiB free\n", p1 * fs->csize / 2);
		}
	}
	
	if (fr) 
	{
		printf("f_readdir fail(%d) \n", fr); 		
	}

	return;
}

/*
cmd example:
------------
sdcd "0:/"
sdcd "0:/sd_45"
sdcd "sd_45"
*/
/* 
<pPathName> - Change current directory
*/
void sdcd(char* pPathName)
{
	char *ptr;
	FRESULT fr;

	/* fg <path> - Change current directory */
	/*while (*tr == ' ') */
	/*	ptr++;*/
	ptr = pPathName;
	
	fr = f_chdir(ptr);
	
	if (fr == FR_OK) 
	{
		printf("sdcd to: %s ok! \n", pPathName);
	}
	else
	{
		printf("sdcd to: %s fail! \n", pPathName);
	}

	return;
}


/* 
Show current dir path
*/
void sdpwd(void)
{
	char line[120];
	char *ptr;
	FRESULT fr;
	
	/* fq - Show current dir path */
	fr = f_getcwd(line, sizeof(line));
	
	if (fr == FR_OK) 
	{
		printf("%s\n", line);
	} 
	else
	{
		printf("sdpwd fail! \n");
	}

	return;
}

/*
sd disk_status show:
待完善...
*/
void sdds(void)
{
	char line[120];
	char *ptr, *ptr2;
	long p1, p2, p3;
	LBA_t lba;
	FRESULT fr;
	BYTE b1, *bp;
	UINT s1, s2, cnt, blen = sizeof Buff;
	DWORD ofs, sect = 0;
	static const char* const fst[] = {"", "FAT12", "FAT16", "FAT32", "exFAT"};
	FATFS *fs;
	int i = 0;
	
	/* ds <pd#> - Show disk status */
	/*if (!xatoi(&ptr, &p1)) */
	/*	break;*/
	p1 = 0;
	
	if (disk_ioctl((BYTE)p1, GET_SECTOR_COUNT, &lba) == RES_OK) 
	{
		printf("Drive size: %lu sectors \n", (DWORD)lba);
	}
	
	if (disk_ioctl((BYTE)p1, GET_BLOCK_SIZE, &p2) == RES_OK) 
	{
		printf("Erase block: %lu sectors \n", p2);
	}
	
	if (disk_ioctl((BYTE)p1, MMC_GET_TYPE, &b1) == RES_OK) 
	{
		printf("Card type: %u \n", b1);
	}
	
	if (disk_ioctl((BYTE)p1, MMC_GET_CSD, Buff) == RES_OK)
	{
		printf("CSD: 0x"); 
		for (i=0; i<16; i++)
		{
			printf("%02X", Buff[i]);
			if (i == 8)
			{
				printf("_");
			}
		}
		printf("\n");
	}
	
	if (disk_ioctl((BYTE)p1, MMC_GET_CID, Buff) == RES_OK) 
	{
		printf("CID: 0x"); 
		for (i=0; i<16; i++)
		{
			printf("%02X", Buff[i]);
			if (i == 8)
			{
				printf("_");
			}
		}
		printf("\n");
	}
	
	if (disk_ioctl((BYTE)p1, MMC_GET_OCR, Buff) == RES_OK) 
	{
		printf("OCR: 0x"); 
		for (i=0; i<4; i++)
		{
			printf("%02X", Buff[i]);
			if (i == 8)
			{
				printf("_");
			}
		}
		printf("\n");
	}
	
	if (disk_ioctl((BYTE)p1, MMC_GET_SDSTAT, Buff) == RES_OK)
	{
		printf("SD Status: ");
		for (s1 = 0; s1 < 64; s1 += 16) 
		{	
			/*put_dump(Buff+s1, s1, 16);			*/
			for (i=0; i<16; i++)
			{
				printf("%02X", Buff[i+s1]);
				if (i == 8)
				{
					printf("_");
				}
			}
			printf("\n");
		}
	}
	
	if (disk_ioctl((BYTE)p1, ATA_GET_MODEL, line) == RES_OK) 
	{
		line[40] = '\0'; 
		printf("Model: %s \n", line);
	}
	
	if (disk_ioctl((BYTE)p1, ATA_GET_SN, line) == RES_OK) 
	{
		line[20] = '\0'; 
		printf("S/N: %s \n", line);
	}

	return;
}

#endif
#endif


