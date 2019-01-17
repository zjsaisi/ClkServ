
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

FILE NAME    : tod.c

AUTHOR       : Ken Ho

DESCRIPTION  :



Revision control header:
$Id: TOD/tod.c 1.5 2011/03/24 18:51:04PDT German Alvarez (galvarez) Exp  $

******************************************************************************
*/


/*--------------------------------------------------------------------------*/
/*                         INCLUDE FILES                                    */
/*--------------------------------------------------------------------------*/
#include "target.h"
#include "stdio.h"
#include "sc_types.h"
#include "sc_api.h"
#include "sc_servo_api.h"
#include "time.h"
#include "tod.h"
#include "CLK/clk_private.h"
#include "PTP/PTPdef.h"
#include "NIF/NIF.h"
#include "PTP/PTP.h"
#include "MNT/MNT.h"
#include "MNT/MNTapi.h"
#include "sc_chan.h"

/*--------------------------------------------------------------------------*/
/*                              defines                                     */
/*--------------------------------------------------------------------------*/



/*--------------------------------------------------------------------------*/
/*                              types                                       */
/*--------------------------------------------------------------------------*/


/*--------------------------------------------------------------------------*/
/*                              data                                        */
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/*                              functions                                   */
/*--------------------------------------------------------------------------*/

/*
----------------------------------------------------------------------------
                                GenerateTimecode(void);

Description:
This function will generates the time code in TAI format and calls the user
defined function SC_TimeCode

Parameters:

Inputs
  None

Outputs:
  None

Return value:
  None
-----------------------------------------------------------------------------
*/
void GenerateTimecode(void)
{
  static BOOLEAN firstTimeRun = TRUE;
  static t_timeCodeType timeCode;

  //channel related variables
  static UINT8 numChannels = 0xff;
  static SC_t_ChanConfig chanConfig[SC_TOTAL_CHANNELS];
  UINT8 timeChan = 0xff;
  SC_t_chanStatus chanStatus[SC_TOTAL_CHANNELS];

  int retValue;
  t_ptpTimeStampType currentTime;
  INT16 utc_unused;
  int i;
  UINT8 maxTimeWeight = 0;

  //variables to get TLV values
  SC_t_MntTLV s_Tlv;
  UINT8 b_tlvData[k_SC_MAX_TLV_PLD_LEN];
  s_Tlv.pb_data = b_tlvData;
  //s_Tlv.w_len = k_SC_MAX_TLV_PLD_LEN;

  if(firstTimeRun)
  {
    firstTimeRun = FALSE;

    //set timeCode to a known state
    timeCode.e_origin = e_TIME_NEVER_SET;
    timeCode.l_timeSeconds = 0;
    timeCode.l_TimeNanoSec = 0;
    timeCode.i_utcOffset = 0;
    timeCode.o_utcOffsetValid = FALSE;
    timeCode.o_leap61 = FALSE;
    timeCode.o_leap59 = FALSE;
    timeCode.o_ptpTimescale = FALSE;
    timeCode.o_activelySourced = FALSE;



  }

  //try to get the number of channles
  //SC_GetChanConfig may not be ready yet...
  if(numChannels == 0xff)
  {
    retValue = SC_GetChanConfig(chanConfig, NULL, NULL, &numChannels);
    if(retValue != 0)
    {
      numChannels = 0xff;
      timeCode.o_activelySourced = FALSE;
      goto getTimeAndExit;
    }
  }

  /*determine active time channel*/
  if(numChannels != 0xff)
  {
    retValue = SC_GetChanStatus(numChannels, chanStatus);
    if(retValue != 0)
    {
      timeCode.o_activelySourced = FALSE;
      goto getTimeAndExit;
    }
  }
  else
  {
    timeCode.o_activelySourced = FALSE;
    goto getTimeAndExit;
  }

  //linear search for the channel with maximum time weight
  maxTimeWeight = 0;
  timeChan = 0xff;
  for(i=0; i < numChannels; i++)
  {
    if(chanStatus[i].b_timeWeight > maxTimeWeight)
    {
      timeChan = i;
      maxTimeWeight = chanStatus[i].b_timeWeight;
    }
  }

  if(timeChan == 0xff) //no active channel
  {
    timeCode.o_activelySourced = FALSE;
    goto getTimeAndExit;
  }

  switch(chanConfig[timeChan].e_ChanType)
  {
    case e_SC_CHAN_TYPE_PTP:
      if(SC_PTP_TimePropDs(e_PARAM_GET, &s_Tlv) == 0)
      {
        timeCode.e_origin = e_TIME_PTP;
        timeCode.i_utcOffset = (s_Tlv.pb_data[k_OFFS_TIDS_UTCOFFS] * 256) + s_Tlv.pb_data[k_OFFS_TIDS_UTCOFFS + 1];
        timeCode.o_utcOffsetValid = (s_Tlv.pb_data[k_OFFS_TIDS_FLAGS] & (1 << k_OFFS_TIDS_FLAGS_UTCV))?TRUE:FALSE;
        timeCode.o_leap61 = (s_Tlv.pb_data[k_OFFS_TIDS_FLAGS] & (1 << k_OFFS_TIDS_FLAGS_LI61))?TRUE:FALSE;
        timeCode.o_leap59 = (s_Tlv.pb_data[k_OFFS_TIDS_FLAGS] & (1 << k_OFFS_TIDS_FLAGS_LI59))?TRUE:FALSE;
        timeCode.o_ptpTimescale = (s_Tlv.pb_data[k_OFFS_TIDS_FLAGS] & (1 << k_OFFS_TIDS_FLAGS_PTP))?TRUE:FALSE;
        timeCode.o_activelySourced = TRUE;
      }
      else
        timeCode.o_activelySourced = FALSE;
    break;

    case e_SC_CHAN_TYPE_GPS:
      timeCode.e_origin = e_TIME_GPS;
      //timeCode.i_utcOffset = 0;
      timeCode.o_utcOffsetValid = FALSE;
      //timeCode.o_leap61 = FALSE;
      //timeCode.o_leap59 = FALSE;
      timeCode.o_ptpTimescale = TRUE;
      timeCode.o_activelySourced = TRUE;
    break;

    default:
      timeCode.o_activelySourced = FALSE;
    break;
  }

getTimeAndExit:

  SC_SystemTime(&currentTime, &utc_unused, e_PARAM_GET);
  timeCode.l_timeSeconds = currentTime.u48_sec + 1; //the time for the NEXT second
  timeCode.l_TimeNanoSec = currentTime.dw_nsec;

  SC_TimeCode(&timeCode);
}


