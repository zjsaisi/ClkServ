/*************************************************************************
**    (C) 2008 - 2009 IXXAT Automation GmbH, all rights reserved
**************************************************************************
**
**       File: TCFdel.c  
**
**    Summary: The transparent clock forward unit 
**             is responsible to forward all received messages 
**             and correct for residence time and path delay.
**
**    Version: 1.01.01
**
**************************************************************************
**************************************************************************
**
**  Functions: TCFdel_Init
**             TCFdel_Task
**
**             AllocTcCorDel
**             SearchTcCorDelTbl
**             StoreTcCorDelTbl
**             ClearTcCorDelTbl
**             Fwd_Dlrq
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
/* table to store data for the delay request residence time correction */
TCF_t_TcCorDel *TCF_aaps_delCor[k_NUM_IF][k_AMNT_DRQ_COR];
#endif

/*************************************************************************
**    static constants, types, macros, variables
*************************************************************************/

/*************************************************************************
**    static function-prototypes
*************************************************************************/
static TCF_t_TcCorDel * AllocTcCorDel(const UINT8        *pb_msgBuf,
                                      UINT16             w_seqId,
                                      const PTP_t_TmStmp *ps_rxTs,
                                      UINT16             w_rcvIfIdx);
static TCF_t_TcCorDel* SearchTcCorDelTbl(UINT16             w_rcvIfIdx,
                                         const PTP_t_PortId *ps_pId,
                                         UINT16             w_seqId,
                                         UINT16             *pw_ifIdx,
                                         UINT32             *pdw_entry);
static BOOLEAN StoreTcCorDelTbl(TCF_t_TcCorDel *ps_tcCor,UINT16 w_recvIf);
static void ClearTcCorDelTbl(void);
static BOOLEAN Fwd_Dlrq(const UINT8    *pb_msgRaw,
                        TCF_t_TcCorDel *ps_tcCor,
                        UINT16         w_seqId,                        
                        UINT16         w_rcvIfIdx);

/*************************************************************************
**    global functions
*************************************************************************/

/*************************************************************************
**
** Function    : TCFdel_Init
**
** Description : Initializes the Unit TCF delay.
**
** Parameters  : -
**
** Returnvalue : -
**                      
** Remarks     : function is reentrant
**
***********************************************************************/
void TCFdel_Init( void )
{
  UINT16 w_i;
  UINT32 dw_i;
#if( k_TWO_STEP == TRUE )
  for( w_i = 0 ; w_i < k_NUM_IF ; w_i++ )
  {
    /* initialize sync correction table */
    for( dw_i = 0 ; dw_i < k_AMNT_DRQ_COR ; dw_i++ )
    {
      TCF_aaps_delCor[w_i][dw_i] = NULL;
    }
  }
#endif /* #if( k_TWO_STEP == TRUE ) */  
}

