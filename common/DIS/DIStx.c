/*************************************************************************
**    (C) 2008 - 2009 IXXAT Automation GmbH, all rights reserved
**************************************************************************
**
**       File: DIStx.c 
**    Summary: This module of the unit DIS defines all functions to send
**             and set data to the lower units by the upper units.
**    Version: 1.01.01
**
**************************************************************************
**************************************************************************
**
**  Functions: DIS_SendAnnc
**             DIS_SendSync
**             DIS_SendFollowUp
**             DIS_SendDelayReq
**             DIS_SendDelayResp
**             DIS_Send_MntMsg
**             DIS_FwMntMsg
**             DIS_FwSignMsg
**             DIS_FwMsg
**             DIS_Send_Sign
**             DIS_SendRawData
**             DIS_SendFlwUpTc
**             DIS_SendPDelRespFlwUpTc
**             DIS_SendPdlrq
**             DIS_SendPdlRsp
**             DIS_SendPdlRspFlw
**
**             HandleSendError
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
#include "PTP/PTPdef.h"
#include "SIS/SIS.h"
#include "GOE/GOE.h"
#include "TSU/TSU.h"
#include "DIS/DIS.h"
#include "DIS/DISint.h"

/*************************************************************************
**    global variables
*************************************************************************/
/* outbound latency */
PTP_t_TmIntv DIS_as_outbLat[k_NUM_IF];

/*************************************************************************
**    static constants, types, macros, variables
*************************************************************************/

/*************************************************************************
**    static function-prototypes
*************************************************************************/
static void HandleSendError(UINT16 w_ifIdx);
/*************************************************************************
**    global functions
*************************************************************************/

#if((k_CLK_IS_OC) || (k_CLK_IS_BC))
/*************************************************************************
**
** Function    : DIS_SendAnnc
**
** Description : Sends an announce message 
**
** Parameters  : ps_pAddr (IN) - pointer to destination port address
**                               (=NULL for multicast messages)
**               w_ifIdx  (IN) - communication interface index
**               w_seqId  (IN) - Sequence Id for the Message
** 
** Returnvalue : -
**
** Remarks     : -
**
*************************************************************************/
void DIS_SendAnnc( const PTP_t_PortAddr *ps_pAddr,
                   UINT16 w_ifIdx,UINT16 w_seqId )
{
  /* send packet */
  if( NIF_SendAnnc(ps_pAddr,w_ifIdx,w_seqId,DIS_ps_ClkDs) == FALSE )
  {
    HandleSendError(w_ifIdx);
  }
}

/*************************************************************************
**
** Function    : DIS_SendSync
**
** Description : Sends a Sync message.
**
** See Also    : DIS_SendDelayReq()
**
** Parameters  : ps_pAddr  (IN) - pointer to destination port address
**                                (=NULL for multicast messages)
**               w_ifIdx   (IN) - communication interface index 
**               c_synIntv (IN) - ld sync interval
**               w_seqId   (IN) - Sequence Id for the Message
**               ps_txTs  (OUT) - Tx send-time 
** 
** Returnvalue : TRUE           - success
**               FALSE          - function failed
**
** Remarks     : -
**
*************************************************************************/
BOOLEAN DIS_SendSync( const PTP_t_PortAddr *ps_pAddr,
                      UINT16               w_ifIdx,
                      INT8                 c_synIntv,
                      UINT16               w_seqId, 
                      PTP_t_TmStmp         *ps_txTs)
{
  BOOLEAN o_ret = FALSE;
  /* call send function */
  o_ret = NIF_SendSync(ps_pAddr,w_ifIdx,w_seqId,c_synIntv);
#if( k_TWO_STEP == TRUE )
  if( o_ret == TRUE )
  {
    /* get the timestamp */  
    o_ret = TSU_GetTxTimeStamp(w_ifIdx,k_PTP_VERSION,
                               k_SOCK_EVT,w_seqId,ps_txTs);
    if( o_ret == TRUE )
    {      
      /* correct for outbound latency */
      o_ret = PTP_AddTmIntvTmStmp(ps_txTs,&DIS_as_outbLat[w_ifIdx],ps_txTs);
    }
    else
    {
      /* set error */
      PTP_SetError(k_DIS_ERR_ID,DIS_k_ERR_TXTS,e_SEVC_NOTC);
    }
  }
#endif /* #if( k_TWO_STEP == TRUE ) */
  /* handle error */
  if( o_ret == FALSE )
  {
    HandleSendError(w_ifIdx);
  }
  return o_ret;
}

