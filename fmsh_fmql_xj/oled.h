/*
 * oled.h
 *
 *  Created on: 2025��2��27��
 *      Author: 70429
 */

#ifndef SRC_OLED_H_
#define SRC_OLED_H_

//-----------------OLED�˿ڶ���----------------

#define PL_AXI_WriteReg(BaseAddress, RegOffset, Data) \
  	sysAxiWriteLong((BaseAddress) + (RegOffset), (uint32_t)(Data))

#define PL_AXI_ReadReg(BaseAddress, RegOffset) \
    sysAxiReadLong((BaseAddress) + (RegOffset))

#define PL_AXI_BASE  0x40000000   

#define OLED_SCL_Clr() PL_AXI_WriteReg(PL_AXI_BASE,0x00,0)//SCL
#define OLED_SCL_Set() PL_AXI_WriteReg(PL_AXI_BASE,0x00,1)

#define OLED_SDA_Clr() PL_AXI_WriteReg(PL_AXI_BASE,0x04,0)//SDA
#define OLED_SDA_Set() PL_AXI_WriteReg(PL_AXI_BASE,0x04,1)

#define OLED_RES_Clr() PL_AXI_WriteReg(PL_AXI_BASE,0x08,0)//RES
#define OLED_RES_Set() PL_AXI_WriteReg(PL_AXI_BASE,0x08,1)

#define OLED_DC_Clr()  PL_AXI_WriteReg(PL_AXI_BASE,0x0C,0)//DC
#define OLED_DC_Set()  PL_AXI_WriteReg(PL_AXI_BASE,0x0C,1)

#define OLED_CS_Clr()  PL_AXI_WriteReg(PL_AXI_BASE,0x10,0)//CS
#define OLED_CS_Set()  PL_AXI_WriteReg(PL_AXI_BASE,0x10,1)


#define OLED_CMD  0	//д����
#define OLED_DATA 1	//д����

//port
#define OLED0_Clr() PL_AXI_WriteReg(PL_AXI_BASE,0x30,0)
#define OLED0_Set() PL_AXI_WriteReg(PL_AXI_BASE,0x30,1)

#define OLED1_Clr() PL_AXI_WriteReg(PL_AXI_BASE,0x34,0)
#define OLED1_Set() PL_AXI_WriteReg(PL_AXI_BASE,0x34,1)

#define OLED2_Clr() PL_AXI_WriteReg(PL_AXI_BASE,0x38,0)
#define OLED2_Set() PL_AXI_WriteReg(PL_AXI_BASE,0x38,1)

#define OLED3_Clr()  PL_AXI_WriteReg(PL_AXI_BASE,0x3C,0)
#define OLED3_Set()  PL_AXI_WriteReg(PL_AXI_BASE,0x3C,1)

#define OLED4_Clr()  PL_AXI_WriteReg(PL_AXI_BASE,0x40,0)
#define OLED4_Set()  PL_AXI_WriteReg(PL_AXI_BASE,0x40,1)

#define OLED5_Clr() PL_AXI_WriteReg(PL_AXI_BASE,0x44,0)
#define OLED5_Set() PL_AXI_WriteReg(PL_AXI_BASE,0x44,1)

#define OLED6_Clr() PL_AXI_WriteReg(PL_AXI_BASE,0x48,0)
#define OLED6_Set() PL_AXI_WriteReg(PL_AXI_BASE,0x48,1)

#define OLED7_Clr() PL_AXI_WriteReg(PL_AXI_BASE,0x4C,0)
#define OLED7_Set() PL_AXI_WriteReg(PL_AXI_BASE,0x4C,1)

#define OLED8_Clr()  PL_AXI_WriteReg(PL_AXI_BASE,0x50,0)
#define OLED8_Set()  PL_AXI_WriteReg(PL_AXI_BASE,0x50,1)

#define OLED9_Clr()  PL_AXI_WriteReg(PL_AXI_BASE,0x54,0)
#define OLED9_Set()  PL_AXI_WriteReg(PL_AXI_BASE,0x54,1)

#define OLED10_Clr() PL_AXI_WriteReg(PL_AXI_BASE,0x58,0)
#define OLED10_Set() PL_AXI_WriteReg(PL_AXI_BASE,0x58,1)

#define OLED11_Clr() PL_AXI_WriteReg(PL_AXI_BASE,0x5C,0)
#define OLED11_Set() PL_AXI_WriteReg(PL_AXI_BASE,0x5C,1)

#define OLED12_Clr() PL_AXI_WriteReg(PL_AXI_BASE,0x60,0)
#define OLED12_Set() PL_AXI_WriteReg(PL_AXI_BASE,0x60,1)

