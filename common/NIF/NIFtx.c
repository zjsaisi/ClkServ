/*************************************************************************
**    (C) 2008 - 2009 IXXAT Automation GmbH, all rights reserved
**************************************************************************
**
**       File: NIFtx.c 
**    Summary: Provides the rx interface to the network communication. 
**             This file must be adapted, when changing the 
**             communication technology.
**
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
**             NIF_Set_PTP_subdomain
**             NIF_InitMsgTempls
**
**             SendEvntMsg
**             InitAnncTemplate
**             InitSyncTemplate
**             InitFlwUpTemplate
**             InitDlrqTemplate
**             InitDlrspTemplate
**             InitMntTemplate
**             InitPdlrqTemplate
**             InitPdlrspTemplate
**             InitPdlrspFlwTemplate
**
**   Compiler: ANSI-C
**    Remarks:  
**             especially Annex D 
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
#include "GOE/GOE.h"
#include "NIF/NIF.h"
#include "NIF/NIFint.h"

/*************************************************************************
**    global variables
*************************************************************************/
/* Message templates for the important messages */
#if((k_CLK_IS_OC == TRUE)|| (k_CLK_IS_BC == TRUE ))
UINT8 NIF_aab_AnncMsg[k_NUM_IF][k_PTPV2_HDR_SZE + k_PTPV2_ANNC_PLD];
UINT8 NIF_aab_SyncMsg[k_NUM_IF][k_PTPV2_EVT_PAD_SZE];
UINT8 NIF_aab_DlrqMsg[k_NUM_IF][k_PTPV2_EVT_PAD_SZE];
UINT8 NIF_aab_DlrspMsg[k_NUM_IF][k_PTPV2_HDR_SZE + k_PTPV2_DLRSP_PLD];
UINT8 NIF_aab_FlwupMsg[k_NUM_IF][k_PTPV2_HDR_SZE + k_PTPV2_FLUP_PLD];
#endif /*#if((k_CLK_IS_OC == TRUE)|| (k_CLK_IS_BC == TRUE )) */
UINT8 NIF_aab_MntMsg[k_NUM_IF][k_PTPV2_HDR_SZE + k_MAX_PTP_PLD];

/* Peer to Peer delay request message types */
#if(k_CLK_DEL_P2P == TRUE)
UINT8 NIF_aab_P_DlrqMsg[k_NUM_IF][k_PTPV2_EVT_PAD_SZE];
UINT8 NIF_aab_P_DlrspMsg[k_NUM_IF][k_PTPV2_EVT_PAD_SZE];
#if( k_TWO_STEP == TRUE )
UINT8 NIF_aab_P_DlrspFlw[k_NUM_IF][k_PTPV2_HDR_SZE + k_PTPV2_P_DLRSP_FLW_PLD];
#endif /* #if( k_TWO_STEP == FALSE ) */
#endif /* #if(k_CLK_DEL_P2P == TRUE) */

/*************************************************************************
**    static constants, types, macros, variables
*************************************************************************/

/*************************************************************************
**    static function-prototypes
*************************************************************************/
static BOOLEAN SendEvntMsg( const PTP_t_PortAddr *ps_pAddr,
                            UINT8                *pb_buf,
                            UINT16               w_ifIdx,
                            UINT16               w_seqId, 
                            INT8                 c_Intv);
#if((k_CLK_IS_OC == TRUE)|| (k_CLK_IS_BC == TRUE ))
static void InitAnncTemplate( void );
static void InitSyncTemplate( void );
static void InitFlwUpTemplate( void );
static void InitDlrqTemplate( void );
static void InitDlrspTemplate( void );
#endif /* #if((k_CLK_IS_OC == TRUE)|| (k_CLK_IS_BC == TRUE )) */
static void InitMntTemplate( void );
#if( k_CLK_DEL_P2P == TRUE )
  static void InitPdlrqTemplate( void );
  static void InitPdlrspTemplate( void );
#if( k_TWO_STEP == TRUE )
  static void InitPdlrspFlwTemplate( void );
#endif /* #if( k_TWO_STEP == TRUE ) */
#endif /* #if( k_CLK_DEL_P2P == TRUE ) */

