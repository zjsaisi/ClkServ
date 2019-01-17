/******************************************************************************
$Id: chan_processing_if/sc_SSM_handling.c 1.6 2011/03/04 13:15:41PST German Alvarez (galvarez) Exp  $

Copyright (C) 2009, 2010, 2011 Symmetricom, Inc., all rights reserved
This software contains proprietary and confidential information of Symmetricom.
It may not be reproduced, used, or disclosed to others for any purpose
without the written authorization of Symmetricom.

Original author: German Alvarez, Feb 10, 2011

Last revision by: $Author: German Alvarez (galvarez) $

File name: $Source: chan_processing_if/sc_SSM_handling.c $

Functionality:
Handle the SSM and validity attributes from the input drivers and hand it to the
soft clock. Also allow for injections of values to help testing.

******************************************************************************
Log:
$Log: chan_processing_if/sc_SSM_handling.c  $
Revision 1.6 2011/03/04 13:15:41PST German Alvarez (galvarez) 
corrected a problem with manual override for SSM and valid.
Revision 1.5 2011/02/15 15:13:30PST Daniel Brown (dbrown)
Removed printf debug in SC_ChanSSM().
Revision 1.4 2011/02/14 09:51:23PST Daniel Brown (dbrown)
Updates to implement consistent naming of "b_ChanIndex" and "b_ChanCount".
Revision 1.3 2011/02/11 11:40:32PST German Alvarez (galvarez)
Make the code more general, not only handle SSM/ESMC, also channel validity status.
Added SC_SetChanValid and SC_ChanValid
Revision 1.2 2011/02/10 16:53:57PST German Alvarez (galvarez)
Handle the SSM from the input drivers and hand it to the soft clock.
Also allow for injections of SSM values to help testing.
Revision 1.1 2011/02/10 16:46:55PST German Alvarez (galvarez)
Initial revision
Member added to project e:/MKS_Projects/PTPSoftClientDevKit/SCRDB_MPC8313/ptp/common/project.pj


******************************************************************************/

/*--------------------------------------------------------------------------*/
//              include
/*--------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include "sc_chan.h"

#include "sc_SSM_handling.h"

/*--------------------------------------------------------------------------*/
//              defines
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
//              types
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
//              data
/*--------------------------------------------------------------------------*/
t_chanAttributes gChanAttributesTable[SC_TOTAL_CHANNELS];

/*--------------------------------------------------------------------------*/
//              externals
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
//              function prototypes
/*--------------------------------------------------------------------------*/
static void initSSMHandling(void);

/*--------------------------------------------------------------------------*/
//              functions
/*--------------------------------------------------------------------------*/


/*--------------------------------------------------------------------------
    void initSSMHandling(void)

Description:
This needs to be called by any SSM handling function at the beginning.
It initializes the SSM handling system, if needed.

Return value:
  void


Global variables affected and border effects:
  Read
    gChanAttributesTable

  Write
    gChanAttributesTable
--------------------------------------------------------------------------*/
static void initSSMHandling(void)
{
  static BOOLEAN isSSMInitialized = FALSE;

  if(isSSMInitialized)
    return;

  //clear the gChanAttributesTable table
  memset(&gChanAttributesTable, 0, sizeof(t_chanAttributes)*SC_TOTAL_CHANNELS);

  isSSMInitialized = TRUE;
}


