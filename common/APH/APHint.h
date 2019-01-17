/*************************************************************************
**    (C) 2008 - 2009 IXXAT Automation GmbH, all rights reserved
**************************************************************************
**
**       File: APHint.h  
**    Summary: Hook Unit APH
**             The hook unit is an additional code unit, normally not used by
**             the standard stack. The hooks are used for testing. 
**             They provide the possibility to change parts of the network 
**             packets to test the reaction of other nodes and/or the 
**             complete network. 
**             These functions are used stack-internally.
**
**    Version: 1.01.01
**
**************************************************************************
**************************************************************************
**
**  Functions: APH_Init
**             APH_Hook_Send_All
**             APH_Hook_Snd
**             APH_Hook_SndTsSeq
**             APH_Hook_SndTsVers
**             APH_Hook_RxTs
**             APH_Hook_TxTs
**             APH_Hook_Recv
**             APH_Hook_Send
**             APH_CheckTxTs
**             APH_OneStepHook
**
**   Compiler: Ansi-C
**    Remarks: 
**
**************************************************************************
**    all rights reserved
*************************************************************************/

#ifndef __APHINT_H__
#define __APHINT_H__

#ifdef __cplusplus
extern "C"
{
#endif

/*******************************************************************************
**    constants and macros
*******************************************************************************/
#if( k_TSTHOOKS_USED == TRUE)
  /* hook functions for testing */
  #define APH_INIT()                               APH_Init()
  #define APH_SEND_HOOK(pb_msg,b_len)              APH_Hook_Send_All(pb_msg,  \
                                                                     b_len)
  #define APH_CHECK_TXTS()                         APH_CheckTxTs()
  #define APH_SEND_HOOKTS_SEQ(e_mType,w_seqId)     APH_Hook_SndTsSeq(e_mType, \
                                                                     w_seqId)  
  #define APH_SEND_HOOKTS_VRS(e_mType)             APH_Hook_SndTsVers(e_mType)  
  #define APH_RECV_HOOK(pb_rxMsg,pw_len,dw_ipAddr) APH_Hook_Recv(pb_rxMsg,    \
                                                                 pw_len,      \
                                                                 dw_ipAddr)
  #define APH_RXTS_HOOK(ps_ts,w_sId,ps_pId,w_if,e_mT) APH_Hook_RxTs(ps_ts,    \
                                                                    w_sId,    \
                                                                    ps_pId,   \
                                                                    w_if,     \
                                                                    e_mT)
  #define APH_TXTS_HOOK(ps_ts,w_sId,w_if,e_mT)     APH_Hook_TxTs(ps_ts,       \
                                                                 w_sId,       \
                                                                 w_if,        \
                                                                 e_mT)
#else
  /* empty function definitions for the release version */
  #define APH_INIT() 
  #define APH_SEND_HOOK(pb_msg,b_len)              (pb_msg)
  #define APH_CHECK_TXTS()                         (FALSE)
  #define APH_SEND_HOOKTS_SEQ(e_mType,w_seqId)     (w_seqId)
  #define APH_SEND_HOOKTS_VRS(e_mType)             (k_PTP_VERSION)
  #define APH_RECV_HOOK(pb_rxMsg,pw_len,dw_ipAddr) (TRUE)
  #define APH_RXTS_HOOK(a,b,c,d,e)
  #define APH_TXTS_HOOK(a,b,c,d) 
#endif

/*******************************************************************************
**    data types
*******************************************************************************/

/************************************************************************/
/** APH_t_SndHookCmd :
            Command for a send hook. This struct used to store hook commands.

*/
typedef struct
{
  BOOLEAN           o_hookIsSet;   /* determines an active 
                                      hook function */
  UINT8             b_cmd;         /* command (SET/ADD) */
  PTP_t_msgTypeEnum e_msgType;     /* message type to change 
                                      in hook function */
  UINT64            ddw_val;       /* value to set or add */
  UINT8             b_offs;        /* offset of variable to 
                                      change in message */
  UINT8             b_valLen;      /* size of variable in bytes */
  BOOLEAN           o_chgsSeqId;   /* flag determines, if seq-id 
                                      of hooked msgs are changed */
  BOOLEAN           o_chgsVers;    /* flag determines, if version of 
                                      hooked msgs are changed */
  PTP_t_PortId      s_srcPId;      /* port id to set */
  BOOLEAN           o_srcPIDToSet; /* flag determines, if pId is to
                                      be changed in hooked msgs */  
}APH_t_SndHookCmd;

/*******************************************************************************
**    global variables
*******************************************************************************/


/*******************************************************************************
**    function prototypes
*******************************************************************************/

/***********************************************************************
**  
** Function    : APH_Init
**  
** Description : Initializes the unit APH. 
**  
** Parameters  : -
**               
** Returnvalue : -
** 
** Remarks     : -
**  
***********************************************************************/
void APH_Init( void );


/***********************************************************************
**  
** Function    : APH_Hook_Send_All
**  
** Description : Calls all send-hook functions after each other.
**               First, the one-step-hook is called, then the 
**               hook-function to add or change defined values is called
**               and least the send-callback function is used.
**
**    See Also : APH_OneStepHook(), APH_Hook_Snd(), APH_Hook_Send()
**  
** Parameters  : pb_msg    (IN) - message in network format
**               w_len     (IN) - length of message
**               
** Returnvalue : -
** 
** Remarks     : -
**  
***********************************************************************/
const UINT8* APH_Hook_Send_All( const UINT8* pb_msg,UINT16 w_len);

