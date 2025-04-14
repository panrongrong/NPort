#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "config.h"

#include "../../common/fmsh_ps_parameters.h"
#include "../../common/fmsh_common.h"
#include "../../slcr/fmsh_slcr.h"


/*#include "pc_interface.h"*/
#include "fmsh_sdmmc_lib.h"
#include "fmsh_sdmmc_verify.h"
/*#include "fmsh_uart_mix.h"*/
/*#include "fmsh_dmac_verify.h"*/

#include "sd.h"

#include "./fatFS/ff.h"

/*#include "fmsh_gic.h"*/
/*#include "fmsh_gic_hw.h"*/

/*#define SDMMC_SELF_TEST*/
/*#define GTC_TIME_OUTPUT*/

/* Declare global variables*/
FSdmmcPs_T g_SDMMC[BOARD_NUM_SDMMC];
FSdmmcPs_Instance_T g_SDMMC_instance[BOARD_NUM_SDMMC];
SDMMC_dataParam g_SDMMC_dataParam;

extern FIL fil;		/* File object */
extern FATFS fatfs;
/** Number of errors found */
static u32 nbErrors;

/**
 * \brief Max Error Break
 * Check if max number of error achieved.
 * \param halt Whether halt the device if error number achieved.
 */
static u8 MaxErrorBreak(u8 halt)
{
    if (NB_ERRORS)
    {
        if (nbErrors ++ > NB_ERRORS)
        {

            while(halt);

            nbErrors = 0;
            return 1;
        }
    }
    return 0;
}

int FSdmmcPs_irqHandler(FSdmmcPs_T *pSdmmc)
{
    u32 retval;
    u32 irq_active;
    u32 mask = 0;
    FSdmmcPs_SdCmd_T *pCmd;
    FSdmmcPs_Instance_T *pInstance;
    pInstance = pSdmmc->instance;
    pCmd = &pInstance->sdCmd;

    /* read the interrupt status register*/
    irq_active = FSdmmcPs_getMaskIntStat(pSdmmc);

    retval = FALSE;

    /* card detect interrupt*/
    if (irq_active & RAW_INT_STATUS_CD)
    {
        int card_in;
        card_in = FSdmmcPs_cardDetect(pSdmmc);
        if(card_in == CARD_INSERT)
            g_SDMMC_dataParam.card_detect = 1;

        /* clear CD int;*/
        FSdmmcPs_setRawIntStat(pSdmmc, MASK_INT_STATUS_CD);

        /* disable CD int*/
        /*mask = FSdmmcPs_getIntMask(pSdmmc);*/
        /*FSdmmcPs_setIntMask(pSdmmc, mask & (~MASK_INT_STATUS_CD));*/

        return 0;
    }
    /* Check errors */
    if (irq_active & INT_STATUS_ERRORS)
    {
        if (irq_active & RAW_INT_STATUS_DCRC)
        {
            pCmd->ret_status = SDMMC_ERROR;
            /* clear DCRC int;*/
            FSdmmcPs_setRawIntStat(pSdmmc, RAW_INT_STATUS_DCRC);

            /* disable DCRC int*/
            mask = FSdmmcPs_getIntMask(pSdmmc);
            FSdmmcPs_setIntMask(pSdmmc, mask & (~RAW_INT_STATUS_DCRC));
        }
        
        if (irq_active & RAW_INT_STATUS_RTO)
        {
            pCmd->ret_status = SDMMC_ERROR_NORESPONSE;
            /* clear RTO int;*/
            FSdmmcPs_setRawIntStat(pSdmmc, RAW_INT_STATUS_RTO);

            /* disable RTO int*/
            mask = FSdmmcPs_getIntMask(pSdmmc);
            FSdmmcPs_setIntMask(pSdmmc, mask & (~MASK_INT_STATUS_RTO));
        }
        
        if (irq_active & RAW_INT_STATUS_DRTO)
        {
            pCmd->ret_status = SDMMC_ERROR_NORESPONSE;
            /* clear DRTO int;*/
            FSdmmcPs_setRawIntStat(pSdmmc, RAW_INT_STATUS_DRTO);

            /* disable DRTO int*/
            mask = FSdmmcPs_getIntMask(pSdmmc);
            FSdmmcPs_setIntMask(pSdmmc, mask & (~RAW_INT_STATUS_DRTO));
        }
        
        if (irq_active & RAW_INT_STATUS_HTO)
        {
            pCmd->ret_status = SDMMC_ERROR;
            /* clear HTO int;*/
            FSdmmcPs_setRawIntStat(pSdmmc, RAW_INT_STATUS_HTO);

            /* disable HTO int*/
            mask = FSdmmcPs_getIntMask(pSdmmc);
            FSdmmcPs_setIntMask(pSdmmc, mask & (~MASK_INT_STATUS_HTO));
        }
        
        if (irq_active & RAW_INT_STATUS_FRUM)
        {
            pCmd->ret_status = SDMMC_ERROR;
            /* clear FRUM int;*/
            FSdmmcPs_setRawIntStat(pSdmmc, RAW_INT_STATUS_FRUM);

            /* disable FRUM int*/
            mask = FSdmmcPs_getIntMask(pSdmmc);
            FSdmmcPs_setIntMask(pSdmmc, mask & (~MASK_INT_STATUS_FRUM));
        }
        
        if (irq_active & RAW_INT_STATUS_HLE)
        {
            pCmd->ret_status = SDMMC_ERROR;
            /* clear HLE int;*/
            FSdmmcPs_setRawIntStat(pSdmmc, RAW_INT_STATUS_HLE);

            /* disable HLE int*/
            mask = FSdmmcPs_getIntMask(pSdmmc);
            FSdmmcPs_setIntMask(pSdmmc, mask & (~RAW_INT_STATUS_HLE));
        }
        
        if (irq_active & RAW_INT_STATUS_EBE)
        {
            pCmd->ret_status = SDMMC_ERROR;
            /* clear EBE int;*/
            FSdmmcPs_setRawIntStat(pSdmmc, RAW_INT_STATUS_EBE);

            /* disable EBE int*/
            mask = FSdmmcPs_getIntMask(pSdmmc);
            FSdmmcPs_setIntMask(pSdmmc, mask & (~RAW_INT_STATUS_EBE));
        }
    }
    /* clear err int;*/
    FSdmmcPs_setRawIntStat(pSdmmc, (irq_active & INT_STATUS_ERRORS));

    /* Check command complete */
    if (irq_active & MASK_INT_STATUS_CMDD)
    {
        /* clear CMDO int;*/
        /*FSdmmcPs_setRawIntStat(pSdmmc, RAW_INT_STATUS_CD);*/

        /* disable CMDD int*/
        mask = FSdmmcPs_getIntMask(pSdmmc);
        FSdmmcPs_setIntMask(pSdmmc, mask & (~MASK_INT_STATUS_CMDD));
    }
   
    /* Check if TX request */
    if (irq_active & RAW_INT_STATUS_TXDR)
    {
        /* clear TXDR int;*/
        FSdmmcPs_setRawIntStat(pSdmmc, RAW_INT_STATUS_TXDR);

        /* disable TXDR int*/
        mask = FSdmmcPs_getIntMask(pSdmmc);
        FSdmmcPs_setIntMask(pSdmmc, mask & (~MASK_INT_STATUS_TXDR));
    }
    /* Check if RX request */
    if (irq_active & RAW_INT_STATUS_RXDR)
    {
        /* clear RXDR int;*/
        FSdmmcPs_setRawIntStat(pSdmmc, RAW_INT_STATUS_RXDR);

        /* disable RXDR int*/
        mask = FSdmmcPs_getIntMask(pSdmmc);
        FSdmmcPs_setIntMask(pSdmmc, mask & (~MASK_INT_STATUS_RXDR));
    }
     /* Check if Auto cmd done */
    if (irq_active & RAW_INT_STATUS_ACD)
    {
        /* clear ACD int;*/
        FSdmmcPs_setRawIntStat(pSdmmc, RAW_INT_STATUS_ACD);
    }
    
    /* Check if transfer finished */
    if (irq_active & RAW_INT_STATUS_DTO)
    {
        /* clear DTO int;*/
        /*FSdmmcPs_setRawIntStat(pSdmmc, RAW_INT_STATUS_DTO);*/

        /* disable DTO int*/
        mask = FSdmmcPs_getIntMask(pSdmmc);
        FSdmmcPs_setIntMask(pSdmmc, mask & (~MASK_INT_STATUS_DTO));
    }

    retval = TRUE;
    return retval;
}

void FSdmmcPs_IRQ (void *InstancePtr)
{
	FSdmmcPs_irqHandler((FSdmmcPs_T *)InstancePtr);
}

s32 FSdmmcPs_regRdWtCheck(FSdmmcPs_T *pSdmmc)
{
	u32 i;
	u32 reg, offset, write_data;
	u32 baseAddr = pSdmmc->config.BaseAddress;
	s32 checkResult = FMSH_SUCCESS;
	u32 rd_wr_check_data;
	/* default value*/
	u32 regs_mask[30] = {0x01FF0870,
                         0x00000001,
                         0xFFFFFFFF,
                         0x00000003,
                         0x00010001,
                         0xFFFFFFFF,
                         0x00010001,
                         0x0000FFFF,
                         0xFFFFFFFF,
                         0xFFFFFFFF,
                         0xFFFFFFFF,
                         0x1FFFFFFF,
                         0x00000000,
                         0x00000000,
                         0x00000000,
                         0x00000000,
                         0x00000000,
                         0x00000000,
                         0x00000000,
                         0x77FF0FFF,
                         0x00000000,
                         0x00000000,
                         0x00000000,
                         0x00000000,
                         0x00000000,
                         0x00FFFFFF,
                         0x00000000,
                         0x00000000,
                         0x00000000,
                         0xFFFFFFFF};
	rd_wr_check_data = 0xFFFFFFFF;
	/* check registers*/
	for (i = 0; i < 30; i++)
	{
		/* write 0xFFFFFFFF*/
		offset = 0x0000 + i * 4;
		write_data = rd_wr_check_data & regs_mask[i];
		FMSH_WriteReg(baseAddr, offset, write_data);
		
		reg = 0;
		reg = FMSH_ReadReg(baseAddr, offset) & regs_mask[i];
		if (reg != write_data)
		{
			TRACE_OUT(DEBUG_OUT, "SDMMC register[0x%04x] write 0xFFFFFFFF error!\r\n", offset);
			checkResult = FMSH_FAILURE;
		}
	}
	for (i = 0; i < 30; i++)
	{
		/* write 0x0*/
		offset = 0x0000 + i * 4;
		write_data = 0;
		FMSH_WriteReg(baseAddr, offset, write_data);
		
		reg = 0xFFFFFFFF;
		reg = FMSH_ReadReg(baseAddr, offset) & regs_mask[i];
		if (reg != write_data)
		{
			TRACE_OUT(DEBUG_OUT, "SDMMC register[0x%04x] write 0x0 error!\r\n", offset);
			checkResult = FMSH_FAILURE;
		}
	}
	
	if (checkResult == FMSH_SUCCESS)
		TRACE_OUT(DEBUG_OUT, "The read/write of SDMMC registers check done, no error occurs!\r\n");

	return checkResult;
}

s32 FSdmmcPs_regInitCheck(FSdmmcPs_T *pSdmmc)
{
	u32 i;
	u32 reg, offset;
	u32 baseAddr = pSdmmc->config.BaseAddress;
	s32 checkResult = FMSH_SUCCESS;
	/* default value*/
	u32 sdmmc_reg[30] = {0x00000000, /* CTRL*/
                         0x00000000, /* PWREN*/
                         0x00000000, /* CLKDIV*/
                         0x00000000, /* CLKSRC*/
                         0x00000000, /* CLKENA*/
                         0xFFFFFF40, /* TMOUT*/
                         0x00000000, /* CTYPE*/
                         0x00000200, /* BLKSIZ*/
                         0x00000200, /* BYTCNT*/
                         0x00000000, /* INTMASK*/
                         0x00000000, /* CMDARG*/
                         0x00000000, /* CMD*/
                         0x00000000, /* RESP0*/
                         0x00000000, /* RESP1*/
                         0x00000000, /* RESP2*/
                         0x00000000, /* RESP3*/
                         0x00000000, /* MINTSTS*/
                         0x00000000, /* RINTSTS*/
                         0x00000106, /* STATUS*/
                         0x001F0000, /* FIFOTH*/
                         0x00000001, /* CDETECT, 0--card in; 1--no card*/
                         0x00000001, /* WRTPRT*/
                         0x00000000, /* GPIO*/
                         0x00000000, /* TCBCNT*/
                         0x00000000, /* TBBCNT*/
                         0x00FFFFFF, /* DEBNCE*/
                         0x00000000, /* USRID*/
                         0x5342220A, /* VERID*/
                         0x07E52CC1, /* HCON*/
                         0x00000000  /* UHS_REG*/
                        };
	/* check registers*/
	for (i = 0; i < 30; i++)
	{
		offset = 0x0000 + i * 4;
		reg = FMSH_ReadReg(baseAddr, offset);
		if (sdmmc_reg[i] != reg)
		{
			TRACE_OUT(DEBUG_OUT, "SDMMC register[0x%04x] error, read value is 0x%08x !\r\n", offset, reg);
			checkResult = FMSH_FAILURE;
		}
	}

	return checkResult;
}

