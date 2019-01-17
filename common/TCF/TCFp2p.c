/*************************************************************************
**    (C) 2008 - 2009 IXXAT Automation GmbH, all rights reserved
**************************************************************************
**
**       File: TCFp2p.c 
**
**    Summary: The transparent clock forward unit 
**             is responsible to forward all received smessages 
**             and correct for residence time and path delay.
**             This file forwards P2P messages, if this is an E2E TC
**
**    Version: 1.01.01
**
**************************************************************************
**************************************************************************
**
**  Functions: TCFp2p_Init
**             TCFp2p_Task
**             AllocTcCorP2P
**             SearchTcCorP2PTbl
**             StoreTcCorP2PTbl
**             ClearTcCorP2PTbl
**             Fwd_PDlrq
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
#if( k_CLK_IS_TC == TRUE )
#if( k_CLK_DEL_E2E == TRUE )
#include "PTP/PTPdef.h"
#include "SIS/SIS.h"
#include "GOE/GOE.h"
#include "NIF/NIF.h"
#include "PTP/PTP.h"
#include "DIS/DIS.h"
#include "TCF/TCF.h"
#include "TCF/TCFint.h"

/*************************************************************************
**    global variables
*************************************************************************/

#if( k_TWO_STEP == TRUE )
/* table to store data for the P2P residence time correction */
TCF_t_TcCorP2P *TCF_aaps_p2pCor[k_NUM_IF][k_AMNT_P2P_COR];
#endif

/*************************************************************************
**    static constants, types, macros, variables
*************************************************************************/
static PTP_t_PortId s_pIdNone = {{{0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0}},0x0};
/*************************************************************************
**    static function-prototypes
*************************************************************************/
static TCF_t_TcCorP2P * AllocTcCorP2P(const UINT8        *pb_msgBuf,
                                      UINT16             w_seqId,
                                      const PTP_t_TmStmp *ps_rxTs,
                                      UINT16             w_rcvIfIdx);
static TCF_t_TcCorP2P* SearchTcCorP2PTbl(UINT16             w_rcvIfIdx,
                                         const PTP_t_PortId *ps_pId,
                                         UINT16             w_seqId,
                                         UINT16             *pw_ifIdx,
                                         UINT32             *pdw_entry);
static BOOLEAN StoreTcCorP2PTbl(TCF_t_TcCorP2P *ps_tcCor,UINT16 w_recvIf);
static void ClearTcCorP2PTbl(void);
static BOOLEAN Fwd_PDlrq(const UINT8   *pb_msgRaw,
                        TCF_t_TcCorP2P *ps_tcCor,
                        UINT16         w_seqId,                        
                        UINT16         w_rcvIfIdx);

/*************************************************************************
**    global functions
*************************************************************************/

/*************************************************************************
**
** Function    : TCFp2p_Init
**
** Description : Initializes the Unit TCF peer to peer.
**
** Parameters  : -
**
** Returnvalue : -
**                      
** Remarks     : function is reentrant
**
***********************************************************************/
void TCFp2p_Init( void )
{
  UINT16 w_i;
  UINT32 dw_i;
#if( k_TWO_STEP == TRUE )
  for( w_i = 0 ; w_i < k_NUM_IF ; w_i++ )
  {
    /* initialize sync correction table */
    for( dw_i = 0 ; dw_i < k_AMNT_P2P_COR ; dw_i++ )
    {
      TCF_aaps_p2pCor[w_i][dw_i] = NULL;
    }
  }
#endif /* #if( k_TWO_STEP == TRUE ) */  
}

