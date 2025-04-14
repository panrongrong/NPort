/* usbSynopsysHcdDataStructures.h - data structures for Synopsys HCD */

/*
 * Copyright (c) 2009-2011, 2013, 2014 Wind River Systems, Inc.
 *
 * The right to copy, distribute, modify or otherwise make use
 * of this software may be licensed only pursuant to the terms
 * of an applicable Wind River license agreement.
 */

/*
modification history
-------------------
01i,09sep14,wyy  unlink request before callback (VXW6-83137)
01h,25jul13,ljg  add dmaChannelMutex to protect uIdleDmaChannelMap (WIND00427761)
01g,10jul13,ljg  add alt_soc_gen5 support
01f,02sep11,m_y  using lstLib replace the old list structure add polling mode
                 definitions (WIND00298534)
01e,23aug11,m_y  modify usb_synopsyshcd_pipe structure (WIND00289879)
01d,06jan11,m_y  remove two members uHCCHARReg and uHCTSIZReg from the structure 
                 usb_synopsyshcd_data (WIND00247987)
01c,08sep10,m_y  modify to remove build warning
01b,17mar10,m_y  add two members into structure usb_synopsyshcd_data to record
                 HCTSIZ and HCCHAR register in ISR.
01a,06nov09,m_y  written.
*/

/*
DESCRIPTION

This file contains the data structures used by the Synopsys Dual-Role USB
host controller driver.

*/

#ifndef __INCSynopsysHcdDataStructuresh
#define __INCSynopsysHcdDataStructuresh

