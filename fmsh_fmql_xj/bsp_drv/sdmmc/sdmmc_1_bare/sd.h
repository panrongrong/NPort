/******************************************************************************
*
* Copyright (C) 2018 - 2028 FMSH, Inc.  All rights reserved.
*
******************************************************************************/
/******************************************************************************
*
* @file  sd.h
*
* This file contains
*
* @note		None.
*
* MODIFICATION HISTORY:
*
*<pre>
* Ver   Who  Date     Changes
* ----- ---- -------- ---------------------------------------------
* 0.01   yl  12/20/2018  First Release
*</pre>
******************************************************************************/
#ifndef _SD_H_
#define _SD_H_

/***************************** Include Files *********************************/

/*#include "fmsh_common_types.h"*/

#ifdef __cplusplus
extern "C" {
#endif

/************************** Constant Definitions *****************************/

/**************************** Type Definitions *******************************/

/***************** Macros (Inline Functions) Definitions *********************/

/************************** Function Prototypes ******************************/

/************************** Function Prototypes ******************************/

u32 InitSD(const char *);
u32 SDAccess( u32 SourceAddress, u32 DestinationAddress, u32 LengthWords);
void ReleaseSD(void);
u32 SDWriteAccess(u32 SourceAddress, u32 LengthBytes, const char *filename);

#ifdef __cplusplus
}
#endif


#endif /* #ifndef _SD_H_ */

