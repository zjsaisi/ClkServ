/*************************************************************************
**    (C) 2008 - 2009 IXXAT Automation GmbH, all rights reserved
**************************************************************************
**
**       File: NIFrx.c 
**    Summary: Provides the rx interface to the network communication. 
**             This file must be adapted, when changing the 
**             communication technology.
**
**    Version: 1.01.01
**
**************************************************************************
**************************************************************************
**
**  Functions: NIF_RecvMsg
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

/*************************************************************************
**    static constants, types, macros, variables
*************************************************************************/

/*************************************************************************
**    static function-prototypes
*************************************************************************/

/*************************************************************************
**    global functions
*************************************************************************/

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
                     PTP_t_PortAddr *ps_pAdr)
{
  BOOLEAN o_ret;
  /* call GOE function to receive message */
  o_ret = GOE_RecvMsg(w_ifIdx,b_socket,pb_rxBuf,pi_bufSize,ps_pAdr);
  if( o_ret == TRUE )
  {
    /* get first bit to determine, if receiver wants padding */
    if( pb_rxBuf[0] & 0x10 )
    {
      as_padding[w_ifIdx].o_pad   = TRUE;
      as_padding[w_ifIdx].dw_tick = SIS_GetTime() + 
#if((k_CLK_IS_OC == TRUE) || (k_CLK_IS_BC == TRUE))
        (NIF_ps_ClkDs->as_PortDs[w_ifIdx].b_anncRecptTmo * NIF_dw_secTicks);
#else /* #if((k_CLK_IS_OC == TRUE) || (k_CLK_IS_BC == TRUE)) */
        NIF_dw_secTicks;
#endif /* #if((k_CLK_IS_OC == TRUE) || (k_CLK_IS_BC == TRUE)) */
    }
    else
    {
      /* check, if period is over */
      if( SIS_TIME_OVER(as_padding[w_ifIdx].dw_tick) )
      {
        as_padding[w_ifIdx].o_pad = FALSE;
      }
    }
  }
  return o_ret;
}

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
void NIF_UnPackHdrPortId(PTP_t_PortId *ps_portId,const UINT8 *pb_buf)
{
  PTP_BCOPY(ps_portId->s_clkId.ab_id,&pb_buf[k_OFFS_SRCPRTID],k_CLKID_LEN);
  ps_portId->w_portNmb = GOE_ntoh16(&pb_buf[k_OFFS_SRCPRTID+k_CLKID_LEN]);
}

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
void NIF_UnPackTrgtPortId(PTP_t_PortId *ps_trgtPId,const UINT8 *pb_buf)
{
  /* unpack target port identity */
  PTP_BCOPY(ps_trgtPId->s_clkId.ab_id,&pb_buf[k_OFFS_MNT_TRGTPID],k_CLKID_LEN);
  ps_trgtPId->w_portNmb = GOE_ntoh16(&pb_buf[k_OFFS_MNT_TRGTPID+k_CLKID_LEN]);
}

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
void NIF_UnPackReqPortId(PTP_t_PortId *ps_portId,const UINT8 *pb_buf)
{
  PTP_BCOPY(ps_portId->s_clkId.ab_id,&pb_buf[k_OFFS_P2P_REQPRTID],k_CLKID_LEN);
  ps_portId->w_portNmb = GOE_ntoh16(&pb_buf[k_OFFS_P2P_REQPRTID+k_CLKID_LEN]);
}

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
void NIF_UnPackHdr(NIF_t_PTPV2_Head *ps_hdr,const UINT8 *pb_buf)
{
  ps_hdr->e_msgType         = NIF_UNPACKHDR_MSGTYPE(pb_buf);
  ps_hdr->b_versPTP         = NIF_UNPACKHDR_VERS(pb_buf);
  ps_hdr->w_msgLen          = NIF_UNPACKHDR_MLEN(pb_buf);
  ps_hdr->b_dmnNmb          = NIF_UNPACKHDR_DOMNMB(pb_buf);
  ps_hdr->w_flags           = NIF_UNPACKHDR_FLAGS(pb_buf);
  ps_hdr->ll_corField       = NIF_UNPACKHDR_COR(pb_buf);
  NIF_UnPackHdrPortId(&ps_hdr->s_srcPortId,pb_buf);
  ps_hdr->w_seqId           = NIF_UNPACKHDR_SEQID(pb_buf);
  ps_hdr->e_ctrl            = NIF_UNPACKHDR_CTRL(pb_buf);
  ps_hdr->c_logMeanMsgIntv  = NIF_UNPACKHDR_MMINTV(pb_buf);
}

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
void NIF_UnPackAnncMsg(NIF_t_PTPV2_AnnMsg *ps_anncMsg,
                       const UINT8 *pb_buf)
{
  /* unpack the header */
  NIF_UnPackHdr(&ps_anncMsg->s_ptpHead,pb_buf);
  /* unpack origin timestamp */
  ps_anncMsg->s_origTs.u48_sec = GOE_ntoh48(&pb_buf[k_OFFS_TS_ANNC_SEC]);
  ps_anncMsg->s_origTs.dw_Nsec = GOE_ntoh32(&pb_buf[k_OFFS_TS_ANNC_NSEC]);
  /* unpack current UTC offset */
  ps_anncMsg->i_curUTCOffs = (INT16)GOE_ntoh16(&pb_buf[k_OFFS_CURUTCOFFS]);
  /* unpack grandmaster priority 1 */
  ps_anncMsg->b_grdMstPrio1 = pb_buf[k_OFFS_GM_PRIO1];
  /* unpack grandmaster clock quality */
  ps_anncMsg->s_grdMstClkQual.e_clkClass  
                               = (PTP_t_clkClassEnum)pb_buf[k_OFFS_GM_CLKQUAL];
  ps_anncMsg->s_grdMstClkQual.e_clkAccur 
                               = (PTP_t_clkAccEnum)pb_buf[k_OFFS_GM_CLKQUAL+1];
  ps_anncMsg->s_grdMstClkQual.w_scldVar  
                               = GOE_ntoh16(&pb_buf[k_OFFS_GM_CLKQUAL+2]);
  /* unpack grandmaster priority 2 */
  ps_anncMsg->b_grdMstPrio2 = pb_buf[k_OFFS_GM_PRIO2];
  /* unpack grandmaster Identity */
  PTP_BCOPY(ps_anncMsg->s_grdMstId.ab_id,&pb_buf[k_OFFS_GM_ID],k_CLKID_LEN);
  /* unpack steps removed */
  ps_anncMsg->w_stepsRemvd = GOE_ntoh16(&pb_buf[k_OFFS_STEPS_REM]);
  /* unpack time source */
  ps_anncMsg->e_timeSrc = (PTP_t_tmSrcEnum)pb_buf[k_OFFS_TMESRC];  
}

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
void NIF_UnPackSyncMsg(NIF_t_PTPV2_SyncMsg *ps_syncMsg,const UINT8 *pb_buf)
{
  /* unpack the header */
  NIF_UnPackHdr(&ps_syncMsg->s_ptpHead,pb_buf);
  /* unpack timestamp seconds */
  ps_syncMsg->s_origTs.u48_sec = GOE_ntoh48(&pb_buf[k_OFFS_TS_SYN_SEC]);
  /* unpack timestamp nanoseconds */
  ps_syncMsg->s_origTs.dw_Nsec = GOE_ntoh32(&pb_buf[k_OFFS_TS_SYN_NSEC]);
}