/***********************************************************************
**
** Function    : TCFp2p_Task
**
** Description : This task forwards pdelay request- and pdelay response
**               messages and corrects the residence time in pdelay 
**               response follow up messages.
**              
** Parameters  : w_hdl        (IN) - the task-handle
**
** Returnvalue : -
**                      
** Remarks     : function is reentrant
**
***********************************************************************/
void TCFp2p_Task(UINT16 w_hdl )
{  
  UINT16               w_rcvIfIdx;
  UINT8                *pb_rcvMsg; 
  TCF_t_TcCorP2P       *ps_tcCor;
  static PTP_t_MboxMsg *ps_msg;
  PTP_t_TmIntv         s_corr;
  PTP_t_TmStmp         *ps_rxTs;
  PTP_t_PortId         s_pId;
  UINT16               w_seqId;
  UINT16               w_ifIdx;
  UINT32               dw_entry;
  PTP_t_TmStmp         s_txTs;
  BOOLEAN              o_delEntr;
  UINT16               w_flags;
  BOOLEAN              o_twoStep;

#if( k_TWO_STEP == FALSE )
  PTP_t_TmStmp         s_txTs;
#endif /* #if( k_TWO_STEP == FALSE ) */

  /* multitasking restart point - generates a 
     jump to the last BREAK at env  */
  SIS_Continue(w_hdl);

  while( TRUE )/*lint !e716 */
  {
    /* got ready through message arrival ?  */
    while( (ps_msg = (PTP_t_MboxMsg*)SIS_MboxGet()) != NULL )
    {
      /* PDELAY REQUEST MESSAGE */
      if( ps_msg->e_mType == e_IMSG_FWD_P_DLRQ )
      {
        /* get receiving interface */
        w_rcvIfIdx = ps_msg->s_etc1.w_u16;
        /* extract needed values */
        pb_rcvMsg = ps_msg->s_pntData.pb_u8;
        ps_rxTs   = ps_msg->s_pntExt1.ps_tmStmp;
        /* get sequence id */
        w_seqId = NIF_UNPACKHDR_SEQID(pb_rcvMsg);
#if( k_TWO_STEP == TRUE )        
        /* allocate a new struct to store the Pdelay request message */
        ps_tcCor = AllocTcCorP2P(pb_rcvMsg,w_seqId,ps_rxTs,w_rcvIfIdx);
        if( ps_tcCor != NULL )
        { 
          /* forward Pdelay request message */
          if( Fwd_PDlrq(pb_rcvMsg,ps_tcCor,w_seqId,w_rcvIfIdx) == TRUE )
          {
            /* store the struct in the table */
            if( StoreTcCorP2PTbl(ps_tcCor,w_rcvIfIdx) == FALSE )
            {
              /* free allocated memory */
              SIS_Free(ps_tcCor);
            }
          }
          else
          {
            /* free allocated memory */
            SIS_Free(ps_tcCor);
          }
        }
#else /* #if( k_TWO_STEP == TRUE ) */
        /* correction field change must be done by HW */
        for( w_i = 0 ; w_i < TCF_dw_amntNetIf ; w_i++)
        {
          /* forward the message on all interfaces except the receiving */
          if( w_i != w_rcvIfIdx )
          {
            /* try to forward peer delay request message */
            if( DIS_SendRawData(pb_rcvMsg,
                                k_SOCK_EVT_PDEL,
                                w_seqId,
                                w_i,
                                ps_msg->s_etc2.w_u16,
                                TRUE,
                                &s_txTs) == FALSE )
            {
              /* set error */
              PTP_SetError(k_TCF_ERR_ID,TCF_k_SEND_ERR,e_SEVC_NOTC);
            }
          }
        }
#endif /* #if( k_TWO_STEP == TRUE ) */       
        /* free allocated memory */
        SIS_Free(ps_rxTs);
        SIS_Free(pb_rcvMsg);
      }
      /* PDELAY RESPONSE MESSAGE */
      else if(ps_msg->e_mType == e_IMSG_FWD_P_DLRSP )
      {
        /* get receiving interface */
        w_rcvIfIdx = ps_msg->s_etc1.w_u16;
        /* get pointer to receive buffer */
        pb_rcvMsg = ps_msg->s_pntData.pb_u8;
        /* get rx timestamp */
        ps_rxTs   = ps_msg->s_pntExt1.ps_tmStmp;
        /* get requesting pId */
        NIF_UnPackReqPortId(&s_pId,pb_rcvMsg);
        /* get sequence id */
        w_seqId = NIF_UNPACKHDR_SEQID(pb_rcvMsg);
        /* search for the appropriate table entry */
        ps_tcCor = SearchTcCorP2PTbl(w_rcvIfIdx,
                                     &s_pId,
                                     w_seqId,
                                     &w_ifIdx,
                                     &dw_entry);
        /* if not found, free allocated memory */
        if( ps_tcCor == NULL )
        {
          /* do nothing - discard */     
        }
        else
        {
          /* check, if there is an answer already */
          if( ps_tcCor->o_answer == FALSE )
          {
            /* preset deletion flag to TRUE */
            o_delEntr = TRUE;
            /* set answered flag to TRUE */
            ps_tcCor->o_answer = TRUE;
#if( k_TWO_STEP == TRUE )
            /* if there is a tx timestamp */
            if( (ps_tcCor->as_txTs[w_rcvIfIdx].u48_sec != 0 ) &&
                (ps_tcCor->as_txTs[w_rcvIfIdx].dw_Nsec != 0 ))
            {
              /* calculate residence time of Pdelay Request*/
              if(PTP_SubtrTmStmpTmStmp(&ps_tcCor->as_txTs[w_rcvIfIdx],
                                       &ps_tcCor->s_rxTs,
                                      &ps_tcCor->s_tiCorrPDRq) == TRUE )
              {
                /* store responder pId */
                NIF_UnPackHdrPortId(&ps_tcCor->s_rspPId,pb_rcvMsg);
                /* get two-step flag */
                w_flags = NIF_UNPACKHDR_FLAGS(pb_rcvMsg);
                o_twoStep = GET_FLAG(w_flags,k_FLG_TWO_STEP); 
                /* if this is a one-step, change flag to two-step */
                if( o_twoStep == FALSE )
                {
                  /* change two-step-flag to TRUE */
                  NIF_PackHdrFlag(pb_rcvMsg,k_FLG_TWO_STEP,TRUE);
                }
                /* forward peer delay response message on the 
                   rx interface of the peer delay request */
                if( DIS_SendRawData(pb_rcvMsg,
                                    k_SOCK_EVT_PDEL,
                                    w_seqId,
                                    ps_tcCor->w_rxIfIdx,
                                    (k_PTPV2_HDR_SZE + k_PTPV2_P_DLRSP_PLD),
                                    FALSE,
                                    &s_txTs) == TRUE )
                {
                  /* get correction value */
                  if( PTP_SubtrTmStmpTmStmp(&s_txTs,
                                            ps_rxTs,
                                            &ps_tcCor->s_tiCorrPDRsp) == TRUE )
                  {
                    /* everything ok - wait for peer delay response */
                    o_delEntr = FALSE;
                  }
                  /* generate a peer delay response follow up message in 
                     a two-step TC that receives a one-step message */
                  if( o_twoStep == FALSE )
                  {
                    /* add correction fields */
                    s_corr.ll_scld_Nsec  = ps_tcCor->s_tiCorrPDRq.ll_scld_Nsec;
                    s_corr.ll_scld_Nsec += ps_tcCor->s_tiCorrPDRsp.ll_scld_Nsec;
                    /* generate a peer delay response follow up message
                       based on the peer delay response message */
                    DIS_SendPDelRespFlwUpTc(pb_rcvMsg,
                                            &s_corr,
                                            ps_tcCor->w_rxIfIdx);
                    /* delete entry - all done */
                    o_delEntr = TRUE;
                  }
                }
              }
            }
#else /* #if( k_TWO_STEP == TRUE ) */
            ps_tcCor->s_tiCorrPDRq.ll_scld_Nsec = 0;
#endif /* #if( k_TWO_STEP == TRUE ) */
            if( o_delEntr == TRUE )
            {
              /* free table entry */
              SIS_Free(TCF_aaps_p2pCor[w_ifIdx][dw_entry]);
              TCF_aaps_p2pCor[w_ifIdx][dw_entry] = NULL;
            }
          }
        }
        /* free allocated memory */
        SIS_Free(ps_rxTs);
        SIS_Free(pb_rcvMsg);             
      }
      /* PDELAY RESPONSE FOLLOW UP MESSAGE */
      else if(ps_msg->e_mType == e_IMSG_FWD_P_DLRFLW )
      {
        /* get receiving port */
        w_rcvIfIdx = ps_msg->s_etc1.w_u16;
        /* get pointer to receive buffer */
        pb_rcvMsg = ps_msg->s_pntData.pb_u8;
        /* get requesting pId */
        NIF_UnPackReqPortId(&s_pId,pb_rcvMsg);
        /* get sequence id */
        w_seqId = NIF_UNPACKHDR_SEQID(pb_rcvMsg);
        /* search for the appropriate table entry */
        ps_tcCor = SearchTcCorP2PTbl(w_rcvIfIdx,
                                     &s_pId,
                                     w_seqId,
                                     &w_ifIdx,
                                     &dw_entry);
        /* if found, forward */
        if( ps_tcCor != NULL )
        {
          /* extract correction field */
          s_corr.ll_scld_Nsec = NIF_UNPACKHDR_COR(pb_rcvMsg);
          /* add pdelay request and pdelay response residence times */
          s_corr.ll_scld_Nsec += ps_tcCor->s_tiCorrPDRq.ll_scld_Nsec;
          s_corr.ll_scld_Nsec += ps_tcCor->s_tiCorrPDRsp.ll_scld_Nsec;
          /* repack it into the packet buffer */
          NIF_PACKHDR_CORR(pb_rcvMsg,&s_corr.ll_scld_Nsec);
          /* forward peer delay response follow up message on the 
             rx interface of the peer delay request message*/
          if( DIS_SendRawData(pb_rcvMsg,
                              k_SOCK_GNRL_PDEL,
                              0,
                              ps_tcCor->w_rxIfIdx,
                              (k_PTPV2_HDR_SZE + k_PTPV2_P_DLRSP_FLW_PLD),
                              FALSE,
                              NULL) == FALSE )
          {

          }
          /* free table entry */
          SIS_Free(TCF_aaps_p2pCor[w_ifIdx][dw_entry]);
          TCF_aaps_p2pCor[w_ifIdx][dw_entry] = NULL;
        }
        /* free allocated memory */
        SIS_Free(pb_rcvMsg);
      }
      else
      {
        /* set error - unknown message*/
        PTP_SetError(k_TCF_ERR_ID,TCF_k_MSG_ERR,e_SEVC_NOTC);
      } 
      /* release the last mbox entry */
      SIS_MboxRelease();
    }
    /* got ready with expired timer? */
    if( SIS_TimerStsRead( w_hdl ) == SIS_k_TIMER_EXPIRED )
    { 
      /* restart timer */
      SIS_TimerStart(w_hdl,TCF_w_intv); 
      /* clear delay correction table */
      ClearTcCorP2PTbl();
    }    
    else /* ExecReq */
    {
      /* do nothing */
    }
    /* cooperative multitasking 
       set task to blocked, till it is ready through an event,
       message or timeout */
    SIS_Break(w_hdl,1); /*lint !e646 !e717*/  
  } 
  SIS_Return(w_hdl,0);/*lint !e744*/
}

