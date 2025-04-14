/* vxbMmcStorage.h - MMC driver header file for vxBus */

/*
 * Copyright (c) 2012, 2015 Wind River Systems, Inc.
 *
 * The right to copy, distribute, modify or otherwise make use
 * of this software may be licensed only pursuant to the terms
 * of an applicable Wind River license agreement.
 */

/*
modification history
--------------------
01b,26oct15,myt  add support of reliable write for eMMC (VXW6-84383)
01a,01dec12,e_d  created
*/

#ifndef __INCvxbMmcStorageh
#define __INCvxbMmcStorageh

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

/* includes */

#include <../src/hwif/h/storage/blkXbd.h>

/* SD/MMC XBD service task priority */

#define SDMMC_XBD_SVC_TASK_PRI           50

/* SD/MMC XBD service task stack size */

#define MMC_RCA_BASE                     0x100
#define EXT_CSD_SEC_CNT                  212

/* forward declarations */

#define MMCCARD_NAME                      "mmcStorage"

#define MMC_MAX_RW_SECTORS                2048  /* origin*/
/*#define MMC_MAX_RW_SECTORS                32*/

#define MMC_2G_SECTORS                    0x400000
#define MMC_DEFAULT_SECTOR_SIZE           512

#define EXT_CSD_DEVICE_TYPE_26            (1<<0)    /* Card can run at 26MHz */
#define EXT_CSD_DEVICE_TYPE_52            (1<<1)    /* Card can run at 52MHz */
#define EXT_CSD_DEVICE_TYPE_DDR_18V       (1<<2)   /* Card can run at 52MHz */
#define EXT_CSD_DEVICE_TYPE_DDR_12V       (1<<3)   /* Card can run at 52MHz */
#define EXT_CSD_DEVICE_TYPE_SDR_HS_18V    (1<<4)    /* Card can run at 200MHz */
#define EXT_CSD_DEVICE_TYPE_SDR_HS_12V    (1<<5)    /* Card can run at 200MHz */
#define EXT_CSD_DEVICE_TYPE_MASK          0x3F     /* Mask out reserved bits */

#define EXT_CSD_DEVICE_TYPE_DDR_52          (EXT_CSD_DEVICE_TYPE_DDR_18V | \
                                             EXT_CSD_DEVICE_TYPE_DDR_12V)
#define EXT_CSD_DEVICE_TYPE_SDR_200         (EXT_CSD_DEVICE_TYPE_SDR_HS_18V | \
                                             EXT_CSD_DEVICE_TYPE_SDR_HS_12V)

#define EXT_CSD_DEVICE_TYPE_SDR_ALL         (EXT_CSD_DEVICE_TYPE_SDR_200 | \
                                             EXT_CSD_DEVICE_TYPE_52 | \
                                             EXT_CSD_DEVICE_TYPE_26)

#define EXT_CSD_DEVICE_TYPE_SDR12V_ALL      (EXT_CSD_DEVICE_TYPE_SDR_HS_12V | \
                                             EXT_CSD_DEVICE_TYPE_52 | \
                                             EXT_CSD_DEVICE_TYPE_26)

#define EXT_CSD_DEVICE_TYPE_SDR18V_ALL      (EXT_CSD_DEVICE_TYPE_SDR_HS_18V | \
                                             EXT_CSD_DEVICE_TYPE_52 | \
                                             EXT_CSD_DEVICE_TYPE_26)

#define EXT_CSD_DEVICE_TYPE_SDR12V_DDR18V   (EXT_CSD_DEVICE_TYPE_SDR_HS_12V | \
                                             EXT_CSD_DEVICE_TYPE_DDR_18V | \
                                             EXT_CSD_DEVICE_TYPE_52 | \
                                             EXT_CSD_DEVICE_TYPE_26)

#define EXT_CSD_DEVICE_TYPE_SDR18V_DDR18V   (EXT_CSD_DEVICE_TYPE_SDR_HS_18V | \
                                             EXT_CSD_DEVICE_TYPE_DDR_18V | \
                                             EXT_CSD_DEVICE_TYPE_52 | \
                                             EXT_CSD_DEVICE_TYPE_26)

#define EXT_CSD_DEVICE_TYPE_SDR12V_DDR12V   (EXT_CSD_DEVICE_TYPE_SDR_HS_12V | \
                                             EXT_CSD_DEVICE_TYPE_DDR_12V | \
                                             EXT_CSD_DEVICE_TYPE_52 | \
                                             EXT_CSD_DEVICE_TYPE_26)

#define EXT_CSD_DEVICE_TYPE_SDR18V_DDR12V   (EXT_CSD_DEVICE_TYPE_SDR_HS_18V | \
                                             EXT_CSD_DEVICE_TYPE_DDR_12V | \
                                             EXT_CSD_DEVICE_TYPE_52 | \
                                             EXT_CSD_DEVICE_TYPE_26)

#define EXT_CSD_DEVICE_TYPE_SDR12V_DDR_52   (EXT_CSD_DEVICE_TYPE_SDR_HS_12V | \
                                             EXT_CSD_DEVICE_TYPE_DDR_52 | \
                                             EXT_CSD_DEVICE_TYPE_52 | \
                                             EXT_CSD_DEVICE_TYPE_26)

#define EXT_CSD_DEVICE_TYPE_SDR18V_DDR_52   (EXT_CSD_DEVICE_TYPE_SDR_HS_18V | \
                                             EXT_CSD_DEVICE_TYPE_DDR_52 | \
                                             EXT_CSD_DEVICE_TYPE_52 | \
                                             EXT_CSD_DEVICE_TYPE_26)

