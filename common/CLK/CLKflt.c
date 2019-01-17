
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

FILE NAME    : CLKflt.c

AUTHOR       : Ken Ho

DESCRIPTION  : 

These functions are used to move timestamp data into the PTP servo task.

  Functions: SC_ClkFltInit
             CLKflt_inpMTSD
             CLKflt_inpSTMD
             CLKflt_inpP2Pdel
             CLKflt_inpNewMst
             CLKflt_resetFilter

Revision control header:
$Id: CLK/CLKflt.c 1.22 2011/06/29 10:24:55PDT Kenneth Ho (kho) Exp  $

******************************************************************************
*/



/*--------------------------------------------------------------------------*/
/*                         INCLUDE FILES                                    */
/*--------------------------------------------------------------------------*/
#include <stdio.h>
#include "target.h"
#include "sc_types.h"
#include "sc_servo_api.h"
#include "clk_private.h"
#include "sc_ptp_servo.h"
#include "ptp.h"
#include "CLKflt.h"
#include "DBG/DBG.h"


/*--------------------------------------------------------------------------*/
/*                              defines                                     */
/*--------------------------------------------------------------------------*/
#define IF_INDEX 0



/*--------------------------------------------------------------------------*/
/*                              types                                       */
/*--------------------------------------------------------------------------*/


/*--------------------------------------------------------------------------*/
/*                              data                                        */
/*--------------------------------------------------------------------------*/

static int counter_m2s;
static int counter_s2m;
//static INT64 ll_timestamp_offset = 0;
//static INT32 current_rtd_1sec_min = 0;

/*--------------------------------------------------------------------------*/
/*                              functions                                   */
/*--------------------------------------------------------------------------*/

/*
----------------------------------------------------------------------------
                                TemplateFunction

Description...


Parameters:

Return value:

-----------------------------------------------------------------------------
*/

/***********************************************************************
**  
** Function    : FLTno_Init
**  
** Description : Initialization function for this filter implementation.
**               Initializes the function pointers for the input functions
**               of this filter implementation.
**  
** Parameters  : ps_fltCfg      (IN) - filter configuration file
**               ppf_inpMtsd   (OUT) - input MTSD function-pointer
**               ppf_inpStmd   (OUT) - input STMD function-pointer
**               ppf_inpP2P    (OUT) - input P2P delay function-pointer
**               ppf_inpNewMst (OUT) - input function for new master
**               ppf_fltReset  (OUT) - reset function
**               
** Returnvalue : TRUE  - filter initialization succeeded
**               FALSE - filter initialization failed
** 
** Remarks     : -
**  
***********************************************************************/
BOOLEAN SC_ClkFltInit(const t_fltCfg    *ps_fltCfg,
                      t_FUNC_INP_MTSD   *ppf_inpMtsd,
                      t_FUNC_INP_STMD   *ppf_inpStmd,
                      t_FUNC_INP_P2P    *ppf_inpP2P,
                      t_FUNC_INP_NEWMST *ppf_inpNewMst,
                      t_FUNC_FLT_RESET  *ppf_fltReset)
{
  /* unused variables */
  ps_fltCfg = ps_fltCfg;

  /* set function pointers */
  *ppf_inpMtsd   = CLKflt_inpMTSD;
  *ppf_inpStmd   = CLKflt_inpSTMD;
  *ppf_inpP2P    = CLKflt_inpP2Pdel;
  *ppf_inpNewMst = CLKflt_inpNewMst;
  *ppf_fltReset  = CLKflt_resetFilter;

  /* call reset function */
  CLKflt_resetFilter();
  /* always return TRUE - this implementation cannot fail */

  return TRUE;
}

