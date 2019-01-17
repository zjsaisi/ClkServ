/*************************************************************************
**    (C) 2008 - 2009 IXXAT Automation GmbH, all rights reserved
**************************************************************************
**
**       File: MSTdel.c  
**    Summary: The unit MST encapsulates all master activities of a PTP node.
**             The task MSTdel_Task is active in the PTP states PTP_PRE_MASTER
**             and PTP_MASTER. It answers the delay requests of all plugged 
**             slaves.
**
**    Version: 1.01.01
**
**************************************************************************
**************************************************************************
**
**  Functions: UCMdel_Task
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
#include "NIF/NIF.h"
#include "PTP/PTP.h"
#include "SIS/SIS.h"
#include "UCM/UCM.h"
#include "UCM/UCMint.h"

/* just for unicast enabled */
#if((k_UNICAST_CPBL == TRUE) && ((k_CLK_IS_OC) || (k_CLK_IS_BC)))
/*************************************************************************
**    global variables
*************************************************************************/

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
** Function    : UCMdel_Task
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
void UCMdel_Task( UINT16 w_hdl )
{
  PTP_t_PortAddr    *ps_pAddr;
  PTP_t_msgTypeEnum e_msgType;  
  PTP_t_MboxMsg     *ps_msg;
  UINT16            w_ifIdx;

  /* multitasking restart point - generates a 
     jump to the last BREAK at env */
  SIS_Continue(w_hdl); 

  while( TRUE )/*lint !e716 */
  {       
    /* got ready with message arrival? */
    while( ( ps_msg = (PTP_t_MboxMsg*)SIS_MboxGet()) != NULL )
    {
      /* delay mechanism service request */
      if( ps_msg->e_mType == e_IMSG_UC_SRV_REQ )
      {
        /* get port address of requestor */
        ps_pAddr = ps_msg->s_pntData.ps_pAddr;
#if( k_CLK_DEL_E2E == TRUE )
        e_msgType = e_MT_DELRESP;
#else /* ( k_CLK_DEL_E2E == TRUE ) */
        e_msgType = e_MT_PDELRESP;
#endif /* ( k_CLK_DEL_E2E == TRUE ) */
        /* get interface index */
        w_ifIdx = ps_msg->s_etc3.w_u16;
        /* search, if sync service is granted */
        if( UCM_IsSyncSrvGranted(ps_pAddr) == TRUE )
        {
          /* if sync service is granted, grant delay mechanism */
          UCM_RespUcRequest(ps_pAddr,w_ifIdx,0,ps_msg->s_etc1.dw_u32,
                            ps_msg->s_etc2.c_s8,e_msgType);          
        }
        else
        {
          /* if no sync service granted, delay mechanism grant makes no sense */
          /* refuse service request */
          UCM_RespUcRequest(ps_pAddr,
                            w_ifIdx,
                            0,
                            0,
                            ps_msg->s_etc2.c_s8,
                            e_msgType);
        }
        /* free allocated memory */
        SIS_Free(ps_pAddr);
      }      
      /* cancel unicast service request */
      else if( ps_msg->e_mType == e_IMSG_UC_CNCL )
      {
        /* get interface index */
        w_ifIdx = ps_msg->s_etc3.w_u16;
        /* nothing to do - just acknowledge */
        /* send acknowledge */
        UCM_RespUcCnclAck(ps_msg->s_pntData.ps_pAddr,w_ifIdx,0,e_MT_DELREQ);
        /* free allocated memory */
        SIS_Free(ps_msg->s_pntData.ps_pAddr);
        SIS_Free(ps_msg->s_pntExt1.ps_pId);
      }
      else
      {
        /* set error - message misplaces */
        PTP_SetError(k_UCM_ERR_ID,UCM_k_MSGERR,e_SEVC_NOTC);
      }      
      /* release mbox entry */
      SIS_MboxRelease();
    }   
    /* cooperative multitasking 
       set task to blocked, till it gets ready through an event,
       message or timeout  */
    SIS_Break(w_hdl,1); /*lint !e646 !e717*/ 
  }
  SIS_Return(w_hdl,0);/*lint !e744*/
}


/*************************************************************************
**    static functions
*************************************************************************/
#endif /* #if((k_UNICAST_CPBL == TRUE) && ((k_CLK_IS_OC) || (k_CLK_IS_BC)))) */
