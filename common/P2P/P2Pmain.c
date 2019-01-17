/*************************************************************************
**    (C) 2008 - 2009 IXXAT Automation GmbH, all rights reserved
**************************************************************************
**
**       File: PTPmain.c  
**    Summary: Peer Delay main unit.
**             Implements the peer delay mechanism. 
**             
**    Version: 1.01.01
**
**************************************************************************
**************************************************************************
**
**  Functions: P2P_Init
**             P2P_GetErrStr
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
#include "P2P/P2P.h"
#include "P2P/P2Pint.h"

/*************************************************************************
**    global variables
*************************************************************************/

UINT32 P2P_dw_amntNetIf;
PTP_t_PortId P2P_as_portId[k_NUM_IF];

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
** Function    : P2P_Init
**
** Description : Initializes the Unit P2P with the Port ids and the 
**               used network interfaces 
**
** Parameters  : ps_portId       (IN) - pointer to the port id array
**               dw_amntNetIf    (IN) - number of initializable net interfaces
**
** Returnvalue : -
**                      
** Remarks     : function is reentrant
**
***********************************************************************/
void P2P_Init( const PTP_t_PortId *ps_portId,UINT32 dw_amntNetIf)
{
  UINT16 w_ifIdx;
  /* initialize port id array */
  for( w_ifIdx = 0 ; w_ifIdx < k_NUM_IF ; w_ifIdx++ )
  {
    P2P_as_portId[w_ifIdx] = ps_portId[w_ifIdx];
  } 
  /* initialize number of network interfaces */
  P2P_dw_amntNetIf = dw_amntNetIf;  
}

#ifdef ERR_STR
/***********************************************************************
**  
** Function    : P2P_GetErrStr
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
const CHAR* P2P_GetErrStr(UINT32 dw_errNmb)
{
  const CHAR* pc_ret;
  /* return according error string */
  switch(dw_errNmb)
  {
    case P2P_k_UNKWN_MSG:
    {
      pc_ret ="Unknown internal message";
      break;
    }
    case P2P_k_ERR_MEM:
    {
      pc_ret ="Error getting memory";
      break;
    }
    case P2P_k_ERR_MBOX:
    {
      pc_ret ="Not possible to send to mbox control";
      break;
    }
    default:
    {
      pc_ret ="unknown error";
      break;
    }
  }
  return pc_ret;
}
#endif

/*************************************************************************
**    static functions
*************************************************************************/





