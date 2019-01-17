/*************************************************************************
**    (C) 2008 - 2009 IXXAT Automation GmbH, all rights reserved
**************************************************************************
**
**       File: PTP.c  
**    Summary: Super Unit for binding the IEEE1588 engine.
**             Functions for initializing and deinitializing the PTP 
**             engine and to get and set the system time by the application.
**
**    Version: 1.01.01
**
**************************************************************************
**************************************************************************
**
**  Functions: PTP_Init
**             PTP_Close
**             PTP_ReadClkDs
**             PTP_ChgFltCfg
**             PTP_Main
**             PTP_GetSysTime
**             PTP_SetSysTime
**             PTP_SetSysTimeOffset
**             PTP_SetSysTimeOffsSec
**             PTP_SetError
**             PTP_GetErrStr
**             
**   Compiler: Ansi-C
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
#include "PTP/PTPdef.h"
#include "PTP/PTPint.h"
#include "SIS/SIS.h"
#include "GOE/GOE.h"
#include "NIF/NIF.h"
#include "PTP/PTP.h"
#include "CIF/CIF.h"
#include "TSU/TSU.h"
#include "DIS/DIS.h"
#include "SLV/SLV.h"
#include "MST/MST.h"
#include "MNT/MNT.h"
#include "TCF/TCF.h"
#include "P2P/P2P.h"
#include "CTL/CTL.h"
#include "CTL/CTLflt.h"
#include "UCD/UCD.h"
#include "UCM/UCM.h"
#include "FIO/FIO.h"

/*************************************************************************
**    global variables
*************************************************************************/
/* stack engine status */
#define k_PTP_STS_STOP  (0u) /* stopped status */
#define k_PTP_STS_INIT  (1u) /* initializing status */
#define k_PTP_STS_RUN   (2u) /* running status */

/* pointer to application error callback function */
PTP_t_pfCbErr PTP_pf_cbErr;
 
UINT8  PTP_b_engSts = k_PTP_STS_STOP;
UINT32 PTP_dw_errCount;                   /* global error counter */ 
const PTP_t_ClkDs* ps_roClkDs = NULL;

/*************************************************************************
**    static constants, types, macros, variables
*************************************************************************/

/*************************************************************************
**    static function-prototypes
*************************************************************************/

/*************************************************************************
**    global functions
*************************************************************************/

/***********************************************************************
**
** Function    : PTP_Init
**
** Description : Initializes and starts the PTP engine. The return
**               value determines the unique clock id of the node.
**
** Parameters  : pf_cbErr       (IN) - pointer to callback function
**                                     that is called with all errors
**               ps_extTmPropDs (IN) - pointer to external time properties
**                                     data set provided by an external 
**                                     primary reference clock.
**                                     Must be NULL, if there is none.
**
** Returnvalue : <>NULL - PTP initialization started / pointer to clock id
**               ==NULL - PTP initialization failed
**                      
** Remarks     : -
**
***********************************************************************/
PTP_t_ClkId* PTP_Init(PTP_t_pfCbErr pf_cbErr, PTP_t_TmPropDs* ps_extTmPropDs)
{ 
  static PTP_t_ClkId s_clkId;
  PTP_t_ClkId *ps_ret;
  /* initialize callback function */
  PTP_pf_cbErr = pf_cbErr;
  /* change status to initializing */
  PTP_b_engSts = k_PTP_STS_INIT;

  /* set error counter to zero */
  PTP_dw_errCount = 0;
  /* Init Ctl */ 
  if( (ps_roClkDs = CTL_Init(ps_extTmPropDs,&s_clkId)) != NULL )
  {
    /* check, if error counter exceeded max errors at startup */
    if( PTP_dw_errCount > k_ERR_EXIT_THRESH )
    {
      /* stack startup was not OK */
      CTL_Close();
      /* change status to stopped */
      PTP_b_engSts = k_PTP_STS_STOP;
      ps_ret       = NULL;
    }
    else
    {
      /* stack startup was OK */
      ps_ret       = &s_clkId;
      /* change status to running */
      PTP_b_engSts = k_PTP_STS_RUN;
    }    
  }
  else
  {
    /* change status to stopped */
    PTP_b_engSts = k_PTP_STS_STOP;
    ps_ret       = NULL;
  }
  /* return initialization status */
  return ps_ret;
}