/*************************************************************************
**
** Function    : DIS_SendFollowUp
**
** Description : Sends a follow up message.
**
** Parameters  : ps_pAddr   (IN) - pointer to destination port address
**                                 (=NULL for multicast messages)
**               w_ifIdx    (IN) - communication interface index
**               w_assSeqId (IN) - Sequence Id of the regarding sync msg
**               ps_tiCorr  (IN) - corrections for fractional nanoseconds,
**                                 residence time in TC,
**                                 path delay in P2P TC and
**                                 assymetry corrections
**               c_syncIntv (IN) - ld sync interval
**               ps_precTs (OUT) - precise origin timestamp of sync msg
**
** Returnvalue : -
**
** Remarks     : -
**
*************************************************************************/
void DIS_SendFollowUp(const PTP_t_PortAddr    *ps_pAddr,
                      UINT16                  w_ifIdx,
                      UINT16                  w_assSeqId,
                      const PTP_t_TmIntv      *ps_tiCorr,                      
                      INT8                    c_syncIntv,
                      const PTP_t_TmStmp      *ps_precTs)
{
  /* send message */
  if( NIF_SendFollowUp(ps_pAddr,
                       w_ifIdx,
                       w_assSeqId,
                       ps_tiCorr,
                       ps_precTs,
                       c_syncIntv) == FALSE )
  {
    HandleSendError(w_ifIdx);
  }
}

/*************************************************************************
**
** Function    : DIS_SendDelayReq
**
** Description : Sends a delay request message.
**
** See Also    : DIS_SendSync()
**
** Parameters  : ps_pAddr  (IN) - pointer to destination port address
**                                (=NULL for multicast messages)
**               w_ifIdx   (IN) - communication interface index
**               w_seqId   (IN) - Sequence Id for the Message
**               ps_txTs  (OUT) - Real Tx send-time for systems that
**                                return the timestamp over GOE
**
** Returnvalue : TRUE                 - success
**               FALSE                - function failed
**
** Remarks     : -
**
*************************************************************************/
BOOLEAN DIS_SendDelayReq( const PTP_t_PortAddr *ps_pAddr,
                          UINT16               w_ifIdx,
                          UINT16               w_seqId, 
                          PTP_t_TmStmp         *ps_txTs)
{
  // volatile int i, j=0;

  BOOLEAN o_ret = FALSE;
  /* call send function */
  o_ret = NIF_SendDelayReq(ps_pAddr,w_ifIdx,w_seqId);
  if( o_ret == TRUE )
  {
    /* get the timestamp */  
    o_ret = TSU_GetTxTimeStamp(w_ifIdx,k_PTP_VERSION,
                               k_SOCK_EVT,w_seqId,ps_txTs);

    if( o_ret == TRUE )
    {      
      /* correct for outbound latency */
      o_ret = PTP_AddTmIntvTmStmp(ps_txTs,&DIS_as_outbLat[w_ifIdx],ps_txTs);
    }
    else
    {
      /* set error */
      PTP_SetError(k_DIS_ERR_ID,DIS_k_ERR_TXTS,e_SEVC_NOTC);
    }
  }
  /* handle error */
//  if( o_ret == FALSE )
//  {
//    HandleSendError(w_ifIdx);
//  }
// Do not send error into system, it will cause GoToFaultyState() to 
// run and shutdown the delay request channel
  return o_ret;
}

