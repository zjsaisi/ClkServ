/*************************************************************************
**    (C) 2008 - 2009 IXXAT Automation GmbH, all rights reserved
**************************************************************************
**
**       File: UCDint.h  
**    Summary: The unicast discovery task is responsible to establish
**             the connection to a unicast master. First announce messages
**             get requested from the master. If the BMC algorithm detected
**             a unicast master to be the best master, the sync and delay
**             messages have to be requested from this master.
**             This services are limited to some duration. After that, they
**             have to be re-requested from the master.
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

#ifndef __UCDINT_H__
#define __UCDINT_H__

#ifdef __cplusplus
extern "C"
{
#endif

/*************************************************************************
**    constants and macros
*************************************************************************/
/* errors */ 
#define UCD_k_MSG_ERR    (1u) /* misplaced task message */
#define UCD_k_MEM_ERR    (2u) /* SIS memory error */
#define UCD_k_MBOX_ERR   (3u) /* message box error */

/*************************************************************************
**    data types
*************************************************************************/

/************************************************************************/
/** UCD_t_srv 
          This struct defines a service to be requested from a unicast master.
**/
typedef struct
{
  PTP_t_msgTypeEnum e_msgType; /* message type of service */
  BOOLEAN           o_req;     /* flag determines, if service is requested */
  BOOLEAN           o_grnt;    /* flag determines, if service is granted */
  INT8              c_logIntv; /* log2 of interval to request */
  UINT16            w_seqId;   /* sequence if to send with request */
  UINT32            dw_prdEnd; /* granted period */  
}UCD_t_srv;

/************************************************************************/
/** UCD_t_srvReq 
          This struct defines all message services to be requested from a 
          unicast master.
**/
typedef struct
{
  PTP_t_PortAddr s_pAddr;   /* port address of potential master */
  PTP_t_PortId   s_pId;     /* port id of master */
  UCD_t_srv      s_srvAnnc; /* announce service */
  UCD_t_srv      s_srvSync; /* synchronization service (sync/flwUp) */
  UCD_t_srv      s_srvDel;  /* delay mechanism service (e2e/p2p) */
  UINT16         w_ifIdx;   /* communication interface index to use */
  void*          pv_next;   /* pointer to next struct */
  void*          pv_prev;   /* pointer to last struct */
}UCD_t_srvReq;
/*************************************************************************
**    global variables
*************************************************************************/

/*************************************************************************
**    function prototypes
*************************************************************************/

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* __UCDINT_H__ */

