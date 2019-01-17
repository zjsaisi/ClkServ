
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

FILE NAME    : sc_system.c

AUTHOR       : Ken Ho

DESCRIPTION  : 

The functions in this file are used to set up the SoftClient System.
	SC_GetRevInfo()
	SC_Init()
	SC_Run()
	SC_Shutdown()
	SC_Timer()


Revision control header:
$Id: SLV/SLVdel_queue.c 1.1 2011/03/02 13:20:34PST Kenneth Ho (kho) Exp  $

******************************************************************************
*/



/*--------------------------------------------------------------------------*/
/*                         INCLUDE FILES                                    */
/*--------------------------------------------------------------------------*/
#include "target.h"
#include "PTP/PTPdef.h"
#include "SIS/SIS.h"
#include "NIF/NIF.h"
#include "PTP/PTP.h"
#include "DIS/DIS.h"
#include "SLV/SLV.h"
#include "SLV/SLVint.h"
#include "ctype.h" // memset()
/*--------------------------------------------------------------------------*/
/*                              defines                                     */
/*--------------------------------------------------------------------------*/

#define DELAY_TIMESTAMP_BUF_SIZE 128

/*--------------------------------------------------------------------------*/
/*                              types                                       */
/*--------------------------------------------------------------------------*/

typedef struct {
	SLV_t_LastDlrq s_lastDlrq;
	UINT8  unused_flag;
} DELAY_TS_STRUCT;

typedef struct {
	DELAY_TS_STRUCT	Data[DELAY_TIMESTAMP_BUF_SIZE];/* buffer for writes to MMFR   (page + reg)  */
	UINT16 InIndex;             /* points to next   empty entry position    */ 
	UINT16 OutIndex;            /* points to oldest unused position         */ 
} DELAY_TS_BUF_STRUCT;

/*--------------------------------------------------------------------------*/
/*                              data                                        */
/*--------------------------------------------------------------------------*/
static DELAY_TS_BUF_STRUCT s_dl;
static BOOLEAN o_ranOnce = FALSE;

/*--------------------------------------------------------------------------*/
/*                              functions                                   */
/*--------------------------------------------------------------------------*/



/*
----------------------------------------------------------------------------
                                Put_Delay_Timestamp()

Description:

This function will place the delay request timestamps in a buffer to be
search when the delay response packets arrive.


Parameters:

Inputs
   SLV_t_LastDlrq *ps_lastDlrq
   Timestamp and sequence number data to be stored.
	
Outputs:
   None

Return value:
	None

-----------------------------------------------------------------------------
*/
void Put_Delay_Timestamp(SLV_t_LastDlrq *ps_lastDlrq)
{
   UINT16 index;

   if(o_ranOnce)
   {
      memset((void *)&s_dl, 0,sizeof(DELAY_TS_BUF_STRUCT));
      o_ranOnce = TRUE;
   }

	index = s_dl.InIndex;

	s_dl.Data[index].s_lastDlrq = *ps_lastDlrq;
   s_dl.Data[index].unused_flag = TRUE;

	if(++s_dl.InIndex >= DELAY_TIMESTAMP_BUF_SIZE)
	{
		s_dl.InIndex = 0;
	}
   return;
}
/*
----------------------------------------------------------------------------
                                Get_Delay_Timestamp()

Description:

This function will place the delay request timestamps in a buffer to be
search when the delay response packets arrive.


Parameters:

Inputs
   UINT16 seq_num
   Sequency number to use to search for timestamp data.

   SLV_t_LastDlrq *ps_lastDlrq
   copy the data into this address.
	
Outputs:
   None

Return value:
	1 = success
   0 = fail

-----------------------------------------------------------------------------
*/
UINT8 Get_Delay_Timestamp(UINT16 seq_num, SLV_t_LastDlrq *ps_lastDlrq)
{
	UINT8 status = 0;
	UINT16 i;

/* search list from In to Out Index */	

	for(i=0;i<DELAY_TIMESTAMP_BUF_SIZE;i++)
	{
		if((seq_num == s_dl.Data[i].s_lastDlrq.w_SeqId) && 
		   (s_dl.Data[i].unused_flag))
		{
			*ps_lastDlrq = s_dl.Data[i].s_lastDlrq;
			s_dl.OutIndex = i;
			s_dl.Data[i].unused_flag = FALSE;
			status = 1;
			break;
		}
	
	}
	return status;
}

