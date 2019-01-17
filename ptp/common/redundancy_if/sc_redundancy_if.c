/******************************************************************************
$Id: redundancy_if/sc_redundancy_if.c 1.10 2011/10/07 16:35:29PDT German Alvarez (galvarez) Exp  $

Copyright (C) 2009, 2010, 2011 Symmetricom, Inc., all rights reserved
This software contains proprietary and confidential information of Symmetricom.                                                             *
It may not be reproduced, used, or disclosed to others for any purpose
without the written authorization of Symmetricom.


Original author: Ken Ho

Last revision by: $Author: German Alvarez (galvarez) $

File name: $Source: redundancy_if/sc_redundancy_if.c $

Functionality:

This file implements the minimal subset of the arbiter needed for
redundancy operation. The functionality implemented here is only used to be able to test
the redundancy API, but in no way represents how a real system will operate.
In particular the UDP messaging may not be robust by itself to convey messages and
commands between the peers, and there is no confirmation for the commands.

******************************************************************************
Log:
$Log: redundancy_if/sc_redundancy_if.c  $
Revision 1.10 2011/10/07 16:35:29PDT German Alvarez (galvarez) 
Added checks for redundancy channel not defined and peer IP not defined
Revision 1.9 2011/10/07 14:08:03PDT German Alvarez (galvarez) 
spelling fix on remarks
Revision 1.8 2011/10/06 17:24:21PDT German Alvarez (galvarez) 

Revision 1.7 2011/09/23 12:11:14PDT German Alvarez (galvarez) 
minor changes to the redundancy UI


******************************************************************************/

/*--------------------------------------------------------------------------*/
/*                         INCLUDE FILES                                    */
/*--------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>

#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h>
#include <signal.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <time.h>

#include "sc_api.h"
#include "local_debug.h"
#include "sc_chan.h"
#include "config_if/sc_readconfig.h"
#include "user_if/sc_ui.h"
#include "../../drivers/fpga_map.h"
#include "clock_if/sc_clock_if.h"
#include "../main.h"

#include "sc_redundancy_if.h"
/*--------------------------------------------------------------------------*/
/*                              defines                                     */
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/*                              types                                       */
/*--------------------------------------------------------------------------*/
typedef struct
{
  int dataInittialized;

  //inbound data
  int readPending;
  int inboundSocket, inboundSocketValid;
  struct sockaddr_in inboundSocketAddress;
  t_ptpTimeStampType inboundTimestamp;
  UINT32             peerEventMap;
  char               inboundCommand; // ' '; do nothing, 'a': go active, 's': go standby
  SC_t_activeStandbyEnum peerStatus;
  struct timespec localReceptionTime;
  int inboundPrintPkt;

  //outbound data
  int sendPending;
  int outboundSocket, outboundSocketValid;
  struct sockaddr_in outboundSocketAddress;
  t_ptpTimeStampType outboundTimestamp;
  char               outboundCommand; // ' '; do nothing, 'a': go active, 's': go standby
  int outboundPrintPkt;

  t_ptpTimeStampType    lastTimeStampToServo;
  INT32                 lastPhaseToServo;
  struct timespec       lastDataToServoTime;
} t_RedundancyCtl;

/*--------------------------------------------------------------------------*/
/*                              data                                        */
/*--------------------------------------------------------------------------*/
static t_RedundancyCtl gRedundancyCtl;

/*--------------------------------------------------------------------------*/
/*                              functions                                   */
/*--------------------------------------------------------------------------*/

static int initRedundancy();
static int transferPeerMeasurement();

/*
----------------------------------------------------------------------------
                                SC_InitRedIf()

Description:
   Opens redundancy interface port. Called by SCi2000 upon invoking SC_InitConfigComplete()

Parameters:

Inputs
        None

Outputs:
        None

Return value:
        TRUE:  function succeeded
        FALSE: function failed

-----------------------------------------------------------------------------
*/
int SC_InitRedIf(UINT8 b_ChanIndex)
{
#ifdef SC_RED_IF_DEBUG
   printf( "+SC_InitRedIf b_ChanIndex: %d\r\n", b_ChanIndex);
#endif

   initRedundancy();

   /* pass value back to SoftClock */
   return(0);
}

