/*****************************************************************************
*                                                                            *
*   Copyright (C) 2009 Symmetricom, Inc., all rights reserved                *
*                                                                            *
*   This software contains proprietary and confidential information of       *
*   Symmetricom.                                                             *
*   It may not be reproduced, used, or disclosed to others for any purpose   *
*   without the written authorization of Symmetricom.                        *
*                                                                            *
******************************************************************************

FILE NAME    : DBG.h

AUTHOR       : Ken Ho

DESCRIPTION  :

This header file contains the debug definitions for SoftClient.

Revision control header:
$Id: DBG/DBG.h 1.19 2012/02/16 15:09:26PST German Alvarez (galvarez) Exp  $

******************************************************************************
*/

#ifndef H_DBG_h
#define H_DBG_h

#ifdef __cplusplus
extern "C"
{
#endif

#include "sc_servo_api.h"

/*************************************************************************
**    constants and macros
*************************************************************************/

#define PDV_PROBE_PRT 	0x1  /* channel 0 */
#define GEORGE_PTP_PRT 	0x2  /* channel 1 */
#define CLK_COR_PRT     0x4  /* channel 2 */
#define GEORGE_PTPD_PRT 0x8  /* channel 3 */
#define GEORGE_FAST_PRT 0x10  /* channel 4 */
#define PHASOR_PROBE_PRT 	0x20  /* channel 5 */
#define RTD_1SEC_PRT 	0x40  /* channel 6 */
#define TIME_PRT 	0x80  /* channel 7 */
#define PLL_PRT 	0x100  /* channel 8 */
#define CPU_BANDWIDTH_PRT 	0x200  /* channel 9 */
//#define SELECT_DEBUG_PRT 0x400  /* channel 10 */
#define GPS_NAV_DEBUG_PRT  0x400  /* channel 10 */
//#define PKT_DEBUG_PRT 0x800  /* channel 11 */
#define RATE_PRT 0x1000  /* channel 12 */
#define LOAD_PRT 0x2000  /* channel 13 */
//#define SEQ_PRT 0x4000  /* channel 14 */
#define KEN_PRT 0x8000  /* channel 15 */
#define CHAN_STAT_PRT 0x10000  /* channel 16 */
//#define TEST_PRT 0x20000  /* channel 17 */
//#define TSCK_PROBE_PRT 0x40000  /* channel 18 */
#define PTP_STACK_PRT 0x80000000  /* channel 31 */

#define UNMASK_PRT 0xFFFFFFFF  /* cannot be masked */

#define k_DEFAULT_DEBUG_MASK 0x0

#define DEBUG_BUFLEN    512

#define k_Interval_CPU_BANDWIDTH_PRT_NS    (UINT64)(1*1000000000)
/* for the threads that run at more than 1Hz this defines how often
 * to print the time usage in ns
 */

/*
 * Values included in kIsDiagnosticMessage will always generate a call to
 * debug_printf_function independently of the current value of get_debug_print()
 *
 * Values NOT included will only call debug_printf_function if that particular
 * debug flag is enabled.
 *
 */
#define kIsDiagnosticMessage (GEORGE_PTP_PRT | CLK_COR_PRT)


/*
 * Messages are divided in two main categories:
 *   * Diagnostic messages
 *      Messages that may be useful for end users and general troubleshooting.
 *      Having this messages ON should NOT affect the performance of the system.
 *
 *   * Debug messages
 *      Messages that are more useful during the development process.
 *      This messages because of the volume or the part of the code where they are
 *      generated may have potential effects in the performance of the system.
 *
 * Depending of the current messages threshold (set_debug_print(UINT32 mask))level
 * diagnostic messages are squelched just before they are printed, after all the
 * conversions to string and other computations are done to try to maintain the CPU
 * load of the system uniform independently of the printing threshold level.
 *
 * Debug messages are squelched as early as possible depending on the value of
 * set_debug_print(UINT32 mask) to try to maintain the CPU load as little as possible
 * when the messages are not enabled.
 *
 * This is implemented with two macros:
 *   debug_NeedToCallPrintf to determine if always call debug_printf_function
 *
 *   debug_printf that depending on debug_NeedToCallPrintf calls debug_printf_function
 *
 * In some cases debug_NeedToCallPrintf can be used as part of a conditional
 * expression.
 */

#define debug_NeedToCallPrintf(flag) (((flag) == UNMASK_PRT) || \
    ((flag) & (kIsDiagnosticMessage)) ||\
    ((flag) & get_debug_print()))

#define debug_printf(dbgLevel, ...) \
  ({ \
    if(debug_NeedToCallPrintf(dbgLevel)) \
      debug_printf_function((dbgLevel), __VA_ARGS__); \
  })


/*************************************************************************
**    debug timestamp mismatch default limits
*************************************************************************/
#define k_DEFAULT_DEBUG_TS_LIMIT 2000000

/***********************************************************************
**
** Function    : event_printf
**
** Description : This function prints an event message with severity
**               level.
**
** See Also    : debug_printf_function()
**
** Parameter   : event_id (IN) - event Id
**               pri (IN) - severity level
**               state (IN) - set (1) or clear (0)
**               fmtptr (IN) - pointer to printf format string
**               aux_info (IN) - additional information
**
** Returnvalue : SC_Log return
**
** Remarks     : This function coded by Symmetricom
**
***********************************************************************/
int event_printf(UINT16 event_id, t_eventSevEnum pri, t_eventStateEnum state, const char *fmt_ptr, INT32 aux_info);

/***********************************************************************
**
** Function    : debug_printf_function
**
** Description : This function prints the a debug message with a
**               qualifying debug mask.
**               This function is called by the macro debug_printf
**
** See Also    : set_debug_print()
**
** Parameter   : message_num (IN) - debug mask
**               fmtptr (IN) - pointer to printf format string
**
** Returnvalue : SC_Log return
**
** Remarks     : This function coded by Symmetricom
**
***********************************************************************/
int debug_printf_function(UINT32 message_num, const char *fmt_ptr, ...);

/***********************************************************************
**
** Function    : nm_debug_printf
**
** Description : This function prints debug message without masking.
**
** See Also    : set_debug_print()
**
** Parameter   : fmtptr (IN) - pointer to printf format string
**
** Returnvalue : vprintf return
**
** Remarks     : This function coded by Symmetricom
**
***********************************************************************/
int nm_debug_printf(const char *fmt_ptr, ...);

/***********************************************************************
**
** Function    : set_debug_print
**
** Description : This function sets the debug mask for debug_pintf()
**
** See Also    : debug_printf_function()
**
** Parameter   : mask - 32 bit debug mask
**
** Returnvalue : -
**
** Remarks     : This function coded by Symmetricom
**
***********************************************************************/
void set_debug_print(UINT32 mask);

/***********************************************************************
**
** Function    : get_debug_print
**
** Description : This function gets the debug mask for debug_pintf()
**
** See Also    : debug_printf_function()
**
** Parameter   : -
**
** Returnvalue : 32 bit debug mask
**
** Remarks     : This function coded by Symmetricom
**
***********************************************************************/
UINT32 get_debug_print(void);


#ifdef __cplusplus
}
#endif
#endif /* H_DBG_h */
