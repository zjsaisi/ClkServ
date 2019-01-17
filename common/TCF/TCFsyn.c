/*************************************************************************
**    (C) 2008 - 2009 IXXAT Automation GmbH, all rights reserved
**************************************************************************
**
**       File: TCFsyn.c 
**
**    Summary: The transparent clock forward unit 
**             is responsible to forward all received smessages 
**             and correct for residence time and path delay.
**             This file forwards sync messages and the appropriate
**             follow up messages.
**
**    Version: 1.01.01
**
**************************************************************************
**************************************************************************
**
**  Functions: TCFsyn_Init
**             TCFsyn_Task
**
**             StoreTcCorSynTbl
**             ClearTcCorSynTbl
**             FreeTcCor
**             SearchTcCorSynTbl
**             Fwd_Sync
**             Fwd_FlwUp
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
#include "PTP/PTPdef.h"
#include "SIS/SIS.h"
#include "GOE/GOE.h"
#include "NIF/NIF.h"
#include "DIS/DIS.h"
#include "PTP/PTP.h"
#include "TCF/TCF.h"
#include "TCF/TCFint.h"

/*************************************************************************
**    global variables
*************************************************************************/

#if( k_TWO_STEP == TRUE )
/* table to store data for the sync residence time correction */
static TCF_t_TcCorSyn *TCF_aaps_synCor[k_NUM_IF][k_AMNT_SYN_COR];
#endif

/*************************************************************************
**    static constants, types, macros, variables
*************************************************************************/

/*************************************************************************
**    static function-prototypes
*************************************************************************/
static BOOLEAN StoreTcCorSynTbl(TCF_t_TcCorSyn *ps_tcCor,UINT16 w_recvIf);
static void ClearTcCorSynTbl(void);
static void FreeTcCor(UINT16 w_ifIdx,UINT32 dw_entry);
static BOOLEAN SearchTcCorSynTbl(UINT16             w_recvIf,
                                 const PTP_t_PortId *ps_pId,
                                 UINT16             w_seqId,
                                 UINT32             *pdw_entry);
static UINT32 Fwd_Sync(UINT8              *pb_sync,
                       UINT16             w_seqId,
                       BOOLEAN            o_twoStep,
                       UINT16             w_rcvIfIdx,
                       const PTP_t_TmStmp *ps_rxTs,
                       PTP_t_TmStmp       *as_txTs,
                       BOOLEAN            *ao_sent,
                       const PTP_t_PortId *ps_sendpId);
static void Fwd_FlwUp(UINT8              *pb_flwUp,
                      const PTP_t_TmStmp *ps_rxTs,
                      const PTP_t_TmStmp *as_txTs,
                      const BOOLEAN      *ao_sent,
                      UINT16             w_rcvIfIdx,
                      const PTP_t_PortId *ps_sendpId);
/*************************************************************************
**    global functions
*************************************************************************/

/*************************************************************************
**
** Function    : TCFsyn_Init
**
** Description : Initializes the Unit TCF sync.
**
** Parameters  : -
**
** Returnvalue : -
**                      
** Remarks     : function is reentrant
**
***********************************************************************/
void TCFsyn_Init( void )
{
  UINT16 w_i;
  UINT32 dw_i;
#if( k_TWO_STEP == TRUE )
  for( w_i = 0 ; w_i < k_NUM_IF ; w_i++ )
  {
    /* initialize sync correction table */
    for( dw_i = 0 ; dw_i < k_AMNT_SYN_COR ; dw_i++ )
    {
      TCF_aaps_synCor[w_i][dw_i] = NULL;
    }
  }
#endif /* #if( k_TWO_STEP == TRUE ) */
}

