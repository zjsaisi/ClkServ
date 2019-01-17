/*************************************************************************
**    (C) 2008 - 2009 IXXAT Automation GmbH, all rights reserved
**************************************************************************
**
**       File: UCMsyn.c  
**    Summary: The file UCMsyn encapsulates all master activities of a 
**             unicast capable PTP node.
**             The task UCMsyn_Task is active for all unicast slaves that
**             requested the sync message service and this service was 
**             granted.
**
**    Version: 1.01.01
**
**************************************************************************
**************************************************************************
**
**  Functions: UCMsyn_Task
**             UCM_IsSyncSrvGranted
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
#include "NIF/NIF.h"
#include "PTP/PTP.h"
#include "SIS/SIS.h"
#include "DIS/DIS.h"
#include "UCD/UCD.h"
#include "UCM/UCM.h"
#include "UCM/UCMint.h"

/* just for unicast enabled */
#if((k_UNICAST_CPBL == TRUE) && ((k_CLK_IS_OC) || (k_CLK_IS_BC)))
/*************************************************************************
**    global variables
*************************************************************************/

/*************************************************************************
**    static constants, types, macros, variables
*************************************************************************/
static UCM_t_slvSrv s_reqLstHead; /* just beginning of list - nothing stored */
/*************************************************************************
**    static function-prototypes
*************************************************************************/

/*************************************************************************
**    global functions
*************************************************************************/

