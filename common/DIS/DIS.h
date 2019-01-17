/*************************************************************************
**    (C) 2008 - 2009 IXXAT Automation GmbH, all rights reserved
**************************************************************************
**
**       File: DIS.h 
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
**             DISnet_Init
**             DISnet_Task
**             DIS_Task
**             DIS_GetErrStr
**             DIS_SendAnnc
**             DIS_SendSync
**             DIS_SendFollowUp
**             DIS_SendDelayReq
**             DIS_SendDelayResp
**             DIS_Send_MntMsg
**             DIS_Send_Sign
**             DIS_SendRawData
**             DIS_SendFlwUpTc
**             DIS_SendPDelRespFlwUpTc
**             DIS_SendPdlrq
**             DIS_SendPdlRsp
**             DIS_SendPdlRspFlw
**             DIS_WhoIsReceiver
**
**   Compiler: Ansi-C
**    Remarks: 
**
**************************************************************************
**    all rights reserved
*************************************************************************/

#ifndef __DIS_H__
#define __DIS_H__

#ifdef __cplusplus
extern "C"
{
#endif

/*************************************************************************
**    constants and macros
*************************************************************************/
                                       
/*************************************************************************
**    data types
*************************************************************************/

/*************************************************************************
**    global variables
*************************************************************************/

/*************************************************************************
**    function prototypes
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
              PTP_t_ClkDs const *ps_clkDataSet);

/***********************************************************************
**
** Function    : DISnet_Init
**
** Description : Initializes the network interfaces.
**              
** Parameters  : dw_amntNetIf (IN) - number of initializable net interfaces
**               pao_ifInit  (OUT) - array of flags indicating if interfaces
**                                   are initialized.
**
** Returnvalue : -
**                      
** Remarks     : -
**
***********************************************************************/
void DISnet_Init(UINT32 dw_amntNetIf, BOOLEAN *pao_ifInit);

/***********************************************************************
**
** Function    : DISnet_Task
**
** Description : The DISnet_Task handles the net interfaces. Initialization,
**               deinitialization and link speed/latency detection is
**               done.
**              
** Parameters  : w_hdl        (IN) - the SIS-task-handle
**
** Returnvalue : -
**                      
** Remarks     : -
**
***********************************************************************/
void DISnet_Task(UINT16 w_hdl);

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
void DIS_Task(UINT16 w_hdl);

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
const CHAR* DIS_GetErrStr(UINT32 dw_errNmb);

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
                  UINT16 w_ifIdx,UINT16 w_seqId );

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
                      PTP_t_TmStmp         *ps_txTs);

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
                      INT8                    c_synIntv,
                      const PTP_t_TmStmp      *ps_precTs);

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
                          UINT16       w_ifIdx,
                          UINT16       w_seqId, 
                          PTP_t_TmStmp *ps_txTs);

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
                       const PTP_t_TmStmp   *ps_recvTs);

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
                     UINT8                b_actionFld);

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
                   UINT16               w_dataLen);

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
                        PTP_t_TmStmp *ps_txTs);

#if(k_CLK_IS_TC)
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
                     UINT16             w_ifIdx);

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
                             UINT16             w_ifIdx);

#endif /* #if(k_CLK_IS_TC) */
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
                       PTP_t_TmStmp         *ps_txTs);

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
                        PTP_t_TmStmp         *ps_txTs);

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
                        const INT64          *pll_corr);

#endif /* #if( k_CLK_DEL_P2P == TRUE ) */
/***********************************************************************
**
** Function    : DIS_WhoIsReceiver
**
** Description : This function determines, if the own node is
**               addressed by the portId. If all 1s are set the own
**               node as well as all other nodes are addressed.
**              
** Parameters  : ps_portId      (IN) - portId to compare
**               po_ownNode    (OUT) - TRUE own node addressed
**                                     FALSE own node not addessed
**               po_remoteNode (OUT) - TRUE remote node addressed
**                                     FALSE remote node not addressed
**               po_oneMsg     (OUT) - TRUE more answers expected
**                                     FALSE only one answer expected
**
** Returnvalue : -
**                      
** Remarks     : -
**
***********************************************************************/
void DIS_WhoIsReceiver( const PTP_t_PortId *ps_portId,
                        BOOLEAN *po_ownNode,
                        BOOLEAN *po_remoteNode,
                        BOOLEAN *po_oneMsg );
#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* __DIS_H__ */