/***********************************************************************
**
** Function    : TCFsyn_Task
**
** Description : The task forwards sync- and follow up messages.
**              
** Parameters  : w_hdl        (IN) - the task-handle
**
** Returnvalue : -
**                      
** Remarks     : function is reentrant
**
***********************************************************************/
void TCFsyn_Task(UINT16 w_hdl )
{  
  UINT16            w_rcvIfIdx;
  UINT8             *pb_rcvMsg = NULL; 
  static TCF_t_TcCorSyn    *ps_tcCor = NULL;
  PTP_t_MboxMsg     *ps_msg = NULL;
  UINT16            w_i;
  PTP_t_TmStmp      *ps_rxTs = NULL;
  PTP_t_PortId      s_pId;
  UINT16            w_seqId;
  UINT16            w_flags;
  BOOLEAN           o_twoStep;
  UINT32            dw_entry = 0;
  BOOLEAN           o_found;
  PTP_t_TmStmp      as_txTs[k_NUM_IF];
  BOOLEAN           ao_sent[k_NUM_IF];

  /* multitasking restart point - generates a 
     jump to the last BREAK at env  */
  SIS_Continue(w_hdl);
    
  while( TRUE )/*lint !e716 */
  {
    /* got ready through message arrival ?  */
    while( (ps_msg = (PTP_t_MboxMsg*)SIS_MboxGet()) != NULL )
    {
      /* ANNOUNCE MESSAGE */
      if( ps_msg->e_mType == e_IMSG_FWD_ANNC )
      {
        /* get message */
        pb_rcvMsg = ps_msg->s_pntData.pb_u8;
        /* get receiving port */
        w_rcvIfIdx = ps_msg->s_etc1.w_u16;
        /* get sender pId */
        NIF_UnPackHdrPortId(&s_pId,pb_rcvMsg); 
        for( w_i = 0 ; w_i < TCF_dw_amntNetIf ; w_i++)
        {
          /* forward the message without convertion on all interfaces except the receiving */
          if( w_i != w_rcvIfIdx )
          {
            /* check, if the message is from this clock -> 
                avoid broadcast-storms in network cycles */
            if( PTP_CompareClkId(&TCF_as_pId[0].s_clkId,
                                 &s_pId.s_clkId) != PTP_k_SAME )
            {
              /* send raw data frame */
              DIS_SendRawData(pb_rcvMsg,
                              k_SOCK_GNRL,
                              0,
                              w_i,
                              (k_PTPV2_HDR_SZE + k_PTPV2_ANNC_PLD),
                              TRUE,
                              NULL);/*lint !e534*/
            }
          }
        }
        /* free allocated memory */
        SIS_Free(pb_rcvMsg);
      }
      /* SYNC MESSAGE */
      else if(ps_msg->e_mType == e_IMSG_FWD_SYN )
      {
        /* get receiving port */
        w_rcvIfIdx = ps_msg->s_etc1.w_u16;
        /* extract needed values */
        pb_rcvMsg  = ps_msg->s_pntData.pb_u8;
        ps_rxTs    = ps_msg->s_pntExt1.ps_tmStmp;
        /* get sequence id */
        w_seqId    = NIF_UNPACKHDR_SEQID(pb_rcvMsg);
        /* get two-step flag */
        w_flags    = NIF_UNPACKHDR_FLAGS(pb_rcvMsg);
        o_twoStep  = GET_FLAG(w_flags,k_FLG_TWO_STEP); 
        /* get sender pId */
        NIF_UnPackHdrPortId(&s_pId,pb_rcvMsg);  
        /* is there a follow up message to be received ? */
        if( o_twoStep == TRUE )        
        {
          if( SearchTcCorSynTbl(w_rcvIfIdx,&s_pId,w_seqId,&dw_entry) == TRUE )
          { 
            /* is there really a follow up message ? 
               Otherwise this is a duplicated sync message */
            if( TCF_aaps_synCor[w_rcvIfIdx][dw_entry]->pb_flwUp != NULL )
            {
              /* forward sync message */
              if( Fwd_Sync(pb_rcvMsg,w_seqId,o_twoStep,w_rcvIfIdx,ps_rxTs,
                           as_txTs,ao_sent,&s_pId) > 0)
              {
                /* forward follow up message */
                Fwd_FlwUp(TCF_aaps_synCor[w_rcvIfIdx][dw_entry]->pb_flwUp,
                          ps_rxTs,as_txTs,ao_sent,w_rcvIfIdx,&s_pId);
              }
            }          
            /* free allocated memory */
            SIS_Free(pb_rcvMsg); 
            SIS_Free(ps_rxTs);
            FreeTcCor(w_rcvIfIdx,dw_entry);
          }
          else
          {
            /* forward sync message */
            if( Fwd_Sync(pb_rcvMsg,w_seqId,o_twoStep,w_rcvIfIdx,ps_rxTs,
                         as_txTs,ao_sent,&s_pId) > 0 )
            {
              /* store sync message data */
              ps_tcCor = (TCF_t_TcCorSyn*)SIS_Alloc(sizeof(TCF_t_TcCorSyn));
              if( ps_tcCor != NULL )
              {
                /* store data */
                for( w_i = 0 ; w_i < TCF_dw_amntNetIf ; w_i++ )
                {
                  ps_tcCor->as_txTs[w_i] = as_txTs[w_i];
                  ps_tcCor->ao_sent[w_i] = ao_sent[w_i];
                }
                ps_tcCor->pb_flwUp = NULL;
                ps_tcCor->pb_sync  = pb_rcvMsg;
                ps_tcCor->ps_rxTs  = ps_rxTs;
                ps_tcCor->s_sndPId = s_pId;
                ps_tcCor->w_seqId  = w_seqId;
                ps_tcCor->dw_tickCnt = SIS_GetTime() + PTP_getTimeIntv(NIF_UNPACKHDR_MMINTV(pb_rcvMsg));
                /* store it in table */
                if( StoreTcCorSynTbl(ps_tcCor,w_rcvIfIdx) == FALSE )
                {
                  /* free allocated memory */
                  SIS_Free(ps_tcCor);
                  SIS_Free(ps_rxTs);
                  SIS_Free(pb_rcvMsg);
                }
              }
              else
              {
                /* free allocated memory */
                SIS_Free(pb_rcvMsg); 
                SIS_Free(ps_rxTs);
              }
            }
            else
            {
              /* free allocated memory */
              SIS_Free(pb_rcvMsg); 
              SIS_Free(ps_rxTs);
            }
          }
        }
        else
        {
          /* forward sync message */
          Fwd_Sync(pb_rcvMsg,w_seqId,o_twoStep,w_rcvIfIdx,ps_rxTs,
                   as_txTs,ao_sent,&s_pId);/*lint !e534*/
          /* free allocated memory */
          SIS_Free(pb_rcvMsg);
          SIS_Free(ps_rxTs);
        }        
      }
      /* FOLLOW UP MESSAGE */
      else if(ps_msg->e_mType == e_IMSG_FWD_FLW )
      {
        /* get receiving port */
        w_rcvIfIdx = ps_msg->s_etc1.w_u16;
        /* get pointer to receive buffer */
        pb_rcvMsg = ps_msg->s_pntData.pb_u8;
        /* get sender pId */
        NIF_UnPackHdrPortId(&s_pId,pb_rcvMsg);
        /* get sequence id */
        w_seqId = NIF_UNPACKHDR_SEQID(pb_rcvMsg);
        /* search for the appropriate table entry */
        o_found = SearchTcCorSynTbl(w_rcvIfIdx,&s_pId,w_seqId,&dw_entry);
        /* if not found, allocate a new one */
        if( o_found == FALSE )
        {
          /* allocate a new struct */
          ps_tcCor = (TCF_t_TcCorSyn*)SIS_Alloc(sizeof(TCF_t_TcCorSyn));
          if( ps_tcCor == NULL )
          {
            /* set error */
            PTP_SetError(k_TCF_ERR_ID,TCF_k_ERR_MPOOL_SYN,e_SEVC_ERR);    
            /* free allocated memory */
            SIS_Free(pb_rcvMsg);
          }
          else
          {
            /* fill data in struct */
            ps_tcCor->pb_flwUp = pb_rcvMsg;
            ps_tcCor->pb_sync  = NULL;      
            ps_tcCor->ps_rxTs  = NULL;
            ps_tcCor->s_sndPId = s_pId;
            ps_tcCor->w_seqId  = w_seqId;
            /* wait at least one message interval 
              to get all data before destroying sturct */
            ps_tcCor->dw_tickCnt = SIS_GetTime() + 
                                   PTP_getTimeIntv(NIF_UNPACKHDR_MMINTV(pb_rcvMsg));
            /* store the struct in the table */
            if( StoreTcCorSynTbl(ps_tcCor,w_rcvIfIdx) == FALSE )
            {
              /* free allocated memory */
              SIS_Free(ps_tcCor);
              SIS_Free(pb_rcvMsg);              
            }  
          }
        }
        else
        {
          if( (TCF_aaps_synCor[w_rcvIfIdx][dw_entry]->ps_rxTs != NULL) &&
              (TCF_aaps_synCor[w_rcvIfIdx][dw_entry]->pb_sync != NULL))
          {
            /* forward follow up message on all interfaces except the receiving */
            Fwd_FlwUp(pb_rcvMsg,
                      TCF_aaps_synCor[w_rcvIfIdx][dw_entry]->ps_rxTs,
                      TCF_aaps_synCor[w_rcvIfIdx][dw_entry]->as_txTs,
                      TCF_aaps_synCor[w_rcvIfIdx][dw_entry]->ao_sent,
                      w_rcvIfIdx,
                      &s_pId);
          }
          /* free allocated memory */
          SIS_Free(pb_rcvMsg);
          FreeTcCor(w_rcvIfIdx,dw_entry);
        }
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
      /* clear sync correction table */
      ClearTcCorSynTbl();
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
  SIS_Return(w_hdl,2);/*lint !e744*/
}

/*************************************************************************
**    static functions
*************************************************************************/

/*************************************************************************
**
** Function    : StoreTcCorSynTbl
**
** Description : Stores a TcCor struct in the sync table to wait
**               for the follow up message and for the tx timestamps.
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
static BOOLEAN StoreTcCorSynTbl(TCF_t_TcCorSyn *ps_tcCor,UINT16 w_recvIf)
{
  BOOLEAN o_ret = FALSE;
  UINT32 dw_i;

  for( dw_i = 0 ; dw_i < k_AMNT_SYN_COR ; dw_i++ )
  {
    /* if this is a free entry - store the data */
    if( TCF_aaps_synCor[w_recvIf][dw_i] == NULL )
    {
      /* register new entry */
      TCF_aaps_synCor[w_recvIf][dw_i] = ps_tcCor;
      o_ret = TRUE;    
      break;  
    }
    else
    {
      /* check age of entry */
      if( SIS_TIME_OVER(TCF_aaps_synCor[w_recvIf][dw_i]->dw_tickCnt ))
      {
        /* too old - destroy */
        FreeTcCor(w_recvIf,dw_i);
        /* entry is now empy - register new entry */
        TCF_aaps_synCor[w_recvIf][dw_i] = ps_tcCor;
        o_ret = TRUE; 
        break;       
      }
    }
  }
  return o_ret;
}

/*************************************************************************
**
** Function    : ClearTcCorSynTbl
**
** Description : Removes all old TcCor struct in the sync table.
**
** Parameters  : -
**               
** Returnvalue : -
** 
** Remarks     : -
**  
***********************************************************************/
static void ClearTcCorSynTbl(void)
{
  UINT16  w_intFc;
  UINT32  dw_i;

  /* all interfaces */
  for( w_intFc = 0 ; w_intFc < k_NUM_IF ; w_intFc++ )
  {
    /* all entries of the table */
    for( dw_i = 0 ; dw_i < k_AMNT_SYN_COR ; dw_i++ )
    {
      /* if this is a free entry - store the data */
      if( TCF_aaps_synCor[w_intFc][dw_i] != NULL )
      {
        /* check age of entry */
        if( SIS_TIME_OVER(TCF_aaps_synCor[w_intFc][dw_i]->dw_tickCnt )) 
        {
          /* too old - destroy */
          FreeTcCor(w_intFc,dw_i);
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
** Function    : FreeTcCor
**
** Description : Deallocates a TcCor struct and the containing memory 
**
** Parameters  : w_ifIdx (IN) - communication interface index in table
**               dw_entry  (IN) - entry in port table
**               
** Returnvalue : -
** 
** Remarks     : -
**  
***********************************************************************/
static void FreeTcCor(UINT16 w_ifIdx,UINT32 dw_entry)
{
  /* free all containing memory */
  /* sync message buffer */
  if( TCF_aaps_synCor[w_ifIdx][dw_entry]->pb_sync != NULL )
  {
    SIS_Free(TCF_aaps_synCor[w_ifIdx][dw_entry]->pb_sync);
    TCF_aaps_synCor[w_ifIdx][dw_entry]->pb_sync = NULL;
  }
  /* follow up message buffer */
  if( TCF_aaps_synCor[w_ifIdx][dw_entry]->pb_flwUp != NULL )
  {
    SIS_Free(TCF_aaps_synCor[w_ifIdx][dw_entry]->pb_flwUp);
    TCF_aaps_synCor[w_ifIdx][dw_entry]->pb_flwUp = NULL;
  }
  /* rx timestamp */
  if( TCF_aaps_synCor[w_ifIdx][dw_entry]->ps_rxTs != NULL )
  {
    SIS_Free(TCF_aaps_synCor[w_ifIdx][dw_entry]->ps_rxTs);
    TCF_aaps_synCor[w_ifIdx][dw_entry]->ps_rxTs = NULL;
  }
  /* free the memory of the struct itself */
  SIS_Free(TCF_aaps_synCor[w_ifIdx][dw_entry]);
  TCF_aaps_synCor[w_ifIdx][dw_entry] = NULL;
}

/*************************************************************************
**
** Function    : SearchTcCorSynTbl
**
** Description : Searches for a TcCor struct in the sync table that 
**               matches the port ID and the sequence id
**
** Parameters  : w_recvIf  (IN)  - receiving communication interface
**               ps_pId    (IN)  - port id to search
**               w_seqId   (IN)  - sequence id to search
**               pdw_entry (OUT) - number of entry, if found
**               
** Returnvalue : TRUE  - struct was found 
**               FALSE - struct was not found
** 
** Remarks     : -
**  
***********************************************************************/
static BOOLEAN SearchTcCorSynTbl(UINT16             w_recvIf,
                                 const PTP_t_PortId *ps_pId,
                                 UINT16             w_seqId,
                                 UINT32             *pdw_entry)
{
  BOOLEAN o_found = FALSE; 
  UINT32  dw_i;
  /* set entry to unvalid value */
  *pdw_entry = k_AMNT_SYN_COR;
  /* Search for it */
  for( dw_i = 0 ; dw_i < k_AMNT_SYN_COR ; dw_i++ )
  {
    /* if this is not a free entry compare the seqId */
    if( TCF_aaps_synCor[w_recvIf][dw_i] != NULL )
    {
      /* compare sequence id */
      if( TCF_aaps_synCor[w_recvIf][dw_i]->w_seqId == w_seqId )
      {
        /* compare port id */
        if( PTP_ComparePortId(&TCF_aaps_synCor[w_recvIf][dw_i]->s_sndPId,
                              ps_pId)==PTP_k_SAME )
        {
          /* found */
          o_found = TRUE;
          *pdw_entry = dw_i;
          break;   
        }
      }
    }
  }
  return o_found;
}

/*************************************************************************
**
** Function    : Fwd_Sync
**
** Description : Forwards a sync message on all interfaces except the 
**               receiving interface.
**
** Parameters  : pb_sync    (IN) - sync message to forward
**               w_seqId    (IN) - sequence id of sync message
**               o_twoStep  (IN) - two-step flag of received sync message
**               w_rcvIfIdx (IN) - port where sync was received
**               ps_rxTs    (IN) - rx timestamp of sync message
**               as_txTs   (OUT) - tx timestamp array of sync message
**               ao_sent   (OUT) - flag array shows if sync was sent 
**               ps_sendpId (IN) - sender port id
**               
** Returnvalue : UINT32 - amount of successive forwarded sync messages
** 
** Remarks     : -
**  
***********************************************************************/
static UINT32 Fwd_Sync(UINT8              *pb_sync,
                       UINT16             w_seqId,
                       BOOLEAN            o_twoStep,
                       UINT16             w_rcvIfIdx,
                       const PTP_t_TmStmp *ps_rxTs,
                       PTP_t_TmStmp       *as_txTs,
                       BOOLEAN            *ao_sent,
                       const PTP_t_PortId *ps_sendpId)
{
  UINT16       w_txIfIdx;
  PTP_t_TmIntv s_corr;
  UINT32       dw_sentSync = 0;

/* change two-step flag if this is a two-step clock */
#if( k_TWO_STEP == TRUE )  
  if( o_twoStep == FALSE )
  {
    /* change two-step-flag to TRUE */
    NIF_PackHdrFlag(pb_sync,k_FLG_TWO_STEP,TRUE);
  }
#endif /* #if( k_TWO_STEP == TRUE )   */
  /* forward the message on all interfaces except the receiving */
  for( w_txIfIdx = 0 ; w_txIfIdx < TCF_dw_amntNetIf ; w_txIfIdx++)
  {    
    if( w_txIfIdx != w_rcvIfIdx )
    {
      /* check, if the message is from this port -> 
                avoid broadcast-storms in network cycles */
      if( PTP_CompareClkId(&TCF_as_pId[w_txIfIdx].s_clkId,
                           &ps_sendpId->s_clkId) != PTP_k_SAME )
      {
        /* try to forward sync message */
        ao_sent[w_txIfIdx] = DIS_SendRawData(pb_sync,
                                            k_SOCK_EVT,
                                            w_seqId,
                                            w_txIfIdx,
                                            (k_PTPV2_HDR_SZE + k_PTPV2_SYNC_PLD),
                                            FALSE,
                                            &as_txTs[w_txIfIdx]);
        /* increment number of sent messages */
        if( ao_sent[w_txIfIdx] == TRUE )
        {
          dw_sentSync++;
        }
      }
      else
      {
        ao_sent[w_txIfIdx] = FALSE;
      }
    }
    else
    {
      ao_sent[w_txIfIdx] = FALSE;
    }
  }
#if( k_TWO_STEP == TRUE ) 
  /* if there will be no follow up message, create one */
  if( o_twoStep == FALSE )
  {
    /* send the message on all interfaces except the receiving */
    for( w_txIfIdx = 0 ; w_txIfIdx < TCF_dw_amntNetIf ; w_txIfIdx++)
    { 
      /* all interfaces except the receiving */
      if( w_txIfIdx != w_rcvIfIdx )
      {
        /* just send follow up, if sync was sended */
        if( ao_sent[w_txIfIdx] == TRUE )        
        {
          /* calculate residence time */
          if( PTP_SubtrTmStmpTmStmp(&as_txTs[w_txIfIdx],
                                    ps_rxTs,
                                    &s_corr) == TRUE )
          {
            /* send follow up */
            DIS_SendFlwUpTc(pb_sync,&s_corr,w_txIfIdx);
          }
        }
      }
    }
  }
  return dw_sentSync;
#endif /* #if( k_TWO_STEP == TRUE )   */
}

/*************************************************************************
**
** Function    : Fwd_FlwUp
**
** Description : Forwards a follow up message on all interfaces except the 
**               receiving interface.
**
** Parameters  : pb_flwUp   (IN) - follow-up message to forward
**               ps_rxTs    (IN) - rx timestamp of sync message
**               as_txTs    (IN) - tx timestamps of sync message
**               ao_sent   (OUT) - flag array shows if sync was sent 
**               w_rcvIfIdx (IN) - port where sync was received
**               ps_sendpId (IN) - sender port id
**               
** Returnvalue : -
** 
** Remarks     : -
**  
***********************************************************************/
static void Fwd_FlwUp(UINT8              *pb_flwUp,
                      const PTP_t_TmStmp *ps_rxTs,
                      const PTP_t_TmStmp *as_txTs,
                      const BOOLEAN      *ao_sent,
                      UINT16             w_rcvIfIdx,
                      const PTP_t_PortId *ps_sendpId)
{
  UINT32       w_txIfIdx;
  PTP_t_TmIntv s_corrFlwUp;
  PTP_t_TmIntv s_resTime;
  /* just for P2P devices */
#if( k_CLK_DEL_P2P == TRUE ) 
  PTP_t_TmIntv s_pDel;

  /* get act path delay of receiving interface */
  s_pDel.ll_scld_Nsec = TCF_pf_cbGetActPDel(w_rcvIfIdx);
#endif /* #if( k_CLK_DEL_P2P == TRUE ) */

  /* get correction field value */
  s_corrFlwUp.ll_scld_Nsec = NIF_UNPACKHDR_COR(pb_flwUp);

  for( w_txIfIdx = 0 ; w_txIfIdx < TCF_dw_amntNetIf ; w_txIfIdx++ )
  {
    /* forward follow up message on all ports, where a tx timestamp is stored */
    if( w_txIfIdx != w_rcvIfIdx )
    {
      if( ao_sent[w_txIfIdx] == TRUE )
      {
        /* check, if the message is from this port -> 
           avoid broadcast-storms in network cycles */
        if( PTP_ComparePortId(&TCF_as_pId[w_txIfIdx],ps_sendpId) != PTP_k_SAME )
        {
          /* get residence time */
          if( PTP_SubtrTmStmpTmStmp(&as_txTs[w_txIfIdx],
                                    ps_rxTs,
                                    &s_resTime) == TRUE)
          {
            /* add correction field of follow up message */
            s_resTime.ll_scld_Nsec += s_corrFlwUp.ll_scld_Nsec;
#if( k_CLK_DEL_P2P == TRUE)
            s_resTime.ll_scld_Nsec += s_pDel.ll_scld_Nsec;      
#endif /* #if( k_CLK_DEL_P2P == TRUE) */
            /* repack it in the raw buffer */
            NIF_PACKHDR_CORR(pb_flwUp,&s_resTime.ll_scld_Nsec);
            /* forward follow up message */
            DIS_SendRawData(pb_flwUp,
                            k_SOCK_GNRL,
                            0,
                            w_txIfIdx,
                            k_PTPV2_HDR_SZE+k_PTPV2_FLUP_PLD,
                            TRUE,
                            NULL);   /*lint !e534*/
          }
        }
      }
    }
  }
}
#endif /* #if( k_CLK_IS_TC == TRUE ) */





