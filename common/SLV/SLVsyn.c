/*************************************************************************
**    (C) 2008 - 2009 IXXAT Automation GmbH, all rights reserved
**************************************************************************
**
**       File: SLVsyn.c 
**
**    Summary: Synchronizing slave task module. 
**
**    Version: 1.01.01
**
**************************************************************************
**************************************************************************
**
**  Functions: SLV_SetSyncInt
**             SLV_SyncTask
**             Handle_Sync
**             Handle_Follow_up
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
#include "SIS/SIS.h"
#include "NIF/NIF.h"
#include "PTP/PTP.h"
#include "SLV/SLV.h"
#include "SLV/SLVint.h"

/* not used for TCs without OC */
#if(( k_CLK_IS_OC == TRUE)||(k_CLK_IS_BC))
/*************************************************************************
**    global variables
*************************************************************************/
/* the actual sync interval timeout */
static UINT32 SLV_adw_SynIntvTmo[k_NUM_IF];
/* sync receive timeout */
static UINT32 SLV_adw_SynRcvTmo[k_NUM_IF];

/*************************************************************************
**    static constants, types, macros, variables
*************************************************************************/

/*************************************************************************
**    static function-prototypes
*************************************************************************/
static BOOLEAN Handle_Sync(const NIF_t_PTPV2_SyncMsg *ps_msgSyn,
                           const PTP_t_TmStmp        *ps_rxTs,
                           UINT16                    w_ifIdx,
                           SLV_t_LastSyn             *ps_lastSync,
                           BOOLEAN                   *po_rct);
static BOOLEAN Handle_Follow_up(const NIF_t_PTPV2_FlwUpMsg *ps_msgFlwUp,
                                const SLV_t_LastSyn        *ps_lastSync,
                                BOOLEAN                    *po_rct,
                                UINT16                     w_actIfIdx);  

/*************************************************************************
**    global functions
*************************************************************************/

/***********************************************************************
**
** Function    : SLV_SetSyncInt
**
** Description : Set´s the exponent for calculating the Sync interval
**               Sync interval = 2 ^ x  
**
** Parameters  : w_ifIdx        (IN) - communication interface index to set
**                                     the new sync interval 
**               c_syncInterval (IN) - exponent for calculating the sync 
**                                     interval
**
** Returnvalue : -
**                      
** Remarks     : function is reentrant
**
***********************************************************************/
void SLV_SetSyncInt(UINT16 w_ifIdx,INT8 c_syncInterval)
{
  UINT32 dw_synIntvTmo;
  /* calculate actual sync interval timeout */
  dw_synIntvTmo = PTP_getTimeIntv(c_syncInterval);
  /* change it to 110 % of the calculated value */
  dw_synIntvTmo *= 11;
  dw_synIntvTmo /= 10;
  SLV_adw_SynIntvTmo[w_ifIdx] = dw_synIntvTmo;
  /* calculate actual sync receive timeout interval */
  SLV_adw_SynRcvTmo[w_ifIdx]  = (UINT32)(dw_synIntvTmo * 4);
}

