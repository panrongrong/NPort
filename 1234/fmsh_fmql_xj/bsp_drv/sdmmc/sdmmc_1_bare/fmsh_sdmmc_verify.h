#ifndef _FMSH_SDMMC_VERIFY_H_
#define _FMSH_SDMMC_VERIFY_H_

#include "../../common/fmsh_common.h"

#include "fmsh_sdmmc_lib.h"
#include "fmsh_sdmmc_hw.h"

#define SDMMC0_SELECT 0     /* sdmmc0*/
#define SDMMC1_SELECT 1     /* sdmmc1*/

/**  Number of errors displayed */
#define NB_ERRORS            5

/** Split R/W to 2, first R/W 4 blocks then remaining */
#define NB_SPLIT_MULTI       4

/** Maximum number of blocks read once */
#define MAX_MULTI_BLOCKS     128

typedef struct _sdmmc_data_param {
	u32 write_addr;
	u32 read_addr;
	u8 write_type;    /* 0: memory2sdmmc; 1: peripheral2sdmmc */
	u8 read_type;     /* 0: sdmmc2memory; 1: sdmmc2peripheral */
	u32 block_num;
	enum SDMMC_TransMode trans_mode;
	volatile u8 card_detect;
}SDMMC_dataParam;

extern FSdmmcPs_T g_SDMMC[BOARD_NUM_SDMMC];
extern SDMMC_dataParam g_SDMMC_dataParam;

/* Function prototypes*/
s32 FSdmmcPs_regRdWtCheck(FSdmmcPs_T *pSdmmc);
s32 FSdmmcPs_regInitCheck(FSdmmcPs_T *pSdmmc);
s32 FSdmmcPs_resetCheck(FSdmmcPs_T *pSdmmc, int index);
s32 FSdmmcPs_cardInitializeVerify(FSdmmcPs_T *pSdmmc, FDmaPs_T *dev_dmac, enum SDMMC_TransMode trans_mode);
s32 FSdmmcPs_cardDataTransfer(FSdmmcPs_T *pSdmmc, enum SDMMC_TransMode trans_mode, u8 wr, u8 rd);
s32 FSdmmcPs_intVerify(FSdmmcPs_T *pSdmmc, enum SDMMC_TransMode trans_mode);
s32 FSdmmcPs_fileVerify(void);
s32 FSdmmcPs_memAccessVerify(FSdmmcPs_T *pSdmmc);
u32 FSdmmcPs_initSdmmcCtrl(FSdmmcPs_T *pSdmmc, enum SDMMC_TransMode trans_mode);
void FSdmmcPs_initDmac(FSdmmcPs_T *pSdmmc);
int FSdmmcPs_verify(void);
int FSdmmcPs_readWriteData(FSdmmcPs_T *pSdmmc, u32 address, u32 len,  int rw);
void FSdmmcPs_checkCardState(FSdmmcPs_T *pSdmmc);
void FSdmmcPs_waitWriteDone(FSdmmcPs_T *pSdmmc);

#endif /* _FMSH_SDMMC_VERIFY_H_ */

