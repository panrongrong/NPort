/* test_can.c - test app & interface for can */

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

#include "../../bsp_drv/can/can_2_vx/vxCan.h"
#include "test_can.h"

/*
extern prototype
*/
extern vxT_CAN * g_pCan0;


/*
defines 
*/
T_TEST_CAN gTestCan = {0};


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
void test_can_read_id(void)
{
	printf("can ctrl id: no \n");
	return;
}

void test_can_read_regs(void)
{
	printf("can_read_regs: to do \n");
	return;
}

void test_can_write_regs(void)
{
	printf("can_write_regs: to do \n");
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
void test_can_loop_ip(void)
{
	printf("no can_loop_ip! \n");
	return;
}

/* 
can SLCR loop: can0_tx  -> can1_rx
*/
void test_can_loop_slcr(void)
{
	int i = 0;
	vxT_CAN* pCan0 = g_pCan0;
	vxT_CAN* pCan1 = g_pCan1;
	
    int error = 0;

	int id = 0x55;	
	int len = 8;
	
	UINT8 tx_buf[16] = {0};
	UINT8 rx_buf[16] = {0};

	test_can_init();
	
    /* 
    enable SLCR loop 
	*/
    canSlcr_Set_CanLoop(TRUE);
	
	for (i=0; i<8; i++)
	{        
		tx_buf[i] = gTestCan.base_cnt + i;
	}
	gTestCan.base_cnt++;
	
	
#if 1 
	canCtrl_SndFrm_Poll(pCan0, id, tx_buf, len);	/* spi1-slave, spi0-master */
	printf("<slcr_loop>can_%d: snd \n", pCan0->pCanCtrl->ctrl_x);
		
	delay_ms(10);	
	printf("<------>\n");	
	
	canCtrl_RcvFrm_Poll(pCan1, rx_buf); 
	printf("<slcr_loop>can_%d: rcv \n\n", pCan1->pCanCtrl->ctrl_x);
#endif

    for (i=0; i<11; i++)
    {
       /*tx_buf[i] = canCtrl_Rd_CfgReg32((0x180 + 4 *i));*/
		tx_buf[i] = canCtrl_Rd_CfgReg32(g_pCan0, (CAN_REG_TXFIFO + 4 * i));
    }
	
	printf("\ncan_tx: \n");
    for(i = 0; i < 11; i++)
    {
       printf("%02X ", tx_buf[i]);
    }
	printf("\n");
	
	printf("can_rx: \n");
    for(i = 0; i < 11; i++)
    {
       printf("%02X ", rx_buf[i]);
    }
	printf("\n\n");
	
	return;
}


/* 
X_loop: phy_pin <-->phy_pin
*/
void test_can_loop_x(void)
{
	UINT32 tx_loop_data = 0;
	
	/*canCtrl_X_Loop(tx_loop_data);*/
	printf("can_loop_x: to do \n");

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
void test_can_read_speed(void)
{
	printf("can_read_speed: to do \n");
	return;
}

void test_can_write_speed(void)
{
	printf("can_write_speed: to do \n");
	return;
}

#endif

#if 1
void test_can_init(void)
{
	if (gTestCan.init_flag == 0)
	{
		vxInit_Can(0);
		vxInit_Can(1);
		
		gTestCan.init_flag = 1;
		gTestCan.test_ver = 10;  /* 10->1.0; 11->1.1*/
	}

	return;
}

/*
1. three type test: regs, loop, speed
2. some type is not exist or to do
3. test app update by time and needs
*/
void test_can(void)
{
	printf("\n\n");
	printf("==test can build time: %s-%s == \n", __DATE__, __TIME__);
	printf("\n");

	/**/
	/* regs*/
	/**/
	test_can_read_id();
	test_can_read_regs();
	test_can_write_regs();
	printf("\n\n");

	/**/
	/* loop*/
	/**/
	test_can_loop_ip();
	test_can_loop_slcr();
	test_can_loop_x();
	printf("\n\n");

	/**/
	/* speed*/
	/**/
	test_can_read_speed();
	test_can_write_speed();
	printf("\n\n");

	printf("==test can ver: %d.%d == \n", \
		   (gTestCan.test_ver/10), (gTestCan.test_ver%10));
	printf("\n");	
	
	return;
}

#endif