/*************************************************************************
**    global functions
*************************************************************************/

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
void NIF_PackHdrFlag(UINT8 *pb_buf,UINT8 b_pos,BOOLEAN o_val)  
{
  UINT16 w_flags;
  /* get the actual flags */
  w_flags = GOE_ntoh16(&pb_buf[k_OFFS_FLAGS]);
  /* set the new flag */
  SET_FLAG(w_flags,b_pos,o_val);
  /* rewrite flags */
  GOE_hton16(&pb_buf[k_OFFS_FLAGS],w_flags);
}

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
void NIF_PackHdr(UINT8 *pb_buf,const NIF_t_PTPV2_Head *ps_hdr)
{
  NIF_PACKHDR_MTYPE(pb_buf,ps_hdr->e_msgType);
  pb_buf[k_OFFS_VER_PTP]             = (0xF & ps_hdr->b_versPTP);
  GOE_hton16(&pb_buf[k_OFFS_MSG_LEN],ps_hdr->w_msgLen);
  NIF_PACKHDR_DOMNMB(pb_buf,ps_hdr->b_dmnNmb);
  pb_buf[5]                          = 0;  /* reserved */
  GOE_hton16(&pb_buf[k_OFFS_FLAGS],ps_hdr->w_flags);
  NIF_PACKHDR_CORR(pb_buf,&ps_hdr->ll_corField);
  PTP_MEMSET(&pb_buf[16],'\0',4); /* reserved */
  PTP_BCOPY(&pb_buf[k_OFFS_SRCPRTID],
            ps_hdr->s_srcPortId.s_clkId.ab_id,
            k_CLKID_LEN);
  GOE_hton16(&pb_buf[k_OFFS_SRCPRTID+k_CLKID_LEN],
             ps_hdr->s_srcPortId.w_portNmb);
  GOE_hton16(&pb_buf[k_OFFS_SEQ_ID],ps_hdr->w_seqId);
  NIF_PACKHDR_CTRL(pb_buf,ps_hdr->e_ctrl);
  pb_buf[k_OFFS_LOGMEAN]             = (UINT8)ps_hdr->c_logMeanMsgIntv;
}

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
void NIF_PackAnncMsg(UINT8 *pb_buf,const NIF_t_PTPV2_AnnMsg *ps_anncMsg)
{
  /* pack the header  */
  NIF_PackHdr(pb_buf,&ps_anncMsg->s_ptpHead);
  /* pack origin timestamp */
  GOE_hton48(&pb_buf[k_OFFS_TS_ANNC_SEC],&ps_anncMsg->s_origTs.u48_sec);
  GOE_hton32(&pb_buf[k_OFFS_TS_ANNC_NSEC],ps_anncMsg->s_origTs.dw_Nsec);
  /* pack current UTC offset */
  GOE_hton16(&pb_buf[k_OFFS_CURUTCOFFS],(UINT16)ps_anncMsg->i_curUTCOffs);
  /* reserved space */
  pb_buf[k_OFFS_CURUTCOFFS+2] = 0;
  /* pack time source */
  pb_buf[k_OFFS_TMESRC] = (UINT8)ps_anncMsg->e_timeSrc;
  /* pack steps removed */
  GOE_hton16(&pb_buf[k_OFFS_STEPS_REM],ps_anncMsg->w_stepsRemvd);
  /* pack grandmaster Identity */
  PTP_BCOPY(&pb_buf[k_OFFS_GM_ID],ps_anncMsg->s_grdMstId.ab_id,k_CLKID_LEN);
  /* pack grandmaster clock quality */
  pb_buf[k_OFFS_GM_CLKQUAL]   = (UINT8)ps_anncMsg->s_grdMstClkQual.e_clkClass;
  pb_buf[k_OFFS_GM_CLKQUAL+1] = (UINT8)ps_anncMsg->s_grdMstClkQual.e_clkAccur;
  GOE_hton16(&pb_buf[k_OFFS_GM_CLKQUAL+2],
                  ps_anncMsg->s_grdMstClkQual.w_scldVar);
  /* pack grandmaster priority 1 */
  pb_buf[k_OFFS_GM_PRIO1] = ps_anncMsg->b_grdMstPrio1;
  /* pack grandmaster priority 1 */
  pb_buf[k_OFFS_GM_PRIO2] = ps_anncMsg->b_grdMstPrio2;
}

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
void NIF_PackSyncMsg(UINT8 *pb_buf,const NIF_t_PTPV2_SyncMsg *ps_syncMsg)
{
  /* pack the header */
  NIF_PackHdr(pb_buf,&ps_syncMsg->s_ptpHead);
  /* pack timestamp seconds */
  GOE_hton48(&pb_buf[k_OFFS_TS_SYN_SEC],&ps_syncMsg->s_origTs.u48_sec);
  /* pack timestamp nanoseconds */
  GOE_hton32(&pb_buf[k_OFFS_TS_SYN_NSEC],ps_syncMsg->s_origTs.dw_Nsec);
}

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
/* defined in header */

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
void NIF_PackFlwUpMsg(UINT8 *pb_buf,const NIF_t_PTPV2_FlwUpMsg *ps_flwUpMsg)
{
  /* pack the header */
  NIF_PackHdr(pb_buf,&ps_flwUpMsg->s_ptpHead);
  /* pack timestamp seconds */
  GOE_hton48(&pb_buf[k_OFFS_PRECTS_SEC],&ps_flwUpMsg->s_precOrigTs.u48_sec);
  /* pack timestamp nanoseconds */
  GOE_hton32(&pb_buf[k_OFFS_PRECTS_NSEC],ps_flwUpMsg->s_precOrigTs.dw_Nsec);
}

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
void NIF_PackDelRespMsg(UINT8 *pb_buf,const NIF_t_PTPV2_DlRspMsg *ps_delRspMsg)
{
  /* pack the header */
  NIF_PackHdr(pb_buf,&ps_delRspMsg->s_ptpHead);
  /* pack timestamp seconds */
  GOE_hton48(&pb_buf[k_OFFS_RECVTS_SEC],&ps_delRspMsg->s_recvTs.u48_sec);
  /* pack timestamp nanoseconds */
  GOE_hton32(&pb_buf[k_OFFS_RECVTS_NSEC],ps_delRspMsg->s_recvTs.dw_Nsec);
  /* pack requesting port identity */
  PTP_BCOPY(&pb_buf[k_OFFS_REQSRCPRTID],
            ps_delRspMsg->s_reqPortId.s_clkId.ab_id,
            k_CLKID_LEN);
  GOE_hton16(&pb_buf[k_OFFS_REQSRCPRTID+k_CLKID_LEN],
             ps_delRspMsg->s_reqPortId.w_portNmb);
}


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
void NIF_PackPDlrq(UINT8 *pb_buf,const NIF_t_PTPV2_PDelReq *ps_pDlrq)
{
  /* pack the header */
  NIF_PackHdr(pb_buf,&ps_pDlrq->s_ptpHead);
  /* pack timestamp seconds */
  GOE_hton48(&pb_buf[k_OFFS_P2P_ORIGTS_SEC],&ps_pDlrq->s_origTs.u48_sec);
  /* pack timestamp nanoseconds */
  GOE_hton32(&pb_buf[k_OFFS_P2P_ORIGTS_NSEC],ps_pDlrq->s_origTs.dw_Nsec);
}

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
void NIF_PackPDlrsp(UINT8 *pb_buf,const NIF_t_PTPV2_PDelResp *ps_pDlrsp)
{
  /* pack the header */
  NIF_PackHdr(pb_buf,&ps_pDlrsp->s_ptpHead);
  /* pack requesting receipt timestamp seconds */
  GOE_hton48(&pb_buf[k_OFFS_P2P_ORIGTS_SEC],&ps_pDlrsp->s_reqRecptTs.u48_sec);
  /* pack requesting receipt timestamp nanoseconds */
  GOE_hton32(&pb_buf[k_OFFS_P2P_ORIGTS_NSEC],ps_pDlrsp->s_reqRecptTs.dw_Nsec);
  /* pack requesting port id */
  PTP_BCOPY(&pb_buf[k_OFFS_P2P_REQPRTID],
            ps_pDlrsp->s_reqPId.s_clkId.ab_id,
            k_CLKID_LEN);
  GOE_hton16(&pb_buf[k_OFFS_P2P_REQPRTID+k_CLKID_LEN],
             ps_pDlrsp->s_reqPId.w_portNmb);
}

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
void NIF_PackPDlrspFlw(UINT8 *pb_buf,const NIF_t_PTPV2_PDlRspFlw *ps_pDlrspFlw)
{
  /* pack the header */
  NIF_PackHdr(pb_buf,&ps_pDlrspFlw->s_ptpHead);
  /* pack requesting receipt timestamp seconds */
  GOE_hton48(&pb_buf[k_OFFS_P2P_ORIGTS_SEC],&ps_pDlrspFlw->s_respTs.u48_sec);
  /* pack requesting receipt timestamp nanoseconds */
  GOE_hton32(&pb_buf[k_OFFS_P2P_ORIGTS_NSEC],ps_pDlrspFlw->s_respTs.dw_Nsec);
  /* pack requesting port id */
  PTP_BCOPY(&pb_buf[k_OFFS_P2P_REQPRTID],
            ps_pDlrspFlw->s_reqPId.s_clkId.ab_id,
            k_CLKID_LEN);
  GOE_hton16(&pb_buf[k_OFFS_P2P_REQPRTID+k_CLKID_LEN],
             ps_pDlrspFlw->s_reqPId.w_portNmb);
}
#endif /* #if( k_CLK_DEL_P2P == TRUE ) */

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
void NIF_PackTlv(UINT8* pb_buf,PTP_t_tlvTypeEnum e_type,UINT16 w_len)
{
  /* copy tlvType into buffer */
  GOE_hton16(pb_buf,(UINT16)e_type);
  /* copy lengthfield into buffer */
  GOE_hton16(&pb_buf[2],w_len);
}
#if((k_CLK_IS_OC == TRUE)|| (k_CLK_IS_BC == TRUE ))
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
                      PTP_t_ClkDs const    *ps_clkDs)
{
  UINT8   *pb_buf = NIF_aab_AnncMsg[w_ifIdx];
  /* 
  - message type is set 
  - version PTP is set
  - message length is set
  - domain number is set
  */
  /* set changing flags */
#if(k_ALT_MASTER == TRUE )
#error ALTERNATE_MASTER flag must be implemented
#endif
  if( ps_pAddr == NULL )
  {
    NIF_PackHdrFlag(pb_buf,k_FLG_UNICAST,FALSE);
  }
  else
  {
    NIF_PackHdrFlag(pb_buf,k_FLG_UNICAST,TRUE);
  }
  NIF_PackHdrFlag(pb_buf,
                  k_FLG_LI_61,
                  ps_clkDs->ps_rdTmPrptDs->o_leap_61);
  NIF_PackHdrFlag(pb_buf,
                  k_FLG_LI_59,
                  ps_clkDs->ps_rdTmPrptDs->o_leap_59);
  NIF_PackHdrFlag(pb_buf,
                  k_FLG_CUR_UTCOFFS_VAL,
                  ps_clkDs->ps_rdTmPrptDs->o_curUtcOffsVal);
  NIF_PackHdrFlag(pb_buf,
                  k_FLG_PTP_TIMESCALE,
                  ps_clkDs->ps_rdTmPrptDs->o_ptpTmScale);
  NIF_PackHdrFlag(pb_buf,
                  k_FLG_TIME_TRACEABLE,
                  ps_clkDs->ps_rdTmPrptDs->o_tmTrcable);
  NIF_PackHdrFlag(pb_buf,
                  k_FLG_FREQ_TRACEABLE,
                  ps_clkDs->ps_rdTmPrptDs->o_frqTrcable);

  /*
  - correction field is set
  - port id is already set
  */
  GOE_hton16(&pb_buf[k_OFFS_SEQ_ID],w_seqId);
  /* control is already set */
  pb_buf[k_OFFS_LOGMEAN] = (UINT8)ps_clkDs->as_PortDs[w_ifIdx].c_AnncIntv;
  /* 
  - origin timestamp is always zero 
  */
  /* pack current UTC offset */
  GOE_hton16(&pb_buf[k_OFFS_CURUTCOFFS],
             (UINT16)ps_clkDs->ps_rdTmPrptDs->i_curUtcOffs);
  /* pack time source */
  pb_buf[k_OFFS_TMESRC] = (UINT8)ps_clkDs->ps_rdTmPrptDs->e_tmSrc;
  /* pack steps removed */
  GOE_hton16(&pb_buf[k_OFFS_STEPS_REM],
                  ps_clkDs->s_CurDs.w_stepsRmvd);
  /* pack grandmaster Identity */
  PTP_BCOPY(&pb_buf[k_OFFS_GM_ID],
            ps_clkDs->s_PrntDs.s_gmClkId.ab_id,k_CLKID_LEN);
  /* pack grandmaster clock quality */
  pb_buf[k_OFFS_GM_CLKQUAL] = (UINT8)ps_clkDs->s_PrntDs.s_gmClkQual.e_clkClass;
  pb_buf[k_OFFS_GM_CLKQUAL+1]=(UINT8)ps_clkDs->s_PrntDs.s_gmClkQual.e_clkAccur;
  GOE_hton16(&pb_buf[k_OFFS_GM_CLKQUAL+2],
             ps_clkDs->s_PrntDs.s_gmClkQual.w_scldVar);
  /* pack grandmaster priority 1 */
  pb_buf[k_OFFS_GM_PRIO1] = ps_clkDs->s_PrntDs.b_gmPrio1;
  /* pack grandmaster priority 1 */
  pb_buf[k_OFFS_GM_PRIO2] = ps_clkDs->s_PrntDs.b_gmPrio2;
  /* check, if padding period is over */
  if( SIS_TIME_OVER(as_padding[w_ifIdx].dw_tick) )
  {
    as_padding[w_ifIdx].o_pad = FALSE;
  }
  /* send packet */
  return GOE_SendMsg(ps_pAddr,
                     pb_buf,
                     k_SOCK_GNRL,
                     w_ifIdx,
                     (k_PTPV2_HDR_SZE + k_PTPV2_ANNC_PLD));
}

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
                      INT8                 c_synIntv)
{
  /* call central function */
  return SendEvntMsg(ps_pAddr,
                     NIF_aab_SyncMsg[w_ifIdx],
                     w_ifIdx,
                     w_seqId,
                     c_synIntv);
}

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
                         INT8                 c_synIntv)
{
  UINT8*  pb_buf = NIF_aab_FlwupMsg[w_ifIdx];
  /* 
  - message type is set 
  - version PTP is set
  - message length is set
  - domain number is set
  */
  /* set changing flags */
#if(k_ALT_MASTER == TRUE )
#error ALTERNATE_MASTER flag must be implemented
#endif
  if( ps_pAddr == NULL )
  {
    NIF_PackHdrFlag(pb_buf,k_FLG_UNICAST,FALSE);
  }
  else
  {
    NIF_PackHdrFlag(pb_buf,k_FLG_UNICAST,TRUE);
  }
  GOE_hton64(&pb_buf[k_OFFS_CORFIELD],(const UINT64*)&ps_tiCorr->ll_scld_Nsec);
  /*
  - port id is already set
  */
  GOE_hton16(&pb_buf[k_OFFS_SEQ_ID],w_assSeqId);
  /* control is already set */
  pb_buf[k_OFFS_LOGMEAN] = (UINT8)c_synIntv;
  /* pack precise origin sync timestamp seconds */
  GOE_hton48(&pb_buf[k_OFFS_PRECTS_SEC],&ps_precTs->u48_sec);
  /* pack precise origin sync timestamp nanoseconds */
  GOE_hton32(&pb_buf[k_OFFS_PRECTS_NSEC],ps_precTs->dw_Nsec);
  /* send message */
  return GOE_SendMsg(ps_pAddr,pb_buf,k_SOCK_GNRL,w_ifIdx,
                     (k_PTPV2_HDR_SZE + k_PTPV2_FLUP_PLD));
}

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
                          UINT16       w_seqId)
{
  /* call central function */
  return SendEvntMsg(ps_pAddr,
                     NIF_aab_DlrqMsg[w_ifIdx],
                     w_ifIdx,
                     w_seqId,
                     0x7F);  
}

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
                          const PTP_t_TmStmp   *ps_recvTs)
{
  UINT8   *pb_buf = NIF_aab_DlrspMsg[w_ifIdx];
  /* 
  - message type is set 
  - version PTP is set
  - message length is set
  - domain number is set
  */
  /* set changing flags */
#if(k_ALT_MASTER == TRUE )
#error ALTERNATE_MASTER flag must be implemented
#endif
  if( ps_pAddr == NULL )
  {
    NIF_PackHdrFlag(pb_buf,k_FLG_UNICAST,FALSE);
  }
  else
  {
    NIF_PackHdrFlag(pb_buf,k_FLG_UNICAST,TRUE);
  }
  GOE_hton64(&pb_buf[k_OFFS_CORFIELD],(const UINT64*)&ps_tiCorr->ll_scld_Nsec);
  /*
  - port id is already set
  */
  GOE_hton16(&pb_buf[k_OFFS_SEQ_ID],w_reqSeqId);
  /* control is already set */
  pb_buf[k_OFFS_LOGMEAN] = (UINT8)c_dlrqIntv;
  /* pack receive timestamp seconds */
  GOE_hton48(&pb_buf[k_OFFS_RECVTS_SEC],&ps_recvTs->u48_sec);
  /* pack receive timestamp nanoseconds */
  GOE_hton32(&pb_buf[k_OFFS_RECVTS_NSEC],ps_recvTs->dw_Nsec);
  /* pack requesting port identity */
  PTP_BCOPY(&pb_buf[k_OFFS_REQSRCPRTID],
            ps_reqPortId->s_clkId.ab_id,
            k_CLKID_LEN);
  GOE_hton16(&pb_buf[k_OFFS_REQSRCPRTID+k_CLKID_LEN],
             ps_reqPortId->w_portNmb);
  /* send message */
  return GOE_SendMsg(ps_pAddr,pb_buf,k_SOCK_GNRL,
                     w_ifIdx,(k_PTPV2_HDR_SZE+k_PTPV2_DLRSP_PLD));
}
#endif /* #if((k_CLK_IS_OC == TRUE)|| (k_CLK_IS_BC == TRUE )) */
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
                        UINT8                b_actionFld)
{
  UINT16  w_totLen;
  UINT8   *pb_buf = NIF_aab_MntMsg[w_ifIdx];
    
  /********************************************
  ** message header                          **
  ********************************************/

  /* 
  - message type is set 
  - version PTP is set
  */

  /* calculate total message length - 4 extra bytes for TLV without payload */
  w_totLen = k_PTPV2_HDR_SZE + k_PTPV2_MNTHDR_SZE + 4 + ps_mntTlv->w_len;
  GOE_hton16(&pb_buf[k_OFFS_MSG_LEN],w_totLen);
  /*
  - domain number is set */
  /* set changing flags */
  if( ps_pAddr == NULL )
  {
    NIF_PackHdrFlag(pb_buf,k_FLG_UNICAST,FALSE);
  }
  else
  {
    NIF_PackHdrFlag(pb_buf,k_FLG_UNICAST,TRUE);
  }
  /*
  - correction field is already set
  - source port id is already set
  */  
  GOE_hton16(&pb_buf[k_OFFS_SEQ_ID],w_seqId);
  /*
  - control field is already set
  - message interval field is already set
  */   
  /********************************************
  ** message body                            **
  ********************************************/
  PTP_BCOPY(&pb_buf[k_OFFS_MNT_TRGTPID],ps_dstPid->s_clkId.ab_id,k_CLKID_LEN);
  GOE_hton16(&pb_buf[k_OFFS_MNT_TRGTPID+k_CLKID_LEN],ps_dstPid->w_portNmb);
  pb_buf[k_OFFS_MNT_STRT_BHOPS] = b_strtBndrHops;
  pb_buf[k_OFFS_MNT_BHOPS]      = b_bndrHops;
  pb_buf[k_OFFS_MNT_ACTION]     = b_actionFld;

  /* set TLV parameters */
  GOE_hton16(&pb_buf[k_OFFS_MNT_TLV_TYPE],ps_mntTlv->w_type);
  GOE_hton16(&pb_buf[k_OFFS_MNT_TLV_LEN],ps_mntTlv->w_len);
  GOE_hton16(&pb_buf[k_OFFS_MNT_TLV_ID],ps_mntTlv->w_mntId);
  
  /* if TLV payload available */
  if(ps_mntTlv->w_len > 2)
  {
    PTP_BCOPY(&pb_buf[k_OFFS_MNT_TLV_DATA],
              ps_mntTlv->pb_data,
              ps_mntTlv->w_len); 
  }
  /* send message */
  return GOE_SendMsg(ps_pAddr,pb_buf,k_SOCK_GNRL,w_ifIdx,w_totLen);
}

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
                      UINT8                b_domain)
{
  UINT16  w_totLen;
  NIF_t_PTPV2_SignMsg s_signMsg;
  UINT8   ab_buf[k_PTPV2_HDR_SZE + k_MAX_PTP_PLD];

  /********************************************
  ** message header                          **
  ********************************************/
  s_signMsg.s_ptpHead.e_msgType = e_MT_SGNLNG;
  s_signMsg.s_ptpHead.b_versPTP = k_PTP_VERSION;
  w_totLen                      = k_PTPV2_HDR_SZE + k_PTPV2_SIGNHDR_SZE +
                                  w_dataLen;
  s_signMsg.s_ptpHead.w_msgLen  = w_totLen;
  s_signMsg.s_ptpHead.b_dmnNmb  = b_domain;
  /* preset flags to zero (=FALSE) */
  s_signMsg.s_ptpHead.w_flags   = 0;  
  /* set changing flags */
  if( ps_pAddr == NULL )
  {
    SET_FLAG(s_signMsg.s_ptpHead.w_flags ,k_FLG_UNICAST ,FALSE);
  }
  else
  {
    SET_FLAG(s_signMsg.s_ptpHead.w_flags ,k_FLG_UNICAST ,TRUE);
  }
  s_signMsg.s_ptpHead.ll_corField      = 0;
  s_signMsg.s_ptpHead.s_srcPortId      = *ps_srcPid;
  s_signMsg.s_ptpHead.w_seqId          = w_seqId;
  s_signMsg.s_ptpHead.e_ctrl           = e_CTLF_ALL_OTHERS;
  s_signMsg.s_ptpHead.c_logMeanMsgIntv = 0x7F;
  /********************************************
  ** pack header in network buffer           **
  ********************************************/
  NIF_PackHdr(ab_buf,&s_signMsg.s_ptpHead);
  /********************************************
  ** message body                            **
  ********************************************/
  PTP_BCOPY(&ab_buf[k_OFFS_TRGT_ID],ps_dstPid->s_clkId.ab_id,k_CLKID_LEN);
  GOE_hton16(&ab_buf[k_OFFS_TRGT_ID + k_CLKID_LEN],ps_dstPid->w_portNmb);
  /********************************************
  ** tlv array                               **
  ********************************************/
  PTP_BCOPY(&ab_buf[k_OFFS_TLVS],pb_tlv,w_dataLen);
  /* send message */
  return GOE_SendMsg(ps_pAddr,ab_buf,k_SOCK_GNRL,w_ifIdx,w_totLen);
}

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
                        UINT16             w_ifIdx)
{
  UINT8 ab_flwUpMsg[k_PTPV2_HDR_SZE + k_PTPV2_FLUP_PLD];
  
  /* copy sync message into buffer */
  PTP_BCOPY(ab_flwUpMsg,pb_syncMsg,k_PTPV2_HDR_SZE + k_PTPV2_FLUP_PLD);
  /* change message type */
  NIF_PACKHDR_MTYPE(ab_flwUpMsg,e_MT_FLWUP);

  /*   
  - version PTP is set
  - message length is set (identically)
  - domain number is set
  */
  /* change flags to TWO-STEP */
  NIF_PackHdrFlag(ab_flwUpMsg,k_FLG_TWO_STEP,FALSE); 
  /* set residence time and path delay in correction field */
  NIF_PACKHDR_CORR(ab_flwUpMsg,&ps_tiCor->ll_scld_Nsec);
  /*
  - port id is already set
  - sequence id is already set
  */
  /* change control field */
  NIF_PACKHDR_CTRL(ab_flwUpMsg,e_CTLF_FOLLOWUP);
  /* precise origin sync timestamp is already set */  
  return GOE_SendMsg(NULL,ab_flwUpMsg,k_SOCK_GNRL,w_ifIdx,
                     (k_PTPV2_HDR_SZE + k_PTPV2_FLUP_PLD));
}

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
                                UINT16             w_ifIdx)
{ 
  UINT8 ab_pDelRspFlwUp[k_PTPV2_HDR_SZE + k_PTPV2_P_DLRSP_FLW_PLD];
  UINT64 ddw_zero = 0LL;
  
  /* copy sync message into buffer */
  PTP_BCOPY(ab_pDelRspFlwUp,
            pb_pDelRsp,
            k_PTPV2_HDR_SZE + k_PTPV2_P_DLRSP_FLW_PLD);
  /* change message type */
  NIF_PACKHDR_MTYPE(ab_pDelRspFlwUp,e_MT_PDELRES_FLWUP);

  /*   
  - version PTP is set
  - message length is set (identically)
  - domain number is set
  */
  /* change flags to TWO-STEP */
  NIF_PackHdrFlag(ab_pDelRspFlwUp,k_FLG_TWO_STEP,FALSE); 
  /* set residence time and path delay in correction field */
  NIF_PACKHDR_CORR(ab_pDelRspFlwUp,&ps_tiCor->ll_scld_Nsec);
  /*
  - port id is already set
  - sequence id is already set
  - control field is set
  */
  /* set the response origin timestamp to zero */  
  GOE_hton48(&ab_pDelRspFlwUp[k_OFFS_PRECTS_SEC],&ddw_zero);
  /* pack precise origin sync timestamp nanoseconds */
  GOE_hton32(&ab_pDelRspFlwUp[k_OFFS_PRECTS_NSEC],0);
  return GOE_SendMsg(NULL,ab_pDelRspFlwUp,k_SOCK_GNRL_PDEL,w_ifIdx,
                     (k_PTPV2_HDR_SZE + k_PTPV2_P_DLRSP_FLW_PLD));
}

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
                        UINT16       w_len)
{
  /* send message */
  return GOE_SendMsg(NULL,pb_buf,b_socket,w_ifIdx,w_len);
}


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
                       UINT16               w_seqId)
{
  UINT8  *pb_buf = NIF_aab_P_DlrqMsg[w_ifIdx];
  UINT16 w_len;
  
  /* 
  - message type is set 
  - version PTP is set
  - message length is set
  - domain number is set */
  /* set changing flags */
  if( ps_pAddr == NULL )
  {
    NIF_PackHdrFlag(pb_buf,k_FLG_UNICAST,FALSE);
  }
  else
  {
    NIF_PackHdrFlag(pb_buf,k_FLG_UNICAST,TRUE);
  }
  /*
  - correction field is set
  - port id is already set
  */
  GOE_hton16(&pb_buf[k_OFFS_SEQ_ID],w_seqId);
  /* 
  - control is already set 
  - log mean message interval is already set 
  - origin timestamp is always zero 
  */
  /* padding or not ? */
  if( as_padding[w_ifIdx].o_pad == TRUE )
  {
    w_len = k_PTPV2_EVT_PAD_SZE;
  }
  else
  {
    w_len = k_PTPV2_HDR_SZE + k_PTPV2_P_DLRQ_PLD;
  }
  /* send packet */
  return GOE_SendMsg(ps_pAddr,pb_buf,k_SOCK_EVT_PDEL,w_ifIdx,w_len);
}

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
                        const PTP_t_TmStmp   *ps_rxTs)
{
  
  UINT8  *pb_buf = NIF_aab_P_DlrspMsg[w_ifIdx];
  UINT16 w_len;
#if( k_TWO_STEP == TRUE )
  PTP_t_TmStmp s_pdrqRcvTs = {(UINT64)0,0};
#endif  
  /* 
  - message type is set 
  - version PTP is set
  - message length is set
  - domain number is set */
  /* set changing flags */
  if( ps_pAddr == NULL )
  {
    NIF_PackHdrFlag(pb_buf,k_FLG_UNICAST,FALSE);
  }
  else
  {
    NIF_PackHdrFlag(pb_buf,k_FLG_UNICAST,TRUE);
  }
  /* correction is already set to zero */
  /*
  - port id is already set
  */
  GOE_hton16(&pb_buf[k_OFFS_SEQ_ID],w_seqId);
  /* 
  - control is already set 
  - log mean message interval is already set 
  */
  /* request receipt timestamp is needed in HW by one-step clocks.
    The one-step-HW sets the timestamp to zero, computes the turnaround time
    and adds it to the correction field */
  /* Attention !! - no define here to be able to use one-step-hook */
#if((k_CLK_IS_OC == TRUE)||(k_CLK_IS_BC == TRUE))
  if( NIF_ps_ClkDs->s_DefDs.o_twoStep == FALSE)
  {
    GOE_hton48(&pb_buf[k_OFFS_P2P_RSP_TS_SEC],&ps_rxTs->u48_sec);
    GOE_hton32(&pb_buf[k_OFFS_P2P_RSP_TS_NSEC],ps_rxTs->dw_Nsec);
  }
  else
  {
    /*  two-step clocks set it to zero */
    GOE_hton48(&pb_buf[k_OFFS_P2P_RSP_TS_SEC],&s_pdrqRcvTs.u48_sec);
    GOE_hton32(&pb_buf[k_OFFS_P2P_RSP_TS_NSEC],s_pdrqRcvTs.dw_Nsec);
  }
#else
#if( k_TWO_STEP == FALSE )
  GOE_hton48(&pb_buf[k_OFFS_P2P_RSP_TS_SEC],&ps_rxTs->u48_sec);
  GOE_hton32(&pb_buf[k_OFFS_P2P_RSP_TS_NSEC],ps_rxTs->dw_Nsec);
#else
  /*  two-step clocks set it to zero */
  GOE_hton48(&pb_buf[k_OFFS_P2P_RSP_TS_SEC],&s_pdrqRcvTs.u48_sec);
  GOE_hton32(&pb_buf[k_OFFS_P2P_RSP_TS_NSEC],s_pdrqRcvTs.dw_Nsec);
#endif
#endif
  /* set requesting port id */
  PTP_BCOPY(&pb_buf[k_OFFS_P2P_REQPRTID],&ps_reqPId->s_clkId,k_CLKID_LEN);
  GOE_hton16(&pb_buf[k_OFFS_P2P_REQPRTID+k_CLKID_LEN],ps_reqPId->w_portNmb); 
  /* padding or not ? */
  if( as_padding[w_ifIdx].o_pad == TRUE )
  {
    w_len = k_PTPV2_EVT_PAD_SZE;
  }
  else
  {
    w_len = k_PTPV2_HDR_SZE + k_PTPV2_P_DLRQ_PLD;
  }
  /* send packet */
  return GOE_SendMsg(ps_pAddr,pb_buf,k_SOCK_EVT_PDEL, w_ifIdx,w_len);
} 

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
                           const INT64          *pll_corr)
{
  UINT8   *pb_buf = NIF_aab_P_DlrspFlw[w_ifIdx];
  
  /* 
  - message type is set 
  - version PTP is set
  - message length is set
  - domain number is set */
  /* set changing flags */
  if( ps_pAddr == NULL )
  {
    NIF_PackHdrFlag(pb_buf,k_FLG_UNICAST,FALSE);
  }
  else
  {
    NIF_PackHdrFlag(pb_buf,k_FLG_UNICAST,TRUE);
  }
  /* set correction */
  GOE_hton64(&pb_buf[k_OFFS_CORFIELD],(const UINT64*)pll_corr);
  /*
  - port id is already set
  */
  GOE_hton16(&pb_buf[k_OFFS_SEQ_ID],w_seqId);
  /* 
  - control is already set 
  - log mean message interval is already set 
  - response origin timestamp is already set
  */
  /* set requesting port id */
  PTP_BCOPY(&pb_buf[k_OFFS_P2P_REQPRTID],&ps_reqPId->s_clkId,k_CLKID_LEN);
  GOE_hton16(&pb_buf[k_OFFS_P2P_REQPRTID+k_CLKID_LEN],ps_reqPId->w_portNmb);  
  /* send packet */
  return GOE_SendMsg(ps_pAddr,
                     pb_buf,
                     k_SOCK_GNRL_PDEL, 
                     w_ifIdx,
                     (k_PTPV2_HDR_SZE + k_PTPV2_P_DLRSP_FLW_PLD));
}

