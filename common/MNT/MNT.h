/*************************************************************************
**    (C) 2008 - 2009 IXXAT Automation GmbH, all rights reserved
**************************************************************************
**
**       File: MNT.h 
**    Summary: The unit MNT handles all management messages 
**             and reacts to them as specified in the
**             standard IEEE 1588.
**
**    Version: 1.01.01
**
**************************************************************************
**************************************************************************
**
**  Functions: MNT_Init
**             MNT_Task
**             MNT_GetPTPText
**             MNT_PreInitClear
**             MNT_LockApi
**             MNT_UnlockApi
**             MNT_GetErrStr
**
**   Compiler: Ansi-C
**    Remarks: 
**
**************************************************************************
**    all rights reserved
*************************************************************************/

#ifndef __MNT_H__
#define __MNT_H__

#ifdef __cplusplus
extern "C"
{
#endif

/*************************************************************************
**    constants and macros
*************************************************************************/

/*************************************************************************
**    data types
*************************************************************************/

/*************************************************************************
**    global variables
*************************************************************************/

/*************************************************************************
**    function prototypes
*************************************************************************/


/***********************************************************************
** 
** Function    : MNT_Init
**
** Description : This function initializes the unit MNT. Therefore
**               each port id is saved within an array to get simple
**               access to the addresses. The size of the management
**               information table is determined to get the current
**               amount of defined management messages and the end 
**               of the table. Furthermore the user description is
**               generated, witch is configurable through the mnt. 
**
** Parameters  : ps_portId    (IN) - pointer to the channel port id array
**               dw_amntNetIf (IN) - number of initializable net interfaces
**               ps_ucMstTbl  (IN) - pointer to unicast master table
**               ps_clkDs     (IN) - pointer to the clock data set
**
** Returnvalue : -
**
** Remarks     : -
**
***********************************************************************/
void MNT_Init(const PTP_t_PortId         *ps_portId,
              UINT32                     dw_amntNetIf,
              const PTP_t_PortAddrQryTbl *ps_ucMstTbl,
              const PTP_t_ClkDs          *ps_clkDs);
                 

/***********************************************************************
**
** Function    : MNT_Task
**
** Description : The management task is responsible for handling all 
**               received management messages and dispatch it to the
**               corresponding functionality. 
**              
** Parameters  : w_hdl        (IN) - the SIS-task-handle
**
** Returnvalue : -
**                      
** Remarks     : -
**
***********************************************************************/
void MNT_Task( UINT16 w_hdl );

/*************************************************************************
**
** Function    : MNT_GetPTPText
**
** Description : Creates a PTP text according an error string for
**               the generation of management status error messages.
**
** Parameters  : pc_string  (IN)  - pointer to error string
**               ps_retText (OUT) - corrsponding error PTP text
**               
** Returnvalue : -       
** 
** Remarks     : -
**  
***********************************************************************/
void MNT_GetPTPText( const CHAR* pc_string,
                     PTP_t_Text* ps_retText );

/***********************************************************************
**
** Function    : MNT_PreInitClear
**
** Description : This is a special function only called out of the
**               CTL unit to generate a RESPONSE or ACKNOWLEDGE 
**               shortly before re-initialization of the stack is 
**               done. Thus a response to the requestor is made. 
**               Furthermore all API requests will be cancelled 
**               because some could wait for a timeout.
**
** Parameters  : ps_msg    (IN)  - the mbox item
**               ps_mntMsg (IN)  - mbox data, in this case MNT msg
**
** Returnvalue : -
**
** Remarks     : -
**
***********************************************************************/
void MNT_PreInitClear(       PTP_t_MboxMsg      *ps_msg,
                       const NIF_t_PTPV2_MntMsg *ps_mntMsg );

/*************************************************************************
**
** Function    : MNT_LockApi
**
** Description : This function sets the BOOLEAN flag o_initialize to 
**               TRUE to indicate, that the stack is in re-
**               initialization. There could be a problem using the API
**               and thus we prohibit the API to request a management 
**               message during this time. As soon as the re-
**               initialization is done, the flag must be reset to 
**               FALSE using MNT_UnlockApi().
**
** See Also    : MNT_UnlockApi()
**
** Parameters  : -
**               
** Returnvalue : -
** 
** Remarks     : -
**  
***********************************************************************/
#ifdef MNT_API
  void MNT_LockApi( void );
#endif /* #ifdef MNT_API */

/*************************************************************************
**
** Function    : MNT_UnlockApi
**
** Description : This function resets the BOOLEAN flag o_initialize 
**               to FALSE to indicate, that the stack is not in re-
**               initialization any more. There could be a problem 
**               using the API and thus we prohibit the API to request 
**               a management message during this time.
**
** See Also    : MNT_LockApi()
**
** Parameters  : -
**               
** Returnvalue : -
** 
** Remarks     : -
**  
***********************************************************************/
#ifdef MNT_API
  void MNT_UnlockApi( void );
#endif /* #ifdef MNT_API */

/*************************************************************************
**
** Function    : MNT_GetErrStr
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
const CHAR* MNT_GetErrStr( UINT32 dw_errNmb );

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* __MNT_H__ */
