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
01a, 14Jan21, jc  written.
*/

#include <vxWorks.h>
#include <stdio.h>
#include <semLib.h>
#include <sysLib.h>
#include <taskLib.h>
#include <vxBusLib.h>
#include <cacheLib.h>
#include <string.h>


#define XADC_BASE_ADDR            (0x43C00000)


#if 1

/****************************************************************************/
/**
*
* This macro converts XADC Raw Data to Temperature(centigrades).
*
* @param	AdcData is the Raw ADC Data from XADC.
*
* @return 	The Temperature in centigrades.
*
* @note		C-Style signature:
*		float XAdcPs_RawToTemperature(u32 AdcData);
*
*****************************************************************************/
#define XAdcPs_RawToTemperature(AdcData)				\
	((((float)(AdcData)/65536.0f)/0.00198421639f ) - 273.15f)


/****************************************************************************/
/**
*
* This macro converts XADC/ADC Raw Data to Voltage(volts).
*
* @param	AdcData is the XADC/ADC Raw Data.
*
* @return 	The Voltage in volts.
*
* @note		C-Style signature:
*		float XAdcPs_RawToVoltage(u32 AdcData);
*
*****************************************************************************/
#define XAdcPs_RawToVoltage(AdcData)  ((((float)(AdcData))* (3.0f))/65536.0f)


/****************************************************************************/
/**
*
* This macro converts Temperature in centigrades to XADC/ADC Raw Data.
*
* @param	Temperature is the Temperature in centigrades to be
*		converted to XADC/ADC Raw Data.
*
* @return 	The XADC/ADC Raw Data.
*
* @note		C-Style signature:
*		int XAdcPs_TemperatureToRaw(float Temperature);
*
*****************************************************************************/
#define XAdcPs_TemperatureToRaw(Temperature)				\
	((int)(((Temperature) + 273.15f)*65536.0f*0.00198421639f))


/****************************************************************************/
/**
*
* This macro converts Voltage in Volts to XADC/ADC Raw Data.
*
* @param	Voltage is the Voltage in volts to be converted to
*		XADC/ADC Raw Data.
*
* @return 	The XADC/ADC Raw Data.
*
* @note		C-Style signature:
*		int XAdcPs_VoltageToRaw(float Voltage);
*
*****************************************************************************/
#define XAdcPs_VoltageToRaw(Voltage)  ((int)((Voltage)*65536.0f/3.0f))

#endif

