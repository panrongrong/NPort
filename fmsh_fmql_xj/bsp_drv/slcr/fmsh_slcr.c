/******************************************************************************
*
* Copyright (C) 2018 - 2028 FMSH, Inc.  All rights reserved.
*
******************************************************************************/
/*****************************************************************************/
/**
* @file  fmsh_slcr.c
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
* 0.01   lsq  11/23/2018  First Release
*</pre>
******************************************************************************/

/***************************** Include Files *********************************/
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "fmsh_slcr.h"
#include "../common/fmsh_ps_parameters.h"

/************************** Constant Definitions *****************************/

/**************************** Type Definitions *******************************/

/***************** Macros (Inline Functions) Definitions *********************/

/************************** Variable Definitions *****************************/

/************************** Function Prototypes ******************************/
/****************************************************************************/
/**
*
*  This function set a bit of the register to 0
*
* @param
*  baseAddr  -- the base address of the register which is to be modified
*  offSet    -- the base address of the register which is to be modified
*  bit_num   -- this bit of the register is to be modified
*
* @return
*  Null
*
* @note
*  Null
*
****************************************************************************/
void FSlcrPs_setBitTo0(u32 baseAddr, u32 offSet,u32 bit_num)
{
    u32 value = 0;
    /* First get the current value of the register */
    value = FMSH_ReadReg(baseAddr, offSet);
    /* Then write the given bit of data as 0 */
    value &= ~(1 << bit_num);
    /* Finally, write the modified data to the register */
    FMSH_WriteReg(baseAddr, offSet, value);
}

/****************************************************************************/
/**
*
*  This function set a bit of the register to 1
*
* @param
*  baseAddr  -- the base address of the register which is to be modified
*  offSet    -- the base address of the register which is to be modified
*  bit_num   -- this bit of the register is to be modified
*
* @return
*  Null
*
* @note
*  Null
*
****************************************************************************/
void FSlcrPs_setBitTo1(u32 baseAddr, u32 offSet,u32 bit_num)
{
    u32 value = 0;
    /* First get the current value of the register */
    value = FMSH_ReadReg(baseAddr, offSet);
    /* Then write the given bit of data as 1 */
    value |= (1 << bit_num);
    /* Finally, write the modified data to the register */
    FMSH_WriteReg(baseAddr, offSet, value);
}

/****************************************************************************/
/**
*
*  This function write the lock key 0xDF0D767B to protect the slcr registers.
*
* @param
*  Null
*
* @return
*  Null
*
* @note
*  Null
*
****************************************************************************/
void FSlcrPs_lock(void)
{
    FMSH_WriteReg(FPS_SLCR_BASEADDR,SLCR_LOCK,0xDF0D767B);   /* SLCR LOCK */
}

/****************************************************************************/
/**
*
*  This function write the unlock key 0xDF0D767B to enable writes to the
*  slcr registers.
*
* @param
*  Null
*
* @return
*  Null
*
* @note
*  Null
*
****************************************************************************/
void FSlcrPs_unlock(void)
{
    FMSH_WriteReg(FPS_SLCR_BASEADDR,SLCR_UNLOCK,0xDF0D767B);   /* SLCR UNLOCK */
}

/****************************************************************************/
/**
*
*  This function enable/unable the pss soft_rst.
*
* @param
*  soft_rst_en -- 0:unable soft_rst;1:enable soft_rst
*
* @return
*  Null
*
* @note
*  Null
*
****************************************************************************/
void FSlcrPs_softRst(u32 soft_rst_en)
{
    FSlcrPs_unlock();
	FMSH_WriteReg(FPS_SLCR_BASEADDR,PSS_RST_CTRL,soft_rst_en);   /* PS soft_rst */
    FSlcrPs_lock();
}


