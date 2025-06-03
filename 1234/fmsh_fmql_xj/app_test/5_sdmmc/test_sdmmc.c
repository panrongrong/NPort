/* test_sdmmc.c - test app & interface for sdmmc */

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

#include "../../bsp_drv/sdmmc/sdmmc_2_vx/vxSdmmc.h"
#include "test_sdmmc.h"

/*
extern prototype
*/
extern vxT_SDMMC * g_pSdmmc0;


/*
defines 
*/
T_TEST_SDMMC gTestSdmmc = {0};


/*
functions 
*/
void test_sdmmc_init(void);


#define __ctrl_reg_test__

/*
Ctrl Regs Read&Write Test
1. read ctrl_id or device_id
2. read read regs
3. write regs
*/
#if 1
void test_sdmmc_read_id(void)
{
	printf("sdmmc ctrl id: no \n");
	return;
}

void test_sdmmc_read_regs(void)
{
	printf("sdmmc_read_regs: to do \n");
	return;
}

void test_sdmmc_write_regs(void)
{
	printf("sdmmc_write_regs: to do \n");
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
void test_sdmmc_loop_ip(void)
{
	printf("no sdmmc_loop_ip! \n");
	return;
}

/* 
sdmmc SLCR loop: sdmmc0_tx  -> sdmmc1_rx
*/
void test_sdmmc_loop_slcr(void)
{
	int i = 0;
	vxT_SDMMC* pSdmmc0 = g_pSdmmc0;
	/*vxT_SDMMC*pSdmmc1 = g_pSdmmc1;*/
	
    int error = 0;

	int id = 0x55;	
	int len = 8;
	
	UINT8 tx_buf[16] = {0};
	UINT8 rx_buf[16] = {0};

	test_sdmmc_init();	
	
	return;
}


/* 
X_loop: phy_pin <-->phy_pin
*/
void test_sdmmc_loop_x(void)
{
	UINT32 tx_loop_data = 0;
	
	/*sdmmcCtrl_X_Loop(tx_loop_data);*/
	printf("sdmmc_loop_x: to do \n");

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
void test_sdmmc_read_speed(void)
{
	printf("sdmmc_read_speed: to do \n");
	return;
}

void test_sdmmc_write_speed(void)
{
	printf("sdmmc_write_speed: to do \n");
	return;
}

#endif

#if 1
void test_sdmmc_init(void)
{
	if (gTestSdmmc.init_flag == 0)
	{
		/*vxInit_Sdmmc(0);*/
		/*vxInit_Sdmmc(1);*/
		
		gTestSdmmc.init_flag = 1;
		gTestSdmmc.test_ver = 10;  /* 10->1.0; 11->1.1*/
	}

	return;
}

/*
1. three type test: regs, loop, speed
2. some type is not exist or to do
3. test app update by time and needs
*/
void test_sdmmc(void)
{
	printf("\n\n");
	printf("==test sdmmc build time: %s-%s == \n", __DATE__, __TIME__);
	printf("\n");

	/**/
	/* regs*/
	/**/
	test_sdmmc_read_id();
	test_sdmmc_read_regs();
	test_sdmmc_write_regs();
	printf("\n\n");

	/**/
	/* loop*/
	/**/
	test_sdmmc_loop_ip();
	test_sdmmc_loop_slcr();
	test_sdmmc_loop_x();
	printf("\n\n");

	/**/
	/* speed*/
	/**/
	test_sdmmc_read_speed();
	test_sdmmc_write_speed();
	printf("\n\n");

	printf("==test sdmmc ver: %d.%d == \n", \
		   (gTestSdmmc.test_ver/10), (gTestSdmmc.test_ver%10));
	printf("\n");	
	
	return;
}

#endif