/*--------------------------------------------------------------------------
    int SC_ChanSSM(UINT8 b_ChanIndex, UINT8 *pb_ssm, BOOLEAN *po_Valid)

Description:
This function is called by the Channel Manager to determine
a channel's SSM.  This function will take the SSM number and the reference
type, and translate to a PQL number that will be used in the channel
selection code.

Return value:
  0          no error
  Any value different from zero is an error.

Parameters:

  Inputs
    UINT8 b_ChanIndex Channel number

  Outputs
    UINT8 *pb_ssm SSM level read from channel indexed by b_ChanIndex
    BOOLEAN *po_Valid
      TRUE - SSM value provided in *po_Valid is valid
      FALSE - SSM value provided in *po_Valid is invalid

Global variables affected and border effects:
  Read
    gChanAttributesTable

  Write

--------------------------------------------------------------------------*/
int SC_ChanSSM(UINT8 b_ChanIndex, UINT8 *pb_ssm, BOOLEAN *po_Valid)
{
  struct timespec currentTime;
  UINT8 numChannels = 0;

  initSSMHandling();

  if((SC_GetChanConfig(NULL, NULL, NULL, &numChannels) == 0) && (b_ChanIndex < numChannels))
  {
    *po_Valid = gChanAttributesTable[b_ChanIndex].isESMCValid;
    *pb_ssm = gChanAttributesTable[b_ChanIndex].ESMCValue;

    if(gChanAttributesTable[b_ChanIndex].ESMCOverride == TRUE)
    { //manual override mode. ignore timer, we are done
      return 0;
    }

    if(gChanAttributesTable[b_ChanIndex].isESMCValid == TRUE)
    { //check the timer for the final word.
      if(gChanAttributesTable[b_ChanIndex].ESMCGoodUntil != 0)
      {
        clock_gettime(CLOCK_MONOTONIC, &currentTime);
        *po_Valid  = (gChanAttributesTable[b_ChanIndex].ESMCGoodUntil >=currentTime.tv_sec)?TRUE:FALSE;
      }
    }
    return 0;
  }
  else
    return -1;
}


/*--------------------------------------------------------------------------
    int SC_SetSSMValue(const UINT8 b_ChanNumber, const UINT8 b_SSMValue, const BOOLEAN o_isSSMValid,
              const UINT8 b_ESMCVersion, const UINT16 timeToLive)


Description:
This function is called by the input channel driver when information about the
SSM/ESMC quality is received for that particular channel.
The information will be then provided to the soft clock when it calls the SC_ChanSSM
function.

Return value:
  0     no error
  -1    invalid b_ChanNumber

Parameters:

  Inputs
    const UINT8 b_ChanIndex       The channel the information applies for
    const UINT8 b_SSMValue      SSM/ESMC quality level
    const BOOLEAN o_isSSMValid  Normally set to TRUE.
    const UINT8 b_ESMCVersion   ESMC version
    const UINT16 timeToLive     For how many seconds this information
                                will be marked as valid when requested by SC_ChanSSM
                                0 means infinite

  Outputs
    None

Global variables affected and border effects:
  Read
    gChanAttributesTable

  Write
    gChanAttributesTable
--------------------------------------------------------------------------*/
int SC_SetSSMValue(const UINT8 b_ChanIndex, const UINT8 b_SSMValue, const BOOLEAN o_isSSMValid,
      const UINT8 b_ESMCVersion, const UINT16 timeToLive)
{
  struct timespec currentTime;
  UINT8 numChannels = 0;

  initSSMHandling();

  if((SC_GetChanConfig(NULL, NULL, NULL, &numChannels) == 0) && (b_ChanIndex < numChannels))
  {
    if(gChanAttributesTable[b_ChanIndex].ESMCOverride == FALSE)
    {
      gChanAttributesTable[b_ChanIndex].isESMCValid = o_isSSMValid;
      gChanAttributesTable[b_ChanIndex].ESMCValue = b_SSMValue;
      gChanAttributesTable[b_ChanIndex].ESMCVersion = b_ESMCVersion;

      clock_gettime(CLOCK_MONOTONIC, &currentTime);
      if(timeToLive != 0)
        gChanAttributesTable[b_ChanIndex].ESMCGoodUntil = currentTime.tv_sec + timeToLive;
      else
        gChanAttributesTable[b_ChanIndex].ESMCGoodUntil = 0;
    }
  }
  else
    return -1;

  return 0;
}