/*
----------------------------------------------------------------------------
                                SC_CloseRedIf()

Description:
This function is called by the SCi2000 to close the Redundancy interface. It 
should be used by the host to free up any resources to be reclaimed 
including but not limited to the Redundancy ports

Parameters:

Inputs
        b_ChanIndex - Channel Index

Outputs
        None
        
Return value:
        None

-----------------------------------------------------------------------------
*/
void SC_CloseRedIf(UINT8 b_ChanIndex)
{
#ifdef SC_RED_IF_DEBUG
   printf( "+SC_CloseRedIf b_ChanIndex: %d\r\n", b_ChanIndex);
#endif

   //close the sockets
   if(gRedundancyCtl.outboundSocketValid)
   {
     gRedundancyCtl.outboundSocketValid = 0;

     close(gRedundancyCtl.outboundSocket);
     gRedundancyCtl.sendPending = 0;
   }

   if(gRedundancyCtl.inboundSocketValid)
   {
     gRedundancyCtl.inboundSocketValid = 0;

     close(gRedundancyCtl.inboundSocket);
     gRedundancyCtl.readPending = 0;
   }

   memset(&gRedundancyCtl, 0, sizeof(gRedundancyCtl));
   gRedundancyCtl.dataInittialized = 0;
}




/*--------------------------------------------------------------------------
    int redundancyTask(struct timespec * currentTime)

Description:
This function should be called periodically (at least 10Hz) to maintain communication between the
redundant peers and transfer measurements to the SoftClock

The packet is an ACII string with values separated by spaces:

<Signature> <TAI seconds> <TAI nanoseconds> <Local event map> <status> <command>\n
Example:
SCRed1.0 1316116069 0 0 0 A

Return value:
  0          no error
  Any value different from zero is an error.

Parameters:

  Inputs
    struct timespec * currentTime
      current time as returned by CLOCK_MONOTONIC, used by the function to keep track of events

  Outputs
    None

Global variables affected and border effects:
  Read
    gRedundancyCtl
  Write
    gRedundancyCtl
--------------------------------------------------------------------------*/
int redundancyTask(const struct timespec * currentTime) //called by the 10Hz thread, start time is from CLOCK_MONOTONIC
{
  const int kComBufferSize = 1000; //has to be big enough for the message
  const char *kRedPktSignature = "SCRed1.0"; //No spaces allowed
  char netBuffer[kComBufferSize];
  struct sockaddr_in dataFrom;
  int itemsParsed;
  socklen_t dataFromLen = sizeof(dataFrom);
  UINT32 eventMap;
  BOOLEAN redEnabled;
  SC_t_activeStandbyEnum redStatus;

  transferPeerMeasurement();

  if(gRedundancyCtl.inboundSocketValid)
  {
    //see if we have received any UDP data...
    if(recvfrom(gRedundancyCtl.inboundSocket, netBuffer, kComBufferSize, MSG_DONTWAIT, (struct sockaddr *)&dataFrom, &dataFromLen) == -1)
    {
      if(errno != EAGAIN) //if EAGAIN -> no data available
      {
        perror("recvfrom");
      }
    }
    else
    { //we have data, check if it comes from our peer, otherwise discard.
      if(dataFrom.sin_addr.s_addr == gRedundancyCtl.outboundSocketAddress.sin_addr.s_addr)
      {
        //check if the first few bytes match the signature
        if(strncmp(kRedPktSignature, netBuffer, strlen(kRedPktSignature)) == 0)
        {
          if(gRedundancyCtl.inboundPrintPkt)
            printf("RX: |%s|\n", netBuffer);

          //parse the packet
          itemsParsed = sscanf(netBuffer, "%*s %llu %u %u %u %c", &gRedundancyCtl.inboundTimestamp.u48_sec,
              &gRedundancyCtl.inboundTimestamp.dw_nsec, &gRedundancyCtl.peerEventMap,
              &gRedundancyCtl.peerStatus, &gRedundancyCtl.inboundCommand);
          clock_gettime(CLOCK_MONOTONIC, &gRedundancyCtl.localReceptionTime);

          //printf("items: %d, time: %llu, eventMap: %u, status: %u, command: |%c|\n", itemsParsed, gRedundancyCtl.inboundTimestamp.u48_sec,
          //    gRedundancyCtl.peerEventMap, gRedundancyCtl.peerStatus, gRedundancyCtl.inboundCommand);

          //handle any command sent by the remote
          if(gRedundancyCtl.inboundCommand != ' ')
          {
            switch(gRedundancyCtl.inboundCommand)
            {
              case 'a': case 'A':
                SC_RedundantEnable(FALSE); //go to Active
                break;
              case 's': case 'S':
                SC_RedundantEnable(TRUE); //go to Standby
                break;
              default:
                printf("Invalid command received from peer.\n");
                break;
            }
            gRedundancyCtl.inboundCommand = ' '; //reset the command
          }
        }
      }
    }
  }

  if(gRedundancyCtl.outboundSocketValid && gRedundancyCtl.sendPending)
  {
    netBuffer[0] = 0;

    SC_GetEventMap(&eventMap);
    if(SC_GetRedundantStatus(&redEnabled, &redStatus) != 0)
    {
      redStatus = e_SC_ACTIVE; //redundancy not enabled, we have to be active!
    }

    snprintf(netBuffer, kComBufferSize, "%s %llu %u %u %u %c", kRedPktSignature, gRedundancyCtl.outboundTimestamp.u48_sec,
             gRedundancyCtl.outboundTimestamp.dw_nsec, eventMap, redStatus, gRedundancyCtl.outboundCommand);

    if(sendto(gRedundancyCtl.outboundSocket, netBuffer, strlen(netBuffer) + 1, MSG_DONTWAIT,
        (struct sockaddr *)&gRedundancyCtl.outboundSocketAddress, sizeof(gRedundancyCtl.outboundSocketAddress)) == -1)
    {
      perror("sendto");
    }
    if(gRedundancyCtl.outboundPrintPkt)
      printf("TX: |%s|\n", netBuffer);

    //clear the outbound command
    gRedundancyCtl.outboundCommand = ' ';

    gRedundancyCtl.sendPending = 0;
  }

  return 0;
}

