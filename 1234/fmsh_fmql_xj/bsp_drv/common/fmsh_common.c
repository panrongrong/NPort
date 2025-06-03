/******************************************************************************
*
* Copyright (C) 2018 - 2028 FMSH, Inc.  All rights reserved.
*
******************************************************************************/
/*****************************************************************************/
/**
* @file  fmsh_common.c
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

#include "fmsh_common.h"
#include "stdlib.h"
#include "fmsh_ps_parameters.h"

/************************** Constant Definitions *****************************/

/**************************** Type Definitions *******************************/

/***************** Macros (Inline Functions) Definitions *********************/

/************************** Variable Definitions *****************************/

/************************** Function Prototypes ******************************/

#ifndef FMSH_NASSERT
void onAssert__(char const *file, unsigned line)
{
	PRINTF("[ERR]: Error Report: %s, line: %u\r\n", file, line);
	abort();
}
#endif

/*bit_no 1-32
*set the 0 to bit_no bits 1
*/
u32 mask_generate(u32 bit_no)
{
    u32 i, temp = 0;
    for(i = 0; i< bit_no; i++)
    {
        temp |= (0x01 << i);
    }
    return temp;
}

void global_timer_enable()
{
 	FMSH_WriteReg(FPS_GTC_BASEADDR, 0, 1);
}

void global_timer_disable()
{
	FMSH_WriteReg(FPS_GTC_BASEADDR, 0, 0);
}

u64 get_current_time()
{
        u32 low;
        u32 high;
        u64 ret;

        /* Reading Global Timer Counter Register */
        do
        {
                high = FMSH_ReadReg(FPS_GTC_BASEADDR, 0xc);
                low = FMSH_ReadReg(FPS_GTC_BASEADDR, 0x8);
        } while(FMSH_ReadReg(FPS_GTC_BASEADDR, 0xc) != high);

	ret = (((u64) high) << 32U) | (u64) low;

        return ret;
}

void delay_ms(double time_ms)
{
        double counts;
        u64 tCur;
        u64 tEnd;

        counts = time_ms * 1000 * GTC_CLK_FREQ;

  	global_timer_enable();
  	tCur = get_current_time();

        while(1)
  	{
		tEnd = get_current_time();
		if((tEnd - tCur) > (u64)counts)
	  	break;
  	}
}

void delay_us(u32 time_us)
{
        u64 counts;
        u64 tCur;
        u64 tEnd;

        counts = time_us * GTC_CLK_FREQ;

        global_timer_enable();
        tCur = get_current_time();

        while(1)
        {
                tEnd = get_current_time();
                if((tEnd - tCur) > counts)
                break;
        }
}

void delay_1ms()
{
        double counts;
        u64 tCur;
        u64 tEnd;

        counts = 1000 * GTC_CLK_FREQ;

  	global_timer_enable();
  	tCur = get_current_time();

        while(1)
  	{
		tEnd = get_current_time();
		if((tEnd - tCur) > counts)
	  	break;
  	}
}

void delay_1us()
{
        double counts;
        u64 tCur;
        u64 tEnd;

        counts = GTC_CLK_FREQ;

        global_timer_enable();
        tCur = get_current_time();

        while(1)
        {
                tEnd = get_current_time();
                if((tEnd - tCur) > counts)
                break;
        }
}


#if 1

extern void fmqlSlcrWrite(UINT32 offset, UINT32 value);

int slcrRegs_Config(unsigned long * ps_config_init) 
{
	unsigned long *ptr = ps_config_init;
	unsigned long opcode; /* current instruction ..*/
	unsigned long args[16]; /* no opcode has so many args ...*/
	int numargs; /* number of arguments of this instruction*/
	int  j; /* general purpose index*/

	volatile unsigned long *addr;
	unsigned long  val,mask;
	volatile unsigned long *addrdst;
	unsigned long  maskdst,masktmp;

	int finish = -1 ; /* loop while this is negative !*/
	int i = 0; /* Timeout variable*/
	int srclow,dstlow;
	
	/* 	unlock slcr	*/
	fmqlSlcrWrite (FMQL_SR_UNLOCK_OFFSET, FMQL_SR_UNLOCK_ALL);  

	while (finish < 0) 
	{
		numargs = ptr[0] & 0xF;
		opcode = ptr[0] >> 4;

		for( j = 0 ; j < numargs ; j ++ )
		{  
			args[j] = ptr[j+1];
		}
		ptr += numargs + 1;

		switch ( opcode ) 
		{
		case OPCODE_EXIT:
			finish = PS_INIT_SUCCESS;
			break;
		
		case OPCODE_CLEAR:
			addr = (unsigned long*) args[0];
			*addr = 0;
			break;
			
		case OPCODE_WRITE:
			addr = (unsigned long*) args[0];
			val = args[1];
			*addr = val;
			break;
			
		case OPCODE_MASKWRITE:
			addr = (unsigned long*) args[0];
			mask = args[1];
			val = args[2];
			*addr = ( val & mask ) | ( *addr & ~mask);
			break;
			
		case OPCODE_MASKCOPY:
			addr = (unsigned long*) args[0];
			addrdst = (unsigned long*) args[1];
			mask = args[2];
			maskdst = args[3];
			masktmp = maskdst;
			val = (*addr & mask);
			srclow = 0;
			while (!(mask & 1)) 
			{ 
				mask >>= 1; 
				++srclow; 
			};
			dstlow = 0;
			while (!(masktmp & 1)) 
			{ 
				masktmp >>= 1; 
				++dstlow; 
			};
			val = (val >> srclow) << dstlow;
			*addrdst = ( val & maskdst ) | ( *addrdst & ~maskdst);
			break;

		case OPCODE_MASKPOLL:
			addr = (unsigned long*) args[0];
			mask = args[1];
			i = 0;
			while ((*addr & mask) != mask) 
			{
				if (i == PS_MASK_POLL_TIME) 
				{
					finish = PS_INIT_TIMEOUT;
					break;
				}
				i++;
			}
			break;

		case OPCODE_MASKDELAY:
			break;

		default:
			finish = PS_INIT_CORRUPT;
			break;
		}
	}

	/* lock slcr */
	fmqlSlcrWrite (FMQL_SR_LOCK_OFFSET, FMQL_SR_LOCK_ALL);  

	return finish;
}

#endif