/*--------------------------------------------------------------------------
    int SC_ChanValid(const UINT8 b_ChanIndex, BOOLEAN *po_valid)

Description:
This function is called by the Channel Manager to determine
if the channel is valid.  This function will be written by the customer.

Return value:
  0     no error
  -1    invalid b_ChanNumber

Parameters:
  Inputs
    const UINT8 b_ChanIndex       The channel the information applies for

  Outputs
    BOOLEAN *po_valid           Address to return validation data

Global variables affected and border effects:
  Read
    gChanAttributesTable

  Write

--------------------------------------------------------------------------*/

int SC_ChanValid(const UINT8 b_ChanIndex, BOOLEAN *po_valid)
{
  UINT8 numChannels = 0;

  initSSMHandling();

  if((SC_GetChanConfig(NULL, NULL, NULL, &numChannels) == 0) && (b_ChanIndex < numChannels))
  {
    if(po_valid != NULL)
      *po_valid = gChanAttributesTable[b_ChanIndex].isChanValid;
  }
  else
    return -1;

  return 0;
}


/*--------------------------------------------------------------------------
    int SC_SetChanValid(const UINT8 b_ChanIndex, const BOOLEAN o_valid)

Description:
This function is called by the input channel driver when information about the
validity for that particular channel. For example when a LOS is detected the validity
should be set to FALSE

Return value:
  0     no error
  -1    invalid b_ChanNumber

Parameters:
  Inputs
    const UINT8 b_ChanIndex       The channel the information applies for
    const BOOLEAN o_valid       is the channel valid

  Outputs

Global variables affected and border effects:
  Read

  Write
    gChanAttributesTable
--------------------------------------------------------------------------*/

int SC_SetChanValid(const UINT8 b_ChanIndex, const BOOLEAN o_valid)
{
  UINT8 numChannels = 0;

  initSSMHandling();

  if((SC_GetChanConfig(NULL, NULL, NULL, &numChannels) == 0) && (b_ChanIndex < numChannels))
  {
    if(gChanAttributesTable[b_ChanIndex].chanValidOverride == FALSE)
      gChanAttributesTable[b_ChanIndex].isChanValid = o_valid;
  }
  else
    return -1;

  return 0;
}


/*--------------------------------------------------------------------------
    int chanAttributes_RAW(const t_paramOperEnum b_operation,
    const UINT8 b_ChanIndex, t_chanAttributes *ps_theChan)

Description:
Used to read/write the table directly. Used for the manual overrides from the UI
and initialization.

Return value:
  0     no error
  -1    invalid b_ChanNumber

Parameters:
  Inputs
    const t_paramOperEnum b_operation   e_PARAM_GET or e_PARAM_SET
    const UINT8 b_ChanIndex               The channel the information applies for
    t_chanAttributes *ps_theChan        when e_PARAM_SET

  Outputs
    t_chanAttributes *ps_theChan        when e_PARAM_GET

Global variables affected and border effects:
  Read
    gChanAttributesTable        e_PARAM_GET

  Write
    gChanAttributesTable        when e_PARAM_SET
--------------------------------------------------------------------------*/
int chanAttributes_RAW(const t_paramOperEnum b_operation,
    const UINT8 b_ChanIndex, t_chanAttributes *ps_theChan)
{
  UINT8 numChannels = 0;

  initSSMHandling();

  if((SC_GetChanConfig(NULL, NULL, NULL, &numChannels) == 0) && (b_ChanIndex < numChannels))
  {
    if(ps_theChan == NULL)
      return 0; //passing null is not an error, if b_ChanIndex is valid.

    switch(b_operation)
    {
      case e_PARAM_GET:
        *ps_theChan = gChanAttributesTable[b_ChanIndex];
      break;

      case e_PARAM_SET:
        gChanAttributesTable[b_ChanIndex] = *ps_theChan;
      break;

      default:
        return -1;
      break;
    }
    return 0;
  }
  else
    return -1;
}