#define OLED13_Clr()  PL_AXI_WriteReg(PL_AXI_BASE,0x64,0)
#define OLED13_Set()  PL_AXI_WriteReg(PL_AXI_BASE,0x64,1)

#define OLED14_Clr()  PL_AXI_WriteReg(PL_AXI_BASE,0x68,0)
#define OLED14_Set()  PL_AXI_WriteReg(PL_AXI_BASE,0x68,1)

#define OLED15_Clr()  PL_AXI_WriteReg(PL_AXI_BASE,0x6C,0)
#define OLED15_Set()  PL_AXI_WriteReg(PL_AXI_BASE,0x6C,1)


//RX
#define OLEDRX0_Clr() PL_AXI_WriteReg(PL_AXI_BASE,0x230,0)
#define OLEDRX0_Set() PL_AXI_WriteReg(PL_AXI_BASE,0x230,1)

#define OLEDRX1_Clr() PL_AXI_WriteReg(PL_AXI_BASE,0x234,0)
#define OLEDRX1_Set() PL_AXI_WriteReg(PL_AXI_BASE,0x234,1)

#define OLEDRX2_Clr() PL_AXI_WriteReg(PL_AXI_BASE,0x238,0)
#define OLEDRX2_Set() PL_AXI_WriteReg(PL_AXI_BASE,0x238,1)

#define OLEDRX3_Clr()  PL_AXI_WriteReg(PL_AXI_BASE,0x23C,0)
#define OLEDRX3_Set()  PL_AXI_WriteReg(PL_AXI_BASE,0x23C,1)

#define OLEDRX4_Clr()  PL_AXI_WriteReg(PL_AXI_BASE,0x240,0)
#define OLEDRX4_Set()  PL_AXI_WriteReg(PL_AXI_BASE,0x240,1)

#define OLEDRX5_Clr() PL_AXI_WriteReg(PL_AXI_BASE,0x244,0)
#define OLEDRX5_Set() PL_AXI_WriteReg(PL_AXI_BASE,0x244,1)

#define OLEDRX6_Clr() PL_AXI_WriteReg(PL_AXI_BASE,0x248,0)
#define OLEDRX6_Set() PL_AXI_WriteReg(PL_AXI_BASE,0x248,1)

#define OLEDRX7_Clr() PL_AXI_WriteReg(PL_AXI_BASE,0x24C,0)
#define OLEDRX7_Set() PL_AXI_WriteReg(PL_AXI_BASE,0x24C,1)

#define OLEDRX8_Clr()  PL_AXI_WriteReg(PL_AXI_BASE,0x250,0)
#define OLEDRX8_Set()  PL_AXI_WriteReg(PL_AXI_BASE,0x250,1)

#define OLEDRX9_Clr()  PL_AXI_WriteReg(PL_AXI_BASE,0x254,0)
#define OLEDRX9_Set()  PL_AXI_WriteReg(PL_AXI_BASE,0x254,1)

#define OLEDRX10_Clr() PL_AXI_WriteReg(PL_AXI_BASE,0x258,0)
#define OLEDRX10_Set() PL_AXI_WriteReg(PL_AXI_BASE,0x258,1)

#define OLEDRX11_Clr() PL_AXI_WriteReg(PL_AXI_BASE,0x25C,0)
#define OLEDRX11_Set() PL_AXI_WriteReg(PL_AXI_BASE,0x25C,1)

#define OLEDRX12_Clr() PL_AXI_WriteReg(PL_AXI_BASE,0x260,0)
#define OLEDRX12_Set() PL_AXI_WriteReg(PL_AXI_BASE,0x260,1)

#define OLEDRX13_Clr()  PL_AXI_WriteReg(PL_AXI_BASE,0x264,0)
#define OLEDRX13_Set()  PL_AXI_WriteReg(PL_AXI_BASE,0x264,1)

#define OLEDRX14_Clr()  PL_AXI_WriteReg(PL_AXI_BASE,0x268,0)
#define OLEDRX14_Set()  PL_AXI_WriteReg(PL_AXI_BASE,0x268,1)

#define OLEDRX15_Clr()  PL_AXI_WriteReg(PL_AXI_BASE,0x26C,0)
#define OLEDRX15_Set()  PL_AXI_WriteReg(PL_AXI_BASE,0x26C,1)

//TX
#define OLEDTX0_Clr() PL_AXI_WriteReg(PL_AXI_BASE,0x130,0)
#define OLEDTX0_Set() PL_AXI_WriteReg(PL_AXI_BASE,0x130,1)

#define OLEDTX1_Clr() PL_AXI_WriteReg(PL_AXI_BASE,0x134,0)
#define OLEDTX1_Set() PL_AXI_WriteReg(PL_AXI_BASE,0x134,1)

