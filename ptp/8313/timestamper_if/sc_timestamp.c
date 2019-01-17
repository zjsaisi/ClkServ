/*****************************************************************************
*                                                                            *
*   Copyright (C) 2009 Symmetricom, Inc., all rights reserved                *
*                                                                            *
*   This software contains proprietary and confidential information of       *
*   Symmetricom.                                                             *
*   It may not be reproduced, used, or disclosed to others for any purpose   *
*   without the written authorization of Symmetricom.                        *
*                                                                            *
******************************************************************************

FILE NAME    : sc_timestamp.c

AUTHOR       : Ken Ho

DESCRIPTION  : 

The functions in this file should be edited by the customer to allow packet
timestamp information to flow to PTP stack from the timestamping hardware.

        SC_InitTimeStamper()
        SC_GetRxTimeStamp()
        SC_GetTxTimeStamp()

Revision control header:
$Id: timestamper_if/sc_timestamp.c 1.10 2011/03/04 09:26:00PST Daniel Brown (dbrown) Exp  $

******************************************************************************
*/

/*--------------------------------------------------------------------------*/
/*                         INCLUDE FILES                                    */
/*--------------------------------------------------------------------------*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include "sc_types.h"
#include "sc_api.h"
#include "network_if/sc_network_if.h"
#include "log/sc_log.h"
#include "sc_timestamp.h"
#include "local_debug.h"

/*--------------------------------------------------------------------------*/
/*                              defines                                     */
/*--------------------------------------------------------------------------*/


/*--------------------------------------------------------------------------*/
/*                              types                                       */
/*--------------------------------------------------------------------------*/


/*--------------------------------------------------------------------------*/
/*                              data                                        */
/*--------------------------------------------------------------------------*/
// net handles to access the netsocket ioctl function
t_netHandle as_netHandle[k_NUM_IF];
int gRcvTsCount=0;
int gTmxTsCount=0;

/*--------------------------------------------------------------------------*/
/*                              functions                                   */
/*--------------------------------------------------------------------------*/

/*
----------------------------------------------------------------------------
                                SC_InitTimeStamper()

Description:
This function initializes the Natsemi timestamper and all required
system resources. The parameter of the function could be used to
determine the corresponding Ethernet interface.

Parameters:

Inputs:
        UINT16 w_ifIndex
        Network interface index

        None

Outputs:
        None

        
Return value:
         0: function succeeded
        -1: function failed

-----------------------------------------------------------------------------
*/
int SC_InitTimeStamper(UINT16 w_ifIndex)
{
  if( w_ifIndex >= k_NUM_IF )
  {
    return -1;
  }
#if 0
  as_netHandle[w_ifIndex].s_evtSock =
        SC_GetSockHdl(w_ifIndex, as_netHandle[w_ifIndex].s_if_data.ifr_name);
  /* flush timestamps */
  if( ioctl(as_netHandle[w_ifIndex].s_evtSock, PTP_CLEANUP_TIMESTAMP_BUFFERS,
            &as_netHandle[w_ifIndex].s_if_data) )
  {
    return -1;
  }
#endif

  return 0;
}