/***********************************************************************
**
** Function    : UCMsyn_Task
**
** Description : The unicast master sync task is responsible for 
**               sending sync messages to all registered unicast slaves. 
**              
** Parameters  : w_hdl        (IN) - the SIS-task-handle
**
** Returnvalue : -
**                      
** Remarks     : Function is reentrant.
**
***********************************************************************/
void UCMsyn_Task( UINT16 w_hdl )
{
  INT8                 c_ret;
  //PTP_t_TmStmp         s_tsTx; 
  //PTP_t_TmIntv         s_tiFlwUpCorr;
  static PTP_t_MboxMsg *ps_msg;
  UINT32               dw_actTime;
  /* the actual smallest sync interval timeout */
  static INT8          c_minIntv = k_MAX_I8;
  static UINT32        dw_intvTick; /* actual minimal interval expressed as SIS timer ticks */
  UCM_t_slvSrv         *ps_srv;
  UCM_t_slvSrv         *ps_next;
  PTP_t_PortAddr       *ps_pAddr;
  UINT16               w_seqId;
  UINT16               w_ifIdx;
  UINT32               dw_evnts;
  
  /* multitasking restart point - generates a 
     jump to the last BREAK at env */
  SIS_Continue(w_hdl); 
    /* initialize service request list head */ 
    UCM_initSrvReqList(&s_reqLstHead);

  while( TRUE ) /*lint !e716 */
  {
    /* get events */
    dw_evnts = SIS_EventGet(k_EVT_ALL);
    {
      if( dw_evnts != 0 )
      {
        if( dw_evnts & k_EV_INIT )
        {
          dw_evnts = 0;
          /* reset variables */
          c_minIntv = k_MAX_I8;
          /* initialize service request list head */ 
          UCM_initSrvReqList(&s_reqLstHead);
        }
      }
    }
    /* got ready with message arrival?  */
    while( ( ps_msg = (PTP_t_MboxMsg*)SIS_MboxGet()) != NULL )
    { 
      /* add new unicast slave to receiverlist */
      if( ps_msg->e_mType == e_IMSG_UC_SRV_REQ )
      {
        /* try to register service request */
        c_ret = UCM_regServReq(ps_msg,&s_reqLstHead,e_MT_SYNC,c_minIntv);
        /* calculate and store new values */
        if( c_ret < c_minIntv )
        {
          /* store new interval */
          c_minIntv = c_ret;
          dw_intvTick = PTP_getTimeIntv(c_minIntv);
          /* restart timer with new interval */
          SIS_TimerStart(w_hdl,dw_intvTick);
        }        
      }
      /* remove a unicast slave request and send ACK */
      else if( ps_msg->e_mType == e_IMSG_UC_CNCL )
      {
        /* get port address */
        ps_pAddr = ps_msg->s_pntData.ps_pAddr;
        /* search for registered service */
        if( UCM_SrchSlv(&s_reqLstHead,ps_pAddr,&ps_srv) == TRUE )
        {
          /* get next sequence id */
          w_seqId = ps_srv->w_seqIdSrv++;
          /* get receiving interface index */
          w_ifIdx = ps_msg->s_etc3.w_u16;
          /* unregister */
          UCM_unregServReq(ps_srv);
          /* send acknowledge */
          UCM_RespUcCnclAck(ps_pAddr,w_ifIdx,w_seqId,e_MT_SYNC);
        }
        /* free allocated memory */
        SIS_Free(ps_pAddr);
        SIS_Free(ps_msg->s_pntExt1.ps_pId);
      }
      /* cancel all unicast slave requests on the desired interface */
      else if( ps_msg->e_mType == e_IMSG_UC_CNCL_IF )
      {
        /* get interface to cancel */
        w_ifIdx = ps_msg->s_etc1.w_u16;
        /* send cancel to all registered services */
        ps_srv = (UCM_t_slvSrv*)s_reqLstHead.pv_next;
        while( ps_srv != NULL )
        {
          /* get next service */
          ps_next = (UCM_t_slvSrv*)ps_srv->pv_next;
          if( ps_srv->w_ifIdx == w_ifIdx )
          {
            /* send cancel to slave */
            UCD_SendUcCancel(&ps_srv->s_pAddr,
                             ps_srv->w_ifIdx,
                             ps_srv->w_seqIdSign,
                             e_MT_SYNC); 
            UCD_SendUcCancel(&ps_srv->s_pAddr,
                             ps_srv->w_ifIdx,
                             ps_srv->w_seqIdSign,
#if( k_CLK_DEL_E2E == TRUE )
                             e_MT_DELRESP
#else /* #if( k_CLK_DEL_E2E == TRUE ) */
                             e_MT_PDELRESP
#endif /* #if( k_CLK_DEL_E2E == TRUE ) */
                             ); 
            /* unregister service request */
            UCM_unregServReq(ps_srv);
          }
          else
          {
            /* do nothing */
          }
          ps_srv = ps_next;
        }
      }
      else
      {
        /* set error */
        PTP_SetError(k_UCM_ERR_ID,UCM_k_MSGERR,e_SEVC_NOTC);         
      } 
      /* release mbox entry */
      SIS_MboxRelease();
    }
    /* got ready with elapsed timer?  */
    if( SIS_TimerStsRead( w_hdl ) == SIS_k_TIMER_EXPIRED )
    {
      /* get actual time */
      dw_actTime = SIS_GetTime();
      /* send sync and follow up messages to all connected slaves */
      ps_srv = (UCM_t_slvSrv*)s_reqLstHead.pv_next;
      while( ps_srv != NULL )
      {
        /* decrement prescaler of slave counter */
        ps_srv->w_actCnt--;
        if( ps_srv->w_actCnt == 0 )
        {
          /* reset counter */
          ps_srv->w_actCnt = ps_srv->w_presc;
          /* increment sync sequence number */
          ps_srv->w_seqIdSrv++; 
          /* issue a sync message */
// jyang: temporarily block master output
//          if( DIS_SendSync(&ps_srv->s_pAddr,
//                           ps_srv->w_ifIdx,
//                           ps_srv->c_intv,
//                           ps_srv->w_seqIdSrv,
//                           &s_tsTx) == TRUE)
//          {
//#if(k_TWO_STEP==TRUE)
//            /* DIS_SendSync returns real send time */
//            s_tiFlwUpCorr.ll_scld_Nsec = 0;
//            /* send follow up msg  */
//            DIS_SendFollowUp(&ps_srv->s_pAddr,
//                             ps_srv->w_ifIdx,
//                             ps_srv->w_seqIdSrv,
//                             &s_tiFlwUpCorr,
//                             ps_srv->c_intv,
//                             &s_tsTx);
//#endif /*/#if(k_TWO_STEP==TRUE)*/
//          }
        }
        /* set pointer to next slave request */
        ps_next = (UCM_t_slvSrv*)ps_srv->pv_next; 
        /* check, if service period is over */
        UCM_checkPeriod(ps_srv,dw_actTime);
        ps_srv = ps_next;
      }      
      /* reset timer for next sync message */
      SIS_TimerStart(w_hdl,dw_intvTick);      
    }
    /* cooperative multitasking 
       set task to blocked, till it gets ready through an event,
       message or timeout  */
    SIS_Break(w_hdl,1); /*lint !e646 !e717*/ 
  }
  SIS_Return(w_hdl,0);/*lint !e744*/
}

/***********************************************************************
**  
** Function    : UCM_IsSyncSrvGranted
**  
** Description : Returns, if a sync service is granted for the requested
**               service.
**  
** Parameters  : ps_pAddr (IN) - port address of requestor
**               
** Returnvalue : TRUE               - sync service is granted
**               FALSE              - sync servic is not granted
** 
** Remarks     : -
**  
***********************************************************************/
BOOLEAN UCM_IsSyncSrvGranted(const PTP_t_PortAddr *ps_pAddr)
{
  UCM_t_slvSrv *ps_fnd = NULL;
  return UCM_SrchSlv(&s_reqLstHead,ps_pAddr,&ps_fnd);
}


/*************************************************************************
**    static functions
*************************************************************************/
#endif /* #if((k_UNICAST_CPBL == TRUE) && ((k_CLK_IS_OC) || (k_CLK_IS_BC))) */
