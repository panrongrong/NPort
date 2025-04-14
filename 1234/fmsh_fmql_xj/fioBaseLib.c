/* fioBaseLib.c - formatted I/O library */

/*
 * Copyright (c) 2005-2010, 2013 Wind River Systems, Inc.
 *
 * The right to copy, distribute, modify or otherwise make use
 * of this software may be licensed only pursuant to the terms
 * of an applicable Wind River license agreement.
 */

/*
modification history
--------------------
02e,04nov13,mcm  Fix for static analysis issue 
02d,25sep10,jxu  Added inclusion of iosLibP.h for _func_consoleOut.
02c,16sep10,mcm  Fixed fioBufPrint prototype
02b,22may10,pch  small footprint
02a,20may10,mze  remove undef of _WRS_CONFIG_FORMATTED_OUT_BASIC
01z,21apr10,jpb  Added excLibP.h for _func_excJobAdd.
01y,16apr10,mcm  Fixing crash observed while printf floating point arguments
                 for LP64 (WIND00200149)
01x,15sep09,mcm  Fixing up printExc's arguments, also updated type of variable
		 holding strlen's return value
01w,01jul09,jmp  Moved _func_shellExcPrint definition to shellLibP.h.
01v,27mar09,mcm  Updates for the LP64 data model
01u,27mar09,zl   updated job deferral API
01v,04sep09,s_s  Fix for defect WIND179235
01u,09feb09,yjl  fix defect WIND00153003 fdprintf(fd,"string %n",0) exception
01t,04sep08,gls  renamed _WRS_CONFIG_FORMATTED_IO to _WRS_CONFIG_FORMATTED_OUT
01s,27aug08,jpb  Renamed VSB header file
01r,18jun08,jpb  Renamed _WRS_VX_SMP to _WRS_CONFIG_SMP.  Added include path
		 for kernel configurations options set in vsb.
01q,12may08,jpb  Modifications for source build.  Renamed
		 INCLUDE_FORMATTED_OUT_BASIC to _WRS_CONFIG_FORMATTED_OUT_BASIC,
		 _WRS_VX_SMP to _WRS_CONFIG_SMP.
01p,08mar07,pch  adjust for generic ED&R locking
01o,26nov06,nld  Documentation update for spinlock restricted APIs
06y,17oct06,pch  SMP support
06x,27sep06,pcs  Update macro KERNEL_CONTEXT to KERNEL_ENTERED_ME
06w,07apr06,dgp  doc: fix prog gd ref, library; synopsis, snprintf()
06v,29sep05,zl   include vmLibP.h
06u,26sep05,kk   moved fioFlt*Rtn routines to funcBind, make FORMATTED_IO
		 and FORMATTED_OUT_BASIC mutually exclusive.
06t,12sep05,yvp  Moved fioFlt* out of #ifdef.
06s,05sep05,dbt  Make sure jobExcAdd() is not called when in kernel context
		 (SPR #111131).
06r,29aug05,rhe  SPR 111376 Replaced MACRO FIO_SCALED_DOWN with
		 INCLUDE_FORMATTED_OUT_BASIC
06q,20jul05,rhe  New File, extracted from fioLib.c of 23jun05 for scalability
*/

/*
DESCRIPTION
This library provides the basic formatting and scanning I/O functions.  It
includes some routines from the ANSI-compliant printf()/scanf()
family of routines.  It also includes several utility routines.

If the floating-point format specifications `e', `E', `f', `g', and `G' are
to be used with these routines, the routine floatInit() must be called
first.  If the configuration macro INCLUDE_FLOATING_POINT is defined,
floatInit() is called by the root task, usrRoot(), in usrConfig.c.

These routines do not use the buffered I/O facilities provided by the
standard I/O facility.  Thus, they can be invoked even if the standard I/O
package has not been included.  This includes printf(), which in most UNIX
systems is part of the buffered standard I/O facilities.  Because printf()
is so commonly used, it has been implemented as an unbuffered I/O function.
This allows minimal formatted I/O to be achieved without the overhead of
the entire standard I/O package.  For more information, see the manual
entry for ansiStdio.

SMP CONSIDERATIONS
Some or all of the APIs in this module are spinlock and intCpulock
restricted.  Spinlock restricted APIs are the ones where it is an error
condition for the caller to acquire any spinlock and then attempt to
call these APIs.  APIs that are intCpuLock restricted are the ones where
it is an error condition for the caller to have disabled interrupts on
the local CPU (by calling intCpuLock()) and then attempt to call these
APIs. The method by which these error conditions are flagged and the
exact behaviour in these situations are described in the individual API
documentation.

INCLUDE FILES: fioLib.h, stdio.h

SEE ALSO: ansiStdio, floatLib,
\tb "VxWorks Kernel Programmer's Guide: I/O System"
*/

#include <vxWorks.h>
#include <vsbConfig.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <taskLib.h>
#include <fioLib.h>
#include <intLib.h>
#include <sysLib.h>
#include <limits.h>
#include <private/fioLibP.h>
#include <private/floatioP.h>
#include <private/funcBindP.h>
#include <private/excLibP.h>	/* _func_excJobAdd */
#include <private/kwriteLibP.h>	/* _func_kprintf, _func_kwrite */
#include <private/shellLibP.h>
#include <private/kernelLibP.h>
#include <private/vmLibP.h>
#include <private/iosLibP.h>	/* _func_consoleOut */

#ifdef _WRS_ALTIVEC_SUPPORT
# ifdef __GNUC__
#include <altivec.h>
# endif /* __GNUC__ */
#endif /* _WRS_ALTIVEC_SUPPORT */

/*
 * FORMATTED_OUT and FORMATTED_OUT_BASIC are mutually exclusive
 *
 * This is maintained in the VSB configuration file located in
 * target/src/common.vxconfig
 */

/* temp fix for va_list addressing issue (SPR 92721) */

#ifndef _WRS_CONFIG_FORMATTED_OUT_BASIC
# ifdef _WRS_VA_ADDR
#   define va_addr(x) _WRS_VA_ADDR(x)
# else	/* _WRS_VA_ADDR */
#   if defined(__x86_64__)
#      define va_addr(x) (x)
#   else
#   define va_addr(x) (&x)
#   endif /* __x86_64__ */
# endif	/* _WRS_VA_ADDR */


/* Macros for converting digits to letters and vice versa */


#define	BUF		400		/* buffer for %dfg etc */


#define	to_digit(c)	((c) - '0')
#define is_digit(c)	((unsigned)to_digit(c) <= 9)
#define	to_char(n)	((char)(n) + '0')

#define	PAD(howmany, with)					\
    {								\
    if ((n = (howmany)) > 0)					\
	{							\
	while (n > PADSIZE)					\
	    {							\
	    if ((*outRoutine) (with, PADSIZE, outarg) != OK)	\
		return (ERROR);					\
	    n -= PADSIZE;					\
	    }							\
	if ((*outRoutine) (with, n, outarg) != OK)		\
	    return (ERROR);					\
	}							\
    }

/* to extend shorts, signed and unsigned arg extraction methods are needed */
#define	SARG()	((doLongLongInt) ? (long long) va_arg(vaList, long long) : \
		 (doLongInt) ? (long long)(long)va_arg(vaList, long) : \
		 (doShortInt) ? (long long)(short)va_arg(vaList, int) : \
		 (long long)(int) va_arg(vaList, int))

#define	UARG()	((doLongLongInt) ? (unsigned long long) va_arg(vaList, unsigned long long) :	\
	 (doLongInt) ? (unsigned long long)(ulong_t)va_arg(vaList,ulong_t):\
	 (doShortInt) ? (unsigned long long)(ushort_t)va_arg(vaList,int):\
	 (unsigned long long)(uint_t) va_arg(vaList, uint_t))


/* typedefs */

/* globals */

/*
 * The fieldSzIncludeSign indicates whether the sign should be included
 * in the precision of a number.
 */