/*************************************************************************
**
** Function    : DIS_SendDelayResp
**
** Description : send a delay response message
**
** Parameters  : ps_pAddr  (IN) - pointer to destination port address
**                                (=NULL for multicast messages)
**               w_ifIdx      (IN) - communication interface index
**               ps_reqPortId (IN) - requesting port id 
**               w_reqSeqId   (IN) - requesting sequence Id of the delreq msg
**               ps_tiCorr    (IN) - corrections for fractional nanoseconds,
**                                   residence time in TC,
**                                   path delay in P2P TC and
**                                   assymetry corrections
**               ps_recvTs   (OUT) - receive timestamp of delreq msg
**
** Returnvalue : -
**
** Remarks     : -
**
*************************************************************************/
void DIS_SendDelayResp(const PTP_t_PortAddr *ps_pAddr,
                       UINT16               w_ifIdx,
                       const PTP_t_PortId   *ps_reqPortId,            
                       UINT16               w_reqSeqId,
                       const PTP_t_TmIntv   *ps_tiCorr,
                       const PTP_t_TmStmp   *ps_recvTs)
{
  /* send message */
  if( NIF_SendDelayResp(ps_pAddr,w_ifIdx,ps_reqPortId,
                        w_reqSeqId,ps_tiCorr,
                        DIS_ps_ClkDs->as_PortDs[w_ifIdx].c_mMDlrqIntv,
                        ps_recvTs) == FALSE )
  {
    HandleSendError(w_ifIdx);
  }
}
#endif /* #if((k_CLK_IS_OC) || (k_CLK_IS_BC)) */

/*************************************************************************
**
** Function    : DIS_Send_MntMsg
**
** Description : Sendfunction for all management messages
**
** Parameters  : ps_pAddr       (IN) - pointer to destination port address
**                                     (=NULL for multicast messages)
**               ps_dstPid      (IN) - destination PID
**               w_seqId        (IN) - sequence id of message
**               ps_mntTlv      (IN) - managment TLV to send
**               w_ifIdx        (IN) - comm. interface index to send to
**               b_strtBndrHops (IN) - starting boundary hops
**               b_bndrHops     (IN) - actual boundary hops
**               b_actionFld    (IN) - management action field command
**
** Returnvalue : -
**
** Remarks     : -
**
*************************************************************************/
void DIS_Send_MntMsg(const PTP_t_PortAddr *ps_pAddr,
                     const PTP_t_PortId   *ps_dstPid,
                     UINT16               w_seqId,  
                     const PTP_t_MntTLV   *ps_mntTlv,
                     UINT16               w_ifIdx,
                     UINT8                b_strtBndrHops,
                     UINT8                b_bndrHops,
                     UINT8                b_actionFld)
{
  /* send message */
  if( NIF_Send_MntMsg(ps_pAddr,ps_dstPid,w_seqId,ps_mntTlv,w_ifIdx,
                      b_strtBndrHops,b_bndrHops,b_actionFld) == FALSE )
  {
    HandleSendError(w_ifIdx);
  }
}