/***********************************************************************
**  
** Function    : FLTno_inpMTSD
**  
** Description : Input function for the master-to-slave-delay measurements.
**               The function stores the actual MTSD. Later this 
**               measurement is used to determine the STMD, if E2E-delay
**               measurement is used. The function returns the 
**               offset-from-master value. Therefore, the last computed
**               one-way-delay or P2P-path-delay is subtracted from the
**               actual MTSD.
**  
** Parameters  : ps_tiMtsd         (IN) - MTSD input
**               ps_txTsSyn        (IN) - origin timestamp of sync message
**               ps_rxTsSyn        (IN) - receive timestamp of sync message
**               w_ifIdx           (IN) - comm. interface index of measurement
**               dw_TaSynIntvUsec  (IN) - sync interval in usec
**               ps_tiOffset      (OUT) - actual offset from master
**               pdw_TaOffsUsec   (OUT) - trigger interval for PI contr. in usec
**               ps_tiFltMtsd     (OUT) - filtered MTSD value
**               pb_mode          (OUT) - filter mode (HOLDOVER/RUNNING)
**               
** Returnvalue : TRUE  - trigger actual value into PI controller
**               FALSE - do not trigger actual value into PI controller
** 
** Remarks     : -
**  
***********************************************************************/
BOOLEAN CLKflt_inpMTSD(const t_TmIntv           *ps_tiMtsd,
                       const t_ptpTimeStampType *ps_txTsSyn,
                       const t_ptpTimeStampType *ps_rxTsSyn,
                       UINT16                   w_ifIdx,
                       UINT32                   dw_TaSynIntvUsec,
                       t_TmIntv                 *ps_tiOffset,
                       UINT32                   *pdw_TaOffsUsec,
                       t_TmIntv                 *ps_tiFltMtsd,
                       UINT8                    *pb_mode)
{
  t_timestampPairType s_timestampPair;

  counter_m2s++;


  s_timestampPair.s_masterTS.u48_sec = ps_txTsSyn->u48_sec;      /* timestamp from master (t1 or t4) */
  s_timestampPair.s_slaveTS.u48_sec = ps_rxTsSyn->u48_sec;       /* timestamp from slave  (t2 or t3) */
  s_timestampPair.s_masterTS.dw_nsec = ps_txTsSyn->dw_nsec;      /* timestamp from master (t1 or t4) */
  s_timestampPair.s_slaveTS.dw_nsec = ps_rxTsSyn->dw_nsec;       /* timestamp from slave  (t2 or t3) */

//  s_timestampPair.s_masterTS.u48_sec = ps_rxTsSyn->u48_sec;      /* timestamp from master (t1 or t4) */
//  s_timestampPair.s_slaveTS.u48_sec = ps_txTsSyn->u48_sec;       /* timestamp from slave  (t2 or t3) */
//  s_timestampPair.s_masterTS.dw_nsec = ps_rxTsSyn->dw_nsec;      /* timestamp from master (t1 or t4) */
//  s_timestampPair.s_slaveTS.dw_nsec = ps_txTsSyn->dw_nsec;       /* timestamp from slave  (t2 or t3) */

  s_timestampPair.w_seqNum = 0;        /* packet sequence number           */
  s_timestampPair.w_channel = 0;       /* channel number  0                 */
  s_timestampPair.e_pairDir = e_SYNC_PAIR;

  //   Transfer_PTP_data_to_servo(PTP_SYNC_TRANSFER_QUEUE , &s_Fifo_element);
  SC_TimestampPairToServo(0, &s_timestampPair);

  return TRUE;
}

/***********************************************************************
**  
** Function    : FLTno_inpSTMD
**  
** Description : Input function for the slave-to-master-delay measurements.
**               The function calculates the actual one-way-delay. Later this 
**               measurement is used to determine the offset-from-master, 
**               if E2E-delay measurement is used. 
**  
** Parameters  : ps_tiStmd      (IN) - input STMD measurement
**               ps_rxTsDlrsp   (IN) - rx timestamp of delay response
**               ps_txTsDlrq    (IN) - tx timestamp of delay request
**               dw_TaDlrqIntv  (IN) - mean time interval of Dreq in usec
**               ps_tiFltStmd  (OUT) - filtered STMD
**               ps_tiFltOwd   (OUT) - filtered OWD
**               pb_mode       (OUT) - filter mode (HOLDOVER/RUNNING)
**               
** Returnvalue : -
** 
** Remarks     : -
**  
***********************************************************************/
void CLKflt_inpSTMD(const t_TmIntv           *ps_tiStmd,
                    const t_ptpTimeStampType *ps_rxTsDlrsp,
                    const t_ptpTimeStampType *ps_txTsDlrq,
                    UINT32                   dw_TaDlrqIntv,
                    t_TmIntv                 *ps_tiFltStmd,
                    t_TmIntv                 *ps_tiFltOwd,
                    UINT8                    *pb_mode)
{
  t_timestampPairType s_timestampPair;

  counter_s2m++;
 
  s_timestampPair.s_masterTS.u48_sec = ps_rxTsDlrsp->u48_sec;      /* timestamp from master (t1 or t4) */
  s_timestampPair.s_slaveTS.u48_sec = ps_txTsDlrq->u48_sec;       /* timestamp from slave  (t2 or t3) */
  s_timestampPair.s_masterTS.dw_nsec = ps_rxTsDlrsp->dw_nsec;      /* timestamp from master (t1 or t4) */
  s_timestampPair.s_slaveTS.dw_nsec =ps_txTsDlrq->dw_nsec;       /* timestamp from slave  (t2 or t3) */
  s_timestampPair.w_seqNum = 0;        /* packet sequence number           */
  s_timestampPair.w_channel = 0;       /* channel number  0                 */
  s_timestampPair.e_pairDir = e_DELAY_PAIR;

  //   Transfer_PTP_data_to_servo(PTP_SYNC_TRANSFER_QUEUE , &s_Fifo_element);
  SC_TimestampPairToServo(0, &s_timestampPair);

  return;
}