int test_xadc(void)
{
	int cnt = 0;
	
	UINT32 TempRawData;
	
	float TempData;
	float VccIntData;
	float VccAuxData;
	float VccPintData;
	float VccPauxData;

	int i = 0;	      
	      
	/* initial PS-PL port*/
	FMSH_WriteReg(FPS_SLCR_BASEADDR, 0x008, 0xDF0D767BU);
	FMSH_WriteReg(FPS_SLCR_BASEADDR, 0x838, 0xF);
	FMSH_WriteReg(FPS_SLCR_BASEADDR, 0x004, 0xDF0D767BU); 

	/*config efuse*/
	/* for(uint8_t i=0;i<15;i++)
	{
	    FMSH_WriteReg(0xE0040000,0x200,0x11);   
	    FMSH_WriteReg(0xE0040000,0x208,81+(i<<8));   
	    while((FMSH_ReadReg(0xE0040000,0x204) & 0x1)!=0x1);
	    
	}
	*/
    FMSH_WriteReg(0xE0040000, 0xe8, 0xf);
    FMSH_WriteReg(0xE0040000, 0x228, 0x1);    
	
    /*delay_ms(1000);*/
    taskDelay(60);

	/* reset XADC*/
	FMSH_WriteReg(XADC_BASE_ADDR, 0x000, 0xA);
	FMSH_WriteReg(XADC_BASE_ADDR, 0x010, 0x1);
	FMSH_WriteReg(XADC_BASE_ADDR, 0x010, 0x0);

	/* Loop to printf XADC value*/
	for (i=0; i<10; i++)
	{
		printf("---------------------XADC-EXAMPLE------------------\n\r");
		printf("Sample %d starting...\n\r", cnt++);
		      
		/* read temperature  */
		/*
		C_BASEADDR + 0x200: Temperature N/A R(6)
			The 12-bit Most Significant Bit (MSB) justified result of 
			on-device temperature	measurement is stored in this register.
		*/
		TempRawData = FMSH_ReadReg(XADC_BASE_ADDR, 0x200);
		TempData = XAdcPs_RawToTemperature(TempRawData);
		printf("Raw Temp %lu -> Real Temp %.3f C \n\r", TempRawData, TempData);

		/*OT Status*/
		/*
		C_BASEADDR + 0x08: Alarm Output Status Register(AOSR) 0x0 R(2) 
			Alarm Output Status Register
		*/
		TempRawData = FMSH_ReadReg(XADC_BASE_ADDR, 0x8);
		/*TempData = XAdcPs_RawToTemperature(TempRawData);*/
		printf("Alarm OT Status : 0x%x \n\r", TempRawData);

		/*OT flag*/
		/*
		C_BASEADDR + 0x2FC: Flag Register N/A R(6)
			The 16-bit register gives general status information of ALARM, 
			Over Temperature(OT), Disable XADC information. 
			Whether the XADC is using the internal reference voltage 
			or external reference voltage is also provided.
		*/
		TempRawData = FMSH_ReadReg(XADC_BASE_ADDR, 0x2FC);
		/*TempData = XAdcPs_RawToTemperature(TempRawData);*/
		printf("OT Flag : 0x%x \n\r", TempRawData);


		/* read temperature upper */
		/*
		C_BASEADDR + 0x340: Alarm Threshold Register 0 0x0 R/W 
			The 12-bit MSB justified alarm threshold register 0 (Temperature Upper)
		*/
		TempRawData = FMSH_ReadReg(XADC_BASE_ADDR, 0x340);
		TempData = XAdcPs_RawToTemperature(TempRawData);
		printf("Temp Upper %lu -> Real Temp Upper %.3f C \n\r", TempRawData, TempData);


		/* read temperature lower */
		/*
		C_BASEADDR + 0x350: Alarm Threshold Register 4 0x0 R/W 
			The 12-bit MSB justified alarm threshold register 4 (Temperature Lower).
		*/
		TempRawData = FMSH_ReadReg(XADC_BASE_ADDR, 0x350);
		TempData = XAdcPs_RawToTemperature(TempRawData);
		printf("Temp Lower %lu -> Real Temp Lower %.3f C \n\r", TempRawData, TempData);


		/* read OT upper */
		/*
		C_BASEADDR + 0x34C: Alarm Threshold Register 3 0x0 R/W(8)(9) 
			The 12-bit MSB justified alarm threshold register 3 (OT Upper)
		*/
		TempRawData = FMSH_ReadReg(XADC_BASE_ADDR, 0x34C);
		TempData = XAdcPs_RawToTemperature(TempRawData);
		printf("OT Upper %lu Real OT Upper %.3f \n\r", TempRawData, TempData);

		/* read OT lower */
		/*
		C_BASEADDR + 0x35C: Alarm Threshold Register 7 0x0 R/W 
			The 12-bit MSB justified alarm threshold register 7 (OT Lower)
		*/
		TempRawData = FMSH_ReadReg(XADC_BASE_ADDR, 0x35C);
		TempData = XAdcPs_RawToTemperature(TempRawData);
		printf("OT Lower %lu -> Real OT Lower %.3f C \n\r", TempRawData, TempData);

		/* delay*/
		printf("\n\n");
		taskDelay(100);		
	}

	return 0;
}


#if 1
/*
cmd & log:
---------
->testvx_update_bit3 "/sd0:1/pl_xadc.bit"

->test_xadc

=========
log:
=========
---------------------XADC-EXAMPLE------------------
Sample 9 starting...
Raw Temp 41592 -> Real Temp 46.696 C 
Alarm OT Status : 0x1e0 
OT Flag : 0x2e0 
Temp Upper 46573 -> Real Temp Upper 85.000 C 
Temp Lower 43322 -> Real Temp Lower 60.000 C 
OT Upper 40067 Real OT Upper 34.969 
OT Lower 39421 -> Real OT Lower 30.001 C 
*/
#endif 


