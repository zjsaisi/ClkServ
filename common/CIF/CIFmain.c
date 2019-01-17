/*************************************************************************
**    (C) 2008 - 2009 IXXAT Automation GmbH, all rights reserved
**************************************************************************
**
**       File: CIFmain.c 
**    Summary: CIF - Clock Interface
**             The unit CIF provides a generic interface to the system 
**             clock. It provides declarations of public functions and
**             definitions for the maintenance of the clock. This unit 
**             is responsible for setting, correcting and returning the 
**             system time. The implementation of these functions
**             depends on the hardware and on the development 
**             environment.
**
**    Version: 1.01.01
**
**************************************************************************
**************************************************************************
**             
**  Functions: CIF_Init
**             CIF_Close
**             CIF_AdjClkDrift
**             CIF_GetTimeStamp
**             CIF_SetSysTime
**             CIF_GetErrStr
**             SetError
**             
**   Compiler: gcc 3.3.2
**    Remarks: 
**
**************************************************************************
**    all rights reserved
*************************************************************************/


/*************************************************************************
**    compiler directives
*************************************************************************/


/*************************************************************************
**    include-files
*************************************************************************/
#include "target.h"
#include "sc_types.h"
#include "sc_api.h"
#include "PTP/PTPdef.h"
#include "CLK/sc_ptp_servo.h"
#include "GOE/GOE.h"
/* implementation specific */
#include "CIF/CIF.h"
#include "DBG/DBG.h"
//#include "TGT/TGT.h"

/*************************************************************************
**    global variables
*************************************************************************/
/*  system time variables */
/* error callback function */
static t_PTP_ERR_CLBK_FUNC CIF_pf_errClbk = NULL;
static INT16 CIF_i_curUtcOffs = k_CUR_UTC_OFFS;
static BOOLEAN o_lock_time_flag = FALSE;

/*************************************************************************
**    static function-prototypes
*************************************************************************/
#if 0
static void SetError(UINT32 dw_err,PTP_t_sevCdEnum e_sevCode );
#endif

extern int Try_SC_SystemTimeSet(PTP_t_TmStmp *ps_sysTimeTry, INT16 i_utcOffset);

/*************************************************************************
**    global functions
*************************************************************************/
/*************************************************************************
**
** Function    : SC_Lock_Time

**
** Description : This function will prevent the IXXAT stack from adjusting
**               the timestamper, which might have a drastic effect on the
**               clock outputs.
**   
** Parameters  : o_lock_time (IN) - set the lock if 1             
**
** Returnvalue : NONE              
**
** Remarks     : -
**
*************************************************************************/
void SC_Lock_Time(BOOLEAN o_lock_time)
{
   o_lock_time_flag = o_lock_time;
}

/*************************************************************************
**
** Function    : CIF_Init
**
** Description : This is the general initialization function of the 
**               clock interface. This function initializes the
**               interface using the local system clock. It is possible
**               that the PTP time must then be set to a default
**               value or to other values due to synchronization
**               to another source (e.g. GPS). All additional 
**               implementation-specific required system resources 
**               must be allocated.
**
**               The function parameter pf_errClbk is a pointer to 
**               the error callback function of the software stack. 
**               The initialization function of the unit CIF must 
**               copy this pointer to a local buffer, if unit CIF
**               wants to generate error messages for example during 
**               network-initialization.
**
** See Also    : CIF_Close()
**   
** Parameters  : pf_errClbk (IN) - error callback function             
**
** Returnvalue : TRUE            - function succeeded
**               FALSE           - function failed                 
**
** Remarks     : -
**
*************************************************************************/
BOOLEAN CIF_Init( t_PTP_ERR_CLBK_FUNC pf_errClbk )
{
  /* init error callback function */
  CIF_pf_errClbk = pf_errClbk;
  if( SC_InitClock() < 0 )
    return FALSE;
  else
    return TRUE;
}

/***********************************************************************
** 
** Function    : CIF_Close
**
** Description : This function is the closing function for the clock
**               interface. It closes all resources that have been 
**               initialized by the function {CIF_Init()}.
**
** See Also    : CIF_Init()
**
** Parameters  : -
**
** Returnvalue : -
**
** Remarks     : -
**
*************************************************************************/
void CIF_Close( void )
{
  /* reset error callback function */
  CIF_pf_errClbk = NULL;  
  /* reset original frequency compensation value */
  CIF_AdjClkDrift(0);/*lint !e534 !e747*/
}

/***********************************************************************
** 
** Function    : CIF_AdjClkDrift
**
** Description : This function adjusts the local clock drift. All slave 
**               nodes must synchronize their clocks to the current master 
**               clock. To do so, the clock implementation must continuous
**               adjust its clock drift to the master clock drift. 
**               Therefore, this function is called periodically (after a 
**               new master-to-slave delay is detected) to set a new 
**               correction value depending on the offset to the current 
**               master.
**
**               The parameter ll_corr_psec_per_sec is defined as the amount 
**               of picoseconds per second that must be added or subtracted 
**               from the local clock. The sign of this 64 bit number 
**               indicates the behavior of the correction. 
**               - A negative value requires the clock frequency to slow down.
**               - A positive value requires the clock frequency to speed up.
** 
**               The real correction value depends on the clock 
**               implementation. If the clock is not able to correct in the 
**               demanded accuracy of picoseconds, this function must 
**               approximate the correction value to fit the resolution of 
**               the implemented correction system.
**
** Parameters  : ll_corr_psec_per_sec (IN) - picoseconds to correct during a
**                                           second
**
** Returnvalue : TRUE                      - function succeeded
**               FALSE                     - function failed
**
** Remarks     : -
**
*************************************************************************/
BOOLEAN CIF_AdjClkDrift(FLOAT64 ll_corr_psec_per_sec )
{
//  FLOAT64 drift = ll_corr_psec_per_sec;
// only control EFC from CLK.c
//  if( SC_FreqeuencyCorrection(&drift, e_PARAM_SET) < 0 )
//    return FALSE;
//  else
    return TRUE;
}

