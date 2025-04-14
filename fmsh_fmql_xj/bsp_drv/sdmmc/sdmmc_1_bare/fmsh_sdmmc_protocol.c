/******************************************************************************
*
* Copyright (C) 2018 - 2028 FMSH, Inc.  All rights reserved.
*
******************************************************************************/
/******************************************************************************
*
* @file  fmsh_sdmmc_protocol.c
*
* This file contains
*
* @note		None.
*
* MODIFICATION HISTORY:
*
*<pre>
* Ver   Who  Date     Changes
* ----- ---- -------- ---------------------------------------------
* 0.01   yl  12/20/2018  First Release
*</pre>
******************************************************************************/
/***************************** Include Files *********************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fmsh_sdmmc_lib.h"

/************************** Constant Definitions *****************************/

static const unsigned int tran_exp[] = {
        100000,         1000000,                10000000,       100000000,
        0,              0,              0,              0
};

static const unsigned char tran_mant[] = {
        0,      10,     12,     13,     15,     20,     25,     30,
        35,     40,     45,     50,     55,     60,     70,     80,
};

static const unsigned int taac_exp[] = {
	1,	10,	100,	1000,	10000,	100000,	1000000, 10000000,
};

static const unsigned int taac_mant[] = {
	0,	10,	12,	13,	15,	20,	26,	30,
	35,	40,	45,	52,	55,	60,	70,	80,
};

/**************************** Type Definitions *******************************/

/***************** Macros (Inline Functions) Definitions *********************/

/************************** Variable Definitions *****************************/

/************************** Function Prototypes ******************************/

static int UNSTUFF_BITS(u32 *rsp_buf, unsigned int start, unsigned int size)
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

/*****************************************************************************
*
* @description
* This function decodes the raw CID information.
*
* @param    
*           pSdmmc is the pointer to a sd card driver instance.
*
* @return	
*           Returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note		NA.
*
*****************************************************************************/
int SDMMC_decodeCID(FSdmmcPs_T *pSdmmc)
{
	u32 *pResp;
	SDMMC_cid_T *pCid;
	FSdmmcPs_Instance_T *pInstance;
	
	SDMMC_COMMON_REQUIREMENTS(pSdmmc);
	
	pInstance = pSdmmc->instance;
	pResp = pInstance->raw_cid;
	pCid = &pInstance->cid;
	
	if (pInstance->card_type == SD)
	{
		/*
		 * SD doesn't currently have a version field so we will
		 * have to assume we can parse this.
		 */
		pCid->manfid		= UNSTUFF_BITS(pResp, 120, 8);
		pCid->oemid			= UNSTUFF_BITS(pResp, 104, 16);
		pCid->prod_name[0]		= UNSTUFF_BITS(pResp, 96, 8);
		pCid->prod_name[1]		= UNSTUFF_BITS(pResp, 88, 8);
		pCid->prod_name[2]		= UNSTUFF_BITS(pResp, 80, 8);
		pCid->prod_name[3]		= UNSTUFF_BITS(pResp, 72, 8);
		pCid->prod_name[4]		= UNSTUFF_BITS(pResp, 64, 8);
		pCid->hwrev			= UNSTUFF_BITS(pResp, 60, 4);
		pCid->fwrev			= UNSTUFF_BITS(pResp, 56, 4);
		pCid->serial		= UNSTUFF_BITS(pResp, 24, 32);
		pCid->year			= UNSTUFF_BITS(pResp, 12, 8);
		pCid->month			= UNSTUFF_BITS(pResp, 8, 4);

		pCid->year += 2000; /* SD cards year offset */
	}
	else
	{
		/*
		 * The selection of the format here is based upon published
		 * specs from sandisk and from what people have reported.
		 */
		switch (pInstance->csd.mmc_version)
		{
			case CSD_SPEC_VER_0: /* MMC v1.0 - v1.2 */
			case CSD_SPEC_VER_1: /* MMC v1.4 */
				pCid->manfid	= UNSTUFF_BITS(pResp, 104, 24);
				pCid->prod_name[0]	= UNSTUFF_BITS(pResp, 96, 8);
				pCid->prod_name[1]	= UNSTUFF_BITS(pResp, 88, 8);
				pCid->prod_name[2]	= UNSTUFF_BITS(pResp, 80, 8);
				pCid->prod_name[3]	= UNSTUFF_BITS(pResp, 72, 8);
				pCid->prod_name[4]	= UNSTUFF_BITS(pResp, 64, 8);
				pCid->prod_name[5]	= UNSTUFF_BITS(pResp, 56, 8);
				pCid->prod_name[6]	= UNSTUFF_BITS(pResp, 48, 8);
				pCid->hwrev		= UNSTUFF_BITS(pResp, 44, 4);
				pCid->fwrev		= UNSTUFF_BITS(pResp, 40, 4);
				pCid->serial	= UNSTUFF_BITS(pResp, 16, 24);
				pCid->month		= UNSTUFF_BITS(pResp, 12, 4);
				pCid->year		= UNSTUFF_BITS(pResp, 8, 4) + 1997;
				break;

			case CSD_SPEC_VER_2: /* MMC v2.0 - v2.2 */
			case CSD_SPEC_VER_3: /* MMC v3.1 - v3.3 */
			case CSD_SPEC_VER_4: /* MMC v4 */
				pCid->manfid	= UNSTUFF_BITS(pResp, 120, 8);
				pCid->oemid		= UNSTUFF_BITS(pResp, 104, 16);
				pCid->prod_name[0]	= UNSTUFF_BITS(pResp, 96, 8);
				pCid->prod_name[1]	= UNSTUFF_BITS(pResp, 88, 8);
				pCid->prod_name[2]	= UNSTUFF_BITS(pResp, 80, 8);
				pCid->prod_name[3]	= UNSTUFF_BITS(pResp, 72, 8);
				pCid->prod_name[4]	= UNSTUFF_BITS(pResp, 64, 8);
				pCid->prod_name[5]	= UNSTUFF_BITS(pResp, 56, 8);
				pCid->serial	= UNSTUFF_BITS(pResp, 16, 32);
				pCid->month		= UNSTUFF_BITS(pResp, 12, 4);
				pCid->year		= UNSTUFF_BITS(pResp, 8, 4) + 1997;
				break;

			default:
				return -FMSH_EINVAL;
		}
	}
	return 0;
}

