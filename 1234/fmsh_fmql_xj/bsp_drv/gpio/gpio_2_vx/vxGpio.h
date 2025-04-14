/* vxWdt.h - vxWdt driver header */

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
01a, 20May26, jc  written.
*/

#ifndef __INC_VX_GPIO_H__
#define __INC_VX_GPIO_H__



#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define  GPIO_REG_COMP_VER       0x18
#define  GPIO_REG_COMP_TYPE      0xFC

/*
Bank A - gpio[ 0:31] -- MIO  gpio number 32
Bank B - gpio[32:53] -- MIO  gpio number 22

Bank C - gpio[54:85] --  EMIO  gpio number 32
Bank D - gpio[86:117] -- EMIO  gpio number 32

GPIO0	4KB	0xE000_3000 ~ 0xE000_3FFF
---------------
Bank_x	offset:
---------------
Bank A  0x000
Bank B  0x100
Bank C  0x200
Bank D  0x400

IRQ:
49	SPI	GPIO0	
68	SPI	GPIO1		
69	SPI	GPIO2		
70	SPI	GPIO3		
*/
#define  VX_GPIO_CFG_BASE               (0xE0003000) 

#define  VX_GPIO_OFFSET_A               (0x000) 
#define  VX_GPIO_OFFSET_B               (0x100) 
#define  VX_GPIO_OFFSET_C               (0x200) 
#define  VX_GPIO_OFFSET_D               (0x400) /* no 0x300*/


/*
(1) Bank A 控制MIO 引脚[31:0];
(2) Bank B 控制MIO引脚[53:32];         // total: 54 GPIO_pin, not 64 gpio_pin

(3) Bank C 控制EMIO引脚信号[31:0]；
(4) Bank D 控制EMIO引脚信号[63:32]；

bankA: 0   ~ 31  (32)
bankB: 32  ~ 53  (22)
bankC: 54  ~ 85  (32)
bankD: 86  ~ 117 (32)
*/
#define  GPIO_BANK_A   (0)
#define  GPIO_BANK_B   (1)
#define  GPIO_BANK_C   (2)
#define  GPIO_BANK_D   (3)

#define GPIO_PIN_TO_BANK(gpio)          ((gpio < 32)  ? 0 :    \
		                                 (gpio < 54)  ? 1 :    \
		                                 (gpio < 86)  ? 2 :    \
		                                 (gpio < 118) ? 3 : 0)
		                                 
#define GPIO_PIN_TO_BIT(gpio)           ((gpio < 32)  ? ((gpio)) :          \
                                         (gpio < 54)  ? ((gpio) - 32) :     \
                                         (gpio < 86)  ? ((gpio) - 54) :     \
                                         (gpio < 118) ? ((gpio) - 86) : 0)
                                         
#define GPIO_BANK_BIT_TO_PIN(bank, bit)    ((bank == 0) ? (bit) :           \
		                                    (bank == 1) ? ((bit) + 32) :    \
		                                    (bank == 2) ? ((bit) + 54) :    \
		                                    (bank == 3) ? ((bit) + 86) : 0)

/*
gpio reg offset
*/
#define GPIO_REG_SWPORT_DR        (0x00)  /*  Port A/B/C/D data register volatile           u32 swport_dr;          //0x00    */
#define GPIO_REG_SWPORT_DDR       (0x04)  /*  Port A/B/C/D data direction registervolatile  u32 swport_ddr;         //0x04    */

#define GPIO_REG_INT_EN           (0x30)  /*  volatile u32 inten;               //0x30   */
#define GPIO_REG_INT_MASK         (0x34)  /*  volatile u32 intmask;             //0x34   */
#define GPIO_REG_INT_TYPELEVEL    (0x38)  /*  volatile u32 inttype_level;       //0x38   */
#define GPIO_REG_INT_POLAR        (0x3C)  /*  volatile u32 int_polarity;        //0x3c   */
#define GPIO_REG_INT_STATUS       (0x40)  /*  volatile u32 intstatus;           //0x40   */
#define GPIO_REG_RAW_INT_STATUS   (0x44)  /*  volatile u32 raw_intstatus;       //0x44   */
#define GPIO_REG_DEBOUNCE         (0x48)  /*  volatile u32 debounce;            //0x48   */
#define GPIO_REG_PORT_EOI         (0x4C)  /*  volatile u32 port_eoi;           //0x4c    */
#define GPIO_REG_EXT_PORT         (0x50)  /*  volatile u32 ext_port;           //0x50    */

#define GPIO_REG_LS_SYNC          (0x60)  /*  volatile u32 ls_sync;             //0x60   */
#define GPIO_REG_ID_CODE          (0x64)  /*  volatile u32 id_code;             //0x64   */
#define GPIO_REG_INT_BOTHEDGE     (0x68)  /*  volatile u32 int_bothedge;        //0x68   */
#define GPIO_REG_VER_IDCODE       (0x6C)  /*  volatile u32 ver_id_code;         //0x6c */

