/*****************************************************************************
*                                                                            *
*   Copyright (C) 2010 Symmetricom, Inc., all rights reserved                *
*                                                                            *
*   This software contains proprietary and confidential information of       *
*   Symmetricom.                                                             *
*   It may not be reproduced, used, or disclosed to others for any purpose   *
*   without the written authorization of Symmetricom.                        *
*                                                                            *
******************************************************************************

FILE NAME    : sc_clock_if.h

AUTHOR       : Ken Ho

DESCRIPTION  : 

This header file contains the definitions related to clock interface.

Revision control header:
$Id: clock_if/sc_clock_if.h 1.9 2011/10/31 12:00:38PDT German Alvarez (galvarez) Exp  $

******************************************************************************
*/

/* Prevent file from being included multiple times. */
#ifndef H_SC_CLOCK_IF_h
#define H_SC_CLOCK_IF_h

#ifdef __cplusplus
extern "C"
{
#endif

/* Currently used phase mode */
#define k_PHASE_MODE      k_PTP_PHASE_MODE_INBAND | k_PTP_PHASE_MODE_STANDARD


extern double SC_WarpFrequency(double df_fin);
extern void putPhaseMode(UINT32 dw_putPhaseMode);
extern int SC_GetPhasorOffset(INT32 *i32_phaseOffset, UINT32 dw_fpgaOffset);
extern int SC_GetPhasorOffset1(INT32 *i32_phaseOffset, UINT32 dw_fpgaOffset);
extern int SC_GetPhasorOffset2(INT32 *i32_phaseOffset, UINT32 dw_fpgaOffset);
extern int SC_GetRefPhaseOffset(INT32 *i32_phaseOffset);
extern int SC_GetPpsPhaseOffset(INT32 *i32_phaseOffset);
extern UINT8 SC_GetPhasorLosCntr(void);
extern UINT8 SC_GetExtRefPhasorLosCntr(void);
extern int setExtRefDivider(BOOLEAN o_isRefA, UINT32 dw_divider);
extern int setE1T1OutEnable(UINT8 enable);
extern UINT32 setExtOutFreq(UINT32 freq);

#ifdef __cplusplus
}
#endif

#endif /* H_SC_CLOCK_IF_h */

