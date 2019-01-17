/*************************************************************************
**    (C) 2008 - 2009 IXXAT Automation GmbH, all rights reserved
**************************************************************************
**
**       File: APH.h  
**    Summary: Hook Unit APH
**             The hook unit is an additional code unit, normally not used by
**             the standard stack. The hooks are used for testing. 
**             They provide the possibility to change parts of the network 
**             packets to test the reaction of other nodes and/or the 
**             complete network. 
**             
**    Version: 1.01.01
**
**************************************************************************
**************************************************************************
**             
**  Functions: APH_RegSndHook
**             APH_UnregSndHook
**             APH_UnregAllSndHook
**             APH_RegRxTsHookCb
**             APH_UnregRxTsHook
**             APH_RegTxTsHookCb
**             APH_UnregTxTsHook
**             APH_RegRecvMsgHook
**             APH_UnregRecvMsgHook
**             APH_RegSendMsgHook
**             APH_UnregSendMsgHook
**             APH_SetToOneStep
**             APH_ResetToTwoStep
**             
**   Compiler: Ansi-C
**    Remarks: 
**
**************************************************************************
**    all rights reserved
*************************************************************************/

#ifndef __APH_H__
#define __APH_H__

#ifdef __cplusplus
extern "C"
{
#endif

/*******************************************************************************
**    constants and macros
*******************************************************************************/

#define k_MAX_SND_HOOK_CMDS  (10) /* maximum hook commands at one time */
#define k_SNDHOOK_SET        (0)  /* 'set value' command for send hooks */
#define k_SNDHOOK_ADD        (1)  /* 'add value' command for send hooks */

/*******************************************************************************
**    data types
*******************************************************************************/

/* rx timestamp hook callback function data type */
typedef void (k_CB_CALL_CONV *APH_t_pfCbRxTs)(PTP_t_TmStmp      *ps_rxTs,
                                              UINT16            w_seqId,
                                              PTP_t_PortId      *ps_pIdSnd,
                                              UINT16            w_rxIfIdx,
                                              PTP_t_msgTypeEnum e_mType);
/* tx timestamp hook callback function data type */
typedef void (k_CB_CALL_CONV *APH_t_pfCbTxTs)(PTP_t_TmStmp      *ps_txTs,
                                              UINT16            w_seqId,
                                              UINT16            w_txIfIdx,
                                              PTP_t_msgTypeEnum e_mType);
/* rx message hook callback function data type */
typedef BOOLEAN (k_CB_CALL_CONV *APH_t_pfCbRxMsg)(UINT8 *pb_rxMsg,
                                                  UINT16 *pw_msgLen,
                                                  UINT32 dw_ipAddr);
/* tx message hook callback function data type */
typedef BOOLEAN (k_CB_CALL_CONV *APH_t_pfCbTxMsg)(UINT8 *pb_txMsg,
                                                  UINT16 *pw_msgLen);


/*******************************************************************************
**    global variables
*******************************************************************************/


/*******************************************************************************
**    function prototypes
*******************************************************************************/

/***********************************************************************
**  
** Function    : APH_RegSndHook
**  
** Description : Registers a send hook. The returned value is a handle
**               to the hook which can be used to stop it. It is possible 
**               to register more than one hook at one time. Use the 
**               unregister function to disable the registered hook.  
**               To unregister use the returned handle.
**
**    See Also : APH_UnregSndHook()
**  
** Parameters  : b_cmd     (IN) - command to be done in hook 
**                                (k_SNDHOOK_SET,k_SNDHOOK_ADD)
**               e_msgType (IN) - message type to be changed in hook 
**                                (PTP_t_msgTypeEnum)
**               ddw_val   (IN) - value to set or add in the hook function
**               ps_srcPId (IN) - value of source port Id to set (shall be
**                                NULL if port ID shall not be changed)
**               b_offs    (IN) - offset of value to set or add
**               b_valLen  (IN) - size of value to add or set
**               
** Returnvalue :  < 0 - function failed
**               >= 0 - function succeeded - number of hook
** 
** Remarks     : -
**  
***********************************************************************/
INT32 APH_RegSndHook( UINT8 b_cmd,
                      PTP_t_msgTypeEnum e_msgType,
                      UINT64 ddw_val,
                      const PTP_t_PortId *ps_srcPId,
                      UINT8 b_offs,
                      UINT8 b_valLen);

/***********************************************************************
**  
** Function    : APH_UnregSndHook
**  
** Description : Unregisters a send hook. To unregister, use the 
**               returned handle of the register function.
**
**    See Also : APH_RegSndHook()
**  
** Parameters  : dw_hdl (IN) - handle of send hook to unregister
**               
** Returnvalue : TRUE        - function succeeded
**               FALSE       - function failed
** 
** Remarks     : -
**  
***********************************************************************/
BOOLEAN APH_UnregSndHook(UINT32 dw_hdl);

/***********************************************************************
**  
** Function    : APH_UnregAllSndHook
**  
** Description : Unregisters all send hooks. 
**
**    See Also : APH_RegSndHook(),APH_UnregSndHook()
**
** Parameters  : -
**               
** Returnvalue : -
** 
** Remarks     : -
**  
***********************************************************************/
void APH_UnregAllSndHook( void );

/***********************************************************************
**  
** Function    : APH_RegRxTsHookCb
**  
** Description : Registers a callback function that hooks the 
**               Rx timestamps and the corresponding data of the timestamped
**               packet to the application. 
**               Only one callback function can be registered at one time.
**
**    See Also : APH_UnregRxTsHook()
**  
** Parameters  : pf_cbRxTs (IN) - Rx timestamp callback function to register
**               
** Returnvalue : -
** 
** Remarks     : -
**  
***********************************************************************/
void APH_RegRxTsHookCb( void (k_CB_CALL_CONV *pf_cbRxTs)
                                           (PTP_t_TmStmp      *ps_rxTs,
                                            UINT16            w_seqId,
                                            PTP_t_PortId      *ps_pIdSnd,
                                            UINT16            w_rxIfIdx,
                                            PTP_t_msgTypeEnum e_mType));

/***********************************************************************
**  
** Function    : APH_UnregRxTsHook
**  
** Description : Unregisters the Rx timestamp hook function.
**    
**    See Also : APH_RegRxTsHookCb()
**  
** Parameters  : -
**               
** Returnvalue : -
** 
** Remarks     : -
**  
***********************************************************************/
void APH_UnregRxTsHook( void );

/***********************************************************************
**  
** Function    : APH_RegTxTsHookCb
**  
** Description : Registers a callback function that hooks the 
**               Tx timestamps and the corresponding data of the timestamped
**               packet to the application. 
**               Only one callback function can be registered at one time.
**
**    See Also : APH_UnregTxTsHook()
**  
** Parameters  : pf_cbTxTs (IN) - Tx timestamp callback function to register
**               
** Returnvalue : -
** 
** Remarks     : -
**  
***********************************************************************/
void APH_RegTxTsHookCb( void (k_CB_CALL_CONV *pf_cbTxTs)
                                            (PTP_t_TmStmp      *ps_txTs,
                                             UINT16            w_seqId,
                                             UINT16            w_txIfIdx,
                                             PTP_t_msgTypeEnum e_mType));

/***********************************************************************
**  
** Function    : APH_UnregTxTsHook
**  
** Description : Unregisters the Tx timestamp hook function.
**    
**    See Also : APH_RegTxTsHookCb()
**  
** Parameters  : -
**               
** Returnvalue : -
** 
** Remarks     : -
**  
***********************************************************************/
void APH_UnregTxTsHook( void );

/***********************************************************************
**  
** Function    : APH_RegRecvMsgHook
**  
** Description : Registers a callback function that hooks the 
**               Rx Messages to the application. 
**               Only one callback function can be registered.
**               It is possible to 
**                 - read the received message 
**                 - change the received message content
**                 - change the received message length
**                 - destroy the reiceived message (by returning FALSE)
**               in the callback function.
**
**    See Also : APH_UnregRecvMsgHook()
**  
** Parameters  : pf_cbRxMsg (IN) - Rx message callback function to register
**               
** Returnvalue : -
** 
** Remarks     : -
**  
***********************************************************************/
void APH_RegRecvMsgHook( BOOLEAN (k_CB_CALL_CONV *pf_cbRxMsg)
                                                    (UINT8 *pb_rxMsg,
                                                     UINT16 *pw_msgLen,
                                                     UINT32 dw_ipAddr));

/***********************************************************************
**  
** Function    : APH_UnregRecvMsgHook
**  
** Description : Unregisters the callback function that hooks the 
**               Rx Messages to the application. 
**
**    See Also : APH_RegRecvMsgHook()
**  
** Parameters  : -
**               
** Returnvalue : -
** 
** Remarks     : -
**  
***********************************************************************/
void APH_UnregRecvMsgHook( void );

/***********************************************************************
**  
** Function    : APH_RegSendMsgHook
**  
** Description : Registers a callback function that hooks the 
**               Tx Messages to the application. 
**               Only one callback function can be registered.
**               It is possible to 
**                 - read the message 
**                 - change the message content
**                 - change the message length
**                 - destroy the tx message (by returning FALSE)
**               in the callback function.
**               ATTENTION: The sequence id of event messages shall not 
**               be changed because it is not possible to get the correct 
**               tx timestamps afterwards. 
**               Use function APH_RegSndHook to do that.
**
**    See Also : APH_UnregSendMsgHook()
**  
** Parameters  : pf_cbTxMsg (IN) - Tx message callback function to register
**               
** Returnvalue : -
** 
** Remarks     : -
**  
***********************************************************************/
void APH_RegSendMsgHook( BOOLEAN (k_CB_CALL_CONV *pf_cbTxMsg)
                                                    (UINT8 *pb_txMsg,
                                                     UINT16 *pw_msgLen));

/***********************************************************************
**  
** Function    : APH_UnregSendMsgHook
**  
** Description : Unregisters the callback function that hooks the 
**               Tx Messages to the application. 
**
**    See Also : APH_RegSendMsgHook()
**  
** Parameters  : -
**               
** Returnvalue : -
** 
** Remarks     : -
**  
***********************************************************************/
void APH_UnregSendMsgHook( void );

/***********************************************************************
**  
** Function    : APH_SetToOneStep
**  
** Description : Changes the clock to a one-step clock. All other send
**               hooks are executed after changing the messages to 
**               one-step.
**
**    See Also : APH_ResetToTwoStep()
**  
** Parameters  : -
**               
** Returnvalue : -
** 
** Remarks     : -
**  
***********************************************************************/
void APH_SetToOneStep( void );

/***********************************************************************
**  
** Function    : APH_ResetToTwoStep
**  
** Description : Resets the Clock to the normal two-step functionality
**
**    See Also : APH_SetToOneStep()
**  
** Parameters  : -
**               
** Returnvalue : -
** 
** Remarks     : -
**  
***********************************************************************/
void APH_ResetToTwoStep( void );

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* __APH_H__ */