/*************************************************************************
**    static functions
*************************************************************************/

/*************************************************************************
**
** Function    : AllocTcCorP2P
**
** Description : Allocates a TcCor struct for Pdelay requests and 
**               initializes it. 
**
** Parameters  : pb_msgBuf  (IN) - pointer to received delay request message 
**               w_seqId    (IN) - sequence id of message
**               ps_rxTs    (IN) - rx timestamp of delay request message
**               w_rcvIfIdx (IN) - index of interface that received
**                                 the delay request
**               
** Returnvalue : ==NULL - no allocation
**               <>NULL - struct was allocated
** 
** Remarks     : -
**  
***********************************************************************/
static TCF_t_TcCorP2P * AllocTcCorP2P(const UINT8        *pb_msgBuf,
                                      UINT16             w_seqId,
                                      const PTP_t_TmStmp *ps_rxTs,
                                      UINT16             w_rcvIfIdx)
{
  TCF_t_TcCorP2P *ps_tcCor;
  UINT16         w_i;

  /* get memory out of memory pool */
  ps_tcCor = (TCF_t_TcCorP2P*)SIS_Alloc(sizeof(TCF_t_TcCorP2P));
  if( ps_tcCor == NULL )
  {
    /* set error */
    PTP_SetError(k_TCF_ERR_ID,TCF_k_ERR_MPOOL_DEL,e_SEVC_ERR);
  }
  else
  {
    /* fill data in struct */
    /* initialize tx timestamp pointers */
    for( w_i = 0 ; w_i < TCF_dw_amntNetIf ; w_i++)
    {                      
      ps_tcCor->as_txTs[w_i].u48_sec = 0;
      ps_tcCor->as_txTs[w_i].dw_Nsec = 0;
    }  
    ps_tcCor->s_rxTs         = *ps_rxTs;
    ps_tcCor->s_tiCorrPDRq.ll_scld_Nsec  = 0;
    ps_tcCor->s_tiCorrPDRsp.ll_scld_Nsec = 0;
    ps_tcCor->o_answer       = FALSE;
    NIF_UnPackHdrPortId(&ps_tcCor->s_reqPId,pb_msgBuf);
    ps_tcCor->s_rspPId       = s_pIdNone;
    ps_tcCor->w_seqId        = w_seqId;
    /* wait at least one second to get all data before destroying struct */
    ps_tcCor->dw_tickCnt     = SIS_GetTime() + PTP_getTimeIntv(0);
    ps_tcCor->w_rxIfIdx      = w_rcvIfIdx;
  }
  return ps_tcCor;
}

