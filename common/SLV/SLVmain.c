/*************************************************************************
**    (C) 2008 - 2009 IXXAT Automation GmbH, all rights reserved
**************************************************************************
**
**       File: SLVmain.c 
**    Summary: The unit SLV encapsulates all slave activities of a PTP node.
**             The synchronization to the master is done by the task 
**             SLV_SyncTask and the slave to master delay calculation is 
**             done by the SLV_DelayTask independently. A PTP node only 
**             configures one channel maximal to PTP_SLAVE. Nevertheless
**             this Unit provides appropriate slave tasks for every channel 
**             of a node. This is a concession to the clarity of this 
**             implementation.
**
**    Version: 1.01.01
**
**************************************************************************
**************************************************************************
**
**  Functions: SLV_Init
**             SLV_GetErrStr
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
#include "SLV/SLV.h"
#include "SLV/SLVint.h"

/* not used for TCs without OC */
#if(( k_CLK_IS_OC == TRUE)||(k_CLK_IS_BC))
/*************************************************************************
**    global variables
*************************************************************************/

/* port Ids of all local ports */
PTP_t_PortId SLV_as_portId[k_NUM_IF];

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
** Function    : SLV_Init
**
** Description : Initializes the Unit SLV with the Port ids and the 
**               used network interfaces.
**
** Parameters  : ps_portId       (IN) - pointer to the port id array
**
** Returnvalue : -
**                      
** Remarks     : function is reentrant
**
***********************************************************************/
void SLV_Init(const PTP_t_PortId *ps_portId)
{
  UINT16 w_ifIdx;
  /* initialize port id array */
  for( w_ifIdx = 0 ; w_ifIdx < k_NUM_IF ; w_ifIdx++ )
  {
    SLV_as_portId[w_ifIdx] = ps_portId[w_ifIdx];
  } 
  return;
}

#ifdef ERR_STR
/*************************************************************************
**
** Function    : SLV_GetErrStr
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
const CHAR* SLV_GetErrStr(UINT32 dw_errNmb)
{
  const CHAR* pc_ret;
  
  /* resolve error number to error string */
  switch(dw_errNmb)
  {
    case SLV_k_MSG_SLD:   
    {
      pc_ret = "SLD State/Msg transition error";
      break;
    }
    case SLV_k_UNKWN_MSG:
    {
      pc_ret = "Unknown message error";
      break;
    }
    case SLV_k_MBOX_WAIT:
    {
      pc_ret = "CTL messagebox error - wait";
      break;
    }
    case SLV_k_MBOX_DSCD:
    {
      pc_ret = "CTL messagebox error - discard";
      break;
    }
    case SLV_k_MPOOL:
    {
      pc_ret = "Memory allocation error";
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

