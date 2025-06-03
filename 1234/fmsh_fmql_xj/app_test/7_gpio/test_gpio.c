/* test_gpio.c - test app & interface for gpio */

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

#include "../../bsp_drv/gpio/gpio_2_vx/vxGpio.h"
#include "test_gpio.h"

/*
extern prototype
*/
extern vxT_GPIO * g_pGpio0;


/*
defines 
*/
T_TEST_GPIO gTestGpio = {0};


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
void test_gpio_read_id(void)
{
	printf("gpio ctrl id: no \n");
	return;
}

void test_gpio_read_regs(void)
{
	printf("gpio_read_regs: to do \n");
	return;
}

void test_gpio_write_regs(void)
{
	printf("gpio_write_regs: to do \n");
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
void test_gpio_loop_ip(void)
{
	printf("no gpio_loop_ip! \n");
	return;
}

/* 
gpio SLCR loop: gpio0_tx  -> gpio1_rx
*/
void test_gpio_loop_slcr(void)
{
	int i = 0;
	vxT_GPIO* pGpio0 = g_pGpio0;
	vxT_GPIO* pGpio1 = g_pGpio1;
	
    int error = 0;

	int id = 0x55;	
	int len = 8;
	
	UINT8 tx_buf[16] = {0};
	UINT8 rx_buf[16] = {0};

	test_gpio_init();	
	
	return;
}


/* 
X_loop: phy_pin <-->phy_pin
*/
void test_gpio_loop_x(void)
{
	UINT32 tx_loop_data = 0;
	
	/*gpioCtrl_X_Loop(tx_loop_data);*/
	printf("gpio_loop_x: to do \n");

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
void test_gpio_read_speed(void)
{
	printf("gpio_read_speed: to do \n");
	return;
}

void test_gpio_write_speed(void)
{
	printf("gpio_write_speed: to do \n");
	return;
}

#endif

#if 1
void test_gpio_init(void)
{
	if (gTestGpio.init_flag == 0)
	{
		vxInit_Gpio(0);
		vxInit_Gpio(1);
		
		gTestGpio.init_flag = 1;
		gTestGpio.test_ver = 10;  /* 10->1.0; 11->1.1*/
	}

	return;
}

/*
1. three type test: regs, loop, speed
2. some type is not exist or to do
3. test app update by time and needs
*/
void test_gpio(void)
{
	printf("\n\n");
	printf("==test gpio build time: %s-%s == \n", __DATE__, __TIME__);
	printf("\n");

	/**/
	/* regs*/
	/**/
	test_gpio_read_id();
	test_gpio_read_regs();
	test_gpio_write_regs();
	printf("\n\n");

	/**/
	/* loop*/
	/**/
	test_gpio_loop_ip();
	test_gpio_loop_slcr();
	test_gpio_loop_x();
	printf("\n\n");

	/**/
	/* speed*/
	/**/
	test_gpio_read_speed();
	test_gpio_write_speed();
	printf("\n\n");

	printf("==test gpio ver: %d.%d == \n", \
		   (gTestGpio.test_ver/10), (gTestGpio.test_ver%10));
	printf("\n");	
	
	return;
}

#endif