/*
----------------------------------------------------------------------------
                                SC_GetRxTimeStamp()

Description:
This function is called by the Softclient code.  This function requests a 
timestamp from the receive timestamp unit.  The function supplies the packet 
data required to identify the timestamp being requested.  The timestamp unit 
should remove the timestamp from the queue when replying.  In the event the 
timestamp is not found, an error is returned. 

Parameters:

Inputs:
        UINT16 w_ifIndex
        This value defines the interface port.

        t_ptpMsgTypeEnum e_messageType
        type of message

        UINT16 w_sequenceId
        sequency number of message

        t_PortIdType *ps_sendPortId
        Port ID of sender

Outputs:
        t_ptpTimeStampType *ps_timestamp
        pointer to the Tx timestamp

        
Return value:
         0: function succeeded
        -1: function failed

-----------------------------------------------------------------------------
*/
int SC_GetRxTimeStamp(UINT16             w_ifIndex,
                      t_ptpMsgTypeEnum   e_messageType,
                      UINT16             w_sequenceId,
                      t_PortIdType       *ps_sendPortId,
                      t_ptpTimeStampType *ps_timestamp)
{
  int ret, status;
  t_8313TimeFormat s_rxTs;
  UINT64 ddw_boardTime;
  UINT32 dw_keyID;
  UINT32 dw_ioctlReq;

  if( w_ifIndex >= k_NUM_IF )
  {
    return -1;
  }

  // get time from driver
  if(e_messageType == e_MTYPE_SYNC)
  {
    dw_ioctlReq = PTP_GET_RX_TIMESTAMP_SYNC;
  }
  else if(e_messageType == e_MTYPE_DELREQ)
  {
    dw_ioctlReq = PTP_GET_RX_TIMESTAMP_DEL_REQ;
  }
  else if(e_messageType == e_MTYPE_PDELREQ)
  {
    dw_ioctlReq = PTP_GET_RX_TIMESTAMP_PDELAY_REQ;
  }
  else if(e_messageType == e_MTYPE_PDELRESP)
  {
    dw_ioctlReq = PTP_GET_RX_TIMESTAMP_PDELAY_RESP;
  }

  dw_keyID = (UINT32)w_sequenceId;
  memcpy(&s_rxTs, &dw_keyID, sizeof(UINT32));

  // prepare structure
  as_netHandle[w_ifIndex].s_if_data.ifr_data = (void*)(&s_rxTs);

#if 0
  if( (status = ioctl(as_netHandle[w_ifIndex].s_evtSock,
                      dw_ioctlReq, &as_netHandle[w_ifIndex].s_if_data)) < 0 )
  {
    syslog(LOG_SYMM|LOG_DEBUG, "GetRxTimestamp failed status= %d", status);
    ret = -1;
  }
  else
  {
    ddw_boardTime = s_rxTs.dw_high;
    ddw_boardTime = (ddw_boardTime << 32 ) | s_rxTs.dw_low;
    ps_timestamp->u48_sec = ddw_boardTime / k_NSEC_IN_SEC;
    ps_timestamp->dw_nsec = ddw_boardTime -
                            (ps_timestamp->u48_sec * k_NSEC_IN_SEC);
    gRcvTsCount++;
	if ((s_localDebCfg.deb_mask & LOCAL_DEB_RXTS_TRACE_MASK) != 0)
	{
		printf("RX, %llu %d sn=%d\n",ps_timestamp->u48_sec,ps_timestamp->dw_nsec,w_sequenceId);
	}
    ret = 0;
  }
#endif
	ret = 0;

  return ret;
}

/*
----------------------------------------------------------------------------
                                SC_GetTxTimeStamp()

Description:
This function is called by the Softclient code.  This function requests a 
timestamp from the transmit timestamp unit.  The function supplies the packet 
data required to identify the timestamp being requested.  The timestamp unit 
should remove the timestamp from the queue when replying.  In the event the 
timestamp is not found, an error is returned. 

Parameters:

Inputs:
        UINT16 w_ifIndex
        This value defines the interface port.

        UINT16 w_sequenceId
        Sequence ID of timestamped message

Outputs:
        t_ptpTimeStampType *ps_timestamp
        pointer to the Tx timestamp

        
Return value:
         0: function succeeded
        -1: function failed

-----------------------------------------------------------------------------
*/
int SC_GetTxTimeStamp(UINT16             w_ifIndex,
                      UINT16             w_sequenceId,
                      t_ptpTimeStampType *ps_timestamp)
{
  int ret;
  int count = 0;
  t_8313TimeFormat s_txTs;
  UINT64 ddw_boardTime;
  /* BOOLEAN o_notFinished = TRUE; */
  static UINT64 ddw_prev_boardTime = 0;
  volatile int i, j=0;

  if( w_ifIndex >= k_NUM_IF )
  {
    return -1;
  }

  // prepare structure
  as_netHandle[w_ifIndex].s_if_data.ifr_data = (void*)(&s_txTs);

  /* the w_sequenceId didn't handle by freescale driver */
  /* we put some delay here for hardware processing, so when */
  /* we access tx timestamp, the ts already being available */

/* try 1000 times to get a different timestamp number */
#if 0
  while(1)
  {
//     for(i=0;i<1000;i++)
     for(i=0;i<10;i++)
     {
       j++;
     }

     // get time from driver
     if( ioctl(as_netHandle[w_ifIndex].s_evtSock, PTP_GET_TX_TIMESTAMP,
               &as_netHandle[w_ifIndex].s_if_data) )
     {
//       ret = -1;
       return -1;
     }
     else
     {
       ddw_boardTime = s_txTs.dw_high;
       ddw_boardTime = (ddw_boardTime << 32 ) | s_txTs.dw_low;
       if(ddw_prev_boardTime != ddw_boardTime)
       {
          ddw_prev_boardTime = ddw_boardTime;
          ps_timestamp->u48_sec = ddw_boardTime / k_NSEC_IN_SEC;
          ps_timestamp->dw_nsec = ddw_boardTime -
                                  (ps_timestamp->u48_sec * k_NSEC_IN_SEC);
          gTmxTsCount++;
      	 if ((s_localDebCfg.deb_mask & LOCAL_DEB_TXTS_TRACE_MASK) != 0)
      	 {
      		printf("TX, %llu %d sn=%d\n",ps_timestamp->u48_sec,ps_timestamp->dw_nsec,w_sequenceId);
      	 }
          return 0;
       }
       if(count++ > 1000)
       {
          return -1;
       }
     }
  }
#endif
	ret = 0;
  return ret;
}

