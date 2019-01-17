/*************************************************************************
**    (C) 2008 - 2009 IXXAT Automation GmbH, all rights reserved
**************************************************************************
**
**       File: PTPreq.c  
**    Summary: Peer Delay determination task.
**             Determines the peer delay to the next node and issues peer 
**             delay requests.
**
**    Version: 1.01.01
**
**************************************************************************
**************************************************************************
**
**  Functions: P2P_SetPDelReqInt
**             P2P_PDelTask
**             CalcPathDelay
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
#if( k_CLK_DEL_P2P == TRUE )
#include "SIS/SIS.h"
#include "NIF/NIF.h"
#include "DIS/DIS.h"
#include "PTP/PTP.h"
#include "P2P/P2P.h"
#include "P2P/P2Pint.h"

/*************************************************************************
**    global variables
*************************************************************************/

/*************************************************************************
**    static constants, types, macros, variables
*************************************************************************/

static UINT32 P2P_adw_PDelReqIntvTmo[k_NUM_IF];
/*************************************************************************
**    static function-prototypes
*************************************************************************/
static void CalcPathDelay( const PTP_t_TmStmp *ps_txTs,
                           const PTP_t_TmStmp *ps_rxTs,
                           INT64              ll_sclCorr,
                           UINT16             w_ifIdx);

/*************************************************************************
**    global functions
*************************************************************************/

/***********************************************************************
**
** Function    : P2P_SetPDelReqInt
**
** Description : Set´s the exponent for calculating the peer delay request
**               interval. Interval = 2 ^ x  
**
** Parameters  : w_ifIdx       (IN) - communication interface index to set 
**                                    the new peer delay request interval
**               c_pDelReqIntv (IN) - exponent for calculating the delay 
**                                    request interval
**
** Returnvalue : -
**                      
** Remarks     : function is reentrant
**
***********************************************************************/
void P2P_SetPDelReqInt(UINT16 w_ifIdx,INT8 c_pDelReqIntv)
{
  /* calculate actual delay request interval */
  P2P_adw_PDelReqIntvTmo[w_ifIdx] = PTP_getTimeIntv(c_pDelReqIntv); 
  /* start task */
  SIS_TimerStart(P2Pdel_TSK1 + w_ifIdx,P2P_adw_PDelReqIntvTmo[w_ifIdx]);
}

