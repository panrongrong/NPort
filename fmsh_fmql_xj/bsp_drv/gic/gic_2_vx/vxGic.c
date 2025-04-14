/* vxGic.c - fmsh 7010/7045 Gic driver */

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
01a, 14Mayc21, jc  written.
*/

/*
DESCRIPTION
	no vxbus struct
*/

/* includes */

#include <vxWorks.h>
#include <stdio.h>
#include <semLib.h>
#include <sysLib.h>
#include <taskLib.h>
#include <vxBusLib.h>
#include <cacheLib.h>
#include <string.h>
#include <vxbTimerLib.h>

#include "vxGic.h"




void vx_FGicPs_Enable(UINT32 Int_Id)
{
	UINT32 Mask;
	UINT32 addr = 0;
	
	/*
	 * The Int_Id is used to create the appropriate mask for the
	 * desired bit position. Int_Id currently limited to 0 - 31
	 */
	Mask = 0x00000001U << (Int_Id % 32U);

	/*
	 * Enable the selected interrupt source by setting the
	 * corresponding bit in the Enable Set register.
	 */
	/*FGicPs_DistWriteReg(InstancePtr, (u32)FGicPs_ENABLE_SET_OFFSET + ((Int_Id / 32U) *4U), Mask);*/
	addr = FPAR_SCUGIC_DIST_BASEADDR + (UINT32)FGicPs_ENABLE_SET_OFFSET + ((Int_Id / 32U) * 4U), 
	
	*(UINT32*)addr = Mask;  /* set irq-en-bit*/
	return;
}


int vxGic_Get_IrqFlag(UINT32 Int_Id)
{
	UINT32 Mask, reg = 0;	
	UINT32 addr = 0;
	int ret = 0;
	
	Mask = 0x00000001U << (Int_Id % 32U);

	/*
	 * Enable the selected interrupt source by setting the
	 * corresponding bit in the Enable Set register.
	 */
	/*FGicPs_DistWriteReg(InstancePtr, (u32)FGicPs_ENABLE_SET_OFFSET + ((Int_Id / 32U) *4U), Mask);*/
	addr = FPAR_SCUGIC_DIST_BASEADDR + (UINT32)FGicPs_ENABLE_SET_OFFSET + ((Int_Id / 32U) * 4U), 
	reg = *((UINT32*)addr);

	if ((reg & Mask) == Mask)
	{
		ret = 1;
	}
	else
	{
		ret = 0;
	}

	return ret;
}


void vx_FGicPs_Disable(u32 Int_Id)
{
	UINT32 Mask;
	UINT32 addr = 0;

	/*
	 * The Int_Id is used to create the appropriate mask for the
	 * desired bit position. Int_Id currently limited to 0 - 31
	 */
	Mask = 0x00000001U << (Int_Id % 32U);

	/*
	 * Disable the selected interrupt source by setting the
	 * corresponding bit in the IDR.
	 */
	/*FGicPs_DistWriteReg(InstancePtr, (UINT32)FGicPs_DISABLE_OFFSET + ((Int_Id / 32U) *4U), Mask);*/
	addr = FPAR_SCUGIC_DIST_BASEADDR + (UINT32)FGicPs_DISABLE_OFFSET + ((Int_Id / 32U) * 4U);
	
	*(UINT32*)addr = Mask;  /* clr irq-en-bit*/
	return;	
}