/*************************************************************************
**
** Function    : SearchTcCorP2PTbl
**
** Description : Searches for a TcCor struct in the P2P table that 
**               matches the port ID and the sequence id. The search is
**               done on all interfaces. PDelay Responses, that are received
**               on the same interface as the Pdelay request get deleted
**               to prevent message storms in circular networks.
**
** Parameters  : w_rcvIfIdx (IN) - communication interface NOT to search
**               ps_pId     (IN) - port id to search
**               w_seqId    (IN) - sequence id to search
**               pw_ifIdx  (OUT) - communication interface index result
**               pdw_entry (OUT) - entry index of searched entry
**               
** Returnvalue : !=NULL  - struct was found 
**               ==NULL  - struct was not found
** 
** Remarks     : -
**  
***********************************************************************/
static TCF_t_TcCorP2P* SearchTcCorP2PTbl(UINT16             w_rcvIfIdx,
                                         const PTP_t_PortId *ps_pId,
                                         UINT16             w_seqId,
                                         UINT16             *pw_ifIdx,
                                         UINT32             *pdw_entry)
{
  UINT32 dw_i;
  UINT16 w_ifIdx;
  /* preinitialize struct pointer to NULL */
  TCF_t_TcCorP2P *ps_tcCor = NULL;
  /* Search for it on all interfaces */
  for( w_ifIdx = 0 ; w_ifIdx < TCF_dw_amntNetIf ; w_ifIdx++ )
  {
    /* search all entries */
    for( dw_i = 0 ; dw_i < k_AMNT_P2P_COR ; dw_i++ )
    {
      /* if this is not a free entry compare the seqId */
      if( TCF_aaps_p2pCor[w_ifIdx][dw_i] != NULL )
      {
        /* compare sequence id */
        if( TCF_aaps_p2pCor[w_ifIdx][dw_i]->w_seqId == w_seqId )
        {
          /* compare port id */
          if( PTP_ComparePortId(&TCF_aaps_p2pCor[w_ifIdx][dw_i]->s_reqPId,
                                ps_pId)==PTP_k_SAME )
          {
            
            /* found - delete, if received on same interface as the request */
            if( w_ifIdx == w_rcvIfIdx )
            {
              /* delete entry */
              SIS_Free(TCF_aaps_p2pCor[w_ifIdx][dw_i]);
              TCF_aaps_p2pCor[w_ifIdx][dw_i] = NULL;
              ps_tcCor = NULL;
            }
            else
            {
              /* set pointer to found struct */
              ps_tcCor = TCF_aaps_p2pCor[w_ifIdx][dw_i];
              /* set interface index and entry index of found struct */
              *pw_ifIdx = w_ifIdx;
              *pdw_entry  = dw_i;
            }
            break;   
          }
        }
      }
    }
  }
  return ps_tcCor;
}


