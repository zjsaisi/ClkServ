/*************************************************************************
**    (C) 2008 - 2009 IXXAT Automation GmbH, all rights reserved
**************************************************************************
**
**       File: CFGchk.c 
**    Summary: Must be used to check the configuration. Avoids compiling
**             of obviously erroneous configurations.
**
**    Version: 1.01.01
**
**************************************************************************
**************************************************************************
**
**  Functions:
**
**   Compiler: ANSI-C
**    Remarks: 
**
**************************************************************************
**    all rights reserved
*************************************************************************/

#include "target.h"
#include "PTP/PTPdef.h"
#include "GOE/GOE.h"

/*************************************************************************
**    compiler errors for wrong configurations
*************************************************************************/
/* check configuration combinations of amount of interfaces with clock type */
#if((k_NUM_IF != 1 )&&(k_IMPL_SLAVE_ONLY == TRUE))
    #error Slave-only nodes communicate only over one network interface!
#endif
#if ((k_NUM_IF == 1)&&(k_CLK_IS_BC == TRUE))
      #error A node with one comm. interface cannot be a boundary clock!    
#endif
#if ((k_NUM_IF == 1)&&(k_CLK_IS_TC == TRUE))
  #error A node with one comm. interface cannot be a transparent clock!
#endif
#if((k_NUM_IF > 1)&&(k_CLK_IS_OC == TRUE))
    #error Nodes with more than one interface are boundary or transparent clocks
#endif
/* check minimum amount of foreign master data sets */ 
#if( k_CLK_NMB_FRGN_RCD < 5 )
  #error A minimum of 5 foreign master data sets per port are mandatory !
#endif
/* check maximum amount of communication interfaces */
#if( k_NUM_IF > 65000 )
  #error Too many net interfaces
#endif
/* check, if a delay request mechanism is determined */
#if(( k_CLK_DEL_P2P == FALSE )&&( k_CLK_DEL_E2E == FALSE ))
   #error No path delay detection mechanism defined
#endif
/* ensure, that not both delay request mechanisms are determined */
#if(( k_CLK_DEL_P2P == TRUE )&&( k_CLK_DEL_E2E == TRUE ))
   #error Too many path delay detection mechanism defined
#endif
/* adapt size of network address field to network technology */
#if(k_MAX_NETW_ADDR_SZE < k_NETW_ADDR_LEN)
   #error Adapt size of define k_MAX_NETW_ADDR_SZE
          to amount of bytes in used network technology
#endif
/* transparent clocks cannot deal unicast */
#if((k_UNICAST_CPBL == TRUE)&&(k_CLK_IS_OC == FALSE)&&(k_CLK_IS_BC == FALSE))
   #error A Transparent Clock only implementation cannot use unicast. 
          Unicast communication ends at a OC or BC
#endif
/* check, if the amount of fault logs is defined > 0 */
#if(k_NUM_OF_FLT_REC < 1)
   #error At least one fault record should be maintained. 
          k_NUM_OF_FLT_REC must be > 0
#endif