/*****************************************************************************
*
* @description
* This function decodes the raw CSD information.
*
* @param    
*           pSdmmc is the pointer to a sd card driver instance.
*
* @return	
*           Returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note		NA.
*
*****************************************************************************/
int SDMMC_decodeCSD(FSdmmcPs_T *pSdmmc)
{
	u32 e, m;
	u32 *pResp;
	SDMMC_csd_T *pCsd;
	FSdmmcPs_Instance_T *pInstance;
	
	SDMMC_COMMON_REQUIREMENTS(pSdmmc);
	
	pInstance = pSdmmc->instance;
	pResp = pInstance->raw_csd;
	pCsd = &pInstance->csd;

	pCsd->csd_struct = UNSTUFF_BITS(pResp, 126, 2);
	if ((pInstance->card_type == MMC) || (pInstance->card_type == EMMC))
	{
		if (pCsd->csd_struct == 0)
		{
			return -FMSH_EINVAL;
		}

		pInstance->csd.mmc_version = UNSTUFF_BITS(pResp, 122, 4);
		switch (pInstance->csd.mmc_version) {
		case CSD_SPEC_VER_0:
			pInstance->version = MMC_VERSION_1_2;
			break;
		case CSD_SPEC_VER_1:
			pInstance->version = MMC_VERSION_1_4;
			break;
		case CSD_SPEC_VER_2:
			pInstance->version = MMC_VERSION_2_2;
			break;
		case CSD_SPEC_VER_3:
			pInstance->version = MMC_VERSION_3;
			break;
		case CSD_SPEC_VER_4:
			pInstance->version = MMC_VERSION_4;
			break;
		default:
			pInstance->version = MMC_VERSION_1_2;
			break;
		}
	}
	m = UNSTUFF_BITS(pResp, 115, 4);
	e = UNSTUFF_BITS(pResp, 112, 3);
	pCsd->taac_ns	 = (taac_exp[e] * taac_mant[m] + 9) / 10;
	pCsd->taac_clks	 = UNSTUFF_BITS(pResp, 104, 8) * 100;
	
	m = UNSTUFF_BITS(pResp, 99, 4);
	e = UNSTUFF_BITS(pResp, 96, 3);
	pCsd->max_dtr	  = tran_exp[e] * tran_mant[m]/10;
	pCsd->cmdclass	  = UNSTUFF_BITS(pResp, 84, 12);
	
	if(pInstance->HighCapacity == 0)
	{
		e = UNSTUFF_BITS(pResp, 47, 3);
		m = UNSTUFF_BITS(pResp, 62, 2) | (UNSTUFF_BITS(pResp, 64, 10) << 2);
		pCsd->blockNR	  = (1 + m) << (e + 2);
	}
	else
	{
		m = UNSTUFF_BITS(pResp, 48, 22);
		pCsd->blockNR = (1 + m) << 10;
	}
	pCsd->read_blkbits = UNSTUFF_BITS(pResp, 80, 4);
	pCsd->read_partial = UNSTUFF_BITS(pResp, 79, 1);
	pCsd->write_misalign = UNSTUFF_BITS(pResp, 78, 1);
	pCsd->read_misalign = UNSTUFF_BITS(pResp, 77, 1);
	pCsd->r2w_factor = UNSTUFF_BITS(pResp, 26, 3);
	pCsd->write_blkbits = UNSTUFF_BITS(pResp, 22, 4);
	pCsd->write_partial = UNSTUFF_BITS(pResp, 21, 1);
	
	return 0;
}

/*****************************************************************************
*
* @description
* This function gets card information from the CSD structure.
*
* @param    
*           pSdmmc is the pointer to a sd card driver instance.
*
* @return	
*           Returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note		NA.
*
*****************************************************************************/
int SDMMC_getCSDInfo(FSdmmcPs_T *pSdmmc) 
{
	SDMMC_csd_T csd;
	FSdmmcPs_Instance_T *pInstance;
	
	SDMMC_COMMON_REQUIREMENTS(pSdmmc);
	
	pInstance = pSdmmc->instance;
	csd = pInstance->csd;
	
	/* read/write block length */
	pInstance->read_blk_len	= 1 << csd.read_blkbits;
	pInstance->write_blk_len = 1 << csd.write_blkbits;

	printf("SDMMC_getCSDInfo->pInstance->read_blk_len:%d \n", pInstance->read_blk_len);
	printf("SDMMC_getCSDInfo->pInstance->write_blk_len:%d \n", pInstance->write_blk_len);

	if (pInstance->read_blk_len != pInstance->write_blk_len)
	{
		return -FMSH_EINVAL;
	}

	/* partial block read/write I/O support */
	if (csd.read_partial) pInstance->stat |= MMC_READ_PART;
	if (csd.write_partial) pInstance->stat |= MMC_WRITE_PART;

	/* calculate total card size in bytes */
    if (pInstance->HighCapacity == 1)
		pInstance->capacity = (u64)pInstance->csd.blockNR * 512;  /* SDHC/SDXC may larger than 32bits*/
	else
		pInstance->capacity = pInstance->csd.blockNR * pInstance->read_blk_len;

	return 0;
}