/*************************************************************************
**
** Function    : StoreTcCorP2PTbl
**
** Description : Stores a TcCor struct in the delay table to wait
**               for the Pdelay response and the 
**               Pdelay response follow up message.
**
** Parameters  : ps_tcCor (IN) - pointer to struct to store
**               w_recvIf (IN) - number of receiving interface
**               
** Returnvalue : TRUE  - struct is stored
**               FALSE - struct could not be stored
** 
** Remarks     : -
**  
***********************************************************************/
static BOOLEAN StoreTcCorP2PTbl(TCF_t_TcCorP2P *ps_tcCor,UINT16 w_recvIf)
{
  BOOLEAN o_ret = FALSE;
  UINT32 dw_i;

  for( dw_i = 0 ; dw_i < k_AMNT_P2P_COR ; dw_i++ )
  {
    /* if this is a free entry - store the data */
    if( TCF_aaps_p2pCor[w_recvIf][dw_i] == NULL )
    {
      /* register new entry */
      TCF_aaps_p2pCor[w_recvIf][dw_i] = ps_tcCor;
      o_ret = TRUE;
      break;
    }
    else
    {
      /* check age of entry */
      if( SIS_TIME_OVER(TCF_aaps_p2pCor[w_recvIf][dw_i]->dw_tickCnt ))
      {
        /* too old - destroy */
        SIS_Free(TCF_aaps_p2pCor[w_recvIf][dw_i]);
        /* entry is now empy - register new entry */
        TCF_aaps_p2pCor[w_recvIf][dw_i] = ps_tcCor;
        o_ret = TRUE;
        break;
      }
    }
  }
  return o_ret;
}