/***********************************************************************
**  
** Function    : FLTno_inpP2Pdel
**  
** Description : Input function for the peer-to-peer-delay measurements.
**               The function stores the actual peer-to-peer-delay. Later this 
**               measurement is used to determine the offset-from-master, 
**               if P2P-delay measurement is used. 
**  
** Parameters  : ps_tiP2PDel       (IN) - actual measured P2P delay
**               ps_txTsPDlrq      (IN) - tx timestamp of Pdel-req
**               ps_rxTsPDlrsp     (IN) - rx timestamp of Pdel-resp (corrected)
**               w_ifIdx           (IN) - comm. interface index of measurement
**               dw_TaP2PIntvUsec  (IN) - mean time interval of P2P in usec
**               ps_tiFltP2Pdel   (OUT) - filtered P2P delay
**               pb_mode          (OUT) - filter mode (HOLDOVER/RUNNING)
**               
** Returnvalue : -
** 
** Remarks     : -
**  
***********************************************************************/
void CLKflt_inpP2Pdel(const t_TmIntv           *ps_tiP2PDel,
                      const t_ptpTimeStampType *ps_txTsPDlrq,
                      const t_ptpTimeStampType *ps_rxTsPDlrsp,
                      UINT16                   w_ifIdx,
                      UINT32                   dw_TaP2PIntvUsec,
                      t_TmIntv                 *ps_tiFltP2Pdel,
                      UINT8                    *pb_mode)
{
#if 0
  /* unused variables in this implementation */
  ps_txTsPDlrq     = ps_txTsPDlrq;
  ps_rxTsPDlrsp    = ps_rxTsPDlrsp;
  dw_TaP2PIntvUsec = dw_TaP2PIntvUsec;
  /* store peer delay */
  as_tiPeerDelay[w_ifIdx] = *ps_tiP2PDel;
  /* return filtered peer delay */
  *ps_tiFltP2Pdel = *ps_tiP2PDel;
  /* mode is always 'RUNNING' */
  *pb_mode = k_RUNNING;  
#endif
}

/***********************************************************************
**  
** Function    : FLTno_inpNewMst
**  
** Description : Informs the filter of a change to a new master
**  
** Parameters  : ps_pIdNewMst   (IN/OUT) - Port ID of new master
**               ps_pAddrNewMst (IN/OUT) - Port Address of new master
**               o_mstIsUnicast (IN/OUT) - flag determines, if master is unicast
**               
** Returnvalue : -
** 
** Remarks     : -
**  
***********************************************************************/
void CLKflt_inpNewMst(const void *ps_pIdNewMst,
                      const void *ps_pAddrNewMst,
                      BOOLEAN    o_mstIsUnicast)
{
#if 0
  /* unused variables in this implementation */
  ps_pIdNewMst   = ps_pIdNewMst;
  ps_pAddrNewMst = ps_pAddrNewMst;
  o_mstIsUnicast = o_mstIsUnicast;
  /* just reset filter */
  FLTno_resetFilter();
#endif
}

/***********************************************************************
**  
** Function    : FLTno_resetFilter
**  
** Description : Resets the filter
**  
** Parameters  : -
**               
** Returnvalue : -
** 
** Remarks     : -
**  
***********************************************************************/
void CLKflt_resetFilter( void )
{
#if 0
  /* initialize old MTSD and STMD values */
  s_tiLastMtsd.ll_scld_Nsec = (INT64)0;
  s_tiOwd.ll_scld_Nsec      = (INT64)0;
#endif
}

void Get_Clear_Counters(int *m2s, int *s2m)
{
   *s2m = counter_s2m;
   *m2s = counter_m2s;

   counter_s2m = 0;
   counter_m2s = 0;
}



/*
----------------------------------------------------------------------------
                                Get_RTD_1SEC_MIN

This function returns the RTD (Round Trip Delay) based on the last 1 second
samples.

Parameters:
IN:
   N/A
OUT:
	N/A


Return value:
	INT32 rtd value in nanoseconds, if 0 then no rtd for this second

-----------------------------------------------------------------------------
*/ 
//INT32 Get_RTD_1sec_min(void)
//{
//   return current_rtd_1sec_min;
//

/*
----------------------------------------------------------------------------
                                SC_newGrandmaster()

Description:
This function informs the PTP servo that a new grandmaster is supplying the
timestamps.

Parameters:

Inputs
	None

Outputs:
	None

Return value:
   None
-----------------------------------------------------------------------------
*/
void SC_NewGrandmaster(void)
{
   return;
}

/*
----------------------------------------------------------------------------
                                SC_GetFltCfg()

Description:
This function replaces FIO_ReadFltCfg function.

Parameters:

Inputs
        None

Outputs:
        ps_cfgFlt
        Pointer to structure the filter structure to be written to.

Return value:
        TRUE:  function succeeded
        FALSE: function failed

-----------------------------------------------------------------------------
*/
BOOLEAN SC_GetFltCfg(t_fltCfg* ps_cfgFlt)
{
        ps_cfgFlt->e_usedFilter = SC_e_FILT_NO;
        ps_cfgFlt->s_filtMin.dw_filtLen = 4;
        ps_cfgFlt->s_filtEst.b_histLen = 10;
        ps_cfgFlt->s_filtEst.ddw_WhtNoise = 1500;
        ps_cfgFlt->s_filtEst.b_amntFlt = 1;
        return TRUE;
}