/*************************************************************************
**
** Function    : CIF_GetTimeStamp
**
** Description : This function gets the current system time as a timestamp 
**               from the implemented system clock. The return value must 
**               contain the elapsed seconds and nanoseconds since start 
**               of the PTP system time in PTP-epoch. 
**
** Parameters  : ps_ts  (OUT) - pointer to the system-time in
**                              PTP Timestamp format
**
** Returnvalue : TRUE         - function succeeded
**               FALSE        - function failed
**
** Remarks     : Function is reentrant.
**
*************************************************************************/
BOOLEAN CIF_GetTimeStamp(PTP_t_TmStmp *ps_ts)
{
  INT16 d_utcOffset;

  if( SC_SystemTime((t_ptpTimeStampType*)ps_ts, &d_utcOffset, e_PARAM_GET) < 0 )
  {
    debug_printf(UNMASK_PRT, "Error while running SC_SystemTime()");
    return FALSE;
  }
  else
    return TRUE;
}

/***********************************************************************
**
** Function    : CIF_SetSysTime
**
** Description : This function is used to set the system time of the 
**               implemented clock with a given absolute value.
**
**               The real clock time depends on the clock implementation. 
**               If the clock is not able to set the time in the demanded 
**               accuracy of nanoseconds, this function has to approximate 
**               the system time value to the resolution of the implemented 
**               clock.
**
** Parameters  : ps_ts          (IN)  - pointer to the system-time in
**                                      PTP Timestamp format
**
** Returnvalue : TRUE                 - function succeeded
**               FALSE                - function failed
**
** Remarks     : Function is not reentrant.
**
*************************************************************************/
BOOLEAN CIF_SetSysTime(const PTP_t_TmStmp *ps_ts)
{  
/* if locked out then return */
   if(o_lock_time_flag)
   {
      return FALSE;
   }
  if( Try_SC_SystemTimeSet((PTP_t_TmStmp *)ps_ts, CIF_i_curUtcOffs) < 0 )
    return FALSE;
  else
    return TRUE;
}

/***********************************************************************
**
** Function    : CIF_SetSysUtcOffset
**
** Description : This function is used to set the system UTC offset.
**               The value is stored for later use. This function is
**               added so that it is compatible with Ixxat code.
**
** Parameters  : utcOffs        (IN)  - UTC offset
**
** Returnvalue : TRUE                 - function succeeded
**               FALSE                - function failed
**
** Remarks     : Function is not reentrant.
**
*************************************************************************/
BOOLEAN CIF_SetSysUtcOffset(INT16 utcOffs)
{  
  CIF_i_curUtcOffs = utcOffs;
  return TRUE;
}

#ifdef ERR_STR
/***********************************************************************
**  
** Function    : CIF_GetErrStr
**
** Description : This function assigns the error number to a corresponding 
**               error string. 
**
** Parameters  : dw_errNmb (IN) - error number code
**               
** Returnvalue : const CHAR*   - pointer to corresponding error string
**
** Remarks     : Is compiled with definition of ERR_STR.
**
***********************************************************************/
const CHAR* CIF_GetErrStr(UINT32 dw_errNmb)
{
  const CHAR* pc_ret = NULL;
  /* return according error string */ 
  switch(dw_errNmb)
  {
    case CIF_k_ERR_INIT_0:
    {
      pc_ret = "Initialization error 1";
      break;
    }
    case CIF_k_ERR_INIT_1:
    {
      pc_ret = "Initialization error 2";
      break;
    }
    case CIF_k_ERR_ADJ:
    {
      pc_ret = "Clock adjustment error";
      break;
    }
    case CIF_k_ERR_GET:
    {
      pc_ret = "Error getting time";
      break;
    }
    case CIF_k_ERR_SET:
    {
      pc_ret = "Error setting time";
      break;
    }
    case CIF_k_PPS:
    {
      pc_ret = "PPS error";
      break;
    }
    default:
    {
      pc_ret = "unknown error";
      break;
    }
  }
  return pc_ret;
}
#endif

/*************************************************************************
**    static functions
*************************************************************************/

/*************************************************************************
**
** Function    : SetError
**
** Description : This is the error function for the unit CIF.
**               It calls the error function of the stack.
**               This functions is registered in the function 
**               CIF_Init
**
** See Also    : CIF_Init()
**
** Parameters  : dw_err    (IN) - internal CIF error number
**               e_sevCode (IN) - severity code
**
** Returnvalue : -
**
** Remarks     : Implementation is CPU/compiler specific.
**
*************************************************************************/
#if 0
static void SetError(UINT32 dw_err,PTP_t_sevCdEnum e_sevCode )
{
  /* if error callback function initialized, call */
  if( CIF_pf_errClbk != NULL )
  {
    CIF_pf_errClbk(k_CIF_ERR_ID,dw_err,e_sevCode);
  }  
}
#endif