/***********************************************************************
** 
** Function    : PTP_Close
**
** Description : This function stops the PTP engine.
**
** Parameters  : -
**
** Returnvalue : -
**
** Remarks     : -
**
***********************************************************************/
void PTP_Close(void)
{
  /* stop stack, if not done yet */
  if( (PTP_b_engSts == k_PTP_STS_RUN) || (PTP_b_engSts == k_PTP_STS_INIT) )
  {
    /* set status to stopped */
    PTP_b_engSts = k_PTP_STS_STOP;
    /* close all open units by the closing function of the unit CTL */
    CTL_Close();
  }
  else
  {
    /* do nothing */   
  }
  /* deinit clock data set pointer */
  ps_roClkDs = NULL;
}


/***********************************************************************
**  
** Function    : PTP_ReadClkDs
**  
** Description : Gives read-only-access to the Clock Data set.
**               This function copies the actual clock data set
**               into the structure, passed by the application.
**               Use this function to get fast read access to the 
**               Clock data set. 
**               For write-access, the MNT API must be used.
**  
** Parameters  : ps_clkDs (OUT) - pointer to copy of clock data
**                                set. Function writes into this location.
**               
** Returnvalue : TRUE           - function succeeded
**               FALSE          - function failed
** 
** Remarks     : -
**  
***********************************************************************/
BOOLEAN PTP_ReadClkDs(PTP_t_ClkDs *ps_clkDs)
{
  void* pv_ctx;
  BOOLEAN o_ret;
  /* lock stack */
  pv_ctx = GOE_LockStack();
  
  /* check pointers */
  if((ps_clkDs != NULL) &&(ps_roClkDs != NULL))
  {
    /* copy clock data set */
    *ps_clkDs = *ps_roClkDs;
    o_ret = TRUE;
  }
  else
  {
    o_ret = FALSE;
  }
  /* unlock stack */
  GOE_UnLockStack(pv_ctx);
  return o_ret;
}

/***********************************************************************
**  
** Function    : PTP_ChgFltCfg
**  
** Description : Allows the user to register its own implementation
**               specific filter while runtime of the stack.
**               Additionally it is possible to change the filter 
**               to an internal filter implementation and to 
**               configure the filter.
**               The reconfiguration of the filter causes internal
**               reinitialization of the filter. The new settings for
**               the filter file are passed through the filter configuration
**               file. This file can be stored in non-volatile memory
**               to recover the filter settings after reboot of the 
**               device.
**               To use an internal filter, set the filter config file
**               with
**
**               filter type:
**               - e_FLT_NO  (no filtering)
**               - e_FLT_MIN (Minimum filter)
**               - e_FLT_EST (Estimator filter)
**
**               filter parameters: 
**               - Take a look into the manual for the filter configuration
**               description.
**
**               To use an external filter, set the filter configuration
**               file -> e_filtType to e_FILT_ISPCFC and pass the implementation
**               specific filter functions to register it in the
**               filter module.
** 
**               See CTLflt.h for the filter functions interface.
**  
** Parameters  : ps_fltCfg       (IN) - filter configuration file
**               o_strFltCfg     (IN) - flag to store the config file
**               pf_inpMtsdExt   (IN) - external input function for MTSD
**               pf_inpStmdExt   (IN) - external input function for STMD
**               pf_inpP2PdExt   (IN) - external input funciton for P2P del
**               pf_inpNewMstExt (IN) - external input function for 
**                                      new Master information
**               pf_rstFltExt    (IN) - extern filter reset function
**               
** Returnvalue : TRUE                 - function succeeded
**               FALSE                - function failed
** 
** Remarks     : -
**  
***********************************************************************/
BOOLEAN PTP_ChgFltCfg(const PTP_t_fltCfg    *ps_fltCfg,
                      BOOLEAN               o_strFltCfg,
                      PTP_t_FUNC_INP_MTSD   pf_inpMtsdExt,
                      PTP_t_FUNC_INP_STMD   pf_inpStmdExt,
                      PTP_t_FUNC_INP_P2P    pf_inpP2PdExt,
                      PTP_t_FUNC_INP_NEWMST pf_inpNewMstExt,
                      PTP_t_FUNC_FLT_RESET  pf_rstFltExt)
{
  void*   pv_ctx;
  BOOLEAN o_ret = FALSE;

  /* lock stack */
  pv_ctx = GOE_LockStack();

  /* stack internal filter */
  if((ps_fltCfg->e_usedFilter == e_FILT_NO ) ||
     (ps_fltCfg->e_usedFilter == e_FILT_MIN ) ||
     (ps_fltCfg->e_usedFilter == e_FILT_EST )) 
  {
    /* shall new configuration be stored ? */
    if( o_strFltCfg == TRUE )
    {
      /* write filter configuration */
      o_ret = FIO_WriteFltCfg(ps_fltCfg);
    }
    else
    { 
      o_ret = TRUE;
    }
    /* reinitialize filter configuration */
    CTLflt_Init(ps_fltCfg);
  }
  /* implementation specific */
  else
  {
    /* register new implementation specific filter */
    o_ret = CTLflt_chgFilter(pf_inpMtsdExt,
                             pf_inpStmdExt,
                             pf_inpP2PdExt,
                             pf_inpNewMstExt,
                             pf_rstFltExt);
  }
  /* unlock stack */
  GOE_UnLockStack(pv_ctx);
  return o_ret;
}