s32 FSdmmcPs_resetCheck(FSdmmcPs_T *pSdmmc, int index)
{
    s32 testStatus = FMSH_FAILURE;
	u32 reg_tmp;

    FSdmmcPs_powerUp(pSdmmc);
    FSdmmcPs_setCardVoltage(pSdmmc, 5);
    FSdmmcPs_setCardVoltage(pSdmmc, 6);
    FSdmmcPs_setCardVoltage(pSdmmc, 7);
    FSdmmcPs_powerDown(pSdmmc);
	
	FSdmmcPs_resetController(index);
	testStatus = FSdmmcPs_regInitCheck(pSdmmc);

	/* watchDog reset*/
	if (0)
	{
		FMSH_WriteReg(0xE0025000,0x4,0x5a);  /*TOP_INIT = 5, TOP = 10*/
		FMSH_WriteReg(0xE0025000,0x0,0x1d);  /* 11101 ennale wdt*/
		reg_tmp = FMSH_ReadReg(pSdmmc->config.BaseAddress, SDMMC_FIFOTH);
		TRACE_OUT(DEBUG_OUT, "\nRead reg data: 0x%08x\r\n", reg_tmp);

		/* set fifo*/
		FSdmmcPs_setDmaMsize(pSdmmc, 2);
		FSdmmcPs_setRXwmark(pSdmmc, 7);
		FSdmmcPs_setTXwmark(pSdmmc, 8);
		
		reg_tmp = FMSH_ReadReg(pSdmmc->config.BaseAddress, SDMMC_FIFOTH);
		TRACE_OUT(DEBUG_OUT, "\nRead reg data: 0x%08x\r\n", reg_tmp);
	}

	/* ps system reset*/
	if (0)
	{
		FSlcrPs_softRst(1);
		testStatus = FSdmmcPs_regInitCheck(pSdmmc);
		reg_tmp = FMSH_ReadReg(pSdmmc->config.BaseAddress, SDMMC_FIFOTH);
		TRACE_OUT(DEBUG_OUT, "\nRead reg data: 0x%08x\r\n", reg_tmp);
	}

    return testStatus;
}

void FSdmmcPs_initDmac(FSdmmcPs_T *pSdmmc)
{
	FSdmmcPs_Instance_T *pInstance;
	pInstance = pSdmmc->instance;

	/* Initialize the DMA controller*/
	FDmaPs_init(pInstance->pDma);
	
	FDmaPs_enable(pInstance->pDma);
}

u32 FSdmmcPs_initSdmmcCtrl(FSdmmcPs_T *pSdmmc, enum SDMMC_TransMode trans_mode)
{
	int ret = FMSH_SUCCESS;
	
    /* Reset the chip. */
    ret = FSdmmcPs_sdControllerReset(pSdmmc);
	if (ret < 0)
		return FMSH_FAILURE;
	
    ret = FSdmmcPs_sdFifoReset(pSdmmc);
	if (ret < 0)
		return FMSH_FAILURE;
	
	/* set fifo */
	FSdmmcPs_setDmaMsize(pSdmmc, 2);  /* 010 - 8*/
	FSdmmcPs_setRXwmark(pSdmmc, 7);   /* MSize = 8，RX_WMark = 7*/
	FSdmmcPs_setTXwmark(pSdmmc, 8);   /* MSize = 8，TX_WMark = 8*/
	
	/* clear int */
	/*FSdmmcPs_setRawIntStat(pSdmmc, FSdmmcPs_getRawIntStat(pSdmmc));*/
	FSdmmcPs_setRawIntStat(pSdmmc, 0xFFFFFFFF);
	
	/* enable int */
	FSdmmcPs_enableInt(pSdmmc);
	
    /* mask interrupt disable */
	FSdmmcPs_disableIntMask(pSdmmc);
	/*FSdmmcPs_setIntMask(pSdmmc, 0xFFFB);*/

	if (trans_mode == sdmmc_trans_mode_dw_dma)
	{
		FSdmmcPs_enableDMA(pSdmmc);
	}

	return ret;
}

s32 FSdmmcPs_cardInitializeVerify(FSdmmcPs_T *pSdmmc, FDmaPs_T *dev_dmac, enum SDMMC_TransMode trans_mode)
{
	s32 status = FMSH_FAILURE;
	
	FSdmmcPs_instanceReset(pSdmmc, dev_dmac);
	
	status = FSdmmcPs_initSdmmcCtrl(pSdmmc, trans_mode);
	if (status != FMSH_SUCCESS)
	{
		return status;
	}
	
	if (trans_mode == sdmmc_trans_mode_dw_dma)
		FSdmmcPs_initDmac(pSdmmc);
	
	status = FSdmmcPs_initCard(pSdmmc);

	return status;
}

s32 FSdmmcPs_cardDataTransfer(FSdmmcPs_T *pSdmmc, enum SDMMC_TransMode trans_mode, u8 wr, u8 rd)
{
    u8 error = 0;
    u32 i, test_cnt = 0, errcnt = 0;
    u32 multiBlock, block, splitMulti, totalBlockNum;
	u32 dataLen, rwDataLen;
	char8 *pWriteBuf, *pReadBuf;

	UINT8 * pBuf8 = NULL;
	int j = 0;
		
#ifdef GTC_TIME_OUTPUT
	u64 gtc_pre_time_cnt = 0;
	u64 gtc_cur_time_cnt = 0;
#endif
	
    /* Perform tests on each block */
    multiBlock = 0;
	/*totalBlockNum = FSdmmcPs_getBlockNumbers(pSdmmc);*/
	totalBlockNum = g_SDMMC_dataParam.block_num;  /* only for test*/

	pWriteBuf = (char8 *)g_SDMMC_dataParam.write_addr;
	pReadBuf = (char8 *)g_SDMMC_dataParam.read_addr;

	printf("totalBlockNum: %d \n", totalBlockNum);
	
	printf("pWriteBuf: 0x%08X \n", pWriteBuf);
	printf("pReadBuf: 0x%08X \n",  pReadBuf);
	
#ifdef GTC_TIME_OUTPUT
	{
		float deltaTime;
		u64 gtc_pre_time_cnt = 0;
		u64 gtc_cur_time_cnt = 0;

		gtc_pre_time_cnt = get_current_time();
		for(i = 0; i< 1; i++);
		gtc_cur_time_cnt = get_current_time();
		
        calc_transTime(gtc_pre_time_cnt, gtc_cur_time_cnt);
	}
#endif
	
    for (block = 0; block < totalBlockNum; block += multiBlock)
	{
        /* Perform different single or multiple bloc operations */
		/*
		if (test_cnt < 5)
		{
			multiBlock = 1;
		}
		else
		{
	        if (multiBlock >= 8)
	    	{
				//multiBlock <<= 1;
				multiBlock = MAX_MULTI_BLOCKS;
	    	}
	        else
	    	{
	    		multiBlock ++;
	    	}
			
	        if (multiBlock > MAX_MULTI_BLOCKS)
	    	{
	        	//multiBlock = 1;
	    	}
	    	//multiBlock = MAX_MULTI_BLOCKS;
		}

        // Multi-block adjustment 
        if (block + multiBlock > totalBlockNum)
		{
            multiBlock = totalBlockNum - block;
        }
		*/
		multiBlock = 1;

        /* ** Perform single block or multi block transfer */
        TRACE_OUT(DEBUG_OUT, "\n-I- Testing block [%6u - %6u] ...\r\n", block, (block + multiBlock -1));
		
		dataLen = SDMMC_BLOCK_SIZE * multiBlock;

        memset(pWriteBuf, 0, dataLen * sizeof(char8));
        /* - Write a checkerboard pattern on the block */
        for (i = 0; i < dataLen; i++)/* = i + 4)*/
		{
            /*if ((i & 1) == 0)*/
                /*pWriteBuf[i] = 0x94;*/
				/*pWriteBuf[i] = (i & 0x55);*/
            /*else*/
                /*pWriteBuf[i] = 0x49;//(i & 0xAA);*/
            	pWriteBuf[i] = i + 1;/*(i & 0xAA);*/
        }

        if (wr) 
		{
/*			memset(pWriteBuf, 0, dataLen *sizeof(char8));*/
/*	        /* - Write a checkerboard pattern on the block */
/*	        for (i = 0; i < dataLen; i++) = i + 4)*/
/*			{*/
/*	            if ((i & 1) == 0)*/
/*                    pWriteBuf[i] = 0xA5;*/
/*					pWriteBuf[i] = (i & 0x55);*/
/*	            else*/
/*                    pWriteBuf[i] = 0x3C;*/
/*	            	pWriteBuf[i] = i + 1;(i & 0xAA);*/
/*	        }*/
			
            for (i = 0; i < multiBlock; )
			{
                splitMulti = ((multiBlock - i) > NB_SPLIT_MULTI) ? NB_SPLIT_MULTI : (multiBlock - i);
				rwDataLen = SDMMC_BLOCK_SIZE * splitMulti;
				
			#ifdef GTC_TIME_OUTPUT
				gtc_pre_time_cnt = get_current_time();
			#endif

                error = FSdmmcPs_writeFun(pSdmmc, block + i, &pWriteBuf[i * SDMMC_BLOCK_SIZE], rwDataLen, trans_mode);
			
			#ifdef GTC_TIME_OUTPUT
				gtc_cur_time_cnt = get_current_time();
                calc_transTime(gtc_pre_time_cnt, gtc_cur_time_cnt);
			#endif
                if (error)
					break;
				else
                	i += splitMulti;
            }
            if (error)
            {
                TRACE_OUT(DEBUG_OUT, "-E- 2. Write block #%u(%u+%u): %d\n\r",  (block+i), block, i, error);
                if(MaxErrorBreak(0))
					return FMSH_FAILURE;
				else
	                continue; /* Skip following test */
            }
        }

        if (rd)
		{
            /* - Read back the data to check the write operation */
            memset(pReadBuf, 0, dataLen * sizeof(char8));
			
           /* for (i = 0; i < multiBlock; )*/
			{
                splitMulti = ((multiBlock - i) > NB_SPLIT_MULTI) ? NB_SPLIT_MULTI : (multiBlock - i);
				rwDataLen = SDMMC_BLOCK_SIZE * splitMulti;
				
			#ifdef GTC_TIME_OUTPUT
				gtc_pre_time_cnt = get_current_time();
			#endif

                /*error = FSdmmcPs_readFun(pSdmmc, block + i, &pReadBuf[i *SDMMC_BLOCK_SIZE], rwDataLen, trans_mode);*/
				/*pBuf8 = (UINT8 *(&pReadBuf[i *SDMMC_BLOCK_SIZE]);*/
                error = FSdmmcPs_readFun(pSdmmc, block, &pReadBuf[0], SDMMC_BLOCK_SIZE, trans_mode);
				pBuf8 = (UINT8 *)(&pReadBuf[0]);
			
			#ifdef GTC_TIME_OUTPUT
				gtc_cur_time_cnt = get_current_time();
                calc_transTime(gtc_pre_time_cnt, gtc_cur_time_cnt);
			#endif
			
                if (error) 
					break;
				else
                	i += splitMulti;

				#if 1
				for (j=0; j<SDMMC_BLOCK_SIZE; j++)
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
				}
				#endif
				
            }
			
            if (error)
            {
                TRACE_OUT(DEBUG_OUT, "-E- 2. Read block #%u(%u+%u): %d \n\r", (block + i), block, i, error);
                if(MaxErrorBreak(0))
					return FMSH_FAILURE;
				else
	                continue; /* Skip following test */
            }
            errcnt = 0;
			
            /*
            for (i=0; i < dataLen; i++)
			{
                if (pReadBuf[i] != pWriteBuf[i])
                {
                    uint32_t j, js;
					
                    TRACE_OUT(DEBUG_OUT, "\n\r-E- 2.%d. Data @ %u (0x%x)\n\r", errcnt, i, i);
                    TRACE_OUT(DEBUG_OUT, "  -Src:");
					
                    js = (i > 8) ? (i - 8) : 0;
					
                    for (j = js; j < i + 8; j ++)
                        TRACE_OUT(DEBUG_OUT, " %02x", pWriteBuf[j]);
					
                    TRACE_OUT(DEBUG_OUT, "\n\r  -Dat:");
					
                    for (j = js; j < i + 8; j ++)
                        TRACE_OUT(DEBUG_OUT, "%c%02x", (i == j) ? '!' : ' ', pReadBuf[j]);
					
                    TRACE_OUT(DEBUG_OUT, "\n\r");
					
                    if(MaxErrorBreak(0))
						return FMSH_FAILURE;
					
                    // Only find first 3 verify error.
                    if (errcnt ++ >= 3)
                        break;
                }
            }
			*/
        }

		test_cnt++;
    }

    return FMSH_SUCCESS;
}