#ifdef NONE
/*
----------------------------------------------------------------------------
                                Print_Timecode_T1()

Description:
This function will return the local TAI time.


Parameters:

Inputs
   None

Outputs:
	None

Return value:
	0: function succeeded
	-1: function failed

-----------------------------------------------------------------------------
*/
/* this define should track loosely to SC_TOTAL_CHANNELS
   it should be set to the maximum value that SC_TOTAL_CHANNELS
   can ever be in any product/library */
int Print_Timecode_T1(void)
{
  const int kNumChanInTOD = 4; /* this define should track loosely to SC_TOTAL_CHANNELS
                                it should be set to the maximum value that SC_TOTAL_CHANNELS
                                can ever be in any product/library */
  int ret_val;
  UINT32 u32_nanoSeconds;
  UINT32 u32_time;
  time_t time;
  struct tm *Tm;
  int i;
  UINT32 bitmap;
  UINT32 alarm = 0;
  UINT8 chan_status[4];
  UINT8 zfom = 0;
  UINT8 status_reg = 0;
  INT16 utc_offset;
  FLOAT32 zfom_float = Get_out_tdev_est()/100.0;
  UINT8 pendLeap = get_pendingLeapsecond();


  as_localTodStr[0] = 0xAA;
  status_reg = get_fll_state();

  ret_val = Get_TAI_Time(&u32_time, &u32_nanoSeconds, &utc_offset);
  time = u32_time + 1;  // add 1 to get next second
  Tm = localtime(&time);

  /* pending +1 leap second */
  if(pendLeap & 1)
  {
    status_reg |= 0x80;
  }
  /* pending -1 leap second */
  else if(pendLeap & 2)
  {
    status_reg |= 0x40;
  }

  if(zfom_float > 255.0)
  {
    zfom_float = 255.0;
  }

  switch(status_reg & 0x3F)
  {
    case FLL_WARMUP:
      zfom = 255; /* bad time */
      break;
    case FLL_FAST:
      if(zfom_float == 0.0) //
      {
        zfom = 255; /* bad time */
      } /*  */
      else
      {
        zfom = zfom_float;
      }
      break;
    case FLL_NORMAL:
    case FLL_BRIDGE:
    case FLL_HOLD:
      zfom = zfom_float; /*  */
      break;
    case FLL_UNKNOWN:
    default:
      zfom = 255; /* bad time */
      break;
  }

  for(i=0;i<kNumChanInTOD;i++)
  {
    if(GetEventBitmap(i, &bitmap))
    {
      chan_status[i] = 0; /* if non-zero then failed */
    }
    else
    {
      chan_status[i] = (UINT8)bitmap; /* only take lsbyte */
    }
  }

  sprintf(&as_localTodStr[1], "T1%02d%02d%02d%02d%02d%04d%02X%02X%02X%02X%02X%02X%02X %02X ",
      (UINT32)Tm->tm_hour,
      (UINT32)Tm->tm_min,
      (UINT32)Tm->tm_sec,
      (UINT32)Tm->tm_mon+1,
      (UINT32)Tm->tm_mday,
      (UINT32)Tm->tm_year+1900,

      (UINT32)zfom,
      (UINT32)status_reg,
      (UINT32)alarm,
      (UINT32)chan_status[0],
      (UINT32)chan_status[1],
      (UINT32)chan_status[2],
      (UINT32)chan_status[3],
      (UINT32)utc_offset);

  sprintf(&as_localTodStr[32], "%02X", Crc_1wire_checksum (&as_localTodStr[3], 29));
  SC_PrintTod(&as_localTodStr[0], u32_nanoSeconds);

  //   printf("%s\n", as_localTodStr);

  return ret_val;
}
#endif