/***********************************************************************
**  
** Function    : APH_Hook_Snd
**  
** Description : Send Hook that changes the message according to the 
**               registered hooks. 
**  
** Parameters  : e_msgType (IN) - message type of hooked message to send
**               pb_msg    (IN) - message in network format
**               w_len     (IN) - length of message
**               
** Returnvalue : -
** 
** Remarks     : -
**  
***********************************************************************/
UINT8* APH_Hook_Snd( PTP_t_msgTypeEnum e_msgType,
                     const UINT8* pb_msg,UINT16 w_len);

/***********************************************************************
**  
** Function    : APH_Hook_SndTsSeq
**  
** Description : Returns changed sequence id to get the correct timestamp.
**  
** Parameters  : e_msgType (IN) - message type of hooked message to send
**               w_seqId   (IN) - original sequence id
**               
** Returnvalue : sequence id before changing in hook
** 
** Remarks     : -
**  
***********************************************************************/
UINT16 APH_Hook_SndTsSeq( PTP_t_msgTypeEnum e_msgType,UINT16 w_seqId);

/***********************************************************************
**  
** Function    : APH_Hook_SndTsVers
**  
** Description : Returns changed version number to get the correct timestamp.
**  
** Parameters  : e_msgType (IN) - message type of hooked message to send
**               
** Returnvalue : version after changing in hook
** 
** Remarks     : -
**  
***********************************************************************/
UINT8 APH_Hook_SndTsVers( PTP_t_msgTypeEnum e_msgType);

/***********************************************************************
**  
** Function    : APH_Hook_RxTs
**  
** Description : Rx timestamp Hook that calls the registered rx timestamp
**               callback function and passes all values.
**
**    See Also : APH_RegRxTsHookCb(),APH_UnregRxTsHook()
**  
** Parameters  : ps_rxTs   (IN) - rx timestamp
**               w_seqId   (IN) - sequence id of timestamped message 
**               ps_pIdSnd (IN) - sender port id of of timestamped message
**               w_rxIfIdx (IN) - interface index where timestamped 
**                                message was received
**               e_mType   (IN) - message type of timestamped message
**               
** Returnvalue : -
** 
** Remarks     : -
**  
***********************************************************************/
void APH_Hook_RxTs(PTP_t_TmStmp      *ps_rxTs,
                   UINT16            w_seqId,
                   PTP_t_PortId      *ps_pIdSnd,
                   UINT16            w_rxIfIdx,
                   PTP_t_msgTypeEnum e_mType);

/***********************************************************************
**  
** Function    : APH_Hook_TxTs
**  
** Description : Tx timestamp Hook that calls the registered tx timestamp
**               callback function and passes all values.
**
**    See Also : APH_RegTxTsHookCb(),APH_UnregTxTsHook()
**  
** Parameters  : ps_txTs   (IN) - tx timestamp
**               w_seqId   (IN) - sequence id of timestamped message 
**               w_txIfIdx (IN) - interface index where timestamped 
**                                message was sent
**               e_mType   (IN) - message type of timestamped message
**               
** Returnvalue : -
** 
** Remarks     : -
**  
***********************************************************************/
void APH_Hook_TxTs(PTP_t_TmStmp      *ps_txTs,
                   UINT16            w_seqId,
                   UINT16            w_txIfIdx,
                   PTP_t_msgTypeEnum e_mType);

/***********************************************************************
**  
** Function    : APH_Hook_Recv
**  
** Description : Rx message hook function. Hooks the received message
**               to the registered callback function. If the callback
**               function returns FALSE, the received message gets destroyed.
**
**    See Also : APH_RegRecvMsgHook(),APH_UnregRecvMsgHook()
**  
** Parameters  : pb_rxMsg (IN/OUT) - Rx message to hook
**               pw_len   (IN/OUT) - length of hooked message
**               dw_ipAddr (IN)    - ip address of sender 
**               
** Returnvalue : TRUE  - message shall be forwarded to stack
**               FALSE - message shall be discarded 
** 
** Remarks     : -
**  
***********************************************************************/
BOOLEAN APH_Hook_Recv(UINT8* pb_rxMsg,UINT16 *pw_len,UINT32 dw_ipAddr);

/***********************************************************************
**  
** Function    : APH_Hook_Send
**  
** Description : Tx message hook function. Hooks the message to send
**               to the registered callback function. If the callback
**               function changes the length of the message to zero,
**               the message does not get sent
**
**    See Also : APH_RegSendMsgHook(),APH_UnregSendMsgHook()
**  
** Parameters  : pb_txMsg (IN/OUT) - tx message
**               pw_len   (IN/OUT) - lenght of tx message
**               
** Returnvalue : <> NULL - message shall get sent
**               == NULL - message shall get discarded 
** 
** Remarks     : -
**  
***********************************************************************/
const UINT8* APH_Hook_Send(const UINT8* pb_txMsg,UINT16 *pw_len);

/***********************************************************************
**  
** Function    : APH_CheckTxTs
**  
** Description : Returns value of destroy-flag. This flag determines, if
**               the last sent event message was destroyed by hook.
**  
** Parameters  : -
**               
** Returnvalue : value of flag
**
** Remarks     : -
**  
***********************************************************************/
BOOLEAN APH_CheckTxTs( void );

/***********************************************************************
**  
** Function    : APH_OneStepHook
**  
** Description : Changes the stack to behave like one-step.
**  
** Parameters  : pb_msg    (IN) - message in network format
**               w_len     (IN) - length of message
**               
** Returnvalue : -
** 
** Remarks     : -
**  
***********************************************************************/
const UINT8* APH_OneStepHook( const UINT8* pb_msg,UINT16 w_len);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* __APH_H__ */