#define OLEDTX2_Clr() PL_AXI_WriteReg(PL_AXI_BASE,0x138,0)
#define OLEDTX2_Set() PL_AXI_WriteReg(PL_AXI_BASE,0x138,1)

#define OLEDTX3_Clr()  PL_AXI_WriteReg(PL_AXI_BASE,0x13C,0)
#define OLEDTX3_Set()  PL_AXI_WriteReg(PL_AXI_BASE,0x13C,1)

#define OLEDTX4_Clr()  PL_AXI_WriteReg(PL_AXI_BASE,0x140,0)
#define OLEDTX4_Set()  PL_AXI_WriteReg(PL_AXI_BASE,0x140,1)

#define OLEDTX5_Clr() PL_AXI_WriteReg(PL_AXI_BASE,0x144,0)
#define OLEDTX5_Set() PL_AXI_WriteReg(PL_AXI_BASE,0x144,1)

#define OLEDTX6_Clr() PL_AXI_WriteReg(PL_AXI_BASE,0x148,0)
#define OLEDTX6_Set() PL_AXI_WriteReg(PL_AXI_BASE,0x148,1)

#define OLEDTX7_Clr() PL_AXI_WriteReg(PL_AXI_BASE,0x14C,0)
#define OLEDTX7_Set() PL_AXI_WriteReg(PL_AXI_BASE,0x14C,1)

#define OLEDTX8_Clr()  PL_AXI_WriteReg(PL_AXI_BASE,0x150,0)
#define OLEDTX8_Set()  PL_AXI_WriteReg(PL_AXI_BASE,0x150,1)

#define OLEDTX9_Clr()  PL_AXI_WriteReg(PL_AXI_BASE,0x154,0)
#define OLEDTX9_Set()  PL_AXI_WriteReg(PL_AXI_BASE,0x154,1)

#define OLEDTX10_Clr() PL_AXI_WriteReg(PL_AXI_BASE,0x158,0)
#define OLEDTX10_Set() PL_AXI_WriteReg(PL_AXI_BASE,0x158,1)

#define OLEDTX11_Clr() PL_AXI_WriteReg(PL_AXI_BASE,0x15C,0)
#define OLEDTX11_Set() PL_AXI_WriteReg(PL_AXI_BASE,0x15C,1)

#define OLEDTX12_Clr() PL_AXI_WriteReg(PL_AXI_BASE,0x160,0)
#define OLEDTX12_Set() PL_AXI_WriteReg(PL_AXI_BASE,0x160,1)

#define OLEDTX13_Clr()  PL_AXI_WriteReg(PL_AXI_BASE,0x164,0)
#define OLEDTX13_Set()  PL_AXI_WriteReg(PL_AXI_BASE,0x164,1)

#define OLEDTX14_Clr()  PL_AXI_WriteReg(PL_AXI_BASE,0x168,0)
#define OLEDTX14_Set()  PL_AXI_WriteReg(PL_AXI_BASE,0x168,1)

#define OLEDTX15_Clr()  PL_AXI_WriteReg(PL_AXI_BASE,0x16C,0)
#define OLEDTX15_Set()  PL_AXI_WriteReg(PL_AXI_BASE,0x16C,1)

void OLED_ClearPoint(uint8_t x,uint8_t y);
void OLED_ColorTurn(uint8_t i);
void OLED_DisplayTurn(uint8_t i);
void OLED_WR_Byte(uint8_t dat,uint8_t mode);
void OLED_DisPlay_On(void);
void OLED_DisPlay_Off(void);
void OLED_Refresh(void);
void OLED_Clear(void);
void OLED_DrawPoint(uint8_t x,uint8_t y,uint8_t t);
void OLED_DrawLine(uint8_t x1,uint8_t y1,uint8_t x2,uint8_t y2,uint8_t mode);
void OLED_DrawCircle(uint8_t x,uint8_t y,uint8_t r);
void OLED_ShowChar(uint8_t x,uint8_t y,uint8_t chr,uint8_t size1,uint8_t mode);
void OLED_ShowChar6x8(uint8_t x,uint8_t y,uint8_t chr,uint8_t mode);
void OLED_ShowString(uint8_t x,uint8_t y,uint8_t *chr,uint8_t size1,uint8_t mode);
void OLED_ShowNum(uint8_t x,uint8_t y,uint32_t num,uint8_t len,uint8_t size1,uint8_t mode);
void OLED_ShowChinese(uint8_t x,uint8_t y,uint8_t num,uint8_t size1,uint8_t mode);
void OLED_ScrollDisplay(uint8_t num,uint8_t space,uint8_t mode);
void OLED_ShowPicture(uint8_t x,uint8_t y,uint8_t sizex,uint8_t sizey,uint8_t BMP[],uint8_t mode);
void OLED_Init(void);

#endif /* SRC_OLED_H_ */
