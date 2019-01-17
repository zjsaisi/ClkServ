/*************************************************************************
**    (C) 2008 - 2009 IXXAT Automation GmbH, all rights reserved
**************************************************************************
**
**       File: P2P.h  
**    Summary: Global header for the P2P unit. 
**             Defines all export functions of the unit P2P.
**
**    Version: 1.01.01
**
**************************************************************************
**************************************************************************
**
**  Functions: P2P_Init
**             P2P_SetPDelReqInt
**             P2P_GetErrStr
**             
**   Compiler: Ansi-C
**    Remarks:  
**
**************************************************************************
**    all rights reserved
*************************************************************************/

#ifndef __P2P_H__
#define __P2P_H__

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
void P2P_Init( const PTP_t_PortId *ps_portId,UINT32 dw_amntNetIf);

/***********************************************************************
**
** Function    : P2P_SetPDelReqInt
**
** Description : Set´s the exponent for calculating the peer delay request
**               interval. Interval = 2 ^ x  
**
** Parameters  : w_ifIdx       (IN) - communication interface index to set 
**                                    the new peer delay request interval
**               c_pDelReqIntv (IN) - exponent for calculating the delay 
**                                    request interval
**
** Returnvalue : -
**                      
** Remarks     : function is reentrant
**
***********************************************************************/
void P2P_SetPDelReqInt(UINT16 w_ifIdx,INT8 c_pDelReqIntv);

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
const CHAR* P2P_GetErrStr(UINT32 dw_errNmb);

#ifdef __cplusplus
}
#endif /* #ifdef __cplusplus */
#endif /* #ifndef __P2P_H__ */

