/*************************************************************************
**    (C) 2008 - 2009 IXXAT Automation GmbH, all rights reserved
**************************************************************************
**
**       File: TCFmain.c 
**
**    Summary: The transparent clock forward unit 
**             is responsible to forward all received smessages 
**             and correct for residence time and path delay.
**             This file implements initialization function and error
**             resolution.
**
**    Version: 1.01.01
**
**************************************************************************
**************************************************************************
**
**  Functions: TCF_Init
**             TCF_GetErrStr
**
**   Compiler: Ansi-C
**    Remarks:  
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
#if( k_CLK_IS_TC == TRUE )
#include "NIF/NIF.h"
#include "PTP/PTP.h"
#include "SIS/SIS.h"
#include "TCF/TCF.h"
#include "TCF/TCFint.h"

/*************************************************************************
**    global variables
*************************************************************************/
UINT32       TCF_dw_amntNetIf;          
UINT16       TCF_w_intv;                /* time interval */
/* pointer to callback function */
INT64 (*TCF_pf_cbGetActPDel)(UINT16);
/* the connected interface port ids */
PTP_t_PortId TCF_as_pId[k_NUM_IF];

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
** Function    : TCF_Init
**
** Description : Initializes the Unit TCF with the Port ids and the 
**               used network interfaces 
**
** Parameters  : ps_portId       (IN) - pointer to the port id´s
**               dw_amntNetIf    (IN) - number of initializable net interfaces
**               pf_cbGetActPDel (IN) - pointer to callback function to get
**                                      the actual peer delay of the regarding
**                                      interface.
**
** Returnvalue : -
**                      
** Remarks     : function is reentrant
**
***********************************************************************/
void TCF_Init( const PTP_t_PortId *ps_portId,
               UINT32 dw_amntNetIf,
               TCF_t_pfCbGetActPDel pf_cbGetActPDel)
{
  UINT16 w_ifIdx;
  /* initialize timer to low rate - 1 per second */
  TCF_w_intv    = PTP_getTimeIntv(0);
  /* start timer to clean correction tables */
  SIS_TimerStart(TCS_TSK,TCF_w_intv);
#if( k_CLK_DEL_E2E == TRUE )
  /* start delay request forward task */
  SIS_TimerStart(TCD_TSK,TCF_w_intv);
  /* start peer delay request forward task */
  SIS_TimerStart(TCP_TSK,TCF_w_intv);
#endif /* #if( k_CLK_DEL_E2E == TRUE ) */
  /* initialize number of network interfaces */
  TCF_dw_amntNetIf = dw_amntNetIf;  
  /* initialize callback function */
  TCF_pf_cbGetActPDel = pf_cbGetActPDel;
  /* initialize TC forward sync task */
  TCFsyn_Init();
#if( k_CLK_DEL_E2E == TRUE )
  /* initialize TC forward delay task */
  TCFdel_Init();
  /* initialize TC forward P2P task */
  TCFp2p_Init();
#endif /* #if( k_CLK_DEL_E2E == TRUE ) */
  for( w_ifIdx = 0 ; w_ifIdx < k_NUM_IF ; w_ifIdx++ )
  {
    TCF_as_pId[w_ifIdx] = ps_portId[w_ifIdx];
  }
}


#ifdef ERR_STR
/*************************************************************************
**
** Function    : TCF_GetErrStr
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
const CHAR* TCF_GetErrStr(UINT32 dw_errNmb)
{
  const CHAR *pc_ret;
  /* return according error string */
  switch(dw_errNmb)
  {
    case TCF_k_ERR_MPOOL_SYN:
    {
      pc_ret = "Too few memory for save sync data";
      break;
    }
    case TCF_k_ERR_MPOOL_DEL:
    {
      pc_ret = "Too few memory for save delay request data";
      break;
    }
    case TCF_k_MSG_ERR:
    {
      pc_ret = "Unknown message";
      break;
    }
    case TCF_k_MSNG_FLWUP:
    {
      pc_ret = "Missing follow up message";
      break;
    }
    case TCF_k_SEND_ERR:
    {
      pc_ret = "Send error";
      break;
    }
    default:
    {
      pc_ret = "unknown error";
      break;
    }
  }
  return pc_ret;
}
#endif /* #ifdef ERR_STR */

/*************************************************************************
**    static functions
*************************************************************************/

#endif /* #if( k_CLK_IS_TC == TRUE ) */





