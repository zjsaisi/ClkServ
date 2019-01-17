/*************************************************************************
**    (C) 2008 - 2009 IXXAT Automation GmbH, all rights reserved
**************************************************************************
**
**       File: MSTmain.c  
**    Summary: The unit MST encapsulates all master activities of a PTP node.
**
**    Version: 1.01.01
**
**************************************************************************
**************************************************************************
**
**  Functions: MST_Init
**             MST_GetErrStr
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
#include "MST/MST.h"
#include "MST/MSTint.h"

/* not used for TCs without OC */
#if(( k_CLK_IS_OC == TRUE)||(k_CLK_IS_BC))
/*************************************************************************
**    global variables
*************************************************************************/

/* the connected channel port ids */
PTP_t_PortId MST_as_pId[k_NUM_IF];

/*************************************************************************
**    static constants, types, macros, variables
*************************************************************************/

/*************************************************************************
**    static function-prototypes
*************************************************************************/

/*************************************************************************
**    global functions
*************************************************************************/

/***********************************************************************
**
** Function    : MST_Init
**
** Description : Initializes the Unit MST 
**              
** Parameters  : ps_portId       (IN) - pointer to the port id
**
** Returnvalue : -
**                      
** Remarks     : -
**
***********************************************************************/
void MST_Init(const PTP_t_PortId *ps_portId)
{
  UINT16 w_ifIdx; 
  /* initilize port id of all interfaces */
  for( w_ifIdx = 0 ; w_ifIdx < k_NUM_IF ; w_ifIdx++ )
  {
    MST_as_pId[w_ifIdx] = ps_portId[w_ifIdx];
  } 
  return;
}


#ifdef ERR_STR
/*************************************************************************
**
** Function    : MST_GetErrStr
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
const CHAR* MST_GetErrStr(UINT32 dw_errNmb)
{
  const CHAR *pc_ret;
                    
  /* handle errors */
  switch(dw_errNmb)
  {
    case MST_k_UNKWN_MSG:
    {
      pc_ret = "Internal message unknown error";
      break;
    }
    case MST_k_EVT_NDEF:
    {
      pc_ret = "Unknown event error";
      break;
    }
   case MST_k_TMO_ERR_MS:
    {
      pc_ret = "Timeout error in MSTsyn_Task";
      break;
    }
    case MST_k_TMO_ERR_MA:
    {
      pc_ret = "Timeout error in MSTannc_Task";
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
#endif

/*************************************************************************
**    static functions
*************************************************************************/
#endif /*#if(( k_CLK_IS_OC == TRUE)||(k_CLK_IS_BC))*/