/***********************************************************************
**
** Function    : P2P_PDelTask
**
** Description : The peer to peer path delay task is responsible for 
**               issuing peer delay request messages and receiving
**               peer delay response adn peer delay response follow up 
**               messages. The task calculates the actual peer delay
**               to the next node and sends it to the control task.
**              
** Parameters  : w_hdl        (IN) - the SIS-task-handle
**
** Returnvalue : -
**                      
** Remarks     : function is reentrant
**
***********************************************************************/
void P2P_PDelTask(UINT16 w_hdl)
{
  UINT16                w_tskIdx = w_hdl - P2Pdel_TSK1;
  PTP_t_TmStmp          s_tsTxPdel,*ps_rxTsResp,*ps_txTsRsp;
  NIF_t_PTPV2_PDelResp  *ps_pDelRsp = NULL; 
  NIF_t_PTPV2_PDlRspFlw *ps_pDlrspFlw = NULL;
  static PTP_t_MboxMsg  *aps_msg[k_NUM_IF];
  static P2P_t_pDelReq  as_actPDlrq[k_NUM_IF];
  static UINT16         aw_seqId[k_NUM_IF] = {0};
  PTP_t_TmIntv          s_tiIntv;
  static PTP_t_PortAddr *aps_mstPAddr[k_NUM_IF] = {NULL};
  UINT32                dw_evnt;
    
  /* multitasking restart point - generates a 
     jump to the last BREAK at env */
  SIS_Continue(w_hdl);
    /* reset act peer delay request struct once at startup */
    as_actPDlrq[w_tskIdx].s_rxTsReq.dw_Nsec = k_MAX_U32;
    as_actPDlrq[w_tskIdx].s_rxTsReq.dw_Nsec = k_MAX_U32;
    as_actPDlrq[w_tskIdx].ll_corr        = 0;
    as_actPDlrq[w_tskIdx].w_seqId        = 0;
    
  while( TRUE )/*lint !e716 */
  {
    /* got ready with a event? */
    dw_evnt=SIS_EventGet(k_EVT_ALL);

    if( dw_evnt != 0 )
    {
      if( dw_evnt & k_EV_INIT )
      {
        /* start immediately */
        SIS_TimerStart(w_hdl,1);
        /* reset all other events */
        dw_evnt = 0;
      }
      /* handle defined events */
      if( dw_evnt & (k_EV_SC_FLT|k_EV_SC_DSBL))
      {
        /* stop timeout */
        SIS_TimerStop(w_hdl);
        /* reset all other events */
        dw_evnt = 0;
      }
    }
    /* got ready through a message? */
    while((aps_msg[w_tskIdx] = (PTP_t_MboxMsg*)SIS_MboxGet()) != NULL )
    {
      /* a peer delay response msg  */
      if( aps_msg[w_tskIdx]->e_mType == e_IMSG_P_DLRSP )
      {
        /* get the message */
        ps_pDelRsp  = aps_msg[w_tskIdx]->s_pntData.ps_pDrsp;
        /* get the rx timestamp of the peer delay response */
        ps_rxTsResp = aps_msg[w_tskIdx]->s_pntExt1.ps_tmStmp;
        /* check : */
        /* - if it corresponds with the sequence id of the 
             last peer delay request message */
        if((ps_pDelRsp->s_ptpHead.w_seqId == as_actPDlrq[w_tskIdx].w_seqId) &&
        /* - if the delay response is addressed to this node */
           (PTP_ComparePortId(&ps_pDelRsp->s_reqPId,
                              &P2P_as_portId[w_tskIdx]) == PTP_k_SAME)      &&
        /* - if there is a tx timestamp of the peer delay request */
            ( as_actPDlrq[w_tskIdx].s_txTsReq.dw_Nsec != k_MAX_U32))
        {
          /* is peer delay responder two-step ? */
          if( GET_FLAG(ps_pDelRsp->s_ptpHead.w_flags,k_FLG_TWO_STEP) == TRUE )
          {
            /* remember rx timestamp of peer delay request */
            as_actPDlrq[w_tskIdx].s_rxTsReq  = ps_pDelRsp->s_reqRecptTs;
            /* remember rx timestamp of peer delay response */
            as_actPDlrq[w_tskIdx].s_rxTsResp = *ps_rxTsResp;
            /* remember correction field of peer delay response */
            as_actPDlrq[w_tskIdx].ll_corr = ps_pDelRsp->s_ptpHead.ll_corField;
          }
          else
          {
            /* calc path delay */
            CalcPathDelay(&as_actPDlrq[w_tskIdx].s_txTsReq,
                          ps_rxTsResp,
                          ps_pDelRsp->s_ptpHead.ll_corField,
                          w_tskIdx);            
          }
        }
        else
        {
          /* do nothing */
        }
        /* free memory */
        SIS_Free(ps_pDelRsp);
        SIS_Free(ps_rxTsResp);
      }
      /* a peer delay response msg  */
      else if( aps_msg[w_tskIdx]->e_mType == e_IMSG_P_DLRSP_FLW )
      {
        /* get the message */
        ps_pDlrspFlw = aps_msg[w_tskIdx]->s_pntData.ps_pDrFu;
        /* check : */
        /* - if it corresponds with the sequence id of the 
             last peer delay request message */
        if((ps_pDlrspFlw->s_ptpHead.w_seqId == as_actPDlrq[w_tskIdx].w_seqId) &&
        /* - if the delay response is addressed to this node */
           (PTP_ComparePortId(&ps_pDlrspFlw->s_reqPId,
                              &P2P_as_portId[w_tskIdx]) == PTP_k_SAME)      &&
        /* - if there is a tx timestamp of the peer delay request */
           ( as_actPDlrq[w_tskIdx].s_txTsReq.dw_Nsec != k_MAX_U32)          &&
        /* if the peer delay response was received */
           ( as_actPDlrq[w_tskIdx].s_rxTsReq.dw_Nsec != k_MAX_U32))
        {
          /* get tx timestamp of peer delay response message */
          ps_txTsRsp = &ps_pDlrspFlw->s_respTs;
          /* get t3-t2 */
          if( PTP_SubtrTmStmpTmStmp(ps_txTsRsp,
                                    &as_actPDlrq[w_tskIdx].s_rxTsReq,
                                    &s_tiIntv) == TRUE )
          {
            /* add correction-field of peer delay response message */
            s_tiIntv.ll_scld_Nsec += as_actPDlrq[w_tskIdx].ll_corr;
            /* add correction-field of peer delay response flwUp message */
            s_tiIntv.ll_scld_Nsec += ps_pDlrspFlw->s_ptpHead.ll_corField;
            /* calculate path delay */
            CalcPathDelay(&as_actPDlrq[w_tskIdx].s_txTsReq,
                          &as_actPDlrq[w_tskIdx].s_rxTsResp,
                          s_tiIntv.ll_scld_Nsec,
                          w_tskIdx);
          }
          else
          {
            /* do nothing - will be fixed by synchronization */
          }
        }
        else
        {
          /* do nothing */
        }
        /* free allocated memory */
        SIS_Free(ps_pDlrspFlw);
      }
      /* new unicast master - send Peer delay request to 
         this node */  
      else if( aps_msg[w_tskIdx]->e_mType == e_IMSG_PORTADDR )
      {
        /* store master port address for this interface */
        aps_mstPAddr[w_tskIdx] = aps_msg[w_tskIdx]->s_pntExt1.ps_pAddr;
      }
      else
      { 
        /* error - unknown message */
        PTP_SetError(k_P2P_ERR_ID,P2P_k_UNKWN_MSG,e_SEVC_ERR); 
      }
      /* release message box entry */
      SIS_MboxRelease(); 
    }
    /* got ready through timeout ?  */
    if( SIS_TimerStsRead( w_hdl ) == SIS_k_TIMER_EXPIRED )
    {
      /* send a peer delay request message */
      aw_seqId[w_tskIdx]++;
      /* set timestamps of act peer delay request struct to invalid */
      as_actPDlrq[w_tskIdx].s_rxTsReq.dw_Nsec = k_MAX_U32;
      as_actPDlrq[w_tskIdx].s_txTsReq.dw_Nsec = k_MAX_U32;
      /* issue a peer delay request */
      if( DIS_SendPdlrq(aps_mstPAddr[w_tskIdx],
                        P2P_as_portId[w_tskIdx].w_portNmb-1,
                        aw_seqId[w_tskIdx],
                        &s_tsTxPdel) == TRUE )
      {
        /* store timestamp */
        as_actPDlrq[w_tskIdx].s_txTsReq = s_tsTxPdel;
        as_actPDlrq[w_tskIdx].w_seqId   = aw_seqId[w_tskIdx];
      }
      /* restart timer */
      SIS_TimerStart(w_hdl,P2P_adw_PDelReqIntvTmo[w_tskIdx]);
    }    
    /* cooperative multitasking - give cpu to other tasks */
    SIS_Break(w_hdl,1); /*lint !e646 !e717*/  
  }
  SIS_Return(w_hdl,0);/*lint !e744*/
}