/***********************************************************************
**
** Function    : SLV_SyncTask
**
** Description : Does the Synchronization to the actual master in the segment. 
**               The task-handles must be named: 
**               SLS1_TSK,SLS2_TSK, .... , SLSn_TSK. They must
**               be declared in ascending order in the SIS-ConfigFile !!!
**               For each channel of the node implementation there must
**               be a seperate slave sync task.
**              
** Parameters  : w_hdl        (IN) - the SIS-task-handle
**
** Returnvalue : -
**                      
** Remarks     : function is reentrant
**
***********************************************************************/
void SLV_SyncTask(UINT16 w_hdl )
{  
  static NIF_t_PTPV2_SyncMsg  *ps_syncMsg;
  static NIF_t_PTPV2_FlwUpMsg *ps_flwUpMsg;
  static UINT32        dw_evnt;  
  static PTP_t_TmStmp  *ps_rxTs;
  PTP_t_PortId         *ps_mstPId;
  BOOLEAN              o_rct = FALSE;

  static UINT16        w_actIfIdx = 0; /* port that is in slave state */
  static PTP_t_MboxMsg s_msg;
  static PTP_t_MboxMsg *ps_Msg;
  static SLV_t_LastSyn s_synData;
  static UINT8         b_tskSts; 
  static BOOLEAN       o_expcFlwUp;
  
  /* multitasking restart point - generates a 
     jump to the last BREAK at env  */
  SIS_Continue(w_hdl);    
    /* initialize the Task with status = Startup */
    b_tskSts   = k_SLV_STOPPED;  

  while( TRUE ) /*lint !e716 */
  {
    /* got ready through event ? */
    dw_evnt = SIS_EventGet(k_EVT_ALL);    
    if( dw_evnt != 0 )
    {
      if( dw_evnt & (k_EV_INIT|k_EV_SC_MST|k_EV_SC_PSV|k_EV_SC_DSBL) )
      {
        dw_evnt = 0;
        /* stop timeouts */
        SIS_TimerStop(SLSintTmo_TSK);
        SIS_TimerStop(SLSrcvTmo_TSK);
        /* mark the sync data to unsynced */
        s_synData.o_unsynced = TRUE;
        /* set the struct to invalid */
        s_synData.o_valid = FALSE;        
        /* reset expecting flag */
        o_expcFlwUp = FALSE;
        /* set status to stopped */
        b_tskSts = k_SLV_STOPPED;               
      }      
    }
    /* got ready through message arrival ?  */
    while( (ps_Msg = (PTP_t_MboxMsg*)SIS_MboxGet()) != NULL )
    {
      /* got ready with sync message arrival?  */
      if( ps_Msg->e_mType == e_IMSG_SYNC )
      {
        /* get the sync message */
        ps_syncMsg = ps_Msg->s_pntData.ps_syn;
        /* get the timestamp */
        ps_rxTs = ps_Msg->s_pntExt1.ps_tmStmp;
        /* handle this sync message */
        while( Handle_Sync(ps_syncMsg,
                           ps_rxTs,
                           ps_Msg->s_etc1.w_u16,
                           &s_synData,
                           &o_rct) == FALSE )
        {  
          /* there are too few mbox entries in CTL_Task -> wait !! */
          PTP_SetError(k_SLV_ERR_ID,SLV_k_MBOX_WAIT,e_SEVC_WARN);            
          /* recall me, when there are entries in the mbox  */
          SIS_MboxWListAdd(w_hdl,CTL_TSK);
          SIS_Break(w_hdl,1); /*lint !e646 !e717*/ 
        }           
        /* if this was the most recent sync message from the parent master
           restart the timeout  */
        if( o_rct == TRUE )
        {  
          /* reset the sync receive timeout  */
          SIS_TimerStart( SLSrcvTmo_TSK,SLV_adw_SynRcvTmo[w_actIfIdx]); 
          /* reset the sync interval timeout */
          SIS_TimerStart( SLSintTmo_TSK,SLV_adw_SynIntvTmo[w_actIfIdx]); 
        } 
        else
        {
          /* do nothing */
        }            
        if( o_expcFlwUp == TRUE )
        {
          /* send a message to the Unit ctl */
          s_msg.e_mType          = e_IMSG_MFLWUP; 
          s_msg.s_pntData.ps_pId = &s_synData.s_prntPID;
          s_msg.s_etc1.w_u16     = w_actIfIdx;
          s_msg.s_etc2.o_bool    = s_synData.o_prntIsUc;
          /* send it */
          if( SIS_MboxPut(CTL_TSK,&s_msg) == FALSE )
          {
            /* set warning error */
            PTP_SetError(k_SLV_ERR_ID,SLV_k_MBOX_WAIT,e_SEVC_WARN);
          }           
        }
        else
        {
          /* do nothing */
        }      
        /* if a follow up message is expected, set the expecting flag */
        if( (GET_FLAG(ps_syncMsg->s_ptpHead.w_flags,k_FLG_TWO_STEP)== TRUE) && 
            (o_rct == TRUE ))
        {
          o_expcFlwUp = TRUE;   
        }
        else
        {
          /* do nothing */
        }
        /* release memory */        
        SIS_Free(ps_syncMsg); /* sync msg struct */
        SIS_Free(ps_rxTs);    /* rx timestamp */
      }
      else if( ps_Msg->e_mType == e_IMSG_FLWUP )
      {
        /* get the follow up message */
        ps_flwUpMsg = ps_Msg->s_pntData.ps_flw;
        /* handle the follow up message */
        while( Handle_Follow_up(ps_flwUpMsg,
                                &s_synData,
                                &o_rct,
                                w_actIfIdx) == FALSE )
        {
          /* there are too few mbox entries in CTL_Task -> wait !! */
          PTP_SetError(k_SLV_ERR_ID,SLV_k_MBOX_WAIT,e_SEVC_WARN);
          /* recall me, when there are entries in the mbox */
          SIS_MboxWListAdd(w_hdl,CTL_TSK);
          SIS_Break(w_hdl,2);/*lint !e646 !e717 */ 
        } 
        if( o_rct == TRUE )
        {
          /* reset expecting flag */
          o_expcFlwUp = FALSE;  
        }
        else
        {
          /* do nothing */
        }
        /* free allocated memory */
        SIS_Free(ps_Msg->s_pntData.pv_data); 
      }      
      /* the parent master changed, determined through bmc algorithm */
      else if(ps_Msg->e_mType == e_IMSG_NEW_MST)
      {
        /* get the port id of the new master */
        ps_mstPId = ps_Msg->s_pntData.ps_pId; 
        /* store it */
        s_synData.s_prntPID = *ps_mstPId;
        /* store communication type (unicast/multicast) */
        s_synData.o_prntIsUc = ps_Msg->s_etc2.o_bool;
        /* get the port on which the node is slave */
        w_actIfIdx = ps_Msg->s_etc1.w_u16;
        /* set the struct to invalid */
        s_synData.o_valid = FALSE;
        s_synData.o_unsynced = TRUE;
        /* set timeouts to wait for external sync msg from the new master */
        SIS_TimerStart(SLSintTmo_TSK,SLV_adw_SynIntvTmo[w_actIfIdx]); 
        SIS_TimerStart(SLSrcvTmo_TSK,SLV_adw_SynRcvTmo[w_actIfIdx]); 
        /* change to operational state */
        b_tskSts = k_SLV_OPERATIONAL;  
      }       
      /* release the last mbox entry */
      SIS_MboxRelease();
    }
    /* got ready with expired timer? */
    if( SIS_TimerStsRead( SLSintTmo_TSK ) == SIS_k_TIMER_EXPIRED )
    { 
      if( b_tskSts == k_SLV_OPERATIONAL )
      {         
        /* there is no new sync packet in the sync interval
           -> send message to CTL-Task to reset clock drift */
        s_msg.e_mType           = e_IMSG_ADJP_END;
        s_msg.s_pntData.pv_data = NULL;
        /* send message */
        if( SIS_MboxPut(CTL_TSK,&s_msg) == FALSE ) 
        {
          /* set note */
          PTP_SetError(k_SLV_ERR_ID,SLV_k_MBOX_DSCD,e_SEVC_NOTC);
        } 
      }
      else
      {
        /* do nothing */
      }
    }
    /* got ready with expired timer? */
    if( SIS_TimerStsRead( SLSrcvTmo_TSK ) == SIS_k_TIMER_EXPIRED )
    {
      if( b_tskSts == k_SLV_OPERATIONAL )
      {
        /* ctl_task handles that error */         
        s_msg.e_mType          = e_IMSG_MSYNC;
        s_msg.s_pntData.ps_pId = &s_synData.s_prntPID;
        s_msg.s_etc1.w_u16     = w_actIfIdx;
        s_msg.s_etc2.o_bool    = s_synData.o_prntIsUc;
        /* send message */      
        if( SIS_MboxPut(CTL_TSK,&s_msg) == FALSE )
        {
          /* set warning error  */
          PTP_SetError(k_SLV_ERR_ID,SLV_k_MBOX_DSCD,e_SEVC_WARN);
          /* retry next time */
          SIS_TimerStart(SLSrcvTmo_TSK,2);
        }
      }
      else
      {
        /* do not reset timer in stopped state */
      }
    }
    else /* ExecReq */
    {
      /* do nothing */
    }
    /* cooperative multitasking 
       set task to blocked, till it is ready through an event,
       message or timeout */
    SIS_Break(w_hdl,3); /*lint !e646 !e717*/ 
  } 
  SIS_Return(w_hdl,4);/*lint !e744 */
}

