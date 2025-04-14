/* test_i2c.c - test app & interface for i2c */

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

#include "../../bsp_drv/i2c/i2c_2_vx/vxI2c.h"
#include "test_i2c.h"

/*
extern prototype
*/
extern vxT_I2C * g_pI2c0;
extern vxT_I2C * g_pI2c1;


/*
defines 
*/
T_TEST_I2C gTestI2c = {0};


/*
functions 
*/
extern int vxInit_I2c(int ctrl_x);

#define __ctrl_reg_test__

/*
Ctrl Regs Read&Write Test
1. read ctrl_id or device_id
2. read read regs
3. write regs
*/
#if 1
void test_i2c_read_id(void)
{
	printf("i2c ctrl id: no \n");
	return;
}

void test_i2c_read_regs(void)
{
	printf("i2c_read_regs: to do \n");
	return;
}

void test_i2c_write_regs(void)
{
	printf("i2c_write_regs: to do \n");
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
void test_i2c_loop_ip(void)
{
	printf("no i2c_loop_ip! \n");
	return;
}

/* 
i2c SLCR loop: i2c0_tx  -> i2c1_rx
*/
void test_i2c_loop_slcr(void)
{
	int i = 0;
	vxT_I2C* pI2c0 = g_pI2c0;
	vxT_I2C* pI2c1 = g_pI2c1;
	
    int error = 0;

	int id = 0x55;	
	int len = 8;
	
	UINT8 tx_buf[16] = {0};
	UINT8 rx_buf[16] = {0};

	test_i2c_init();	
	
	return;
}


/* 
X_loop: phy_pin <-->phy_pin
*/
void test_i2c_loop_x(void)
{
	UINT32 tx_loop_data = 0;
	
	printf("i2c_loop_x: to do \n");

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
void test_i2c_read_speed(void)
{
	printf("i2c_read_speed: to do \n");
	return;
}

void test_i2c_write_speed(void)
{
	printf("i2c_write_speed: to do \n");
	return;
}

#endif

#if 1
void test_i2c_init(void)
{
	if (gTestI2c.init_flag == 0)
	{
		/*vxInit_I2c(0);*/
		/*vxInit_I2c(1);*/
		
		gTestI2c.init_flag = 1;
		gTestI2c.test_ver = 10;  /* 10->1.0; 11->1.1*/
	}

	return;
}

/*
1. three type test: regs, loop, speed
2. some type is not exist or to do
3. test app update by time and needs
*/
void test_i2c(void)
{
	printf("\n\n");
	printf("==test i2c build time: %s-%s == \n", __DATE__, __TIME__);
	printf("\n");

	/**/
	/* regs*/
	/**/
	test_i2c_read_id();
	test_i2c_read_regs();
	test_i2c_write_regs();
	printf("\n\n");

	/**/
	/* loop*/
	/**/
	test_i2c_loop_ip();
	test_i2c_loop_slcr();
	test_i2c_loop_x();
	printf("\n\n");

	/**/
	/* speed*/
	/**/
	test_i2c_read_speed();
	test_i2c_write_speed();
	printf("\n\n");

	printf("==test i2c ver: %d.%d == \n", \
		   (gTestI2c.test_ver/10), (gTestI2c.test_ver%10));
	printf("\n");	
	
	return;
}

#endif


