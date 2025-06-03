 
#ifndef _FMSH_NFCPS_BBM_H_   /* prevent circular inclusions */
#define _FMSH_NFCPS_BBM_H_

/**********************************Include File*********************************/

/**********************************Constant Definition**************************/
#define NANDFLASH_BBT_SCAN_2ND_PAGE		        1	/**< Scan the second page for bad block information */
#define NANDFLASH_BBT_BLOCK_OFFSET		        0 	/**< Block offset of Bad Block Table Desc */
#define NANDFLASH_BBT_SIG_OFFSET		        8 	/**< Bad Block Table signature offset */
#define NANDFLASH_BBT_VER_OFFSET		        12	/**< Bad block Table version offset */
#define NANDFLASH_BBT_SIG_LEN		            4	/**< Bad block Table signature length */
#define NANDFLASH_BBT_MAX_BLOCKS		        3	/**< Bad block Table max blocks */

#define NANDFLASH_BB_PATTERN_OFFSET_SMALL_PAGE	5	/**< Bad block pattern offset in a page */
#define NANDFLASH_BB_PATTERN_LENGTH_SMALL_PAGE	1	/**< Bad block pattern length */
#define NANDFLASH_BB_PATTERN_OFFSET_LARGE_PAGE	0	/**< Bad block pattern offset in a large page */
#define NANDFLASH_BB_PATTERN_LENGTH_LARGE_PAGE	2	/**< Bad block pattern length */
#define NANDFLASH_BB_PATTERN			        0xFF	/**< Bad block pattern to search in a page */  
/*
 * Block definitions for RAM based Bad Block Table (BBT)
 */
#define NANDPS_BLOCK_GOOD			0x3	/**< Block is good */
#define NANDPS_BLOCK_BAD			0x2	/**< Block is bad */
#define NANDPS_BLOCK_RESERVED       0x1
#define NANDPS_BLOCK_FACTORY_BAD    0x0

#define NANDPS_BBT_BLOCK_SHIFT		2	/**< 1 byte represent 4 block in BBT */
#define NANDPS_BLOCK_TYPE_MASK		0x03	/**< Block type mask */
#define NANDPS_BLOCK_SHIFT_MASK		0x06	/**< Block shift mask for a Bad Block Table entry byte */ 

/**********************************Type Definition******************************/

/**********************************Macro (inline function) Definition***********/

/**********************************Variable Definition**************************/

/**********************************Function Prototype***************************/
void FNfcPs_InitBadBlockTableDesc(FNfcPs_T* nfc);
int FNfcPs_ScanBadBlockTable(FNfcPs_T* nfc);
int FNfcPs_CheckBlock(FNfcPs_T* nfc, u16 block);
int FNfcPs_MarkBlockBad(FNfcPs_T* nfc, u32 block);
             
#endif	/* prevent circular inclusions */ 