BOOL fieldSzIncludeSign = TRUE;
#endif /* Not _WRS_CONFIG_FORMATTED_OUT_BASIC */

void (*_func_printExcPrintHook)(long, _Vx_usr_arg_t, _Vx_usr_arg_t, _Vx_usr_arg_t, _Vx_usr_arg_t, _Vx_usr_arg_t);

#if defined(_WRS_CONFIG_SMP)
extern int  edrBufLock   (void);
extern void edrBufUnlock (int oldSR);
#endif	/* _WRS_CONFIG_SMP */

/* locals */
#ifdef _WRS_CONFIG_FORMATTED_OUT_BASIC
LOCAL char tlVal[] = "0123456789abcdef";
# ifdef FIO_HEX_X_SUPPORT
LOCAL char tuVal[] = "0123456789ABCDEF";
# endif	/* FIO_HEX_X_SUPPORT */
#else	/* _WRS_CONFIG_FORMATTED_OUT_BASIC */
/* Choose PADSIZE to trade efficiency vs size.  If larger printf fields occur
 * frequently, increase PADSIZE (and make the initialisers below longer).
 */

LOCAL char blanks[PADSIZE] =
    {
    ' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' '
    };

LOCAL char zeroes[PADSIZE] =
    {
    '0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0'
    };
#endif	/* _WRS_CONFIG_FORMATTED_OUT_BASIC */




/* forwards */
#ifdef _WRS_CONFIG_FORMATTED_OUT_BASIC
LOCAL int fioBasicFormat (FUNCPTR outRoutine, int outarg, unsigned long value,
			  int base, BOOL isNegative, char * pTtanslate);
#endif	/* _WRS_CONFIG_FORMATTED_OUT_BASIC */


/*******************************************************************************
*
* fioBaseLibInit - initialize the formatted I/O support library
*
* This routine initializes the formatted I/O support library.  It should be
* called once in usrRoot() when formatted I/O functions such as printf() and
* scanf() are used.
*
* RETURNS: N/A
*/

void fioBaseLibInit (void)
    {
    _func_printErr = printErr;
    }


/*******************************************************************************
*
* printf - write a formatted string to the standard output stream (ANSI)
*
* This routine writes output to standard output under control of the string
* <fmt>. The string <fmt> contains ordinary characters, which are written
* unchanged, plus conversion specifications, which cause the arguments that
* follow <fmt> to be converted and printed as part of the formatted string.
*
* The number of arguments for the format is arbitrary, but they must
* correspond to the conversion specifications in <fmt>.  If there are
* insufficient arguments, the behavior is undefined.  If the format is
* exhausted while arguments remain, the excess arguments are evaluated but
* otherwise ignored.  The routine returns when the end of the format string
* is encountered.
*
* The format is a multibyte character sequence, beginning and ending in its
* initial shift state.  The format is composed of zero or more directives:
* ordinary multibyte characters (not `%') that are copied unchanged to the
* output stream; and conversion specification, each of which results in
* fetching zero or more subsequent arguments.  Each conversion specification
* is introduced by the `%' character.  After the `%', the following appear in
* sequence:
* .iP "" 4
* Zero or more flags (in any order) that modify the meaning of the
* conversion specification.
* .iP
* An optional minimum field width.  If the converted value has fewer
* characters than the field width, it will be padded with spaces (by
* default) on the left (or right, if the left adjustment flag,
* described later, has been given) to the field width. The field
* width takes the form of an asterisk (`*') (described later) or a decimal
* integer.
* .iP
* An optional precision that gives the minimum number of digits to
* appear for the `d', `i', `o', `u', `x', and `X' conversions, the number of
* digits to appear after the decimal-point character for `e', `E', and `f'
* conversions, the maximum number of significant digits for the `g' and
* `G' conversions, or the maximum number of characters to be written
* from a string in the `s' conversion.  The precision takes the form of a
* period (`.') followed either by an asterisk (`*') (described later) or by
* an optional decimal integer; if only the period is specified, the
* precision is taken as zero.  If a precision appears with any other
* conversion specifier, the behavior is undefined.
* .iP
* An optional `h' specifying that a following `d', `i', `o', `u', `x', and
* `X' conversion specifier applies to a `short int' or `unsigned short int'
* argument (the argument will have been promoted according to the integral
* promotions, and its value converted to `short int' or `unsigned short int'
* before printing); an optional `h' specifying that a following `n'
* conversion specifier applies to a pointer to a `short int' argument.  An
* optional `l' (ell) specifying that a following `d', `i', `o', `u', `x', and
* `X' conversion specifier applies to a `long int' or `unsigned long int'
* argument; or an optional `l' specifying that a following `n' conversion
* specifier applies to a pointer to a `long int' argument.  An optional `ll'
* (ell-ell) specifying that a following `d', `i', `o', `u', `x', and
* `X' conversion specifier applies to a `long long int' or `unsigned long
* long int' argument; or an optional `ll' specifying that a following `n'
* conversion specifier applies to a pointer to a `long long int' argument.
* If a `h', `l' or `ll' appears with any other conversion specifier, the
* behavior is undefined.
* .iP
* \&WARNING: ANSI C also specifies an optional `L' in some of the same
* contexts as `l' above, corresponding to a `long double' argument.
* However, the current release of the VxWorks libraries does not support
* `long double' data; using the optional `L' gives unpredictable results.
* .iP
* A character that specifies the type of conversion to be applied.
* .LP
*
* As noted above, a field width, or precision, or both, can be indicated by
* an asterisk (`*').  In this case, an `int' argument supplies the field width
* or precision.  The arguments specifying field width, or precision, or both,
* should appear (in that order) before the argument (if any) to be converted.
* A negative field width argument is taken as a `-' flag followed by a positive
* field width.  A negative precision argument is taken as if the precision
* were omitted.
*
* The flag characters and their meanings are:
* .iP `-'
* The result of the conversion will be left-justified within the field.
* (it will be right-justified if this flag is not specified.)
* .iP `+'
* The result of a signed conversion will always begin with a plus or
* minus sign.  (It will begin with a sign only when a negative value
* is converted if this flag is not specified.)
* .iP `space'
* If the first character of a signed conversion is not a sign, or
* if a signed conversion results in no characters, a space will be
* prefixed to the result.  If the `space' and `+' flags both appear, the
* `space' flag will be ignored.
* .iP `#'
* The result is to be converted to an "alternate form."  For `o' conversion
* it increases the precision to force the first digit of the result to be a
* zero.  For `x' (or `X') conversion, a non-zero result will have "0x" (or
* "0X") prefixed to it.  For `e', `E', `f', `g', and `g' conversions, the
* result will always contain a decimal-point character, even if no digits
* follow it.  (Normally, a decimal-point character appears in the result of
* these conversions only if no digit follows it).  For `g' and `G'
* conversions, trailing zeros will not be removed from the result.  For
* other conversions, the behavior is undefined.
* .iP `0'
* For `d', `i', `o', `u', `x', `X', `e', `E', `f', `g', and `G' conversions,
* leading zeros (following any indication of sign or base) are used to pad
* to the field width; no space padding is performed.  If the `0' and `-'
* flags both appear, the `0' flag will be ignored.  For `d', `i', `o', `u',
* `x', and `X' conversions, if a precision is specified, the `0' flag will
* be ignored.  For other conversions, the behavior is undefined.
* .LP
*
* The conversion specifiers and their meanings are:
* .iP "`d', `i'"
* The `int' argument is converted to signed decimal in the style
* `[-]dddd'.  The precision specifies the minimum number of digits
* to appear; if the value being converted can be represented in
* fewer digits, it will be expanded with leading zeros.  The
* default precision is 1.  The result of converting a zero value
* with a precision of zero is no characters.
* .iP "`o', `u', `x', `X'"
* The `unsigned int' argument is converted to unsigned octal (`o'),
* unsigned decimal (`u'), or unsigned hexadecimal notation (`x' or `X')
* in the style `dddd'; the letters abcdef are used for `x' conversion
* and the letters ABCDEF for `X' conversion.  The precision specifies
* the minimum number of digits to appear; if the value being
* converted can be represented in fewer digits, it will be
* expanded with leading zeros.  The default precision is 1.  The
* result of converting a zero value with a precision of zero is
* no characters.
* .iP `f'
* The `double' argument is converted to decimal notation in the
* style `[-]ddd.ddd', where the number of digits after the decimal
* point character is equal to the precision specification.  If the
* precision is missing, it is taken as 6; if the precision is zero
* and the `#' flag is not specified, no decimal-point character
* appears.  If a decimal-point character appears, at least one
* digit appears before it.  The value is rounded to the appropriate
* number of digits.
* .iP "`e', `E'"
* The `double' argument is converted in the style `[-]d.ddde+/-dd',
* where there is one digit before the decimal-point character
* (which is non-zero if the argument is non-zero) and the number
* of digits after it is equal to the precision; if the precision
* is missing, it is taken as 6; if the precision is zero and the
* `#' flag is not specified, no decimal-point character appears.  The
* value is rounded to the appropriate number of digits.  The `E'
* conversion specifier will produce a number with `E' instead of `e'
* introducing the exponent.  The exponent always contains at least
* two digits.  If the value is zero, the exponent is zero.
* .iP "`g', `G'"
* The `double' argument is converted in style `f' or `e' (or in style
* `E' in the case of a `G' conversion specifier), with the precision
* specifying the number of significant digits.  If the precision
* is zero, it is taken as 1.  The style used depends on the
* value converted; style `e' (or `E') will be used only if the
* exponent resulting from such a conversion is less than -4 or
* greater than or equal to the precision.  Trailing zeros are
* removed from the fractional portion of the result; a decimal-point
* character appears only if it is followed by a digit.
* .iP `c'
* The `int' argument is converted to an `unsigned char', and the
* resulting character is written.
* .iP `s'
* The argument should be a pointer to an array of character type.
* Characters from the array are written up to (but not including)
* a terminating null character; if the precision is specified,
* no more than that many characters are written.  If the precision
* is not specified or is greater than the size of the array, the
* array will contain a null character.
* .iP `p'
* The argument should be a pointer to `void'.  The value of the
* pointer is converted to a sequence of printable characters,
* in hexadecimal representation (prefixed with "0x").
* .iP `n'
* The argument should be a pointer to an integer into which
* the number of characters written to the output stream
* so far by this call to fprintf() is written.  No argument is converted.
* .iP `%'
* A `%' is written.  No argument is converted.  The complete
* conversion specification is %%.
* .LP
*
* If a conversion specification is invalid, the behavior is undefined.
*
* If any argument is, or points to, a union or an aggregate (except for an
* array of character type using `s' conversion, or a pointer using `p'
* conversion), the behavior is undefined.
*
* In no case does a non-existent or small field width cause truncation of a
* field if the result of a conversion is wider than the field width, the
* field is expanded to contain the conversion result.
*
* INCLUDE FILES: fioLib.h
*
* SMP CONSIDERATIONS
* This API is spinlock and intCpuLock restricted.
*
* SMALL FOOTPRINT CONSIDERATIONS
* Although not part of the default small footprint configuration, printf()
* is available in a small footprint VSB build and will be included in the
* VIP if used by other code included in the VIP.
*
* In a small footprint configuration, interrupts may be locked for the
* duration of the operation because printf() may use the same polled-mode
* console port driver as kprintf().  The choice of output mechanism is
* an implementation detail and subject to change -- in particular, any 
* output that _must_ use polled mode should be written with kprintf() 
* instead of with printf() -- however users of printf() in small footprint 
* configurations should be aware of the likely impact on system performance 
* and interrupt latency when polled mode is used.
*
* RETURNS:
* The number of characters written, or a negative value if an
* output error occurs.
*
* SEE ALSO: fprintf(),
* .I "American National Standard for Information Systems -"
* .I "Programming Language - C, ANSI X3.159-1989: Input/Output (stdio.h)"
*
* VARARGS1
*/