#endif /* #if( k_CLK_DEL_P2P == TRUE ) */

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
void NIF_Set_PTP_subdomain( UINT8 b_domain )
{
  UINT16 w_ifIdx;

  /* set domain in all message templates */
  for( w_ifIdx = 0 ; w_ifIdx < k_NUM_IF ; w_ifIdx++ )
  {
#if((k_CLK_IS_OC == TRUE)|| (k_CLK_IS_BC == TRUE ))
    NIF_PACKHDR_DOMNMB(NIF_aab_AnncMsg[w_ifIdx],b_domain);
    NIF_PACKHDR_DOMNMB(NIF_aab_SyncMsg[w_ifIdx],b_domain);
    NIF_PACKHDR_DOMNMB(NIF_aab_DlrqMsg[w_ifIdx],b_domain);
    NIF_PACKHDR_DOMNMB(NIF_aab_FlwupMsg[w_ifIdx],b_domain);
    NIF_PACKHDR_DOMNMB(NIF_aab_DlrspMsg[w_ifIdx],b_domain);
#endif /* #if((k_CLK_IS_OC == TRUE)|| (k_CLK_IS_BC == TRUE )) */
    NIF_PACKHDR_DOMNMB(NIF_aab_MntMsg[w_ifIdx],b_domain);
/* just for P2P devices */
#if( k_CLK_DEL_P2P == TRUE )
    NIF_PACKHDR_DOMNMB(NIF_aab_P_DlrqMsg[w_ifIdx],b_domain);
    NIF_PACKHDR_DOMNMB(NIF_aab_P_DlrspFlw[w_ifIdx],b_domain);
    NIF_PACKHDR_DOMNMB(NIF_aab_P_DlrspFlw[w_ifIdx],b_domain);
#endif /* #if( k_CLK_DEL_P2P == TRUE ) */
  }
}