s32 FileDataCmp(int mode, u32 len, u32 addr)
{
	s32 status = FMSH_SUCCESS;
	int i;
	u8 *buf, *source;
	FILE *fid;
	
	buf = (u8 *)(FPS_AHB_SRAM_BASEADDR + 0x10000);
        
	fid = fopen("boot.bin","rb");
	if (fid == NULL)
	{
		TRACE_OUT(DEBUG_OUT, "Read file error!\r\n");
		return FMSH_FAILURE;
	}
	
	/* mode==1, know the data num; mode==2,unknow*/
	if (mode == 1)
	{
		/*source = (u8 *malloc(sizeof(u8)*en);*/
        memset(buf, 0, len * sizeof(u8));
		fread(buf, sizeof(u8), len, fid);
		/*free(buf);*/
	}
	else
	{
		fseek(fid, 0, SEEK_END);
		LONG lSize = ftell(fid);
		rewind(fid);
		int num = lSize / sizeof(u8);
		/*u8 *uf = (u8 *malloc(num *sizeof(u8));*/
		memset(buf, 0, num * sizeof(u8));
		if (buf == NULL)
		{
			TRACE_OUT(DEBUG_OUT, "malloc memory error!\r\n");
			return FMSH_FAILURE;
		}
		fread(buf, sizeof(u8), num, fid);
		/*free(buf);*/
	}
	fclose(fid);
	
	source = (u8 *)addr;
	for (i = 0; i < len; i++)
	{
		if (*source != *buf)
        {
        	TRACE_OUT(DEBUG_OUT, "Read file data error, data[%d]:%x, r:%x!\r\n", i, *buf, *source);
			status = FMSH_FAILURE;
            break;
        }
		source++;
		buf++;
	}
	free(buf);

	return status;
}



s32 FSdmmcPs_intVerify(FSdmmcPs_T *pSdmmc, enum SDMMC_TransMode trans_mode)
{
	s32 status = FMSH_FAILURE;
    u8 error = 0;
    u32 i, errcnt = 0;
    u32 multiBlock, block;
	u32 dataLen;
	char8 *pWriteBuf, *pReadBuf;
	
	status = FSdmmcPs_initCard(pSdmmc);
	if (status != FMSH_SUCCESS)
	{
		return status;
	}

	multiBlock = 2;
	dataLen = SDMMC_BLOCK_SIZE * multiBlock;
	pWriteBuf = (char8 *)FPS_AHB_SRAM_BASEADDR;
	pReadBuf = (char8 *)(FPS_AHB_SRAM_BASEADDR + 0x10000);
	
    for (block = 0; block < 1; block += multiBlock)
	{
        /* ** Perform single block or multi block transfer */
        TRACE_OUT(DEBUG_OUT, "-I- Testing block [%6u - %6u] ...\r\n", block, (block + multiBlock -1));

		memset(pWriteBuf, 0, dataLen * sizeof(char8));
        /* - Write a checkerboard pattern on the block */
        for (i = 0; i < dataLen; i++)
		{
            if ((i & 1) == 0) 
				pWriteBuf[i] = (i & 0x55);
            else
            	pWriteBuf[i] = (i & 0xAA);
        }
        error = FSdmmcPs_writeFun(pSdmmc, block, pWriteBuf, dataLen, trans_mode);
        if (error)
        {
            TRACE_OUT(DEBUG_OUT, "-E- 2. Write block #%u(%u+%u): %d\n\r",  block, block, i, error);
            if(MaxErrorBreak(0)) 
				return FMSH_FAILURE;
			else
                continue; /* Skip following test */
        }

        /* - Read back the data to check the write operation */
        memset(pReadBuf, 0, SDMMC_BLOCK_SIZE * sizeof(char8));
        error = FSdmmcPs_readFun(pSdmmc, block, pReadBuf, dataLen, trans_mode);
        if (error)
        {
            TRACE_OUT(DEBUG_OUT, "-E- 2. Read block #%u(%u+%u): %d\n\r", (block + i), block, i, error);
            if(MaxErrorBreak(0))
				return FMSH_FAILURE;
			else
                continue; /* Skip following test */
        }
        errcnt = 0;
        for (i = 0; i < dataLen; i++)
		{
            if (pReadBuf[i] != pWriteBuf[i])
            {
                uint32_t j, js;
                TRACE_OUT(DEBUG_OUT, "\n\r-E- 2.%d. Data @ %u (0x%x)\n\r", errcnt, i, i);
                TRACE_OUT(DEBUG_OUT, "  -Src:");
                js = (i > 8) ? (i - 8) : 0;
                for (j = js; j < i + 8; j ++)
                    TRACE_OUT(DEBUG_OUT, " %02x", pWriteBuf[j]);
                TRACE_OUT(DEBUG_OUT, "\n\r  -Dat:");
                for (j = js; j < i + 8; j ++)
                    TRACE_OUT(DEBUG_OUT, "%c%02x", (i == j) ? '!' : ' ', pReadBuf[j]);
                TRACE_OUT(DEBUG_OUT, "\n\r");
                if(MaxErrorBreak(0))
					return FMSH_FAILURE;
                /* Only find first 3 verify error.*/
                if (errcnt ++ >= 3)
                    break;
            }
        }
    }

    /* check HLE error*/
    {
        u32 temp, baseAddr;

        baseAddr = pSdmmc->config.BaseAddress;
        /* disable clock */
        FMSH_WriteReg(baseAddr, SDMMC_CLKENA, 0);
        temp = START_CMD + UPDATE_CLOCK_REG_ONLY + WAIT_PRVDATA_COMPLETE;
        FMSH_WriteReg(baseAddr, SDMMC_CMD_VAL, temp);
        
        /* disable clock */
        FMSH_WriteReg(baseAddr, SDMMC_CLKENA, 0);
        temp = START_CMD + UPDATE_CLOCK_REG_ONLY + WAIT_PRVDATA_COMPLETE;
        FMSH_WriteReg(baseAddr, SDMMC_CMD_VAL, temp);

        /* enable clock */
        FMSH_WriteReg(baseAddr, SDMMC_CLKENA, 0xFFFF);
        temp = START_CMD + UPDATE_CLOCK_REG_ONLY + WAIT_PRVDATA_COMPLETE;
        FMSH_WriteReg(baseAddr, SDMMC_CMD_VAL, temp);
    }

	return status;
}

#if 0  /* jc*/

