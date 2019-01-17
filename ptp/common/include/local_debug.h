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

FILE NAME    : local_debug.h

AUTHOR       : Jay Wang

DESCRIPTION  : 

This header file contains various local debug setting.

Revision control header:
$Id: local_debug.h 1.1 2010/08/06 13:36:23PDT Jay Wang (Jwang) Exp  $

******************************************************************************
*/

/* Prevent file from being included multiple times. */
#ifndef H_INTERNAL_DEBUG_h
#define H_INTERNAL_DEBUG_h

/* deb_mask definition */
#define LOCAL_DEB_FC_TRACE_MASK		0x01
#define LOCAL_DEB_RXTS_TRACE_MASK	0x02
#define LOCAL_DEB_TXTS_TRACE_MASK	0x04
#define LOCAL_DEB_PH_TRACE_MASK		0x08

typedef struct
{
	char	log_disp_on_console;	/* 0=no, 1= yes */
	char	one_sec_trace;	/* 0=no, 1= yes */
	char	force_fc_zero;	/* 0=no, 1= yes */
	unsigned int	deb_mask;	/* 0x0 = clear, see above definition */
} t_LocalDebugType;

extern t_LocalDebugType s_localDebCfg;

#endif

