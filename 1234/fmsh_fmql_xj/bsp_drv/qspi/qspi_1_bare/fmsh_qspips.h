#ifndef _FMSH_QSPIPS_H_	/* prevent circular inclusions */ 
#define _FMSH_QSPIPS_H_	/* by using protection macros */

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned char	 uint8_t;
typedef unsigned short	 u16;
typedef unsigned int     uint32_t;

typedef uint8_t  u8;     /* usigned 8-bit  integer   */
typedef uint16_t u16;   /* usigned 16-bit  integer   */
typedef UINT32   u32;    /* usigned 32-bit  integer   */
typedef uint64_t u64;   /* usigned 64-bit  integer   */
typedef int8_t   s8;     /* signed 8-bit  integer   */
typedef int16_t  s16;    /* signed 16-bit  integer   */
typedef int32_t  s32;    /* signed 32-bit  integer   */
typedef int64_t  s64;    /* signed 64-bit  integer   */
typedef float    f32;	 /* 32-bit floating point */
typedef double   f64;    /* 64-bit double precision FP */
typedef char     char8; 
typedef int      sint32;

/***************************** Include Files *********************************/ 



/************************** Constant Definitions *****************************/   

#ifndef FPAR_QSPIPS_0_DEVICE_ID    
    #define FPS_QSPI0_DEVICE_ID          (0)   
#else
    #define FPS_QSPI0_DEVICE_ID          FPAR_QSPIPS_0_DEVICE_ID
#endif

#ifndef  FPS_QSPI0_BASEADDR  
    #define FPS_QSPI0_BASEADDR           (0xe0000000)
#endif
    
#ifndef  FPS_QSPI0_D_BASEADDR  
    #define FPS_QSPI0_D_BASEADDR         (0xe8000000)
#endif
    
#ifndef FPAR_QSPIPS_1_DEVICE_ID    
    #define FPS_QSPI1_DEVICE_ID          (1)        
#else
    #define FPS_QSPI1_DEVICE_ID          FPAR_QSPIPS_1_DEVICE_ID  
#endif

#ifndef  FPS_QSPI1_BASEADDR  
    #define FPS_QSPI1_BASEADDR           (0xe0020000)
#endif
    
#ifndef  FPS_QSPI1_D_BASEADDR  
    #define FPS_QSPI1_D_BASEADDR         (0xe9000000)
#endif
    
#define FPAR_QSPIPS_NUM_INSTANCES               (2) 

/**************************** Type Definitions *******************************/  
    
typedef void (*FQspiPs_StatusHandler)(void *callBackRef, 
                                     u32 statusEvent, u32 byteCount);

typedef struct {
  u16 deviceId;		                /**< Unique ID  of device */
  u32 dataBaseAddress;              /**< Data Base address of the device */
  u32 baseAddress;                  /**< Config Base address of the device */
} FQspiPs_Config_T;

typedef struct FQspiPs_Tag{
  FQspiPs_Config_T config;          /**< Configuration structure */ 
  
    
  u32 flag;
  u8 maker;                         /**< Flash maker */
  u32 devSize;                      /**< Flash device size in bytes */
  u32 sectorSize;                   /**< Flash sector size in bytes */ 
  
  int isBusy;             
  
  u8 *sendBufferPtr;	            /**< Buffer to send (state) */
  u8 *recvBufferPtr;	            /**< Buffer to receive (state) */
  int requestedBytes;	            /**< Number of bytes to transfer (state) */
  int remainingBytes;	            /**< Number of bytes left to transfer(state) */
  
  FQspiPs_StatusHandler statusHandler;
  void* statusRef;
} FQspiPs_T;
           
/***************** Macros (Inline Functions) Definitions *********************/  
        
/************************** Variable Definitions *****************************/

/************************** Function Prototypes ******************************/

FQspiPs_Config_T* FQspiPs_LookupConfig(u16 deviceId);
int FQspiPs_CfgInitialize(FQspiPs_T* qspi, FQspiPs_Config_T* configPtr);
void FQspiPs_Reset(FQspiPs_T* qspi);
void FQspiPs_SetStatusHandler(FQspiPs_T* qspi, void* callBackRef, 
                              FQspiPs_StatusHandler funcPtr);
int FQspiPs_SelfTest(FQspiPs_T* qspi);
int FQspiPs_SetFlashReadMode(FQspiPs_T* qspi, u8 cmd);
void FQspiPs_InterruptHandler(void* instancePtr);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif	/* prevent circular inclusions */