int printf
    (
    const char *  fmt,	/* format string to write */
    ...			/* optional arguments to format string */
    )
    {
    va_list vaList;	/* traverses argument list */
    int nChars;

    va_start (vaList, fmt);
    nChars = fioFormatV (fmt, vaList, (FIOFORMATV_OUTPUT_FUNCPTR)fioBufPrint,
			 (_Vx_usr_arg_t)1);
    va_end (vaList);

    return (nChars);
    }


/*******************************************************************************
*
* oprintf - write a formatted string to an output function
*
* This routine prints a formatted string via the function specified by
* <prtFunc>.  The function will receive as parameters a pointer to a buffer,
* an integer indicating the length of the buffer, and the argument <prtArg>.
* If NULL is specified as the output function, the output will be sent to
* stdout.
*
* The function and syntax of oprintf are otherwise identical to printf().
*
* SMP CONSIDERATIONS
* This API is spinlock and intCpuLock restricted.
*
* RETURNS:
* The number of characters output, not including the NULL terminator.
*
* SEE ALSO: printf()
*
* VARARGS2
*/

int oprintf
    (
    OPRINTF_OUTPUT_FUNCPTR prtFunc,/* pointer to output function */
    _Vx_usr_arg_t prtArg,	/* argument for output function */
    const char *  fmt,		/* format string to write */
    ...				/* optional arguments to format string */
    )
    {
    va_list	vaList;	/* traverses argument list */
    int		nChars;

    va_start (vaList, fmt);

    /* if no output routine, do what printf does */

    if (prtFunc == NULL)
	{
	prtFunc = (FIOFORMATV_OUTPUT_FUNCPTR)fioBufPrint;
	prtArg = 1;
	}

    /* format the data */

    nChars = fioFormatV (fmt, vaList, prtFunc, prtArg);

    va_end(vaList);
    return(nChars);
    }


/*******************************************************************************
*
* printErr - write a formatted string to the standard error stream
*
* This routine writes a formatted string to standard error.  Its function and
* syntax are otherwise identical to printf().
*
* SMP CONSIDERATIONS
* This API is spinlock and intCpuLock restricted.
*
* RETURNS: The number of characters output, or ERROR if there is an error
* during output.
*
* SEE ALSO: printf()
*
* VARARGS1
*/

int printErr
    (
    const char *  fmt,	/* format string to write */
    ...			/* optional arguments to format */
    )
    {
    va_list vaList;	/* traverses argument list */
    int nChars;

    va_start (vaList, fmt);
    nChars = fioFormatV (fmt, vaList, (FIOFORMATV_OUTPUT_FUNCPTR)fioBufPrint,
			 (_Vx_usr_arg_t)2);
    va_end (vaList);

    return (nChars);
    }


/*******************************************************************************
*
* printExc - print an exception-generated error message
*
* If at interrupt level or other invalid/fatal state, then "print"
* into System Exception Message area. Otherwise, post the message to
* the exception-task (tExcTask) to be processed in a safe task
* context.
*
* This function is called by the architecture-specific exception
* handlers (see target/src/arch/xxx/excArchShow.c), so without
* changing every arch-specific exception handler, ED&R needs a way to
* ensure that its output comes out -- this is what the hook
* _func_printExcPrintHook is for.
*
* RETURNS: N/A
*
* \NOMANUAL
*/