/*--------------------------------------------------------------------------
    int setRedundantPeerAddress(char * peerAddress)

Description:
Set the IP address used to communicate with the other peer

Return value:
  0          no error
  Any value different from zero is an error.

Parameters:

  Inputs
    t_PortAddr * peerAddress
      IP address

  Outputs
    None

Global variables affected and border effects:
  Read

  Write

--------------------------------------------------------------------------*/
int setRedundantPeerAddress(const char * peerAddress)
{
  const int kRedundancyComPort = 8888;
  int err;

  initRedundancy();

  if(gRedundancyCtl.outboundSocketValid || gRedundancyCtl.inboundSocketValid)
  {
    //this can only be done once
    printf("Error: Peer IP address already set.\n");
    return -1;
  }

  //create the socket for outbound packets
  gRedundancyCtl.outboundSocket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if(gRedundancyCtl.outboundSocket == -1)
  {
    perror("outbound socket()");
    return -1;
  }
  memset(&gRedundancyCtl.outboundSocketAddress, 0, sizeof(gRedundancyCtl.outboundSocketAddress));
  gRedundancyCtl.outboundSocketAddress.sin_family = AF_INET;
  gRedundancyCtl.outboundSocketAddress.sin_port = htons(kRedundancyComPort);
  err=inet_pton(AF_INET, peerAddress, &gRedundancyCtl.outboundSocketAddress.sin_addr);
  if(err <= 0)
  {
    perror("inet_pton");

    //undo what we have done so far...
    close(gRedundancyCtl.outboundSocket);
    return -1;
  }

  //create the socket for inbound packets
  gRedundancyCtl.inboundSocket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if(gRedundancyCtl.inboundSocket == -1)
  {
    perror("inbound socket()");

    //undo what we have done so far...
    close(gRedundancyCtl.outboundSocket);
    return -1;
  }
  memset(&gRedundancyCtl.inboundSocketAddress, 0, sizeof(gRedundancyCtl.inboundSocketAddress));
  gRedundancyCtl.inboundSocketAddress.sin_family = AF_INET;
  gRedundancyCtl.inboundSocketAddress.sin_port = htons(kRedundancyComPort);
  gRedundancyCtl.inboundSocketAddress.sin_addr.s_addr = htonl(INADDR_ANY);
  err = bind(gRedundancyCtl.inboundSocket, (struct sockaddr *)&gRedundancyCtl.inboundSocketAddress, sizeof(gRedundancyCtl.inboundSocketAddress));
  if(err)
  {
    perror("bind");

    //undo what we have done so far...
    close(gRedundancyCtl.outboundSocket);
    close(gRedundancyCtl.inboundSocket);
    return -1;
  }

  gRedundancyCtl.outboundSocketValid = 1;
  gRedundancyCtl.inboundSocketValid = 1;

  return 0;
}

