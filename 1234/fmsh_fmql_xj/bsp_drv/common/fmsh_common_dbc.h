/******************************************************************************
*
* Copyright (C) 2018 - 2028 FMSH, Inc.  All rights reserved.
*
******************************************************************************/
/*****************************************************************************/
/**
* @file  fmsh_common_dbc.h
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
#ifndef _FMSH_COMMON_DBC_H_
#define _FMSH_COMMON_DBC_H_

#ifdef __cplusplus
extern "C" {    /* allow C++ to use these headers */
#endif

/***************************** Include Files *********************************/

/************************** Constant Definitions *****************************/

#define DEBUG_OUT   1

/**************************** Type Definitions *******************************/

/* FMSH_NASSERT macro disables all contract validations */
/* (assertions, preconditions, postconditions, and invariants). */
#ifndef FMSH_NASSERT 

/* callback invoked in case of assertion failure */
extern void onAssert__(char const *file, unsigned line);

#define FMSH_ASSERT(test_)   \
        if (test_) {    \
        }               \
        else onAssert__(__FILE__, __LINE__)

#define FMSH_REQUIRE(test_)           FMSH_ASSERT(test_)
#define FMSH_ENSURE(test_)            FMSH_ASSERT(test_)
#define FMSH_INVARIANT(test_)         FMSH_ASSERT(test_)
#define FMSH_ALLEGE(test_)            FMSH_ASSERT(test_)

#else   /* FMSH_NASSERT */
#define FMSH_ASSERT(test_)
#define FMSH_REQUIRE(test_)
#define FMSH_ENSURE(test_)
#define FMSH_INVARIANT(test_)
#define FMSH_ALLEGE(test_)   \
   if(test_) {          \
   }                    \
   else

#endif  /* FMSH_NASSERT */


#define PRINTF(...)         \
	do {						\
		printf(__VA_ARGS__);	\
	} while(0)

#define TRACE_OUT(flag, ...)      \
	do {						   \
		if(flag) {					   \
		printf(__VA_ARGS__);   \
		}						   \
	} while(0)

/***************** Macros (Inline Functions) Definitions *********************/

/************************** Function Prototypes ******************************/

#ifdef __cplusplus
}
#endif

#endif /* #ifndef _FMSH_COMMON_DBC_H_ */

