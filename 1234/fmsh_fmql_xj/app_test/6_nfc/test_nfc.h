/* test_nfc.h - test head file for nfc */

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

#ifndef __INC_TEST_NFC_H__
#define __INC_TEST_NFC_H__



#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


typedef struct _test_nfc_ 
{
	int init_flag;

	int base_cnt;
	
	int test_ver;
	
} T_TEST_NFC;


/*
local protoypte
*/
void test_nfc_init(void);
void test_nfc(void);

/*
extern protoypte
*/
extern void test_nfc(void);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __INC_TEST_NFC_H__ */


