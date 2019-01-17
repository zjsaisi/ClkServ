/*************************************************************************
**    (C) 2008 - 2009 IXXAT Automation GmbH, all rights reserved
**************************************************************************
**
**       File: NIF.h 
**    Summary: Network interface unit
**             Declares the network interface functions and defines the 
**             PTP-message-mapping for the specified communication 
**             technology. Functions and types depend on 
**             platform and/or communication technology. 
**             This implementation refers to ethernet IPv4 communication 
**             technology
**    Version: 1.01.01
**
**************************************************************************
**************************************************************************
**
**  Functions: NIF_PackHdrFlag
**             NIF_PackHdr
**             NIF_PackAnncMsg
**             NIF_PackSyncMsg
**             NIF_PackFlwUpMsg
**             NIF_PackDelRespMsg
**             NIF_PackPDlrq
**             NIF_PackPDlrsp
**             NIF_PackPDlrspFlw
**             NIF_PackTlv
**             NIF_RecvMsg
**             NIF_SendAnnc
**             NIF_SendSync
**             NIF_SendFollowUp
**             NIF_SendDelayReq
**             NIF_SendDelayResp
**             NIF_Send_MntMsg
**             NIF_Send_Sign
**             NIF_SendFlwUpTc
**             NIF_SendPDelRespFlwUpTc
**             NIF_SendRawData
**             NIF_SendPdlrq
**             NIF_SendPdlRsp
**             NIF_SendPdlRspFlw
**             NIF_Init
**             NIF_Set_PTP_subdomain
**             NIF_UnPackHdrPortId
**             NIF_UnPackTrgtPortId
**             NIF_UnPackReqPortId
**             NIF_UnPackHdr
**             NIF_UnPackAnncMsg
**             NIF_UnPackSyncMsg
**             NIF_UnPackFlwUpMsg
**             NIF_UnPackDelRespMsg
**             NIF_UnPackMntMsg
**             NIF_UnpackPDlrq
**             NIF_UnpackPDlrsp
**             NIF_UnpackPDlrspFlw
**             NIF_GetIfAddrs
**
**   Compiler: ANSI-C
**    Remarks:  
**
**************************************************************************
**    all rights reserved
*************************************************************************/

#ifndef __NIF_H__
#define __NIF_H__

