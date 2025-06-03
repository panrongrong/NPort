#ifndef __TEST_H__
#define __TEST_H__



#ifdef __cplusplus
extern "C" void funCpp0();
extern "C" void funCpp1(int i);

extern "C" void funC0();
extern "C" void funC1(int i);

#else
	
extern void funCpp0();
extern void funCpp1(int i);

extern void funC0();
extern void funC1(int i);
	
#endif

#endif