/*****************************************************************************/
/**
*
* Allows software to simulate an interrupt in the interrupt controller.  This
* function will only be successful when the interrupt controller has been
* started in simulation mode.  A simulated interrupt allows the interrupt
* controller to be tested without any device to drive an interrupt input
* signal into it.
*
* @param	Int_Id is the software interrupt ID to simulate an interrupt.
* @param	Cpu_Id is the list of CPUs to send the interrupt. 
            CPU0: CPu_Id = 1;
            CPU1: CPu_Id = 2;
            CPU2: CPu_Id = 4;
            CPU3: CPu_Id = 8;
*
* @return
*
* GIC_SUCCESS if successful, or GIC_FAILURE if the interrupt could not be
* simulated
*
* @note		None.
*
******************************************************************************/
int  vx_FGicPs_SoftwareIntr(UINT32 Int_Id, UINT32 Cpu_Id)
{
	UINT32 Mask;
	UINT32 addr = 0;

	/*
	 * The Int_Id is used to create the appropriate mask for the
	 * desired interrupt. Int_Id currently limited to 0 - 15
	 * Use the target list for the Cpu ID.
	 */
	Mask = ((Cpu_Id << 16U) | Int_Id) &	(FGicPs_SFI_TRIG_CPU_MASK | FGicPs_SFI_TRIG_INTID_MASK);

	/*
	 * Write to the Software interrupt trigger register. Use the appropriate
	 * CPU Int_Id.
	 */
	/*FGicPs_DistWriteReg(InstancePtr, FGicPs_SFI_TRIG_OFFSET, Mask);*/
	addr = FPAR_SCUGIC_DIST_BASEADDR + (UINT32)FGicPs_SFI_TRIG_OFFSET;

	/* Indicate the interrupt was successfully simulated */	
	*(UINT32*)addr = Mask; 
	return OK;
}



/************************** Constant Definitions *****************************/


/**************************** Type Definitions *******************************/

/***************** Macros (Inline Functions) Definitions *********************/

/************************** Variable Definitions *****************************/

/************************** Function Prototypes ******************************/



/****************************************************************************/
/**
* Sets the interrupt priority and trigger type for the specificd IRQ source.
*
* @param	InstancePtr is a pointer to the instance to be worked on.
* @param	Int_Id is the IRQ source number to modify
* @param	Priority is the new priority for the IRQ source. 0 is highest
* 			priority, 0xF8 (248) is lowest. There are 32 priority levels
*			supported with a step of 8. Hence the supported priorities are
*			0, 8, 16, 32, 40 ..., 248.
* @param	Trigger is the new trigger type for the IRQ source.
* Each bit pair describes the configuration for an INT_ID.
* SFI    Read Only    b10 always
* PPI    Read Only    depending on how the PPIs are configured.
*                    b01    Active HIGH level sensitive
*                    b11 Rising edge sensitive
* SPI                LSB is read only.
*                    b01    Active HIGH level sensitive
*                    b11 Rising edge sensitive/
*
* @return	None.
*
* @note		None.
*
*****************************************************************************/
void vx_FGicPs_Set_PrioTrigType(UINT32 Int_Id, UINT8 Priority, UINT8 Trigger)
{
	UINT32 addr = 0;
	UINT32 RegValue;
	UINT8 LocalPriority = Priority;
	UINT8 LocalTrig = Trigger;


	/*
	 * Determine the register to write to using the Int_Id.
	 */
	/* RegValue = FGicPs_DistReadReg(InstancePtr, FGicPs_PRIORITY_OFFSET_CALC(Int_Id));*/
	addr = FPAR_SCUGIC_DIST_BASEADDR + FGicPs_PRIORITY_OFFSET_CALC(Int_Id);
	RegValue = *((UINT32*)addr);  /* read*/

	/*
	 * The priority bits are Bits 7 to 3 in GIC Priority Register. This
	 * means the number of priority levels supported are 32 and they are
	 * in steps of 8. The priorities can be 0, 8, 16, 32, 48, ... etc.
	 * The lower order 3 bits are masked before putting it in the register.
	 */
	LocalPriority = LocalPriority & (UINT8)FGicPs_INTR_PRIO_MASK;
	
	/*
	 * Shift and Mask the correct bits for the priority and trigger in the
	 * register
	 */
	RegValue &= ~(FGicPs_PRIORITY_MASK << ((Int_Id % 4U) * 8U));
	RegValue |= (UINT32)LocalPriority << ((Int_Id % 4U) * 8U);

	/*
	 * Write the value back to the register.
	 */
	/*FGicPs_DistWriteReg(InstancePtr, FGicPs_PRIORITY_OFFSET_CALC(Int_Id), RegValue);*/
	addr = FPAR_SCUGIC_DIST_BASEADDR + FGicPs_PRIORITY_OFFSET_CALC(Int_Id);
	*(UINT32*)addr = RegValue; 

	/*
	 * Determine the register to write to using the Int_Id.
	 */
	/*RegValue = FGicPs_DistReadReg(InstancePtr, FGicPs_INT_CFG_OFFSET_CALC (Int_Id));*/
	addr = FPAR_SCUGIC_DIST_BASEADDR + FGicPs_INT_CFG_OFFSET_CALC(Int_Id);
	RegValue = *((UINT32*)addr);  /* read*/

	/*
	 * Shift and Mask the correct bits for the priority and trigger in the
	 * register
	 */
	RegValue &= ~(FGicPs_INT_CFG_MASK << ((Int_Id % 16U) * 2U));
	/*RegValue |= (UINT32)Trigger << ((Int_Id % 16U) *2U);*/
	RegValue |= (UINT32)LocalTrig << ((Int_Id % 16U) * 2U);

	/*
	 * Write the value back to the register.
	 */
	/*FGicPs_DistWriteReg(InstancePtr, FGicPs_INT_CFG_OFFSET_CALC(Int_Id), RegValue);*/
	addr = FPAR_SCUGIC_DIST_BASEADDR + FGicPs_INT_CFG_OFFSET_CALC(Int_Id);
	*(UINT32*)addr = RegValue;  /* write*/

	return;
}

