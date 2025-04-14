/* test_qspi.h - test head file for qspi */

/*
 * Copyright (c) 2013 Wind River Systems, Inc.
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

#ifndef __INC_TEST_QSPI_H__
#define __INC_TEST_QSPI_H__



#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


typedef struct _test_qspi_ 
{
	int init_flag;

	int base_cnt;
	
	int test_ver;
	
} T_TEST_QSPI;


/*
local protoypte
*/
void test_qspi_init(void);
void test_qspi(void);

/*
extern protoypte
*/
extern void test_qspi(void);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __INC_TEST_QSPI_H__ */


