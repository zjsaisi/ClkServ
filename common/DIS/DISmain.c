/*************************************************************************
**    (C) 2008 - 2009 IXXAT Automation GmbH, all rights reserved
**************************************************************************
**
**       File: DISmain.c 
**    Summary: The Unit DIS is responsible for reading messages from all
**             connected channels and dispatching them to the related Units
**             The Units send their messages to the net over function of 
**             this Unit.
**    Version: 1.01.01
**
**************************************************************************
**************************************************************************
**
**  Functions: DIS_Init
**             DIS_Task
**             DIS_GetErrStr
**             ChangePollIntv
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
#include "GOE/GOE.h"
#include "TSU/TSU.h"
#include "DIS/DIS.h"
#include "DIS/DISint.h"

/*************************************************************************
**    global variables
*************************************************************************/

/* Handles of the tasks to address messageboxes */
UINT16 DIS_aw_MboxHdl_mas[k_NUM_IF];
UINT16 DIS_aw_MboxHdl_mad[k_NUM_IF];
UINT16 DIS_aw_MboxHdl_maa[k_NUM_IF];
#if( k_CLK_DEL_P2P == TRUE )
UINT16 DIS_aw_MboxHdl_p2p_del[k_NUM_IF];
UINT16 DIS_aw_MboxHdl_p2p_resp[k_NUM_IF];
#endif /* #if( k_CLK_DEL_P2P == TRUE ) */

/* the actual sync interval */
UINT32 DIS_dw_poll_tmo;
UINT32 DIS_dw_MaxPollTmo;

/* inbound latency */
PTP_t_TmIntv DIS_as_inbLat[k_NUM_IF];

/* pointer to clock data set */
PTP_t_ClkDs const *DIS_ps_ClkDs = NULL;

/* Net status struct */
DIS_t_NetSts DIS_s_NetSts;

/*************************************************************************
**    static constants, types, macros, variables
*************************************************************************/

/*************************************************************************
**    static function-prototypes
*************************************************************************/
#if( k_CLK_IS_TC == FALSE)
//static void ChangePollIntv(UINT32 dw_rcvdMsg);
#endif
/*************************************************************************
**    global functions
*************************************************************************/

/***********************************************************************
**
** Function    : DIS_Init
**
** Description : Initializes the Unit DIS
**               with the messageboxhandles of the tasks of the Units
**               MST,SLV and MNT and a pointer to the
**               clock data set, to fill changing data from the data set´s
**               in the tx messages and to retrieve the port status.
**              
** Parameters  : pw_hdl_mas      (IN) - pointer to an array of the task-
**                                      handles of the Master sync tasks
**               pw_hdl_mad      (IN) - pointer to an array of the task-
**                                      handles of the Master delay tasks
**               pw_hdl_maa      (IN) - pointer to an array of the task-
**                                      handles of the Master announce tasks
**               pw_hdl_p2p_del  (IN) - pointer to an array of the task-
**                                      handles of the peer to peer delay task
**               pw_hdl_p2p_resp (IN) - pointer to an array of the task-handles
**                                      of the peer to peer delay response tasks
**               ps_clkDataSet   (IN) - read only pointer to clock data set 
**
**
** Returnvalue : -
**                      
** Remarks     : -
**
***********************************************************************/
void DIS_Init(
#if((k_CLK_IS_OC) || (k_CLK_IS_BC))              
              const UINT16  *pw_hdl_mas,
              const UINT16  *pw_hdl_mad,
              const UINT16  *pw_hdl_maa,
#endif
#if(k_CLK_DEL_P2P == TRUE )
              const UINT16  *pw_hdl_p2p_del,
              const UINT16  *pw_hdl_p2p_resp,
#endif /* #if(k_CLK_DEL_P2P == TRUE ) */
              PTP_t_ClkDs const *ps_clkDataSet)
{
  /* Initialize the taskhandles */
#if((k_CLK_IS_OC) || (k_CLK_IS_BC))
  /* for MST_Tasks */ 
  PTP_BCOPY(DIS_aw_MboxHdl_mas,pw_hdl_mas,(k_NUM_IF*sizeof(UINT16)));
  PTP_BCOPY(DIS_aw_MboxHdl_mad,pw_hdl_mad,(k_NUM_IF*sizeof(UINT16)));
  PTP_BCOPY(DIS_aw_MboxHdl_maa,pw_hdl_maa,(k_NUM_IF*sizeof(UINT16)));
#endif /* #if((k_CLK_IS_OC) || (k_CLK_IS_BC)) */
#if(k_CLK_DEL_P2P == TRUE )
  PTP_BCOPY(DIS_aw_MboxHdl_p2p_del,pw_hdl_p2p_del,(k_NUM_IF*sizeof(UINT16)));
  PTP_BCOPY(DIS_aw_MboxHdl_p2p_resp,pw_hdl_p2p_resp,(k_NUM_IF*sizeof(UINT16)));
#endif /* #if(k_CLK_DEL_P2P == TRUE ) */
  /* initialize the read-only pointer to the clock data set */
  DIS_ps_ClkDs = ps_clkDataSet;
  /* set maximum polling interval to 64 milliseconds */
  DIS_dw_MaxPollTmo = PTP_getTimeIntv(-4);
  /* Initialize the poll interval to minimum value 
     (PTP poll interval) */
  DIS_dw_poll_tmo = k_DIS_MIN_POLL_TMO;
}

