/* test_all.c - test app & interface for all devices */

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
01a, 04Sep20, jc  written.
*/

#include <vxWorks.h>
#include <stdio.h>
#include <semLib.h>
#include <sysLib.h>
#include <taskLib.h>
#include <vxBusLib.h>
#include <cacheLib.h>
#include <string.h>
#include <vxbTimerLib.h>


#include "./1_can/test_can.h"

#include "./2_i2c/test_i2c.h"

#include "./3_spi/test_spi.h"

#include "./4_qspi/test_qspi.h"

#include "./5_sdmmc/test_sdmmc.h"

#include "./6_nfc/test_nfc.h"

#include "./7_gpio/test_gpio.h"

#include "./8_gmac/test_gmac.h"


/*
the entry of all test 
*/
void test_dev(int dev_idx)
{
	switch (dev_idx)
	{
	case 1:  /* 1_can*/
		printf("\n--test_dev %d (can)--\n\n", dev_idx);
		/*test_can();*/
		break;
	
	case 2:  /* 2_i2c*/
		printf("\n--test_dev %d (i2c)--\n\n", dev_idx);
		/*test_i2c();*/
		break;
		
	case 3:  /* 3_spi*/
		printf("\n--test_dev %d (spi)--\n\n", dev_idx);
		/*test_spi();*/
		break;
	
	case 4:  /* 4_qspi*/
		printf("\n--test_dev %d (qspi)--\n\n", dev_idx);
		/*test_qspi();*/
		break;
	
	case 5:  /* 5_sdmmc*/
		printf("\n--test_dev %d (sdmmc)--\n\n", dev_idx);
		/*test_sdmmc();*/
		break;
	
	case 6:  /* 6_nfc*/
		printf("\n--test_dev %d (nfc)--\n\n", dev_idx);
		/*test_nfc();*/
		break;
	
	case 7:  /* 7_gpio*/
		printf("\n--test_dev %d (gpio)--\n\n", dev_idx);
		/*test_gpio();*/
		break;
	
	case 8:  /* 8_gmac*/
		printf("\n--test_dev %d (gmac)--\n\n", dev_idx);
		/*test_gmac();*/
		break;
	
	case 0:	/* help*/
		printf("\n");
		printf("shell cmd: test_dev x \n"); 
		printf("help	 : test_dev 0 \n"); 
		printf("test_dev 1 : can   \n"); 
		printf("test_dev 2 : i2c   \n"); 
		printf("test_dev 3 : spi   \n"); 
		printf("test_dev 4 : qspi  \n"); 
		printf("test_dev 5 : sdmmc \n"); 
		printf("test_dev 6 : nfc   \n"); 
		printf("test_dev 7 : gpio  \n"); 
		printf("test_dev 8 : gmac  \n"); 
		printf("\n\n");
		break;
		
	default:
		printf("\n");
		printf("test dev_idx(%d) error, pls check! \n", dev_idx);
		printf("help	 : test_dev 0 \n"); 
		printf("\n\n");
		break;
	}

	return;
}

/*
auto test all dev
*/
void test_all(void)
{
	test_dev(1);  /* 1_can*/
	test_dev(2);  /* 2_i2c*/
	
	test_dev(3);  /* 3_spi*/
	test_dev(4);  /* 4_qspi-norflash*/
	
	test_dev(5);  /* 5_sdmmc*/
	test_dev(6);  /* 6_nfc*/
	
	test_dev(7);  /* 7_gpio	*/
	test_dev(8);  /* 8_gmac*/

	return;
}