/****************************************************************************/
/**
*
*  This function set reset of the given IP feature.
*
* @param
*  rst_id   -- the address of reset_ctrl register
*  rst_mode -- the reset mode of the ip feature
*
* @return
*  Null
*
* @note
*  Null
*
****************************************************************************/
void FSlcrPs_ipSetRst(u32 rst_id, u32 rst_mode)
{
    FSlcrPs_unlock();
    FSlcrPs_setBitTo1(FPS_SLCR_BASEADDR,rst_id,rst_mode);
    FSlcrPs_lock();
}

/****************************************************************************/
/**
*
*  This function release reset of the given IP feature.
*
* @param
*  rst_id   -- the address of reset_ctrl register
*  rst_mode -- the reset mode of the ip feature
*
* @return
*  Null
*
* @note
*  Null
*
****************************************************************************/
void FSlcrPs_ipReleaseRst(u32 rst_id, u32 rst_mode)
{
    FSlcrPs_unlock();
    FSlcrPs_setBitTo0(FPS_SLCR_BASEADDR,rst_id,rst_mode);
    FSlcrPs_lock();
}

/****************************************************************************/
/**
*
*  This function read the data of the given slcr address and printf it out.
*
* @param
*  Addr  --  the exist address of slcr module
*
* @return
*  0 -- read successful
*
* @note
*  Null
*
****************************************************************************/
u32 FSlcrPS_regRead(u32 addr)
{
	u32 read_data;
	read_data = FMSH_ReadReg(FPS_SLCR_BASEADDR,addr);
    TRACE_OUT(DEBUG_OUT,"Addr = 0x%x, Read_Data = 0x%x!\n",addr,read_data);

	return 0;
}

/****************************************************************************/
/**
*
*  This function read all the data of the slcr address and print them out.
*
* @param
*  Null
*
* @return
*  Null
*
* @note
*  Null
*
****************************************************************************/
void FSlcrPS_rsvRegPrint(void)
{
	int i;
    FSlcrPs_unlock();   /* SLCR UNLOCK */
	for(i=0;i<600;i++)
	{
		FSlcrPS_regRead(0x100+4*i);
	}
}

/****************************************************************************/
/**
*
*  This function loop I2C0's outputs to I2C1's inputs,and I2C1's outputs to
*  I2C0's inputs
*
* @param
*  loop_en -- 0:connect I2C inputs according to MIO mapping;1:set the loop
*
* @return
*  Null
*
* @note
*  Null
*
****************************************************************************/
void FSlcrPS_setI2cLoop(u32 loop_en)
{
    FSlcrPs_unlock();   /* SLCR UNLOCK */
	if (loop_en == 0)
	{
	    FSlcrPs_setBitTo0(FPS_SLCR_BASEADDR, SLCR_MIO_LOOPBACK,3);
	}
	else if (loop_en == 1)
	{
	    FSlcrPs_setBitTo1(FPS_SLCR_BASEADDR, SLCR_MIO_LOOPBACK,3);
    }
	FSlcrPs_lock();   /* SLCR LOCK */
}

/****************************************************************************/
/**
*
*  This function loop CAN0's Tx to CAN1's Rx,and CAN1's Tx to CAN0's Rx.
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
void FSlcrPS_setCanLoop(u32 loop_en)
{
    FSlcrPs_unlock();   /* SLCR UNLOCK */
	if (loop_en == 0)
	{
	    FSlcrPs_setBitTo0(FPS_SLCR_BASEADDR, SLCR_MIO_LOOPBACK,2);
	}
	else if (loop_en == 1)
	{
	    FSlcrPs_setBitTo1(FPS_SLCR_BASEADDR, SLCR_MIO_LOOPBACK,2);
    }
	FSlcrPs_lock();   /* SLCR LOCK */
}

