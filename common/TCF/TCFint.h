/*************************************************************************
**    (C) 2008 - 2009 IXXAT Automation GmbH, all rights reserved
**************************************************************************
**
**       File: TCFint.h  
**    Summary: Internal header of the TCF unit. 
**             Defines internal used functions and data types.
**
**    Version: 1.01.01
**
**************************************************************************
**************************************************************************
**
**  Functions: TCFsyn_Init
**             TCFdel_Init
**             TCFp2p_Init
**
**   Compiler: Ansi-C
**    Remarks:  
**
**************************************************************************
**    all rights reserved
*************************************************************************/

#ifndef __TCFINT_H__
#define __TCFINT_H__

#ifdef __cplusplus
extern "C"
{
#endif

/*************************************************************************
**    constants and macros
*************************************************************************/
#define TCF_k_ERR_MPOOL_SYN   (1u) /* too few memory for save sync data */
#define TCF_k_ERR_MPOOL_DEL   (2u) /* too few memory for save delay req. data */
#define TCF_k_MSG_ERR         (3u) /* unknown message */
#define TCF_k_MSNG_FLWUP      (4u) /* missing follow up message */
#define TCF_k_SEND_ERR        (5u) /* send error */

/*************************************************************************
**    data types
*************************************************************************/

/***
   TCF_t_TcCorSyn:
        Type is used to store rx timestamp and the regarding information
        of received sync and/or follow up messages.
**/
typedef struct
{
  PTP_t_TmStmp *ps_rxTs;    /* rx timestamp of sync message*/
  PTP_t_TmStmp as_txTs[k_NUM_IF]; /* tx timestamps of sync message */
  BOOLEAN      ao_sent[k_NUM_IF]; /* flag shows, if sync message was sent */
  UINT8        *pb_flwUp;  /* pointer to follow up message (raw) */ 
  UINT8        *pb_sync;   /* pointer to sync message (raw) */
  UINT16       w_seqId;    /* sequence id to identify */
  UINT32       dw_tickCnt;  /* SIS tick count at receive time */
  PTP_t_PortId s_sndPId;   /* sender PId to identify */  
}TCF_t_TcCorSyn;

/***
   TCF_t_TcCorDel:
        Type is used to store rx timestamp and the regarding information
        of received delay request and response messages.
**/
typedef struct
{
  PTP_t_TmStmp s_rxTs;   /* pointer to rx timestamp of delay request */
  PTP_t_TmStmp as_txTs[k_NUM_IF]; /* pointer to tx timestamps of delay request 
                                          of each interface */
  UINT16            w_seqId;    /* sequence id to identify */
  UINT32            dw_tickCnt;  /* SIS tick count at receive time */
  UINT16            w_rxIfIdx;  /* receiving interface of the delay request */
  PTP_t_PortId      s_sndPId;   /* sender PId to identify */  
}TCF_t_TcCorDel;

/***
   TCF_t_TcCorP2P:
        Type is used to store rx timestamp and the regarding information
        of received P2P event messages
**/
typedef struct
{
  PTP_t_TmStmp  s_rxTs;          /* pointer to rx timestamp of Pdelay request */
  PTP_t_TmStmp  as_txTs[k_NUM_IF]; /* pointer to tx timestamps of Pdelay request 
                                          of each interface */
  PTP_t_TmIntv  s_tiCorrPDRq;  /* correction value for Pdelay request */
  PTP_t_TmIntv  s_tiCorrPDRsp; /* correction value for Pdelay response */
  UINT16        w_seqId;    /* sequence id to identify */
  UINT32        dw_tickCnt; /* SIS tick count at receive time */
  UINT16        w_rxIfIdx;  /* receiving interface of the Pdelay request */
  PTP_t_PortId  s_reqPId;   /* requestor PId to identify */
  PTP_t_PortId  s_rspPId;   /* responder PId for networks that respond multiple */
  BOOLEAN       o_answer;   /* flag for first answer */
}TCF_t_TcCorP2P;

/*************************************************************************
**    global variables
*************************************************************************/
extern UINT32       TCF_dw_amntNetIf;          
/* time interval */
extern UINT16       TCF_w_intv;     
/* pointer to callback function */
extern INT64 (*TCF_pf_cbGetActPDel)(UINT16);
/* the connected interface port ids */
extern PTP_t_PortId TCF_as_pId[k_NUM_IF];

/*************************************************************************
**    function prototypes
*************************************************************************/
/*************************************************************************
**
** Function    : TCFsyn_Init
**
** Description : Initializes the Unit TCF sync.
**
** Parameters  : -
**
** Returnvalue : -
**                      
** Remarks     : function is reentrant
**
***********************************************************************/
void TCFsyn_Init( void );

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
void TCFdel_Init( void );

/*************************************************************************
**
** Function    : TCFp2p_Init
**
** Description : Initializes the Unit TCF peer to peer.
**
** Parameters  : -
**
** Returnvalue : -
**                      
** Remarks     : function is reentrant
**
***********************************************************************/
void TCFp2p_Init( void );


#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* #ifndef __TCFINT_H__ */

