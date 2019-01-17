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

FILE NAME    : sc_timestamp.h

AUTHOR       : Jining Yang

DESCRIPTION  : 

This header file contains the definitions related to MPC8313 timestamper.

Revision control header:
$Id: timestamper_if/sc_timestamp.h 1.1 2009/10/22 17:58:24PDT jyang Exp  $

******************************************************************************
*/

/* Prevent file from being included multiple times. */
#ifndef H_SC_TIMESTAMP_h
#define H_SC_TIMESTAMP_h

#ifdef __cplusplus
extern "C"
{
#endif

#include <linux/sockios.h>
#include <net/if.h>

#define k_NSEC_IN_SEC      1000000000

#define PTP_GET_RX_TIMESTAMP_SYNC       (SIOCDEVPRIVATE)
#define PTP_GET_RX_TIMESTAMP_DEL_REQ    (SIOCDEVPRIVATE + 1)
#define PTP_GET_RX_TIMESTAMP_FOLLOWUP   (SIOCDEVPRIVATE + 2)
#define PTP_GET_RX_TIMESTAMP_DEL_RESP   (SIOCDEVPRIVATE + 3)
#define PTP_GET_TX_TIMESTAMP            (SIOCDEVPRIVATE + 4)
#define PTP_SET_CNT                     (SIOCDEVPRIVATE + 5)
#define PTP_GET_CNT                     (SIOCDEVPRIVATE + 6)
#define PTP_SET_FIPER_ALARM             (SIOCDEVPRIVATE + 7)
#define PTP_ADJ_ADDEND                  (SIOCDEVPRIVATE + 9)
#define PTP_GET_ADDEND                  (SIOCDEVPRIVATE + 10)
#define PTP_GET_RX_TIMESTAMP_PDELAY_REQ (SIOCDEVPRIVATE + 11)
#define PTP_GET_RX_TIMESTAMP_PDELAY_RESP (SIOCDEVPRIVATE + 12)
#define PTP_CLEANUP_TIMESTAMP_BUFFERS   (SIOCDEVPRIVATE + 13)

/*
----------------------------------------------------------------------------
Description:
This struct defines the time format used in MPC8313 time stamper.
-----------------------------------------------------------------------------
*/
typedef struct
{
  UINT32  dw_high;
  UINT32  dw_low;
} t_8313TimeFormat;

#ifdef __cplusplus
}
#endif

#endif /* H_SC_TIMESTAMP_h */