/****************************************************************************/
/**
* Gets the interrupt priority and trigger type for the specificd IRQ source.
*
* @param	InstancePtr is a pointer to the instance to be worked on.
* @param	Int_Id is the IRQ source number to modify
* @param	Priority is a pointer to the value of the priority of the IRQ
*		source. This is a return value.
* @param	Trigger is pointer to the value of the trigger of the IRQ
*		source. This is a return value.
*
* @return	None.
*
* @note		None
*
*****************************************************************************/
void vx_FGicPs_Get_PrioTrigType(UINT32 Int_Id, UINT8 *Priority, UINT8 *Trigger)
{
	UINT32 RegValue;
	UINT32 addr = 0;

	/*
	 * Determine the register to read to using the Int_Id.
	 */
	/*RegValue = FGicPs_DistReadReg(InstancePtr, FGicPs_PRIORITY_OFFSET_CALC(Int_Id));*/
	addr = FPAR_SCUGIC_DIST_BASEADDR + FGicPs_PRIORITY_OFFSET_CALC(Int_Id);
	RegValue = *((UINT32*)addr);  /* read*/

	/*
	 * Shift and Mask the correct bits for the priority and trigger in the
	 * register
	 */
	RegValue = RegValue >> ((Int_Id % 4U) * 8U);
	*Priority = (UINT8)(RegValue & FGicPs_PRIORITY_MASK);

	/*
	 * Determine the register to read to using the Int_Id.
	 */
	/*RegValue = FGicPs_DistReadReg(InstancePtr,	FGicPs_INT_CFG_OFFSET_CALC (Int_Id));*/
	addr = FPAR_SCUGIC_DIST_BASEADDR + FGicPs_INT_CFG_OFFSET_CALC(Int_Id);
	RegValue = *((UINT32*)addr);  /* read*/

	/*
	 * Shift and Mask the correct bits for the priority and trigger in the
	 * register
	 */
	RegValue = RegValue >> ((Int_Id % 16U) * 2U);
	*Trigger = (UINT8)(RegValue & FGicPs_INT_CFG_MASK);

	return;
}

void gic_show(UINT32 Int_Id)
{
	int ret = 0;
	UINT8 prio = 0, trig = 0;

	printf("\n==========================\n");
	
	ret = vxGic_Get_IrqFlag(Int_Id);
	if (ret == 1)
	{
		printf("Gic-No.(%d) Irq: Enable! \n", Int_Id);
	}
	else
	{
		printf("Gic-No.(%d) Irq: Disable! \n", Int_Id);	
	}
	
	vx_FGicPs_Get_PrioTrigType(Int_Id, &prio, &trig);	
    printf("      priority : %d \n", prio);	
	if (trig == 1)
	{
		printf("      trig_type: %d-%s \n", trig, "ACTIVE_HIGH");  /* 1-VXB_INTR_TRIG_ACTIVE_HIGH;*/
	}
	else
	{
		printf("      trig_type: %d-%s \n", trig, "TRIG_EDGE");  /* 3-VXB_INTR_TRIG_EDGE*/
	}
	
	printf("==========================\n\n");
	return;
}

