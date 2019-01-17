/*************************************************************************
**    (C) 2008 - 2009 IXXAT Automation GmbH, all rights reserved
**************************************************************************
**
**       File: NIFmain.c 
**    Summary: Provides functions for the network communication interface. 
**             This file must be adapted, when changing the 
**             communication technology.
**
**    Version: 1.01.01
**
**************************************************************************
**************************************************************************
**
**  Functions: NIF_Init
**             NIF_GetIfAddrs
**
**   Compiler: ANSI-C
**    Remarks:  
**             especially Annex D 
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
#include "GOE/GOE.h"
#include "NIF/NIF.h"
#include "NIF/NIFint.h"
#include "PTP/PTP.h"

/*************************************************************************
**    global variables
*************************************************************************/
PTP_t_ClkDs const *NIF_ps_ClkDs;
NIF_t_Padding as_padding[k_NUM_IF] = {{FALSE,0}};
UINT32 NIF_dw_secTicks;
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
** Function    : NIF_Init
**
** Description : Initializes the Unit NIF
**              
** Parameters  : ps_clkDataSet   (IN) - read only pointer to clock data set 
**
**
** Returnvalue : -
**                      
** Remarks     : -
**
***********************************************************************/
void NIF_Init(PTP_t_ClkDs const *ps_clkDataSet)
{
  NIF_ps_ClkDs = ps_clkDataSet;
  /* Initialize message templates */
  NIF_InitMsgTempls();
  /* get a second in SIS ticks */
  NIF_dw_secTicks = PTP_getTimeIntv(0);
}

/*************************************************************************
**
** Function    : NIF_GetIfAddrs
**
** Description : This function gets the current addresses defined
**               by the interface index. Addresses are returned by 
**               setting the corresponding pointer of the parameter 
**               list to the buffer containing the address octets. 
**               The size determines the amount of octets to read 
**               and if necessary must be updated.
**
** See Also    : GOE_GetIfAddrs()
**
** Parameters  : pw_phyAddrLen  (OUT) - number of phyAddr octets
**               ppb_phyAddr    (OUT) - pointer to phyAddr
**               pw_portAddrLen (OUT) - number of portAdrr octets
**               ppb_portAddr   (OUT) - pointer to portAddr
**               w_ifIdx        (IN)  - interface index
**               
** Returnvalue : TRUE           - function succeeded
**               FALSE          - function failed
**
** Remarks     : -
**
*************************************************************************/
BOOLEAN NIF_GetIfAddrs(UINT16 *pw_phyAddrLen,
                       UINT8  **ppb_phyAddr,
                       UINT16 *pw_portAddrLen,
                       UINT8  **ppb_portAddr,
                       UINT16 w_ifIdx)
{
  /* call target specific function */
  return GOE_GetIfAddrs(pw_phyAddrLen,ppb_phyAddr,
                        pw_portAddrLen,ppb_portAddr,w_ifIdx);
}


/*************************************************************************
**    static functions
*************************************************************************/