/***********************************************************************
**
** Function    : NIF_InitMsgTempls
**
** Description : Preinitializes the message templates 
**               with the non-volatile values. The message template for
**               sync messages is also used for delay_req messages.
**
** See Also    : NIF_SendSync(),NIF_SendDelayReq(),SendEvntMsg
**              
** Parameters  : -
**
** Returnvalue : -                                           
**                      
** Remarks     : function is not reentrant
**
***********************************************************************/
void NIF_InitMsgTempls(void)
{
#if((k_CLK_IS_OC == TRUE)|| (k_CLK_IS_BC == TRUE ))
  /* announce messages */
  InitAnncTemplate();
  /* sync messages */
  InitSyncTemplate();
  /* follow up messages */
  InitFlwUpTemplate();
  /* delay request messages */
  InitDlrqTemplate();  
  /* delay response message */
  InitDlrspTemplate();
#endif /* #if((k_CLK_IS_OC == TRUE)|| (k_CLK_IS_BC == TRUE )) */
  /* management messages */
  InitMntTemplate( );
#if( k_CLK_DEL_P2P == TRUE )
  /* peer delay request */
  InitPdlrqTemplate();
  /* peer delay response messages */
  InitPdlrspTemplate();
#if( k_TWO_STEP == TRUE )
  /* peer delay response follow up messages */
  InitPdlrspFlwTemplate();
#endif /* #if( k_TWO_STEP == TRUE ) */
#endif /* #if( k_CLK_DEL_P2P == TRUE ) */
}