/*************************************************************************
**    static functions
*************************************************************************/

/***********************************************************************
**
** Function    : Handle_Sync
**
** Description : Handles received sync messages for the SLV_SyncTask.
**               This function checks, if the sync message is from the 
**               actual parent master and if it´s new. 
**               If it´s from the master and a new one, then the ctl_task
**               must update the parent data set and store it for the bmc
**               algorithm. If the master doesn´t send a follow-up, this 
**               function calculates the master-to-slave-delay and sends
**               it to the ctl_task, to update the current data set.
**               If the master sends a follow up, the receive-timestamp is
**               stored with the associated sequence-id.
**               If the sync message is old ( a newer one was already received)
**               this function deletes it.
**               If the sync message is from a foreign master, the ctl_task 
**               gets it for bmc-algorithm purposes.
**               This function has to be called, until all required messages
**               can be sent to the CTL_Task. If the mbox of the CTL_Task has
**               too few entries left, this function aborts and must be called 
**               again.
**              
** Parameters  : ps_msgSyn       (IN) - pointer to the recvd. sync msg
**               ps_rxTs         (IN) - receive timestamp
**               w_ifIdx         (IN) - receiving comm. interface index
**               ps_lastSync (IN/OUT) - pointer to the last-sync-struct
**               po_rct         (OUT) - pointer to flag. TRUE, if sync was  
**                                      from the actual parent master AND 
**                                      is the newest sync; else FALSE
**
** Returnvalue : TRUE                 - sync msg was handled
**               FALSE                - results could not be postet to the
**                                      mbox of the CTL_Task - function has
**                                      to be called again.
**                      
** Remarks     : function is reentrant
**
***********************************************************************/
static BOOLEAN Handle_Sync(const NIF_t_PTPV2_SyncMsg *ps_msgSyn,
                           const PTP_t_TmStmp        *ps_rxTs,
                           UINT16                    w_ifIdx,
                           SLV_t_LastSyn             *ps_lastSync,
                           BOOLEAN                   *po_rct)                
{
  PTP_t_MboxMsg s_msg;
  PTP_t_TmStmp  *ps_tsOrgn;
  PTP_t_TmIntv  *ps_llDiff;
  PTP_t_TmStmp  *ps_rxTsCpy;
#ifndef LET_ALL_TS_THROUGH
  INT64         ll_sec;
#endif
  BOOLEAN       o_msgIsUc;

  /* get unicast flag */
  o_msgIsUc = GET_FLAG(ps_msgSyn->s_ptpHead.w_flags,k_FLG_UNICAST);
  /* check if this message is from the parent master */
  if( (ps_lastSync->o_prntIsUc == o_msgIsUc) &&
      (PTP_ComparePortId(&ps_lastSync->s_prntPID,
                         &ps_msgSyn->s_ptpHead.s_srcPortId) == PTP_k_SAME ))
  {    
    /* if this is a new message */
    if( (ps_msgSyn->s_ptpHead.w_seqId > ps_lastSync->w_SeqId)              ||
        /* rollover condition */
        ((ps_msgSyn->s_ptpHead.w_seqId < 2)&&(ps_lastSync->w_SeqId > 65533)) ||
        /* startup condition */
        (ps_lastSync->o_unsynced == TRUE ))
    {      
      /* enough space in mbox of CTL_Task to handle the sync? */
      if( SIS_MboxQuery(CTL_TSK) < 1 )
      {
        /* there is no space in the mbox - try again later */
        return FALSE;
      }
      /* get and store the actual sequence id */
      ps_lastSync->w_SeqId = ps_msgSyn->s_ptpHead.w_seqId;
      /* store received correction data */
      ps_lastSync->ll_corrSyn = ps_msgSyn->s_ptpHead.ll_corField;
      
      /* if the parent master doesn´t send a follow up message,
         the time calculation must now be done */
      if( GET_FLAG(ps_msgSyn->s_ptpHead.w_flags,k_FLG_TWO_STEP) == FALSE )
      {
        /* enough space in mbox of CTL_Task to handle the sync? */
        if( (SIS_MboxQuery(CTL_TSK) < 1) || (SIS_MemPoolQuery(k_POOL_64) < 3))
        {
          /* there is no space in the mbox - try again later */
          return FALSE;
        }
        /* Alloc memory for the time-interval  */
        ps_llDiff  = (PTP_t_TmIntv*)SIS_AllocHdl(k_POOL_64);
        /* alloc memory for the origin timestamp */
        ps_tsOrgn  = (PTP_t_TmStmp*)SIS_AllocHdl(k_POOL_64);
        /* alloc memory for the rx timetamp */
        ps_rxTsCpy = (PTP_t_TmStmp*)SIS_AllocHdl(k_POOL_64);
        /* get origin timestamp */
        *ps_tsOrgn = ps_msgSyn->s_origTs;/*lint !e613*/
        /* copy rx timestamp */
        *ps_rxTsCpy = *ps_rxTs;/*lint !e613*/
#ifndef LET_ALL_TS_THROUGH
        /* get the difference between receive and origin timestamp
           = master_to_slave_delay */
        if( PTP_SubtrTmStmpTmStmp(ps_rxTs,ps_tsOrgn,ps_llDiff)==FALSE)
        {
          /* subtract correction field */ 
          ll_sec = PTP_INTV_TO_NSEC(ps_msgSyn->s_ptpHead.ll_corField);
          ll_sec /= (INT64)k_NSEC_IN_SEC;
          ps_llDiff->ll_scld_Nsec -= ll_sec;/*lint !e613*/
          /* time difference not applicable to time interval type */
          /* function returned unscaled nanoseconds */         
          s_msg.e_mType  = e_IMSG_MTSDBIG;                  
        }
        else
#else
/* still report timestamp, just he difference is incorrect */
         PTP_SubtrTmStmpTmStmp(ps_rxTs,ps_tsOrgn,ps_llDiff);
#endif
        {
          /* subtract correction field */ 
          ps_llDiff->ll_scld_Nsec -= 
                                 ps_msgSyn->s_ptpHead.ll_corField;/*lint !e613*/
          /* pack the timestamp in a SIS message */
          s_msg.e_mType = e_IMSG_MTSD;
        }
        s_msg.s_pntData.ps_tmIntv = ps_llDiff; 
        s_msg.s_pntExt1.ps_tmStmp = ps_tsOrgn;
        s_msg.s_pntExt2.ps_tmStmp = ps_rxTsCpy;
        s_msg.s_etc1.w_u16        = w_ifIdx;
        s_msg.s_etc2.c_s8         = ps_msgSyn->s_ptpHead.c_logMeanMsgIntv;
        /* delivering of the time difference for synchronization 
           to the parent master  */
        if( SIS_MboxPut(CTL_TSK,&s_msg) == FALSE )
        { 
          /* free memory */
          SIS_Free(ps_llDiff);
          SIS_Free(ps_tsOrgn);
          SIS_Free(ps_rxTsCpy);
          /* not possible to come in here - FATAL !! */
          PTP_SetError(k_SLV_ERR_ID,SLV_k_MBOX_DSCD,e_SEVC_CRIT);
        }   
      }
      /* there comes a follow up  */
      else
      { 
        /* store receive TimeStamp from data */
        ps_lastSync->s_rxTs = *ps_rxTs;
        /* set the timestamp in the struct to valid */
        ps_lastSync->o_valid = TRUE;         
      } 
#if( k_CLK_DEL_E2E == TRUE )
      /* send message to delay task to trigger a delay request message */
      s_msg.e_mType = e_IMSG_TRGDLRQ;
      /* send message to delay task */
      if( SIS_MboxPut(SLD_TSK,&s_msg) == FALSE )
      {
        /* set warning error  */
        PTP_SetError(k_SLV_ERR_ID,SLV_k_MBOX_DSCD,e_SEVC_WARN); 
      }   
#endif /* #if( k_CLK_DEL_E2E == TRUE ) */
      /* message was from actual parent master and is most recent */
      *po_rct = TRUE;
    }
    /* this is an old message - a newer is already here  */
    else
    { 
      /* message war from actual parent master, but is not the most recent */
      *po_rct = FALSE;
    }    
  }
  /* if sync isn´t from the parent master discard the msg,
     it is a babbling idiot */
  else  
  {    
    *po_rct = FALSE;
  }
  return TRUE;
}