/***********************************************************************
**
** Function    : DIS_Task
**
** Description : The Dispatchertask handles messages between net and engine.
**               It reads all connected channels, adds the
**               related timestamps to the event-messages and transfers the
**               messages per messagebox to the concerning tasks.
**              
** Parameters  : w_hdl        (IN) - the SIS-task-handle
**
** Returnvalue : -
**                      
** Remarks     : -
**
***********************************************************************/
void DIS_Task(UINT16 w_hdl)
{
  static PTP_t_TmStmp      *ps_rxTs;
  static BOOLEAN           o_done;
  static UINT16            w_ifIdx;
  static UINT32            dw_evnt;
  static INT16             i_len;
  static UINT8             ab_rxMsg[k_MAX_PTP_PLD];
  static BOOLEAN           o_msg;  
  static PTP_t_msgTypeEnum e_msgType;
  static UINT16            w_seqId;
  static PTP_t_PortId      s_pId;
  static PTP_t_PortAddr    s_pAdr;
  static UINT8             b_vers;
  static UINT8             b_currMsgCnt;
#if( k_CLK_IS_TC == FALSE)
  static UINT32            dw_recvdMsg;
#endif /* #if( k_CLK_IS_TC == FALSE) */
 
  /* starting point for the task */
  SIS_Continue(w_hdl); 
        
  while( TRUE ) /*lint !e716 */
  {
    /* Events */
    dw_evnt = SIS_EventGet(k_EVT_ALL);
    if( dw_evnt != 0 )
    {
      /* (Re-)Initialization for the Dispatchertask */
      /* start timer that polls reading from the general ports  */
      if( dw_evnt & k_EV_INIT )
      {
        /* start polling general port */
        SIS_TimerStart(w_hdl,DIS_dw_poll_tmo);
      } 
      else
      {
        /* event not applicable */
        PTP_SetError(k_DIS_ERR_ID,DIS_k_ERR_EVT,e_SEVC_NOTC);
      }
    }   
    /* got ready through message?  */
    while( SIS_MboxGet() != NULL )
    {
      /* message type unknown */ 
      PTP_SetError(k_DIS_ERR_ID,DIS_k_ERR_MSGTP,e_SEVC_NOTC);        
      /* Release Mbox-entry */ 
      SIS_MboxRelease();
    }    
    /* Read-Timeout for reading the ptp messages */
    if( SIS_TimerStsRead( DIS_TSK ) == SIS_k_TIMER_EXPIRED )
    {
#if( k_CLK_IS_TC == FALSE)
      /* reset number of received messages */
      dw_recvdMsg = 0;
#endif
      b_currMsgCnt = 0;
      do
      {
        /* Get the Messages from the event ports */
        for( w_ifIdx = 0 ; w_ifIdx < DIS_s_NetSts.dw_amntIf ; w_ifIdx++ )
        {         
          do
          {
            /* get memory buffer for timestamp */
            ps_rxTs = (PTP_t_TmStmp*)SIS_Alloc(sizeof(PTP_t_TmStmp));
            if( ps_rxTs != NULL )
            {              
              /* preinitialize flag to receive */
              o_msg = FALSE;                  
              /* set RxBuffer size (size of SIS-memory-block) */
              i_len = k_MAX_PTP_PLD;
              /* Get Messages from the event port */
              o_msg = NIF_RecvMsg(w_ifIdx,k_SOCK_EVT,ab_rxMsg,&i_len,&s_pAdr);
              if( o_msg == TRUE )
              {
                b_currMsgCnt++;
                /* get version */
                b_vers = NIF_UNPACKHDR_VERS(ab_rxMsg);
                /* check version */
                if( b_vers == k_PTP_VERSION )
                {
#if( k_CLK_IS_TC == FALSE)
                  /* increment received message count */
                  dw_recvdMsg++;
#endif /* #if( k_CLK_IS_TC == FALSE) */
                  /* extract message type */
                  e_msgType = NIF_UNPACKHDR_MSGTYPE(ab_rxMsg);
                  /* extract sequence id */
                  w_seqId   = NIF_UNPACKHDR_SEQID(ab_rxMsg);
                  /* extract sender port id */
                  NIF_UnPackHdrPortId(&s_pId,ab_rxMsg);
                  /* get rx timestamp */
                  if( TSU_GetRxTimeStamp(w_ifIdx,k_PTP_VERSION,e_msgType,
                                         w_seqId,&s_pId,ps_rxTs) == TRUE )
                  {
                    /* correct for inbound latency */
                    if(PTP_SubtrTmIntvTmStmp(ps_rxTs,
                                            &DIS_as_inbLat[w_ifIdx],
                                            ps_rxTs)== FALSE )
                    {
                      /* free allocated memory */
                      SIS_Free(ps_rxTs);
                    } 
                    else
                    { 
#if( k_CLK_IS_TC == TRUE )
                      /* first dispatch it to the transparent 
                         clock units to forward */
                      DIS_DispEvtMsgTc(w_ifIdx,ab_rxMsg,ps_rxTs,
                                       i_len,e_msgType);
#endif /* #if( k_CLK_IS_TC == TRUE ) */
                      /* dispatch it */ 
                      DIS_DispEvntMsg(w_ifIdx,ab_rxMsg,ps_rxTs,
                                      i_len,e_msgType,&s_pAdr);
                      /* break a while and resume task later */
                      SIS_TaskExeReq(w_hdl);
                      SIS_Break(w_hdl,6);/*lint !e646 !e717*/
                    }
                  }
                  else
                  {
                    /* timestamp error - free allocated memory */
                    SIS_Free(ps_rxTs);
                    /* timestamp error */
                    PTP_SetError(k_DIS_ERR_ID,DIS_k_ERR_RXTS,e_SEVC_ERR);
                  }
                }
                /* other version of PTP */
                else
                {
                  /* get timestamp to flush timestamp Q */
                  TSU_GetRxTimeStamp(w_ifIdx,k_PTP_VERSION,
                                     e_msgType,w_seqId,
                                     &s_pId,ps_rxTs);/*lint !e139 !e534*/
#if( k_CLK_IS_TC == TRUE )
                  /* forward message without residence time correction */
                  DIS_FwMsg(ab_rxMsg,k_SOCK_EVT,(UINT16)i_len,w_ifIdx);
#endif /* #if( k_CLK_IS_TC == TRUE ) */
                  /* wrong PTP version - free allocated memory */
                  SIS_Free(ps_rxTs);
                }
              }
              else
              {
                /* no message - free allocated memory */
                SIS_Free(ps_rxTs);
              }
            }
            else
            {
              /* memory error */
              PTP_SetError(k_DIS_ERR_ID,DIS_k_ERR_MPOOL_1,e_SEVC_ERR);
              /* stop the loop !! */
              o_msg = FALSE;
            }
            if( b_currMsgCnt >= k_NUM_MSG_THRESH )
              o_msg = FALSE;
          }
          while( o_msg == TRUE );          
        } 
        /* Get the Messages from the general ports */
        o_done = TRUE;
        for( w_ifIdx = 0 ; w_ifIdx < DIS_s_NetSts.dw_amntIf ; w_ifIdx++ )
        {       
          /* set RxBuffer size */
          i_len = k_MAX_PTP_PLD;
          /* Read Msg  */
          o_msg = NIF_RecvMsg(w_ifIdx,k_SOCK_GNRL ,ab_rxMsg,&i_len,&s_pAdr);
          if( o_msg == TRUE )
          {
            b_currMsgCnt++;
            /* get version */
            b_vers = NIF_UNPACKHDR_VERS(ab_rxMsg);
            /* check version */
            if( b_vers == k_PTP_VERSION )
            {
#if( k_CLK_IS_TC == FALSE)
              /* increment received message count */
              dw_recvdMsg++;
#endif /* #if( k_CLK_IS_TC == FALSE) */
              /* set flag to immediately read all interfaces again */
              o_done = FALSE;
              /* extract message type */
              e_msgType = NIF_UNPACKHDR_MSGTYPE(ab_rxMsg);
#if( k_CLK_IS_TC == TRUE )
              /* first dispatch it to the transparent clock units to forward */
              DIS_DispGnrlMsgTc(w_ifIdx,ab_rxMsg,i_len,e_msgType);
#endif /* #if( k_CLK_IS_TC == TRUE ) */
              /* dispatch the message to the according task  */
              DIS_DispGnrlMsg(w_ifIdx,ab_rxMsg,i_len,e_msgType,&s_pAdr);
              /* break a while and restart task later */
              SIS_TaskExeReq(w_hdl);
              SIS_Break(w_hdl,8);/*lint !e646 !e717*/
            }
            else
            {
#if( k_CLK_IS_TC == TRUE )
              /* forward message without residence time correction */
              DIS_FwMsg(ab_rxMsg,k_SOCK_GNRL,(UINT16)i_len,w_ifIdx);
#endif /* #if( k_CLK_IS_TC == TRUE ) */
            }
          }
        }
        /* jyang: when number of packets exceeds limit, throw them away */
        if( b_currMsgCnt >= k_NUM_MSG_THRESH)
        {
		  // printf("b_currMsgCnt over threshold, %d\n", b_currMsgCnt);
          while (NIF_RecvMsg(0,k_SOCK_EVT,ab_rxMsg,&i_len,&s_pAdr) == TRUE);
          while (NIF_RecvMsg(0,k_SOCK_GNRL ,ab_rxMsg,&i_len,&s_pAdr) == TRUE);
          o_done = TRUE;
        }
      }
      while( o_done == FALSE);
/* transparent clocks shall not adjust the poll interval 
   due to minimal residence time */
#if( k_CLK_IS_TC == FALSE )      
      /* change poll interval */
// jyang: The algorithm for polling adjustment is incorrect. There is really
//        no need to adjust it. Just poll at max rate.
//      ChangePollIntv(dw_recvdMsg);
#else 
      /* poll at every call of PTP_Main */
      DIS_dw_poll_tmo = k_DIS_MIN_POLL_TMO;      
#endif
      /* normal timeout */
      SIS_TimerStart(w_hdl,DIS_dw_poll_tmo);
    }    
    /* cooperative multitasking - give cpu to other tasks */ 
    SIS_Break(w_hdl,9); /*lint !e717 !e646*/ 
  }
  /* finish multitasking */
  SIS_Return(w_hdl,0); /*lint !e744 */
}