#ifdef __cplusplus
extern "C"
{
#endif

/** C2DOC_stop */
/*************************************************************************
**    constants and macros
*************************************************************************/

/* max. length of MNT TLV payload data */
#define k_MAX_TLV_PLD_LEN    (1418)   

/* message sizes in bytes */
#define k_PTPV2_HDR_SZE         (34) /* PTP V2 header size */
#define k_PTPV2_ANNC_PLD        (30) /* announce message payload */
#define k_PTPV2_SYNC_PLD        (10) /* snyc message payload */
#define k_PTPV2_DLRQ_PLD        (10) /* delay request message payload */
#define k_PTPV2_FLUP_PLD        (10) /* follow up message payload */
#define k_PTPV2_DLRSP_PLD       (20) /* delay response message payload */
#define k_PTPV2_MNTHDR_SZE      (14) /* management header extensions payload */
#define k_PTPV2_SIGNHDR_SZE     (10) /* signaling header extensions payload */
#define k_PTPV2_EVT_PAD_SZE    (124) /* size of an event message with padding */
/* P2P TC message sizes */
#define k_PTPV2_P_DLRQ_PLD      (20) /* peer delay request payload */
#define k_PTPV2_P_DLRSP_PLD     (20) /* peer delay response payload */
#define k_PTPV2_P_DLRSP_FLW_PLD (20) /* peer delay response follow up payload */

/* byte offset of data in the PTP V2 headers */
#define k_OFFS_MSG_TYPE          (0)
#define k_OFFS_VER_PTP           (1)
#define k_OFFS_MSG_LEN           (2)
#define k_OFFS_DOM_NMB           (4)
#define k_OFFS_FLAGS             (6)
#define k_OFFS_CORFIELD          (8)
#define k_OFFS_SRCPRTID         (20)
#define k_OFFS_SEQ_ID           (30)
#define k_OFFS_CTRL             (32)
#define k_OFFS_LOGMEAN          (33)
/* byte offset of data in announce message buffer */
#define k_OFFS_TS_ANNC_SEC      (34)
#define k_OFFS_TS_ANNC_NSEC     (40)
#define k_OFFS_CURUTCOFFS       (44)
#define k_OFFS_GM_PRIO1         (47)
#define k_OFFS_GM_CLKQUAL       (48)
#define k_OFFS_GM_PRIO2         (52)
#define k_OFFS_GM_ID            (53)
#define k_OFFS_STEPS_REM        (61)
#define k_OFFS_TMESRC           (63)
/* byte offset of data in sync message buffer */
#define k_OFFS_TS_SYN_SEC       (34)
#define k_OFFS_TS_SYN_NSEC      (40)
/* byte offset of data in follow up message buffer */
#define k_OFFS_PRECTS_SEC       (34)
#define k_OFFS_PRECTS_NSEC      (40)
/* byte offset of data in delay response message buffer */
#define k_OFFS_RECVTS_SEC       (34)
#define k_OFFS_RECVTS_NSEC      (40)
#define k_OFFS_REQSRCPRTID      (44)
/* byte offset of signaling message buffer */
#define k_OFFS_TRGT_ID          (34)
#define k_OFFS_TLVS             (44)
/* byte offsets in management message */
#define k_OFFS_MNT_TRGTPID      (34)
#define k_OFFS_MNT_STRT_BHOPS   (44)
#define k_OFFS_MNT_BHOPS        (45)
#define k_OFFS_MNT_ACTION       (46)
#define k_OFFS_MNT_TLV_TYPE     (48)
#define k_OFFS_MNT_TLV_LEN      (50)
#define k_OFFS_MNT_TLV_ID       (52)
#define k_OFFS_MNT_TLV_DATA     (54)
/* byte offsets in P2P transparent clocks */
#define k_OFFS_P2P_ORIGTS_SEC   (34)
#define k_OFFS_P2P_ORIGTS_NSEC  (40)
#define k_OFFS_P2P_REQTS_SEC    (34)
#define k_OFFS_P2P_REQTS_NSEC   (40)
#define k_OFFS_P2P_REQPRTID     (44)
#define k_OFFS_P2P_RSP_TS_SEC   (34)
#define k_OFFS_P2P_RSP_TS_NSEC  (40)
    
/** C2DOC_start */
    
/*************************************************************************
**    data types
*************************************************************************/

/************************************************************************/
/** NIF_t_PTPV2_Head
                 This struct defines the representation of a Header for
                 PTP V2 Messages. This struct cannot be used to send 
                 over Ethernet.
**/
typedef struct
{
  PTP_t_msgTypeEnum e_msgType;
  UINT8             b_versPTP; 
  UINT16            w_msgLen;
  UINT8             b_dmnNmb;  
  UINT16            w_flags;
  INT64             ll_corField;
  PTP_t_PortId      s_srcPortId;
  UINT16            w_seqId;
  PTP_t_cfEnum      e_ctrl;
  INT8              c_logMeanMsgIntv;
}NIF_t_PTPV2_Head;

/************************************************************************/
/** NIF_t_PTPV2_AnnMsg
                 This struct defines the representation of a 
                 Announce Message to propagate over Ethernet 
**/
typedef struct 
{
  NIF_t_PTPV2_Head  s_ptpHead;
  /* specific data fields */
  PTP_t_TmStmp      s_origTs;
  INT16             i_curUTCOffs;
  UINT8             b_grdMstPrio1;
  PTP_t_ClkQual     s_grdMstClkQual;
  UINT8             b_grdMstPrio2;
  PTP_t_ClkId       s_grdMstId;
  UINT16            w_stepsRemvd;
  PTP_t_tmSrcEnum   e_timeSrc;
}NIF_t_PTPV2_AnnMsg;

/************************************************************************/
/** NIF_t_PTPV2_SyncMsg
                 This struct defines the representation of a Sync Message.
**/
typedef struct
{
  NIF_t_PTPV2_Head s_ptpHead;
  PTP_t_TmStmp     s_origTs;
}NIF_t_PTPV2_SyncMsg;

/************************************************************************/
/** NIF_t_PTPV2_DlrqMsg
                 This struct defines the representation of a Delay Request
                 Message. It is the same structure as a Sync Message
**/
typedef NIF_t_PTPV2_SyncMsg NIF_t_PTPV2_DlrqMsg;

/************************************************************************/
/** NIF_t_PTPV2_FlwUpMsg
                 This struct defines the representation of a Follow Up
                 Message.
**/
typedef struct  
{
  NIF_t_PTPV2_Head s_ptpHead;
  PTP_t_TmStmp     s_precOrigTs;
}NIF_t_PTPV2_FlwUpMsg;

/************************************************************************/
/** NIF_t_PTPV2_DlRspMsg
                 This struct defines the representation of a 
                 Delay Response Message.
**/
typedef struct 
{
  NIF_t_PTPV2_Head  s_ptpHead;
  PTP_t_TmStmp      s_recvTs;
  PTP_t_PortId      s_reqPortId;
}NIF_t_PTPV2_DlRspMsg;

/************************************************************************/
/** NIF_t_PTPV2_SignMsg
                 This struct defines the representation of a 
                 Signaling Message. 
**/
typedef struct 
{
  NIF_t_PTPV2_Head s_ptpHead;
  PTP_t_PortId     s_trgtPrtId;
}NIF_t_PTPV2_SignMsg;

/************************************************************************/
/** NIF_t_PTPV2_MntMsg
                 This struct defines the representation of a 
                 Management Message. 
**/
typedef struct 
{
  NIF_t_PTPV2_Head s_ptpHead;
  PTP_t_PortId     s_trgtPid;
  UINT8            b_strtBndrHops;
  UINT8            b_bndrHops;
  UINT8            b_actField;
  PTP_t_MntTLV     s_mntTlv;   
}NIF_t_PTPV2_MntMsg;

/************************************************************************/
/** NIF_t_PTPV2_PDelReq
                 This struct defines the representation of a 
                 Peer Delay Request Message.
**/
typedef struct
{
  NIF_t_PTPV2_Head s_ptpHead;
  PTP_t_TmStmp     s_origTs;
  /* 10 bytes reserved */
}NIF_t_PTPV2_PDelReq;

/************************************************************************/
/** NIF_t_PTPV2_PDelResp
                 This struct defines the representation of a 
                 Peer Delay Response Message.
**/
typedef struct
{
  NIF_t_PTPV2_Head s_ptpHead;
  PTP_t_TmStmp     s_reqRecptTs;
  PTP_t_PortId     s_reqPId;
}NIF_t_PTPV2_PDelResp;

/************************************************************************/
/** NIF_t_PTPV2_PDlRspFlw
                 This struct defines the representation of a 
                 Peer Delay Response Follow up Message.
**/
typedef struct
{
  NIF_t_PTPV2_Head s_ptpHead; /* PTP V2 header */
  PTP_t_TmStmp     s_respTs;  /* response origin timestamp */
  PTP_t_PortId     s_reqPId;  /* requesting port Id */
}NIF_t_PTPV2_PDlRspFlw;

/*************************************************************************
**    global variables
*************************************************************************/

/*************************************************************************
**    function prototypes
*************************************************************************/

/** C2DOC_stop */

/*************************************************************************
**
** Function    : NIF_PACKHDR_MTYPE
**
** Description : Sets the message type directly in the packet byte array
**
** Returnvalue : -
**               
** Remarks     : -
**
*************************************************************************/
#if( k_HW_NEEDS_PADDING == TRUE )
#define NIF_PACKHDR_MTYPE(pb_buf,e_mType) \
                               (pb_buf[k_OFFS_MSG_TYPE] = (0x10 | (UINT8)e_mType))
#else
#define NIF_PACKHDR_MTYPE(pb_buf,e_mType) \
                               (pb_buf[k_OFFS_MSG_TYPE] = (0x0F & (UINT8)e_mType))
#endif

/** C2DOC_start */

/*************************************************************************
**
** Function    : NIF_PackHdrFlag
**
** Description : Sets a flag in the header of a PTP V2 message struct. 
**
** Parameters  : pb_buf  (IN/OUT) - pointer to header to set flag
**               b_pos   (IN)     - bit position of flag
**               o_val   (IN)     - boolean input value
**
** Returnvalue : -
**               
** Remarks     : -
**
*************************************************************************/
void NIF_PackHdrFlag(UINT8 *pb_buf,UINT8 b_pos,BOOLEAN o_val);

/** C2DOC_stop */

/*************************************************************************
**
** Function    : NIF_PACKHDR_DOMNMB
**
** Description : Sets the domain number directly in the packet byte array
**
** Returnvalue : -
**               
** Remarks     : -
**
*************************************************************************/
#define NIF_PACKHDR_DOMNMB(pb_buf,b_dmnNmb) (pb_buf[k_OFFS_DOM_NMB] = b_dmnNmb)

/*************************************************************************
**
** Function    : NIF_PACKHDR_CORR
**
** Description : Sets the correction field directly in the packet byte array
**
** Returnvalue : -
**               
** Remarks     : -
**
*************************************************************************/
#define NIF_PACKHDR_CORR(pb_buf,pll_corr) \
        (GOE_hton64(&pb_buf[k_OFFS_CORFIELD],(const UINT64*)pll_corr))


/*************************************************************************
**
** Function    : NIF_PACKHDR_CTRL
**
** Description : Sets the control field directly in the packet byte array
**
** Returnvalue : -
**               
** Remarks     : -
**
*************************************************************************/
#define NIF_PACKHDR_CTRL(pb_buf,e_ctrl) (pb_buf[k_OFFS_CTRL] = (UINT8)e_ctrl)

/*************************************************************************
**
** Function    : NIF_PACK_MNT_BNDRHOPS
**
** Description : Sets the boundary hops field directly
**               in the packet byte array
**
** Returnvalue : -
**               
** Remarks     : -
**
*************************************************************************/
#define NIF_PACK_MNT_BNDRHOPS(pb_buf,b_bndrHops) \
            (pb_buf[k_OFFS_MNT_BHOPS] = (UINT8)(b_bndrHops))    

/** C2DOC_start */
            
/*************************************************************************
**
** Function    : NIF_PackHdr
**
** Description : Composes a header out of a message struct.
**
** Parameters  : pb_buf  (OUT) - pointer to buffer to fill
**               ps_hdr  (IN)  - pointer to message header struct
**
** Returnvalue : -
**               
** Remarks     : -
**
*************************************************************************/
void NIF_PackHdr(UINT8 *pb_buf,const NIF_t_PTPV2_Head *ps_hdr);

/*************************************************************************
**
** Function    : NIF_PackAnncMsg
**
** Description : Composes a announce message out of a 
**               announce message struct
**
** Parameters  : pb_buf     (OUT) - pointer to buffer to fill
**               ps_anncMsg (IN)  - pointer to announce message struct
**
** Returnvalue : -
**               
** Remarks     : -
**
*************************************************************************/
void NIF_PackAnncMsg(UINT8 *pb_buf,const NIF_t_PTPV2_AnnMsg *ps_anncMsg);

/*************************************************************************
**
** Function    : NIF_PackSyncMsg
**
** Description : Composes a sync message out of a sync message struct
**
** Parameters  : pb_buf     (OUT) - pointer to buffer to fill
**               ps_syncMsg (IN)  - pointer to sync message struct
**
** Returnvalue : -
**               
** Remarks     : -
**
*************************************************************************/
void NIF_PackSyncMsg(UINT8 *pb_buf,const NIF_t_PTPV2_SyncMsg *ps_syncMsg);

/*************************************************************************
**
** Function    : NIF_PackDelReqMsg
**
** Description : Composes a delay request message out of a delay request
**               message struct
**
** Parameters  : pb_buf     (OUT) - pointer to buffer to fill
**               ps_struct  (IN)  - pointer to delay request message struct
**
** Returnvalue : -
**               
** Remarks     : -
**
*************************************************************************/
#define NIF_PackDelReqMsg(pb_buf,ps_struct) NIF_PackSyncMsg(pb_buf,ps_struct)

/*************************************************************************
**
** Function    : NIF_PackFlwUpMsg
**
** Description : Composes a follow up message out of a 
**               follow up message struct
**
** Parameters  : pb_buf     (OUT) - pointer to buffer to fill
**               ps_flwUpMsg (IN) - pointer to follow up message struct
**               
** Returnvalue : -
**               
** Remarks     : -
**
*************************************************************************/
void NIF_PackFlwUpMsg(UINT8 *pb_buf,const NIF_t_PTPV2_FlwUpMsg *ps_flwUpMsg);


/*************************************************************************
**
** Function    : NIF_PackDelRespMsg
**
** Description : Composes a delay response message out of a 
**               delay response message struct
**
** Parameters  : pb_buf      (OUT) - pointer to buffer to fill
**               ps_delRspMsg (IN) - pointer to follow up message struct
**
** Returnvalue : -
**               
** Remarks     : -
**
*************************************************************************/
void NIF_PackDelRespMsg(UINT8 *pb_buf,const NIF_t_PTPV2_DlRspMsg *ps_delRspMsg);

#if( k_CLK_DEL_P2P == TRUE )
/*************************************************************************
**
** Function    : NIF_PackPDlrq
**
** Description : Composes a peer delay request message out of a 
**               peer delay request message struct
**
** Parameters  : pb_buf   (OUT) - pointer to buffer to fill
**               ps_pDlrq (IN)  - pointer to peer delay request message struct
**
** Returnvalue : -
**               
** Remarks     : -
**
*************************************************************************/
void NIF_PackPDlrq(UINT8 *pb_buf,const NIF_t_PTPV2_PDelReq *ps_pDlrq);

/*************************************************************************
**
** Function    : NIF_PackPDlrsp
**
** Description : Composes a peer delay response message out of a 
**               peer delay response message struct
**
** Parameters  : pb_buf   (OUT) - pointer to buffer to fill
**               ps_pDlrsp (IN) - pointer to peer delay response message struct
**
** Returnvalue : -
**               
** Remarks     : -
**
*************************************************************************/
void NIF_PackPDlrsp(UINT8 *pb_buf,const NIF_t_PTPV2_PDelResp *ps_pDlrsp);

/*************************************************************************
**
** Function    : NIF_PackPDlrspFlw
**
** Description : Composes a peer delay response follow up message out of a 
**               peer delay response follow up message struct
**
** Parameters  : pb_buf      (OUT) - pointer to buffer to fill
**               ps_pDlrspFlw (IN) - pointer to peer delay response 
**                                   follow up message struct
**
** Returnvalue : -
**               
** Remarks     : -
**
*************************************************************************/
void NIF_PackPDlrspFlw(UINT8 *pb_buf,const NIF_t_PTPV2_PDlRspFlw *ps_pDlrspFlw);

#endif /*#if( k_CLK_DEL_P2P == TRUE ) */

/***********************************************************************
**  
** Function    : NIF_PackTlv
**  
** Description : Packs the tlv type and tlv length into the send buffer
**  
** Parameters  : pb_buf (IN) - buffer to send 
**               e_type (IN) - tlv type 
**               w_len  (IN) - tlv data length
**               
** Returnvalue : -
** 
** Remarks     : function is reentrant
**  
***********************************************************************/
void NIF_PackTlv(UINT8* pb_buf,PTP_t_tlvTypeEnum e_type,UINT16 w_len);

/** C2DOC_stop */

/*************************************************************************
**
** Function    : NIF_RecvMsg
**
** Description : Calls the communication-specific function to read from 
**               the specified channel/port.
**
** Parameters  : w_ifIdx       (IN) - communication interface index to read
**               b_socket      (IN) - socket type (evt/general + peer delay)
**               pb_rxBuf     (OUT) - RX buffer to read in
**               pi_bufSize (IN/OUT)- Size of RX buffer
**               ps_pAdr      (OUT) - port address of sender
**                                    (used to determine unicast sender)
**
** Returnvalue : TRUE                  - message was received
**               FALSE                 - no message available
**               
** Remarks     : function is not reentrant, does never block
**
*************************************************************************/
BOOLEAN NIF_RecvMsg( UINT16         w_ifIdx,
                     UINT8          b_socket,
                     UINT8          *pb_rxBuf,
                     INT16          *pi_bufSize,
                     PTP_t_PortAddr *ps_pAdr);

/*************************************************************************
**
** Function    : NIF_SendAnnc
**
** Description : Sends an announce message 
**
** Parameters  : ps_pAddr   (IN) - pointer to destination port address
**                                 (=NULL for multicast messages)
**               w_ifIdx    (IN) - communication interface index
**               w_seqId    (IN) - Sequence Id for the Message
**               ps_clkDs   (IN) - pointer to clock data set
** 
** Returnvalue : TRUE           - success
**               FALSE          - function failed
**
** Remarks     : -
**
*************************************************************************/
BOOLEAN NIF_SendAnnc( const PTP_t_PortAddr *ps_pAddr,
                      UINT16               w_ifIdx,
                      UINT16               w_seqId,
                      PTP_t_ClkDs const    *ps_clkDs);

/*************************************************************************
**
** Function    : NIF_SendSync
**
** Description : Sends a Sync message 
**               by calling the wrapper function 'SendEvntMsg'.
**
** See Also    : NIF_SendDelayReq(),SendEvntMsg()
**
** Parameters  : ps_pAddr  (IN) - pointer to destination port address
**                                (=NULL for multicast messages)
**               w_ifIdx   (IN) - communication interface index 
**               w_seqId   (IN) - Sequence Id for the Message
**               c_synIntv (IN) - sync interval
** 
** Returnvalue : TRUE           - success
**               FALSE          - function failed
**
** Remarks     : -
**
*************************************************************************/
BOOLEAN NIF_SendSync( const PTP_t_PortAddr *ps_pAddr,
                      UINT16               w_ifIdx,
                      UINT16               w_seqId,
                      INT8                 c_synIntv);

/*************************************************************************
**
** Function    : NIF_SendFollowUp
**
** Description : Sends a follow up message.
**
** Parameters  : ps_pAddr  (IN) - pointer to destination port address
**                                (=NULL for multicast messages)
**               w_ifIdx    (IN) - communication interface index
**               w_assSeqId (IN) - Sequence Id of the regarding sync msg
**               ps_tiCorr  (IN) - corrections for fractional nanoseconds,
**                                 residence time in TC,
**                                 path delay in P2P TC and
**                                 assymetry corrections
**               ps_precTs (OUT) - precise origin timestamp of sync msg
**               c_synIntv  (IN) - sync interval
**
** Returnvalue : TRUE  - function succeeded
**               FALSE - function failed
**
** Remarks     : -
**
*************************************************************************/
BOOLEAN NIF_SendFollowUp(const PTP_t_PortAddr *ps_pAddr,
                         UINT16               w_ifIdx,
                         UINT16               w_assSeqId,
                         const PTP_t_TmIntv   *ps_tiCorr,
                         const PTP_t_TmStmp   *ps_precTs,
                         INT8                 c_synIntv);

/*************************************************************************
**
** Function    : NIF_SendDelayReq
**
** Description : Sends a delay request message 
**               by calling the wrapper function 'SendEvntMsg()'.
**
** See Also    : NIF_SendSync(),SendEvntMsg()()
**
** Parameters  : ps_pAddr  (IN) - pointer to destination port address
**                                (=NULL for multicast messages)
**               w_ifIdx   (IN) - communication interface index
**               w_seqId   (IN) - Sequence Id for the Message
**
** Returnvalue : TRUE                 - success
**               FALSE                - function failed
**
** Remarks     : -
**
*************************************************************************/
BOOLEAN NIF_SendDelayReq( const PTP_t_PortAddr *ps_pAddr,
                          UINT16       w_ifIdx,
                          UINT16       w_seqId);

/*************************************************************************
**
** Function    : NIF_SendDelayResp
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
**               c_dlrqIntv   (IN) - minimal delay request interval
**               ps_recvTs   (OUT) - receive timestamp of delreq msg
**
** Returnvalue : TRUE              - success
**               FALSE             - function failed
**
** Remarks     : -
**
*************************************************************************/
BOOLEAN NIF_SendDelayResp(const PTP_t_PortAddr *ps_pAddr,
                          UINT16               w_ifIdx,
                          const PTP_t_PortId   *ps_reqPortId,            
                          UINT16               w_reqSeqId,
                          const PTP_t_TmIntv   *ps_tiCorr,
                          INT8                 c_dlrqIntv,
                          const PTP_t_TmStmp   *ps_recvTs);

/*************************************************************************
**
** Function    : NIF_Send_MntMsg
**
** Description : Sendfunction for all management messages
**
** Parameters  : ps_pAddr       (IN) - pointer to destination port address
**                                     (=NULL for multicast messages)
**               ps_dstPid      (IN) - destination PID
**               w_seqId        (IN) - sequence id of message
**               ps_mntTlv      (IN) - managment TLV to send
**               w_ifIdx        (IN) - communication interface index to send to
**               b_strtBndrHops (IN) - starting boundary hops
**               b_bndrHops     (IN) - actual boundary hops
**               b_actionFld    (IN) - management action field command
**
** Returnvalue : TRUE              - success
**               FALSE             - function failed
**
** Remarks     : -
**
*************************************************************************/
BOOLEAN NIF_Send_MntMsg(const PTP_t_PortAddr *ps_pAddr,
                        const PTP_t_PortId   *ps_dstPid,
                        UINT16               w_seqId,  
                        const PTP_t_MntTLV   *ps_mntTlv,
                        UINT16               w_ifIdx,
                        UINT8                b_strtBndrHops,
                        UINT8                b_bndrHops,
                        UINT8                b_actionFld);

/*************************************************************************
**
** Function    : NIF_Send_Sign
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
**               b_domain       (IN) - actual domain
**
** Returnvalue : TRUE                - success
**               FALSE               - function failed
**
** Remarks     : -
**
*************************************************************************/
BOOLEAN NIF_Send_Sign(const PTP_t_PortAddr *ps_pAddr,
                      const PTP_t_PortId   *ps_srcPid,
                      const PTP_t_PortId   *ps_dstPid,
                      UINT16               w_ifIdx,
                      UINT16               w_seqId,
                      const UINT8          *pb_tlv,
                      UINT16               w_dataLen,
                      UINT8                b_domain);

#if(k_CLK_IS_TC == TRUE)
/*************************************************************************
**
** Function    : NIF_SendFlwUpTc
**
** Description : Assembles and sends a follow up message in
**               transparent clocks. Is used in TwoStep implementations
**               when sync messages of one-step clocks are forwarded.
**
** Parameters  : pb_syncMsg (IN) - pointer to raw sync message
**               ps_tiCor   (IN) - residence time and path delay
**               w_ifIdx    (IN) - communication interface index to send to
**
** Returnvalue : TRUE            - success
**               FALSE           - function failed
**
** Remarks     : -
**
*************************************************************************/
BOOLEAN NIF_SendFlwUpTc(const UINT8        *pb_syncMsg,
                        const PTP_t_TmIntv *ps_tiCor,
                        UINT16             w_ifIdx);

/*************************************************************************
**
** Function    : NIF_SendPDelRespFlwUpTc
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
BOOLEAN NIF_SendPDelRespFlwUpTc(const UINT8        *pb_pDelRsp,
                                const PTP_t_TmIntv *ps_tiCor,
                                UINT16             w_ifIdx);

#endif /* #if(k_CLK_IS_TC == TRUE) */
/*************************************************************************
**
** Function    : NIF_SendRawData
**
** Description : Sends a ready compiled buffer without changing from 
**               host to network order.
**
** Parameters  : pb_buf   (IN) - buffer to send
**               b_socket (IN) - socket type (EVENT/GENERAL)
**               w_ifIdx  (IN) - communication interface index
**               w_len    (IN) - length of buffer
**
** Returnvalue : TRUE            - success
**               FALSE           - function failed
**
** Remarks     : -
**
*************************************************************************/
BOOLEAN NIF_SendRawData(const UINT8  *pb_buf,
                        UINT8        b_socket,
                        UINT16       w_ifIdx,
                        UINT16       w_len);


#if( k_CLK_DEL_P2P == TRUE )
/*************************************************************************
**
** Function    : NIF_SendPdlrq
**
** Description : Sends an peer delay request message
**
** Parameters  : ps_pAddr  (IN) - pointer to destination port address
**                                (=NULL for multicast messages)
**               w_ifIdx  (IN) - communication interface index
**               w_seqId  (IN) - Sequence Id for the Message
** 
** Returnvalue : TRUE                 - success
**               FALSE                - function failed
**
** Remarks     : -
**
*************************************************************************/
BOOLEAN NIF_SendPdlrq( const PTP_t_PortAddr *ps_pAddr,
                       UINT16               w_ifIdx,
                       UINT16               w_seqId);

/*************************************************************************
**
** Function    : NIF_SendPdlRsp
**
** Description : Sends a peer delay response message
**
** Parameters  : ps_pAddr  (IN) - pointer to destination port address
**                                (=NULL for multicast messages)
**               w_ifIdx   (IN) - communication interface index
**               w_seqId   (IN) - Sequence Id for the Message
**               ps_reqPId (IN) - requesting port id (of peer delay request)
**               ps_rxTs   (IN) - peer delay request receive timestamp
** 
** Returnvalue : TRUE           - success
**               FALSE          - function failed
**
** Remarks     : -
**
*************************************************************************/
BOOLEAN NIF_SendPdlRsp( const PTP_t_PortAddr *ps_pAddr,
                        UINT16               w_ifIdx,
                        UINT16               w_seqId,
                        const PTP_t_PortId   *ps_reqPId,
                        const PTP_t_TmStmp   *ps_rxTs);

/*************************************************************************
**
** Function    : NIF_SendPdlRspFlw
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
** Returnvalue : TRUE           - success
**               FALSE          - function failed
**
** Remarks     : -
**
*************************************************************************/
BOOLEAN NIF_SendPdlRspFlw( const PTP_t_PortAddr *ps_pAddr,
                           UINT16               w_ifIdx,
                           UINT16               w_seqId,
                           const PTP_t_PortId   *ps_reqPId,
                           const INT64          *pll_corr);

#endif /* #if( k_CLK_DEL_P2P == TRUE ) */

/***********************************************************************
**
** Function    : NIF_Init
**
** Description : Initializes the Unit NIF
**              
** Parameters  : ps_clkDataSet   (IN) - read only pointer to clock data set 
**
**
** Returnvalue : -
**                      
** Remarks     : -
**
***********************************************************************/
void NIF_Init(PTP_t_ClkDs const *ps_clkDataSet);

/*************************************************************************
**
** Function    : NIF_Set_PTP_subdomain
**
** Description : Sets the PTP subdomain. It is inserted in the tx
**               packets.
**
** Parameters  : b_domain (IN) - domain to communicate with
**
** Returnvalue : -
**               
** Remarks     : -
**
*************************************************************************/
void NIF_Set_PTP_subdomain( UINT8 b_domain );

/*************************************************************************
**
** Function    : NIF_InitMcConn
**
** Description : Calls the communication-specific function to initialize 
**               a multicast communication channel  
**
** Parameters  : w_ifIdx (IN) - specified interface to init
**
** Returnvalue : TRUE         - initialization succeeded
**               FALSE        - initialization failed
**               
** Remarks     : function is not reentrant, does never block
**
*************************************************************************/
#define NIF_InitMcConn(w_ifIdx) GOE_InitMcConn(w_ifIdx)

/*************************************************************************
**
** Function    : NIF_CloseMcConn
**
** Description : Calls the communication-specific function to deinitialize 
**               a multicast communication channel  
**
** Parameters  : w_ifIdx (IN) - specified interface to init
**
** Returnvalue : -
**               
** Remarks     : function is not reentrant, does never block
**
*************************************************************************/
#define NIF_CloseMcConn(w_ifIdx) GOE_CloseMcConn(w_ifIdx)

#if( k_UNICAST_CPBL == TRUE )
/*************************************************************************
**
** Function    : NIF_InitUcConn
**
** Description : Calls the communication-specific function to initialize 
**               a unicast communication channel  
**
** Parameters  : -
**
** Returnvalue : TRUE  - initialization succeeded
**               FALSE - initialization failed
**               
** Remarks     : function is not reentrant, does never block
**
*************************************************************************/
#define NIF_InitUcConn() GOE_InitUcConn()

/*************************************************************************
**
** Function    : NIF_CloseUcConn
**
** Description : Calls the communication-specific function to deinitialize 
**               a unicast communication channel  
**
** Parameters  : -
**
** Returnvalue : -
**               
** Remarks     : function is not reentrant, does never block
**
*************************************************************************/
#define NIF_CloseUcConn() GOE_CloseUcConn()
#endif /* #if( k_UNICAST_CPBL == TRUE ) */

/*************************************************************************
**
** Function    : NIF_UNPACKHDR_MSGTYPE
**
** Description : Gets the message type out of a PTP V2 header
**
** Parameters  : pb_buf  (IN)  - pointer to buffer to fill
**
** Returnvalue : UINT8         - the message type
**               
** Remarks     : -
**
*************************************************************************/
#define NIF_UNPACKHDR_MSGTYPE(pb_buf) (PTP_t_msgTypeEnum)(0xF & ((const UINT8*)pb_buf)[k_OFFS_MSG_TYPE])

/*************************************************************************
**
** Function    : NIF_UNPACKHDR_VERS
**
** Description : Gets the version number out of a PTP V2 header
**
** Parameters  : pb_buf  (IN)  - pointer to buffer to fill
**
** Returnvalue : UINT8 - the version number 
**               
** Remarks     : -
**
*************************************************************************/
#define NIF_UNPACKHDR_VERS(pb_buf) ( 0xF & ((const UINT8*)pb_buf)[k_OFFS_VER_PTP])

/*************************************************************************
**
** Function    : NIF_UNPACKHDR_MLEN
**
** Description : Gets the message length out of a PTP V2 header
**
** Parameters  : pb_buf  (IN)  - pointer to buffer to fill
**
** Returnvalue : UINT16 - the message length
**               
** Remarks     : -
**
*************************************************************************/
#define NIF_UNPACKHDR_MLEN(pb_buf) ( GOE_ntoh16(&(((const UINT8*)pb_buf)[k_OFFS_MSG_LEN])))
  
/*************************************************************************
**
** Function    : NIF_UNPACKHDR_DOMNMB
**
** Description : Gets the domain number out of a PTP V2 header
**
** Parameters  : pb_buf  (IN)  - pointer to buffer to fill
**
** Returnvalue : UINT8         - the domain number
**               
** Remarks     : -
**
*************************************************************************/
#define NIF_UNPACKHDR_DOMNMB(pb_buf)  (((const UINT8*)pb_buf)[k_OFFS_DOM_NMB])

/*************************************************************************
**
** Function    : NIF_UNPACKHDR_FLAGS
**
** Description : Gets the flags out of a PTP V2 header
**
** Parameters  : pb_buf  (IN)  - pointer to buffer to fill
**
** Returnvalue : UINT16         - the flags
**               
** Remarks     : -
**
*************************************************************************/
#define NIF_UNPACKHDR_FLAGS(pb_buf)  (GOE_ntoh16(&((const UINT8*)pb_buf)[k_OFFS_FLAGS]))

/*************************************************************************
**
** Function    : NIF_UNPACKHDR_COR
**
** Description : Gets the correction field out of a PTP V2 header
**
** Parameters  : pb_buf  (IN)  - pointer to buffer to fill
**
** Returnvalue : INT64         - the correction field
**               
** Remarks     : -
**
*************************************************************************/
#define NIF_UNPACKHDR_COR(pb_buf)  ((INT64)GOE_ntoh64(&((const UINT8*)pb_buf)[k_OFFS_CORFIELD]))

/** C2DOC_start */
/*************************************************************************
**
** Function    : NIF_UnPackHdrPortId
**
** Description : Gets the port id out of a PTP V2 header
**
** Parameters  : ps_portId (OUT) - pointer to port id
**               pb_buf    (IN)  - pointer to network buffer
**
** Returnvalue : UINT16 - the sequence id
**               
** Remarks     : -
**
*************************************************************************/
void NIF_UnPackHdrPortId(PTP_t_PortId *ps_portId,const UINT8 *pb_buf);

/*************************************************************************
**
** Function    : NIF_UnPackTrgtPortId
**
** Description : Gets the target port id out of a PTP V2 Management
**               or Signaling message
**
** Parameters  : ps_trgtPId (OUT) - pointer to target port id
**               pb_buf     (IN)  - pointer to network buffer
**
** Returnvalue : UINT16 - the sequence id
**               
** Remarks     : -
**
*************************************************************************/
void NIF_UnPackTrgtPortId(PTP_t_PortId *ps_trgtPId,const UINT8 *pb_buf);

/*************************************************************************
**
** Function    : NIF_UnPackReqPortId
**
** Description : Gets the requesting port id out of a PTP V2 delay response
**               message
**
** Parameters  : ps_portId (OUT) - pointer to port id
**               pb_buf    (IN)  - pointer to network buffer
**
** Returnvalue : UINT16 - the sequence id
**               
** Remarks     : -
**
*************************************************************************/
void NIF_UnPackReqPortId(PTP_t_PortId *ps_portId,const UINT8 *pb_buf);

/** C2DOC_stop */

/*************************************************************************
**
** Function    : NIF_UNPACKHDR_SEQID
**
** Description : Gets the sequence Id out of a PTP V2 header
**
** Parameters  : pb_buf  (IN)  - pointer to buffer to fill
**
** Returnvalue : UINT16 - the sequence id
**               
** Remarks     : -
**
*************************************************************************/
#define NIF_UNPACKHDR_SEQID(pb_buf) (GOE_ntoh16(&((const UINT8*)pb_buf)[k_OFFS_SEQ_ID]))

/*************************************************************************
**
** Function    : NIF_UNPACKHDR_CTRL
**
** Description : Gets the control field out of a PTP V2 header
**
** Parameters  : pb_buf  (IN)  - pointer to buffer to fill
**
** Returnvalue : UINT8 - control field
**               
** Remarks     : -
**
*************************************************************************/
#define NIF_UNPACKHDR_CTRL(pb_buf) ((PTP_t_cfEnum)((const UINT8*)pb_buf)[k_OFFS_CTRL])

/*************************************************************************
**
** Function    : NIF_UNPACKHDR_MMINTV
**
** Description : Gets the mean message interval out of a PTP V2 header
**
** Parameters  : pb_buf  (IN)  - pointer to buffer to fill
**
** Returnvalue : INT8 - log mean message interval
**               
** Remarks     : -
**
*************************************************************************/
#define NIF_UNPACKHDR_MMINTV(pb_buf) ((INT8)((const UINT8*)pb_buf)[k_OFFS_LOGMEAN])

/*************************************************************************
**
** Function    : NIF_UNPACK_MNT_BNDRHOPS
**
** Description : Gets the boundary hops field out of a 
**               PTP V2 management message header
**
** Parameters  : pb_buf  (IN)  - pointer to buffer to fill
**
** Returnvalue : UINT8 - boundary hops field value
**               
** Remarks     : -
**
*************************************************************************/
#define NIF_UNPACK_MNT_BNDRHOPS(pb_buf) (pb_buf[k_OFFS_MNT_BHOPS]);

/** C2DOC_start */

/*************************************************************************
**
** Function    : NIF_UnPackHdr
**
** Description : Gets the data out of a network buffer header in the message 
**               header struct.
**
** Parameters  : ps_hdr  (OUT) - pointer to message header struct
**               pb_buf  (IN)  - pointer to network buffer
**
** Returnvalue : -
**               
** Remarks     : -
**
*************************************************************************/
void NIF_UnPackHdr(NIF_t_PTPV2_Head *ps_hdr,const UINT8 *pb_buf);

/*************************************************************************
**
** Function    : NIF_UnPackAnncMsg
**
** Description : Gets the data out of a network buffer in the announce  
**               message struct.
**
** Parameters  : ps_anncMsg (OUT) - pointer to announce message struct
**               pb_buf      (IN) - pointer to buffer 
**
** Returnvalue : -
**               
** Remarks     : -
**
*************************************************************************/
void NIF_UnPackAnncMsg(NIF_t_PTPV2_AnnMsg *ps_anncMsg,const UINT8 *pb_buf);

/*************************************************************************
**
** Function    : NIF_UnPackSyncMsg
**
** Description : Gets the data out of a network buffer in the sync message 
**               struct.
**
** Parameters  : ps_syncMsg (OUT) - pointer to sync message struct
**               pb_buf      (IN) - pointer to buffer 
**
** Returnvalue : -
**               
** Remarks     : -
**
*************************************************************************/
void NIF_UnPackSyncMsg(NIF_t_PTPV2_SyncMsg *ps_syncMsg,const UINT8 *pb_buf);

/** C2DOC_stop */

/*************************************************************************
**
** Function    : NIF_UnPackDelReqMsg
**
** Description : Gets the data out of a network buffer in the delay request  
**               message struct.
**
** Parameters  : ps_struct (OUT) - pointer to delay request message struct
**               pb_buf    (IN)  - pointer to buffer to fill
**
** Returnvalue : -
**               
** Remarks     : -
**
*************************************************************************/
#define NIF_UnPackDelReqMsg(ps_struct,pb_buf) NIF_UnPackSyncMsg(ps_struct,pb_buf)

/** C2DOC_start */

/*************************************************************************
**
** Function    : NIF_UnPackFlwUpMsg
**
** Description : Gets the data out of a network buffer in the follow up  
**               message struct.
**
** Parameters  : ps_flwUpMsg (OUT) - pointer to follow up message struct
**               pb_buf       (IN) - pointer to network buffer
**
** Returnvalue : -
**               
** Remarks     : -
**
*************************************************************************/
void NIF_UnPackFlwUpMsg(NIF_t_PTPV2_FlwUpMsg *ps_flwUpMsg,const UINT8 *pb_buf);

/*************************************************************************
**
** Function    : NIF_UnPackDelRespMsg
**
** Description : Gets the data out of a network buffer in the delay response  
**               message struct. 
**
** Parameters  : ps_delRspMsg (OUT) - pointer to delay response message struct
**               pb_buf        (IN) - pointer to network buffer
**
** Returnvalue : -
**               
** Remarks     : -
**
*************************************************************************/
void NIF_UnPackDelRespMsg(NIF_t_PTPV2_DlRspMsg *ps_delRspMsg,
                          const UINT8 *pb_buf);

/*************************************************************************
**
** Function    : NIF_UnPackMntMsg
**
** Description : Gets the data out of a network buffer in the management   
**               message struct. The management TLV has to be unpacked
**               by the function NIF_unpackMntTlv
**
**    See also : NIF_unpackMntTlv()
**
** Parameters  : ps_mntMsg (OUT) - pointer to management message struct
**               pb_buf     (IN) - pointer to buffer
**
** Returnvalue : -
**               
** Remarks     : -
**
*************************************************************************/
void NIF_UnPackMntMsg(NIF_t_PTPV2_MntMsg *ps_mntMsg,const UINT8 *pb_buf);

#if( k_CLK_DEL_P2P == TRUE )
/*************************************************************************
**
** Function    : NIF_UnpackPDlrq
**
** Description : Gets the data out of a network buffer in the 
**               peer delay request message struct. 
**
** Parameters  : ps_pDlrq (IN)  - pointer to peer delay request message struct
**               pb_buf   (OUT) - pointer to network buffer
**
** Returnvalue : -
**               
** Remarks     : -
**
*************************************************************************/
void NIF_UnpackPDlrq(NIF_t_PTPV2_PDelReq *ps_pDlrq,const UINT8 *pb_buf);

/*************************************************************************
**
** Function    : NIF_UnpackPDlrsp
**
** Description : Gets the data out of a network buffer in the 
**               peer delay response message struct. 
**
** Parameters  : ps_pDlrsp (IN) - pointer to peer delay response 
**                                message struct
**               pb_buf    OUT) - pointer to network buffer
**
** Returnvalue : -
**               
** Remarks     : -
**
*************************************************************************/
void NIF_UnpackPDlrsp(NIF_t_PTPV2_PDelResp *ps_pDlrsp,const UINT8 *pb_buf);

/*************************************************************************
**
** Function    : NIF_UnpackPDlrspFlw
**
** Description : Gets the data out of a network buffer in the 
**               peer delay response follou up message struct. 
**
** Parameters  : ps_pdlrspFlw (IN) - pointer to peer delay response 
**                                   follow up message struct
**               pb_buf      (OUT) - pointer to network buffer
**
** Returnvalue : -
**               
** Remarks     : -
**
*************************************************************************/
void NIF_UnpackPDlrspFlw(NIF_t_PTPV2_PDlRspFlw *ps_pdlrspFlw,
                         const UINT8 *pb_buf);

#endif /* #if( k_CLK_DEL_P2P == TRUE ) */

/*************************************************************************
**
** Function    : NIF_GetIfAddrs
**
** Description : This function gets the current addresses defined
**               by the interface index. Addresses are returned by 
**               setting the corresponding pointer of the parameter 
**               list to the buffer containing the address octets. 
**               The size determines the amount of octets to read 
**               and if necessary must be updated.
**
** See Also    : GOE_GetIfAddrs()
**
** Parameters  : pw_phyAddrLen  (OUT) - number of phyAddr octets
**               ppb_phyAddr    (OUT) - pointer to phyAddr
**               pw_portAddrLen (OUT) - number of portAdrr octets
**               ppb_portAddr   (OUT) - pointer to portAddr
**               w_ifIdx        (IN)  - interface index
**               
** Returnvalue : TRUE           - function succeeded
**               FALSE          - function failed
**
** Remarks     : -
**
*************************************************************************/
BOOLEAN NIF_GetIfAddrs(UINT16 *pw_phyAddrLen,
                       UINT8  **ppb_phyAddr,
                       UINT16 *pw_portAddrLen,
                       UINT8  **ppb_portAddr,
                       UINT16 w_ifIdx);
                       
#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* __NIF_H__ */