/***********************************************************************
**
** Function    : Handle_Follow_up
**
** Description : Handles received follow up messages when node is in slave
**               state.In this function, the master to slave delay is 
**               calculated and sent to the ctl_task, if it is from the 
**               actual parent master, and if it associates to the last 
**               received syncmessage. Otherwise, this function deletes 
**               the message.
**              
** Parameters  : ps_msgFlwUp  (IN) - pointer to the recvd. follow up msg
**               ps_lastSync  (IN) - pointer to the last-sync-struct
**               po_rct   (IN/OUT) - pointer to flag. TRUE, if the msg was  
**                                   from the actual parent master AND 
**                                   corresponds to the last recvd sync msg;
**                                   else FALSE
**               w_actIfIdx   (IN) - communication interface index 
**                                   of actual port in slave state
**
** Returnvalue : TRUE              - follow up msg was handled
**               FALSE             - results could not be postet to the
**                                   mbox of the CTL_Task.
**                      
** Remarks     : function is reentrant
**
***********************************************************************/
static BOOLEAN Handle_Follow_up(const NIF_t_PTPV2_FlwUpMsg *ps_msgFlwUp,
                                const SLV_t_LastSyn        *ps_lastSync,
                                BOOLEAN                    *po_rct,
                                UINT16                     w_actIfIdx)
{
  PTP_t_TmStmp  *ps_tsOrgn;
  PTP_t_TmIntv  *ps_llDiff;
  PTP_t_TmStmp  *ps_rxTs;
  PTP_t_MboxMsg s_msg;
#ifndef LET_ALL_TS_THROUGH
  INT64         ll_sec;
#endif
  BOOLEAN       o_msgIsUc;

  /* don´t handle it, if the sync data are invalid */
  if( ps_lastSync->o_valid == FALSE )
  {
    /* discard msg */
    return TRUE;
  }
  /* get unicast flag */
  o_msgIsUc = GET_FLAG(ps_msgFlwUp->s_ptpHead.w_flags,k_FLG_UNICAST);
  /* check, if this message is from the actual parent master */
  if( (ps_lastSync->o_prntIsUc == o_msgIsUc) &&
      (PTP_ComparePortId(&ps_lastSync->s_prntPID,
                        &ps_msgFlwUp->s_ptpHead.s_srcPortId) == PTP_k_SAME))
  {
    /* check, if this follow up associates to the last sync msg */
    if( ps_msgFlwUp->s_ptpHead.w_seqId == ps_lastSync->w_SeqId )
      
    {
      /* enough space in mbox of CTL_Task to handle the sync? */
      if( (SIS_MboxQuery(CTL_TSK) < 1) || (SIS_MemPoolQuery(k_POOL_64) < 3) )
      {
        /* there is no space in the mbox - try again later */
        return FALSE;
      }
      /* Alloc memory for the timestamp */
      ps_llDiff = (PTP_t_TmIntv*)SIS_AllocHdl(k_POOL_64);
      /* Alloc memory for the origin timestamp */
      ps_tsOrgn = (PTP_t_TmStmp*)SIS_AllocHdl(k_POOL_64);
      /* Alloc memory for the rx timestamp */
      ps_rxTs   = (PTP_t_TmStmp*)SIS_AllocHdl(k_POOL_64);
      /* get origin timestamp  */
      *ps_tsOrgn = ps_msgFlwUp->s_precOrigTs;/*lint !e613*/
      /* get the rx timestamp */
      *ps_rxTs   = ps_lastSync->s_rxTs;/*lint !e613*/
      /* compute master to slave delay */
#ifndef LET_ALL_TS_THROUGH
      if( PTP_SubtrTmStmpTmStmp(ps_rxTs,ps_tsOrgn,ps_llDiff) == FALSE)
      {
        /* subtract correction field of follow up message */ 
        ll_sec = PTP_INTV_TO_NSEC(ps_msgFlwUp->s_ptpHead.ll_corField);
        ll_sec /= (INT64)k_NSEC_IN_SEC;
        ps_llDiff->ll_scld_Nsec -= ll_sec;/*lint !e613*/
        /* subtract correction field of sync message */
        ll_sec = PTP_INTV_TO_NSEC(ps_lastSync->ll_corrSyn);
        ll_sec /= (INT64)k_NSEC_IN_SEC;
        ps_llDiff->ll_scld_Nsec -= ll_sec;/*lint !e613*/   
        /* time difference too big to scale - result are pure nanoseconds */
        s_msg.e_mType = e_IMSG_MTSDBIG;        
      }
      else
#else
/* still report timestamp, just he difference is incorrect */
      PTP_SubtrTmStmpTmStmp(ps_rxTs,ps_tsOrgn,ps_llDiff);
#endif
      {
        /* subtract correction field of follow up message */
        ps_llDiff->ll_scld_Nsec -= ps_msgFlwUp->s_ptpHead.ll_corField;/*lint !e613*/
        /* subtract correction field of sync message */
        ps_llDiff->ll_scld_Nsec -= ps_lastSync->ll_corrSyn;/*lint !e613*/
        /* pack the timestamp in a SIS message */
        s_msg.e_mType  = e_IMSG_MTSD;                
      } 
      s_msg.s_pntData.ps_tmIntv = ps_llDiff;
      s_msg.s_pntExt1.ps_tmStmp = ps_tsOrgn;
      s_msg.s_pntExt2.ps_tmStmp = ps_rxTs;
      s_msg.s_etc1.w_u16        = w_actIfIdx;     
      s_msg.s_etc2.c_s8         = ps_msgFlwUp->s_ptpHead.c_logMeanMsgIntv;
      /* send it to the control task */
      if( SIS_MboxPut(CTL_TSK,&s_msg)==FALSE)
      {
        /* free allocated memory */
        SIS_Free(ps_llDiff);
        SIS_Free(ps_tsOrgn);
        SIS_Free(ps_rxTs);
        /* set warning error */
        PTP_SetError(k_SLV_ERR_ID,SLV_k_MBOX_DSCD,e_SEVC_ERR);
      }
      /* this message was from the actual parent master AND corresponds to
         the last received sync msg */
      *po_rct = TRUE;
    }
    else  
    {
      /* this message was from the actual parent master but
        associates not with the last sync message        */
      *po_rct = FALSE;
    }     
  }
  /* this message was sent by a foreign master  */
  else  
  {
    /* this message was not from the actual parent master */
    *po_rct = FALSE;
  }  
  return TRUE;
}
#endif /*#if(( k_CLK_IS_OC == TRUE)||(k_CLK_IS_BC))*/