/*************************************************************************
**    static functions
*************************************************************************/

/***********************************************************************
** 
** Function    : SendEvntMsg
**
** Description : Wrapper function for sending sync and delay_req messages.
**               The only difference between this function is the type.
**
** See Also    : NIF_SendSync(),NIF_SendDelayReq()
**
** Parameters  : ps_pAddr  (IN) - pointer to destination port address
**                                ( = NULL for multicast messages)
**               pb_buf    (IN) - send buffer
**               w_ifIdx   (IN) - communication interface index
**               w_seqId   (IN) - Sequence Id for the Message
**               c_Intv    (IN) - interval of message type
** 
** Returnvalue : TRUE           - success
**               FALSE          - function failed
**
** Remarks     : -
**
*************************************************************************/
static BOOLEAN SendEvntMsg( const PTP_t_PortAddr *ps_pAddr,
                            UINT8                *pb_buf,
                            UINT16               w_ifIdx,
                            UINT16               w_seqId, 
                            INT8                 c_Intv)
{
  UINT16  w_len;
  /* 
  - message type is set 
  - version PTP is set
  - message length is set
  - domain number is set
  */
  /* set changing flags */
  if( ps_pAddr == NULL )
  {
    NIF_PackHdrFlag(pb_buf,k_FLG_UNICAST,FALSE);
  }
  else
  {
    NIF_PackHdrFlag(pb_buf,k_FLG_UNICAST,TRUE);
  }
  /*
  - correction field is already set
  - port id is already set
  */
  GOE_hton16(&pb_buf[k_OFFS_SEQ_ID],w_seqId);
  /* control is already set */
  pb_buf[k_OFFS_LOGMEAN] = (UINT8)c_Intv;
  /*
   - timestamp is always zero - one-step clocks fill the timestamp in HW   
  */
  /* padding or not ? */
  if( as_padding[w_ifIdx].o_pad == TRUE )
  {
    w_len = k_PTPV2_EVT_PAD_SZE;
  }
  else
  {
    w_len = k_PTPV2_HDR_SZE + k_PTPV2_SYNC_PLD;
  }
  /* send message */
  return GOE_SendMsg(ps_pAddr,pb_buf,k_SOCK_EVT,w_ifIdx,w_len);
}

#if((k_CLK_IS_OC == TRUE)|| (k_CLK_IS_BC == TRUE ))
/***********************************************************************
** 
** Function    : InitAnncTemplate
**
** Description : Initializes the network ordered message buffers for
**               the announce message. All data that do not change get 
**               set in the buffer.
**
** See Also    : NIF_InitMsgTempls
**
** Parameters  : -
** 
** Returnvalue : -
**
** Remarks     : -
**
*************************************************************************/
static void InitAnncTemplate( void )
{
  NIF_t_PTPV2_AnnMsg     s_anncMsg;
  UINT16                 w_ifIdx;
  
  s_anncMsg.s_ptpHead.e_msgType = e_MT_ANNOUNCE; 
  s_anncMsg.s_ptpHead.b_versPTP = k_PTP_VERSION;
  s_anncMsg.s_ptpHead.w_msgLen  = k_PTPV2_HDR_SZE + k_PTPV2_ANNC_PLD;
  s_anncMsg.s_ptpHead.b_dmnNmb  = NIF_ps_ClkDs->s_DefDs.b_domn;
  /* preset flags to zero (=FALSE) */
  s_anncMsg.s_ptpHead.w_flags   = 0;
  SET_FLAG(s_anncMsg.s_ptpHead.w_flags ,k_FLG_ALT_MASTER     ,FALSE);
  SET_FLAG(s_anncMsg.s_ptpHead.w_flags ,k_FLG_TWO_STEP       ,FALSE);
  SET_FLAG(s_anncMsg.s_ptpHead.w_flags ,k_FLG_UNICAST        ,FALSE);
  SET_FLAG(s_anncMsg.s_ptpHead.w_flags,k_FLG_PTP_PROF_SPEC1,k_PROF_SPEC_1);
  SET_FLAG(s_anncMsg.s_ptpHead.w_flags,k_FLG_PTP_PROF_SPEC2,k_PROF_SPEC_2);
  SET_FLAG(s_anncMsg.s_ptpHead.w_flags ,k_FLG_LI_61          ,FALSE);
  SET_FLAG(s_anncMsg.s_ptpHead.w_flags ,k_FLG_LI_59          ,FALSE);
  SET_FLAG(s_anncMsg.s_ptpHead.w_flags ,k_FLG_CUR_UTCOFFS_VAL,FALSE);
  SET_FLAG(s_anncMsg.s_ptpHead.w_flags ,k_FLG_PTP_TIMESCALE  ,FALSE);
  SET_FLAG(s_anncMsg.s_ptpHead.w_flags ,k_FLG_TIME_TRACEABLE ,FALSE);
  SET_FLAG(s_anncMsg.s_ptpHead.w_flags ,k_FLG_FREQ_TRACEABLE ,FALSE);

  s_anncMsg.s_ptpHead.ll_corField = 0;   
  s_anncMsg.s_ptpHead.w_seqId = 0;
  s_anncMsg.s_ptpHead.e_ctrl  = e_CTLF_ALL_OTHERS;
  /* set timestamp to zero */
  s_anncMsg.s_origTs.u48_sec = 0;
  s_anncMsg.s_origTs.dw_Nsec = 0;
  /* port specific things */
  for( w_ifIdx = 0 ; w_ifIdx < k_NUM_IF ; w_ifIdx++)
  {
    /* for ordinary clock implementations, this must not be changed */
    s_anncMsg.s_ptpHead.s_srcPortId = NIF_ps_ClkDs->as_PortDs[w_ifIdx].s_portId;
    s_anncMsg.s_ptpHead.c_logMeanMsgIntv = 
              NIF_ps_ClkDs->as_PortDs[w_ifIdx].c_AnncIntv;
    /* pack it in the predefined header */
    NIF_PackAnncMsg(NIF_aab_AnncMsg[w_ifIdx],&s_anncMsg);
  }
}

/***********************************************************************
** 
** Function    : InitSyncTemplate
**
** Description : Initializes the network ordered message buffers for
**               the sync message. All data that do not change get 
**               set in the buffer.
**
** See Also    : NIF_InitMsgTempls
**
** Parameters  : -
** 
** Returnvalue : -
**
** Remarks     : -
**
*************************************************************************/
static void InitSyncTemplate( void )
{
  NIF_t_PTPV2_SyncMsg    s_syncMsg;
  UINT16                 w_ifIdx;

  s_syncMsg.s_ptpHead.e_msgType = e_MT_SYNC; 
  s_syncMsg.s_ptpHead.b_versPTP = k_PTP_VERSION;
  s_syncMsg.s_ptpHead.w_msgLen  = k_PTPV2_HDR_SZE + k_PTPV2_SYNC_PLD;
  s_syncMsg.s_ptpHead.b_dmnNmb  = NIF_ps_ClkDs->s_DefDs.b_domn; 
  /* preset flags to zero (=FALSE) */
  s_syncMsg.s_ptpHead.w_flags   = 0;
  SET_FLAG(s_syncMsg.s_ptpHead.w_flags ,k_FLG_ALT_MASTER     ,FALSE);
  SET_FLAG(s_syncMsg.s_ptpHead.w_flags ,k_FLG_TWO_STEP       ,k_TWO_STEP);
  SET_FLAG(s_syncMsg.s_ptpHead.w_flags ,k_FLG_UNICAST        ,FALSE);
  SET_FLAG(s_syncMsg.s_ptpHead.w_flags,k_FLG_PTP_PROF_SPEC1,k_PROF_SPEC_1);
  SET_FLAG(s_syncMsg.s_ptpHead.w_flags,k_FLG_PTP_PROF_SPEC2,k_PROF_SPEC_2);
  SET_FLAG(s_syncMsg.s_ptpHead.w_flags ,k_FLG_LI_61          ,FALSE);
  SET_FLAG(s_syncMsg.s_ptpHead.w_flags ,k_FLG_LI_59          ,FALSE);
  SET_FLAG(s_syncMsg.s_ptpHead.w_flags ,k_FLG_CUR_UTCOFFS_VAL,FALSE);
  SET_FLAG(s_syncMsg.s_ptpHead.w_flags ,k_FLG_PTP_TIMESCALE  ,FALSE);
  SET_FLAG(s_syncMsg.s_ptpHead.w_flags ,k_FLG_TIME_TRACEABLE ,FALSE);
  SET_FLAG(s_syncMsg.s_ptpHead.w_flags ,k_FLG_FREQ_TRACEABLE ,FALSE);
  s_syncMsg.s_ptpHead.ll_corField = 0;   
  s_syncMsg.s_ptpHead.w_seqId = 0;
  s_syncMsg.s_ptpHead.e_ctrl  = e_CTLF_SYNC;
  s_syncMsg.s_origTs.u48_sec  = 0;
  s_syncMsg.s_origTs.dw_Nsec  = 0;

  /* port specific things */
  for( w_ifIdx = 0 ; w_ifIdx < k_NUM_IF; w_ifIdx++)
  {
    /* for ordinary clock implementations, this must not be changed */
    s_syncMsg.s_ptpHead.s_srcPortId = NIF_ps_ClkDs->as_PortDs[w_ifIdx].s_portId;
    s_syncMsg.s_ptpHead.c_logMeanMsgIntv = 
                      NIF_ps_ClkDs->as_PortDs[w_ifIdx].c_SynIntv;
    /* pack it in the predefined header */
    NIF_PackSyncMsg(NIF_aab_SyncMsg[w_ifIdx],&s_syncMsg);
    /* set padding bytes to zero */
    PTP_MEMSET(&NIF_aab_SyncMsg[w_ifIdx][k_PTPV2_HDR_SZE+k_PTPV2_SYNC_PLD],
               '\0',
               (k_PTPV2_EVT_PAD_SZE - (k_PTPV2_HDR_SZE+k_PTPV2_SYNC_PLD)));
  }
}