/***********************************************************************
**
** Function    : DIS_FwMntMsg
**
** Description : If a received management message was not targeted to 
**               this node, the value of boundary hops is not zero after
**               decrementing, and the node is a boundary clock, the 
**               message shall be forwarded on the other communication 
**               interfaces of the node. If the node is a  
**               transparent clock, the boundary hops field does not 
**               get decremented.
**              
** Parameters  : pb_msg    (IN) - pointer to the management message 
**                                to be retransmitted
**               w_len     (IN) - length of message
**               w_rxIfIdx (IN) - receiving interface index
**
** Returnvalue : -
**                      
** Remarks     : -
**
***********************************************************************/
void DIS_FwMntMsg(const UINT8 *pb_msg,UINT16 w_len,UINT16 w_rxIfIdx)
{
  UINT16       w_ifIdx;
  UINT8        *pb_mem = NULL;
  const PTP_t_ClkId *ps_clkId;
  PTP_t_PortId s_pIdSrc;
#if(k_CLK_IS_TC == FALSE)
  UINT8        b_bndrHops;
#endif
  /* get memory to copy */
  pb_mem = (UINT8*)SIS_Alloc(w_len);
  if( pb_mem != NULL )
  {
    /* copy message */
    PTP_BCOPY(pb_mem,pb_msg,w_len);    
  }
  else
  {
    /* alloc error */
    return;
  }
#if(k_CLK_IS_TC == FALSE)
  /* read boundary hops */
  b_bndrHops = NIF_UNPACK_MNT_BNDRHOPS(pb_msg);
  /* if it no is not equal to zero - retransmit */
  if( b_bndrHops > 0 )
  {
    /* decrement boundary hops field */
    NIF_PACK_MNT_BNDRHOPS(pb_mem,(b_bndrHops-1));    
  }
  else
  {
    /* free memory */
    SIS_Free(pb_mem);
    /* boundary hops is zero - no retransmission */
    return;
  }
  ps_clkId = &DIS_ps_ClkDs->as_PortDs[0].s_portId.s_clkId;
#else
  ps_clkId = &DIS_ps_ClkDs->as_TcPortDs[0].s_pId.s_clkId;
#endif /* #if(k_CLK_IS_TC == FALSE) */
  /* get source port id */
  NIF_UnPackHdrPortId(&s_pIdSrc,pb_msg);
  /* just retransmit, if not from this node */
  if( PTP_CompareClkId(&s_pIdSrc.s_clkId,ps_clkId) != PTP_k_SAME )
  {
    /* retransmit on all other communication interfaces except the receiving */
    for( w_ifIdx = 0 ; w_ifIdx < DIS_s_NetSts.dw_amntIf ; w_ifIdx++ )
    {
      /* Not the receiving interface and is interface initialized ? */
      if( (w_ifIdx != w_rxIfIdx ) &&  
          (DIS_s_NetSts.ao_mcIfInit[w_ifIdx] == TRUE))
      {
        /* send it */
        NIF_SendRawData(pb_mem,k_SOCK_GNRL,w_ifIdx,w_len); /*lint !e534 */
      }
    }  
  }
  /* free memory */
  SIS_Free(pb_mem);
}

/***********************************************************************
**
** Function    : DIS_FwSignMsg
**
** Description : Forwards signaling messages over all
**               communication interfaces except the receiving one.
**
** See Also    : DIS_FwMntMsg
**              
** Parameters  : pb_msg    (IN) - pointer to the signaling message 
**                                to be retransmitted
**               w_len     (IN) - length of message
**               w_rxIfIdx (IN) - receiving interface index
**
** Returnvalue : -
**                      
** Remarks     : -
**
***********************************************************************/
void DIS_FwSignMsg(const UINT8 *pb_msg,UINT16 w_len,UINT16 w_rxIfIdx)
{
  UINT16             w_ifIdx;
  PTP_t_PortId       s_pId;
  const PTP_t_ClkId *ps_clkIdSelf;
  
#if(k_CLK_IS_TC == FALSE)
  ps_clkIdSelf = &DIS_ps_ClkDs->as_PortDs[0].s_portId.s_clkId; 
#else
  ps_clkIdSelf = &DIS_ps_ClkDs->as_TcPortDs[0].s_pId.s_clkId; 
#endif
  /* get source port id */
  NIF_UnPackHdrPortId(&s_pId,pb_msg);
  /* check, if this message is from own node */
  if( PTP_CompareClkId(&s_pId.s_clkId,ps_clkIdSelf) != PTP_k_SAME )
  {  
    /* retransmit on all other communication interfaces except the receiving */
    for( w_ifIdx = 0 ; w_ifIdx < DIS_s_NetSts.dw_amntIf ; w_ifIdx++ )
    {    
      if( (w_ifIdx != w_rxIfIdx )
#if( (k_CLK_IS_OC == TRUE) || (k_CLK_IS_BC == TRUE ))
          && (DIS_ps_ClkDs->as_PortDs[w_ifIdx].e_portSts != e_STS_DISABLED )
#endif
        )
      {
        /* send it */
        NIF_SendRawData(pb_msg,k_SOCK_GNRL,w_ifIdx,w_len); /*lint !e534 */
      }
    }  
  }
  return;
}

