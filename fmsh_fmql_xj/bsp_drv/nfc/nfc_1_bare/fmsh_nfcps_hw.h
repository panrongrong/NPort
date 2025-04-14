 
#ifndef _FMSH_NFCPS_HW_H_   /* prevent circular inclusions */
#define _FMSH_NFCPS_HW_H_

#include "fmsh_nfcps.h"

#ifdef __cplusplus
extern "C" {
#endif

/***************************** Include Files *********************************/ 

/************************** Constant Definitions *****************************/ 
    
/***********************
 * Registers
 ***********/
#define NFCPS_COMMAND_OFFSET              0x00
#define NFCPS_CONTROL_OFFSET              0x04
#define NFCPS_STATUS_OFFSET               0x08
#define NFCPS_STATUS_MASK_OFFSET          0x0C
#define NFCPS_INT_MASK_OFFSET             0x10
#define NFCPS_INT_STATUS_OFFSET           0x14
#define NFCPS_ECC_CTRL_OFFSET             0x18
#define NFCPS_ECC_OFFSET_OFFSET           0x1C
#define NFCPS_ECC_STAT_OFFSET             0x20
#define NFCPS_ADDR0_COL_OFFSET            0x24
#define NFCPS_ADDR0_ROW_OFFSET            0x28
#define NFCPS_ADDR1_COL_OFFSET            0x2C
#define NFCPS_ADDR1_ROW_OFFSET            0x30
#define NFCPS_PROTECT_OFFSET              0x34
#define NFCPS_FIFO_DATA_OFFSET            0x38
#define NFCPS_DATA_REG_OFFSET             0x3C
#define NFCPS_DATA_REG_SIZE_OFFSET        0x40
#define NFCPS_DEV0_PTR_OFFSET             0x44
#define NFCPS_DEV1_PTR_OFFSET             0x48
#define NFCPS_DMA_ADDR_L_OFFSET           0x64
#define NFCPS_DMA_CNT_OFFSET              0x6C
#define NFCPS_DMA_CTRL_OFFSET             0x70
#define NFCPS_BBM_CTRL_OFFSET             0x74
#define NFCPS_MEM_CTRL_OFFSET             0x80
#define NFCPS_DATA_SIZE_OFFSET            0x84
#define NFCPS_TIMINGS_ASYN_OFFSET         0x88
#define NFCPS_TIME_SEQ_0_OFFSET           0x90
#define NFCPS_TIME_SEQ_1_OFFSET           0x94
#define NFCPS_TIME_GEN_SEQ_0_OFFSET       0x98
#define NFCPS_TIME_GEN_SEQ_1_OFFSET       0x9C
#define NFCPS_TIME_GEN_SEQ_2_OFFSET       0xA0
#define NFCPS_FIFO_INIT_OFFSET            0xB0
#define NFCPS_FIFO_STATE_OFFSET           0xB4
#define NFCPS_GEN_SEQ_CTRL_OFFSET         0xB8
#define NFCPS_MLUN_OFFSET                 0xBC
#define NFCPS_DEV0_SIZE_OFFSET            0xC0
#define NFCPS_DEV1_SIZE_OFFSET            0xC4
#define NFCPS_SS_CCNT0_OFFSET             0xE0
#define NFCPS_SS_CCNT1_OFFSET             0xE4
#define NFCPS_SS_SCNT_OFFSET              0xE8
#define NFCPS_SS_ADDR_DEV_CTRL_OFFSET     0xEC
#define NFCPS_SS_CMD0_OFFSET              0xF0
#define NFCPS_SS_CMD1_OFFSET              0xF4
#define NFCPS_SS_CMD2_OFFSET              0xF8
#define NFCPS_SS_CMD3_OFFSET              0xFC
#define NFCPS_SS_ADDR_OFFSET              0x100
#define NFCPS_SS_MSEL_OFFSET              0x104
#define NFCPS_SS_REQ_OFFSET               0x108
#define NFCPS_SS_BRK_OFFSET               0x10C
#define NFCPS_DMA_TLVL_OFFSET             0x114
#define NFCPS_CMD_MARK_OFFSET             0x124
#define NFCPS_LUN_STATUS_0_OFFSET         0x128
#define NFCPS_TIME_GEN_SEQ_3_OFFSET       0x134
#define NFCPS_INTERNAL_STATUS_OFFSET      0x148
#define NFCPS_BBM_OFFSET_OFFSET           0x14C
#define NFCPS_PARAM_REG_OFFSET            0x150

/*************************
 * Interrupt Mask
 ************/
#define NFCPS_INTR_ECC_INT1_MASK            (0x1 << 25)
#define NFCPS_INTR_ECC_INT0_MASK            (0x1 << 24)
#define NFCPS_INTR_STAT_ERR_INT1_MASK       (0x1 << 17)
#define NFCPS_INTR_STAT_ERR_INT0_MASK       (0x1 << 16)
#define NFCPS_INTR_MEM1_RDY_INT_MASK        (0x1 << 9)
#define NFCPS_INTR_MEM0_RDY_INT_MASK        (0x1 << 8)
#define NFCPS_INTR_PG_SZ_ERR_INT_MASK       (0x1 << 6)
#define NFCPS_INTR_SS_READY_INT_MASK        (0x1 << 5)
#define NFCPS_INTR_TRANS_ERR_MASK           (0x1 << 4)
#define NFCPS_INTR_DMA_INT_MASK             (0x1 << 3)
#define NFCPS_INTR_DATA_REG_MASK            (0x1 << 2)
#define NFCPS_INTR_CMD_END_INT_MASK         (0x1 << 1)
#define NFCPS_INTR_PORT_INT_MASK            (0x1 << 0)
     
#define NFCPS_INTR_ALL                      (0xffffffff)

/*********************************
 * CONTROL 
 *****************/
#define NFCPS_AUTO_READ_STAT_EN_MASK        (0x1 << 23)
#define NFCPS_MLUN_EN_MASK                  (0x1 << 22)
#define NFCPS_SMALL_BLOCK_EN_MASK           (0x1 << 21)
#define NFCPS_ADDR1_AUTO_INCR_MASK          (0x1 << 17)
#define NFCPS_ADDR0_AUTO_INCR_MASK          (0x1 << 16)
#define NFCPS_PROT_EN_MASK                  (0x1 << 14)
#define NFCPS_BBM_EN_MASK                   (0x1 << 13)
#define NFCPS_IOWIDTH16_MASK                (0x1 << 12)
#define NFCPS_BLOCK_SIZE_MASK               (0x3 << 6)
#define NFCPS_ECC_EN_MASK                   (0x1 << 5)
#define NFCPS_INT_EN_MASK                   (0x1 << 4)
#define NFCPS_RETRY_EN_MASK                 (0x1 << 3)
#define NFCPS_ECC_BLOCK_SIZE_MASK           (0x3 << 1)
#define NFCPS_READ_STATUS_EN_MASK           (0x1 << 0)
     
#define NFCPS_USER_CTRL_MASK                (NFCPS_AUTO_READ_STAT_EN_MASK |\
                                                NFCPS_MLUN_EN_MASK |\
                                                NFCPS_SMALL_BLOCK_EN_MASK |\
                                                NFCPS_ADDR1_AUTO_INCR_MASK |\
                                                NFCPS_ADDR0_AUTO_INCR_MASK |\
                                                NFCPS_PROT_EN_MASK |\
                                                NFCPS_BBM_EN_MASK |\
                                                NFCPS_ECC_EN_MASK |\
                                                NFCPS_INT_EN_MASK |\
                                                NFCPS_RETRY_EN_MASK |\
                                                NFCPS_READ_STATUS_EN_MASK )