/***********************************************************************
**  
** Function    : PTP_Main
**  
** Description : The main function of the PTP engine must be called
**               cyclically within a interval of k_PTP_TIME_RES_NSEC.
**               The function must be called in the same thread-
**               context as PTP_Init and PTP_Close.
**  
** Parameters  : ps_tiActOffs (OUT) - current offset
**               
** Returnvalue : TRUE          - PTP engine is still running
**               FALSE         - PTP engine stopped
** 
** Remarks     : Function is not reentrant.
**  
***********************************************************************/
BOOLEAN PTP_Main( PTP_t_TmIntv *ps_tiActOffs )
{
  void *pv_ctx;
  BOOLEAN o_ret = FALSE;
  
  if( PTP_b_engSts == k_PTP_STS_RUN )
  {
    /* lock stack */
    pv_ctx = GOE_LockStack(); 
    /* call SIS Scheduler */
    SIS_Scheduler(); 
    /* get actual offset - the stack is not running 
       after SIS scheduler is ready */
#if((k_CLK_IS_OC == TRUE) || (k_CLK_IS_BC == TRUE))
    CTL_GetActOffs(ps_tiActOffs);
#else /* #if((k_CLK_IS_OC == TRUE) || (k_CLK_IS_BC == TRUE)) */
    ps_tiActOffs->ll_scld_Nsec = 0;
#endif /* #if((k_CLK_IS_OC == TRUE) || (k_CLK_IS_BC == TRUE)) */
    /* unlock stack */
    GOE_UnLockStack(pv_ctx);
    
    /* decrement error counter if greater than zero */
    if( PTP_dw_errCount > 0 )
    {
      PTP_dw_errCount--;
    }
    else
    {
      /* do nothing, no idemnification */
    }
    /* return running stack */
    o_ret = TRUE;
  }
  return o_ret;
}


/***********************************************************************
** 
** Function    : PTP_GetSysTime
**
** Description : This function gets the system time as a timestamp 
**               in PTP-epoch.
**
** Parameters  : ps_ts          (OUT) - pointer to the system-time in
**                                      PTP Timestamp format
**
** Returnvalue : TRUE                 - function succeeded
**               FALSE                - function failed
**
** Remarks     : Function is not reentrant.
**
*************************************************************************/
BOOLEAN PTP_GetSysTime(PTP_t_TmStmp *ps_ts)
{
  return CIF_GetTimeStamp(ps_ts);
}

