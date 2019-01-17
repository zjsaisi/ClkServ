/*************************************************************************
**    (C) 2008 - 2009 IXXAT Automation GmbH, all rights reserved
**************************************************************************
**
**       File: SLVdel.c 
**    Summary: E2E Delay determination task.
**
**    Version: 1.01.01
**
**************************************************************************
**************************************************************************
**
**  Functions: SLV_SetDelReqInt
**             SLV_DelayTask
**
**             GetDelReqIntv
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
/* not used for TCs without OC */
#if(( k_CLK_IS_OC == TRUE) || (k_CLK_IS_BC))
#if( k_CLK_DEL_E2E == TRUE )

#include "PTP/PTPdef.h"
#include "SIS/SIS.h"
#include "NIF/NIF.h"
#include "PTP/PTP.h"
#include "DIS/DIS.h"
#include "SLV/SLV.h"
#include "SLV/SLVint.h"
#include "SLV/SLVdel_queue.h"

/*************************************************************************
**    global variables
*************************************************************************/
static UINT32 SLV_adw_DelReqIntvTmo[k_NUM_IF];

/*************************************************************************
**    static constants, types, macros, variables
*************************************************************************/

/*************************************************************************
**    static function-prototypes
*************************************************************************/
//static UINT32 GetDelReqIntv(UINT16 w_ifIdx);

/*************************************************************************
**    global functions
*************************************************************************/