#ifdef    __cplusplus
extern "C" {
#endif

#include <vxWorks.h>
#include <semLib.h>
#include <lstLib.h>
#include <hwif/vxbus/vxBus.h>
#include <usb/usbOsal.h>
#include <usb/usbHst.h>
#include <usb/usbHcdInstr.h>

/* 
 * If user want to use polling mode to handle the interrupt, 
 * make this macro active default is disable
 * #define USB_SHCD_POLLING_MODE
 */
#undef USB_SHCD_POLLING_MODE  /* origin */
/*#define USB_SHCD_POLLING_MODE  // jc*/

#define USB_SHCD_POLLING_THREAD_PRIORITY            (37)
#define USB_SHCD_POLLING_INTERVAL                   (4)

/* Magic code for Synopsys Dual-Role USB host controller driver */

#define USB_SYNOPSYSHCD_MAGIC_ALIVE                 (0x4F54291A)   /* 0x4F544F54  */
#define USB_SYNOPSYSHCD_MAGIC_DEAD                  (0xDEAD4F54)

#define USB_SYNOPSYSHCD_MAX_DEVICE_ADDRESS          (127)
#define USB_SYNOPSYSHCD_INT_THREAD_PRIORITY         (100)

#define USB_SYNOPSYSHCD_DIR_OUT                     (0x00)
#define USB_SYNOPSYSHCD_DIR_IN                      (0x80)

#define USB_SYNOPSYSHCD_HIGH_SPEED                  (0x00)
#define USB_SYNOPSYSHCD_FULL_SPEED                  (0x01)
#define USB_SYNOPSYSHCD_LOW_SPEED                   (0x02)

#define USB_SYNOPSYSHCD_DEV_MODE                    (0x00)
#define USB_SYNOPSYSHCD_HOST_MODE                   (0x01)

#define USB_SYNOPSYSHCD_FULL_FRAME_NUM_MAX          (0x800)
#define USB_SYNOPSYSHCD_FRAME_NUM_MAX               (0x4000)

#define USB_SYNOPSYSHCD_MAX_CHANNELS                (0x20) /* extend */
#define USB_SHDRC_DMA_INVALID_CHANNEL               (0xFF)


#define USB_SYNOPSYSHCD_RH_PORT_STATUS_SIZE         (4)    /* Port Status Size */

#define USB_SYNOPSYSHCD_RH_PORT_CHANGE_MASK         (0x02)
#define USB_SYNOPSYSHCD_ENDPOINT_NUMBER_MASK        (0x0F) /* Mask for EP num */

#define USB_SYNOPSYSHCD_HUBSTATUS_SIZE              (4)
#define USB_SYNOPSYSHCD_ENDPOINT_TYPE_MASK          (0x03)
#define USB_SYNOPSYSHCD_ENDPOINT_MAX_PACKET_SIZE_MASK   (0x7FF)

#define USB_SYNOPSYSHCI_PLATFORM_ALTERA_SOC_GEN5    (0x1)

/* Mask value to retrieve the number of transactions in a microframe */

#define USB_SYNOPSYSHCD_ENDPOINT_NUMBER_OF_TRANSACTIONS_MASK   (0x1800)

/* MSGQ count */

#define USB_SYNOPSYSHCD_MSGQ_COUNT                  (50)

/* Macros used for converting between CPU memory and bus specific memory */

/*
 * Macro definition to convet a Cpu address to a Bus address. If no converisons
 * are required, we return the address itelf, else call the function to do
 * required conversion.
 */

#define USB_SYNOPSYSHCD_CONVERT_TO_BUS_MEM(INDEX, ADDRESS)                     \
    (((ADDRESS == 0)|| ((g_pSynopsysHCDData[INDEX]->pCpuToBus) == NULL)) ?     \
    (ULONG)ADDRESS: (*(g_pSynopsysHCDData[INDEX]->pCpuToBus))((pVOID)ADDRESS))

/*
 * Macro definition to convet a Bus address to a Cpu address. If no converisons
 * are required, we return the address itelf, else call the function to do
 * required conversion.
 */

#define USB_SYNOPSYSHCD_CONVERT_FROM_BUS_MEM(INDEX, ADDRESS)                   \
    (((ADDRESS == 0) || ((g_pSynopsysHCDData[INDEX]->pBusToCpu) == NULL)) ?    \
    (pVOID)ADDRESS : (*(g_pSynopsysHCDData[INDEX]->pBusToCpu))((ULONG)ADDRESS))

/* Macro used for swapping the 32 bit values of HC data strcutures */

#define USB_SYNOPSYSHCD_SWAP_DESC_DATA(INDEX,VALUE)                            \
                 ((((g_pSynopsysHCDData[INDEX])->pDescSwap) == NULL) ? VALUE : \
                             (*(g_pSynopsysHCDData[INDEX]->pDescSwap))(VALUE))

/* Macro used for swapping the 32 bit values of USB formated data */

#define USB_SYNOPSYSHCD_SWAP_USB_DATA(INDEX,VALUE)                             \
                 ((((g_pSynopsysHCDData[INDEX])->pUsbSwap) == NULL) ? VALUE :  \
                             (*(g_pSynopsysHCDData[INDEX]->pUsbSwap))(VALUE))

/* Flags for PIPE */

typedef enum
    {
    USB_SYNOPSYSHCD_PIPE_FLAG_OPEN = (0x1 << 0),
    USB_SYNOPSYSHCD_PIPE_FLAG_SCHEDULED = (0x1 << 1),
    USB_SYNOPSYSHCD_PIPE_FLAG_NEED_PING = (0x1 << 2),
    USB_SYNOPSYSHCD_PIPE_FLAG_DELETE = (0x1 << 3),
    USB_SYNOPSYSHCD_PIPE_FLAG_MODIFY_DEFAULT_PIPE = (0x1 << 4),
    USB_SYNOPSYSHCD_PIPE_FLAG_NEED_ZLP = (0x1 << 5),
    USB_SYNOPSYSHCD_PIPE_FLAG_NEED_PROCESS_ZLP = (0x1 << 6)
    }USB_SYNOPSYSHCD_PIPE_FLAG;

/* Flags for request info */

#define USB_SYNOPSYSHCD_REQUEST_TO_BE_CANCELED            (0x1 << 0)
#define USB_SYNOPSYSHCD_REQUEST_STILL_TRANSFERING         (0x1 << 1)

/* Flags for transfer stage */

typedef enum
    {
    USB_SYNOPSYSHCD_REQUEST_STAGE_NON_CONTROL = 0,
    USB_SYNOPSYSHCD_REQUEST_STAGE_NON_CONTROL_SPLIT_COMPLETE,
    USB_SYNOPSYSHCD_REQUEST_STAGE_SETUP,
    USB_SYNOPSYSHCD_REQUEST_STAGE_SETUP_SPLIT_COMPLETE,
    USB_SYNOPSYSHCD_REQUEST_STAGE_DATA,
    USB_SYNOPSYSHCD_REQUEST_STAGE_DATA_SPLIT_COMPLETE,
    USB_SYNOPSYSHCD_REQUEST_STAGE_STATUS,
    USB_SYNOPSYSHCD_REQUEST_STAGE_STATUS_SPLIT_COMPLETE
    }USB_SYNOPSYSHCD_REQUEST_FLAG;

/* Command for the transfer task */

typedef enum
    {
    USB_SYNOPSYSHCD_TRANSFER_CMD_START = (0x1 << 0),
    USB_SYNOPSYSHCD_TRANSFER_CMD_COMPLITE = (0x1 << 1)
    }USB_SYNOPSYSHCD_TRANSFER_CMD;

struct usb_synopsyshcd_request_info;
struct usb_synopsyshcd_data;

typedef struct usb_synopsyshcd_pipe
    {
    NODE         pipeNode;
    LIST         reqList;
    OS_EVENT_ID  listEventID;

    LIST         reqFreeList;
    OS_EVENT_ID  reqFreeListEventID;

    UINT16 uMicroFrameNumberLast; /* The last micro frame number the pipe
                                   * was scheduled
                                   */

    UINT16 uMicroFrameNumberNext; /* The next micro frame number the pipe
                                   * will be scheduled
                                   */

    UINT16 uFullFrameNumberLast;  /* The last full frame number the pipe
                                   * was scheduled
                                   */

    UINT16 uFullFrameNumberNext;  /* The next full frame number the pipe
                                   * will be scheduled
                                   */

    UINT16 uMaximumPacketSize;    /* To hold the maximum packet size */

    UINT16 uMultiTransPerMicroFrame;    /* To hold the maximum transfer number
                                         * in one micro frame
                                         */

    /*
     * Nearest high speed hub and port number info; used for split
     * transfer to full/low speed devices. High byte holds high speed
     * parent hub address, low byte is port number to which the device
     * is connected to.
     */

    UINT16 uHubInfo; /* This holds any additional details for an endpoint */

    UINT8  uEndpointAddress;    /* Address of the endpoint */
    UINT8  uDeviceAddress;      /* Address of the device holding the endpoint */
    UINT8  uSpeed;              /* Speed of the device */
    UINT8  uEndpointType;       /* Type of endpoint */
    UINT8  uEndpointDir;        /* Direction of the endpoint */
    UINT8  uPidToggle;          /* Current toggle for this pipe */
    UINT8  uDmaChannel;         /* Hardware DMA channel for this pipe */
    UINT8  uPipeFlag;           /* Flag indicating whether the endpoint
                                 * is being deleted.
                                 */
    UINT8  bInterval;          /* The interval between packets in cycles */                        
    UINT8  uHalted;            /* Halt times for this pipe */       
    UINT32 uHaltChannel;       /* Channel halted for this pipe */
    }USB_SYNOPSYSHCD_PIPE,*pUSB_SYNOPSYSHCD_PIPE;


/* Data structure maintained by HCD to hold the request information */

typedef struct usb_synopsyshcd_request_info
    {
    NODE                                  schedNode; /* Node for schedule list */                                                         
    NODE                                  listNode;  /* Node for pipe list */
    pUSBHST_URB                           pUrb;      /* Pointer to the URB */
    pUSB_SYNOPSYSHCD_PIPE                 pHCDPipe;     /* Pointer to the
                                                         * USB_SYNOPSYSHCD_PIPE
                                                         */

    struct usb_synopsyshcd_data *         pHCDData;     /* pointer to the
                                                         * USB_SYNOPSYSHCD_DATA
                                                         */

    UINT32                                uActLength;   /* Actual length of the
                                                         * transfer
                                                         */

    UINT32                                uXferSize;    /* Bytes size we expect
                                                         * to transfer
                                                         */

    UINT32                                uPktcnt;      /* Packet count for
                                                         * current transfer
                                                         */
    UINT32                                uUrbTotalPacketCount;  /* Total packet
                                                                  * count of URB
                                                                  */
    UINT8                                 uTransferFlag; /* Flag for current
                                                          * transfer
                                                          */
    UINT8                                 uStage;        /* Transfer Stage */

    UINT8                                 uRetried;      /* Retries for
                                                          * current transfer
                                                          */
    }USB_SYNOPSYSHCD_REQUEST_INFO, *pUSB_SYNOPSYSHCD_REQUEST_INFO;

/* Macro to convert schedNode to USB_SYNOPSYSHCD_REQUEST_INFO */

#define SCHED_NODE_TO_USB_SYNOPSYSHCD_REQUEST_INFO(pNode)       \
     ((USB_SYNOPSYSHCD_REQUEST_INFO *) ((char *) (pNode) -      \
                            OFFSET(USB_SYNOPSYSHCD_REQUEST_INFO, schedNode)))
    
#define LIST_NODE_TO_USB_SYNOPSYSHCD_REQUEST_INFO(pNode)       \
         ((USB_SYNOPSYSHCD_REQUEST_INFO *) ((char *) (pNode) -      \
                                OFFSET(USB_SYNOPSYSHCD_REQUEST_INFO, listNode)))

/* This data structure holds the details of the root hub */

typedef struct _USB_SYNOPSYSHCD_RH_DATA
    {
    UCHAR *      pPortStatus;   /* Pointer to an array of port status
                                 * information
                                 */

    UCHAR        HubStatus[USB_SYNOPSYSHCD_HUBSTATUS_SIZE]; /* Buffer holding
                                                             * the hub status
                                                             */
    UINT8        uNumPorts;   /* Number of downstream ports supported
                               * by the Root hub.
                               */

    UCHAR *      pHubInterruptData;     /* Buffer holding the data to be
                                         * returned on an interrupt request.
                                         */

    UINT32       uSizeInterruptData;    /* To hold the size of
                                         * the interrupt data
                                         */

    pUSB_SYNOPSYSHCD_PIPE  pControlPipe;   /* Control pipe information */

    pUSB_SYNOPSYSHCD_PIPE  pInterruptPipe; /* Pointer to the USB_SYNOPSYSHCD_PIPE
                                            * data structure for the interrupt
                                            * endpoint.
                                            */

    UINT8        bRemoteWakeupEnabled;  /* Flag indicating whether Remote Wakeup
                                         * is enabled or not
                                         */

    UINT8        bInterruptEndpointHalted;  /* Flag indicating whether the
                                             * interrupt endpoint is halted or not.
                                             */

    UINT8        uDeviceAddress;    /* Address of the Root hub. */

    UINT8        uConfigValue;      /* Value of the configuration which is set. */
    }USB_SYNOPSYSHCD_RH_DATA, *pUSB_SYNOPSYSHCD_RH_DATA;

/* This is the data structure maintained for every Synopsys host controller. */

typedef struct usb_synopsyshcd_data
    {
    UINT32              uBusIndex;
    OS_EVENT_ID         dmaChannelMutex;
    UINT32              uIdleDmaChannelMap;   /* Map for current dma channel
                                               * Bit[n] == 1 means channel[n] is idle
                                               */

    UINT8               addressMode64;       /* Indicate the HC's address
                                              * mode is 64 bit or not
                                              */

    UINT8               usbNumPorts;         /* Count of usb ports */
    UINT8               hostNumDmaChannels;  /* Count of host channel */
    UINT8               initDone;            /* Flag for initialization */
    UINT32              uPlatformType;       /* Platform Type */
    UINT32              uMicroFrameNumChangeFlag; /* Flag to indicate the micro
                                                   * frame number change or not
                                                   */
    UINT32              uMicroFrameNumStep;  /* Step add to the micro frame
                                              * number it the micro frame number
                                              * doesn't change
                                              */

    BUS_DEVICE_ID       pDev;                /* struct vxbDev */

    OS_EVENT_ID         RequestSynchEventID;  /* Event ID used to sync requests */

    pUSB_SYNOPSYSHCD_PIPE pPipeInChannel[USB_SYNOPSYSHCD_MAX_CHANNELS]; /* pipe map
                                                                         * for channels
                                                                         */

    pUSB_SYNOPSYSHCD_PIPE        pDefaultPipe;  /* Default pipe information */

    LIST        pipeList;           /* Pipe List */
    
    LIST        periodicReqList;    /* List for periodic requests
                                     * waiting for transfer
                                     */

    LIST        periodicReqReadyList;/* List for periodic requests
                                      * already started transfer
                                      */

    LIST        nonPeriodicReqList; /* List for NON-periodic requests
                                     * waiting for transfer
                                     */

    LIST        nonPeriodicReqReadyList;  /* List for NON-periodic requests
                                           * already started transfer
                                           */

    atomic_t    uInterruptStatus;   /* To hold the status of the interrupt */
    atomic_t    uHAINTStatus; /* To hold the status of HAINT register in ISR */
    atomic_t    uHPRTStatus;  /* To hold the status of HPRT register in ISR */
    UINT32      uHCINTnStatus[USB_SYNOPSYSHCD_MAX_CHANNELS];  /* To hold the
                                                               * status of HCINT
                                                               * register in ISR
                                                               */
    UINT32          isrMagic;   /* Magic value for shared interrupts */
    OS_EVENT_ID     interruptEvent;  /* Event indicate that an
                                      * interrupt has occurred
                                      */
    OS_THREAD_ID    intHandlerThread; /* Thread ID of interrupt
                                       * handler thread
                                       */

    MSG_Q_ID        transferThreadMsgQ; /* Pointer to the message queue */
    OS_THREAD_ID    transferThread;   /* Thread ID of the
                                       * transfer handler thread
                                       */

    USB_SYNOPSYSHCD_RH_DATA RHData; /* Data structure holding the
                                     * detail of the root hub
                                     */

#if CPU==MIPSI64R2
    UINT64       regBase;             /* 64-bit base address for register */
#else
    UINT32       regBase;             /* 32-bit base address for register */
#endif

    VOID *       pRegAccessHandle;    /* Handle for the register access methods */
    UINT32       (*pDescSwap)(UINT32 data); /* Function pointer to hold the
                                             * function doing byte conversions
                                             * for HC data structure
                                             */

    UINT32       (*pUsbSwap)(UINT32 data);  /* Function pointer to hold the
                                             * function doing byte conversions
                                             * for USB endian
                                             */

    UINT32       (*pRegSwap)(UINT32 data);  /* Function pointer to hold the
                                             * function doing byte conversions
                                             * for HC register endian
                                             */

    pVOID        (*pBusToCpu)(UINT32 addr); /* Function pointer to hold the
                                             * function doing physical address
                                             * to CPU Address Conversion
                                             */

    UINT32       (*pCpuToBus)(pVOID pAddr); /* Function pointer to hold the
                                             * function doing CPU Memory to
                                             * physical memory conversions
                                             */

#if CPU==MIPSI64R2
    UINT32      (*pRegRead32Handle)(UINT64 addr);   /* Function pointer to read
                                                     * 32-bit content from
                                                     * register
                                                     */

    VOID        (*pRegWrite32Handle)(UINT64 addr, UINT32 value); /* Function pointer
                                                                  * to write 32-bit content
                                                                  * to 64-bit register
                                                                  */
#else
    UINT32      (*pRegRead32Handle)(UINT32 addr);   /* Function pointer to read
                                                     * 32-bit content from
                                                     * register
                                                     */

    VOID        (*pRegWrite32Handle)(UINT32 addr, UINT32 value); /* Function pointer
                                                                  * to write 32-bit content
                                                                  * to 32-bit register
                                                                  */
#endif

    UINT64      (*pRegRead64Uint64Handle)(UINT64 addr);   /* Function pointer to read
                                                           * 64-bit content from 64-bit
                                                           * register
                                                           */
    void        (*pRegWrite64Uint64Handle)(UINT64 addr, UINT64 value); /* Function pointer
                                                                        * to write 64-bit
                                                                        * content to
                                                                        * 64-bit register
                                                                        */
    FUNCPTR      pPostResetHook;   /* Function pointer to hold the
                                    * function doing post reset operation
                                    */

    }USB_SYNOPSYSHCD_DATA, *pUSB_SYNOPSYSHCD_DATA;

/* Data structure for transfer task */

typedef struct _USB_SYNOPSYSHCD_TRANSFER_TASK_INFO_
    {
    pUSB_SYNOPSYSHCD_DATA          pHCDData;     /* Pointer to the
                                                  * USB_SYNOPSYSHCD_DATA
                                                  */

    pUSB_SYNOPSYSHCD_PIPE          pHCDPipe;     /* Pointer to the
                                                  * USB_SYNOPSYSHCD_PIPE
                                                  */

    pUSB_SYNOPSYSHCD_REQUEST_INFO  pRequestInfo; /* Pointer to the
                                                  * USB_SYNOPSYSHCD_REQUEST_INFO
                                                  */

    UINT32                         uDmaChannel;  /* Current Dma channel
                                                  * for transfer
                                                  */

    UINT32                         uCmdCode;     /* Command code */

    USBHST_STATUS                  uCompleteStatus; /* Complete state */
    } USB_SYNOPSYSHCD_TRANSFER_TASK_INFO;

#ifdef    __cplusplus
}
#endif

#endif /* __INCSynopsysHcdDataStructuresh */

/* End of file */
