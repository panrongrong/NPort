==============================
一、vxWorks下测试过程：
==============================

/*
接口代码在： fmsh_fmql_xj\bsp_drv\devc\devc_2_vx\vxDevC.c: 
															void testvx_update_bit3(char* bit_file)

接口代码在： fmsh_fmql_xj\bsp_drv\gpio\gpio_2_vx\vxGpio.c: 
															void test_gpio_led(void)
															void test_gpio_button(void)
*/

#if 1  // test_note_&_log

/*
test cmd & procedure:

1. call the function: void testvx_update_bit3(char* bit_file)
->testvx_update_bit3 "/sd0:1/pl_led_key.bit"

2. first:
-> test_gpio_led

   then: 
		observe the led flash

3. first: 
-> test_gpio_button

   then: 
		press the any button of SW5,6,7,8,19 of Demo_board
*/

#endif