/***********************************************************************
**
** Function    : SLV_SetDelReqInt
**
** Description : Set the exponent for calculating the delay request
**               interval. Interval = 2 ^ x  
**
** Parameters  : w_ifIdx      (IN) - communication interface index to set the 
**                                   new delay request interval (0,...,n-1)
**               c_delReqIntv (IN) - exponent for calculating the delay 
**                                   request interval
**
** Returnvalue : -
**                      
** Remarks     : function is reentrant
**
***********************************************************************/
void SLV_SetDelReqInt(UINT16 w_ifIdx,INT8 c_delReqIntv)
{
  /* calculate actual delay request interval */
  SLV_adw_DelReqIntvTmo[w_ifIdx] = PTP_getTimeIntv(c_delReqIntv); 
  /* restart timer immediately */
  SIS_TimerStart(SLD_TSK,1);
}
/***********************************************************************
**
** Function    : SLV_DelayTask
**
** Description : The slave delay task is responsible for calculating
**               the 'slave to master delay'. Therefore it sends delay
**               request messages to the master node. The master sends
**               a delay response message with his Rx Timestamp of the 
**               delay request. 
**              
** Parameters  : w_hdl        (IN) - the SIS-task-handle
**
** Returnvalue : -
**                      
** Remarks     : function is reentrant
**
***********************************************************************/
void SLV_DelayTask(UINT16 w_hdl)
{
  static UINT16               w_actIfIdx = 0;
  static PTP_t_PortId         *ps_mstPId; 
  static PTP_t_PortAddr       *ps_mstPAddr;
  static PTP_t_TmStmp         *ps_rxTs,*ps_txTs;
  static PTP_t_TmIntv         *ps_tiStmd;
  static NIF_t_PTPV2_DlRspMsg *ps_msgDRsp = NULL; 
  static UINT32               dw_evnt;
  static PTP_t_MboxMsg        s_msg;
  static PTP_t_MboxMsg        *ps_msg;
  static SLV_t_LastDlrq       s_lastDlrq;
  static SLV_t_LastDlrq       s_matchedDlrq;
  static UINT8                b_tskSts; 
  static BOOLEAN              o_expcDelResp;
  static UINT16               aw_seqId[k_NUM_IF] = {0};
  static BOOLEAN              o_waitForSync = FALSE;
  PTP_t_PortAddr              *ps_pAddr;
  
  /* multitasking restart point - generates a 
     jump to the last BREAK at env */
  SIS_Continue(w_hdl); 

    /* initialize delay request struct */
    s_lastDlrq.w_SeqId        = 0;
    s_lastDlrq.s_TsTx.u48_sec = 0;
    s_lastDlrq.s_TsTx.dw_Nsec = k_MAX_U32;

    /* initialize the random number generator */
    PTP_SRAND();/*lint !e586*/

    /* initialize the Task with status = ready */
    b_tskSts = k_SLV_STOPPED;
  
  while( TRUE ) /*lint !e716 */
  {
    dw_evnt = SIS_EventGet(k_EVT_ALL);
    if( dw_evnt != 0 )
    {
      /* stop task for the following events: */
      if( dw_evnt & (k_EV_INIT   | /* event init */
                     k_EV_SC_PSV | /* event state change passive */
                     k_EV_SC_FLT | /* event state change faulty */
                     k_EV_SC_DSBL| /* event state change disabled */
                     k_EV_SC_MST)) /* event state change master */
      {
        /* stop timer */
        SIS_TimerStop(w_hdl);
        dw_evnt=0;
        /* reset expecting flag */
        o_expcDelResp = FALSE;
        /* change state */
        b_tskSts = k_SLV_STOPPED;   
      }      
    }
    /* got ready through a message? */
    while((ps_msg = (PTP_t_MboxMsg*)SIS_MboxGet()) != NULL )
    {
      /* delay request message triggering */
      if( ps_msg->e_mType == e_IMSG_TRGDLRQ )
      { 
        if( b_tskSts == k_SLV_OPERATIONAL )
        {
          /* timeout for next delay request is already over
             just waiting for next arrival of a sync message, to 
             optimize the calculation of one-way-delay */
          if( o_waitForSync == TRUE )
          {            
            /* actualize value of the delay request sequence number and
               initialize 'last delay request' object */
            aw_seqId[w_actIfIdx]++;
            s_lastDlrq.w_SeqId = aw_seqId[w_actIfIdx];      
            /* reset timestamp to unvalid */
            s_lastDlrq.s_TsTx.u48_sec = 0;
            s_lastDlrq.s_TsTx.dw_Nsec = k_MAX_U32;
            /* Send a delay request */
            if( s_lastDlrq.o_prntIsUc == TRUE )
            {
              ps_pAddr = &s_lastDlrq.s_prntPAddr;
            }
            else
            {
              ps_pAddr = NULL;
            }
            if( DIS_SendDelayReq(ps_pAddr,
                                 SLV_as_portId[w_actIfIdx].w_portNmb-1,
                                 s_lastDlrq.w_SeqId,
                                 &s_lastDlrq.s_TsTx) == TRUE )
            {
              /* set expecting flag */
              o_expcDelResp = TRUE;
            }  
            /* reset flag */
            o_waitForSync = FALSE;        
          }
          else
          {
            /* do nothing */
          }
        }
        else
        {
          /* do nothing */
        }      
      }
      /* a delay response msg  */
      else if( ps_msg->e_mType == e_IMSG_DLRSP )
      {
        /* get the message */
        ps_msgDRsp = ps_msg->s_pntData.ps_dlrsp; 
        if( b_tskSts == k_SLV_OPERATIONAL )
        { 
/* go find the match based on sequence number (kjh) */
          if(!Get_Delay_Timestamp(ps_msgDRsp->s_ptpHead.w_seqId, &s_matchedDlrq))
          {
/* fail */
          }

          /* check : */
          /* - if it coresponds with the sequence id 
               of the delay request message */
          if((ps_msgDRsp->s_ptpHead.w_seqId == s_matchedDlrq.w_SeqId)         &&
          /* - if we are waiting for the delay response */   
             (s_matchedDlrq.s_TsTx.dw_Nsec != k_MAX_U32)                      &&
          /* - if the delay response is addressed to this node */
             (PTP_ComparePortId(&ps_msgDRsp->s_reqPortId,
                                &SLV_as_portId[w_actIfIdx]) == PTP_k_SAME) &&
          /* - if it is from the actual master */
             (PTP_ComparePortId(&ps_msgDRsp->s_ptpHead.s_srcPortId,
                                &s_matchedDlrq.s_prntPid) == PTP_k_SAME))
          {
            /* Allocate memory for the Time difference 
               and for the rx and tx timestamps to pass it to the CTL task */
            ps_tiStmd = (PTP_t_TmIntv*)SIS_Alloc(sizeof(PTP_t_TmIntv));
            ps_rxTs   = (PTP_t_TmStmp*)SIS_Alloc(sizeof(PTP_t_TmStmp));
            ps_txTs   = (PTP_t_TmStmp*)SIS_Alloc(sizeof(PTP_t_TmStmp));
            if((ps_tiStmd != NULL)&&(ps_rxTs != NULL)&&(ps_txTs != NULL))
            {
              /* Get the Rx timestamp out of the delay response message */
              *ps_rxTs = ps_msgDRsp->s_recvTs;
              /* get the slave to master delay */
#ifndef LET_ALL_TS_THROUGH
              if( PTP_SubtrTmStmpTmStmp(ps_rxTs,
                                        &s_matchedDlrq.s_TsTx,
                                        ps_tiStmd) == FALSE )
              {
                /* time difference not applicable to time interval type */
                /* discard - wait for clock reset */
                /* free memory */
                SIS_Free(ps_tiStmd);
                SIS_Free(ps_rxTs);
                SIS_Free(ps_txTs);
              }
              else
#else
              PTP_SubtrTmStmpTmStmp(ps_rxTs,
                                        &s_matchedDlrq.s_TsTx,
                                        ps_tiStmd);
#endif
              {
                /* subtract correction field */
                ps_tiStmd->ll_scld_Nsec -= ps_msgDRsp->s_ptpHead.ll_corField;
                /* get tx-Timestamp to pass it to Unit CTL */
                *ps_txTs = s_matchedDlrq.s_TsTx;
                /* pack the timestamp in a SIS message */
                s_msg.e_mType             = e_IMSG_STMD;
                s_msg.s_pntData.ps_tmIntv = ps_tiStmd;  
                s_msg.s_pntExt1.ps_tmStmp = ps_rxTs;
                s_msg.s_pntExt2.ps_tmStmp = ps_txTs;
                s_msg.s_etc1.w_u16   = w_actIfIdx;
                s_msg.s_etc2.c_s8    = ps_msgDRsp->s_ptpHead.c_logMeanMsgIntv;
                /* send it to the control task */
                while( SIS_MboxPut(CTL_TSK,&s_msg) == FALSE )
                {
                  /* set warning */
                  PTP_SetError(k_SLV_ERR_ID,SLV_k_MBOX_WAIT,e_SEVC_WARN);
                  SIS_MboxWListAdd(w_hdl,CTL_TSK);
                  SIS_Break(w_hdl,1); /*lint !e646 !e717 */ 
                }
              }
            }
            else
            {
              /* free allocated memory */
              if( ps_tiStmd != NULL )
              {
                SIS_Free(ps_tiStmd);
              }
              if( ps_rxTs != NULL )
              {
                SIS_Free(ps_rxTs);
              }
              if( ps_txTs != NULL )
              {
                SIS_Free(ps_txTs);
              }
              /* set an error */
              PTP_SetError(k_SLV_ERR_ID,SLV_k_MPOOL,e_SEVC_WARN);
            }         
            /* reset expecting flag */
            o_expcDelResp = FALSE;    
          }
          else
          {
            /* message corresponds not to the last delay request - 
               do nothing */
          }          
        }        
        else
        {
          /* there should be no message in this state */
          PTP_SetError(k_SLV_ERR_ID,SLV_k_MSG_SLD,e_SEVC_ERR);
        }
        /* free memory */
        SIS_Free(ps_msgDRsp);
      }
      else if( ps_msg->e_mType == e_IMSG_NEW_MST )
      {
        /* get the port id of the new master */
        ps_mstPId   = ps_msg->s_pntData.ps_pId;
        ps_mstPAddr = ps_msg->s_pntExt1.ps_pAddr;
        /* store parent pid */
        s_lastDlrq.s_prntPid = *ps_mstPId;
        /* store communication type (unicast/multicast) */
        s_lastDlrq.o_prntIsUc = ps_msg->s_etc2.o_bool;
        /* store port address struct, if it is an unicast master */
        if( s_lastDlrq.o_prntIsUc == TRUE )
        {
          /* copy port address */
          s_lastDlrq.s_prntPAddr.e_netwProt = ps_mstPAddr->e_netwProt;
          s_lastDlrq.s_prntPAddr.w_AddrLen  = ps_mstPAddr->w_AddrLen;
          PTP_BCOPY(s_lastDlrq.s_prntPAddr.ab_Addr,
                    ps_mstPAddr->ab_Addr,
                    ps_mstPAddr->w_AddrLen);  
          /* set delay request interval mode 
             to send delay request after sync */
          s_lastDlrq.o_randIntv = FALSE;
        }
        else
        {
          /* set delay request interval mode to send 
             delay request randomly */
          s_lastDlrq.o_randIntv = TRUE;
        }
        /* get the port on which the node is slave */
        w_actIfIdx = ps_msg->s_etc1.w_u16;
        /* set delay request timeout to start immediately */
        SIS_TimerStart(w_hdl,2);
        /* avoid error */
        o_expcDelResp = FALSE;
        /* set task to operational */
        b_tskSts = k_SLV_OPERATIONAL;          
      }
      else
      { 
        /* error - unknown message */
        PTP_SetError(k_SLV_ERR_ID,SLV_k_UNKWN_MSG,e_SEVC_ERR);
      }
      /* release message box entry */
      SIS_MboxRelease(); 
    }
    /* got ready through timeout ?  */
    if( SIS_TimerStsRead( w_hdl ) == SIS_k_TIMER_EXPIRED )
    {
      /* handle timeout just in operational status */
      if( b_tskSts == k_SLV_OPERATIONAL )
      {       
        /* check, if the last expected delay response arrived */
        if( o_expcDelResp == TRUE )
        {
          o_expcDelResp = FALSE;          
          /* send message to unit CTL - error handling by CTL task */
          s_msg.e_mType          = e_IMSG_MDLRSP; 
          s_msg.s_pntData.ps_pId = &s_lastDlrq.s_prntPid;
          s_msg.s_etc1.w_u16     = w_actIfIdx;
          s_msg.s_etc2.o_bool    = s_lastDlrq.o_prntIsUc;
          /* try to send error to CTL task */
          if( SIS_MboxPut(CTL_TSK,&s_msg) == FALSE )
          {
            /* set warning */
            PTP_SetError(k_SLV_ERR_ID,SLV_k_MBOX_WAIT,e_SEVC_WARN);
          }          
        }
        else
        {
          /* do nothing - delay response was received */
        }
#if 0
// jyang: Make delay request independent of sync message
        /* if interval is static and delay request shall be sent 
           right after sync message (just possible, if interval 
           is at least 4 times of the minimal polling interval */
        if( (s_lastDlrq.o_randIntv == FALSE) && 
            (SLV_adw_DelReqIntvTmo[w_actIfIdx] >= 4) )
        {
          /* timeout to send the next delay request
           -> set waiting flag */
          o_waitForSync = TRUE;
          /* restart sync delay timeout */
          SIS_TimerStart(w_hdl,SLV_adw_DelReqIntvTmo[w_actIfIdx]);
        }
        /* interval is randomized */
        else
#endif
        {          
          /* actualize value of the delay request sequence number and
              initialize 'last delay request' object */
          aw_seqId[w_actIfIdx]++;
          s_lastDlrq.w_SeqId = aw_seqId[w_actIfIdx];      
          /* reset timestamp to unvalid */
          s_lastDlrq.s_TsTx.u48_sec = 0;
          s_lastDlrq.s_TsTx.dw_Nsec = k_MAX_U32;
          if( s_lastDlrq.o_prntIsUc == TRUE )
          {
            ps_pAddr = &s_lastDlrq.s_prntPAddr;
          }
          else
          {
            ps_pAddr = NULL;
          }
          /* Send a delay request */
          if( DIS_SendDelayReq(ps_pAddr,
                                SLV_as_portId[w_actIfIdx].w_portNmb-1,
                                s_lastDlrq.w_SeqId,
                                &s_lastDlrq.s_TsTx) == TRUE )
          {
/* store in buffer (kjh)*/
            Put_Delay_Timestamp(&s_lastDlrq);
            /* set expecting flag */
            o_expcDelResp = TRUE;
          }  
          /* restart sync delay timeout with random interval */
// jyang: Not use randow interval
//          SIS_TimerStart(w_hdl,GetDelReqIntv(w_actIfIdx));   
          SIS_TimerStart(w_hdl,SLV_adw_DelReqIntvTmo[w_actIfIdx]);
        }
      }
      else
      {
        /* no operation in STOPPED state */
      }      
    }    
    /* cooperative multitasking - give cpu to other tasks */
    SIS_Break(w_hdl,2); /*lint !e646 !e717*/ 
  }
  SIS_Return(w_hdl,0); /*lint !e717 !e744*/
}
#if 0
/*************************************************************************
**    static functions
*************************************************************************/

/***********************************************************************
**
** Function    : GetDelReqIntv
**
** Description : This function determines the delay_req interval. The random
**               distribution shall be a uniform random distribution with a 
**               minimum value of 0 and a maximum value of 
**               {2 ^ (logMinDelayReqInterval+1)} seconds The interval is
**               randomized, to avoid a capacity overload for the master, that 
**               has to answer delay requests for all clocks in his segment.
**              
** Parameters  : w_ifIdx (IN) - interface index of actual slave interface
**
** Returnvalue : UINT32 - time interval in SIS ticks to next delay_req msg
**
**                      
** Remarks     : function is reentrant
**
***********************************************************************/
static UINT32 GetDelReqIntv(UINT16 w_ifIdx)
{  
  UINT32 dw_rand;

  /* get a random number R in the closed interval
     0 < R < (2*delay request interval) */
  dw_rand = (UINT32)(((UINT32)PTP_RAND()) % 
                                  ((2*SLV_adw_DelReqIntvTmo[w_ifIdx])-1))+2;
  return dw_rand;
}
#endif // #if 0
#endif /* #if( k_CLK_DEL_E2E == TRUE ) */
#endif /*#if(( k_CLK_IS_OC == TRUE)||(k_CLK_IS_BC))*/
