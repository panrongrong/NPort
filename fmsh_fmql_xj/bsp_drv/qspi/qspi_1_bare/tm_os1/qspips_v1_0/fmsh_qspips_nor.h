/******************************************************************************
*
* Copyright (C) FMSH, Corp.  All rights reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
* FMSH BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
* WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
* OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*
* Except as contained in this notice, the name of the FMSH shall not be used
* in advertising or otherwise to promote the sale, use or other dealings in
* this Software without prior written authorization from FMSH.
*
******************************************************************************/
/*****************************************************************************/
/**
*
* @file fmsh_qspips_nor.h
* @addtogroup qspips_v1_1
* @{
*
* This header file contains the identifiers and basic spi nor driver
* functions (or macros) that can be used to access the device.
*
* <pre>
* MODIFICATION HISTORY:
*
* Ver   Who Date     Changes
* ----- --- -------- -----------------------------------------------
* 1.10  hzq 11/26/20 
* 		     First release
*
* </pre>
*
******************************************************************************/

#ifndef _FMSH_QSPIPS_NOR_H_	/* prevent circular inclusions */ 
#define _FMSH_QSPIPS_NOR_H_	/* by using protection macros */

#ifdef __cplusplus
extern "C" {
#endif

/**********************************Include File*********************************/

/**********************************Constant Definition**************************/  
/* qspi flash size(bits) code*/
#define QSPIFLASH_SIZE_ID_256K              (0x09)
#define QSPIFLASH_SIZE_ID_512K              (0x10)
#define QSPIFLASH_SIZE_ID_1M                (0x11)
#define QSPIFLASH_SIZE_ID_2M                (0x12)
#define QSPIFLASH_SIZE_ID_4M                (0x13)
#define QSPIFLASH_SIZE_ID_8M                (0x14)
#define QSPIFLASH_SIZE_ID_16M               (0x15)
#define QSPIFLASH_SIZE_ID_32M               (0x16)
#define QSPIFLASH_SIZE_ID_64M               (0x17)
#define QSPIFLASH_SIZE_ID_128M              (0x18)
#define QSPIFLASH_SIZE_ID_256M              (0x19)
#define QSPIFLASH_SIZE_ID_512M              (0x20)
#define QSPIFLASH_SIZE_ID_512M_TYPE2        (0x1A)
#define QSPIFLASH_SIZE_ID_1G                (0x21)
#define QSPIFLASH_SIZE_ID_1G_TYPE2          (0x1B)

/* qspi flash size(bytes)*/
#define QSPIFLASH_PAGE_SIZE                 (0x00000100)

#define QSPIFLASH_SECTOR_4K                 (0x00001000)
#define QSPIFLASH_SECTOR_64K                (0x00010000)
#define QSPIFLASH_SECTOR_256K               (0x00040000)    

#define QSPIFLASH_SIZE_256K                 (0x00008000)
#define QSPIFLASH_SIZE_512K                 (0x00010000)    
#define QSPIFLASH_SIZE_1M                   (0x00020000)
#define QSPIFLASH_SIZE_2M                   (0x00040000)
#define QSPIFLASH_SIZE_4M                   (0x00080000)    
#define QSPIFLASH_SIZE_8M                   (0x00100000)
#define QSPIFLASH_SIZE_16M                  (0x00200000)
#define QSPIFLASH_SIZE_32M                  (0x00400000)
#define QSPIFLASH_SIZE_64M                  (0x00800000)    
#define QSPIFLASH_SIZE_128M                 (0x01000000)
#define QSPIFLASH_SIZE_256M                 (0x02000000)
#define QSPIFLASH_SIZE_512M                 (0x04000000)
#define QSPIFLASH_SIZE_1G                   (0x08000000)
#define QSPIFLASH_SIZE_2G                   (0x10000000)
#define SPI_UNKNOWN_SIZE                    (0xFFFFFFFF) 

/* qspi commands    */
#define RDID_CMD                            (0x9F)
#define WREN_CMD                            (0x06)
#define WRDI_CMD                            (0x04)
#define RDSR1_CMD                           (0x05)
#define WRR_CMD                             (0x01)
#define READ_CMD                            (0x03)
#define FAST_READ_CMD                       (0x0B)
#define DOR_CMD                             (0x3B)
#define QOR_CMD                             (0x6B)
#define DIOR_CMD                            (0xBB)
#define QIOR_CMD                            (0xEB)    
#define SE_CMD                              (0xD8)
#define CE_CMD                              (0xC7)
#define PP_CMD                              (0x02)    

#define SPANSION_RDSR2_CMD                  (0x07) /*< Read Status Rregister-2 */
#define SPANSION_RDCR_CMD                   (0x35) /*< Read Configuration Rregister*/
#define SPANSION_BRRD_CMD                   (0x16) /*< Read Bank Rregister*/
#define SPANSION_BRWR_CMD                   (0x17) /*< Write Bank Rregister*/
   
#define MICRON_RDNVCR_CMD                   (0xB5) /*< Read Nonvolatile Configuration Rregister*/
#define MICRON_RDVCR_CMD                    (0x85) /*< Read Volatile Configuration Rregister*/
#define MICRON_RDECR_CMD                    (0x65) /*< Read Enhanced Volatile Configuration Rregister*/
#define MICRON_RDEAR_CMD                    (0xC8) /*< Read Enhanced Address Rregister*/
#define MICRON_WRNVCR_CMD                   (0xB1) /*< Write Nonvolatile Configuration Rregister*/
#define MICRON_WRVCR_CMD                    (0x81) /*< Write Volatile Configuration Rregister*/
#define MICRON_WRECR_CMD                    (0x61) /*< Write Enhanced Volatile Configuration Rregister*/
#define MICRON_WREAR_CMD                    (0xC5) /*< Write Enhanced Address Rregister*/

#define WINBOND_RDSR2_CMD                   (0x35) /*< Read Status Rregister-2 */
#define WINBOND_RDSR3_CMD                   (0x15) /*< Read Status Rregister-3 */
#define WINBOND_RDEAR_CMD                   (0xC8) /*< Read Extended Address Rregister */
#define WINBOND_WRSR2_CMD                   (0x31) /*< Write Status Rregister-2 */
#define WINBOND_WRSR3_CMD                   (0x11) /*< Write Status Rregister-3 */
#define WINBOND_WREAR_CMD                   (0xC5) /*< Write Extended Address Rregister */
         
#define MACRONIX_RDCR_CMD                   (0x15) /*< Read Configuration Rregister*/
#define MACRONIX_RDEAR_CMD                  (0xC8) /*< Read Extended Address Rregister */
#define MACRONIX_WREAR_CMD                  (0xC5) /*< Write Extended Address Rregister */
#define MACRONIX_EN4B_CMD                   (0xB7) /*< Enter 4B Mode */
#define MACRONIX_EX4B_CMD                   (0xE9) /*< Exit 4B Mode */
        
#define ISSI_RDPAR_CMD                      (0x61) /*< Read Read Parameter(Volatile) Rregister*/
#define ISSI_RDWRP_CMD                      (0x81) /*< Read Extended Read Parameter(Volatile) Rregister*/
#define ISSI_RDBR_CMD                       (0x16) /*< Read Bank Address Rregister*/
#define ISSI_WRBRV_CMD                      (0x17) /*< Write Bank Address(Volatile) Rregister*/
#define ISSI_WRBRNV_CMD                     (0x18) /*< Write Bank Address(Non-Volatile) Rregister*/        
#define ISSI_SPRNV_CMD                      (0x65) /*< Set Read Parameter(Non-Volatile) Rregister*/
#define ISSI_SPRV_CMD                       (0xC0) /*< Set Read Parameter(Volatile) Rregister*/   
#define ISSI_SEPRNV_CMD                     (0x85) /*< Set Extended Read Parameter(Non-Volatile) Rregister*/
#define ISSI_SEPRV_CMD                      (0x83) /*< Set Extended Read Parameter(Volatile) Rregister*/   
#define ISSI_EN4B_CMD                       (0xB7) /*< Enter 4B Mode */
#define ISSI_EX4B_CMD                       (0x29) /*< Exit 4B Mode */ 
         
#define FMSH_RDSR2_CMD                      (0x35) /*< Read Status Rregister-2 */
     
#define GD_RDSR2_CMD                        (0x35) /*< Read Status Rregister-2 */
    
/* qspi flash register bits*/
#define SRWD_MASK                           (0x80) /*< ignore WRR command when WP# is low */   
#define BP_MASK                             (0x1C) /*< block protection */
#define WEL_MASK                            (0x02)
#define BUSY_MASK                           (0x01)    

#define SPANSION_LC_MASK                    (0xC0)
#define SPANSION_QUAD_EN_MASK               (0x02)
#define SPANSION_EXTADDR_MASK               (0x80)     
#define SPANSION_BA24_MASK                  (0x01)

#define MICRON_NV_DUMMY_MASK                (0xF000) /*< dummy cycle numbers */
#define MICRON_SEG_SEL_MASK                 (0x0002) /*< select 128Mb segment at upper(0) or lower(1) */
#define MICRON_EXTADDR_MASK                 (0x0001) /*< select 4B addr(0) or 3B addr(1) */         
#define MICRON_V_DUMMY_MASK                 (0xF0) /*< dummy cycle numbers */

#define WINBOND_QUAD_EN_MASK                (0x02) /*< status2 bit 2 */
#define WINBOND_ADP_MASK                    (0x02)
#define WINBOND_ADS_MASK                    (0x01)
         
#define MACRONIX_QUAD_EN_MASK               (0x40) /*< status bit 6 */
#define MACRONIX_DC_MASK                    (0xC0) /*< cfg bits 6~7 */
#define MACRONIX_4B_MASK                    (0x20) /*< cfg bits 5 */
     
#define ISSI_QUAD_EN_MASK                   (0x40) /*< status bit 6 */
#define ISSI_DUMMY_MASK                     (0x78) /*< read parameter bits 3~6 */
#define ISSI_EXTADDR_MASK                   (0x80)

#define FMSH_QUAD_EN_MASK                   (0x02) /*< status2 bit 2 */

#define GD_QUAD_EN_MASK                     (0x02) /*< status2 bit 2 */
    
/* qspi flash delay parameters*/
#define QSPIFLASH_CSDA                      (0x00)
#define QSPIFLASH_CSDADS                    (0x00) 
#define QSPIFLASH_CSEOT                     (0x00)
#define QSPIFLASH_CSSOT                     (0x00)

/* qspi mode bits*/
#define SPANSION_MODE_BIT                   (0xA0)
#define MICRON_MODE_BIT                     (0x00)
#define WINBOND_MODE_BIT                    (0x20)
#define MACRONIX_MODE_BIT                   (0xA5)
#define ISSI_MODE_BIT                       (0xA0)
#define FMSH_MODE_BIT                       (0x20)

    
#define QSPI_FSR                            (0x1 << 1)
/**********************************Type Definition******************************/     
typedef struct{
    int version; 
    char* name;
    int id_len;
    u8 maker;
    u32 pageSize;
    u32 blockSize;
    u64 deviceSize; 
    struct FQspiPs_Param_Tag* param;
} FQspiPs_Nor_Device;

typedef struct{
    u8 speed_level;
    u8 io_mode;
    u8 latency;
    u8 mode_cycle;
    u8 dummy_cycle;
} FQspiPs_Nor_Timing;  

typedef struct FQspiPs_Param_Tag{
    int tinming_num;
    FQspiPs_Nor_Timing* timing;
    u8 enter_xip_mode_bit;
    u8 exit_xip_mode_bit;
    u16 tw_max_ms;
    u16 tpp_max_us;
    u16 tse_max_ms;
    u16 tbe_max_s;
} FQspiPs_Nor_Param; 

/**********************************Macro (inline function) Definition***********/


/**********************************Variable Definition**************************/
extern FQspiPs_Nor_Device qspi_nor_dev;
extern FQspiPs_Caps qspi_nor_caps_default;

/**********************************Function Prototype***************************/     
int FQspiPs_Nor_Initr(FQspiPs_T* qspiPtr); 

int FQspiPs_Nor_GetFlashInfo(FQspiPs_T* qspiPtr, u8* id);
int FQspiPs_Nor_SetFlashMode(FQspiPs_T* qspiPtr, u8 mode);

int FQspiPs_Nor_Write(FQspiPs_T* qspiPtr, u32 offset, u32 byteCount, u8 *sendBuffer);
int FQspiPs_Nor_Read(FQspiPs_T* qspiPtr, u32 offset, u32 byteCount, u8 *recvBuffer);
int FQspiPs_Nor_Erase(FQspiPs_T* qspiPtr, u32 offset, u32 byteCount, u32 blockSize);
int FQspiPs_Nor_SectorErase(FQspiPs_T* qspiPtr, u32 start_offset, u8 cmd);
int FQspiPs_Nor_ChipErase(FQspiPs_T* qspiPtr, u8 cmd);

int FQspiPs_Nor_Reset(FQspiPs_T* qspiPtr);
int FQspiPs_Nor_EnableQuad(FQspiPs_T* qspiPtr);
int FQspiPs_Nor_EnterXIP(FQspiPs_T* qspiPtr, u8 cmd);
int FQspiPs_Nor_Lock(FQspiPs_T* qspiPtr);
int FQspiPs_Nor_Unlock(FQspiPs_T* qspiPtr);
int FQspiPs_Nor_SetSegment(FQspiPs_T* qspiPtr, u8 highAddr);
int FQspiPs_Nor_Enter4B(FQspiPs_T* qspiPtr);
int FQspiPs_Nor_Exit4B(FQspiPs_T* qspiPtr);
u8 FQspiPs_Nor_GetBankReg(FQspiPs_T* qspiPtr);

u8 FQspiPs_Nor_GetStatus1(FQspiPs_T* qspiPtr);
u8 FQspiPs_Nor_GetFlagStatus(FQspiPs_T* qspiPtr);
void FQspiPs_Nor_ClearFlagStatus(FQspiPs_T* qspiPtr);

u16 FQspiPs_Nor_GetReg16(FQspiPs_T *qspiPtr, u8 cmd);
u8 FQspiPs_Nor_GetReg8(FQspiPs_T *qspiPtr, u8 cmd);
int FQspiPs_Nor_SetReg16(FQspiPs_T* qspiPtr, u8 cmd, u8 high_value, u8 low_value);
int FQspiPs_Nor_SetReg8(FQspiPs_T* qspiPtr, u8 cmd, u8 value);

int FQspiPs_Nor_WaitForWIP(FQspiPs_T* qspiPtr, u32 expire);
int FQspiPs_Nor_WaitForFSR(FQspiPs_T* qspiPtr, u32 expire);
int FQspiPs_Nor_WaitForReady(FQspiPs_T* qspiPtr, u32 expire);

int FQspiPs_Nor_WREN(FQspiPs_T* qspiPtr);
int FQspiPs_Nor_WRDI(FQspiPs_T* qspiPtr);  

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* prevent circular inclusions */o