#ifdef ERR_STR
/*************************************************************************
**
** Function    : DIS_GetErrStr
**
** Description : Resolves the error number to the according error string
**
** Parameters  : dw_errNmb (IN) - error number
**               
** Returnvalue : const CHAR*   - pointer to error string
** 
** Remarks     : Gets compiled with definition of ERR_STR.
**  
***********************************************************************/
const CHAR* DIS_GetErrStr(UINT32 dw_errNmb)
{
  const CHAR* pc_ret;
  static CHAR ac_str[50];
  /* return according error string */ 
  switch(dw_errNmb)
  {
    case DIS_k_ERR_MSG_FRMT:
    {
      pc_ret = "Malformed PTP message error";
      break;
    }
    case DIS_k_ERR_MSGTP:
    {
      pc_ret = "Undefined message type error";
      break;
    }
    case DIS_k_ERR_WRG_PORT:
    {
      pc_ret = "Message uses wrong port - message discarded";
      break;
    }
    case DIS_k_ERR_EVT:
    {
      pc_ret = "Event not applicable";
      break;
    }
    case DIS_k_MBOX_WAIT:
    {
      pc_ret = "DIS task waits for message box";
      break;
    }
    case DIS_k_ERR_MSG_MBOX:
    {
      pc_ret = "Message box of other task is full";
      break;
    }
    case DIS_k_ERR_MPOOL_1:
    case DIS_k_ERR_MPOOL_2:
    case DIS_k_ERR_MPOOL_3:
    case DIS_k_ERR_MPOOL_4:
    case DIS_k_ERR_MPOOL_5:
    case DIS_k_ERR_MPOOL_6:
    case DIS_k_ERR_MPOOL_7:
    case DIS_k_ERR_MPOOL_8:
    case DIS_k_ERR_MPOOL_9:
    case DIS_k_ERR_MPOOL_10:
    case DIS_k_ERR_MPOOL_11:
    case DIS_k_ERR_MPOOL_12:
    case DIS_k_ERR_MPOOL_13:
    case DIS_k_ERR_MPOOL_14:
    case DIS_k_ERR_MPOOL_15:
    case DIS_k_ERR_MPOOL_16:
    case DIS_k_ERR_MPOOL_17:
    case DIS_k_ERR_MPOOL_18:
    {
      PTP_SPRINTF((ac_str,"Allocation error %i",dw_errNmb-DIS_k_ERR_MPOOL_1));
      pc_ret = ac_str;
      break;    
    }
    case DIS_k_ERR_RXTS:
    {
      pc_ret = "Rx timestamp error";
      break;
    }
    case DIS_k_ERR_TXTS:
    {
      pc_ret = "Tx timestamp error";
      break;
    }
    case DIS_k_ERR_SEND:
    {
      pc_ret = "Send error";
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
#endif /* #ifdef ERR_STR */

/*************************************************************************
**    static functions
*************************************************************************/
#if( k_CLK_IS_TC == FALSE )
#if 0
/***********************************************************************
**
** Function    : ChangePollIntv
**
** Description : Sets the new poll interval.
**               
** Parameters  : dw_rcvdMsg (IN) - number of received messages
**
** Returnvalue : -
**                      
** Remarks     : -
**
***********************************************************************/
static void ChangePollIntv(UINT32 dw_rcvdMsg)
{
  /* increment poll interval, if no messages were received */
  if( dw_rcvdMsg == 0 )
  {
    /* increment poll interval*/
    DIS_dw_poll_tmo++;
  }
  else
  {
    /* divide poll interval by the number of received messages */
    DIS_dw_poll_tmo = DIS_dw_poll_tmo / (UINT16)dw_rcvdMsg;    
  }
  /* check ranges */
  if( DIS_dw_poll_tmo > DIS_dw_MaxPollTmo )
  {
    DIS_dw_poll_tmo = DIS_dw_MaxPollTmo;
  }
  if( DIS_dw_poll_tmo < k_DIS_MIN_POLL_TMO )
  {
    DIS_dw_poll_tmo = k_DIS_MIN_POLL_TMO;
  }
}
#endif // #if 0
#endif /*#if( k_CLK_IS_TC == FALSE) */
