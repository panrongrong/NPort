/* test_qspi.c - test app & interface for qspi */

/*
 * Copyright (c) 2020 Wind River Systems, Inc.
 *
 * The right to copy, distribute, modify or otherwise make use
 * of this software may be licensed only pursuant to the terms
 * of an applicable Wind River license agreement.
 */
 
/*
modification history
--------------------
01a, 17Oct20, jc  written.
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

#include "../../bsp_drv/qspi/qspi_2_vx/nor/vxQspi_Norflash.h"
#include "test_qspi.h"

/*
extern prototype
*/
extern vxT_QSPI * g_pQspi0;


/*
defines 
*/
T_TEST_QSPI gTestQspi = {0};


/*
functions 
*/

#define __ctrl_reg_test__

/*
Ctrl Regs Read&Write Test
1. read ctrl_id or device_id
2. read read regs
3. write regs
*/
#if 1
void test_qspi_read_id(void)
{
	UINT32 id[2] = {0};
	
	qspiFlash_ReadId_2(g_pQspi0, id);
	
	printf("qspi flash id: 0x%08X_%08X \n", id[0], id[1]);	
	return;
}

void test_qspi_read_regs(void)
{
	printf("qspi_read_regs: to do \n");
	return;
}

void test_qspi_write_regs(void)
{
	printf("qspi_write_regs: to do \n");
	return;
}
#endif


#define __loop_test__

/*
Loop Test:
1. three kind loop: ip, slcr, phy_pin
2. maybe some loop not exist
3. loop excute once at one cmd 
*/
#if 1

/* 
IP loop: inside ip
*/
void test_qspi_loop_ip(void)
{
	printf("no qspi_loop_ip! \n");
	return;
}

/* 
qspi SLCR loop: qspi0_tx  -> qspi1_rx
*/
void test_qspi_loop_slcr(void)
{
	printf("no qspi_loop_slcr! \n");
	return;
}


/* 
X_loop: phy_pin <-->phy_pin
*/
void test_qspi_loop_x(void)
{
	printf("no qspi_loop_x! \n");
	return;
}

#endif


#define __speed_test__

/*
Speed Test:
1. read speed test
2. write speed test
3. speed val: bsp
*/

#if 1

UINT8 rcvBuf[1024] = {0};

void test_qspi_read_speed(void)
{
	vxT_QSPI * pQspi = g_pQspi0;
	
	vxT_QSPI_CTRL * pQspiCtrl = pQspi->pQspiCtrl;
	vxT_QSPI_FLASH * pQspiFlash = pQspi->pQspiFlash;
	
	UINT8 * pBuf8 = (UINT8 *)(&rcvBuf[0]);
	
	int j = 0, byteCnt = 0;
	int offset = 0;
	int sect_cnt = 20;

	int sect_idx = 0;
	float speed = 0.0;
	UINT32 all_size = 0;
	int tick_start = 0, tick_end = 0;
	
	test_qspi_init();	
	
	tick_start = tickGet();

	for (sect_idx=0; sect_idx<sect_cnt; sect_idx++)
	{
		byteCnt = pQspiFlash->sectSize;
		offset = sect_idx * pQspiFlash->sectSize;
		
#if  QSPI_DIRECT_MODE
		qspiFlash_RcvBytes_Direct(pQspi, offset, byteCnt, pBuf8);
#else
		qspiFlash_Read_Sect(pQspi, sect_idx, pBuf8);
#endif
	}

	tick_end = tickGet();
	all_size = sect_cnt * pQspiFlash->sectSize;
		
	speed = (float)(all_size * 1.0) / ((float)(tick_end - tick_start)*1.0 / sysClkRateGet());

	printf("test qspi norflash read speed: %.2f bytes/s \n", speed);
	
	return;
}


UINT8 sndBuf[1024] = {0};

void test_qspi_write_speed(void)
{
	vxT_QSPI * pQspi = g_pQspi0;
	
	vxT_QSPI_CTRL * pQspiCtrl = pQspi->pQspiCtrl;
	vxT_QSPI_FLASH * pQspiFlash = pQspi->pQspiFlash;	

	UINT8 * pBuf8 = (UINT8 *)(&sndBuf[0]);
	
	int i = 0, j = 0, byteCnt = 0;
	int offset = 0;
	int sect_cnt = 20;

	int sect_idx = 0;
	float speed = 0.0;
	UINT32 all_size = 0;
	int tick_start = 0, tick_end = 0;
	
	test_qspi_init();	
	
	byteCnt = pQspiFlash->sectSize;
	for (i=0; i<byteCnt; i++)
	{
		pBuf8[i] = gTestQspi.base_cnt + i;
	}
	gTestQspi.base_cnt++;


	/*/////////////////////////////////////////////////////////////*/
	/**/
	tick_start = tickGet();	
	
	for (sect_idx=0; sect_idx<sect_cnt; sect_idx++)
	{
		qspiFlash_Erase_Sect(pQspi, sect_idx);
	}
	
	tick_end = tickGet();
	all_size = sect_cnt * pQspiFlash->sectSize;
		
	speed = (float)(all_size * 1.0) / ((float)(tick_end - tick_start)*1.0 / sysClkRateGet());

	printf("test qspi norflash erase speed: %.2f bytes/s \n", speed);
	/**/
	/*/////////////////////////////////////////////////////////////*/


	/*/////////////////////////////////////////////////////////////*/
	/**/
	tick_start = tickGet();		

	for (sect_idx=0; sect_idx<sect_cnt; sect_idx++)
	{		
		byteCnt = pQspiFlash->sectSize;
		offset = sect_idx * pQspiFlash->sectSize;
		
#if  QSPI_DIRECT_MODE
		qspiFlash_SndBytes_Direct(pQspi, offset, byteCnt, pBuf8);
#else
		qspiFlash_Write_Sect(pQspi, sect_idx, pBuf8);
#endif
	}
	
	tick_end = tickGet();
	all_size = sect_cnt * pQspiFlash->sectSize;
		
	speed = (float)(all_size * 1.0) / ((float)(tick_end - tick_start)*1.0 / sysClkRateGet());

	printf("test qspi norflash wrtie speed: %.2f bytes/s \n", speed);
	/**/
	/*/////////////////////////////////////////////////////////////*/

	return;
}

#endif


#if 1
void test_qspi_init(void)
{
	if (gTestQspi.init_flag == 0)
	{
		vxInit_Qspi(0);
		/*vxInit_Qspi(1);*/
		
		gTestQspi.init_flag = 1;
		gTestQspi.test_ver = 10;  /* 10->1.0; 11->1.1*/
	}

	return;
}

/*
1. three type test: regs, loop, speed
2. some type is not exist or to do
3. test app update by time and needs
*/
void test_qspi(void)
{
	printf("\n\n");
	printf("==test qspi build time: %s-%s == \n", __DATE__, __TIME__);
	printf("\n");

	/**/
	/* regs*/
	/**/
	test_qspi_read_id();
	test_qspi_read_regs();
	test_qspi_write_regs();
	printf("\n\n");

	/**/
	/* loop*/
	/**/
	test_qspi_loop_ip();
	test_qspi_loop_slcr();
	test_qspi_loop_x();
	printf("\n\n");

	/**/
	/* speed*/
	/**/
	test_qspi_read_speed();
	test_qspi_write_speed();
	printf("\n\n");

	printf("==test qspi ver: %d.%d == \n", \
		   (gTestQspi.test_ver/10), (gTestQspi.test_ver%10));
	printf("\n");	
	
	return;
}

#endif