/*--------------------------------------------------------------------------
    static int initRedundancy()

Description:
Do one-time initialization of the data structures. Uses a flag in the
data structure to determine if data was initialized before

Return value:
  0          no error
  1          data was initialized before, nothing done
  Any other value is an error.

Parameters:

  Inputs

  Outputs
    None

Global variables affected and border effects:
  Read
    gRedundancyCtl
  Write
    gRedundancyCtl
--------------------------------------------------------------------------*/
static int initRedundancy()
{
  if(gRedundancyCtl.dataInittialized != 0)
  {
    return 1;
  }

  //clean the data structure
  memset(&gRedundancyCtl, 0, sizeof(gRedundancyCtl));
  gRedundancyCtl.inboundCommand = ' ';
  gRedundancyCtl.outboundCommand = ' ';

  gRedundancyCtl.dataInittialized = 1;

  return 0;
}

/*--------------------------------------------------------------------------
    void setOutboundTimestamp(UINT32 TAIseconds, UINT32 TAInanoseconds)

Description:
This function gets called by SC_TimeCode to update the time.
The time reported by SC_TimeCode is the time for the NEXT PPS pulse.

Return value:
  none

Parameters:

  Inputs
    currentTAI current TAI as reported by the SC_TimeCode function
  Outputs
    None

Global variables affected and border effects:
  Read

  Write
    gRedundancyCtl
--------------------------------------------------------------------------*/
void setOutboundTimestamp(const UINT64 TAIseconds, const UINT32 TAInanoseconds)
{
  initRedundancy();

  gRedundancyCtl.outboundTimestamp.u48_sec = TAIseconds;
  gRedundancyCtl.outboundTimestamp.dw_nsec = TAInanoseconds;

  gRedundancyCtl.sendPending = 1;
}