/*************************************************************************
**
** Function    : ClearTcCorP2PTbl
**
** Description : Removes all old TcCor struct in the delay table.
**
** Parameters  : -
**               
** Returnvalue : -
** 
** Remarks     : -
**  
***********************************************************************/
static void ClearTcCorP2PTbl(void)
{
  UINT32  dw_if;
  UINT32  dw_entr;

  /* all interfaces */
  for( dw_if = 0 ; dw_if < k_NUM_IF ; dw_if++ )
  {
    /* all entries of the table */
    for( dw_entr = 0 ; dw_entr < k_AMNT_P2P_COR ; dw_entr++ )
    {
      /* if this is a free entry - store the data */
      if( TCF_aaps_p2pCor[dw_if][dw_entr] != NULL )
      {
        /* check age of entry */
        if( SIS_TIME_OVER(TCF_aaps_p2pCor[dw_if][dw_entr]->dw_tickCnt )) 
        {
          /* too old - destroy */
          SIS_Free(TCF_aaps_p2pCor[dw_if][dw_entr]);
          TCF_aaps_p2pCor[dw_if][dw_entr] = NULL;
        }
        else
        {
          /* do nothing */
        }
      }
    }
  }
  return;
}


/*************************************************************************
**
** Function    : Fwd_PDlrq
**
** Description : Forwards a Pdelay request message on all interfaces except the 
**               receiving interface.
**
** Parameters  : pb_msgRaw  (IN) - raw Pdelay request message
**               ps_tcCor   (IN) - pointer to table struct
**               w_seqId    (IN) - sequence id of message
**               w_rcvIfIdx (IN) - interface where Pdelay request was received
**               
** Returnvalue : TRUE  - message was forwarded on at least one interface
**               FALSE - message could not be forwarded
** 
** Remarks     : -
**  
***********************************************************************/
static BOOLEAN Fwd_PDlrq(const UINT8    *pb_msgRaw,
                         TCF_t_TcCorP2P *ps_tcCor,
                         UINT16         w_seqId,                        
                         UINT16         w_rcvIfIdx)
{
  UINT16  w_i;
  BOOLEAN o_ret = FALSE;

  for( w_i = 0 ; w_i < TCF_dw_amntNetIf ; w_i++)
  {
    /* forward the message on all interfaces except the receiving */
    if( w_i != w_rcvIfIdx )
    {
      /* try to forward delay request message */
      if( DIS_SendRawData(pb_msgRaw,
                          k_SOCK_EVT_PDEL,
                          w_seqId,
                          w_i,
                          (k_PTPV2_HDR_SZE + k_PTPV2_P_DLRQ_PLD),
                          FALSE,
                          &ps_tcCor->as_txTs[w_i])== FALSE)
      {
        ps_tcCor->as_txTs[w_i].u48_sec = 0;
        ps_tcCor->as_txTs[w_i].dw_Nsec = 0;
      }
      else
      {
        /* return value is TRUE if one transmission was OK */
        o_ret = TRUE;
      }
    }
  }
  return o_ret;
}
#endif /* #if( k_CLK_DEL_E2E == TRUE ) */
#endif /* #if( k_CLK_IS_TC == TRUE ) */