/****************************************************************************/
/**
*
*  This function loop UART0's Tx to UART1's Rx,and UART1's Tx to UART0's Rx.
*
* @param
*  loop_en -- 0:connect UART inputs according to MIO mapping;1:set the loop
*
* @return
*  Null
*
* @note
*  Null
*
****************************************************************************/
void FSlcrPS_setUartLoop(u32 loop_en)
{
    FSlcrPs_unlock();   /* SLCR UNLOCK */
	if (loop_en == 0)
	{
	    FSlcrPs_setBitTo0(FPS_SLCR_BASEADDR, SLCR_MIO_LOOPBACK,1);
	}
	else if (loop_en == 1)
	{
	    FSlcrPs_setBitTo1(FPS_SLCR_BASEADDR, SLCR_MIO_LOOPBACK,1);
    }
	FSlcrPs_lock();   /* SLCR LOCK */
}

/****************************************************************************/
/**
*
*  This function loop SPI0's outputs to SPI1's inputs,and SPI1's outputs to
*  SPI0's inputs
*
* @param
*  loop_en -- 0:connect SPI inputs according to MIO mapping;1:set the loop
*
* @return
*  Null
*
* @note
*  Null
*
****************************************************************************/
void FSlcrPS_setSpiLoop(u32 loop_en)
{
    FSlcrPs_unlock();   /* SLCR UNLOCK */
	if (loop_en == 0)
	{
	    FSlcrPs_setBitTo0(FPS_SLCR_BASEADDR, SLCR_MIO_LOOPBACK,0);
	}
	else if (loop_en == 1)
	{
	    FSlcrPs_setBitTo1(FPS_SLCR_BASEADDR, SLCR_MIO_LOOPBACK,0);
    }
	FSlcrPs_lock();   /* SLCR LOCK */
}


#if 0

#define TASK_HPRIO        (101)                                           
#define TASK_LPRIO        (102)                                        
#define DUM_TASK_PRIO     (103)  

#define ITERCOUNT  1000

static UINT32 start_time = 0;
static UINT32 end_time = 0;
static UINT32 start_suspend_time1 = 0;
static UINT32 start_suspend_time2 = 0;
static UINT32 suspend_resume_time = 0;
static UINT32 cxt_sw_time2[ITERCOUNT];

static UINT32 cxt_start_time[ITERCOUNT];
static UINT32 cxt_end_time[ITERCOUNT];

static int task_id[3];
static int task_time_id;

/* #define sys_timestamp_freq_ex sysTimestampFreq */
/* #define sys_timestamp_ex sysTimestamp */

UINT32 sys_timestamp_freq_ex()
{
	return sysTimestampFreq();
}

UINT32 sys_timestamp_ex()
{
	return sysTimestamp();
}

void data_statistic(UINT32 *array, UINT32 num, UINT32 *max_v, UINT32 *min_v, double *avr_v)
{
	int i;
	UINT32 max, min; 
	UINT64 sum;		
	double avr;
	
	/* 求取最大数、最小数、平均数、和 */
	max = array[0];
	min = array[0]; 	
	avr = 0;
	sum = array[0];	
	
		
	for(i = 1; i < num; i++) 
	{
		if(array[i] > max)
			max = array[i];
		
		if(array[i] < min)
			min = array[i];
		
		sum += (UINT64)array[i];		
	}
 
	avr = (double)(sum * 1.0)  / (num);
	
	*max_v = max;
	*min_v = min;
	*avr_v = avr;

	for (i=0; i<num; i++)
	{
		printf("%d=>%d(%d) \t", cxt_start_time[i], cxt_end_time[i], (cxt_end_time[i]-cxt_start_time[i]));
		if (((i+1) % 5) == 0)
		{
			printf("\n");			
		}
	}
	printf("\n");	
	
	for (i=0; i<num; i++)
	{
		printf("%d: %d \t", i, array[i]);
		if (((i+1) % 10) == 0)
		{
			printf("\n");			
		}
	}
	printf("\n");			

	return;
}