/*--------------------------------------------------------------------------
    static int transferPeerMeasurement()

Description:
This function transfer a phase measurement to the SofClock using the
SC_TimeMeasToServo call.
As soon as a new measurement is available from the FPA it is transfered
with the last time received (if any) from the peer.

Return value:

Parameters:

  Inputs

  Outputs
    None

Global variables affected and border effects:
  Read
    gRedundancyCtl
  Write

--------------------------------------------------------------------------*/
static int transferPeerMeasurement()
{
  int peerInChanA;
  int phaseMeasurementAvailable;
  const UINT32 tsShelfLife = 1200000; //In microseconds. Value a little bit bigger than 1s to allow communications
  UINT8 currLosCntr;
  INT32 phase;
  static UINT8 prevlosCntr = 0;
  struct timespec currentTime;

  initRedundancy();

  if(GetExtAChan() == GetRedundancyChan())
  {
    peerInChanA = TRUE;
  }
  else
  {
    if(GetExtBChan() == GetRedundancyChan())
    {
      peerInChanA = FALSE;
    }
    else
    {
      return -1; //Redundancy channel is not A or B
    }
  }

  //see if new data is available
  phaseMeasurementAvailable = FALSE;
  if(peerInChanA)
  {
    currLosCntr = (SC_GetExtRefPhasorLosCntr() & FPGA_LOS_COUNT_EXT_A_MASK) >> FPGA_LOS_COUNT_EXT_A_SHIFT;
    if(currLosCntr != prevlosCntr)
    {
      SC_GetPhasorOffset(&phase, FPGA_EXT_REF_A_IN_CNT3);
      prevlosCntr = currLosCntr;
      phaseMeasurementAvailable = TRUE;
    }
  }
  else
  {
    currLosCntr = (SC_GetExtRefPhasorLosCntr() & FPGA_LOS_COUNT_EXT_B_MASK) >> FPGA_LOS_COUNT_EXT_B_SHIFT;
    if(currLosCntr != prevlosCntr)
    {
      SC_GetPhasorOffset(&phase, FPGA_EXT_REF_B_IN_CNT3);
      prevlosCntr = currLosCntr;
      phaseMeasurementAvailable = TRUE;
    }
  }

  if(phaseMeasurementAvailable)
  {
    clock_gettime(CLOCK_MONOTONIC, &currentTime);
    //if we have a recent packet with time, pass it along with the phase measurement, otherwise just
    //pass the phase measurement
    if(diff_usec(gRedundancyCtl.localReceptionTime, currentTime) < tsShelfLife)
    {
      SC_TimeMeasToServo(GetRedundancyChan(), phase, &gRedundancyCtl.inboundTimestamp);

      gRedundancyCtl.lastPhaseToServo = phase;
      gRedundancyCtl.lastTimeStampToServo = gRedundancyCtl.inboundTimestamp;
      gRedundancyCtl.lastDataToServoTime = currentTime;
    }
    else //just pass the phase measurement
    {
      SC_TimeMeasToServo(GetRedundancyChan(), phase, NULL);

      gRedundancyCtl.lastPhaseToServo = phase;
      gRedundancyCtl.lastTimeStampToServo.u48_sec = 0;
      gRedundancyCtl.lastTimeStampToServo.dw_nsec = 0;
      gRedundancyCtl.lastDataToServoTime = currentTime;
    }
  }

  return 0;
}

/*--------------------------------------------------------------------------
    int getPeerInfo(t_ptpTimeStampType *remoteTime, UINT32 *remoteEventMap, SC_t_activeStandbyEnum *remoteStatus, struct timespec *localReceptionTime)

Description:
This function returns the last information available from the peer.
The information returned is for "display only" and may not be consistent
because this function does not block any update received while the function is
running.
Any parameter can be NULL if not needed.

Return value:
  0: at least one message has been received from the peer
  1: not a single message has been received from the peer

Parameters:

  Inputs
    Pointers to the corresponding data.
    Any parameter can be NULL if the response is not needed.

  Outputs
    t_ptpTimeStampType *remoteTime
    UINT32 *remoteEventMap
    SC_t_activeStandbyEnum *remoteStatus
    struct timespec *localReceptionTime

Global variables affected and border effects:
  Read
    gRedundancyCtl
  Write

--------------------------------------------------------------------------*/
int getPeerInfo(t_ptpTimeStampType *remoteTime, UINT32 *remoteEventMap, SC_t_activeStandbyEnum *remoteStatus, struct timespec *localReceptionTime)
{
  initRedundancy();

  if(remoteTime != NULL)
  {
    *remoteTime = gRedundancyCtl.inboundTimestamp;
  }

  if(remoteEventMap != NULL)
  {
    *remoteEventMap = gRedundancyCtl.peerEventMap;
  }

  if(remoteStatus != NULL)
  {
    *remoteStatus = gRedundancyCtl.peerStatus;
  }

  if(localReceptionTime != NULL)
  {
    *localReceptionTime = gRedundancyCtl.localReceptionTime;
  }

  if(gRedundancyCtl.localReceptionTime.tv_sec != 0)
    return 0;
  else
    return -1;
}