void printExc
    (
    char *	  fmt,	/* format string */
    _Vx_usr_arg_t arg1,	/* argument #1 */
    _Vx_usr_arg_t arg2,	/* argument #2 */
    _Vx_usr_arg_t arg3,	/* argument #3 */
    _Vx_usr_arg_t arg4,	/* argument #4 */
    _Vx_usr_arg_t arg5	/* argument #5 */
    )
    {
    UINT	state;
    size_t	pageSize;
    char *	pageAddr;

    /*
     * Dump formatted string to sysExcMsg area if called from an ISR, or from
     * kernel context or before the kernel has initialized, or
     * if none of the hooks have been installed.
     */

    if ((INT_CONTEXT ()) || (taskIdCurrent == NULL) || (KERNEL_ENTERED_ME ()) ||
	((_func_printExcPrintHook == NULL) && (_func_excJobAdd == NULL)))
	{
	/* Exception happened during exception processing, or before
	 * any task could be initialized. Tack the message onto the end
	 * of a well-known location.
	 */

	/* see if we need to write enable the memory */

	if (vmLibInfo.vmLibInstalled)
	    {
	    pageSize = VM_PAGE_SIZE_GET();
	    pageAddr = (char *) ((ptrdiff_t) sysExcMsg / pageSize * pageSize);

	    if ((VM_STATE_GET (NULL, (void *) pageAddr, &state) != ERROR) &&
		((state & VM_STATE_MASK_WRITABLE) == VM_STATE_WRITABLE_NOT))
		{
		TASK_SAFE();			/* safe from deletion */

		VM_STATE_SET (NULL, pageAddr, (msize_t)pageSize,
			      VM_STATE_MASK_WRITABLE, VM_STATE_WRITABLE);

#if defined(_WRS_CONFIG_SMP)
		/*
		 * Get the ED&R record-allocation lock.  This is not strictly
		 * necessary, but ensures that the entire message from this
		 * CPU will be intact rather than possibly corrupted by those
		 * from other CPUs that happen to be doing printExc at the
		 * same time.  This lock supports recursive acquisition.
		 */
		{ int key = edrBufLock();
#endif	/* _WRS_CONFIG_SMP */
		sysExcMsg += sprintf (sysExcMsg,fmt,arg1,arg2,arg3,arg4,arg5);
#if defined(_WRS_CONFIG_SMP)
		edrBufUnlock(key);	}
#endif	/* _WRS_CONFIG_SMP */

		VM_STATE_SET (NULL, pageAddr, (msize_t)pageSize,
			      VM_STATE_MASK_WRITABLE, VM_STATE_WRITABLE_NOT);

		TASK_UNSAFE();			/* unsafe from deletion */
		return;
		}
	    }

#if defined(_WRS_CONFIG_SMP)
	{ int key = edrBufLock();
#endif	/* _WRS_CONFIG_SMP */
	sysExcMsg += sprintf (sysExcMsg, fmt, arg1, arg2, arg3, arg4, arg5);
#if defined(_WRS_CONFIG_SMP)
	edrBufUnlock(key);	}
#endif	/* _WRS_CONFIG_SMP */
	}
    else
	{
	/* If the hook is set up, then use that to print out the
	 * exception info, otherwise post it off to tExcTask as
	 * previously.
	 */
	if (_func_printExcPrintHook != NULL)
	    {
	    (* _func_printExcPrintHook) ((long) fmt, arg1,arg2,arg3,arg4,arg5);
	    }
	else if (_func_excJobAdd != NULL)
	    {
	    /* Queue printErr/_func_shellExcPrint to the exception task */

	    if (_func_shellExcPrint != NULL)
		_func_excJobAdd ((VOIDFUNCPTR)_func_shellExcPrint,
				 (_Vx_usr_arg_t)taskIdSelf(),
				 (_Vx_usr_arg_t)fmt,
				 arg1, arg2, arg3, arg4);
	    else
		_func_excJobAdd ((VOIDFUNCPTR)printErr, (_Vx_usr_arg_t)fmt,
				 arg1, arg2, arg3, arg4, arg5);
	    }
	/*
	 * At this point we have an unhandled exception but no exception
	 * task to handle the message.  Use kprintf() if it is available.
	 * (Unhandled exceptions are not supposed to occur, so it is
	 * appropriate to use a debugging function to report them if
	 * no other mechanism is available.)
	*/
	else if (_func_kprintf != NULL)
		_func_kprintf (fmt, arg1, arg2, arg3, arg4, arg5);
	}
    }


/*******************************************************************************
*
* sprintf - write a formatted string to a buffer (ANSI)
*
* This routine copies a formatted string to a specified buffer, which is
* null-terminated.  Its function and syntax are otherwise identical
* to printf().
*
* RETURNS:
* The number of characters copied to <buffer>, not including the NULL
* terminator.
*
* SEE ALSO: printf(),
* .I "American National Standard for Information Systems -"
* .I "Programming Language - C, ANSI X3.159-1989: Input/Output (stdio.h)"
*
* VARARGS2
*/

int sprintf
    (
    char *  buffer,	/* buffer to write to */
    const char *  fmt,	/* format string */
    ...			/* optional arguments to format */
    )
    {
    va_list	vaList;		/* traverses argument list */
    int		nChars;

    va_start (vaList, fmt);
    nChars = fioFormatV (fmt, vaList, (FIOFORMATV_OUTPUT_FUNCPTR)fioBufPut,
			 (_Vx_usr_arg_t) &buffer);
    va_end (vaList);

    *buffer = EOS;

    return (nChars);
    }


/*******************************************************************************
*
* snprintf - write a formatted string to a buffer, not exceeding buffer size (ANSI)
*
* This routine copies a formatted string to a specified buffer, up to a given
* number of characters.  The formatted string will be null terminated.  This
* routine guarantees never to write beyond the provided buffer regardless of
* the format specifier or the arguments to be formatted.  The <count>
* argument specifies the maximum number of characters to store in the buffer,
* including the null terminator.
*
* Its function and syntax are otherwise identical to printf().
*
* RETURNS: The number of characters copied to <buffer>, not including the
* NULL terminator.  Even when the supplied <buffer> is too small to
* hold the complete formatted string, the return value represents
* the number of characters that would have been written to <buffer>
* if <count> was sufficiently large.
*
* SEE ALSO: sprintf(), printf(),
* \tb "International Organization for Standardization, ISO/IEC 9899:1999,"
* \tb "Programming languages - C: Input/output (stdio.h)"
*
* VARARGS2
*/

int snprintf
    (
    char *       buffer, /* buffer to write to */
    size_t       count,  /* max number of characters to store in buffer */
    const char * fmt,    /* format string */
    ...                  /* optional arguments to format */
    )
    {
    va_list	  vaList;	 /* traverses the argument list */
    int		  nChars;
    SNPUTBUF_ARG  snputbufArg;

    snputbufArg.pBuf	= buffer;
    snputbufArg.pBufEnd	= (char *)(buffer + count);

    va_start (vaList, fmt);
    nChars = fioFormatV (fmt, vaList, (FIOFORMATV_OUTPUT_FUNCPTR)fioSnBufPut,
			 (_Vx_usr_arg_t) &snputbufArg);
    va_end (vaList);

    if (count != 0)
	*snputbufArg.pBuf = EOS; /* null-terminate the string */

    return (nChars);
    }


#ifdef _WRS_ALTIVEC_SUPPORT
typedef union {
      __vector unsigned int	vul;
      float			f32[4];
      unsigned long		u32[4];
      unsigned short		u16[8];
      unsigned char		u8[16];
      signed long		s32[4];
      signed short		s16[8];
      signed char		s8[16];
    } VECTOR;
#endif /* _WRS_ALTIVEC_SUPPORT */