/***********************************************************************
** 
** Function    : PTP_SetSysTime
**
** Description : This function sets the system time in PTP-epoch. 
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
BOOLEAN PTP_SetSysTime(const PTP_t_TmStmp *ps_ts)
{
  return CIF_SetSysTime(ps_ts);
}

/*************************************************************************
**
** Function    : PTP_SetSysTimeOffset
**
** Description : This function adds or subtracts a step offset to the
**               current system time. It is only used to correct offsets
**               between the master and slave clock, if these offsets
**               exceeds one second during the first synchronization or
**               after switching to a new master.
** 
**               The function parameter is not the absolute time to be set.
**               It represents the offset to the current master given
**               in PTP time interval (see also {PTP_t_TmIntv}). It
**               should be noted that a positive offset value describes
**               a slave time located within the future compared to the
**               master clock. Hence, a //positive offset// must be
**               //subtracted// from the current slave time. Likewise, a
**               //negative offset// value must be //added// to the 
**               current slave time.
**
**               //Remark//: To correct the clock drift during normal
**               operation the function {CIF_AdjClkDrift()} is used.
**
** See Also    : PTP_SetSysTimeOffsSec()
**
** Parameters  : ps_ti          (IN)  - offset to correct
**
** Returnvalue : TRUE                 - function succeeded
**               FALSE                - function failed
**
** Remarks     : Function is not reentrant.
**
*************************************************************************/
BOOLEAN PTP_SetSysTimeOffset(const PTP_t_TmIntv *ps_ti)
{
  BOOLEAN      o_ret = FALSE;
  PTP_t_TmStmp s_actTime,s_newTime;
  /* get act time */
  if( CIF_GetTimeStamp(&s_actTime) == TRUE )
  {
    /* subtract offset */
    if( PTP_SubtrTmIntvTmStmp(&s_actTime,ps_ti,&s_newTime) == TRUE )
    {
      /* set new system time */
      o_ret = CIF_SetSysTime(&s_newTime);
    }
  }
  return o_ret;
}

/*************************************************************************
**
** Function    : PTP_SetSysTimeOffsSec
**
** Description : This function adds or subtracts a step offset to the
**               current system time, if the offset between the master and
**               slave clock exceeds 140737 seconds. The time interval
**               data type in {PTP_SetSysTimeOffset()} is not sufficient
**               enough for that correction value and therefore this
**               function is used. The function parameter represents the
**               offset in seconds to be added or subtracted from the
**               current time.
**
**               It should be noted, that a positive offset value describes
**               a slave time, which is located within the future compared
**               to the master clock. Hence, a //positive offset// must
**               be //subtracted// from the current slave time. The other
**               way round, a //negative offset// value must be //added// 
**               to the current slave time.
**
**               //Remark//: To correct the clock drift during normal
**               operation the function {CIF_AdjClkDrift()} is used.
**
** See Also    : PTP_SetSysTimeOffset()
**
** Parameters  : pll_secOffs    (IN)  - second offset to correct
**
** Returnvalue : TRUE                 - function succeeded
**               FALSE                - function failed
**
** Remarks     : Function is not reentrant.
**
*************************************************************************/
BOOLEAN PTP_SetSysTimeOffsSec(const INT64* pll_secOffs)
{
  BOOLEAN      o_ret = FALSE;
  PTP_t_TmStmp s_actTime;
  /* get act time */
  if( CIF_GetTimeStamp(&s_actTime) == TRUE )
  {
    /* subtract offset */
    if( ((INT64)s_actTime.u48_sec) > *pll_secOffs )
    {
      s_actTime.u48_sec = (UINT64)((INT64)s_actTime.u48_sec - (*pll_secOffs));
    }
    else
    {
      s_actTime.dw_Nsec = 0;
    }
    /* set new system time */
    o_ret = CIF_SetSysTime(&s_actTime);
  }
  return o_ret;
}

