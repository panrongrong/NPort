#ifndef _FMSH_QSPIPS_FLASH_H_	/* prevent circular inclusions */ 
#define _FMSH_QSPIPS_FLASH_H_	/* by using protection macros */

#ifdef __cplusplus
extern "C" {
#endif

/**********************************Include File*********************************/

/**********************************Constant Definition**************************/

/****************************
 * qspi flash manufacturer code
 ************/
#define SPI_SPANSION_ID                     (0x01)
#define SPI_MICRON_ID                       (0x20)
#define SPI_WINBOND_ID                      (0xEF)
#define SPI_MACRONIX_ID                     (0xC2)
#define SPI_ISSI_ID                         (0x9D)
#define SPI_FMSH_ID                         (0xA1)

/*****************************
 * qspi flash size(bits) code
 ************/
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
#define QSPIFLASH_SIZE_ID_1G                (0x21)
#define MACRONIX_QSPIFLASH_SIZE_ID_512M     (0x1A)
#define MACRONIX_QSPIFLASH_SIZE_ID_1G       (0x1B)
#define ISSI_QSPIFLASH_SIZE_ID_512M         (0x1A)

#define SPI_WRONG_ID                        (0xFF)

/*************************
 * qspi flash size(bytes)
 **************/
#define QSPIFLASH_PAGE_256                 (0x00000100)

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

/**************************
 * qspi flash command set (general)
 *************/

#define WRR_CMD                             (0x01)
#define PP_CMD                              (0x02)
#define READ_CMD                            (0x03)
#define WRDI_CMD                            (0x04)
#define RDSR1_CMD                           (0x05)
#define WREN_CMD                            (0x06)

#define FAST_READ_CMD                       (0x0B)
#define DOR_CMD                             (0x3B)
#define QOR_CMD                             (0x6B)
#define RDID_CMD                            (0x9F)
#define DIOR_CMD                            (0xBB)
#define CE_CMD                              (0xC7)
#define SE_CMD                              (0xD8)
#define QIOR_CMD                            (0xEB)    

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

/*************************** 
 * qspi flash register bits
 *****************/   
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

/***************************
 * qspi flash delay parameters
 *****************/
#define QSPIFLASH_CSDA                      (0x00)
#define QSPIFLASH_CSDADS                    (0x00) 
#define QSPIFLASH_CSEOT                     (0x00)
#define QSPIFLASH_CSSOT                     (0x00)

/***************************
 * qspiflash mode bits
 *******************/
#define SPANSION_MODE_BIT                   (0xA0)
#define MICRON_MODE_BIT                     (0x00)
#define WINBOND_MODE_BIT                    (0x20)
#define MACRONIX_MODE_BIT                   (0xA5)
#define ISSI_MODE_BIT                       (0xA0)
#define FMSH_MODE_BIT                       (0x20)

/*****************************
 * qspiflash dummy
 *************/

/**********************************Type Definition******************************/

/**********************************Macro (inline function) Definition***********/

/**********************************Variable Definition**************************/
    
/**********************************Function Prototype***************************/     

int FQspiFlash_WREN(FQspiPs_T* qspi);
int FQspiFlash_WRDI(FQspiPs_T* qspi);   
u8 FQspiFlash_GetStatus1(FQspiPs_T* qspi);
u16 FQspiFlash_GetReg16(FQspiPs_T *qspi, u8 cmd);
u8 FQspiFlash_GetReg8(FQspiPs_T *qspi, u8 cmd);
int FQspiFlash_SetReg16(FQspiPs_T* qspi, u8 cmd, u8 high_value, u8 low_value);
int FQspiFlash_SetReg8(FQspiPs_T* qspi, u8 cmd, u8 value);
u32 FQspiFlash_ReadId(FQspiPs_T* qspi);
int FQspiFlash_WaitForWIP(FQspiPs_T* qspi);
int FQspiFlash_SectorErase(FQspiPs_T* qspi, u32 start_offset, u8 cmd);
int FQspiFlash_ChipErase(FQspiPs_T* qspi, u8 cmd);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* prevent circular inclusions */
