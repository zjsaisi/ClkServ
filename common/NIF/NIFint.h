/*************************************************************************
**    (C) 2008 - 2009 IXXAT Automation GmbH, all rights reserved
**************************************************************************
**
**       File: NIFint.h 
**    Summary: Declares the unit internal used  network interface functions.
**             Functions and types depend on platform and/or 
**             communication technology. 
**             
**    Version: 1.01.01
**
**************************************************************************
**************************************************************************
**
**  Functions: NIF_InitMsgTempls
**                       
**   Compiler: ANSI-C
**    Remarks:  
**
**************************************************************************
**    all rights reserved
*************************************************************************/

#ifndef __NIFINT_H__
#define __NIFINT_H__

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

/************************************************************************/
/** NIF_t_Padding
                 This struct is used to pad messages for receivers that 
                 need this.
**/
typedef struct 
{
  BOOLEAN o_pad;  /* padding is needed */
  UINT32  dw_tick; /* last time it was needed */
}NIF_t_Padding;

/*************************************************************************
**    global variables
*************************************************************************/
extern NIF_t_Padding as_padding[k_NUM_IF];
extern PTP_t_ClkDs const *NIF_ps_ClkDs;
extern UINT32 NIF_dw_secTicks;
/*************************************************************************
**    function prototypes
*************************************************************************/

  /***********************************************************************
**
** Function    : NIF_InitMsgTempls
**
** Description : Preinitializes the message templates 
**               with the non-volatile values. The message template for
**               sync messages is also used for delay_req messages.
**
** See Also    : NIF_SendSync(),NIF_SendDelayReq(),SendEvntMsg
**              
** Parameters  : -
**
** Returnvalue : -                                           
**                      
** Remarks     : function is not reentrant
**
***********************************************************************/
void NIF_InitMsgTempls(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* __NIFINT_H__ */