#define NFCPS_ECCBLOCKSIZE(bytes)           (bytes)
#define NFCPS_BLOCKSIZE(pages)              (pages)

#define NFCPS_ECC_BLOCK_SIZE_SHIFT           (1)
#define NFCPS_BLOCK_SIZE_SHIFT               (6)

/*****************************
 * CTRL & STATUS
 **************/
#define NFCPS_CTRL_MEM0WP_MASK              (0x1 << 8)
#define NFCPS_CTRL_MEMCE_MASK               (0x1) 
     
#define NFCPS_CTRL_MEM0WP_SHIFT             (8)

/****************************     
 * PROTECT
 ************/    
#define NFCPS_PROT_UP_MASK                  (0xffff << 16) 
#define NFCPS_PROT_DOWN_MASK                (0xffff)   

#define NFCPS_PROT_UP_SHIFT                 (16)      
 
/****************************
 * ECC CTRL
 ****************/     
#define NFCPS_ECC_SEL_MASK                  (0x3 << 16)
#define NFCPS_ECC_THRESHOLD_MASK            (0x3f << 8)
#define NFCPS_ECC_CAP_MASK                  (0x7)

#define NFCPS_ECC_SEL_SHIFT                 (16)
#define NFCPS_ECC_THRESHOLD_SHIFT           (8)

#define NFCPS_ECC_ERROR_STATE               (0x0)
#define NFCPS_ECC_UNC_STATE                 (0x1)
#define NFCPS_ECC_OVER_STATE                (0x2)
#define NFCPS_ECC_CAP2_STATE                (0x0)
#define NFCPS_ECC_CAP4_STATE                (0x1)
#define NFCPS_ECC_CAP8_STATE                (0x2)
#define NFCPS_ECC_CAP16_STATE               (0x3)
#define NFCPS_ECC_CAP24_STATE               (0x4)
#define NFCPS_ECC_CAP32_STATE               (0x5)

