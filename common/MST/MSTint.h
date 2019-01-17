/*************************************************************************
**    (C) 2008 - 2009 IXXAT Automation GmbH, all rights reserved
**************************************************************************
**
**       File: MSTint.h  
**    Summary: This unit defines the unit-internal variables, 
**             types and functions.
**             Error definitions of unit MST.
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

#ifndef __MSTINT_H__
#define __MSTINT_H__

#ifdef __cplusplus
extern "C"
{
#endif

/*************************************************************************
**    constants and macros
*************************************************************************/
/* errors */
#define MST_k_UNKWN_MSG    (1u) /* misplaced message */ 
#define MST_k_EVT_NDEF     (2u) /* event not defined */
#define MST_k_TMO_ERR_MS   (4u) /* state machine - wrong timeout-state
                                   combination in master sync task */
#define MST_k_TMO_ERR_MA   (5u) /* state machine - wrong timeout-state 
                                   combination in master announce task */
#define MST_k_MSG_ERR      (6u) /* message not applicable to task state */

/* task states  */
#define k_MST_STOPPED      (0u)
#define k_MST_PRE_MASTER   (1u)
#define k_MST_OPERATIONAL  (2u)

/*************************************************************************
**    data types
*************************************************************************/

/*************************************************************************
**    global variables
*************************************************************************/
/* the connected channel Port ids */
extern PTP_t_PortId MST_as_pId[k_NUM_IF];

/*************************************************************************
**    function prototypes
*************************************************************************/
#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* __MSTINT_H__ */