/*--------------------------------------------------------------------------
    int getLastPeerTimestamp(INT32 *phase, t_ptpTimeStampType *lastTimeStampToServo, struct timespec *lastDataToServoTime)

Description:
This function returns the last information passed to the SoftClock by the SC_TimeMeasToServo
function. The information returned is for "display only" and may not be consistent
because this function does not block any update received while the function is running.
Any parameter can be NULL if not needed.

Return value:
  0: at least one measurement has been passed to the SotClock
  1: not a single measurement has been passed to the SotClock

Parameters:

  Inputs
    Pointers to the corresponding data.
    Any parameter can be NULL if the response is not needed.

  Outputs
    INT32 *phase
    t_ptpTimeStampType *lastTimeStampToServo
    struct timespec *lastDataToServoTime

Global variables affected and border effects:
  Read
    gRedundancyCtl
  Write

--------------------------------------------------------------------------*/
int getLastPeerTimestampToSoftClock(INT32 *phase, t_ptpTimeStampType *lastTimeStampToServo, struct timespec *lastDataToServoTime)
{
  initRedundancy();

  if(phase != NULL)
  {
    *phase = gRedundancyCtl.lastPhaseToServo;
  }

  if(lastTimeStampToServo != NULL)
  {
    *lastTimeStampToServo = gRedundancyCtl.lastTimeStampToServo;
  }

  if(lastDataToServoTime != NULL)
  {
    *lastDataToServoTime = gRedundancyCtl.lastDataToServoTime;
  }

  if(gRedundancyCtl.lastDataToServoTime.tv_sec != 0)
    return 0;
  else
    return -1;
}

/*--------------------------------------------------------------------------
    int sendCommandToPeer(const char command)

Description:
This function queues a command to be sent to the remote peer.
Commands are a single char.
So far recognized commands are:
  ' ': no command
  'a': go active
  's': go standby

Return value:
  0: command is queued OK, but remember this is UDP...
  -1: no socket active to the peer

Parameters:

  Inputs
    char command

  Outputs
    none

Global variables affected and border effects:
  Read
    gRedundancyCtl
  Write
    gRedundancyCtl
--------------------------------------------------------------------------*/
int sendCommandToPeer(const char command)
{
  initRedundancy();

  if(gRedundancyCtl.outboundSocketValid == TRUE)
  {
    gRedundancyCtl.outboundCommand = command;

    return 0;
  }
  else
  {
    return -1;
  }
}

/*--------------------------------------------------------------------------
    int setRedPckTrace(const int command)

Description:
This function sets, clears or toggles the printing of inbound and outbound UPD packets
for diagnostics. The new state is returned.

Return value:
  0: the new state: No printing of UDP packets
  1: the new state: Printing of UDP packets

Parameters:

  Inputs
    int command
      0: disable printing
      1: enable printing
      2: toggle current state

  Outputs
    none

Global variables affected and border effects:
  Read
    gRedundancyCtl
  Write
    gRedundancyCtl
--------------------------------------------------------------------------*/
int setRedPckTrace(const int command)
{
  int newState;

  initRedundancy();

  switch(command)
  {
    case 0: default: newState = 0; break;
    case 1: newState = 1; break;
    case 2: newState = (gRedundancyCtl.inboundPrintPkt == 0)?1:0; break;
  }

  gRedundancyCtl.outboundPrintPkt = gRedundancyCtl.inboundPrintPkt = newState;
  return newState;
}
