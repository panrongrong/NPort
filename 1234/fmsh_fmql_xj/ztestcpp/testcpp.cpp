#include <stdio.h>
#include "test.h"


void funCpp0()
{
	printf("this is %s \n", __FUNCTION__);
}

void funCpp1(int i)
{
	printf("this is %s: %d \n", __FUNCTION__, i);
	funC0();
}