/*************************************************************************
**
** Function    : NIF_UnPackDelReqMsg
**
** Description : Gets the data out of a network buffer in the delay request  
**               message struct.
**
** Parameters  : ps_struct  (OUT) - pointer to delay request message struct
**               pb_buf      (IN) - pointer to network buffer
**
** Returnvalue : -
**               
** Remarks     : -
**
*************************************************************************/
/* defined in NIF.h */

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
void NIF_UnPackFlwUpMsg(NIF_t_PTPV2_FlwUpMsg *ps_flwUpMsg,
                        const UINT8 *pb_buf)
{
  /* pack the header */
  NIF_UnPackHdr(&ps_flwUpMsg->s_ptpHead,pb_buf);
  /* unpack timestamp seconds */
  ps_flwUpMsg->s_precOrigTs.u48_sec = GOE_ntoh48(&pb_buf[k_OFFS_PRECTS_SEC]);
  /* unpack timestamp nanoseconds */
  ps_flwUpMsg->s_precOrigTs.dw_Nsec = GOE_ntoh32(&pb_buf[k_OFFS_PRECTS_NSEC]);
}

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
                          const UINT8 *pb_buf)
{
  /* unpack the header */
  NIF_UnPackHdr(&ps_delRspMsg->s_ptpHead,pb_buf);
  /* unpack timestamp seconds */
  ps_delRspMsg->s_recvTs.u48_sec = GOE_ntoh48(&pb_buf[k_OFFS_RECVTS_SEC]);
  /* unpack timestamp nanoseconds */
  ps_delRspMsg->s_recvTs.dw_Nsec = GOE_ntoh32(&pb_buf[k_OFFS_RECVTS_NSEC]);
  /* unpack requesting port identity */
  NIF_UnPackReqPortId(&ps_delRspMsg->s_reqPortId,pb_buf);
}

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
void NIF_UnPackMntMsg(NIF_t_PTPV2_MntMsg *ps_mntMsg,const UINT8 *pb_buf)
{
  /* unpack the header */
  NIF_UnPackHdr(&ps_mntMsg->s_ptpHead,pb_buf);
  /* unpack target port identity */
  PTP_BCOPY(ps_mntMsg->s_trgtPid.s_clkId.ab_id,
            &pb_buf[k_OFFS_MNT_TRGTPID],k_CLKID_LEN);
  ps_mntMsg->s_trgtPid.w_portNmb = GOE_ntoh16(&pb_buf[k_OFFS_MNT_TRGTPID+
                                                      k_CLKID_LEN]);
  /* unpack starting boundary hops field */
  ps_mntMsg->b_strtBndrHops      = pb_buf[k_OFFS_MNT_STRT_BHOPS];
  /* unpack boundary hops field */
  ps_mntMsg->b_bndrHops          = NIF_UNPACK_MNT_BNDRHOPS(pb_buf);
  /* unpack action field */
  ps_mntMsg->b_actField          = (pb_buf[k_OFFS_MNT_ACTION] & 0x0F);
  /* unpack management type */
  ps_mntMsg->s_mntTlv.w_type     = GOE_ntoh16(&pb_buf[k_OFFS_MNT_TLV_TYPE]);
  /* unpack management TLV length */
  ps_mntMsg->s_mntTlv.w_len      = GOE_ntoh16(&pb_buf[k_OFFS_MNT_TLV_LEN]);
  /* unpack management id */
  ps_mntMsg->s_mntTlv.w_mntId    = GOE_ntoh16(&pb_buf[k_OFFS_MNT_TLV_ID]);
  /* unpack TLV payload - must be saved into SIS MPool */
  if(ps_mntMsg->s_mntTlv.w_len > 2)
  {
    ps_mntMsg->s_mntTlv.pb_data =(UINT8*)SIS_Alloc(ps_mntMsg->s_mntTlv.w_len-2);
    /* copy bytes only if memory was allocated */
    if(ps_mntMsg->s_mntTlv.pb_data != NULL )
    {
      PTP_BCOPY(ps_mntMsg->s_mntTlv.pb_data,
                &pb_buf[k_OFFS_MNT_TLV_DATA],
                ps_mntMsg->s_mntTlv.w_len-2);
    }
  }
  else
  {
    ps_mntMsg->s_mntTlv.pb_data = NULL;
  }
}

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
void NIF_UnpackPDlrq(NIF_t_PTPV2_PDelReq *ps_pDlrq,const UINT8 *pb_buf)
{
  /* unpack the header */
  NIF_UnPackHdr(&ps_pDlrq->s_ptpHead,pb_buf);
  /* unpack timestamp seconds */
  ps_pDlrq->s_origTs.u48_sec = GOE_ntoh48(&pb_buf[k_OFFS_P2P_ORIGTS_SEC]);
  /* unpack timestamp nanoseconds */
  ps_pDlrq->s_origTs.dw_Nsec =GOE_ntoh32(&pb_buf[k_OFFS_P2P_ORIGTS_NSEC]);
}

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
void NIF_UnpackPDlrsp(NIF_t_PTPV2_PDelResp *ps_pDlrsp,const UINT8 *pb_buf)
{
  /* unpack the header */
  NIF_UnPackHdr(&ps_pDlrsp->s_ptpHead,pb_buf);
  /* unpack timestamp seconds */
  ps_pDlrsp->s_reqRecptTs.u48_sec = GOE_ntoh48(&pb_buf[k_OFFS_P2P_REQTS_SEC]);
  /* unpack timestamp nanoseconds */
  ps_pDlrsp->s_reqRecptTs.dw_Nsec = GOE_ntoh32(&pb_buf[k_OFFS_P2P_REQTS_NSEC]);
  /* unpack requesting port id */
  PTP_BCOPY(ps_pDlrsp->s_reqPId.s_clkId.ab_id,
            &pb_buf[k_OFFS_P2P_REQPRTID],
            k_CLKID_LEN);
  ps_pDlrsp->s_reqPId.w_portNmb 
            = GOE_ntoh16(&pb_buf[k_OFFS_P2P_REQPRTID+k_CLKID_LEN]);
}

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
                         const UINT8 *pb_buf)
{
  /* unpack the header */
  NIF_UnPackHdr(&ps_pdlrspFlw->s_ptpHead,pb_buf);
  /* unpack request receipt timestamp */
  ps_pdlrspFlw->s_respTs.u48_sec = GOE_ntoh48(&pb_buf[k_OFFS_P2P_RSP_TS_SEC]);
  ps_pdlrspFlw->s_respTs.dw_Nsec = GOE_ntoh32(&pb_buf[k_OFFS_P2P_RSP_TS_NSEC]);
  /* unpack requesting port id */
  PTP_BCOPY(ps_pdlrspFlw->s_reqPId.s_clkId.ab_id,
            &pb_buf[k_OFFS_P2P_REQPRTID],
            k_CLKID_LEN);
  ps_pdlrspFlw->s_reqPId.w_portNmb 
            = GOE_ntoh16(&pb_buf[k_OFFS_P2P_REQPRTID+k_CLKID_LEN]);
}
#endif /* #if( k_CLK_DEL_P2P == TRUE ) */
/*************************************************************************
**    static functions
*************************************************************************/