/*******************************************************************************
*
* fioFormatV - convert a format string
*
* This routine is used by the printf() family of routines to handle the
* actual conversion of a format string.  The first argument is a format
* string, as described in the entry for printf().  The second argument is a
* variable argument list <vaList> that was previously established.
*
* As the format string is processed, the result will be passed to the output
* routine whose address is passed as the third parameter, <outRoutine>.
* This output routine may output the result to a device, or put it in a
* buffer.  In addition to the buffer and length to output, the fourth
* argument, <outarg>, will be passed through as the third parameter to the
* output routine.  This parameter could be a file descriptor, a buffer
* address, or any other value that can be passed in a variable of type
* "_Vx_usr_arg_t".
*
* The output routine should be declared as follows:
* \cs
*     STATUS outRoutine
*         (
*         char          *buffer, /@ buffer passed to routine            @/
*         int           nchars,  /@ length of buffer                    @/
*         _Vx_usr_arg_t outarg   /@ arbitrary arg passed to fmt routine @/
*         )
* \ce
* The output routine should return OK if successful, or ERROR if unsuccessful.
*
* RETURNS:
* The number of characters output, or ERROR if the output routine
* returned ERROR.
*
* INTERNAL
* Warning, this routine is extremely complex and its integrity is easily
* destroyed. Do not change this code without absolute understanding of all
* ramifications and consequences.
*/

int fioFormatV
    (
    FAST const char *fmt,	/* format string */
    va_list	vaList,		/* pointer to varargs list */
    FIOFORMATV_OUTPUT_FUNCPTR outRoutine,/* handler for args as they're formatted */
    _Vx_usr_arg_t outarg	/* argument to routine */
    )