#define GPIO_REG_CFG_REG2         (0x70)  /*  volatile u32 config_reg2;         //0x70   */
#define GPIO_REG_CFG_REG1         (0x74)  /*  volatile u32 config_reg1;         //0x74   */

/*
register offsets
*/
#define SWPORTA_DR_OFFSET               (0x00)
#define SWPORTA_DDR_OFFSET              (0x04)
#define INTEN_OFFSET                    (0x30)
#define INTMASK_OFFSET                  (0x34)
#define INTTYPELEVEL_OFFSET             (0x38)
#define INTPOLARITY_OFFSET              (0x3C)
#define INTSTATUS_OFFSET                (0x40)
#define RAWINTSTATUS_OFFSET             (0x44)
#define DEBOUNCE_OFFSET                 (0x48)
#define EOI_OFFSET                      (0x4C)
#define EXTPORTA_OFFSET                 (0x50)
#define LSSYNC_OFFSET                   (0x60)
#define IDCODE_OFFSET                   (0x64)
#define COMPVERSION_OFFSET              (0x6C)
#define CONFIGREG1_OFFSET               (0x70)
#define CONFIGREG2_OFFSET               (0x74)

#define GPIO_INTERRUPT_MASK             (0x1FFFFFFF)

/*
49	SPI	GPIO0	
68	SPI	GPIO1		
69	SPI	GPIO2		
70	SPI	GPIO3	
*/
#define INT_VEC_GPIO_BANK_A             IVEC_TO_INUM(49)       /* BANK_A Interrupt */
#define INT_VEC_GPIO_BANK_B             IVEC_TO_INUM(68)       /* BANK_B Interrupt */
#define INT_VEC_GPIO_BANK_C             IVEC_TO_INUM(69)       /* BANK_C Interrupt */
#define INT_VEC_GPIO_BANK_D             IVEC_TO_INUM(70)       /* BANK_D Interrupt */

#define GPIO_PIN_TO_IRQ_NO(gpio)        ((gpio < 32)  ? (INT_VEC_GPIO_BANK_A) :    \
		                                 (gpio < 54)  ? (INT_VEC_GPIO_BANK_B) :    \
		                                 (gpio < 86)  ? (INT_VEC_GPIO_BANK_C) :    \
		                                 (gpio < 118) ? (INT_VEC_GPIO_BANK_D) : (INT_VEC_GPIO_BANK_A))
		                                 

#define GPIO_IRQ_SENSITIVE_NONE          (0x0)
#define GPIO_IRQ_LOW_LEVEL_SENSITIVE     (0x1)
#define GPIO_IRQ_HIGH_LEVEL_SENSITIVE    (0x2)
#define GPIO_IRQ_RISING_EDGE_SENSITIVE   (0x4)
#define GPIO_IRQ_FALLING_EDGE_SENSITIVE  (0x8)


#define  GPIO_LOW      (0)
#define  GPIO_HIGH     (1)

#define  GPIO_DIR_IN      (0)    /* bitX = 0  --  input;*/
#define  GPIO_DIR_OUT     (1)    /* bitX = 1  --   output.*/

#define  GPIO_IRQ_TYPE_LEVEL   (0)  /* Gpio_level_sensitive = 0x0,*/
#define  GPIO_IRQ_TYPE_EDGE    (1)  /* Gpio_edge_sensitive = 0x1*/


#define  GPIO_ACT_LOW_FALL_EDGE    (0)  /* Gpio_active_low_falling_edge = 0x0,*/
#define  GPIO_ACT_HIGH_RISE_EDGE   (1)  /* Gpio_active_high_rising_edge = 0x1*/

#define  GPIO_BANK_A_INT_ID		(49)
#define  GPIO_BANK_B_INT_ID		(68)
#define  GPIO_BANK_C_INT_ID		(69)
#define  GPIO_BANK_D_INT_ID		(70)

/*
gpio ctrl
*/
typedef struct _t_gpio_ctrl_
{
	int  ctrl_x;
	
	UINT16 devId;				/**< Unique ID of device */
	
	UINT32 cfgBaseAddr;		    /**< APB Base address of the device */
	UINT32 sysClk;				/**< Input clock frequency */
	
	UINT32 offset_A;
	UINT32 offset_B;
	UINT32 offset_C;
	UINT32 offset_D;
	
	UINT32 devIP_ver;
    UINT32 devIP_type;      /* identify peripheral types:  */

	int status;
} vxT_GPIO_CTRL;


typedef struct _t_gpio_
{
	int gpio_x;     /* ctrl-0*/
	
	int gpio_bank_x;      /* a/b/c/d: 0 ~ 3*/
	int gpio_pin_x;       /* 0 ~ 117*/
	
	vxT_GPIO_CTRL * pGpioCtrl;

	int init_flag;
} vxT_GPIO;
	

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __INC_VX_GPIO_H__ */


