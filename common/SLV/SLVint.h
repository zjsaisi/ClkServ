/*************************************************************************
**    (C) 2008 - 2009 IXXAT Automation GmbH, all rights reserved
**************************************************************************
**
**       File: SLVint.h 
**    Summary: The synchronization to the master is done by the task 
**             SLV_SyncTask and the slave to master delay calculation is 
**             done by the SLV_DelayTask independently. A PTP node only 
**             configures one channel maximal to PTP_SLAVE.
**             Defines unit global constants and macros of the unit slv.
**
**    Version: 1.01.01
**
**************************************************************************
**************************************************************************
**
**  Functions: 
**
**   Compiler: Ansi-C
**    Remarks:  
**
**************************************************************************
**    all rights reserved
*************************************************************************/

#ifndef __SLVINT_H__
#define __SLVINT_H__

#ifdef __cplusplus
extern "C"
{
#endif

/*************************************************************************
**    constants and macros
*************************************************************************/
/* errors */ 
#define SLV_k_MSG_SLD    (1u) /* there should be no message in this state */
#define SLV_k_UNKWN_MSG  (2u) /* message is unknown */
#define SLV_k_MBOX_WAIT  (3u) /* wait for putting msg into mbox */
#define SLV_k_MBOX_DSCD  (4u) /* message box was full - msg discarded */ 
#define SLV_k_MPOOL      (5u) /* can´t get a memory pool entry */

/* task states */
#define k_SLV_STOPPED     (0u)
#define k_SLV_OPERATIONAL (1u)

/*************************************************************************
**    data types
*************************************************************************/

/************************************************************************/
/** SLV_t_LastSyn
                 This struct stores data of the last received sync messages to
                 deal with, when the corresponding follow-up msg arrives
**/
typedef struct SLV_s_LastSyn
{
  UINT16       w_SeqId;    /* the last sync msg sequence id */
  UINT8        o_valid;    /* valid timestamp */
  PTP_t_PortId s_prntPID;  /* the current parent master  */
  BOOLEAN      o_prntIsUc; /* are messages sent by unicast or multicast? */
  PTP_t_TmStmp s_rxTs;     /* the Rx Timestamp of the last sync msg */
  INT64        ll_corrSyn; /* correction field of the sync message */
  BOOLEAN      o_unsynced; /* flag to not use the old sequence id */
}SLV_t_LastSyn;

/************************************************************************/
/** SLV_t_LastDlrq
                 This struct stores data of the last transitted delay 
                 request messages to deal with, when the corresponding 
v               delay response message arrives
**/
typedef struct SLV_s_LastDlrq
{
  UINT16         w_SeqId;    /* the last delay req sequence id */
  PTP_t_PortId   s_prntPid;  /* the parent pid */
  PTP_t_PortAddr s_prntPAddr;/* parent port address */
  BOOLEAN        o_prntIsUc; /* are messages sent by unicast or multicast? */
  PTP_t_TmStmp   s_TsTx;     /* the tx timestamp of the last del. req. msg */
  BOOLEAN        o_randIntv; /* Is interval random or directly after sync */
}SLV_t_LastDlrq;

/*************************************************************************
**    global variables
*************************************************************************/

/* port Ids of all local ports */
extern PTP_t_PortId SLV_as_portId[k_NUM_IF];

/*************************************************************************
**    function prototypes
*************************************************************************/

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* __SLVINT_H__ */