u32 SDFileVerify()
{
	FRESULT res_sd;
	UINT bw;
	u32 i;
	u32 *src, *dst, dataLen;
	u8 work[FF_MAX_SS]; /* Work area (larger is better for process time) */

	src = (u32*)FPS_AHB_SRAM_BASEADDR;
	dst = (u32*)(FPS_AHB_SRAM_BASEADDR + 0x10000);
	dataLen = 512;

	for(i = 0; i < dataLen; i++)
    {
    	*src = i + 1;
		src++;
    }
	src = (u32*)FPS_AHB_SRAM_BASEADDR;

	memset(dst, 0, dataLen * sizeof(u32));

	res_sd = f_mount(&fatfs, "0:/", 0);
	
	/*----------------------- Format test ---------------------------*/
	/* Format and create file system if no file in SD card */
	if(res_sd == FR_NO_FILESYSTEM)
	{
		TRACE_OUT(DEBUG_OUT, "> No file in SD card, will format first...\r\n");
		
		/* Format */
		res_sd = f_mkfs("0:", FM_FAT32, 0, work, sizeof work);
		if(res_sd == FR_OK)
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
	else if(res_sd != FR_OK)
	{
		TRACE_OUT(DEBUG_OUT, "> SD card mount file system failed.(%d)\r\n", res_sd);
		return FMSH_FAILURE;
	}
	else
	{
		TRACE_OUT(DEBUG_OUT, "> File system mount succeed, begin to write/read test.\r\n");
	}
	
	/*--------------------- Write test -----------------------*/
	/* Open file or create file if no file in SD card */
	TRACE_OUT(DEBUG_OUT, "****** Start file write test... ******\r\n");
	
	res_sd = f_open(&fil, "0:test.txt", FA_CREATE_ALWAYS|FA_WRITE);
	if(res_sd == FR_OK)
	{
		TRACE_OUT(DEBUG_OUT, "> Open/create test.txt file succeed, start to write data into the file.\r\n");
		
		/* Write data to the file */
		res_sd = f_write(&fil, (void*)src, dataLen, &bw);
		if (res_sd == FR_OK)
		{
			TRACE_OUT(DEBUG_OUT, "> File write succeed, the write data is: %d\r\n", bw);
		}
		else
		{
			TRACE_OUT(DEBUG_OUT, "> File write failed: (%d)\r\n", res_sd);
		}
		
		/* close the file */
		res_sd = f_close(&fil);
		if (res_sd != FR_OK)
		{
			return FMSH_FAILURE;
		}
	}
	else
	{
		TRACE_OUT(DEBUG_OUT, "> Open/create file failed.\r\n");
	}
	
	/*------------------ Read test --------------------------*/
	TRACE_OUT(DEBUG_OUT, "****** Start file read test... ******\r\n");
	
	res_sd=f_open(&fil, "0:test.txt", FA_OPEN_EXISTING|FA_READ);
	if(res_sd == FR_OK)
	{
		TRACE_OUT(DEBUG_OUT, "> open file succeed.\r\n");
		res_sd = f_read(&fil, (void*)dst, dataLen, &bw);
		if (res_sd==FR_OK)
		{
			TRACE_OUT(DEBUG_OUT, "> File read succeed, the read data is: %d\r\n", bw);
		}
		else
		{
			TRACE_OUT(DEBUG_OUT, "> File read failed: (%d)\n", res_sd);
		}
	}
	else
	{
		TRACE_OUT(DEBUG_OUT, "> Open file failed.\r\n");
	}
	
	/* close the file */
	res_sd = f_close(&fil);
	if (res_sd != FR_OK)
	{
		return FMSH_FAILURE;
	}
	
	/* unuse the file system, cancel the mounted file system */
	f_mount(NULL, "0:", 0);

	src = (u32*)FPS_AHB_SRAM_BASEADDR;
	dst = (u32*)(FPS_AHB_SRAM_BASEADDR + 0x10000);
	for(i = 0; i < (dataLen/4); i++)
	{
		if(*dst != *src)
		{
			TRACE_OUT(DEBUG_OUT, "Write/Read file data error, data[%d]--w:%x, r:%x!\r\n", i, *src, *dst);
			return FMSH_FAILURE;
		}
		dst++;
		src++;
	}
	
	return FMSH_SUCCESS;
}

s32 FSdmmcPs_fileVerify(void)
{
}

#endif

int FSdmmcPs_verify(void)
{
    int status;
    FSdmmcPs_Config *pSdCfg;

    /* Initialize the SD_0 Driver */
    pSdCfg = FSdmmcPs_LookupConfig(FPAR_SDPS_0_DEVICE_ID);
    if (pSdCfg == NULL) {
        return FMSH_FAILURE;
    }
	
    FSdmmcPs_initDev(&g_SDMMC[0], &g_SDMMC_instance[0], pSdCfg);
	
	/*
	status = FGicPs_registerInt(&IntcInstance, SDMCC0_INT_ID,
	          (FMSH_InterruptHandler)FSdmmcPs_IRQ, &g_SDMMC[0]);
    if (status != FMSH_SUCCESS)
    {
        return FMSH_FAILURE;
    }
	*/

    /* Initialize the SD_1 Driver */
    pSdCfg = FSdmmcPs_LookupConfig(FPAR_SDPS_1_DEVICE_ID);
    if (pSdCfg == NULL) {
        return FMSH_FAILURE;
    }
	
    FSdmmcPs_initDev(&g_SDMMC[1], &g_SDMMC_instance[1], pSdCfg);
	
	/*
	status = FGicPs_registerInt(&IntcInstance, SDMCC1_INT_ID,
	          (FMSH_InterruptHandler)FSdmmcPs_IRQ, &g_SDMMC[1]);
    if (status != FMSH_SUCCESS)
    {
        return FMSH_FAILURE;
    }
	*/

    memset(&g_SDMMC_dataParam, 0, sizeof(SDMMC_dataParam));

#ifdef SDMMC_SELF_TEST
    u8 test_done = 0;
    u16 test_cmd = 0x1080;
    g_UartRXBuffer[6] = SDMMC1_SELECT;
    while(!test_done)
    {
        pc_interface_sdmmc(test_cmd);

        if (test_cmd == 0x1080)
            test_cmd = 0x1000;
        else
            test_cmd++;

        if (test_cmd == 0x1007)
            test_cmd++;
        
		if (test_cmd > 0x1008)
		{
			test_done = 1;
		}
	}
#endif

    return FMSH_SUCCESS;
}

void FSdmmcPs_checkCardState(FSdmmcPs_T *pSdmmc)
{
	int ret;
	FSdmmcPs_SdCmd_T *pCmd;
	FSdmmcPs_Instance_T *pInstance;
	pInstance = pSdmmc->instance;
	pCmd = &pInstance->sdCmd;
	
	do
	{
		/* check card-state. if card-state != StandBy, return BUSY */
		pCmd->cmd = CMD_SEND_STATUS;
		pCmd->arg = (pInstance->rca << 16) & 0xFFFF0000;
		pCmd->res_type = CMD_RESP_TYPE_R1;
		pCmd->res_flag = 0;
		ret = pInstance->sendCmd(pSdmmc);
	} while(pCmd->rsp_buf[0] != 0x900);
}

void FSdmmcPs_waitWriteDone(FSdmmcPs_T *pSdmmc)
{
	int tmp;
	u32 baseAddr = pSdmmc->config.BaseAddress;
	
	tmp = FMSH_ReadReg(baseAddr, SDMMC_RINTSTS);
	while(!(tmp & RAW_INT_STATUS_DTO))
	{
		tmp = FMSH_ReadReg(baseAddr, SDMMC_RINTSTS);
        	delay_1us();
	}
}

int FSdmmcPs_sendRwCmd(FSdmmcPs_T *pSdmmc)
{
    u32 cmd_data = 0;
	u32 baseAddr = pSdmmc->config.BaseAddress;
	FSdmmcPs_Instance_T *pInstance;
	FSdmmcPs_SdCmd_T *pCmd;
	u32 tmp32 = 0;
	
	pInstance = pSdmmc->instance;
	pCmd = &pInstance->sdCmd;

    if(pCmd->res_flag & (CMD_RESP_FLAG_RDATA | CMD_RESP_FLAG_WDATA))
    {
		FMSH_WriteReg(baseAddr, SDMMC_BYTCNT, pCmd->data_len);
		FMSH_WriteReg(baseAddr, SDMMC_BLKSIZ, 512);
    }

	delay_ms(1);
	tmp32 = FSdmmcPs_getRawIntStat(pSdmmc);
	FSdmmcPs_setRawIntStat(pSdmmc, tmp32);

    cmd_data = FSdmmcPs_prepareCmd(pInstance->card_type, pCmd->cmd);
    /* Send the command (CRC calculated by host). */
	FMSH_WriteReg(baseAddr, SDMMC_CMDARG, pCmd->arg);
	FMSH_WriteReg(baseAddr, SDMMC_CMD_VAL, cmd_data);
	
#if 0  /* old*/
    /*while (!(FSdmmcPs_getRawIntStat(pSdmmc) & RAW_INT_STATUS_CMD_DONE))*/
	while (!(FSdmmcPs_getRawIntStat(pSdmmc) & 0xE9000000))
    {
        delay_us(1);
    }
#else
	do 
	{
		delay_ms(1);
		tmp32 = FSdmmcPs_getRawIntStat(pSdmmc);
		
	} while (!(tmp32 & 0xE9000000));
#endif
	
	/* clear CMDD int */
	/*FSdmmcPs_setRawIntStat(pSdmmc, RAW_INT_STATUS_CMD_DONE);*/
	FSdmmcPs_setRawIntStat(pSdmmc, 0xE9000000);
	
    return 0;
}

int FSdmmcPs_readWriteData(FSdmmcPs_T *pSdmmc, u32 address, u32 len,  int rw)
{
	int ret;
	FSdmmcPs_SdCmd_T *pCmd;
	FSdmmcPs_Instance_T *pInstance;
	pInstance = pSdmmc->instance;
	pCmd = &pInstance->sdCmd;

	if(((pInstance->card_type == MMC) || (pInstance->card_type == EMMC)) && (len > 0x200))
	{
		pCmd->cmd = CMD_SET_BLOCK_COUNT;
		pCmd->arg = len >> 9;
		pCmd->res_type = CMD_RESP_TYPE_R1;
		pCmd->res_flag = 0;
		ret = pInstance->sendCmd(pSdmmc);
		if(ret)
		{
			TRACE_OUT(DEBUG_OUT, "send cmd23 error.\r\n");
			return ret;
		}
	}

	if (rw) 
	{
		pCmd->cmd = len > 512 ? CMD_READ_MULTIPLE_BLOCK : CMD_READ_SINGLE_BLOCK;
		pCmd->res_flag = CMD_RESP_FLAG_RDATA;
	} 
	else 
	{
		pCmd->cmd = len > 512 ? CMD_WRITE_MULTIPLE_BLOCK : CMD_WRITE_BLOCK;
		pCmd->res_flag = CMD_RESP_FLAG_WDATA;
	}
	if(pInstance->HighCapacity == 0)
		pCmd->arg = address << 9; /* block size: 512Bytes*/
	else
		pCmd->arg = address; /* unit: block number*/
	pCmd->res_type = CMD_RESP_TYPE_R1;
	pCmd->data = (char *)pInstance->priv;
	pCmd->data_len = len ;
	ret = FSdmmcPs_sendRwCmd(pSdmmc);
	if (ret )
	{
		TRACE_OUT(DEBUG_OUT, "CMD%d failed.\r\n", pCmd->cmd);
		return ret;
	}

	return 0;
}


#if 1

extern s32 test_dmac_init(void);

int g_test_sdmmc = 0;

int test_sdmmc_init(void)
{
    int status;
    FSdmmcPs_Config *pSdCfg;
		

    /* Initialize the SD_0 Driver */
    pSdCfg = FSdmmcPs_LookupConfig(FPAR_SDPS_0_DEVICE_ID);
    if (pSdCfg == NULL) {
        return FMSH_FAILURE;
    }
	
    FSdmmcPs_initDev(&g_SDMMC[0], &g_SDMMC_instance[0], pSdCfg);

#if 0 /* move to back*/
	test_dmac_init();	
	extern FDmaPs_T g_DMA_dmac;
	g_SDMMC[0].instance->pDma = (FDmaPs_T*)(&g_DMA_dmac);

#endif

	/*
	status = FGicPs_registerInt(&IntcInstance, SDMCC0_INT_ID,
	          (FMSH_InterruptHandler)FSdmmcPs_IRQ, &g_SDMMC[0]);
    if (status != FMSH_SUCCESS)
    {
        return FMSH_FAILURE;
    }
	*/

    /* Initialize the SD_1 Driver */
    pSdCfg = FSdmmcPs_LookupConfig(FPAR_SDPS_1_DEVICE_ID);
    if (pSdCfg == NULL) {
        return FMSH_FAILURE;
    }
	
    FSdmmcPs_initDev(&g_SDMMC[1], &g_SDMMC_instance[1], pSdCfg);
	
	/*
	status = FGicPs_registerInt(&IntcInstance, SDMCC1_INT_ID,
	          (FMSH_InterruptHandler)FSdmmcPs_IRQ, &g_SDMMC[1]);
    if (status != FMSH_SUCCESS)
    {
        return FMSH_FAILURE;
    }
	*/

    memset(&g_SDMMC_dataParam, 0, sizeof(SDMMC_dataParam));
	

#if 1
	FSdmmcPs_T * pSdmmc = (FSdmmcPs_T *)(&g_SDMMC[0]);	/* SD0*/
	
	if (g_test_sdmmc == 0)
	{
		g_test_sdmmc = 1;
		
		/*test_sdmmc_init();*/
#if 0 /* move to here*/
	test_dmac_init();	
	extern FDmaPs_T g_DMA_dmac;
	g_SDMMC[0].instance->pDma = (FDmaPs_T*)(&g_DMA_dmac);
		
#endif

		/*
		g_SDMMC_dataParam.block_num = block_idx; // 2000;
		//g_SDMMC_dataParam.trans_mode = sdmmc_trans_mode_normal;  // sdmmc_trans_mode_dw_dma;
		
		g_SDMMC_dataParam.write_addr = (u32)FPS_AHB_SRAM_BASEADDR;
		g_SDMMC_dataParam.read_addr = (u32)(FPS_AHB_SRAM_BASEADDR + 0x10000);
		
		g_SDMMC_dataParam.write_type = 0;
		g_SDMMC_dataParam.read_type = 0;
		*/

#if 1  /* fifo-mode*/
		g_SDMMC_dataParam.trans_mode = sdmmc_trans_mode_normal;  /* sdmmc_trans_mode_dw_dma;		*/
		status = FSdmmcPs_cardInitializeVerify(pSdmmc, NULL, g_SDMMC_dataParam.trans_mode);
#else  /* dma-mode*/
		g_SDMMC_dataParam.trans_mode = sdmmc_trans_mode_dw_dma;  /* ;*/
		status = FSdmmcPs_cardInitializeVerify(pSdmmc, pSdmmc->instance->pDma, g_SDMMC_dataParam.trans_mode);
#endif

		if (status != FMSH_SUCCESS)
		{
			TRACE_OUT(DEBUG_OUT, "SDMMC verify case ....[ FAILED ] <%s mode> \r\n", \
					  (g_SDMMC_dataParam.trans_mode==1)?"dma":"fifo");
		}
		else
		{
			TRACE_OUT(DEBUG_OUT, "SDMMC verify case ....[ PASSED ] <%s mode> \r\n", \
					  (g_SDMMC_dataParam.trans_mode==1)?"dma":"fifo");
		}

		/*delay_ms(1000);*/
	}
#endif

    return FMSH_SUCCESS;
}




#if 1  /*sd_0*/

int g_test_sdmmc0 = 0;

void test_rd_sector0(int block_idx)
{
	int status = -1;
	
	UINT8 pBuf8[512+4] = {0};
	int j = 0;
	
	FSdmmcPs_T * pSdmmc = (FSdmmcPs_T *)(&g_SDMMC[0]);	/* SD-0*/
	
	if (g_test_sdmmc0 == 0)
	{
		g_test_sdmmc0 = 1;
		
		test_sdmmc_init();

		g_SDMMC_dataParam.block_num = block_idx; /* 2000;*/
		g_SDMMC_dataParam.trans_mode = sdmmc_trans_mode_normal;  /* sdmmc_trans_mode_dw_dma;*/
		
		g_SDMMC_dataParam.write_addr = (u32)FPS_AHB_SRAM_BASEADDR;
		g_SDMMC_dataParam.read_addr = (u32)(FPS_AHB_SRAM_BASEADDR + 0x10000);
		
		g_SDMMC_dataParam.write_type = 0;
		g_SDMMC_dataParam.read_type = 0;
		
		status = FSdmmcPs_cardInitializeVerify(pSdmmc, NULL, g_SDMMC_dataParam.trans_mode);
		if (status != FMSH_SUCCESS)
		{
			TRACE_OUT(DEBUG_OUT, "SDMMC verify case ....[ FAILED ]\r\n");
		}
		else
		{
			TRACE_OUT(DEBUG_OUT, "SDMMC verify case ....[ PASSED ]\r\n");
		}
	}

#if 0
	/*s32 FSdmmcPs_cardDataTransfer(FSdmmcPs_T *Sdmmc, enum SDMMC_TransMode trans_mode, u8 wr, u8 rd)	*/
	status = FSdmmcPs_cardDataTransfer(pSdmmc, g_SDMMC_dataParam.trans_mode, 0, 1);
	if (status != FMSH_SUCCESS)
	{
		TRACE_OUT(DEBUG_OUT, "SDMMC verify ....[ FAILED ] \r\n");
	}
	else
	{
		TRACE_OUT(DEBUG_OUT, "SDMMC verify ....[ PASSED ] \r\n");
	}
#endif
	
	FSdmmcPs_setClock(pSdmmc, 25000000);
	
	FSdmmcPs_readFun(pSdmmc, block_idx, &pBuf8[0], SDMMC_BLOCK_SIZE, sdmmc_trans_mode_normal);
	for (j=0; j<SDMMC_BLOCK_SIZE; j++)
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
	}

	return;
}