typedef struct _irq_name_no_ {
	char irq_name[16];
	int irq_no;
} T_IRQ_NAME_NO;

T_IRQ_NAME_NO g_irq_name_no[44] = 
{
	{"TIMER0_1_INT_ID",    42},
	{"TIMER0_2_INT_ID",    43},
	{"TIMER0_3_INT_ID",    44},
	{"DMA_INT_ID	 ",    45},  
	{"QSPI0_INT_ID	 ",    46},
	{"QSPI1_INT_ID	 ",    47}, 
	{"NFC_INT_ID	 ",    48},
	{"GPIO0_INT_ID	 ",    49},
	{"USB0_INT_ID	 ",    50}, 
	{"GMAC0_INT_ID	 ",    51},
	{"SDMCC0_INT_ID  ",    52},  
	{"I2C0_INT_ID	 ",    53},
	{"SPI0_INT_ID	 ",	   54},
	{"UART0_INT_ID	 ",    55},
	{"CAN0_INT_ID	 ",    56},
	{"PL0_INT_ID	 ",    57},
	{"PL1_INT_ID	 ",    58},
	{"PL2_INT_ID	 ",    59},
	{"PL3_INT_ID	 ",    60}, 	
	{"PL4_INT_ID	 ",    61},
	{"PL5_INT_ID	 ",    62},
	{"PL6_INT_ID	 ",    63},
	{"PL7_INT_ID	 ",    64},
	{"TIMER1_1_INT_ID",    65},
	{"TIMER1_2_INT_ID",    66},
	{"TIMER1_3_INT_ID",    67},
	{"GPIO1_INT_ID	 ",    68},
	{"GPIO2_INT_ID	 ",    69},
	{"GPIO3_INT_ID	 ",    70},
	{"USB1_INT_ID	 ",    71},
	{"GMAC1_INT_ID	 ",    72},
	{"SDMCC1_INT_ID  ",    73},  
	{"I2C1_INT_ID	 ",    74},
	{"SPI1_INT_ID	 ",    75},
	{"UART1_INT_ID	 ",    76},
	{"CAN1_INT_ID	 ",    77},
	{"PL8_INT_ID	 ",    84},
	{"PL9_INT_ID	 ",    85},
	{"PL10_INT_ID	 ",    86},
	{"PL11_INT_ID	 ",    87}, 	
	{"PL12_INT_ID	 ",    88},
	{"PL13_INT_ID	 ",    89},
	{"PL14_INT_ID	 ",    90},
	{"PL15_INT_ID	 ",    91}
};


void gic_show2(UINT32 Int_Id)
{
	int ret = 0;
	UINT8 prio = 0, trig = 0;

	ret = vxGic_Get_IrqFlag(Int_Id);
	if (ret == 1)
	{
		printf("Irq: Enable, \t", Int_Id);
	}
	else
	{
		printf("Irq: Disable, \t", Int_Id);	
	}
	
	vx_FGicPs_Get_PrioTrigType(Int_Id, &prio, &trig);	
    printf("prio: %d, \t", prio);	
	if (trig == 1)
	{
		printf("trig: %d-%s ", trig, "ACTIVE_HIGH");  /* 1-VXB_INTR_TRIG_ACTIVE_HIGH;*/
	}
	else
	{
		printf("trig: %d-%s ", trig, "TRIG_EDGE");  /* 3-VXB_INTR_TRIG_EDGE*/
	}

	printf("\n");
	
	return;
}

void gic_show_all()
{
	int i = 0;
	int irq_no;
	char * pIrq_name;	

	printf("\n=====================================\n");
	for (i=0; i<44; i++)
	{
		irq_no = g_irq_name_no[i].irq_no;
		pIrq_name = (char*)g_irq_name_no[i].irq_name;

		printf("GIC-(%d): %s \t", irq_no, pIrq_name);
		gic_show2(irq_no);

		if (((i+1) % 5) == 0)
		{
			printf("\n");
		}
	}
	printf("=====================================\n\n");
	return;
}