/***********************************************************************
**
** Function    : DIS_FwMsg
**
** Description : Forwards received messages over all
**               communication interfaces except the receiving one.
**               This function can be used for:
**               - messages with another PTP version
**               - signaling messages
**               Shall not be used for management messages
**
** See Also    : DIS_FwMntMsg
**              
** Parameters  : pb_msg    (IN) - pointer to the signaling message 
**                                to be retransmitted
**               b_socket  (IN) - socket type (k_SOCK_GNRL,k_SOCK_EVT etc.)
**               w_len     (IN) - length of message
**               w_rxIfIdx (IN) - receiving interface index
**
** Returnvalue : -
**                      
** Remarks     : -
**
***********************************************************************/
void DIS_FwMsg(const UINT8 *pb_msg,UINT8 b_socket,UINT16 w_len,UINT16 w_rxIfIdx)
{
  UINT16       w_ifIdx;
  UINT16       w_seqId;
  PTP_t_TmStmp s_ts;

  w_seqId = NIF_UNPACKHDR_SEQID(pb_msg);
  /* retransmit on all other communication interfaces except the receiving */
  for( w_ifIdx = 0 ; w_ifIdx < DIS_s_NetSts.dw_amntIf ; w_ifIdx++ )
  {    
    if( (w_ifIdx != w_rxIfIdx ) &&/* not the receiving interface */
        (DIS_s_NetSts.ao_mcIfInit[w_ifIdx] == TRUE)) /* interface initialized */
    {
      /* send it */
      DIS_SendRawData(pb_msg,b_socket,w_seqId,
                      w_ifIdx,w_len,TRUE,&s_ts); /*lint !e534 */
      /* discard timestamp if any */
    }
  }  
  return;
}



/*************************************************************************
**
** Function    : DIS_Send_Sign
**
** Description : Send function for signaling messages
**
** Parameters  : ps_pAddr       (IN) - pointer to destination port address
**                                     ( = NULL for multicast messages)
**               ps_srcPid      (IN) - source PID to write in msg header
**               ps_dstPid      (IN) - destination PID
**               w_ifIdx        (IN) - comm. interface index to send over
**               w_seqId        (IN) - sequence id of message
**               pb_tlv         (IN) - TLV to send
**               w_dataLen      (IN) - length of data array (complete tlv size)
**
** Returnvalue : -
**
** Remarks     : -
**
*************************************************************************/
void DIS_Send_Sign(const PTP_t_PortAddr *ps_pAddr,
                   const PTP_t_PortId   *ps_srcPid,
                   const PTP_t_PortId   *ps_dstPid,
                   UINT16               w_ifIdx,
                   UINT16               w_seqId,
                   const UINT8          *pb_tlv,
                   UINT16               w_dataLen)
{
  UINT8 b_domn;
#if((k_CLK_IS_OC) || (k_CLK_IS_BC))
  b_domn = DIS_ps_ClkDs->s_DefDs.b_domn;
#else /* #if((k_CLK_IS_OC) || (k_CLK_IS_BC)) */
  b_domn = DIS_ps_ClkDs->s_TcDefDs.b_primDom;
#endif /* #if((k_CLK_IS_OC) || (k_CLK_IS_BC)) */
  /* send message */
  if( NIF_Send_Sign(ps_pAddr,ps_srcPid,ps_dstPid,w_ifIdx,w_seqId,
                    pb_tlv,w_dataLen,b_domn ) == FALSE)
  {
    /* set error */
    HandleSendError(w_ifIdx);
  }
}