/***********************************************************************
** 
** Function    : InitFlwUpTemplate
**
** Description : Initializes the network ordered message buffers for
**               the follow up message. All data that do not change get 
**               set in the buffer.
**
** See Also    : NIF_InitMsgTempls
**
** Parameters  : -
** 
** Returnvalue : -
**
** Remarks     : -
**
*************************************************************************/
static void InitFlwUpTemplate( void )
{
  NIF_t_PTPV2_FlwUpMsg s_flupMsg;
  UINT16               w_ifIdx;

  s_flupMsg.s_ptpHead.e_msgType = e_MT_FLWUP; 
  s_flupMsg.s_ptpHead.b_versPTP = k_PTP_VERSION;
  s_flupMsg.s_ptpHead.w_msgLen  = k_PTPV2_HDR_SZE + k_PTPV2_FLUP_PLD;
  s_flupMsg.s_ptpHead.b_dmnNmb  = NIF_ps_ClkDs->s_DefDs.b_domn; 
  /* preset flags to zero (=FALSE) */
  s_flupMsg.s_ptpHead.w_flags   = 0;
  SET_FLAG(s_flupMsg.s_ptpHead.w_flags ,k_FLG_ALT_MASTER     ,FALSE);
  SET_FLAG(s_flupMsg.s_ptpHead.w_flags ,k_FLG_TWO_STEP       ,FALSE);
  SET_FLAG(s_flupMsg.s_ptpHead.w_flags ,k_FLG_UNICAST        ,FALSE);
  SET_FLAG(s_flupMsg.s_ptpHead.w_flags,k_FLG_PTP_PROF_SPEC1,k_PROF_SPEC_1);
  SET_FLAG(s_flupMsg.s_ptpHead.w_flags,k_FLG_PTP_PROF_SPEC2,k_PROF_SPEC_2);
  SET_FLAG(s_flupMsg.s_ptpHead.w_flags ,k_FLG_LI_61          ,FALSE);
  SET_FLAG(s_flupMsg.s_ptpHead.w_flags ,k_FLG_LI_59          ,FALSE);
  SET_FLAG(s_flupMsg.s_ptpHead.w_flags ,k_FLG_CUR_UTCOFFS_VAL,FALSE);
  SET_FLAG(s_flupMsg.s_ptpHead.w_flags ,k_FLG_PTP_TIMESCALE  ,FALSE);
  SET_FLAG(s_flupMsg.s_ptpHead.w_flags ,k_FLG_TIME_TRACEABLE ,FALSE);
  SET_FLAG(s_flupMsg.s_ptpHead.w_flags ,k_FLG_FREQ_TRACEABLE ,FALSE);

  s_flupMsg.s_ptpHead.ll_corField = 0;   
  s_flupMsg.s_ptpHead.w_seqId = 0;
  s_flupMsg.s_ptpHead.e_ctrl  = e_CTLF_FOLLOWUP;
  /* port specific things */
  for( w_ifIdx = 0; w_ifIdx < k_NUM_IF; w_ifIdx++)
  {
    s_flupMsg.s_ptpHead.s_srcPortId = NIF_ps_ClkDs->as_PortDs[w_ifIdx].s_portId;
    s_flupMsg.s_ptpHead.c_logMeanMsgIntv = 
              NIF_ps_ClkDs->as_PortDs[w_ifIdx].c_SynIntv;
    /* pack it in the predefined buffer */
    NIF_PackFlwUpMsg(NIF_aab_FlwupMsg[w_ifIdx],&s_flupMsg);
  }
}

/***********************************************************************
** 
** Function    : InitDlrqTemplate
**
** Description : Initializes the network ordered message buffers for
**               the delay request message. All data that do not change get 
**               set in the buffer.
**
** See Also    : NIF_InitMsgTempls
**
** Parameters  : -
** 
** Returnvalue : -
**
** Remarks     : -
**
*************************************************************************/
static void InitDlrqTemplate( void ) 
{
  NIF_t_PTPV2_DlrqMsg s_dlrqMsg;
  UINT16              w_ifIdx;

  s_dlrqMsg.s_ptpHead.e_msgType = e_MT_DELREQ; 
  s_dlrqMsg.s_ptpHead.b_versPTP = k_PTP_VERSION;
  s_dlrqMsg.s_ptpHead.w_msgLen  = k_PTPV2_HDR_SZE + k_PTPV2_DLRQ_PLD;
  s_dlrqMsg.s_ptpHead.b_dmnNmb  = NIF_ps_ClkDs->s_DefDs.b_domn; 
  /* preset flags to zero (=FALSE) */
  s_dlrqMsg.s_ptpHead.w_flags   = 0;
  SET_FLAG(s_dlrqMsg.s_ptpHead.w_flags ,k_FLG_ALT_MASTER     ,FALSE);
  SET_FLAG(s_dlrqMsg.s_ptpHead.w_flags ,k_FLG_TWO_STEP       ,FALSE);
  SET_FLAG(s_dlrqMsg.s_ptpHead.w_flags ,k_FLG_UNICAST        ,FALSE);
  SET_FLAG(s_dlrqMsg.s_ptpHead.w_flags,k_FLG_PTP_PROF_SPEC1,k_PROF_SPEC_1);
  SET_FLAG(s_dlrqMsg.s_ptpHead.w_flags,k_FLG_PTP_PROF_SPEC2,k_PROF_SPEC_2);
  SET_FLAG(s_dlrqMsg.s_ptpHead.w_flags ,k_FLG_LI_61          ,FALSE);
  SET_FLAG(s_dlrqMsg.s_ptpHead.w_flags ,k_FLG_LI_59          ,FALSE);
  SET_FLAG(s_dlrqMsg.s_ptpHead.w_flags ,k_FLG_CUR_UTCOFFS_VAL,FALSE);
  SET_FLAG(s_dlrqMsg.s_ptpHead.w_flags ,k_FLG_PTP_TIMESCALE  ,FALSE);
  SET_FLAG(s_dlrqMsg.s_ptpHead.w_flags ,k_FLG_TIME_TRACEABLE ,FALSE);
  SET_FLAG(s_dlrqMsg.s_ptpHead.w_flags ,k_FLG_FREQ_TRACEABLE ,FALSE);

  s_dlrqMsg.s_ptpHead.ll_corField = 0; 
  s_dlrqMsg.s_ptpHead.w_seqId = 0;
  s_dlrqMsg.s_ptpHead.e_ctrl  = e_CTLF_DELAY_REQ;
  s_dlrqMsg.s_origTs.u48_sec  = 0;
  s_dlrqMsg.s_origTs.dw_Nsec  = 0;
  s_dlrqMsg.s_ptpHead.c_logMeanMsgIntv = 0x7F;
  /* port specific things */
  for( w_ifIdx = 0 ; w_ifIdx < k_NUM_IF; w_ifIdx++)
  {
    s_dlrqMsg.s_ptpHead.s_srcPortId = NIF_ps_ClkDs->as_PortDs[w_ifIdx].s_portId;
    /* pack it in the predefined buffer */
    NIF_PackDelReqMsg(NIF_aab_DlrqMsg[w_ifIdx],&s_dlrqMsg);
    /* set padding bytes to zero */
    PTP_MEMSET(&NIF_aab_DlrqMsg[w_ifIdx][k_PTPV2_HDR_SZE+k_PTPV2_DLRQ_PLD],
               '\0',
               (k_PTPV2_EVT_PAD_SZE - (k_PTPV2_HDR_SZE+k_PTPV2_DLRQ_PLD)));
  }
}

/***********************************************************************
** 
** Function    : InitDlrspTemplate
**
** Description : Initializes the network ordered message buffers for
**               the delay response message. All data that do not change get 
**               set in the buffer.
**
** See Also    : NIF_InitMsgTempls
**
** Parameters  : -
** 
** Returnvalue : -
**
** Remarks     : -
**
*************************************************************************/
static void InitDlrspTemplate( void )
{
  NIF_t_PTPV2_DlRspMsg   s_dlrspMsg;
  UINT16                 w_ifIdx;
  
  s_dlrspMsg.s_ptpHead.e_msgType = e_MT_DELRESP; 
  s_dlrspMsg.s_ptpHead.b_versPTP = k_PTP_VERSION;
  s_dlrspMsg.s_ptpHead.w_msgLen  = k_PTPV2_HDR_SZE + k_PTPV2_DLRSP_PLD;
  s_dlrspMsg.s_ptpHead.b_dmnNmb  = NIF_ps_ClkDs->s_DefDs.b_domn; 
  /* preset flags to zero (=FALSE) */
  s_dlrspMsg.s_ptpHead.w_flags   = 0;
  SET_FLAG(s_dlrspMsg.s_ptpHead.w_flags ,k_FLG_ALT_MASTER     ,FALSE);
  SET_FLAG(s_dlrspMsg.s_ptpHead.w_flags ,k_FLG_TWO_STEP       ,FALSE);
  SET_FLAG(s_dlrspMsg.s_ptpHead.w_flags ,k_FLG_UNICAST        ,FALSE);
  SET_FLAG(s_dlrspMsg.s_ptpHead.w_flags,k_FLG_PTP_PROF_SPEC1,k_PROF_SPEC_1);
  SET_FLAG(s_dlrspMsg.s_ptpHead.w_flags,k_FLG_PTP_PROF_SPEC2,k_PROF_SPEC_2);
  SET_FLAG(s_dlrspMsg.s_ptpHead.w_flags ,k_FLG_LI_61          ,FALSE);
  SET_FLAG(s_dlrspMsg.s_ptpHead.w_flags ,k_FLG_LI_59          ,FALSE);
  SET_FLAG(s_dlrspMsg.s_ptpHead.w_flags ,k_FLG_CUR_UTCOFFS_VAL,FALSE);
  SET_FLAG(s_dlrspMsg.s_ptpHead.w_flags ,k_FLG_PTP_TIMESCALE  ,FALSE);
  SET_FLAG(s_dlrspMsg.s_ptpHead.w_flags ,k_FLG_TIME_TRACEABLE ,FALSE);
  SET_FLAG(s_dlrspMsg.s_ptpHead.w_flags ,k_FLG_FREQ_TRACEABLE ,FALSE);

  s_dlrspMsg.s_ptpHead.ll_corField = 0; 
  s_dlrspMsg.s_ptpHead.w_seqId = 0;
  s_dlrspMsg.s_ptpHead.e_ctrl  = e_CTLF_DELAY_RESP;
  /* port specific things */
  for( w_ifIdx = 0 ; w_ifIdx < k_NUM_IF; w_ifIdx++)
  {
    s_dlrspMsg.s_ptpHead.s_srcPortId 
                               = NIF_ps_ClkDs->as_PortDs[w_ifIdx].s_portId;
    s_dlrspMsg.s_ptpHead.c_logMeanMsgIntv             
                               = NIF_ps_ClkDs->as_PortDs[w_ifIdx].c_mMDlrqIntv;
    /* pack it in the predefined buffer */
    NIF_PackDelRespMsg(NIF_aab_DlrspMsg[w_ifIdx],&s_dlrspMsg);
  }
}
#endif /* #if((k_CLK_IS_OC == TRUE)|| (k_CLK_IS_BC == TRUE )) */