#define EXT_CSD_DEVICE_TYPE_SDR_ALL_DDR18V  (EXT_CSD_DEVICE_TYPE_SDR_200 | \
                                             EXT_CSD_DEVICE_TYPE_DDR_18V | \
                                             EXT_CSD_DEVICE_TYPE_52 | \
                                             EXT_CSD_DEVICE_TYPE_26)

#define EXT_CSD_DEVICE_TYPE_SDR_ALL_DDR_12V (EXT_CSD_DEVICE_TYPE_SDR_200 | \
                                             EXT_CSD_DEVICE_TYPE_DDR_12V | \
                                             EXT_CSD_DEVICE_TYPE_52 | \
                                             EXT_CSD_DEVICE_TYPE_26)

#define EXT_CSD_DEVICE_TYPE_SDR_ALL_DDR_52  (EXT_CSD_DEVICE_TYPE_SDR_200 | \
                                             EXT_CSD_DEVICE_TYPE_DDR_52 | \
                                             EXT_CSD_DEVICE_TYPE_52 | \
                                             EXT_CSD_DEVICE_TYPE_26)

#define MMC_DEVICE_VOLTAGE_30V        (1 << 0)
#define MMC_DEVICE_VOLTAGE_12V        (1 << 1)
#define MMC_DEVICE_VOLTAGE_18V        (1 << 2)

#define MMC_EXT_CSD_VERSION_0         0
#define MMC_EXT_CSD_VERSION_1         1
#define MMC_EXT_CSD_VERSION_2         2
#define MMC_EXT_CSD_VERSION_3         3
#define MMC_EXT_CSD_VERSION_4         4
#define MMC_EXT_CSD_VERSION_5         5
#define MMC_EXT_CSD_VERSION_6         6

#define MMC_EXT_CSD_CACHE_EN          1

/* EXT_CSD bit define */

typedef struct mmcExtCsd
    {
    UINT8       rev;
    UINT8       eraseGroupDef;
    UINT8       secFeatureSupport;
    UINT8       relSectors;
    UINT8       relSet;
    UINT8       relParam;
    UINT8       partConfig;
    UINT8       cacheCtrl;
    UINT8       rstFunction;
    UINT8       eraseTimeoutMult;
    UINT8       outIntTimeout;
    UINT32      partTime;           /* Units: ms */
    UINT32      saTimeout;          /* Units: 100ns */
    UINT32      genericCmd6Time;    /* Units: 10ms */
    UINT32      powerOffLongtime;   /* Units: ms */
    UINT32      hsMaxDtr;
    UINT32      sectors;
    UINT32      deviceType;
    UINT32      hcEraseSize;        /* In sectors */
    UINT32      hcEraseTimeout;     /* In milliseconds */
    UINT32      secTrimMult;        /* Secure trim multiplier  */
    UINT32      secEraseMult;       /* Secure erase multiplier */
    UINT32      trimTimeout;        /* In milliseconds */
    BOOL        enhancedAreaEnable; /* enable bit */
    UINT64      enhancedAreaOffset; /* Units: Byte */
    UINT32      enhancedAreaSize;   /* Units: KB */
    UINT32      cacheSize;          /* Units: KB */
    BOOL        hpiEnable;          /* HPI enablebit */
    BOOL        hpiSupport;         /* HPI support bit */
    UINT32      hpiCmdSet;          /* cmd used as HPI */
    UINT32      dataSectorSize;     /* 512 bytes or 4KB */
    UINT32      dataTagSize;        /* DATA TAG UNIT size */
    UINT32      bootLock;
    BOOL        bootLockEnable;
    UINT32      fetureSupport;
    }MMC_EXT_CSD;

/* SD/MMC card */

typedef struct mmcCardCtrl
    {
    BLK_XBD_DEV     xbdDev;             /* XBD block device */
    VXB_DEVICE_ID   pInst;              /* VXB_DEVICE_ID pointer */
    void *          pInfo;              /* detailed information */
    UINT16          voltage;            /* voltage value */
    BOOL            attached;           /* attached to host */
    BOOL            isWp;               /* write protected */
    BOOL            isExendCsd;         /* if extend csd */
    BOOL            isBlockDevice;      /* if block device */ 
    BOOL            highCapacity;       /* high capacity > 2GB */
    BOOL            highSpeed;          /* 50 MHz */
    BOOL            hasDsr;             /* DSR implemented */
    UINT32          idx;                /* card index */
    UINT32          tranSpeed;          /* transfer speed */
    UINT32          sectorSize;         /* sector size */
    UINT32          readBlkLen;         /* read block length */
    UINT32          writeBlkLen;        /* write block length */
    UINT32          eraseSize;          /* erase size */
    UINT64          blkNum;             /* number of blocks */
    UINT64          capacity;           /* user data capacity */
    MMC_EXT_CSD     extCsdInfo;         /* ext CSD info */
    } MMC_CARD_CTRL;

typedef struct mmcInfo
    {
    UINT32          sdSpec;             /* SD spec version */
    UINT32          sdSec;              /* SD security verison */
    UINT8           busWidth;           /* bus width */
    UINT32          cardSts;            /* card status */
    MMC_CID         cid;                /* CID register */
    SDMMC_CSD       csd;                /* CSD register */
    UINT8           extCsd[512];
    UINT16          dsrValue;
    } MMC_INFO;

#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /* __INCvxbMmcStorageh */