/***********************************************************************
**
** Function    : TCFdel_Task
**
** Description : This task forwards delay request- and delay response
**               messages.
**              
** Parameters  : w_hdl        (IN) - the task-handle
**
** Returnvalue : -
**                      
** Remarks     : function is reentrant
**
***********************************************************************/
void TCFdel_Task(UINT16 w_hdl )
{  
  UINT16               w_rcvIfIdx;
  UINT8                *pb_rcvMsg; 
  TCF_t_TcCorDel       *ps_tcCor;
  static PTP_t_MboxMsg *ps_msg;
  PTP_t_TmIntv         s_corr;
  PTP_t_TmStmp         *ps_rxTs;
  PTP_t_PortId         s_pId;
  UINT16               w_seqId;
  UINT16               w_ifIdx;
  UINT32               dw_entry;
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
      /* DELAY REQUEST MESSAGE */
      if( ps_msg->e_mType == e_IMSG_FWD_DLRQ )
      {
        /* get receiving interface */
        w_rcvIfIdx = ps_msg->s_etc1.w_u16;
        /* extract needed values */
        pb_rcvMsg = ps_msg->s_pntData.pb_u8;
        ps_rxTs   = ps_msg->s_pntExt1.ps_tmStmp;
        /* get sequence id */
        w_seqId = NIF_UNPACKHDR_SEQID(pb_rcvMsg);
#if( k_TWO_STEP == TRUE )        
        /* allocate a new struct to store the delay request message*/
        ps_tcCor = AllocTcCorDel(pb_rcvMsg,w_seqId,ps_rxTs,w_rcvIfIdx);
        if( ps_tcCor != NULL )
        { 
          /* forward delay request message */
          if( Fwd_Dlrq(pb_rcvMsg,ps_tcCor,w_seqId,w_rcvIfIdx) == TRUE )
          {
            /* store the struct in the table */
            if( StoreTcCorDelTbl(ps_tcCor,w_rcvIfIdx) == FALSE )
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
            /* try to forward delay request message */
            if( DIS_SendRawData(pb_rcvMsg,
                                k_SOCK_EVT,
                                w_seqId,
                                w_i,
                                ps_msg->s_etc2.w_u16,
                                FALSE,
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
      /* DELAY RESPONSE MESSAGE */
      else if(ps_msg->e_mType == e_IMSG_FWD_DLRSP )
      {
        /* get receiving interface */
        w_rcvIfIdx = ps_msg->s_etc1.w_u16;
        /* get pointer to receive buffer */
        pb_rcvMsg = ps_msg->s_pntData.pb_u8;
        /* get requesting pId */
        NIF_UnPackReqPortId(&s_pId,pb_rcvMsg);
        /* get sequence id */
        w_seqId = NIF_UNPACKHDR_SEQID(pb_rcvMsg);
        /* search for the appropriate table entry */
        ps_tcCor = SearchTcCorDelTbl(w_rcvIfIdx,
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
          /* if there is an tx timestamp */
          if( (ps_tcCor->as_txTs[w_rcvIfIdx].u48_sec != 0 ) &&
              (ps_tcCor->as_txTs[w_rcvIfIdx].dw_Nsec != 0 ))
          {
            /* calculate correction time */
            if( PTP_SubtrTmStmpTmStmp(&ps_tcCor->as_txTs[w_rcvIfIdx],
                                      &ps_tcCor->s_rxTs,
                                      &s_corr) == TRUE )
            {
              /* add correction field of delay response message */
              s_corr.ll_scld_Nsec += NIF_UNPACKHDR_COR(pb_rcvMsg);
              /* pack result in delay response message */
              NIF_PACKHDR_CORR(pb_rcvMsg,&s_corr.ll_scld_Nsec);
              /* forward delay response message on the 
                rx interface of the delay request */
              DIS_SendRawData(pb_rcvMsg,
                              k_SOCK_GNRL,
                              0,
                              ps_tcCor->w_rxIfIdx,
                              (k_PTPV2_HDR_SZE + k_PTPV2_DLRSP_PLD),
                              FALSE,
                              NULL);/*lint !e534*/
            }
          }
          /* free table entry */
          SIS_Free(TCF_aaps_delCor[w_ifIdx][dw_entry]);
          TCF_aaps_delCor[w_ifIdx][dw_entry] = NULL;
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
      ClearTcCorDelTbl();
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
** Function    : AllocTcCorDel
**
** Description : Allocates a TcCor struct for delay requests 
**               and initializes it.
**
** Parameters  : pb_msgBuf  (IN) - pointer to received delay request message 
**               w_seqId    (IN) - sequence id of message
**               ps_rxTs    (IN) - rx timestamp of delay request message
**               w_rcvIfIdx (IN) - index of interface that received the 
**                                 delay request
**               
** Returnvalue : ==NULL - no allocation
**               <>NULL - struct was allocated
** 
** Remarks     : -
**  
***********************************************************************/
static TCF_t_TcCorDel * AllocTcCorDel(const UINT8        *pb_msgBuf,
                                      UINT16             w_seqId,
                                      const PTP_t_TmStmp *ps_rxTs,
                                      UINT16             w_rcvIfIdx)
{
  TCF_t_TcCorDel *ps_tcCor;
  UINT16         w_i;

  /* get memory out of memory pool */
  ps_tcCor = (TCF_t_TcCorDel*)SIS_Alloc(sizeof(TCF_t_TcCorDel));
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
    NIF_UnPackHdrPortId(&ps_tcCor->s_sndPId,pb_msgBuf);
    ps_tcCor->w_seqId        = w_seqId;
    /* wait at least one second to get all data before destroying struct */
    ps_tcCor->dw_tickCnt     = SIS_GetTime() + PTP_getTimeIntv(0);
    ps_tcCor->w_rxIfIdx      = w_rcvIfIdx;
  }
  return ps_tcCor;
}

/*************************************************************************
**
** Function    : SearchTcCorDelTbl
**
** Description : Searches for a TcCor struct in the delay table that 
**               matches the port ID and the sequence id. The search is
**               done on all interfaces. Delay Responses, that are received
**               on the same interface as the delay request get deleted.
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
static TCF_t_TcCorDel* SearchTcCorDelTbl(UINT16             w_rcvIfIdx,
                                         const PTP_t_PortId *ps_pId,
                                         UINT16             w_seqId,
                                         UINT16             *pw_ifIdx,
                                         UINT32             *pdw_entry)
{
  UINT32 dw_i;
  UINT16 w_ifIdx;
  /* preinitialize struct pointer to NULL */
  TCF_t_TcCorDel *ps_tcCor = NULL;
  /* Search for it on all interfaces */
  for( w_ifIdx = 0 ; w_ifIdx < TCF_dw_amntNetIf ; w_ifIdx++ )
  {
    /* search all entries */
    for( dw_i = 0 ; dw_i < k_AMNT_DRQ_COR ; dw_i++ )
    {
      /* if this is not a free entry compare the seqId */
      if( TCF_aaps_delCor[w_ifIdx][dw_i] != NULL )
      {
        /* compare sequence id */
        if( TCF_aaps_delCor[w_ifIdx][dw_i]->w_seqId == w_seqId )
        {
          /* compare port id */
          if( PTP_ComparePortId(&TCF_aaps_delCor[w_ifIdx][dw_i]->s_sndPId,
                                ps_pId)==PTP_k_SAME )
          {
            
            /* found - delete, if received on same interface as the request */
            if( w_ifIdx == w_rcvIfIdx )
            {
              /* delete entry */
              SIS_Free(TCF_aaps_delCor[w_ifIdx][dw_i]);
              TCF_aaps_delCor[w_ifIdx][dw_i] = NULL;
              ps_tcCor = NULL;
            }
            else
            {
              /* set pointer to found struct */
              ps_tcCor = TCF_aaps_delCor[w_ifIdx][dw_i];
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
** Function    : StoreTcCorDelTbl
**
** Description : Stores a TcCor struct in the delay table to wait
**               for the delay response message and for the tx timestamps.
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
static BOOLEAN StoreTcCorDelTbl(TCF_t_TcCorDel *ps_tcCor,UINT16 w_recvIf)
{
  BOOLEAN o_ret = FALSE;
  UINT32 dw_i;

  for( dw_i = 0 ; dw_i < k_AMNT_DRQ_COR ; dw_i++ )
  {
    /* if this is a free entry - store the data */
    if( TCF_aaps_delCor[w_recvIf][dw_i] == NULL )
    {
      /* register new entry */
      TCF_aaps_delCor[w_recvIf][dw_i] = ps_tcCor;
      o_ret = TRUE;
      break;
    }
    else
    {
      /* check age of entry */
      if( SIS_TIME_OVER(TCF_aaps_delCor[w_recvIf][dw_i]->dw_tickCnt ))
      {
        /* too old - destroy */
        SIS_Free(TCF_aaps_delCor[w_recvIf][dw_i]);
        /* entry is now empy - register new entry */
        TCF_aaps_delCor[w_recvIf][dw_i] = ps_tcCor;
        o_ret = TRUE;
        break;
      }
    }
  }
  return o_ret;
}

/*************************************************************************
**
** Function    : ClearTcCorDelTbl
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
static void ClearTcCorDelTbl(void)
{
  UINT32  dw_intFc;
  UINT32  dw_i;

  /* all interfaces */
  for( dw_intFc = 0 ; dw_intFc < k_NUM_IF ; dw_intFc++ )
  {
    /* all entries of the table */
    for( dw_i = 0 ; dw_i < k_AMNT_DRQ_COR ; dw_i++ )
    {
      /* if this is a free entry - store the data */
      if( TCF_aaps_delCor[dw_intFc][dw_i] != NULL )
      {
        /* check age of entry */
        if( SIS_TIME_OVER(TCF_aaps_delCor[dw_intFc][dw_i]->dw_tickCnt )) 
        {
          /* too old - destroy */
          SIS_Free(TCF_aaps_delCor[dw_intFc][dw_i]);
          TCF_aaps_delCor[dw_intFc][dw_i] = NULL;
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
** Function    : Fwd_Dlrq
**
** Description : Forwards a delay request message on all interfaces except the 
**               receiving interface.
**
** Parameters  : pb_msgRaw  (IN) - raw delay request message
**               ps_tcCor   (IN) - pointer to delay struct
**               w_seqId    (IN) - sequence id of message
**               w_rcvIfIdx (IN) - interface where delay request was received
**               
** Returnvalue : TRUE  - message was forwarded on at least one interface
**               FALSE - message could not be forwarded
** 
** Remarks     : -
**  
***********************************************************************/
static BOOLEAN Fwd_Dlrq(const UINT8    *pb_msgRaw,
                        TCF_t_TcCorDel *ps_tcCor,
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
                          k_SOCK_EVT,
                          w_seqId,
                          w_i,
                          (k_PTPV2_HDR_SZE + k_PTPV2_DLRQ_PLD),
                          FALSE,
                          &ps_tcCor->as_txTs[w_i])== FALSE)
      {
        ps_tcCor->as_txTs[w_i].u48_sec = 0;
        ps_tcCor->as_txTs[w_i].dw_Nsec = 0;
      }
      else
      {
        o_ret = TRUE;
      }
    }
  }
  return o_ret;
}
#endif /* #if( k_CLK_DEL_E2E == TRUE ) */
#endif /* #if( k_CLK_IS_TC == TRUE ) */