/*****************************
 * ECC STATUS
 **************/
#define NFCPS_ECC_OVER_MASK                 (0x1 << 16)
#define NFCPS_ECC_UNC_MASK                  (0x1 << 8)
#define NFCPS_ECC_ERROR_MASK                (0x1)

/******************************
 * FIFO_INIT
 ***********/
#define NFCPS_FIFO_INIT_MASK                (0x1)

/*****************************
 * FIFO STAT
 *************/ 
#define NFCPS_STAT_DFIFO_W_EMPTY_MASK       (0x1 << 7)
#define NFCPS_STAT_DFIFO_R_FULL_MASK        (0x1 << 6)
#define NFCPS_STAT_CFIFO_ACCPT_W_MASK       (0x1 < <5)
#define NFCPS_STAT_CFIFO_ACCPT_R_MASK       (0x1 << 4)
#define NFCPS_STAT_CFIFO_FULL_MASK          (0x1 << 3)
#define NFCPS_STAT_CFIFO_EMPTY_MASK         (0x1 << 2)
#define NFCPS_STAT_DFIFO_W_FULL_MASK        (0x1 << 1)
#define NFCPS_STAT_DFIFO_R_EMPTY_MASK       (0x1 << 0)

#define NFCPS_FIFO_DEPTH                    (2048)
/******************************
 * DATA REG SIZE
 ***********/
#define NFCPS_DATAREG_SIZE_MASK             (0x3)   

#define NFCPS_DATA_1B_STATE                 (0x0) 
#define NFCPS_DATA_2B_STATE                 (0x1)
#define NFCPS_DATA_3B_STATE                 (0x2)
#define NFCPS_DATA_4B_STATE                 (0x3)

/******************************
 * DATA SIZE
 ***********/
#define NFCPS_DATA_SIZE_MASK                (0x3fff) 

/*****************************
 * DMA CTRL
 *************/
#define NFCPS_DMA_READY_MASK                (0x1)
#define NFCPS_DMA_ERR_MASK                  (0x1 << 1)
#define NFCPS_DMA_BURSTTYPE_MASK            (0x7 << 2)
#define NFCPS_DMA_MODE_MASK                 (0x1 << 5)    
#define NFCPS_DMA_START_MASK                (0x1 << 7)    
     
#define NFCPS_DMA_START_SHIFT               (7)     
#define NFCPS_DMA_MODE_SHIFT                (5)  
#define NFCPS_DMA_BURSTTYPE_SHIFT           (2)  
#define NFCPS_DMA_ERR_SHIFT                 (1)  
     
#define NFCPS_DMA_SFR_STATE                 (0x0)  
#define NFCPS_DMA_SG_STATE                  (0x1) 
     
#define NFCPS_DMA_BURSTTYPE0_STATE          (0x0) 
#define NFCPS_DMA_BURSTTYPE1_STATE          (0x1)
#define NFCPS_DMA_BURSTTYPE2_STATE          (0x2)
#define NFCPS_DMA_BURSTTYPE3_STATE          (0x3)
#define NFCPS_DMA_BURSTTYPE4_STATE          (0x4)
#define NFCPS_DMA_BURSTTYPE5_STATE          (0x5)

/*****************************
 * STATUS
 *************/    
#define NFCPS_STAT_CMD_ID_MASK               (0xff << 16)
#define NFCPS_STAT_DATA_REG_RDY_MASK         (0x1 << 10)
#define NFCPS_STAT_DATA_SIZE_ERROR_MASK      (0x1 << 9)
#define NFCPS_STAT_CTRL_RDY_MASK             (0x1 << 8)  
#define NFCPS_STAT_MEM0_RDY_MASK             (0x1)
     
#define NFCPS_RST_CTRL_MASK                 (0x3)
/**************************** Type Definitions *******************************/  
typedef struct{
  u32 command;
  u32 value;
} FNfcPs_Command_T;

/***************** Macros (Inline Functions) Definitions *********************/  
        
/************************** Variable Definitions *****************************/

