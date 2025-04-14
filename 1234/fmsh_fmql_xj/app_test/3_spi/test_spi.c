/* test_spi.c - test app & interface for can */

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

#include "../../bsp_drv/spi/spi_2_vx/vxSpi.h"
#include "test_spi.h"

/*
extern prototype
*/
extern vxT_SPI * g_pSpi0;


/*
defines 
*/
T_TEST_SPI gTestSpi = {0};


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
void test_spi_read_id(void)
{
	printf("spi ctrl id: no \n");
	return;
}

void test_spi_read_regs(void)
{
	printf("spi_read_regs: to do \n");
	return;
}

void test_spi_write_regs(void)
{
	printf("spi_write_regs: to do \n");
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
void test_spi_loop_ip(void)
{
	vxT_SPI * pSpi = NULL;
	UINT32 tx_loop_data = 0;
	int spi_x = SPI_CTRL_0;
	
	test_spi_init();
	
	tx_loop_data = gTestSpi.base_cnt;
	gTestSpi.base_cnt++;	
	 
	spi_x = SPI_CTRL_0;
	pSpi = g_pSpi0;
	vxInit_Spi(spi_x, MODE_SPI_MASTER);

	spiCtrl_IP_Loop(pSpi, tx_loop_data);
	
	spi_x = SPI_CTRL_1;
	pSpi = g_pSpi1;	
	vxInit_Spi(spi_x, MODE_SPI_MASTER);
	
	spiCtrl_IP_Loop(pSpi, tx_loop_data);

	return;
}

/* 
can SLCR loop: can0_tx  -> can1_rx
*/
void test_spi_loop_slcr(void)
{
	UINT32 tx_loop_data = 0;
	
	test_spi_init();
	
	tx_loop_data = gTestSpi.base_cnt;
	gTestSpi.base_cnt++;	
	
	spiCtrl_SLCR_Loop(tx_loop_data);
	
	return;
}


/* 
X_loop: phy_pin <-->phy_pin
*/
void test_spi_loop_x(void)
{
	UINT32 tx_loop_data = 0;
	
	test_spi_init();
	
	tx_loop_data = gTestSpi.base_cnt;
	gTestSpi.base_cnt++;	
	
	spiCtrl_X_Loop(tx_loop_data);

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
void test_spi_read_speed(void)
{
	printf("spi_read_speed: to do \n");
	return;
}

void test_spi_write_speed(void)
{
	printf("spi_write_speed: to do \n");
	return;
}

#endif

#if 1
void test_spi_init(void)
{
	if (gTestSpi.init_flag == 0)
	{
		gTestSpi.init_flag = 1;
		gTestSpi.test_ver = 10;  /* 10->1.0; 11->1.1*/
	}

	return;
}

/*
1. three type test: regs, loop, speed
2. some type is not exist or to do
3. test app update by time and needs
*/
void test_spi(void)
{
	printf("\n\n");
	printf("==test spi build time: %s-%s == \n", __DATE__, __TIME__);
	printf("\n");

	/**/
	/* regs*/
	/**/
	test_spi_read_id();
	test_spi_read_regs();
	test_spi_write_regs();
	printf("\n\n");

	/**/
	/* loop*/
	/**/
	test_spi_loop_ip();
	test_spi_loop_slcr();
	test_spi_loop_x();
	printf("\n\n");

	/**/
	/* speed*/
	/**/
	test_spi_read_speed();
	test_spi_write_speed();
	printf("\n\n");

	printf("==test spi ver: %d.%d == \n", \
		   (gTestSpi.test_ver/10), (gTestSpi.test_ver%10));
	printf("\n");	
	
	return;
}

#endif


