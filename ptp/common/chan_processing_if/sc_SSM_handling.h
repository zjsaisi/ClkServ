/******************************************************************************
$Id: chan_processing_if/sc_SSM_handling.h 1.4 2011/02/14 09:49:54PST Daniel Brown (dbrown) Exp  $

Copyright (C) 2009, 2010, 2011 Symmetricom, Inc., all rights reserved
This software contains proprietary and confidential information of Symmetricom.
It may not be reproduced, used, or disclosed to others for any purpose
without the written authorization of Symmetricom.


Original author: German Alvarez

Last revision by: $Author: Daniel Brown (dbrown) $

File name: $Source: chan_processing_if/sc_SSM_handling.h $

Functionality:
Handle the SSM from the input drivers and hand it to the soft clock.
Also allow for injections of SSM values to help testing.

******************************************************************************
Log:
$Log: chan_processing_if/sc_SSM_handling.h  $
Revision 1.4 2011/02/14 09:49:54PST Daniel Brown (dbrown) 
Updates to implement consistent naming of "b_ChanIndex" and "b_ChanCount".
Revision 1.3 2011/02/11 11:40:32PST German Alvarez (galvarez) 
Make the code more general, not only handle SSM/ESMC, also channel validity status.
Added SC_SetChanValid and SC_ChanValid
Revision 1.2 2011/02/10 16:53:58PST German Alvarez (galvarez)
Handle the SSM from the input drivers and hand it to the soft clock.
Also allow for injections of SSM values to help testing.
Revision 1.1 2011/02/10 16:46:56PST German Alvarez (galvarez)
Initial revision
Member added to project e:/MKS_Projects/PTPSoftClientDevKit/SCRDB_MPC8313/ptp/common/project.pj


******************************************************************************/

#ifndef SC_SSM_HANDLING_H_
#define SC_SSM_HANDLING_H_

/*--------------------------------------------------------------------------*/
//              include
// Nested includes should not be used as a general practice. If there is a
// good reason to include an include within an include, do so here.
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
//              defines
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
//              types
/*--------------------------------------------------------------------------*/
typedef struct //Structure to keep the attributes for all channels
{
  UINT8 ESMCVersion; //Version from the ESMC packet
  UINT8 ESMCValue;
  BOOLEAN isESMCValid;
  BOOLEAN ESMCOverride;
  time_t ESMCGoodUntil; //in CLOCK_MONOTONIC timescale, 0 means forever
  BOOLEAN isChanValid;  //for SC_chanValid()
  BOOLEAN chanValidOverride;
} t_chanAttributes;

/*--------------------------------------------------------------------------*/
//              data
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
//              externals
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
//              function prototypes
/*-------------------------------------------------------iSSMValid------------*/

/*--------------------------------------------------------------------------
    int SC_SetSSMValue(const UINT8 b_ChanNumber, const UINT8 b_SSMValue, const BOOLEAN o_iSSMValid,
              const UINT8 b_version, const UINT16 timeToLive, const UINT8 b_Priority)


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
    const BOOLEAN o_iSSMValid   Normally set to TRUE
    const UINT8 b_version       ESMC version
    const UINT16 timeToLive     For how many seconds this information
                                will be marked as valid when requested by SC_ChanSSM

  Outputs
    None

Global variables affected and border effects:
  Read
    gChanAttributesTable

  Write
    gChanAttributesTable
--------------------------------------------------------------------------*/
int SC_SetSSMValue(const UINT8 b_ChanIndex, const UINT8 b_SSMValue, const BOOLEAN o_isSSMValid,
      const UINT8 b_ESMCVersion, const UINT16 timeToLive);


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

int SC_SetChanValid(const UINT8 b_ChanIndex, const BOOLEAN o_valid);

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
    const UINT8 b_ChanIndex, t_chanAttributes *ps_theChan);


#endif /* SC_SSM_HANDLING_H_ */