/*************************************************************************
**
** Function    : DIS_SendRawData
**
** Description : Sends a ready compiled buffer without changing from 
**               host to network order.
**
** Parameters  : pb_buf   (IN) - buffer to send
**               b_socket (IN) - socket type (EVENT/GENERAL)
**               w_seqId  (IN) - sequence id of message
**               w_ifIdx  (IN) - communication interface index
**               w_len    (IN) - length of buffer
**               o_flshTs (IN) - flag determines, if timestamp is needed
**               ps_txTs (OUT) - Tx timestamps 
**
** Returnvalue : TRUE            - success
**               FALSE           - function failed
**
** Remarks     : -
**
*************************************************************************/
BOOLEAN DIS_SendRawData(const UINT8  *pb_buf,
                        UINT8        b_socket,
                        UINT16       w_seqId,
                        UINT16       w_ifIdx,
                        UINT16       w_len,
                        BOOLEAN      o_flshTs,
                        PTP_t_TmStmp *ps_txTs)
{
  BOOLEAN o_ret = FALSE;
  
  /* send, if interface is initialized */
  if( DIS_s_NetSts.ao_mcIfInit[w_ifIdx] == TRUE )
  {
    /* send message */
    o_ret = NIF_SendRawData(pb_buf,b_socket,w_ifIdx,w_len);
#if( k_TWO_STEP == TRUE )
    if( o_ret == TRUE ) 
    {
      /* get timestamp for event sockets */
      if((b_socket == k_SOCK_EVT )||(b_socket == k_SOCK_EVT_PDEL))
      {
        /* get the timestamp */   
        o_ret = TSU_GetTxTimeStamp(w_ifIdx,k_PTP_VERSION,
                                   b_socket,w_seqId,ps_txTs);
        if( o_ret == TRUE )
        {      
          /* correct for outbound latency */
          o_ret = PTP_AddTmIntvTmStmp(ps_txTs,&DIS_as_outbLat[w_ifIdx],ps_txTs);
        }
        else
        {
          /* if timestamp is flushed, it is no error, if there is no TS */
          if( o_flshTs == TRUE )
          {
            o_ret = TRUE;
          }
          else
          {
            /* set error */
            PTP_SetError(k_DIS_ERR_ID,DIS_k_ERR_TXTS,e_SEVC_NOTC);
          }
        }
      }   
    }
#endif /* #if( k_TWO_STEP == TRUE ) */
    /* handle error */
    if( o_ret == FALSE )
    {
      HandleSendError(w_ifIdx);
    }
  } 
  return o_ret;
}

#if(k_CLK_IS_TC == TRUE)
/*************************************************************************
**
** Function    : DIS_SendFlwUpTc
**
** Description : Assembles and sends a follow up message in
**               transparent clocks. Is used in TwoStep implementations
**               when sync messages of one-step clocks are forwarded.
**
** Parameters  : pb_syncMsg (IN) - pointer to raw sync message
**               ps_tiCor   (IN) - residence time and path delay
**               w_ifIdx    (IN) - communication interface index to send to
**
** Returnvalue : -
**
** Remarks     : -
**
*************************************************************************/
void DIS_SendFlwUpTc(const UINT8        *pb_syncMsg,
                     const PTP_t_TmIntv *ps_tiCor,
                     UINT16             w_ifIdx)
{
  /* send, if interface is initialized */
  if( DIS_s_NetSts.ao_mcIfInit[w_ifIdx] == TRUE )
  {
    /* call sending function */  
    if( NIF_SendFlwUpTc(pb_syncMsg,ps_tiCor,w_ifIdx) == FALSE )
    {
      HandleSendError(w_ifIdx);
    }
  }
}