/*************************************************************************
**    static functions
*************************************************************************/

/***********************************************************************
**  
** Function    : CalcPathDelay
**  
** Description : Calculate the path delay and sends the information to
**               the control task.
**   
** Parameters  : ps_txTsReq  (IN) - pointer to tx Timestamp of 
**                                  path delay request
**               ps_rxTsResp (IN) - pointer to rx Timestamp of 
**                                  path delay response
**               ll_sclCorr  (IN) - scaled nanoseconds of 
**                                  turnaround time and correction
**               w_ifIdx     (IN) - communication interface index  
**                                  to calculate path delay
**               
** Returnvalue : -
** 
** Remarks     : function is not reentrant
**  
***********************************************************************/
static void CalcPathDelay( const PTP_t_TmStmp *ps_txTsReq,
                           const PTP_t_TmStmp *ps_rxTsResp,
                           INT64              ll_sclCorr,
                           UINT16             w_ifIdx)
{
  PTP_t_TmIntv  *ps_tiPdel = NULL;
  PTP_t_TmStmp  *ps_txTsPreqCpy = NULL;
  PTP_t_TmStmp  *ps_rxTsPrespCpy = NULL;
  PTP_t_TmIntv  s_tiTemp;
  PTP_t_MboxMsg s_msg;

  ps_tiPdel  = (PTP_t_TmIntv*)SIS_Alloc(sizeof(PTP_t_TmIntv));
  ps_txTsPreqCpy = (PTP_t_TmStmp*)SIS_Alloc(sizeof(PTP_t_TmStmp));
  ps_rxTsPrespCpy = (PTP_t_TmStmp*)SIS_Alloc(sizeof(PTP_t_TmStmp));
  /* check, if all alloc were successful */
  if( (ps_tiPdel != NULL )     && 
      (ps_txTsPreqCpy != NULL) && 
      (ps_rxTsPrespCpy != NULL) )
  {
    /* get the peer delay */
    if( PTP_SubtrTmStmpTmStmp(ps_rxTsResp,
                              ps_txTsReq,
                              ps_tiPdel) == FALSE )
    {
      /* time difference not applicable to time interval type
        -> discard and wait until the clocks are synchronized */
      SIS_Free(ps_tiPdel);
      SIS_Free(ps_txTsPreqCpy);
      SIS_Free(ps_rxTsPrespCpy);
    }
    else 
    {
      /* copy timestamps */
      *ps_txTsPreqCpy = *ps_rxTsResp;
      *ps_rxTsPrespCpy = *ps_txTsReq;
      /* subtract turnaround time of peer delay request receiver */
      ps_tiPdel->ll_scld_Nsec -= ll_sclCorr;
      /* devide it by two */
      ps_tiPdel->ll_scld_Nsec /= 2;
      /* correct rx timestamp with correction */
      s_tiTemp.ll_scld_Nsec = ll_sclCorr;
      PTP_SubtrTmIntvTmStmp(ps_rxTsPrespCpy,
                            &s_tiTemp,
                            ps_rxTsPrespCpy);/*lint !e534*/
      /* send act path delay and timestamps to ctrl task */
      s_msg.s_pntData.ps_tmIntv = ps_tiPdel;
      s_msg.s_pntExt1.ps_tmStmp = ps_rxTsPrespCpy;
      s_msg.s_pntExt2.ps_tmStmp = ps_txTsPreqCpy;
      s_msg.e_mType             = e_IMSG_ACT_PDEL;
      s_msg.s_etc1.w_u16        = w_ifIdx;
      if( SIS_MboxPut(CTL_TSK,&s_msg) == FALSE )
      {
        /* set error */
        PTP_SetError(k_P2P_ERR_ID,P2P_k_ERR_MBOX,e_SEVC_NOTC);
        /* free allocated memory */
        SIS_Free(ps_tiPdel);
        SIS_Free(ps_txTsPreqCpy);
        SIS_Free(ps_rxTsPrespCpy);
      }
    }
  }
  else
  {
    /* free allocated memory */
    if( ps_tiPdel != NULL )
    {
      SIS_Free(ps_tiPdel);
    }
    if( ps_txTsPreqCpy != NULL )
    {
      SIS_Free(ps_txTsPreqCpy);
    }
    if( ps_rxTsPrespCpy != NULL )
    {
      SIS_Free(ps_rxTsPrespCpy);
    }
    /* set error */
    PTP_SetError(k_P2P_ERR_ID,P2P_k_ERR_MEM,e_SEVC_NOTC);
  }
}

#endif /* #if( k_CLK_DEL_P2P == TRUE ) */