#if 0

/****************************************************************************/
/**
* Sets the target CPU for the interrupt of a peripheral
*
* @param	InstancePtr is a pointer to the instance to be worked on.
* @param	Cpu_Id is a CPU number for which the interrupt has to be targeted
* @param	Int_Id is the IRQ source number to modify,CPU0:CPu_Id=1;CPU1:CPu_Id = 2;CPU2:CPu_Id = 4;CPU3:CPu_Id = 8
*
* @return	None.
*
* @note		None
*
*****************************************************************************/
void FGicPs_InterruptMaptoCpu(FGicPs *InstancePtr, UINT8 Cpu_Id, UINT32 Int_Id)
{
	UINT32 RegValue, Offset;
	RegValue = FGicPs_DistReadReg(InstancePtr,
			FGicPs_SPI_TARGET_OFFSET_CALC(Int_Id));

	Offset =  (Int_Id & 0x3);

	/*RegValue = (RegValue | (~(0xFF << (Offset*))) );*/
	RegValue = (RegValue & (0xFFFFFFFF&(~(0xFF << (Offset*8)))) );
	RegValue |= ((Cpu_Id) << (Offset*8));

	FGicPs_DistWriteReg(InstancePtr,
		FGicPs_SPI_TARGET_OFFSET_CALC(Int_Id),
		RegValue);
}
/** @} */

/******Interrupt Setup********/
UINT32  FGicPs_SetupInterruptSystem(FGicPs *InstancePtr)
{
	UINT32 RegValue1 = 0U;
	UINT32 Index;
	UINT32 Status;
        static FGicPs_Config* GicConfig; 
        
        GicConfig = FGicPs_LookupConfig(GIC_DEVICE_ID);
         if (NULL == GicConfig) {
            return GIC_FAILURE;
         }
         InstancePtr->Config = GicConfig;
	/*
	 * Read the ID registers.
	 */
	for(Index=0U; Index<=3U; Index++) {
		RegValue1 |= FGicPs_DistReadReg(InstancePtr,
			((UINT32)FGicPs_PCELLID_OFFSET + (Index * 4U))) << (Index * 8U);
	}

	if(FGicPs_PCELL_ID != RegValue1){
              return GIC_FAILURE;
	} 
        /*
        FGicPs_DistWriteReg(InstancePtr,
		FGicPs_INT_CFG_OFFSET_CALC(32U),
		0U);
        */
         {
            /*int Status;*/
            Status = FGicPs_CfgInitialize(InstancePtr, GicConfig,
				GicConfig->CpuBaseAddress);
            if (Status != GIC_SUCCESS) {
		return GIC_FAILURE;
            }
	 }
   
        FMSH_ExceptionEnable();  
        return Status;
}


/******Enable interrupt  Group********/
void   FGicPs_EnableSelGroup(FGicPs *InstancePtr)
{
 
        FGicPs_DistWriteReg(InstancePtr, FGicPs_DIST_EN_OFFSET,
						0x03);     
        FGicPs_CPUWriteReg(InstancePtr, FGicPs_CONTROL_OFFSET, 0x1FU);    

}
/******Set interrupt  Group********/


void FGicPs_SetGroup(FGicPs *InstancePtr, UINT32 Int_Id, UINT8 groupNo)
{
        UINT32 RegValue;

        RegValue = FGicPs_DistReadReg(InstancePtr,
			FGicPs_SECURITY_OFFSET_CALC(Int_Id));

	/*
	 * Enable the selected interrupt source by setting the
	 * corresponding bit in the Enable Set register.

	 */
        
        
        RegValue &= ~(0x00000001 << (Int_Id%32U));
	RegValue |= ((UINT32)groupNo <<(Int_Id%32U));

        
        
        
        FGicPs_DistWriteReg(InstancePtr,(UINT32)FGicPs_SECURITY_OFFSET + 
                              ((Int_Id / 32U) * 4U), RegValue);
}

#endif

