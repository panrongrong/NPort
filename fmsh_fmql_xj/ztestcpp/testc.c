#include <stdio.h>
#include "test.h"


void funC0()
{
	printf("this is %s \n", __FUNCTION__);
}

void funC1(int i)
{
	printf("this is %s: %d \n", __FUNCTION__, i);
	funCpp1(i);
}

void test()
{
	int option;
	taskOptionsGet(0, &option);
	
	if ((VX_FP_TASK & option) == VX_FP_TASK)
	{
			printf("task %s supports float piont! \n", taskName(0));
	}
	else
	{
		printf("task %s don't supports floa piont!!! \n", taskName(0));
	}
	
	return;
}


/*
test cmd:
==========

-> test
task tShell0 supports float piont! 
value = 36 = 0x24 = '$'

-> funC1
this is funC1: 0 
this is funCpp1: 0 
this is funC0 
value = 15 = 0xf
-> 

*/