static void high_task()
{
	int i = 0;
	UINT64 t1, t2;	
	
	for(i = 0; i < ITERCOUNT; i++)
	{
#if 0
		start_time = sys_timestamp_ex();
		taskSuspend(0);  /* 挂起自己, 切换到low_task */
		end_time = sys_timestamp_ex();
		
		/* 有效性检测 */
		 if ((suspend_resume_time < 0) || (start_time > end_time) || (end_time - start_time) < suspend_resume_time){	
			i--; /* 测量无效，实际上有可能是错误 */
		} else {
			cxt_sw_time2[i] = end_time - start_time - suspend_resume_time;  /* 2次上下文切换时间 */	 
		} 
#else
		t1 = tickGet();
		start_time = sys_timestamp_ex();
		/*printf("high suspend1  \n"); */
		taskSuspend(0);  /* 挂起自己, 切换到low_task */
		/*printf("high suspend2 \n\n");*/
		end_time = sys_timestamp_ex();
		t2 = tickGet();
		
		/* 有效性检测 */
		if ((t1 != t2) /* || (start_time > end_time) || (end_time - start_time) < suspend_resume_time*/)
		{	
			i--;       /* 测量无效，实际上有可能是错误 */
			printf("t1:%d, t2:%d \n", t1, t2);
		} 
		else 
		{
			/*cxt_sw_time2[i] = end_time - start_time - suspend_resume_time;  */ /* 2次上下文切换时间 */
			cxt_sw_time2[i] = end_time - start_time;  /* 2次上下文切换时间 */
			
			cxt_start_time[i] = start_time;
			cxt_end_time[i] = end_time;
		} 
#endif
	}
	
	taskResume(task_time_id);
	printf("high Resume dum_task \n");
}


static void low_task()
{
    while(1)
    {    	
    	taskResume(task_id[0]);
		/* printf("--low Resume high-> \n"); */
    }
}
   
void dum_task()
{
	while(1);
}


void test_context_switch_time()
{
	UINT32 max, min;		
	double avr;
	UINT32 freq;
	
	bzero(cxt_sw_time2, ITERCOUNT);
	

	task_time_id = taskIdSelf();

	#if 0
	task_id[2] = taskSpawn ("dum_task", DUM_TASK_PRIO, 0, 4096, dum_task, 0,
               0, 0, 0, 0, 0, 0, 0, 0, 0);	
	
	taskSuspend(task_id[2]);
	taskResume(task_id[2]);
	
	/* sys_timestamp_ex(); */
	/* sys_timestamp_ex(); */
	
	start_suspend_time1 = sys_timestamp_ex();
	taskSuspend(task_id[2]);
	taskResume(task_id[2]);
	start_suspend_time2 = sys_timestamp_ex();	
	#endif
	
	suspend_resume_time = start_suspend_time2 - start_suspend_time1;  /* 接口的时间消耗 */
	printf("suspend_resume_time: %d \n", suspend_resume_time);
	
	
	task_id[0] = taskSpawn ("high_task", TASK_HPRIO, 0, 4096, high_task, 0,
               0, 0, 0, 0, 0, 0, 0, 0, 0);	
	
	task_id[1] = taskSpawn ("low_task", TASK_LPRIO, 0, 4096, low_task, 0,
               0, 0, 0, 0, 0, 0, 0, 0, 0);	
	
	taskSuspend(0);  /* 自己挂起后，切换到high_task, 等待high_task将其恢复 */
	
	data_statistic(&cxt_sw_time2[0], ITERCOUNT, &max, &min, &avr);
	
	freq = sys_timestamp_freq_ex();
	
	printf("freq = %u \n",freq);
	
	printf("context switch test %d times, max time: %10.6f us, min time: %10.6f us, avr time: %10.6f us\n",
			ITERCOUNT,
			(max * 1000000.0 / freq / 2.0), 
			(min * 1000000.0 / freq / 2.0), 
			(avr * 1000000.0 / freq / 2.0));   /* 2次切换，所以除以2 */

	/* taskDelete(task_id[2]); */
	taskDelete(task_id[1]);
	taskDelete(task_id[0]);

	return;
}

#endif