/***********************************************************************
** 
** Function    : PTP_SetError
**
** Description : This function calls the application callback 
**               function and sends an error to the unit MNT to 
**               log the errors. 
**
** Parameters  : dw_unitId   (IN) - unit specific error identifier
**               dw_err      (IN) - error number
**               e_sevCode   (IN) - severity code of error
**
** Returnvalue : -
**
** Remarks     : -
**
*************************************************************************/
void PTP_SetError(UINT32          dw_unitId,
                  UINT32          dw_err,
                  PTP_t_sevCdEnum e_sevCode)
{
  PTP_t_MboxMsg s_msg;
  PTP_t_TmStmp  *ps_tsErr = NULL;

  /* alloc memory for error timestamp */
  ps_tsErr = (PTP_t_TmStmp*)SIS_Alloc(sizeof(PTP_t_TmStmp));
  if( ps_tsErr != NULL )
  {
    /* get error time */
    CIF_GetTimeStamp(ps_tsErr);/*lint !e534*/
  }
  /* call error callback function */
  if( PTP_pf_cbErr != NULL )
  {
    /* call applications error callback function */
    PTP_pf_cbErr(dw_unitId,dw_err,e_sevCode,ps_tsErr,
                 ((PTP_b_engSts==k_PTP_STS_INIT)||
                  (PTP_b_engSts==k_PTP_STS_RUN)));/*lint !e730*/
  }
  /* increment error count dependent from severity code */
  switch ( e_sevCode )
  {
    /* Emergency: system is unusable */
    case e_SEVC_EMRGC:
    {
      /* force reinitialization */
      PTP_dw_errCount += (k_ERR_REINIT_THRESH + 1);
      break;
    }
    /* Alert: immediate action needed */
    case e_SEVC_ALRT:
    {
      PTP_dw_errCount += k_WGHT_ERR_ALRT;
      break;
    }
    /* Critical: critical conditions */
    case e_SEVC_CRIT:
    {
      PTP_dw_errCount += k_WGHT_ERR_CRIT;
      break;
    }
    /* Error: error conditions */
    case e_SEVC_ERR:
    {
      PTP_dw_errCount += k_WGHT_ERR_ERR;
      break;
    }
    /* Warning: warning conditions */
    case e_SEVC_WARN:
    {
      PTP_dw_errCount += k_WGHT_ERR_WARN;
      break;
    }
    /* do not increment error count for the following error severity
        codes - no error condition */
    /* Notice: normal, but significant condition */
    case e_SEVC_NOTC: {break;}
    /* Informational: informational messages */
    case e_SEVC_INFO: {break;}
    /* Debug: debug level messages */
    case e_SEVC_DBG:  {break;}
    default:
    {
      /* shall not happen */
      break;
    }
  }
  /* if the stack is already completely initialized, 
     handle error internally */
  if( PTP_b_engSts == k_PTP_STS_RUN )
  {
    
    /* send error to management task for the fault log */
    s_msg.e_mType             = e_IMSG_NEW_FLT;
    s_msg.s_pntData.ps_tmStmp = ps_tsErr;
    s_msg.s_etc1.dw_u32       = dw_err;
    s_msg.s_etc2.dw_u32       = dw_unitId;
    s_msg.s_etc3.e_sevC       = e_sevCode;
    if( SIS_MboxPut(MNT_TSK,&s_msg) == FALSE )
    {
      /* call error callback function */
      if(PTP_pf_cbErr != NULL)
      {
        /* call at least application - error log is impossible */
        PTP_pf_cbErr(k_PTP_ERR_ID,
                     PTP_k_MSGBOX_MNT,
                     e_sevCode,
                     ps_tsErr,
                     TRUE);
      }
      /* free allocated memory */
      SIS_Free(ps_tsErr);
    }     
    if( PTP_dw_errCount > k_ERR_EXIT_THRESH )
    {
      /* stop engine */
      PTP_Close();     
    }
    else if( PTP_dw_errCount > k_ERR_REINIT_THRESH )
    {
      /* send message to CTL_Task to reinitialize engine */
      s_msg.e_mType = e_IMSG_INIT;
      if( SIS_MboxPut(CTL_TSK,&s_msg) == FALSE )
      {
        /* call application - error log is impossible */
        PTP_pf_cbErr(k_PTP_ERR_ID,
                     PTP_k_MSGBOX_CTL,
                     e_SEVC_ERR,
                     ps_tsErr,
                     TRUE);
        /* send error to management task for the fault log */
        s_msg.e_mType       = e_IMSG_NEW_FLT;
        s_msg.s_etc1.dw_u32 = PTP_k_MSGBOX_CTL;
        s_msg.s_etc2.dw_u32 = k_PTP_ERR_ID;
        s_msg.s_etc3.e_sevC = e_SEVC_ERR;
        if( SIS_MboxPut(MNT_TSK,&s_msg) == FALSE )
        {
          /* call error callback function */
          if(PTP_pf_cbErr != NULL)
          {
            /* call at least application - error log is impossible */
            PTP_pf_cbErr(k_PTP_ERR_ID,
                         PTP_k_MSGBOX_MNT,
                         e_sevCode,
                         ps_tsErr,
                         TRUE);
          }
        } 
      } 
      /* reset error counter */
      PTP_dw_errCount = 0;
    }  
    else
    {
      /* everything OK */
    }    
  }
  else
  {
    /* free allocated memory */
    SIS_Free(ps_tsErr);
  }
}

