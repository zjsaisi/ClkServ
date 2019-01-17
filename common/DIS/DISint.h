/*************************************************************************
**    (C) 2008 - 2009 IXXAT Automation GmbH, all rights reserved
**************************************************************************
**
**       File: DISint.h 
**    Summary: Declares the internal functions for the Unit DIS.
**             The Unit DIS is responsible for reading messages from all
**             connected channels and dispatching them to the related Units
**             The Units send their messages to the net over function of 
**             this Unit.
**    Version: 1.01.01
**
**************************************************************************
**************************************************************************
**
**  Functions: DIS_DispEvntMsg
**             DIS_DispGnrlMsg
**             DIS_FwMntMsg
**             DIS_FwSignMsg
**             DIS_FwMsg
**             DIS_DispEvtMsgTc
**             DIS_DispGnrlMsgTc
**
**   Compiler: Ansi-C
**    Remarks:  
**
**************************************************************************
**    all rights reserved
*************************************************************************/

#ifndef __DISINT_H__
#define __DISINT_H__

#ifdef __cplusplus
extern "C"
{
#endif

/*************************************************************************
**    constants and macros
*************************************************************************/
#define DIS_k_ERR_MSG_FRMT ((UINT8) 0)  /* malformed PTP message */
#define DIS_k_ERR_MSGTP    ((UINT8) 1)  /* Undefined message type */
#define DIS_k_ERR_WRG_PORT ((UINT8) 2)  /* message uses wrong comm. port */
#define DIS_k_ERR_EVT      ((UINT8) 3)  /* event not applicable */
#define DIS_k_MBOX_WAIT    ((UINT8) 4)  /* DIS task waits for message box */
#define DIS_k_ERR_MSG_MBOX ((UINT8) 5)  /* message box of other task is full */
#define DIS_k_ERR_MPOOL_1  ((UINT8) 6)  /* no memory allocation possible */
#define DIS_k_ERR_MPOOL_2  ((UINT8) 7)  /* no memory allocation possible */
#define DIS_k_ERR_MPOOL_3  ((UINT8) 8)  /* no memory allocation possible */
#define DIS_k_ERR_MPOOL_4  ((UINT8) 9)  /* no memory allocation possible */
#define DIS_k_ERR_MPOOL_5  ((UINT8)10)  /* no memory allocation possible */
#define DIS_k_ERR_MPOOL_6  ((UINT8)11)  /* no memory allocation possible */
#define DIS_k_ERR_MPOOL_7  ((UINT8)12)  /* no memory allocation possible */
#define DIS_k_ERR_MPOOL_8  ((UINT8)13)  /* no memory allocation possible */
#define DIS_k_ERR_MPOOL_9  ((UINT8)14)  /* no memory allocation possible */
#define DIS_k_ERR_MPOOL_10 ((UINT8)15)  /* no memory allocation possible */
#define DIS_k_ERR_MPOOL_11 ((UINT8)16)  /* no memory allocation possible */
#define DIS_k_ERR_MPOOL_12 ((UINT8)17)  /* no memory allocation possible */
#define DIS_k_ERR_MPOOL_13 ((UINT8)18)  /* no memory allocation possible */
#define DIS_k_ERR_MPOOL_14 ((UINT8)19)  /* no memory allocation possible */
#define DIS_k_ERR_MPOOL_15 ((UINT8)20)  /* no memory allocation possible */
#define DIS_k_ERR_MPOOL_16 ((UINT8)21)  /* no memory allocation possible */
#define DIS_k_ERR_MPOOL_17 ((UINT8)22)  /* no memory allocation possible */
#define DIS_k_ERR_MPOOL_18 ((UINT8)23)  /* no memory allocation possible */

#define DIS_k_ERR_RXTS     ((UINT32)30) /* rx timestamp error */
#define DIS_k_ERR_TXTS     ((UINT32)31) /* tx timestamp error */
#define DIS_k_ERR_SEND     ((UINT32)32) /* send error */

/* minimum poll timeout equals to 1 */
#define k_DIS_MIN_POLL_TMO (1u)
  
/*************************************************************************
**    data types
*************************************************************************/

/*************************************************************************
** DIS_t_NetSts
                This struct defines the net status to control the net
                interfaces.
**/
typedef struct
{
  UINT32    dw_amntIf;             /* amount of initialized net interfaces */
  BOOLEAN   o_ucIfInit;            /* flag determines, if unicast net interface
                                      is initialized / operational */ 
  BOOLEAN   ao_mcIfInit[k_NUM_IF]; /* flag determines, if multicast net 
                                      interface is initialized / operational */
  UINT8     ab_ifFltCnt[k_NUM_IF]; /* fault counter for the interfaces */
  BOOLEAN   o_allInit;             /* flag determines, if all net interfaces 
                                      are initialized */
}DIS_t_NetSts;

/*************************************************************************
**    global variables
*************************************************************************/
/* Handles of the tasks to address messageboxes */
extern UINT16 DIS_aw_MboxHdl_mas[k_NUM_IF];
extern UINT16 DIS_aw_MboxHdl_mad[k_NUM_IF];
extern UINT16 DIS_aw_MboxHdl_maa[k_NUM_IF];
#if( k_CLK_DEL_P2P == TRUE )
  extern UINT16 DIS_aw_MboxHdl_p2p_del[k_NUM_IF];
  extern UINT16 DIS_aw_MboxHdl_p2p_resp[k_NUM_IF];
#endif /* #if( k_CLK_DEL_P2P == TRUE ) */

/* timestamping outbound latency */
/* timestamping latencies */
extern PTP_t_TmIntv DIS_as_inbLat[k_NUM_IF];
extern PTP_t_TmIntv DIS_as_outbLat[k_NUM_IF];

/* pointer to clock data set */
extern PTP_t_ClkDs const *DIS_ps_ClkDs;
/* Net status struct */
extern DIS_t_NetSts DIS_s_NetSts;

/*************************************************************************
**    function prototypes
*************************************************************************/

/***********************************************************************
**
** Function    : DIS_DispEvntMsg
**
** Description : Filters and dispatches the messages  
**               depending on the message content and the actual port state.
**              
** Parameters  : w_ifIdx   (IN) - actual communication interface
**               pb_rxMsg  (IN) - rx message of this channel
**               ps_rxTs   (IN) - receive timestamp 
**               i_len     (IN) - amount of received data bytes
**               e_msgType (IN) - message type to dispatch
**               ps_pAddr  (IN) - port address of sender
**
** Returnvalue : -
**                      
** Remarks     : -
**
***********************************************************************/
void DIS_DispEvntMsg( UINT16               w_ifIdx,
                      const UINT8          *pb_rxMsg,
                      PTP_t_TmStmp         *ps_rxTs,
                      INT16                i_len,
                      PTP_t_msgTypeEnum    e_msgType,
                      const PTP_t_PortAddr *ps_pAddr);  

/***********************************************************************
**
** Function    : DIS_DispGnrlMsg
**
** Description : Filters and dispatches the general messages  
**               depending on the message content and the actual port state.
**              
** Parameters  : w_ifIdx   (IN) - actual communication interface
**               pb_rxMsg  (IN) - rx message of this channel
**               i_len     (IN) - amount of received data bytes
**               e_msgType (IN) - message type to dispatch
**               ps_pAddr  (IN) - port address of sender
**
** Returnvalue : -
**                      
** Remarks     : -
**
***********************************************************************/
void DIS_DispGnrlMsg( UINT16               w_ifIdx,
                      const UINT8          *pb_rxMsg,
                      INT16                i_len,
                      PTP_t_msgTypeEnum    e_msgType,
                      const PTP_t_PortAddr *ps_pAddr);

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
void DIS_FwMntMsg(const UINT8 *pb_msg,UINT16 w_len,UINT16 w_rxIfIdx);

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
void DIS_FwSignMsg(const UINT8 *pb_msg,UINT16 w_len,UINT16 w_rxIfIdx);

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
void DIS_FwMsg(const UINT8 *pb_msg,UINT8 b_socket,
               UINT16 w_len,UINT16 w_rxIfIdx);

#if( k_CLK_IS_TC == TRUE )
/***********************************************************************
**
** Function    : DIS_DispEvtMsgTc
**
** Description : Dispatches the event messages to the regarding 
**               transparent clock unit to forward it.
**              
** Parameters  : w_ifIdx    (IN) - actual interface 
**               pb_rxMsg     (IN) - rx message of this channel
**               ps_rxTs      (IN) - receive timestamp (just event messages)
**               i_len        (IN) - amount of received data bytes4
**               e_msgType    (IN) - message type to dispatch
**
** Returnvalue : -
**
** Remarks     : -
**
***********************************************************************/
void DIS_DispEvtMsgTc( UINT16             w_ifIdx,
                       const UINT8        *pb_rxMsg,
                       const PTP_t_TmStmp *ps_rxTs,
                       INT16              i_len,
                       PTP_t_msgTypeEnum  e_msgType);

/***********************************************************************
**
** Function    : DIS_DispGnrlMsgTc
**
** Description : Dispatches the general messages to the regarding 
**               transparent clock unit to forward it.
**              
** Parameters  : w_ifIdx    (IN) - actual interface 
**               pb_rxMsg     (IN) - rx message of this channel
**               i_len        (IN) - amount of received data bytes4
**               e_msgType    (IN) - message type to dispatch
**
** Returnvalue : -
**
** Remarks     : -
**
***********************************************************************/
void DIS_DispGnrlMsgTc( UINT16             w_ifIdx,
                        const UINT8        *pb_rxMsg,
                        INT16              i_len,
                        PTP_t_msgTypeEnum  e_msgType);



#endif /* #if( k_CLK_IS_TC == TRUE ) */
#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* __DISINT_H__ */