void test_wr_sector0(int block_idx)
{
	int status = -1;
	
	UINT8 pBuf8[512+4] = {0};
	int j = 0;
	
	FSdmmcPs_T * pSdmmc = (FSdmmcPs_T *)(&g_SDMMC[0]);	/* SD-0*/
	
	if (g_test_sdmmc0 == 0)
	{
		g_test_sdmmc0 = 1;
		
		test_sdmmc_init();

		g_SDMMC_dataParam.block_num = block_idx; /* 2000;*/
		g_SDMMC_dataParam.trans_mode = sdmmc_trans_mode_normal;  /* sdmmc_trans_mode_dw_dma;*/
		g_SDMMC_dataParam.write_addr = (u32)FPS_AHB_SRAM_BASEADDR;
		g_SDMMC_dataParam.read_addr = (u32)(FPS_AHB_SRAM_BASEADDR + 0x10000);
		g_SDMMC_dataParam.write_type = 0;
		g_SDMMC_dataParam.read_type = 0;
		
		status = FSdmmcPs_cardInitializeVerify(pSdmmc, NULL, g_SDMMC_dataParam.trans_mode);
		if (status != FMSH_SUCCESS)
		{
			TRACE_OUT(DEBUG_OUT, "SDMMC verify case ....[ FAILED ]\r\n");
		}
		else
		{
			TRACE_OUT(DEBUG_OUT, "SDMMC verify case ....[ PASSED ]\r\n");
		}
	}

#if 0
	/*s32 FSdmmcPs_cardDataTransfer(FSdmmcPs_T *Sdmmc, enum SDMMC_TransMode trans_mode, u8 wr, u8 rd)	*/
	status = FSdmmcPs_cardDataTransfer(pSdmmc, g_SDMMC_dataParam.trans_mode, 0, 1);
	if (status != FMSH_SUCCESS)
	{
		TRACE_OUT(DEBUG_OUT, "SDMMC verify ....[ FAILED ] \r\n");
	}
	else
	{
		TRACE_OUT(DEBUG_OUT, "SDMMC verify ....[ PASSED ] \r\n");
	}
#endif
	
	for (j=0; j<SDMMC_BLOCK_SIZE; j++)
	{
		pBuf8[j] = j + g_test_sdmmc0;
	}
	g_test_sdmmc0++;
	
	FSdmmcPs_writeFun(pSdmmc, block_idx, &pBuf8[0], SDMMC_BLOCK_SIZE, sdmmc_trans_mode_normal);
	
	for (j=0; j<SDMMC_BLOCK_SIZE; j++)
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
	}

	return;
}
#endif


#if 1  /* sd_1*/

int g_test_sdmmc1 = 0;

void test_rd_sector1(int block_idx)
{
	int status = -1;
	
	UINT8 pBuf8[512+4] = {0};
	int j = 0;
	
	FSdmmcPs_T * pSdmmc = (FSdmmcPs_T *)(&g_SDMMC[1]);	/* SD1*/
	
	if (g_test_sdmmc1 == 0)
	{
		g_test_sdmmc1 = 1;
		
		test_sdmmc_init();

		g_SDMMC_dataParam.block_num = block_idx; /* 2000;*/
		g_SDMMC_dataParam.trans_mode = sdmmc_trans_mode_normal;  /* sdmmc_trans_mode_dw_dma;*/
		
		g_SDMMC_dataParam.write_addr = (u32)FPS_AHB_SRAM_BASEADDR;
		g_SDMMC_dataParam.read_addr = (u32)(FPS_AHB_SRAM_BASEADDR + 0x10000);
		
		g_SDMMC_dataParam.write_type = 0;
		g_SDMMC_dataParam.read_type = 0;
		
		status = FSdmmcPs_cardInitializeVerify(pSdmmc, NULL, g_SDMMC_dataParam.trans_mode);
		if (status != FMSH_SUCCESS)
		{
			TRACE_OUT(DEBUG_OUT, "SDMMC verify case ....[ FAILED ]\r\n");
		}
		else
		{
			TRACE_OUT(DEBUG_OUT, "SDMMC verify case ....[ PASSED ]\r\n");
		}
	}

#if 0
	/*s32 FSdmmcPs_cardDataTransfer(FSdmmcPs_T *Sdmmc, enum SDMMC_TransMode trans_mode, u8 wr, u8 rd)	*/
	status = FSdmmcPs_cardDataTransfer(pSdmmc, g_SDMMC_dataParam.trans_mode, 0, 1);
	if (status != FMSH_SUCCESS)
	{
		TRACE_OUT(DEBUG_OUT, "SDMMC verify ....[ FAILED ] \r\n");
	}
	else
	{
		TRACE_OUT(DEBUG_OUT, "SDMMC verify ....[ PASSED ] \r\n");
	}
#endif
	
	FSdmmcPs_setClock(pSdmmc, 25000000);
	
	FSdmmcPs_readFun(pSdmmc, block_idx, &pBuf8[0], SDMMC_BLOCK_SIZE, sdmmc_trans_mode_normal);
	for (j=0; j<SDMMC_BLOCK_SIZE; j++)
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
	}

	return;
}


void test_wr_sector1(int block_idx)
{
	int status = -1;
	
	UINT8 pBuf8[512+4] = {0};
	int j = 0;
	
	FSdmmcPs_T * pSdmmc = (FSdmmcPs_T *)(&g_SDMMC[1]);	/* SD1*/
	
	if (g_test_sdmmc1 == 0)
	{
		g_test_sdmmc1 = 1;
		
		test_sdmmc_init();

		g_SDMMC_dataParam.block_num = block_idx; /* 2000;*/
		g_SDMMC_dataParam.trans_mode = sdmmc_trans_mode_normal;  /* sdmmc_trans_mode_dw_dma;*/
		g_SDMMC_dataParam.write_addr = (u32)FPS_AHB_SRAM_BASEADDR;
		g_SDMMC_dataParam.read_addr = (u32)(FPS_AHB_SRAM_BASEADDR + 0x10000);
		g_SDMMC_dataParam.write_type = 0;
		g_SDMMC_dataParam.read_type = 0;
		
		status = FSdmmcPs_cardInitializeVerify(pSdmmc, NULL, g_SDMMC_dataParam.trans_mode);
		if (status != FMSH_SUCCESS)
		{
			TRACE_OUT(DEBUG_OUT, "SDMMC verify case ....[ FAILED ]\r\n");
		}
		else
		{
			TRACE_OUT(DEBUG_OUT, "SDMMC verify case ....[ PASSED ]\r\n");
		}
	}

#if 0
	/*s32 FSdmmcPs_cardDataTransfer(FSdmmcPs_T *Sdmmc, enum SDMMC_TransMode trans_mode, u8 wr, u8 rd)	*/
	status = FSdmmcPs_cardDataTransfer(pSdmmc, g_SDMMC_dataParam.trans_mode, 0, 1);
	if (status != FMSH_SUCCESS)
	{
		TRACE_OUT(DEBUG_OUT, "SDMMC verify ....[ FAILED ] \r\n");
	}
	else
	{
		TRACE_OUT(DEBUG_OUT, "SDMMC verify ....[ PASSED ] \r\n");
	}
#endif
	
	for (j=0; j<SDMMC_BLOCK_SIZE; j++)
	{
		pBuf8[j] = j + g_test_sdmmc1;
	}
	g_test_sdmmc1++;
	
	FSdmmcPs_writeFun(pSdmmc, block_idx, &pBuf8[0], SDMMC_BLOCK_SIZE, sdmmc_trans_mode_normal);
	
	for (j=0; j<SDMMC_BLOCK_SIZE; j++)
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
	}

	return;
}
#endif

#endif


#if 0