/*************************************************************************
**
** Function    : DIS_SendPDelRespFlwUpTc
**
** Description : Assembles and sends a Peer delay response follow up 
**               message in transparent clocks. Is used in TwoStep TCs
**               when Peer delay response messages of one-step clocks 
**               are forwarded.
**
** Parameters  : pb_pDelRsp (IN) - pointer to raw PdelResp message
**               ps_tiCor   (IN) - residence time of PdelReq and PdelResp msgs
**               w_ifIdx    (IN) - communication interface index to send to
**
** Returnvalue : TRUE            - success
**               FALSE           - function failed
**
** Remarks     : -
**
*************************************************************************/
void DIS_SendPDelRespFlwUpTc(const UINT8        *pb_pDelRsp,
                             const PTP_t_TmIntv *ps_tiCor,
                             UINT16             w_ifIdx)
{
  if( DIS_s_NetSts.ao_mcIfInit[w_ifIdx] == TRUE) 
  {
    /* call sending function */  
    if( NIF_SendPDelRespFlwUpTc(pb_pDelRsp,ps_tiCor,w_ifIdx) == FALSE )
    {
      HandleSendError(w_ifIdx);
    }
  }  
}

#endif /* #if(k_CLK_IS_TC == TRUE) */

#if( k_CLK_DEL_P2P == TRUE )
/*************************************************************************
**
** Function    : DIS_SendPdlrq
**
** Description : Sends an peer delay request message
**
** Parameters  : ps_pAddr  (IN) - pointer to destination port address
**                                (=NULL for multicast messages)
**               w_ifIdx  (IN) - communication interface index
**               w_seqId  (IN) - Sequence Id for the Message
**               ps_txTs (OUT) - Real Tx send-time for systems that
**                                return the timestamp over GOE
** 
** Returnvalue : TRUE                 - success
**               FALSE                - function failed
**
** Remarks     : -
**
*************************************************************************/
BOOLEAN DIS_SendPdlrq( const PTP_t_PortAddr *ps_pAddr,
                       UINT16               w_ifIdx,
                       UINT16               w_seqId,
                       PTP_t_TmStmp         *ps_txTs)
{
  BOOLEAN o_ret = FALSE;
  /* send, if interface is initialized */
  if( DIS_s_NetSts.ao_mcIfInit[w_ifIdx] == TRUE )
  {
    /* send packet */
    o_ret = NIF_SendPdlrq(ps_pAddr,w_ifIdx,w_seqId);
    if( o_ret == TRUE )
    {
      /* get the timestamp */   
      o_ret = TSU_GetTxTimeStamp(w_ifIdx,k_PTP_VERSION,
                                 k_SOCK_EVT_PDEL,w_seqId,ps_txTs);
      if( o_ret == TRUE )
      {      
        /* correct for outbound latency */
        o_ret = PTP_AddTmIntvTmStmp(ps_txTs,&DIS_as_outbLat[w_ifIdx],ps_txTs);
      }
      else
      {
        /* set error */
        PTP_SetError(k_DIS_ERR_ID,DIS_k_ERR_TXTS,e_SEVC_NOTC);
      }
    }
    /* handle error */
    if( o_ret == FALSE )
    {
      HandleSendError(w_ifIdx);
    }
  }
  return o_ret;
}