/***********************************************************************
** 
** Function    : InitMntTemplate
**
** Description : Initializes the network ordered message buffers for
**               the management message. All data that do not change get 
**               set in the buffer.
**
** See Also    : NIF_InitMsgTempls
**
** Parameters  : -
** 
** Returnvalue : -
**
** Remarks     : -
**
*************************************************************************/
static void InitMntTemplate( void )
{
  NIF_t_PTPV2_MntMsg s_mntMsg;
  UINT16             w_ifIdx;

  s_mntMsg.s_ptpHead.e_msgType = e_MT_MNGMNT; 
  s_mntMsg.s_ptpHead.b_versPTP = k_PTP_VERSION;
#if((k_CLK_IS_OC == TRUE)|| (k_CLK_IS_BC == TRUE ))
  s_mntMsg.s_ptpHead.b_dmnNmb  = NIF_ps_ClkDs->s_DefDs.b_domn; 
#else /* #if((k_CLK_IS_OC == TRUE)|| (k_CLK_IS_BC == TRUE )) */
  s_mntMsg.s_ptpHead.b_dmnNmb  = NIF_ps_ClkDs->s_TcDefDs.b_primDom; 
#endif /* #if((k_CLK_IS_OC == TRUE)|| (k_CLK_IS_BC == TRUE )) */
  /* preset flags to zero (=FALSE) */
  s_mntMsg.s_ptpHead.w_flags   = 0;
  SET_FLAG(s_mntMsg.s_ptpHead.w_flags ,k_FLG_ALT_MASTER     ,FALSE);
  SET_FLAG(s_mntMsg.s_ptpHead.w_flags ,k_FLG_TWO_STEP       ,FALSE);
  SET_FLAG(s_mntMsg.s_ptpHead.w_flags ,k_FLG_UNICAST        ,FALSE);
  SET_FLAG(s_mntMsg.s_ptpHead.w_flags,k_FLG_PTP_PROF_SPEC1,k_PROF_SPEC_1);
  SET_FLAG(s_mntMsg.s_ptpHead.w_flags,k_FLG_PTP_PROF_SPEC2,k_PROF_SPEC_2);
  SET_FLAG(s_mntMsg.s_ptpHead.w_flags ,k_FLG_LI_61          ,FALSE);
  SET_FLAG(s_mntMsg.s_ptpHead.w_flags ,k_FLG_LI_59          ,FALSE);
  SET_FLAG(s_mntMsg.s_ptpHead.w_flags ,k_FLG_CUR_UTCOFFS_VAL,FALSE);
  SET_FLAG(s_mntMsg.s_ptpHead.w_flags ,k_FLG_PTP_TIMESCALE  ,FALSE);
  SET_FLAG(s_mntMsg.s_ptpHead.w_flags ,k_FLG_TIME_TRACEABLE ,FALSE);
  SET_FLAG(s_mntMsg.s_ptpHead.w_flags ,k_FLG_FREQ_TRACEABLE ,FALSE);

  s_mntMsg.s_ptpHead.ll_corField = 0; 
  s_mntMsg.s_ptpHead.w_seqId = 0;
  s_mntMsg.s_ptpHead.e_ctrl  = e_CTLF_MANAGEMENT;
  s_mntMsg.s_ptpHead.c_logMeanMsgIntv = 0x7F;

  /* port specific things */
  for( w_ifIdx = 0 ; w_ifIdx < k_NUM_IF; w_ifIdx++)
  {
#if((k_CLK_IS_OC == TRUE)|| (k_CLK_IS_BC == TRUE ))
    s_mntMsg.s_ptpHead.s_srcPortId = NIF_ps_ClkDs->as_PortDs[w_ifIdx].s_portId;
#else /* #if((k_CLK_IS_OC == TRUE)|| (k_CLK_IS_BC == TRUE )) */
    s_mntMsg.s_ptpHead.s_srcPortId = NIF_ps_ClkDs->as_TcPortDs[w_ifIdx].s_pId;
#endif /* #if((k_CLK_IS_OC == TRUE)|| (k_CLK_IS_BC == TRUE )) */
    /* pack just the header in the predefined buffer */
    NIF_PackHdr(NIF_aab_MntMsg[w_ifIdx],&s_mntMsg.s_ptpHead);
  }
}

#if(k_CLK_DEL_P2P == TRUE)
/***********************************************************************
** 
** Function    : InitPdlrqTemplate
**
** Description : Initializes the network ordered message buffers for
**               the peer delay request message. All data that do not 
**               change get set in the buffer.
**
** See Also    : NIF_InitMsgTempls
**
** Parameters  : -
** 
** Returnvalue : -
**
** Remarks     : -
**
*************************************************************************/
static void InitPdlrqTemplate( void )
{
  NIF_t_PTPV2_PDelReq    s_p_DlrqMsg; 
  UINT16                 w_ifIdx;
 
  s_p_DlrqMsg.s_ptpHead.e_msgType = e_MT_PDELREQ; 
  s_p_DlrqMsg.s_ptpHead.b_versPTP = k_PTP_VERSION;
  s_p_DlrqMsg.s_ptpHead.w_msgLen  = k_PTPV2_HDR_SZE + k_PTPV2_P_DLRQ_PLD;
#if(k_CLK_IS_TC == FALSE )  
  s_p_DlrqMsg.s_ptpHead.b_dmnNmb  = NIF_ps_ClkDs->s_DefDs.b_domn; 
#else /* #if(k_CLK_IS_TC == FALSE ) */
  s_p_DlrqMsg.s_ptpHead.b_dmnNmb  = NIF_ps_ClkDs->s_TcDefDs.b_primDom;
#endif /*#if(k_CLK_IS_TC == FALSE ) */
  /* preset flags to zero (=FALSE) */
  s_p_DlrqMsg.s_ptpHead.w_flags   = 0;
  SET_FLAG(s_p_DlrqMsg.s_ptpHead.w_flags ,k_FLG_ALT_MASTER     ,FALSE);
  SET_FLAG(s_p_DlrqMsg.s_ptpHead.w_flags ,k_FLG_TWO_STEP       ,FALSE);
  SET_FLAG(s_p_DlrqMsg.s_ptpHead.w_flags ,k_FLG_UNICAST        ,FALSE);
  SET_FLAG(s_p_DlrqMsg.s_ptpHead.w_flags,k_FLG_PTP_PROF_SPEC1,k_PROF_SPEC_1);
  SET_FLAG(s_p_DlrqMsg.s_ptpHead.w_flags,k_FLG_PTP_PROF_SPEC2,k_PROF_SPEC_2);
  SET_FLAG(s_p_DlrqMsg.s_ptpHead.w_flags ,k_FLG_LI_61          ,FALSE);
  SET_FLAG(s_p_DlrqMsg.s_ptpHead.w_flags ,k_FLG_LI_59          ,FALSE);
  SET_FLAG(s_p_DlrqMsg.s_ptpHead.w_flags ,k_FLG_CUR_UTCOFFS_VAL,FALSE);
  SET_FLAG(s_p_DlrqMsg.s_ptpHead.w_flags ,k_FLG_PTP_TIMESCALE  ,FALSE);
  SET_FLAG(s_p_DlrqMsg.s_ptpHead.w_flags ,k_FLG_TIME_TRACEABLE ,FALSE);
  SET_FLAG(s_p_DlrqMsg.s_ptpHead.w_flags ,k_FLG_FREQ_TRACEABLE ,FALSE);

  s_p_DlrqMsg.s_ptpHead.ll_corField = 0; 
  s_p_DlrqMsg.s_ptpHead.w_seqId = 0;
  s_p_DlrqMsg.s_ptpHead.e_ctrl  = e_CTLF_ALL_OTHERS;
  s_p_DlrqMsg.s_origTs.u48_sec  = 0;
  s_p_DlrqMsg.s_origTs.dw_Nsec  = 0;
  s_p_DlrqMsg.s_ptpHead.c_logMeanMsgIntv = 0x7F;    
  
  /* port specific */
  for( w_ifIdx = 0 ; w_ifIdx < k_NUM_IF; w_ifIdx++)
  {
#if(k_CLK_IS_TC == FALSE )
    s_p_DlrqMsg.s_ptpHead.s_srcPortId 
                                   = NIF_ps_ClkDs->as_PortDs[w_ifIdx].s_portId;
#else
    s_p_DlrqMsg.s_ptpHead.s_srcPortId 
                                   = NIF_ps_ClkDs->as_TcPortDs[w_ifIdx].s_pId;
#endif /* #if(k_CLK_IS_TC == FALSE ) */
    /* pack it in the predefined buffer */
    NIF_PackPDlrq(NIF_aab_P_DlrqMsg[w_ifIdx],&s_p_DlrqMsg);
    /* set padding bytes to zero */
    PTP_MEMSET(&NIF_aab_P_DlrqMsg[w_ifIdx][k_PTPV2_HDR_SZE+k_PTPV2_P_DLRQ_PLD],
               '\0',
               (k_PTPV2_EVT_PAD_SZE - (k_PTPV2_HDR_SZE+k_PTPV2_P_DLRQ_PLD)));
  }
}