#ifndef _WRS_CONFIG_FORMATTED_OUT_BASIC
    {
    FAST char	ch;		/* character from fmt */
    FAST int	n;		/* handy integer (short term usage) */
    FAST char *	cp;		/* handy char pointer (short term usage) */
    int		width;		/* width from format (%8d), or 0 */
    char	sign;		/* sign prefix (' ', '+', '-', or \0) */
    unsigned long long
		ulongLongVal;	/* unsigned 64 bit arguments %[diouxX] */
    long long * llTmpAddr;
    long * lTmpAddr;
    int * iTmpAddr;
    short * sTmpAddr;

# ifdef _WRS_SPE_SUPPORT
    int         e500_sign_flag; /* e500-specific 'r' and 'R' modifiers */
    unsigned long long llconst; /* e500-specific (short term usage) */
    int         a, ndigs;       /* e500-specific (short term usage) */
# endif /* _WRS_SPE_SUPPORT */

    int		prec;		/* precision from format (%.3d), or -1 */
    int		oldprec;	/* old precision from format (%.3d), or -1 */
    int		dprec;		/* a copy of prec if [diouxX], 0 otherwise */
    int		fpprec;		/* `extra' floating precision in [eEfgG] */
    size_t	size;		/* size of converted field or string */
    int		fieldsz;	/* field size expanded by sign, etc */
    int		realsz;		/* field size expanded by dprec */

# ifdef _WRS_ALTIVEC_SUPPORT
    FAST int	i;		/* handy integer (short term usage) */
    FAST char *	vp;		/* handy char pointer (short term usage) */
    char	Separator;	/* separator for vector elements */
    char	C_Separator;	/* separator for char vector elements */
    VECTOR	vec;		/* vector argument */
    BOOL	doVector;	/* AltiVec vector */
# endif /* _WRS_ALTIVEC_SUPPORT */
    char	FMT[20];	/* To collect fmt info */
    FAST char *	Collect;	/* char pointer to FMT */

    BOOL	doLongInt;	/* long integer */
    BOOL	doLongLongInt;	/* long long integer - 64 bit */
    BOOL	doShortInt;	/* short integer */
    BOOL	doAlt;		/* alternate form */
    BOOL	doLAdjust;	/* left adjustment */
    BOOL	doZeroPad;	/* zero (as opposed to blank) pad */
    BOOL	doHexPrefix;	/* add 0x or 0X prefix */
    BOOL	doSign;		/* change sign to '-' */
    char	buf[BUF];	/* space for %c, %[diouxX], %[eEfgG] */
    char	ox[4];		/* space for 0x hex-prefix */
    char *	xdigs = NULL;	/* digits for [xX] conversion */
    int		ret = 0;	/* return value accumulator */
    enum {OCT, DEC, HEX} base;	/* base for [diouxX] conversion */


    FOREVER		/* Scan the format for conversions (`%' character) */
	{
	for (cp = CHAR_FROM_CONST(fmt);((ch=*fmt) != EOS)&&(ch != '%'); fmt++)
	    ;

	if ((n = (int)(fmt - cp)) != 0)
	    {
	    if ((*outRoutine) (cp, n, outarg) != OK)
		return (ERROR);

	    ret += n;
	    }

	if (ch == EOS)
	    return (ret);		/* return total length */

	fmt++;				/* skip over '%' */
# ifdef _WRS_ALTIVEC_SUPPORT
	Separator	= ' ';	/* default separator for vector elements */
	C_Separator	= EOS;	/* default separator for char vector elements */
	doVector	= FALSE;	/* no vector modifier */
# endif /* _WRS_ALTIVEC_SUPPORT */

	*FMT		= EOS;
	Collect		= FMT;
	doLongInt	= FALSE;	/* long integer */
	doLongLongInt	= FALSE;	/* 64 bit integer */
	doShortInt	= FALSE;	/* short integer */
	doAlt		= FALSE;	/* alternate form */
	doLAdjust	= FALSE;	/* left adjustment */
	doZeroPad	= FALSE;	/* zero (as opposed to blank) pad */
	doHexPrefix	= FALSE;	/* add 0x or 0X prefix */
	doSign		= FALSE;	/* change sign to '-' */
	dprec		= 0;
	fpprec		= 0;
	width		= 0;
	prec		= -1;
	oldprec		= -1;
	sign		= EOS;
# ifdef _WRS_SPE_SUPPORT
	e500_sign_flag  = 1;	/* default is %r -- signed fixed point value */
# endif /* _WRS_SPE_SUPPORT */

#define get_CHAR  (ch = *Collect++ = *fmt++)

# ifdef _WRS_ALTIVEC_SUPPORT
#define SET_VECTOR_FMT(VFMT,NO)						\
  do									\
    {									\
	char * to;							\
									\
	vec.vul =  va_arg (vaList,__vector unsigned int);		\
	cp = buf;							\
									\
	*Collect = EOS;							\
	i = (NO);							\
	to = VFMT = (char *) malloc (i*20);				\
	if (to != NULL) {						\
		while(i-- > 0) {					\
									\
			char * from = FMT;				\
									\
			*to++ = '%';					\
			while ((*to++ = *from++) != '\0')		\
			    /* do nothing*/;				\
			*(char*)((int)(to)-1) = Separator;		\
		}							\
		*(--to) = EOS;						\
	    }								\
    }									\
  while (0)

#define RESET_VECTOR_FMT(VFMT)			\
						\
			size = strlen(cp);	\
			sign = EOS;		\
						\
			free(VFMT)
# endif /* _WRS_ALTIVEC_SUPPORT */

rflag:
get_CHAR;
reswitch:
	switch (ch)
	    {
	    case ' ':
		    /* If the space and + flags both appear, the space
		     * flag will be ignored. -- ANSI X3J11
		     */
		    if (!sign)
			sign = ' ';
		    goto rflag;
# ifdef _WRS_ALTIVEC_SUPPORT
	    case ',':
	    case ';':
	    case ':':
	    case '_':
		    Collect--;
		    Separator = C_Separator = ch;
		    goto rflag;
# endif /* _WRS_ALTIVEC_SUPPORT */
	    case '#':
		    doAlt = TRUE;
		    goto rflag;

	    case '*':
		    /* A negative field width argument is taken as a
		     * flag followed by a positive field width.
		     *	-- ANSI X3J11
		     * They don't exclude field widths read from args.
		     */
		    if ((width = va_arg(vaList, int)) >= 0)
			goto rflag;

		    width = -width;			/* FALLTHROUGH */

	    case '-':
		    doLAdjust = TRUE;
		    goto rflag;

	    case '+':
		    sign = '+';
		    goto rflag;

	    case '.':
		    get_CHAR;
		    if ( ch == '*' )
			{
			n = va_arg(vaList, int);
			prec = (n < 0) ? -1 : n;
			goto rflag;
			}

		    n = 0;
		    while (is_digit(ch))
			{
			n = 10 * n + to_digit(ch);
			get_CHAR;
			}
		    prec = n < 0 ? -1 : n;
		    goto reswitch;

	    case '0':
		    /* Note that 0 is taken as a flag, not as the
		     * beginning of a field width. -- ANSI X3J11
		     */
		    doZeroPad = TRUE;
		    goto rflag;

	    case '1':
	    case '2':
	    case '3':
	    case '4':
	    case '5':
	    case '6':
	    case '7':
	    case '8':
	    case '9':
		    n = 0;
		    do
			{
			n = 10 * n + to_digit(ch);
			get_CHAR;
			} while (is_digit(ch));

		    width = n;
		    goto reswitch;

	    case 'h':
		    doShortInt = TRUE;
		    goto rflag;

	    case 'l':
		    get_CHAR;
		    if ( ch == 'l' )
			{
			doLongLongInt = TRUE;
			goto rflag;
			}
# ifdef _WRS_SPE_SUPPORT
		    else if ( ch == 'r' || ch == 'R' ) /* e500 fixed point */
			{
			doLongLongInt = TRUE;
			goto reswitch;
			}
# endif /*_WRS_SPE_SUPPORT */
		    else
			{
			doLongInt = TRUE;
			goto reswitch;
			}
# ifdef _WRS_SPE_SUPPORT
	    case 'R': /* e500 fixed point */
		    e500_sign_flag  = 0; /* unsigned fixed point value */
		    /* FALLTHROUGH */
	    case 'r': /* e500 fixed point */
		    sign = EOS;
		    cp = buf;

		    if (e500_sign_flag) {
			ulongLongVal = SARG();
			if ((long long)ulongLongVal < 0)
			   {
			   ulongLongVal = -ulongLongVal;
			   sign = '-';
			   }
		    }
		    else ulongLongVal = UARG();

		    if (doShortInt) {
			ndigs = 5;
			ulongLongVal <<= (32+16);
		    } else if (!(doLongLongInt)) {
			ndigs = 10;
			ulongLongVal <<= 32;
		    } else {
			ndigs = 19;
		    }
		    n = 0;
		    if (e500_sign_flag) {
		      if (ulongLongVal & (1LL << 63)) {
			n = 1;
		      }
		      ulongLongVal <<= 1;
		      ndigs--;
		    }
		    *cp++ = n + '0';
		    *cp++ = '.';
		    ndigs = (prec >0 ? prec : ndigs);
		    llconst = ((1LL << (64-1))-1);
		    while (ndigs) {
		      a = (ulongLongVal >> (64-3)) + (ulongLongVal >> (64-1));
		      ulongLongVal  = (ulongLongVal & llconst)
				      + ((ulongLongVal << 2) & llconst);
		      a += (ulongLongVal >> 63);
		      *cp++ = a + '0';
		      ndigs--;
		      ulongLongVal <<= 1;
		    }
		    *cp = '\0';

		    cp = buf;
		    size = strlen(cp);
		    break;
# endif /* _WRS_SPE_SUPPORT */

# ifdef _WRS_ALTIVEC_SUPPORT
	    case 'v':
		    Collect--;
		    doVector = TRUE;
		    goto rflag;
# endif /* _WRS_ALTIVEC_SUPPORT */

	    case 'c':
# ifdef _WRS_ALTIVEC_SUPPORT
		    if (doVector)
		    {
			vec.vul =  va_arg (vaList,__vector unsigned int);
			cp = buf;
			vp = (unsigned char *)&vec.u8;
			i = 15;

			while(i-- > 0) {

				*cp++ = *vp++;
				if (C_Separator) *cp++ = C_Separator;
			}

			*cp++ = *vp++;

			cp = buf;
			size = 16 + (C_Separator ? 15:0);
			sign = EOS;
		    }
		    else
		    {
# endif /* _WRS_ALTIVEC_SUPPORT */
			*(cp = buf) = (char)va_arg(vaList, int);
			size = 1;
			sign = EOS;
# ifdef _WRS_ALTIVEC_SUPPORT
		    }
# endif /* _WRS_ALTIVEC_SUPPORT */
		    break;

	    case 'D':
		    doLongInt = TRUE;			/* FALLTHROUGH */

	    case 'd':
	    case 'i':
# ifdef _WRS_ALTIVEC_SUPPORT
		    if (doVector)
		    {
			SET_VECTOR_FMT(vp,doShortInt?8:4);

			if (doShortInt)
			    sprintf(cp, vp, vec.s16[0], vec.s16[1], vec.s16[2],
				    vec.s16[3], vec.s16[4], vec.s16[5],
				    vec.s16[6], vec.s16[7]);
			else
			    sprintf(cp, vp, vec.s32[0], vec.s32[1], vec.s32[2],
				    vec.s32[3]);

			RESET_VECTOR_FMT(vp);
			break;
		    }
# endif /* _WRS_ALTIVEC_SUPPORT */

		    ulongLongVal = SARG();
		    if ((long long)ulongLongVal < 0)
			{
			ulongLongVal = -ulongLongVal;
			sign = '-';
			}
		    base = DEC;
		    goto number;

	    case 'n':
		    /* ret is int, so effectively %lln = %ln */
		    if (doLongLongInt)
		    {
		    if ((llTmpAddr = va_arg(vaList, long long *)) != NULL)
		    *llTmpAddr= (long long) ret;
		    }
		    else if (doLongInt)
		    {
		    if ((lTmpAddr = va_arg(vaList, long *)) != NULL)
		    *lTmpAddr = ret;
		    }
		    else if (doShortInt)
		    {
		    if ((sTmpAddr = va_arg(vaList, short *)) != NULL)
		    *sTmpAddr = (short)ret;
		    }
		    else
		    {
		    if ((iTmpAddr = va_arg(vaList, int *)) != NULL)
		    *iTmpAddr = ret;
		    }
		    continue;				/* no output */

	    case 'O':
		    doLongInt = TRUE;			/* FALLTHROUGH */

	    case 'o':
# ifdef _WRS_ALTIVEC_SUPPORT
		    if (doVector)
		    {
			SET_VECTOR_FMT(vp,doShortInt?8:4);

			if (doShortInt)
			    sprintf(cp, vp, vec.s16[0], vec.s16[1], vec.s16[2],
				    vec.s16[3], vec.s16[4], vec.s16[5],
				    vec.s16[6], vec.s16[7]);
			else
			    sprintf(cp, vp, vec.s32[0], vec.s32[1], vec.s32[2],
				    vec.s32[3]);

			RESET_VECTOR_FMT(vp);
			break;
		    }
# endif /* _WRS_ALTIVEC_SUPPORT */
		    ulongLongVal = UARG();
		    base = OCT;
		    goto nosign;

	    case 'p':
		    /* The argument shall be a pointer to void.  The
		     * value of the pointer is converted to a sequence
		     * of printable characters, in an implementation
		     * defined manner. -- ANSI X3J11
		     */
# ifdef _WRS_ALTIVEC_SUPPORT
		    if (doVector)
		    {
			SET_VECTOR_FMT(vp,4);

			sprintf(cp, vp, vec.u32[0], vec.u32[1], vec.u32[2],
				vec.u32[3]);

			RESET_VECTOR_FMT(vp);
			break;
		    }
# endif /* _WRS_ALTIVEC_SUPPORT */
		    ulongLongVal = (unsigned long long) (unsigned long)
				   va_arg(vaList, void *);/* NOSTRICT */
		    base	= HEX;
		    xdigs	= "0123456789abcdef";
		    doHexPrefix = TRUE;
		    ch		= 'x';
		    goto nosign;

	    case 's':
		    if ((cp = va_arg(vaList, char *)) == NULL)
			cp = "(null)";

		    if (prec >= 0)
			{
			/* can't use strlen; can only look for the
			 * NUL in the first `prec' characters, and
			 * strlen() will go further.
			 */

			char *p = (char *)memchr(cp, 0, (size_t)prec);

			if (p != NULL)
			    {
			    size = p - cp;
			    if (size > prec)
				size = prec;
			    }
			else
			    size = prec;
			}
		    else
			size = strlen(cp);

		    sign = EOS;
		    break;

	    case 'U':
		    doLongInt = TRUE;			/* FALLTHROUGH */

	    case 'u':
# ifdef _WRS_ALTIVEC_SUPPORT
		    if (doVector)
		    {
			SET_VECTOR_FMT(vp, doShortInt?8:4);

			if (doShortInt)
			    sprintf(cp, vp, vec.u16[0], vec.u16[1], vec.u16[2],
				    vec.u16[3], vec.u16[4], vec.u16[5],
				    vec.u16[6], vec.u16[7]);
			else
			    sprintf(cp, vp, vec.u32[0], vec.u32[1], vec.u32[2],
				    vec.u32[3]);

			RESET_VECTOR_FMT(vp);
			break;
		    }
# endif /* _WRS_ALTIVEC_SUPPORT */
		    ulongLongVal = UARG();
		    base = DEC;
		    goto nosign;

	    case 'X':
		    xdigs = "0123456789ABCDEF";
		    goto hex;

	    case 'x':
		    xdigs = "0123456789abcdef";

hex:
# ifdef _WRS_ALTIVEC_SUPPORT
		    if (doVector)
		    {
			SET_VECTOR_FMT(vp, doShortInt?8:4);

			if (doShortInt)
			    sprintf(cp, vp, vec.s16[0], vec.s16[1], vec.s16[2],
				    vec.s16[3], vec.s16[4], vec.s16[5],
				    vec.s16[6], vec.s16[7]);
			else
			    sprintf(cp, vp, vec.s32[0], vec.s32[1], vec.s32[2],
				    vec.s32[3]);

			RESET_VECTOR_FMT(vp);
			break;
		    }
# endif /* _WRS_ALTIVEC_SUPPORT */
		    ulongLongVal = UARG();
		    base = HEX;

		    /* leading 0x/X only if non-zero */

		    if (doAlt && (ulongLongVal != 0))
			doHexPrefix = TRUE;

		    /* unsigned conversions */
nosign:		    sign = EOS;

		    /* ... diouXx conversions ... if a precision is
		     * specified, the 0 flag will be ignored. -- ANSI X3J11
		     */

number:		    if ((dprec = prec) >= 0)
			doZeroPad = FALSE;

		    /* The result of converting a zero value with an
		     * explicit precision of zero is no characters.
		     * -- ANSI X3J11
		     */
		    cp = buf + BUF;
		    if ((ulongLongVal != 0) || (prec != 0))
			{
			/* unsigned mod is hard, and unsigned mod
			 * by a constant is easier than that by
			 * a variable; hence this switch.
			 */
			switch (base)
			    {
			    case OCT:
				do
				    {
				    *--cp = to_char(ulongLongVal & 7);
				    ulongLongVal >>= 3;
				    } while (ulongLongVal);

				/* handle octal leading 0 */

				if (doAlt && (*cp != '0'))
				    *--cp = '0';
				break;

			    case DEC:
				/* many numbers are 1 digit */

				while (ulongLongVal >= 10)
				    {
				    *--cp = to_char(ulongLongVal % 10);
				    ulongLongVal /= 10;
				    }

				*--cp = to_char(ulongLongVal);
				break;

			    case HEX:
				do
				    {
				    *--cp = xdigs[ulongLongVal & 15];
				    ulongLongVal >>= 4;
				    } while (ulongLongVal);
				break;

			    default:
				cp = "bug in vfprintf: bad base";
				size = strlen(cp);
				goto skipsize;
			    }
			}

		    size = buf + BUF - cp;
skipsize:
		    break;

	    case 'L':
		    /* NOT IMPLEMENTED */
		    goto rflag;

	    case 'e':
	    case 'E':
	    case 'f':
	    case 'g':
	    case 'G':
# ifdef _WRS_ALTIVEC_SUPPORT
		    if (doVector)
		    {
			SET_VECTOR_FMT(vp, 4);

			sprintf(cp, vp, vec.f32[0], vec.f32[1], vec.f32[2],
			        vec.f32[3]);

			RESET_VECTOR_FMT(vp);
			break;
		    }
# endif /* _WRS_ALTIVEC_SUPPORT */
		    if (_func_fioFltFormatRtn != NULL)
			{
			oldprec = prec;		/* in case of strange float */

			if (prec > MAXFRACT)	/* do realistic precision */
			    {
			    if (((ch != 'g') && (ch != 'G')) || doAlt)
				fpprec = prec - MAXFRACT;
			    prec = MAXFRACT;	/* they asked for it! */
			    }
			else if (prec == -1)
			    prec = 6;		/* ANSI default precision */

			cp  = buf;		/* where to fill in result */
			*cp = EOS;		/* EOS terminate just in case */
			size = (*_func_fioFltFormatRtn) (va_addr(vaList), prec,
						   doAlt, ch, &doSign, cp,
						   buf+sizeof(buf));
			if ((int)size < 0)	/* strange value (Nan,Inf,..) */
			    {
			    size = -size;	/* get string length */
			    prec = oldprec;	/* old precision (not default)*/

			    doZeroPad = FALSE;	/* don't pad with zeroes */
			    if (doSign)		/* is strange value signed? */
				sign = '-';
			    }
			else
			    {
			    if (doSign)
				sign = '-';

			    if (*cp == EOS)
				cp++;
			    }
			break;
			}
		    /* FALLTHROUGH if no floating point format routine */

	    default:			/* "%?" prints ?, unless ? is NULL */
		    if (ch == EOS)
			return (ret);

		    /* pretend it was %c with argument ch */

		    cp   = buf;
		    *cp  = ch;
		    size = 1;
		    sign = EOS;
		    break;
	    }

	/* All reasonable formats wind up here.  At this point,
	 * `cp' points to a string which (if not doLAdjust)
	 * should be padded out to `width' places.  If
	 * doZeroPad, it should first be prefixed by any
	 * sign or other prefix; otherwise, it should be blank
	 * padded before the prefix is emitted.  After any
	 * left-hand padding and prefixing, emit zeroes
	 * required by a decimal [diouxX] precision, then print
	 * the string proper, then emit zeroes required by any
	 * leftover floating precision; finally, if doLAdjust,
	 * pad with blanks.
	 */

	/*
	 * compute actual size, so we know how much to pad.
	 * fieldsz excludes decimal prec; realsz includes it
	 */

	fieldsz = (int)(size + fpprec);

	if (sign)
	    {
	    fieldsz++;
	    if (fieldSzIncludeSign)
		dprec++;
	    }
	else if (doHexPrefix)
	    fieldsz += 2;

	realsz = (dprec > fieldsz) ? dprec : fieldsz;

	/* right-adjusting blank padding */

	if (!doLAdjust && !doZeroPad)
	    PAD(width - realsz, blanks);

	/* prefix */

	if (sign)
	    {
	    if ((*outRoutine) (&sign, 1, outarg) != OK)
		return (ERROR);
	    }
	else if (doHexPrefix)
	    {
	    ox[0] = '0';
	    ox[1] = ch;
	    if ((*outRoutine) (ox, 2, outarg) != OK)
		return (ERROR);
	    }

	/* right-adjusting zero padding */

	if (!doLAdjust && doZeroPad)
	    PAD(width - realsz, zeroes);

	/* leading zeroes from decimal precision */

	PAD(dprec - fieldsz, zeroes);

	/* the string or number proper */

	if ((*outRoutine) (cp, (int)size, outarg) != OK)
	    return (ERROR);

	/* trailing floating point zeroes */

	PAD(fpprec, zeroes);

	/* left-adjusting padding (always blank) */

	if (doLAdjust)
	    PAD(width - realsz, blanks);

	/* finally, adjust ret */

	ret += (width > realsz) ? width : realsz;
	}
    }