/*************************************************************************
**
** Function    : PTP_GetErrStr
**
** Description : This function resolves the error number to the 
**               corrospondent error string
**
** Parameters  : dw_unitId     (IN) - unit where error occured
**               dw_errNmb     (IN) - error number 
**               
** Returnvalue : const CHAR*        - pointer to error string
** 
** Remarks     : Gets compiled with definition of ERR_STR.
**  
***********************************************************************/
const CHAR* PTP_GetErrStr(UINT32 dw_unitId,UINT32 dw_errNmb)
{
  static CHAR ac_errStr[100];

  switch( dw_unitId )      
  {
    /* CIF errors */
    case k_CIF_ERR_ID:
    {
#ifdef ERR_STR
      PTP_SPRINTF((ac_errStr,"Unit CIF: %s",CIF_GetErrStr(dw_errNmb)));
#else /* #ifdef ERR_STR */
      PTP_SPRINTF((ac_errStr,"Unit CIF - Error #%d",dw_errNmb));
#endif /* #ifdef ERR_STR */
      break;
    }
    /* CTL errors */
    case k_CTL_ERR_ID:
    {
#ifdef ERR_STR
      PTP_SPRINTF((ac_errStr,"Unit CTL: %s",CTL_GetErrStr(dw_errNmb)));
#else /* #ifdef ERR_STR */
      PTP_SPRINTF((ac_errStr,"Unit CTL - Error #%d",dw_errNmb));
#endif /* #ifdef ERR_STR */
      break;
    }
    /* DIS errors */
    case k_DIS_ERR_ID:
    {
#ifdef ERR_STR
      PTP_SPRINTF((ac_errStr,"Unit DIS: %s",DIS_GetErrStr(dw_errNmb)));
#else /* #ifdef ERR_STR */
      PTP_SPRINTF((ac_errStr,"Unit DIS - Error #%d",dw_errNmb));
#endif /* #ifdef ERR_STR */
      break;
    }
    /* MNT errors */
    case k_MNT_ERR_ID:
    {
#ifdef ERR_STR
      PTP_SPRINTF((ac_errStr,"Unit MNT: %s",MNT_GetErrStr(dw_errNmb)));
#else /* #ifdef ERR_STR */
      PTP_SPRINTF((ac_errStr,"Unit MNT - Error #%d",dw_errNmb));
#endif /* #ifdef ERR_STR */
      break;
    }
    /* PTP errors */
    case k_PTP_ERR_ID:
    {      
#ifdef ERR_STR
      /* return according error string */
      switch(dw_errNmb)
      {
        case PTP_k_MSGBOX_MNT:
        {
          PTP_SPRINTF((ac_errStr,"Unit PTP: MNT messagebox full"));
          break;
        }
        case PTP_k_MSGBOX_CTL:
        {
          PTP_SPRINTF((ac_errStr,"Unit PTP: CTL messagebox full"));
          break;
        }
        case PTP_k_ERR_INTV:
        {
          PTP_SPRINTF((ac_errStr,"Unit PTP: Interval too short or too big"));
          break;
        }
        case PTP_k_ERR_CLIP_I64:
        {
          PTP_SPRINTF((ac_errStr,"Unit PTP: Clipping in INT64 add function"));
          break;
        }
        default:
        {
          PTP_SPRINTF((ac_errStr,"Unit PTP: unknown error"));
          break;
        }
      }
#else /* #ifdef ERR_STR */
      PTP_SPRINTF((ac_errStr,"Unit PTP - Error #%d",dw_errNmb));
#endif /* #ifdef ERR_STR */

      break;
    }
#if((k_CLK_IS_OC) || (k_CLK_IS_BC))
    /* MST errors */
    case k_MST_ERR_ID:
    {
#ifdef ERR_STR
      PTP_SPRINTF((ac_errStr,"Unit MST: %s",MST_GetErrStr(dw_errNmb)));
#else /* #ifdef ERR_STR */
      PTP_SPRINTF((ac_errStr,"Unit MST - Error #%d",dw_errNmb));
#endif /* #ifdef ERR_STR */
      break;
    }
    /* SLV errors */
    case k_SLV_ERR_ID:
    {
#ifdef ERR_STR
      PTP_SPRINTF((ac_errStr,"Unit SLV: %s",SLV_GetErrStr(dw_errNmb)));
#else /* #ifdef ERR_STR */
      PTP_SPRINTF((ac_errStr,"Unit SLV - Error #%d",dw_errNmb));
#endif /* #ifdef ERR_STR */
      break;
    }
#endif /* #if(((k_CLK_IS_OC) || (k_CLK_IS_BC)) */
    /* TSU errors */
    case k_TSU_ERR_ID:
    {
#ifdef ERR_STR
      PTP_SPRINTF((ac_errStr,"Unit TSU: %s",TSU_GetErrStr(dw_errNmb)));
#else /* #ifdef ERR_STR */
      PTP_SPRINTF((ac_errStr,"Unit TSU - Error #%d",dw_errNmb));
#endif /* #ifdef ERR_STR */
      break;
    }
    /* GOE errors */
    case k_GOE_ERR_ID:
    {
#ifdef ERR_STR
      PTP_SPRINTF((ac_errStr,"Unit GOE: %s",GOE_GetErrStr(dw_errNmb)));
#else /* #ifdef ERR_STR */
      PTP_SPRINTF((ac_errStr,"Unit GOE - Error #%d",dw_errNmb));
#endif /* #ifdef ERR_STR */
      break;
    }
#if( k_CLK_IS_TC == TRUE )
    /* TCF errors */
    case k_TCF_ERR_ID:
    {
#ifdef ERR_STR
      PTP_SPRINTF((ac_errStr,"Unit TCF: %s",TCF_GetErrStr(dw_errNmb)));
#else /* #ifdef ERR_STR */
      PTP_SPRINTF((ac_errStr,"Unit TCF - Error #%d",dw_errNmb));
#endif /* #ifdef ERR_STR */
      break;
    }
#endif /* #if( k_CLK_IS_TC == TRUE ) */
#if( k_CLK_DEL_P2P == TRUE )
    /* P2P errors */
     case k_P2P_ERR_ID:
    {
#ifdef ERR_STR
      PTP_SPRINTF((ac_errStr,"Unit P2P: %s",P2P_GetErrStr(dw_errNmb)));
#else /* #ifdef ERR_STR */
      PTP_SPRINTF((ac_errStr,"Unit P2P - Error #%d",dw_errNmb));
#endif /* #ifdef ERR_STR */
      break;
    }
#endif
/* just for unicast enabled */
#if((k_UNICAST_CPBL == TRUE) && ((k_CLK_IS_OC) || (k_CLK_IS_BC)))
    case k_UCM_ERR_ID:
    {
#ifdef ERR_STR
      PTP_SPRINTF((ac_errStr,"Unit UCM: %s",UCM_GetErrStr(dw_errNmb)));
#else /* #ifdef ERR_STR */
      PTP_SPRINTF((ac_errStr,"Unit UCM - Error #%d",dw_errNmb));
#endif /* #ifdef ERR_STR */
      break;
    }
    case k_UCD_ERR_ID:
    {
#ifdef ERR_STR
      PTP_SPRINTF((ac_errStr,"Unit UCD: %s",UCD_GetErrStr(dw_errNmb)));
#else /* #ifdef ERR_STR */
      PTP_SPRINTF((ac_errStr,"Unit UCD - Error #%d",dw_errNmb));
#endif /* #ifdef ERR_STR */
      break;
    }
#endif /* #if((k_UNICAST_CPBL == TRUE) && ((k_CLK_IS_OC) || (k_CLK_IS_BC))) */
    case k_FIO_ERR_ID:
    {
#ifdef ERR_STR
      PTP_SPRINTF((ac_errStr,"Unit FIO: %s",FIO_GetErrStr(dw_errNmb)));
#else /* #ifdef ERR_STR */
      PTP_SPRINTF((ac_errStr,"Unit FIO - Error #%d",dw_errNmb));
#endif /* #ifdef ERR_STR */
      break;
    }
    default:
    {
      PTP_SPRINTF((ac_errStr,"Unknown error Unit-Nmb %u Error-Nmb %u",
                                                      dw_unitId,dw_errNmb));
    }
  }
  return ac_errStr;
}

/*************************************************************************
**    static functions
*************************************************************************/