/************************** Function Prototypes ******************************/
void FNfcPs_SetCtrl(FNfcPs_T* nfc, UINT32 ctrl);
UINT32 FNfcPs_GetCtrl(FNfcPs_T* nfc);
void FNfcPs_EnableReadStatus(FNfcPs_T* nfc);
void FNfcPs_EnableReadRNB(FNfcPs_T* nfc);
void FNfcPs_EnableRetry(FNfcPs_T* nfc);
void FNfcPs_DisableRetry(FNfcPs_T* nfc);
void FNfcPs_EnableIntr(FNfcPs_T* nfc);
void FNfcPs_DisableIntr(FNfcPs_T* nfc);
void FNfcPs_EnableEcc(FNfcPs_T* nfc);
void FNfcPs_DisableEcc(FNfcPs_T* nfc);
void FNfcPs_EnableBbm(FNfcPs_T* nfc);
void FNfcPs_DisableBbm(FNfcPs_T* nfc);
void FNfcPs_EnableProt(FNfcPs_T* nfc);
void FNfcPs_DisableProt(FNfcPs_T* nfc);
void FNfcPs_EnableAddr0AutoIncr(FNfcPs_T* nfc);
void FNfcPs_DisableAddr0AutoIncr(FNfcPs_T* nfc);
void FNfcPs_EnableAddr1AutoIncr(FNfcPs_T* nfc);
void FNfcPs_DisableAddr1AutoIncr(FNfcPs_T* nfc);
void FNfcPs_EnableSmallBlock(FNfcPs_T* nfc);
void FNfcPs_DisableSmallBlock(FNfcPs_T* nfc);
void FNfcPs_EnableMLun(FNfcPs_T* nfc);
void FNfcPs_DisableMLun(FNfcPs_T* nfc);
void FNfcPs_EnableAutoReadStatus(FNfcPs_T* nfc);
void FNfcPs_DisableAutoReadStatus(FNfcPs_T* nfc);
int FNfcPs_SetIOWidth(FNfcPs_T* nfc, UINT32 width);
int FNfcPs_SetEccBlockSize(FNfcPs_T* nfc, UINT32 size);
int FNfcPs_SetBlockSize(FNfcPs_T* nfc, UINT32 size);
void FNfcPs_EnableHwProt(FNfcPs_T* nfc);
void FNfcPs_DisableHwProt(FNfcPs_T* nfc);
UINT32 FNfcPs_GetHwProt(FNfcPs_T* nfc);
int FNfcPs_SetProtArea(FNfcPs_T* nfc, UINT32 lowBlock, UINT32 highBlock);
void FNfcPs_SetIntrMask(FNfcPs_T* nfc, UINT32 mask);
UINT32 FNfcPs_GetIntrMask(FNfcPs_T* nfc);
void FNfcPs_ClearIntrStatus(FNfcPs_T* nfc);
UINT32 FNfcPs_GetIntrStatus(FNfcPs_T* nfc);
int FNfcPs_SetEccCtrl(FNfcPs_T* nfc, UINT32 sel, UINT32 threshold, UINT32 cap);
void FNfcPs_SetEccOffset(FNfcPs_T* nfc, u16 offset);
u16 FNfcPs_GetEccOffset(FNfcPs_T* nfc);
void FNfcPs_ClearEccStat(FNfcPs_T* nfc);
u32 FNfcPs_GetEccStat(FNfcPs_T* nfc);
void FNfcPs_WriteFIFO(FNfcPs_T* nfc, UINT32 data);
UINT32 FNfcPs_ReadFIFO(FNfcPs_T* nfc);
void FNfcPs_ClearFIFO(FNfcPs_T* nfc);
int FNfcPs_SetDataRegSize(FNfcPs_T* nfc, UINT32 size);
UINT32 FNfcPs_GetDataReg(FNfcPs_T* nfc);
void FNfcPs_SetDataSizeInBytes(FNfcPs_T* nfc, UINT32 size);
void FNfcPs_SetStatusMask(FNfcPs_T* nfc, UINT32 mask);
void FNfcPs_SetDMAAddr(FNfcPs_T* nfc, UINT32 address);
void FNfcPs_SetDMACnt(FNfcPs_T* nfc, UINT32 byteCount);
void FNfcPs_SetDMATrigLvl(FNfcPs_T* nfc, UINT32 level);
int FNfcPs_SetDMACtrl(FNfcPs_T* nfc, UINT32 mode, UINT32 burstType, UINT32 start);
UINT32 FNfcPs_GetDMAStatus(FNfcPs_T* nfc);
void FNfcPs_SetTiming(FNfcPs_T* nfc);
void FNfcPs_InitRemap(FNfcPs_T* nfc);
void FNfcPs_SetDevPtr(FNfcPs_T* nfc, UINT32 address);
void FNfcPs_SetDevSize(FNfcPs_T* nfc, UINT32 size);
int FNfcPs_WaitForBit(FNfcPs_T* nfc, UINT32 regOffset, UINT32 mask, u8 pollBit);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif	/* prevent circular inclusions */_