/*************************************************************************
**
** Function    : DIS_SendPdlRsp
**
** Description : Sends a peer delay response message
**
** Parameters  : ps_pAddr  (IN) - pointer to destination port address
**                                (=NULL for multicast messages)
**               w_ifIdx   (IN) - communication interface index
**               w_seqId   (IN) - Sequence Id for the Message
**               ps_reqPId (IN) - requesting port id (of peer delay request)
**               ps_rxTs   (IN) - peer delay request receive timestamp
**               ps_txTs  (OUT) - Real Tx send-time for systems that
**                                 return the timestamp over GOE
** 
** Returnvalue : TRUE            - success
**               FALSE           - function failed
**
** Remarks     : -
**
*************************************************************************/
BOOLEAN DIS_SendPdlRsp( const PTP_t_PortAddr *ps_pAddr,
                        UINT16               w_ifIdx,
                        UINT16               w_seqId,
                        const PTP_t_PortId   *ps_reqPId,
                        const PTP_t_TmStmp   *ps_rxTs,
                        PTP_t_TmStmp         *ps_txTs)
{
  BOOLEAN o_ret = FALSE;
  /* send, if interface is initialized */
  if( DIS_s_NetSts.ao_mcIfInit[w_ifIdx] == TRUE )  
  { 
    /* send packet */
    o_ret = NIF_SendPdlRsp(ps_pAddr,w_ifIdx,w_seqId,ps_reqPId, ps_rxTs);
#if( k_TWO_STEP == TRUE )
    if( o_ret == TRUE )
    {
      /* get the timestamp */   
      o_ret = TSU_GetTxTimeStamp(w_ifIdx,k_PTP_VERSION,
                                 k_SOCK_EVT_PDEL,w_seqId,ps_txTs);
      if( o_ret == TRUE )
      {      
        /* correct for outbound latency */
        o_ret = PTP_AddTmIntvTmStmp(ps_txTs,&DIS_as_outbLat[w_ifIdx],ps_txTs);
      }
      else
      {
        /* set error */
        PTP_SetError(k_DIS_ERR_ID,DIS_k_ERR_TXTS,e_SEVC_NOTC);
      }
    }
#endif /* #if( k_TWO_STEP == TRUE ) */
    /* handle error */
    if( o_ret == FALSE )
    {
      HandleSendError(w_ifIdx);
    }
  }
  return o_ret;
}

/*************************************************************************
**
** Function    : DIS_SendPdlRspFlw
**
** Description : Sends a peer delay response follow up message
**
** Parameters  : ps_pAddr  (IN) - pointer to destination port address
**                                (=NULL for multicast messages)
**               w_ifIdx   (IN) - communication interface index
**               w_seqId   (IN) - Sequence Id for the Message
**               ps_reqPId (IN) - requesting port id (of peer delay request)
**               pll_corr  (IN) - correction field for the 
**                                peer delay response follow up
** 
** Returnvalue : -
**
** Remarks     : -
**
*************************************************************************/
void DIS_SendPdlRspFlw( const PTP_t_PortAddr *ps_pAddr,
                        UINT16               w_ifIdx,
                        UINT16               w_seqId,
                        const PTP_t_PortId   *ps_reqPId,
                        const INT64          *pll_corr)
{
  /* send, if interface is initialized */
  if( DIS_s_NetSts.ao_mcIfInit[w_ifIdx] == TRUE )
  { 
    /* send packet */
    if( NIF_SendPdlRspFlw(ps_pAddr,w_ifIdx,w_seqId,
                          ps_reqPId,pll_corr) == FALSE)
    {
      HandleSendError(w_ifIdx);
    }
  }
}

#endif /* #if( k_CLK_DEL_P2P == TRUE ) */

/*************************************************************************
**    static functions
*************************************************************************/

/***********************************************************************
** 
** Function    : HandleSendError
**
** Description : Handles send error. Sends a error message to CTL task.
**
** Parameters  : w_ifIdx (IN) - comm. interface to handle the send error
** 
** Returnvalue : -
**
** Remarks     : -
**
*************************************************************************/
static void HandleSendError(UINT16 w_ifIdx)
{
  PTP_t_MboxMsg s_msg;
  /* send it to control task */
  s_msg.e_mType      = e_IMSG_SEND_ERR;
  s_msg.s_etc1.w_u16 = w_ifIdx;
  /* set error */
  PTP_SetError(k_DIS_ERR_ID,DIS_k_ERR_SEND,e_SEVC_NOTC);
  SIS_MboxPut(CTL_TSK,&s_msg); /*lint !e534 */
}