#else	/* _WRS_CONFIG_FORMATTED_OUT_BASIC */
    {
    char *           pBuf;              /* buffer Pointer */
    int              nChars;            /* Number of chars written */
    char             cBuf;              /* One character buffer */
    int              state;             /* Collection State */
    long             value;
    char *           pPtr;
    char *           pStr;              /* Temporary pointer */
    int              length;
    static char      terminate[] = "xscuandXoAFfgeGpEi%";
    BOOL	     isNegative;

    /* initialize */
    nChars = 0;
    while (*fmt != EOS)
	{
	pBuf   = (char *) fmt;	  /* Collects the text from the format string */

	/* Walk throught pFmt and collect text to print */

	while ((*fmt != EOS) && (*fmt != '%'))
	    fmt++;


	/* Output text from format string */

	if (fmt != pBuf)
	    {
	    if ((*outRoutine) (pBuf, fmt - pBuf, outarg) == OK)
		 nChars += (fmt - pBuf);
	    if (*fmt == EOS)
		break;
	    }

	state = FIO_STATE_COLLECT + 1;
	while ((*++fmt != EOS) && (state != FIO_STATE_COLLECT))
	    {
	    for (pPtr = terminate; *pPtr; pPtr++)
		{
		if (*pPtr == *fmt)
		   {
		   switch (*fmt)
			{
			case '%':
			    if ((*outRoutine) ("%", 1, outarg) == OK)
				nChars += 1;
			    break;
			case 'c':
			    cBuf = va_arg(vaList, int);
			    if ((*outRoutine) (&cBuf, 1, outarg) == OK)
				nChars += 1;
			    break;

			case 's':
			    pStr = va_arg(vaList, char*);
			    for (pBuf = pStr, length = 0 ;
				 *pBuf != EOS ;
				 pBuf++, length++)
				;

			    if ((*outRoutine) (pStr, length, outarg) == OK)
				nChars += length;

			    break;

			case 'X':
# ifdef FIO_HEX_X_SUPPORT
			    if ((length = fioBasicFormat (outRoutine,
							  outarg,
							  va_arg(vaList,
								 ptrdiff_t),
							  FIO_HEX,
							  FALSE,
							  tuVal)) > 0)
				nChars += length;

			    break;
# endif /* FIO_HEX_X_SUPPORT */
			case 'x':
			    if ((length = fioBasicFormat (outRoutine,
							  outarg,
							  va_arg(vaList,
								 ptrdiff_t),
							  FIO_HEX,
							  FALSE,
							  tlVal)) > 0)
				nChars += length;

			    break;

			case 'o':
# ifdef FIO_OCT_SUPPORT
			    if ((length = fioBasicFormat (outRoutine,
							  outarg,
							  va_arg(vaList, int),
							  FIO_OCT,
							  FALSE,
							  tlVal)) > 0)
				nChars += length;

			    break;
# endif /* FIO_OCT_SUPPORT */
			case 'd':
			    value = (long) va_arg(vaList, int);
			    if (value < 0)
				{
				isNegative = TRUE;
				value = ((value == INT_MIN)?
					 ((unsigned int)INT_MAX+1) :
					 (unsigned int)(-value));
				}
			    else
				isNegative = FALSE;

			    if ((length = fioBasicFormat (outRoutine,
							  outarg,
							  value,
							  FIO_DEC,
							  isNegative,
							  tlVal)) > 0)
				nChars += length;

			    break;

			case 'u':
			    if ((length = fioBasicFormat (outRoutine,
						   outarg,
						   va_arg(vaList, unsigned int),
						   FIO_DEC,
						   FALSE,
						   tlVal)) > 0)
				nChars += length;

			    break;

			default:
			    break;
			}
		    state = FIO_STATE_COLLECT;
		    break;
		    }
		}
	    }
	}

    return (nChars);
    }


