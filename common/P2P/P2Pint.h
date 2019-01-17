/*************************************************************************
**    (C) 2008 - 2009 IXXAT Automation GmbH, all rights reserved
**************************************************************************
**
**       File: P2Pint.h  
**    Summary: Internal header of the P2P unit. 
**             Defines internal used functions and data types.
**
**    Version: 1.01.01
**
**************************************************************************
**************************************************************************
**
**  Functions: P2P_PDelTask
**             P2P_PrspTask
**
**   Compiler: Ansi-C
**    Remarks:  
**
**************************************************************************
**    all rights reserved
*************************************************************************/

#ifndef __P2PINT_H__
#define __P2PINT_H__

#ifdef __cplusplus
extern "C"
{
#endif

/*************************************************************************
**    constants and macros
*************************************************************************/
#define P2P_k_UNKWN_MSG       (0u) /* unknown internal message */
#define P2P_k_ERR_MEM         (1u) /* error getting memory */
#define P2P_k_ERR_MBOX        (2u) /* not possible to send to mbox control */

/*************************************************************************
**    data types
*************************************************************************/
/***
   P2P_t_pDelReq:
        Type is used to store tx timestamps and the regarding information
        of received peer delay messages.
**/
typedef struct
{
  PTP_t_TmStmp  s_txTsReq; /* tx timestamp of peer delay request at own node */
  PTP_t_TmStmp  s_rxTsReq; /* rx timestamp of peer delay request at forgn node*/
  PTP_t_TmStmp  s_rxTsResp;/* rx timestamp of peer delay response at own node */
  UINT16        w_seqId;   /* sequence id to identify */
  INT64         ll_corr;   /* added correction and turnaround time */
  PTP_t_PortId s_pId;      /* port ID of request sender */
}P2P_t_pDelReq;

/*************************************************************************
**    global variables
*************************************************************************/
extern UINT32 P2P_dw_amntNetIf;
/* port Ids of all local ports */
extern PTP_t_PortId P2P_as_portId[k_NUM_IF]; 

/*************************************************************************
**    function prototypes
*************************************************************************/

/***********************************************************************
**
** Function    : P2P_PDelTask
**
** Description : The peer to peer path delay task is responsible for 
**               issuing peer delay request messages and receiving
**               peer delay response adn peer delay response follow up 
**               messages. The task calculates the actual peer delay
**               to the next node and sends it to the control task.
**              
** Parameters  : w_hdl        (IN) - the SIS-task-handle
**
** Returnvalue : -
**                      
** Remarks     : function is reentrant
**
***********************************************************************/
void P2P_PDelTask(UINT16 w_hdl);

/***********************************************************************
**
** Function    : P2P_PrspTask
**
** Description : The master task is responsible for sending sync messages
**               to the connected net segment. Also it has to answer 
**               received delay request messages.
**              
** Parameters  : w_hdl        (IN) - the SIS-task-handle
**
** Returnvalue : -
**                      
** Remarks     : Function is reentrant.
**
***********************************************************************/
void P2P_PrspTask( UINT16 w_hdl );

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* __ */