u32 test_sd_file(void)
{
	FRESULT res_sd;
	UINT bw;
	u32 i;
	u32 *src, *dst, dataLen;
	u8 work[FF_MAX_SS]; /* Work area (larger is better for process time) */

	src = (u32*)FPS_AHB_SRAM_BASEADDR;
	dst = (u32*)(FPS_AHB_SRAM_BASEADDR + 0x10000);
	dataLen = 512;

	for(i = 0; i < dataLen; i++)
    {
    	*src = i + 1;
		src++;
    }
	src = (u32*)FPS_AHB_SRAM_BASEADDR;

	memset(dst, 0, dataLen * sizeof(u32));

	res_sd = f_mount(&fatfs, "0:/", 0);
	
	/*----------------------- Format test ---------------------------*/
	/* Format and create file system if no file in SD card */
	if(res_sd == FR_NO_FILESYSTEM)
	{
		TRACE_OUT(DEBUG_OUT, "> No file in SD card, will format first...\r\n");
		
		/* Format */
		res_sd = f_mkfs("0:", FM_FAT32, 0, work, sizeof work);
		if(res_sd == FR_OK)
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
	else if(res_sd != FR_OK)
	{
		TRACE_OUT(DEBUG_OUT, "> SD card mount file system failed.(%d)\r\n", res_sd);
		return FMSH_FAILURE;
	}
	else
	{
		TRACE_OUT(DEBUG_OUT, "> File system mount succeed, begin to write/read test.\r\n");
	}


	
	/*
	--------------------- Write test -----------------------
	*/
	
	/* 
	Open file or create file if no file in SD card 
	*/
	TRACE_OUT(DEBUG_OUT, "****** Start file write test... ******\r\n");
	
	res_sd = f_open(&fil, "0:test.txt", FA_CREATE_ALWAYS|FA_WRITE);
	if(res_sd == FR_OK)
	{
		TRACE_OUT(DEBUG_OUT, "> Open/create test.txt file succeed, start to write data into the file.\r\n");
		
		/* Write data to the file */
		res_sd = f_write(&fil, (void*)src, dataLen, &bw);
		if (res_sd == FR_OK)
		{
			TRACE_OUT(DEBUG_OUT, "> File write succeed, the write data is: %d\r\n", bw);
		}
		else
		{
			TRACE_OUT(DEBUG_OUT, "> File write failed: (%d)\r\n", res_sd);
		}
		
		/* close the file */
		res_sd = f_close(&fil);
		if (res_sd != FR_OK)
		{
			return FMSH_FAILURE;
		}
	}
	else
	{
		TRACE_OUT(DEBUG_OUT, "> Open/create file failed.\r\n");
	}

	
	
	/*
	------------------ Read test --------------------------
	*/
	
	TRACE_OUT(DEBUG_OUT, "****** Start file read test... ******\r\n");
	
	res_sd=f_open(&fil, "0:test.txt", FA_OPEN_EXISTING|FA_READ);
	if(res_sd == FR_OK)
	{
		TRACE_OUT(DEBUG_OUT, "> open file succeed.\r\n");
		res_sd = f_read(&fil, (void*)dst, dataLen, &bw);
		if (res_sd==FR_OK)
		{
			TRACE_OUT(DEBUG_OUT, "> File read succeed, the read data is: %d\r\n", bw);
		}
		else
		{
			TRACE_OUT(DEBUG_OUT, "> File read failed: (%d)\n", res_sd);
		}
	}
	else
	{
		TRACE_OUT(DEBUG_OUT, "> Open file failed.\r\n");
	}

	
	/* 
	close the file 
	*/
	res_sd = f_close(&fil);
	if (res_sd != FR_OK)
	{
		return FMSH_FAILURE;
	}

	
	/* 
	unuse the file system, cancel the mounted file system 
	*/
	f_mount(NULL, "0:", 0);

	src = (u32*)FPS_AHB_SRAM_BASEADDR;
	dst = (u32*)(FPS_AHB_SRAM_BASEADDR + 0x10000);
	for(i = 0; i < (dataLen/4); i++)
	{
		if(*dst != *src)
		{
			TRACE_OUT(DEBUG_OUT, "Write/Read file data error, data[%d]--w:%x, r:%x!\r\n", i, *src, *dst);
			return FMSH_FAILURE;
		}
		dst++;
		src++;
	}
	
	return FMSH_SUCCESS;
}


/* Search a directory for objects and display it */

void find_text_file(void)
{
    FRESULT fr;     /* Return value */
    DIR dj;         /* Directory object */
    FILINFO fno;    /* File information */
	
	FRESULT res_sd;
	UINT bw;
	u32 i;
	u32 *src, *dst, dataLen;
	u8 work[FF_MAX_SS]; /* Work area (larger is better for process time) */

	src = (u32*)FPS_AHB_SRAM_BASEADDR;
	dst = (u32*)(FPS_AHB_SRAM_BASEADDR + 0x10000);
	dataLen = 512;
	for(i = 0; i < dataLen; i++)
    {
    	*src = i + 1;
		src++;
    }
	src = (u32*)FPS_AHB_SRAM_BASEADDR;
	memset(dst, 0, dataLen * sizeof(u32));

	res_sd = f_mount(&fatfs, "0:/", 0);
	
	/*----------------------- Format test ---------------------------*/
	/* Format and create file system if no file in SD card */
	if(res_sd == FR_NO_FILESYSTEM)
	{
		TRACE_OUT(DEBUG_OUT, "> No file in SD card, will format first...\r\n");
		
		/* Format */
		res_sd = f_mkfs("0:", FM_FAT32, 0, work, sizeof work);
		if(res_sd == FR_OK)
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
	else if(res_sd != FR_OK)
	{
		TRACE_OUT(DEBUG_OUT, "> SD card mount file system failed.(%d)\r\n", res_sd);
		return FMSH_FAILURE;
	}
	else
	{
		TRACE_OUT(DEBUG_OUT, "> File system mount succeed, begin to write/read test.\r\n");
	}
	
	/*
	fr = f_opendir(&dj, "0:");
	if (fr != FR_OK)
	{
		printf("f_opendir fail:%d! \n", fr);
		return;
	}
	else
	{
		printf("f_opendir ok:%d! \n", fr);
	}
	*/

    fr = f_findfirst(&dj, &fno, "0:", "*.*");  /* Start to search for photo files */
	if (fr != FR_OK)
	{
		printf("empty:%d! \n", fr);
		return;
	}

    while ((fr == FR_OK) && fno.fname[0])           /* Repeat while an item is found */
	{         
        printf("%s \n", fno.fname);               /* Print the object name */
        fr = f_findnext(&dj, &fno);               /* Search for next item */
    }

    f_closedir(&dj);
	return;
}



#endif

#if 0

static int g_sdmmc_fs_mount = 0;

FRESULT scan_files (char* path)
{
    FRESULT res;
    FILINFO fno;
    DIR dir;
    int i;
	int idx = 0;

    res = f_opendir(&dir, path);
    if (res == FR_OK) 
	{
        i = strlen(path);
		
        for (;;) 
		{
            res = f_readdir(&dir, &fno);
			
            if (res != FR_OK || fno.fname[0] == 0) 
				break;
			
            if (fno.fattrib & AM_DIR) 
			{
                sprintf(&path[i], "/%s", fno.fname);
				
                res = scan_files(path);
				
                if (res != FR_OK) 
					break;
				
                path[i] = 0;
            } 
			else 
			{
				#if 1
                	printf("%s/%s \n", path, fno.fname);
                #else
					idx = 0;
	                while (path[idx] != '\0')
                	{
                		idx++;
                	}
					
					if (path[idx-1] == '/')
					{
						printf("%s%s \n", path, fno.fname);
					}
					else
					{
						printf("%s/%s \n", path, fno.fname);
					}
				#endif
            }
        }
    }
	
    res = f_closedir(&dir);

    return res;
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
			
			/* Format */
			res_sd = f_mkfs("0:", FM_FAT32, 0, work, sizeof(work));
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
		else if (res_sd != FR_OK)
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
sdls "0:"
sdls "0:/"
*/
void sdls(char * pPathName)
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


#if 0

extern FDmaPs_T g_DMA_dmac;
extern FDmaPs_Param_T g_DMA_param;
extern FDmaPs_Instance_T g_DMA_instance;


s32 FSdmmcPs_cardDataTransfer2(FSdmmcPs_T *pSdmmc, enum SDMMC_TransMode trans_mode, u8 wr, u8 rd)
{
    u8 error = 0;
    u32 i, test_cnt = 0, errcnt = 0;
    u32 multiBlock, block, splitMulti, totalBlockNum;
	u32 dataLen, rwDataLen;
	char8 *pWriteBuf, *pReadBuf;
	
    /* Perform tests on each block */
    multiBlock = 0;
	totalBlockNum = 500;

	pWriteBuf = (char8 *)FPS_AHB_SRAM_BASEADDR;
	pReadBuf = (char8 *)(FPS_AHB_SRAM_BASEADDR + 0x10000);
	
    for (block = 0; block < totalBlockNum; block += multiBlock)
	{
		#if 0
        /* Perform different single or multiple bloc operations */
		if (test_cnt < 5)
		{
			multiBlock = 1;
		}
		else
		{
	        if (multiBlock >= 8)
	    	{
				multiBlock = MAX_MULTI_BLOCKS;
	    	}
	        else
	    	{
	    		multiBlock ++;
	    	}
		}

        /* Multi-block adjustment */
        if (block + multiBlock > totalBlockNum)
		{
            multiBlock = totalBlockNum - block;
        }
		#else
			multiBlock = 1;
		#endif

        /* ** Perform single block or multi block transfer */
        TRACE_OUT(DEBUG_OUT, "-I- Testing block [%6u - %6u] ...\r\n", block, (block + multiBlock -1));
		
		dataLen = SDMMC_BLOCK_SIZE * multiBlock;

        if (wr) 
		{
			memset(pWriteBuf, 0, dataLen * sizeof(char8));
			
	        /* - Write a checkerboard pattern on the block */
	        for (i = 0; i < dataLen; i++)
			{
	            if ((i & 1) == 0) 
					pWriteBuf[i] = (i & 0x55);
	            else
	            	pWriteBuf[i] = (i & 0xAA);
	        }
			
            for (i = 0; i < multiBlock; )
			{
                splitMulti = ((multiBlock - i) > NB_SPLIT_MULTI) ? NB_SPLIT_MULTI : (multiBlock - i);
				rwDataLen = SDMMC_BLOCK_SIZE * splitMulti;
				
                error = FSdmmcPs_writeFun(pSdmmc, block + i, &pWriteBuf[i * SDMMC_BLOCK_SIZE], rwDataLen, trans_mode);
                if (error)
					break;
				else
                	i += splitMulti;
            }
			
            if (error)
            {
                TRACE_OUT(DEBUG_OUT, "-E- 2. Write block #%u(%u+%u): %d\n\r",  (block+i), block, i, error);
                if(MaxErrorBreak(0))
					return FMSH_FAILURE;
				else
	                continue; /* Skip following test */
            }
        }

        if (rd)
		{
            /* - Read back the data to check the write operation */
            memset(pReadBuf, 0, dataLen * sizeof(char8));
			
            /*for (i = 0; i < multiBlock; )
			{
                splitMulti = ((multiBlock - i) > NB_SPLIT_MULTI) ? NB_SPLIT_MULTI : (multiBlock - i);
				rwDataLen = SDMMC_BLOCK_SIZE * splitMulti;
				
                error = FSdmmcPs_readFun(pSdmmc, block + i, &pReadBuf[i * SDMMC_BLOCK_SIZE], rwDataLen, trans_mode);
                if (error) 
					break;
				else
                	i += splitMulti;
            }
			*/
			error = FSdmmcPs_readFun(pSdmmc, block, &pReadBuf[0], SDMMC_BLOCK_SIZE, trans_mode);
			
            if (error)
            {
                TRACE_OUT(DEBUG_OUT, "-E- 2. Read block #%u(%u+%u): %d\n\r", (block + i), block, i, error);

				if (MaxErrorBreak(0))
					return FMSH_FAILURE;
				else
	                continue; /* Skip following test */
            }
			
            errcnt = 0;
			
            for (i=0; i < dataLen; i++)
			{
                if (pReadBuf[i] != pWriteBuf[i])
                {
                    uint32_t j, js;
					
                    TRACE_OUT(DEBUG_OUT, "\n\r-E- 2.%d. Data @ %u (0x%x)\n\r", errcnt, i, i);
                    TRACE_OUT(DEBUG_OUT, "  -Src:");
					
                    js = (i > 8) ? (i - 8) : 0;
					
                    for (j = js; j < i + 8; j ++)
                        TRACE_OUT(DEBUG_OUT, " %02x", pWriteBuf[j]);
					
                    TRACE_OUT(DEBUG_OUT, "\n\r  -Dat:");
					
                    for (j = js; j < i + 8; j ++)
                        TRACE_OUT(DEBUG_OUT, "%c%02x", (i == j) ? '!' : ' ', pReadBuf[j]);
					
                    TRACE_OUT(DEBUG_OUT, "\n\r");
					
                    if (MaxErrorBreak(0))
						return FMSH_FAILURE;
					
                    /* Only find first 3 verify error.                    */
                    if (errcnt ++ >= 3)
                        break;
                }
            }
        }

		test_cnt++;
    }

    return FMSH_SUCCESS;
}

s32 sdmmc_test_example(u8 id)
{
	s32 status = FMSH_FAILURE;
    u32 int_id;
    enum SDMMC_TransMode trans_mode;
    FSdmmcPs_Config *pSdCfg;
	FSdmmcPs_Instance_T instance;
    FDmaPs_Config *pDmaCfg;
	
    FDmaPs_T *pDmac = &g_DMA_dmac;
	
	FDmaPs_Param_T dma_param;
	FDmaPs_Instance_T dma_instance;

    if (id == 0)
    {
        int_id = 52;  /* SDMCC0_INT_ID;*/
        trans_mode = sdmmc_trans_mode_dw_dma;
    }
    else
    {
        int_id = 73; /* SDMCC1_INT_ID;*/
        trans_mode = sdmmc_trans_mode_normal;
    }
    
    /* Initialize the SD Driver */
    pSdCfg = FSdmmcPs_LookupConfig((u16)id);
    if (pSdCfg == NULL)
    {
		return FMSH_FAILURE;
    }
	
	FSdmmcPs_initDev(&g_SDMMC[id], &instance, pSdCfg);
	
	/*
	status = FGicPs_registerInt(&IntcInstance, int_id,
	          (FMSH_InterruptHandler)FSdmmcPs_IRQ, &g_SDMMC[id]);
	if (status != FMSH_SUCCESS)
	{
        return FMSH_FAILURE;
	}
	*/
	
	TRACE_OUT(DEBUG_OUT, "Begin to run SDMMC test example.\r\n");
	TRACE_OUT(DEBUG_OUT, "/ -------\r\n");

    /* Initialize the DMA Driver */
    pDmaCfg = FDmaPs_LookupConfig(FPAR_DMAPS_DEVICE_ID);
    if (pDmaCfg == NULL) {
        return FMSH_FAILURE;
    }
	
    FDmaPs_initDev(pDmac, &dma_instance, &dma_param, pDmaCfg);
	
	/*///////////////////////////////*/
   /*
   status = FGicPs_registerInt(&IntcInstance, DMA_INT_ID,
              (FMSH_InterruptHandler)FDmaPs_IRQ, pDmac);
	if (status != FMSH_SUCCESS)
	{
        return FMSH_FAILURE;
	}
	*/
	/*///////////////////////////////*/
	
	status = FDmaPs_autoCompParams(pDmac);
	if (status != FMSH_SUCCESS)
	{
        return FMSH_FAILURE;
	}
	
	status = FSdmmcPs_cardInitializeVerify(&g_SDMMC[id], pDmac, trans_mode);
	if (status != FMSH_SUCCESS)
	{
		TRACE_OUT(DEBUG_OUT, "SDMMC test example FAILED.\r\n");
        return FMSH_FAILURE;
	}
    
	status = FSdmmcPs_cardDataTransfer2(&g_SDMMC[id], trans_mode, 1, 1);
	if (status != FMSH_SUCCESS)
	{
		TRACE_OUT(DEBUG_OUT, "SDMMC test example FAILED(%s mode). \r\n", (trans_mode==1)?"dma":"fifo");
	}
	else
	{
		TRACE_OUT(DEBUG_OUT, "SDMMC test example PASSED(%s mode). \r\n", (trans_mode==1)?"dma":"fifo");
	}
		
	return status;
}



int FDmaPs_verify2(void)
{
    int status;
    FDmaPs_Config *pDmaCfg;
	
    FDmaPs_Instance_T *pInstance = &g_DMA_instance;
    FDmaPs_Param_T *pParam = &g_DMA_param;

    /* Initialize the DMA Driver */
    pDmaCfg = FDmaPs_LookupConfig(FPAR_DMAPS_DEVICE_ID);
    if (pDmaCfg == NULL) 
	{
        return FMSH_FAILURE;
    }
	
    FDmaPs_initDev(&g_DMA_dmac, pInstance, pParam, pDmaCfg);

    status = FDmaPs_autoCompParams(&g_DMA_dmac);
    if (status != FMSH_SUCCESS)
    {
        return FMSH_FAILURE;
    }

    return FMSH_SUCCESS;
}


int FSdmmcPs_verify2(void)
{
    int status;
    FSdmmcPs_Config *pSdCfg;

    /* Initialize the SD_0 Driver */
    pSdCfg = FSdmmcPs_LookupConfig(FPAR_SDPS_0_DEVICE_ID);
    if (pSdCfg == NULL) {
        return FMSH_FAILURE;
    }
	
    FSdmmcPs_initDev(&g_SDMMC[0], &g_SDMMC_instance[0], pSdCfg);
	
	/*
	status = FGicPs_registerInt(&IntcInstance, SDMCC0_INT_ID,
	          (FMSH_InterruptHandler)FSdmmcPs_IRQ, &g_SDMMC[0]);
    if (status != FMSH_SUCCESS)
    {
        return FMSH_FAILURE;
    }
	*/

    /* Initialize the SD_1 Driver */
    pSdCfg = FSdmmcPs_LookupConfig(FPAR_SDPS_1_DEVICE_ID);
    if (pSdCfg == NULL) {
        return FMSH_FAILURE;
    }
	
    FSdmmcPs_initDev(&g_SDMMC[1], &g_SDMMC_instance[1], pSdCfg);
	
	/*
	status = FGicPs_registerInt(&IntcInstance, SDMCC1_INT_ID,
	          (FMSH_InterruptHandler)FSdmmcPs_IRQ, &g_SDMMC[1]);
    if (status != FMSH_SUCCESS)
    {
        return FMSH_FAILURE;
    }
	*/

    memset(&g_SDMMC_dataParam, 0, sizeof(SDMMC_dataParam));

    return FMSH_SUCCESS;
}


s32 FSdmmcPs_memAccessVerify2(FSdmmcPs_T *pSdmmc, int case_list)
{
	s32 status = FMSH_FAILURE;
	s32 ret = FMSH_SUCCESS;
	
	/*u16 case_list = 0x0000;*/
	u8 check_done = 0;
	u16 i, index;
	u8 case_state[6];
	
	int dma_fifo_mode = sdmmc_trans_mode_dw_dma;
	int blk_num_max = 10;   /* 100 -> 10*/
    
	for (i = 0; i < 6; i++)
        case_state[i] = 0;

	status = FSdmmcPs_cardInitializeVerify(pSdmmc, &g_DMA_dmac, dma_fifo_mode);
    if (status != FMSH_SUCCESS)
	{
        return status;
    }

	switch (case_list)
	{
		case 0x0000:
		{
			TRACE_OUT(DEBUG_OUT, "Verify AHB_2_SDMMC0_2_AHB in DW-DMA transfer mode.\r\n");
			TRACE_OUT(DEBUG_OUT, "/ -------\r\n");
			
			g_SDMMC_dataParam.block_num = blk_num_max;
			g_SDMMC_dataParam.trans_mode = dma_fifo_mode;
			g_SDMMC_dataParam.write_addr = (u32)FPS_AHB_SRAM_BASEADDR;
			g_SDMMC_dataParam.read_addr = (u32)(FPS_AHB_SRAM_BASEADDR + 0x10000);
			g_SDMMC_dataParam.write_type = 0;
			g_SDMMC_dataParam.read_type = 0;
			break;
		}
		case 0x0001:
		{
			TRACE_OUT(DEBUG_OUT, "Verify AHB_2_SDMMC0_2_AXI in DW-DMA transfer mode.\r\n");
			TRACE_OUT(DEBUG_OUT, "/ -------\r\n");
			
			g_SDMMC_dataParam.block_num = blk_num_max;
			g_SDMMC_dataParam.trans_mode = dma_fifo_mode;
			g_SDMMC_dataParam.write_addr = (u32)FPS_AHB_SRAM_BASEADDR;
			g_SDMMC_dataParam.read_addr = (u32)0x10000000;   /* 0x55000;*/
			g_SDMMC_dataParam.write_type = 0;
			g_SDMMC_dataParam.read_type = 0;
			break;
		}
		case 0x0002:
		{
			TRACE_OUT(DEBUG_OUT, "Verify AHB_2_SDMMC0_2_DDR in DW-DMA transfer mode.\r\n");
			TRACE_OUT(DEBUG_OUT, "/ -------\r\n");
			
			g_SDMMC_dataParam.block_num = blk_num_max;
			g_SDMMC_dataParam.trans_mode = dma_fifo_mode;
			g_SDMMC_dataParam.write_addr = (u32)FPS_AHB_SRAM_BASEADDR;
			g_SDMMC_dataParam.read_addr = (u32)0x10000000;   /* FPS_DDR3MEM_BASEADDR;*/
			g_SDMMC_dataParam.write_type = 0;
			g_SDMMC_dataParam.read_type = 0;
			
			break;
		}
		case 0x0003:
		{
			TRACE_OUT(DEBUG_OUT, "Verify AXI_2_SDMMC0_2_AHB in DW-DMA transfer mode.\r\n");
			TRACE_OUT(DEBUG_OUT, "/ -------\r\n");
			
			g_SDMMC_dataParam.block_num = blk_num_max;
			g_SDMMC_dataParam.trans_mode = dma_fifo_mode;
			g_SDMMC_dataParam.write_addr = (u32)0x10000000;   /* 0x55000;*/
			g_SDMMC_dataParam.read_addr = (u32)FPS_AHB_SRAM_BASEADDR;
			g_SDMMC_dataParam.write_type = 0;
			g_SDMMC_dataParam.read_type = 0;
			
			break;
		}
		case 0x0004:
		{
			TRACE_OUT(DEBUG_OUT, "Verify DDR_2_SDMMC0_2_AHB in DW-DMA transfer mode.\r\n");
			TRACE_OUT(DEBUG_OUT, "/ -------\r\n");
			
			g_SDMMC_dataParam.block_num = blk_num_max;
			g_SDMMC_dataParam.trans_mode = dma_fifo_mode;
			g_SDMMC_dataParam.write_addr = (u32)0x10000000;   /* FPS_DDR3MEM_BASEADDR;*/
			g_SDMMC_dataParam.read_addr = (u32)FPS_AHB_SRAM_BASEADDR;
			g_SDMMC_dataParam.write_type = 0;
			g_SDMMC_dataParam.read_type = 0;
			
			break;
		}
		default:
		{
			break;
		}
	/*==============================		   */
	}

	int wr = 0;  /* 0-disable, 1-enable*/
	int rd = 1;
	
	status = FSdmmcPs_cardDataTransfer(pSdmmc, g_SDMMC_dataParam.trans_mode, wr, rd);
    if (status != FMSH_SUCCESS)
	{
        TRACE_OUT(DEBUG_OUT, "SDMMC verify case[0x%04X] ....[ FAILED ]\r\n", case_list);
		ret |= FMSH_FAILURE;
    }
	else
	{
        TRACE_OUT(DEBUG_OUT, "SDMMC verify case[0x%04X] ....[ PASSED ]\r\n", case_list);
    }

	return status;
}


/**/
/* dma_mode*/
/**/
void test_sdmmc_dma(int case_list)
{
	int status = 0;
	
	FDmaPs_verify2();
	
	FSdmmcPs_verify2();

	
	TRACE_OUT(DEBUG_OUT, "Verify SDMMC memory access in DW-DMA transfer mode.\r\n");
	TRACE_OUT(DEBUG_OUT, "/ -------\r\n");

	status = FSdmmcPs_memAccessVerify2(&g_SDMMC[0], case_list);
	if (status != FMSH_SUCCESS)
	{
		TRACE_OUT(DEBUG_OUT, "SDMMC verify case[0x%04X] ....[ FAILED ]<dma_mode> \r\n", case_list);
	}
	else
	{
		TRACE_OUT(DEBUG_OUT, "SDMMC verify case[0x%04X] ....[ PASSED ]<dma_mode> \r\n", case_list);
	}

	return;
}


s32 FSdmmcPs_memAccessVerify3(FSdmmcPs_T *pSdmmc, int case_list)
{
	s32 status = FMSH_FAILURE;
	s32 ret = FMSH_SUCCESS;
	
	/*u16 case_list = 0x0000;*/
	u8 check_done = 0;
	u16 i, index;
	u8 case_state[6];
	
	int dma_fifo_mode = sdmmc_trans_mode_normal;
	int blk_num_max = 10;   /* 100 -> 10*/
    
	for (i = 0; i < 6; i++)
        case_state[i] = 0;

	status = FSdmmcPs_cardInitializeVerify(pSdmmc, &g_DMA_dmac, dma_fifo_mode);
    if (status != FMSH_SUCCESS)
	{
        return status;
    }

	switch (case_list)
	{
		case 0x0000:
		{
			TRACE_OUT(DEBUG_OUT, "Verify AHB_2_SDMMC0_2_AHB in DW-DMA transfer mode.\r\n");
			TRACE_OUT(DEBUG_OUT, "/ -------\r\n");
			
			g_SDMMC_dataParam.block_num = blk_num_max;
			g_SDMMC_dataParam.trans_mode = dma_fifo_mode;
			g_SDMMC_dataParam.write_addr = (u32)FPS_AHB_SRAM_BASEADDR;
			g_SDMMC_dataParam.read_addr = (u32)(FPS_AHB_SRAM_BASEADDR + 0x10000);
			g_SDMMC_dataParam.write_type = 0;
			g_SDMMC_dataParam.read_type = 0;
			break;
		}
		case 0x0001:
		{
			TRACE_OUT(DEBUG_OUT, "Verify AHB_2_SDMMC0_2_AXI in DW-DMA transfer mode.\r\n");
			TRACE_OUT(DEBUG_OUT, "/ -------\r\n");
			
			g_SDMMC_dataParam.block_num = blk_num_max;
			g_SDMMC_dataParam.trans_mode = dma_fifo_mode;
			g_SDMMC_dataParam.write_addr = (u32)FPS_AHB_SRAM_BASEADDR;
			g_SDMMC_dataParam.read_addr = (u32)0x10000000;   /* 0x55000;*/
			g_SDMMC_dataParam.write_type = 0;
			g_SDMMC_dataParam.read_type = 0;
			break;
		}
		case 0x0002:
		{
			TRACE_OUT(DEBUG_OUT, "Verify AHB_2_SDMMC0_2_DDR in DW-DMA transfer mode.\r\n");
			TRACE_OUT(DEBUG_OUT, "/ -------\r\n");
			
			g_SDMMC_dataParam.block_num = blk_num_max;
			g_SDMMC_dataParam.trans_mode = dma_fifo_mode;
			g_SDMMC_dataParam.write_addr = (u32)FPS_AHB_SRAM_BASEADDR;
			g_SDMMC_dataParam.read_addr = (u32)0x10000000;   /* FPS_DDR3MEM_BASEADDR;*/
			g_SDMMC_dataParam.write_type = 0;
			g_SDMMC_dataParam.read_type = 0;
			
			break;
		}
		case 0x0003:
		{
			TRACE_OUT(DEBUG_OUT, "Verify AXI_2_SDMMC0_2_AHB in DW-DMA transfer mode.\r\n");
			TRACE_OUT(DEBUG_OUT, "/ -------\r\n");
			
			g_SDMMC_dataParam.block_num = blk_num_max;
			g_SDMMC_dataParam.trans_mode = dma_fifo_mode;
			g_SDMMC_dataParam.write_addr = (u32)0x10000000;   /* 0x55000;*/
			g_SDMMC_dataParam.read_addr = (u32)FPS_AHB_SRAM_BASEADDR;
			g_SDMMC_dataParam.write_type = 0;
			g_SDMMC_dataParam.read_type = 0;
			
			break;
		}
		case 0x0004:
		{
			TRACE_OUT(DEBUG_OUT, "Verify DDR_2_SDMMC0_2_AHB in DW-DMA transfer mode.\r\n");
			TRACE_OUT(DEBUG_OUT, "/ -------\r\n");
			
			g_SDMMC_dataParam.block_num = blk_num_max;
			g_SDMMC_dataParam.trans_mode = dma_fifo_mode;
			g_SDMMC_dataParam.write_addr = (u32)0x10000000;   /* FPS_DDR3MEM_BASEADDR;*/
			g_SDMMC_dataParam.read_addr = (u32)FPS_AHB_SRAM_BASEADDR;
			g_SDMMC_dataParam.write_type = 0;
			g_SDMMC_dataParam.read_type = 0;
			
			break;
		}
		default:
		{
			break;
		}
	/*==============================		   */
	}
	
	int wr = 0;  /* 0-disable, 1-enable*/
	int rd = 1;

	status = FSdmmcPs_cardDataTransfer(pSdmmc, g_SDMMC_dataParam.trans_mode, wr, rd);
    if (status != FMSH_SUCCESS)
	{
        TRACE_OUT(DEBUG_OUT, "SDMMC verify case[0x%04X] ....[ FAILED ]\r\n", case_list);
		ret |= FMSH_FAILURE;
    }
	else
	{
        TRACE_OUT(DEBUG_OUT, "SDMMC verify case[0x%04X] ....[ PASSED ]\r\n", case_list);
    }

	return status;
}


/**/
/* fifo_mode*/
/**/
void test_sdmmc_fifo(int case_list)
{
	int status = 0;
	
	FDmaPs_verify2();
	
	FSdmmcPs_verify2();

	
	TRACE_OUT(DEBUG_OUT, "Verify SDMMC memory access in DW-DMA transfer mode.\r\n");
	TRACE_OUT(DEBUG_OUT, "/ -------\r\n");

	status = FSdmmcPs_memAccessVerify3(&g_SDMMC[0], case_list);
	if (status != FMSH_SUCCESS)
	{
		TRACE_OUT(DEBUG_OUT, "SDMMC verify case[0x%04X] ....[ FAILED ]<fifo_mode> \r\n", case_list);
	}
	else
	{
		TRACE_OUT(DEBUG_OUT, "SDMMC verify case[0x%04X] ....[ PASSED ]<fifo_mode> \r\n", case_list);
	}

	return;
}
#endif

#if 1  /* mfq*/

static UINT8 g_pBuf8_bare[512+4] = {0};

SEM_ID semSync_mmc;

/*(void)semTake (pDrvCtrl->muxSem, WAIT_FOREVER);*/
/*semSync_mmc = semBCreate (SEM_Q_PRIORITY, SEM_FULL);*/
/*(semTake (pDrvCtrl->semSync, timeoutTick)*/

/*#define get_sd_blk_num()   (64)    // for multi-block test*/
int g_sd_blk_num;
int  get_sd_blk_num(void)   /* for multi-block test*/
{
	return g_sd_blk_num;
}


void test_rd_sector(int block_idx,UINT8 * rdBuf)
{
	int status = -1;
	
	/*UINT8 pBuf8[512+4] = {0};*/
	UINT8* pBuf8 = rdBuf;/*(UINT8*(&g_pBuf8_bare[0]);*/
	
	int j = 0;
	u32 data_len = 0;
	
	FSdmmcPs_T * pSdmmc = (FSdmmcPs_T *)(&g_SDMMC[0]);	/* SD0*/
	
	semTake (semSync_mmc, WAIT_FOREVER);	

	/*FSdmmcPs_readFun(pSdmmc, block_idx, &pBuf8[0], SDMMC_BLOCK_SIZE, sdmmc_trans_mode_normal);*/
	data_len = get_sd_blk_num() * SDMMC_BLOCK_SIZE;
	FSdmmcPs_readFun(pSdmmc, block_idx, &pBuf8[0], data_len, sdmmc_trans_mode_normal);
	
	semGive(semSync_mmc);
	return;
}

void test_wr_sector(int block_idx,UINT8 * wrBuf)
{
	int status = -1;
	
	/*UINT8 pBuf8[512+4] = {0};	*/
	UINT8* pBuf8 = wrBuf;/*(UINT8*(&g_pBuf8_bare[0]);*/
	
	int j = 0;
	u32 data_len = 0;
	
	FSdmmcPs_T * pSdmmc = (FSdmmcPs_T *)(&g_SDMMC[0]);	/* SD0*/
	
	semTake (semSync_mmc, WAIT_FOREVER);
	
	if (g_test_sdmmc == 0)
	{
		g_test_sdmmc = 1;
		
		test_sdmmc_init();

		g_SDMMC_dataParam.block_num = block_idx; /* 2000;*/
		
		g_SDMMC_dataParam.trans_mode = sdmmc_trans_mode_normal;  /* sdmmc_trans_mode_dw_dma;*/
		/*g_SDMMC_dataParam.trans_mode = sdmmc_trans_mode_dw_dma;  // sdmmc_trans_mode_dw_dma;*/
		
		g_SDMMC_dataParam.write_addr = (u32)FPS_AHB_SRAM_BASEADDR;
		g_SDMMC_dataParam.read_addr = (u32)(FPS_AHB_SRAM_BASEADDR + 0x10000);
		g_SDMMC_dataParam.write_type = 0;
		g_SDMMC_dataParam.read_type = 0;
		
		status = FSdmmcPs_cardInitializeVerify(pSdmmc, NULL, g_SDMMC_dataParam.trans_mode);
		if (status != FMSH_SUCCESS)
		{
			TRACE_OUT(DEBUG_OUT, "SDMMC verify case ....[ FAILED ]\r\n");
		}
		else
		{
			TRACE_OUT(DEBUG_OUT, "SDMMC verify case ....[ PASSED ]\r\n");
		}
	}

	/**/
	/*FSdmmcPs_writeFun(pSdmmc, block_idx, &pBuf8[0], SDMMC_BLOCK_SIZE, sdmmc_trans_mode_normal);*/
	/**/
	data_len = get_sd_blk_num() * SDMMC_BLOCK_SIZE;
	FSdmmcPs_writeFun(pSdmmc, block_idx, &pBuf8[0], data_len, sdmmc_trans_mode_normal);

	semGive(semSync_mmc);
	return;
}

#endif

#if 1  /* mfq*/

int g_test_sd = 0;

void test_mmc_w_r_sects_bare1(int from_sect, int to_sect)
{
	float speed = 0.0;
	UINT32 all_size = 0;
	int tick_start = 0, tick_end = 0;
	
	semSync_mmc = semBCreate (SEM_Q_PRIORITY, SEM_FULL);

	int sect = 0;/*(to_sect - from_sect)/get_sd_blk_num();	*/
	
/*	int ctrl_x = SDMMC_CTRL_0;     // sd_ctrl_1: sd_card*/
/*	//int ctrl_x = SDMMC_CTRL_1;   // sd_ctrl_0: sd_card*/
	
	int sect_nums = (0x800000 / SDMMC_BLOCK_SIZE);  /* 8M*/
	int len = 0;
	
	UINT8 pBuf8[256*SDMMC_BLOCK_SIZE];  
	UINT8 pBuf8_2[256*SDMMC_BLOCK_SIZE]; 
	
	int i = 0, j = 0;	

	g_sd_blk_num = 1;
	
	int byteCnt = SDMMC_BLOCK_SIZE*get_sd_blk_num();

	test_sdmmc_init();
/*	byteCnt = SDMMC_BLOCK_SIZE*et_sd_blk_num();*/
	
	for (sect=from_sect; sect<=(to_sect); sect=sect+get_sd_blk_num())
	{
		for (j=0; j<byteCnt; j++)
		{
			pBuf8[j] = j + g_test_sd;
		}
		g_test_sd++;
			
		/* head*/
		*((UINT32*)(&pBuf8[0])) = 0xabcd1234;
		*((UINT32*)(&pBuf8[4])) = sect;
		
		/* tail*/
		
		*((UINT32*)(&pBuf8[byteCnt-8])) = sect;
		*((UINT32*)(&pBuf8[byteCnt-4])) = 0xcdef6789;
		

		  test_wr_sector(sect,(UINT8 *)(&pBuf8[0]));
		  /*FSdmmcPs_readWriteData(&g_SDMMC[0],sect,byteCnt,1);//wr*/
		  
		  
		  test_rd_sector(sect,(UINT8 *)(&pBuf8_2[0]));
		  /*FSdmmcPs_readWriteData(&g_SDMMC[0],sect,byteCnt,0); //rd*/
		  
		  
		for (i=0; i<byteCnt; i++)
		{
			if (pBuf8[i] != pBuf8_2[i])
			{
				printf("compare err: idx-%d, wr-0x%02X<->rd-0x%02X \n", i, pBuf8[i], pBuf8_2[i]);
				break;
			}
		}

		if (i == byteCnt)
		{
			printf(">>>wr&rd sector(%d) ok,no-err!>>> \n", sect);
		}
	
}
	
	return;
}


UINT8 g_pBuf8[2048*SDMMC_BLOCK_SIZE + 16];  
UINT8 g_pBuf8_2[2048*SDMMC_BLOCK_SIZE + 16]; 

void test_mmc_w_r_sects_bare1_2(int from_sect, int to_sect, int bulk_num)
{
	float speed = 0.0;
	UINT32 all_size = 0;
	int tick_start = 0, tick_end = 0;
	
	UINT8* pBuf8 = (UINT8 *)(&g_pBuf8[0]);	
	UINT8* pBuf8_2 = (UINT8 *)(&g_pBuf8_2[0]);; 
	
	semSync_mmc = semBCreate (SEM_Q_PRIORITY, SEM_FULL);

	int sect = 0;/*(to_sect - from_sect)/get_sd_blk_num();	*/
	
/*	int ctrl_x = SDMMC_CTRL_0;     // sd_ctrl_1: sd_card*/
/*	//int ctrl_x = SDMMC_CTRL_1;   // sd_ctrl_0: sd_card*/
	
	int sect_nums = (0x800000 / SDMMC_BLOCK_SIZE);  /* 8M*/
	int len = 0;	
	
	int i = 0, j = 0;	

	g_sd_blk_num = bulk_num;
	
	int byteCnt = SDMMC_BLOCK_SIZE*get_sd_blk_num();

	test_sdmmc_init();
	
	/*for (sect=from_sect; sect<=(to_sect); sect=sect+get_sd_blk_num())*/
	for (sect=from_sect; sect<=(to_sect); sect=sect+bulk_num)
	{
		/*
		for (j=0; j<byteCnt; j++)
		{
		pBuf8[j] = j + g_test_sd;
		}
		g_test_sd++;

		// head
		*((UINT32*)(&pBuf8[0])) = 0xabcd1234;
		*((UINT32*)(&pBuf8[4])) = sect;

		// tail

		*((UINT32*)(&pBuf8[byteCnt-8])) = sect;
		*((UINT32*)(&pBuf8[byteCnt-4])) = 0xcdef6789;
		*/
		
		/**/
		/* read origin data of sd*/
		/**/
		test_rd_sector(sect,(UINT8 *)(&pBuf8[0]));

		/**/
		/* write origin data of sd*/
		/**/
		test_wr_sector(sect,(UINT8 *)(&pBuf8[0]));
		/*FSdmmcPs_readWriteData(&g_SDMMC[0],sect,byteCnt,1);//wr*/

		/**/
		/* read data of sd, again*/
		/**/
		test_rd_sector(sect,(UINT8 *)(&pBuf8_2[0]));
		/*FSdmmcPs_readWriteData(&g_SDMMC[0],sect,byteCnt,0); //rd*/
		  
		for (i=0; i<byteCnt; i++)
		{
			if (pBuf8[i] != pBuf8_2[i])
			{
				printf("compare err: idx-%d, wr-0x%02X<->rd-0x%02X \n", i, pBuf8[i], pBuf8_2[i]);
				break;
			}
		}

		if (i == byteCnt)
		{
			printf(">>>wr&rd sector(%d) ok,no-err!>>> \n", sect);
		}
	
}
	
	return;
}
#endif


