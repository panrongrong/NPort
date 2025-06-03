/* test_gmac.c - test app & interface for gmac */

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

/*#include "../../bsp_drv/gmac/gmac_2_vx/vxGmac.h"*/
#include "test_gmac.h"

/*
extern prototype
*/
/*extern vxT_GMAC *g_pGmac0;*/


/*
defines 
*/
T_TEST_GMAC gTestGmac = {0};


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
void test_gmac_read_id(void)
{
	printf("gmac ctrl id: no \n");
	return;
}

void test_gmac_read_regs(void)
{
	printf("gmac_read_regs: to do \n");
	return;
}

void test_gmac_write_regs(void)
{
	printf("gmac_write_regs: to do \n");
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
void test_gmac_loop_ip(void)
{
	printf("no gmac_loop_ip! \n");
	return;
}

/* 
gmac SLCR loop: gmac0_tx  -> gmac1_rx
*/
void test_gmac_loop_slcr(void)
{
	int i = 0;
	/*vxT_GMAC*pGmac0 = g_pGmac0;*/
	/*vxT_GMAC*pGmac1 = g_pGmac1;*/
	
    int error = 0;

	int id = 0x55;	
	int len = 8;
	
	UINT8 tx_buf[16] = {0};
	UINT8 rx_buf[16] = {0};

	test_gmac_init();	
	
	return;
}


/* 
X_loop: phy_pin <-->phy_pin
*/
void test_gmac_loop_x(void)
{
	UINT32 tx_loop_data = 0;
	
	/*gmacCtrl_X_Loop(tx_loop_data);*/
	printf("gmac_loop_x: to do \n");

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
void test_gmac_read_speed(void)
{
	printf("gmac_read_speed: to do \n");
	return;
}

void test_gmac_write_speed(void)
{
	printf("gmac_write_speed: to do \n");
	return;
}

#endif

#if 1
void test_gmac_init(void)
{
	if (gTestGmac.init_flag == 0)
	{
		/*vxInit_Gmac(0);*/
		/*vxInit_Gmac(1);*/
		
		gTestGmac.init_flag = 1;
		gTestGmac.test_ver = 10;  /* 10->1.0; 11->1.1*/
	}

	return;
}

/*
1. three type test: regs, loop, speed
2. some type is not exist or to do
3. test app update by time and needs
*/
void test_gmac(void)
{
	printf("\n\n");
	printf("==test gmac build time: %s-%s == \n", __DATE__, __TIME__);
	printf("\n");

	/**/
	/* regs*/
	/**/
	test_gmac_read_id();
	test_gmac_read_regs();
	test_gmac_write_regs();
	printf("\n\n");

	/**/
	/* loop*/
	/**/
	test_gmac_loop_ip();
	test_gmac_loop_slcr();
	test_gmac_loop_x();
	printf("\n\n");

	/**/
	/* speed*/
	/**/
	test_gmac_read_speed();
	test_gmac_write_speed();
	printf("\n\n");

	printf("==test gmac ver: %d.%d == \n", \
		   (gTestGmac.test_ver/10), (gTestGmac.test_ver%10));
	printf("\n");	
	
	return;
}

#endif