/*******************************************************************************
*
* fioBasicFormat - Format long argument into HEX, DEC, or OCT number
*
* This is a support routine for the _WRS_CONFIG_FORMATTED_OUT_BASIC version of
* fioFormatV() Unsigned long numbers are converted into strings.
*
* RETURNS: number of characters transmitted or zero on failure.
*
* ERRNO: N/A
*
* \NOMANUAL
*/

LOCAL int fioBasicFormat
    (
    FUNCPTR     outRoutine,        /* output handler */
    int         outarg,            /* argument to routine */
    unsigned long        value,    /* value to convert into a string */
    int         base,              /* FIO_HEX, FIO_OCT or FIO_DEC */
    BOOL        isNegative,        /* Display negative */
    char *      pTtanslate         /* Translation string */
    )
    {
    FAST char *	pChar;
    char        numArray[NSIZE];
    int         count = 0;

    bzero (numArray, sizeof (numArray));
    pChar = &numArray[NSIZE-1];

    do
	{
	*pChar = (char)(pTtanslate[value % base]);
	--pChar;
	value /= base;
	count++;
	} while (value > 0);

    if (isNegative)
	*pChar-- = '-';

    pChar++;

    if (((*outRoutine) (pChar, &numArray[NSIZE-1] - (int)pChar + 1,
	                outarg)) == OK)
	return count;

    return 0;

    }
#endif /* _WRS_CONFIG_FORMATTED_OUT_BASIC */


/*******************************************************************************
*
* fioBufPut - put characters in a buffer
*
* This routine is a support routine for sprintf().
* This routine copies length bytes from source to destination, leaving the
* destination buffer pointer pointing at byte following block copied.
*
* RETURNS: OK always
*
* \NOMANUAL
*/

STATUS fioBufPut
    (
    char *inbuf,                /* pointer to source buffer */
    int length,                 /* number of bytes to copy */
    char **outptr               /* pointer to destination buffer */
    )
    {
    bcopy (inbuf, *outptr, (size_t)length);
    *outptr += length;

    return (OK);
    }


/*******************************************************************************
*
* fioSnBufPut - put characters in a buffer (safe version of fioBufPut)
*
* This is a support routine for snprintf().  This routine copies <length>
* bytes from source to destination, leaving the destination buffer pointer
* (pArg->pBuf) pointing at byte following block copied.
*
* If <length> exceeds the number of bytes available in the buffer, only the
* number of bytes that fit within the buffer are copied.  In this case
* OK is still returned (although no further copying will be performed) so
* that snprintf() can return the number of characters that would have been
* copied if the supplied buffer was of sufficient size.
*
* RETURNS: OK always
*
* \NOMANUAL
*/

STATUS fioSnBufPut
    (
    char *         pInBuf,       /* pointer to input buffer */
    int            length,       /* length of input buffer */
    SNPUTBUF_ARG * pArg          /* fioSnBufPut argument structure */
    )
    {
    int remaining;

    /* check if sufficient free space remains in the buffer */

    remaining = (int)(pArg->pBufEnd - pArg->pBuf) - 1;

    /* bail if at the end of buffer, recall need a single byte for null */

    if (remaining <= 0)
	return (OK);
    else if (length > remaining)
	length = remaining;

    bcopy (pInBuf, pArg->pBuf, (size_t)length);

    pArg->pBuf += length;

    return (OK);
    }


/*******************************************************************************
*
* fioBufPrint - printf() support routine: print characters in a buffer
*
* RETURNS: OK, or ERROR
*
* \NOMANUAL
*/

STATUS fioBufPrint
    (
    char *buf,
    size_t nbytes,
    int fd
    )
    {
	/* _func_consoleOut = NULL;  */  /* ll */
    if (_func_consoleOut == NULL)
	{
	if (_func_kwrite != NULL)
	    return ((*_func_kwrite) (buf, nbytes));
	return (ERROR);
	}

    if ((* _func_consoleOut) (fd, buf, nbytes) != nbytes)
	return (ERROR);

    return (OK);
    }