/***********************************************************************
** 
** Function    : InitPdlrspTemplate
**
** Description : Initializes the network ordered message buffers for
**               the peer delay response message. All data that do not  
**               change get set in the buffer.
**
** See Also    : NIF_InitMsgTempls
**
** Parameters  : -
** 
** Returnvalue : -
**
** Remarks     : -
**
*************************************************************************/
static void InitPdlrspTemplate( void )
{
  NIF_t_PTPV2_PDelResp   s_p_DlrspMsg;  
  UINT16                 w_ifIdx;

  s_p_DlrspMsg.s_ptpHead.e_msgType = e_MT_PDELRESP; 
  s_p_DlrspMsg.s_ptpHead.b_versPTP = k_PTP_VERSION;
  s_p_DlrspMsg.s_ptpHead.w_msgLen  = k_PTPV2_HDR_SZE + k_PTPV2_P_DLRSP_PLD;
#if(k_CLK_IS_TC == FALSE )  
  s_p_DlrspMsg.s_ptpHead.b_dmnNmb  = NIF_ps_ClkDs->s_DefDs.b_domn; 
#else /* #if(k_CLK_IS_TC == FALSE ) */
  s_p_DlrspMsg.s_ptpHead.b_dmnNmb  = NIF_ps_ClkDs->s_TcDefDs.b_primDom;
#endif /*#if(k_CLK_IS_TC == FALSE ) */ 
  /* preset flags to zero (=FALSE) */
  s_p_DlrspMsg.s_ptpHead.w_flags   = 0;
  SET_FLAG(s_p_DlrspMsg.s_ptpHead.w_flags ,k_FLG_ALT_MASTER     ,FALSE);
  SET_FLAG(s_p_DlrspMsg.s_ptpHead.w_flags ,k_FLG_TWO_STEP       ,k_TWO_STEP);
  SET_FLAG(s_p_DlrspMsg.s_ptpHead.w_flags ,k_FLG_UNICAST        ,FALSE);
  SET_FLAG(s_p_DlrspMsg.s_ptpHead.w_flags,k_FLG_PTP_PROF_SPEC1,k_PROF_SPEC_1);
  SET_FLAG(s_p_DlrspMsg.s_ptpHead.w_flags,k_FLG_PTP_PROF_SPEC2,k_PROF_SPEC_2);
  SET_FLAG(s_p_DlrspMsg.s_ptpHead.w_flags ,k_FLG_LI_61          ,FALSE);
  SET_FLAG(s_p_DlrspMsg.s_ptpHead.w_flags ,k_FLG_LI_59          ,FALSE);
  SET_FLAG(s_p_DlrspMsg.s_ptpHead.w_flags ,k_FLG_CUR_UTCOFFS_VAL,FALSE);
  SET_FLAG(s_p_DlrspMsg.s_ptpHead.w_flags ,k_FLG_PTP_TIMESCALE  ,FALSE);
  SET_FLAG(s_p_DlrspMsg.s_ptpHead.w_flags ,k_FLG_TIME_TRACEABLE ,FALSE);
  SET_FLAG(s_p_DlrspMsg.s_ptpHead.w_flags ,k_FLG_FREQ_TRACEABLE ,FALSE);

  s_p_DlrspMsg.s_ptpHead.ll_corField = 0; 
  s_p_DlrspMsg.s_ptpHead.w_seqId = 0;
  s_p_DlrspMsg.s_ptpHead.e_ctrl  = e_CTLF_ALL_OTHERS;
  s_p_DlrspMsg.s_reqRecptTs.u48_sec  = 0;
  s_p_DlrspMsg.s_reqRecptTs.dw_Nsec  = 0;
  s_p_DlrspMsg.s_ptpHead.c_logMeanMsgIntv = 0x7F; 
  /* port specific things */
  for( w_ifIdx = 0 ; w_ifIdx < k_NUM_IF; w_ifIdx++)
  { 
#if(k_CLK_IS_TC == FALSE )  
    s_p_DlrspMsg.s_ptpHead.s_srcPortId 
                                   = NIF_ps_ClkDs->as_PortDs[w_ifIdx].s_portId;
#else
    s_p_DlrspMsg.s_ptpHead.s_srcPortId 
                                   = NIF_ps_ClkDs->as_TcPortDs[w_ifIdx].s_pId;
#endif
    /* pack it in the predefined buffer */
    NIF_PackPDlrsp(NIF_aab_P_DlrspMsg[w_ifIdx],&s_p_DlrspMsg);
    /* set padding bytes to zero */
    PTP_MEMSET(
       &NIF_aab_P_DlrspMsg[w_ifIdx][k_PTPV2_HDR_SZE+k_PTPV2_P_DLRSP_PLD],
       '\0',
       (k_PTPV2_EVT_PAD_SZE - (k_PTPV2_HDR_SZE+k_PTPV2_P_DLRSP_PLD)));
  }
}
#if( k_TWO_STEP == TRUE )
/***********************************************************************
** 
** Function    : InitPdlrspFlwTemplate
**
** Description : Initializes the network ordered message buffers for
**               the peer delay response message. All data that do not  
**               change get set in the buffer.
**
** See Also    : NIF_InitMsgTempls
**
** Parameters  : -
** 
** Returnvalue : -
**
** Remarks     : -
**
*************************************************************************/
static void InitPdlrspFlwTemplate( void )
{
  NIF_t_PTPV2_PDlRspFlw  s_p_DlrspFlw;  
  UINT16                 w_ifIdx;

  s_p_DlrspFlw.s_ptpHead.e_msgType = e_MT_PDELRES_FLWUP; 
  s_p_DlrspFlw.s_ptpHead.b_versPTP = k_PTP_VERSION;
  s_p_DlrspFlw.s_ptpHead.w_msgLen  = k_PTPV2_HDR_SZE + k_PTPV2_P_DLRSP_FLW_PLD;
#if(k_CLK_IS_TC == FALSE )  
  s_p_DlrspFlw.s_ptpHead.b_dmnNmb  = NIF_ps_ClkDs->s_DefDs.b_domn; 
#else /* #if(k_CLK_IS_TC == FALSE ) */
  s_p_DlrspFlw.s_ptpHead.b_dmnNmb  = NIF_ps_ClkDs->s_TcDefDs.b_primDom;
#endif /*#if(k_CLK_IS_TC == FALSE ) */ 
  /* preset flags to zero (=FALSE) */
  s_p_DlrspFlw.s_ptpHead.w_flags   = 0;
  SET_FLAG(s_p_DlrspFlw.s_ptpHead.w_flags ,k_FLG_ALT_MASTER     ,FALSE);
  SET_FLAG(s_p_DlrspFlw.s_ptpHead.w_flags ,k_FLG_TWO_STEP       ,FALSE);
  SET_FLAG(s_p_DlrspFlw.s_ptpHead.w_flags ,k_FLG_UNICAST        ,FALSE);
  SET_FLAG(s_p_DlrspFlw.s_ptpHead.w_flags,k_FLG_PTP_PROF_SPEC1,k_PROF_SPEC_1);
  SET_FLAG(s_p_DlrspFlw.s_ptpHead.w_flags,k_FLG_PTP_PROF_SPEC2,k_PROF_SPEC_2);
  SET_FLAG(s_p_DlrspFlw.s_ptpHead.w_flags ,k_FLG_LI_61          ,FALSE);
  SET_FLAG(s_p_DlrspFlw.s_ptpHead.w_flags ,k_FLG_LI_59          ,FALSE);
  SET_FLAG(s_p_DlrspFlw.s_ptpHead.w_flags ,k_FLG_CUR_UTCOFFS_VAL,FALSE);
  SET_FLAG(s_p_DlrspFlw.s_ptpHead.w_flags ,k_FLG_PTP_TIMESCALE  ,FALSE);
  SET_FLAG(s_p_DlrspFlw.s_ptpHead.w_flags ,k_FLG_TIME_TRACEABLE ,FALSE);
  SET_FLAG(s_p_DlrspFlw.s_ptpHead.w_flags ,k_FLG_FREQ_TRACEABLE ,FALSE);

  s_p_DlrspFlw.s_ptpHead.ll_corField = 0LL; 
  s_p_DlrspFlw.s_ptpHead.w_seqId = 0;
  s_p_DlrspFlw.s_ptpHead.e_ctrl  = e_CTLF_ALL_OTHERS;
  s_p_DlrspFlw.s_ptpHead.c_logMeanMsgIntv = 0x7F; 
  /* set timestamp to zero */
  s_p_DlrspFlw.s_respTs.u48_sec  = 0LL;
  s_p_DlrspFlw.s_respTs.dw_Nsec  = 0;  
  /* port specific things */
  for( w_ifIdx = 0 ; w_ifIdx < k_NUM_IF; w_ifIdx++)
  { 
#if(k_CLK_IS_TC == FALSE )
    s_p_DlrspFlw.s_ptpHead.s_srcPortId 
                                   = NIF_ps_ClkDs->as_PortDs[w_ifIdx].s_portId;
#else
    s_p_DlrspFlw.s_ptpHead.s_srcPortId 
                                   = NIF_ps_ClkDs->as_TcPortDs[w_ifIdx].s_pId;
#endif
    /* pack it in the predefined buffer */
    NIF_PackPDlrspFlw(NIF_aab_P_DlrspFlw[w_ifIdx],&s_p_DlrspFlw);
  }
}
#endif /* #if( k_TWO_STEP == TRUE ) */
#endif /* #if(k_CLK_DEL_P2P == TRUE) */
