
//****************************************************************************
// GPS IP Centre, ST-Ericsson (UK) Ltd.
// Copyright (c) 2009 ST-Ericsson (UK) Ltd.
// 15-16 Cottesbrooke Park, Heartlands Business Park, Daventry, NN11 8YL, UK.
// All rights reserved.
//
// Filename  GN_AGPS_FreqAid_api.h
//
// $Header: GNS/GN_AGPS_FreqAid_api.h 1.1 2011/01/07 13:30:38PST Daniel Brown (dbrown) Exp  $
// $Locker:  $
//****************************************************************************

#ifndef GN_AGPS_FREQAID_H
#define GN_AGPS_FREQAID_H

#ifdef __cplusplus
   extern "C" {
#endif

//****************************************************************************
//
//  GN AGPS Frequency Aiding Interface API Definitions
//
//*****************************************************************************

#include "gps_ptypes.h"


//*****************************************************************************
//
// GN AGPS Frequency Aiding Interface API/Callback function prototypes
//
// Frequency aiding is implemented by the host sending pulses to the
// EXT_FRAME_SYNC pin of the GNS7560 chip at a known frequency over a period of,
// at least 2 or 3 seconds.
// This calibration frequency is expected to be in the range 100Hz to 10.0 MHz.
// In addition to these pulses, the host is required to make two API function
// calls – one to start the frequency aiding and one to stop it.
//
//*****************************************************************************

//*****************************************************************************
// API Functions that may be called by the Host platform software.


// GN AGPS API Function to start the frequency aiding computations.
// The return value will be TRUE provided that the input Frequency is in the
// range 100 Hz to 10.0 MHz, and that the GN GPS Library believes the GNS7560
// chip to be awake and running.
// If the chip is in Sleep or Coma mode, the return value will be FALSE.
// This function must be called after the input pulses have been enabled, and
// after the API function GN_GPS_Initialise() has been called.

BL GN_AGPS_Start_Freq_Aiding(
   R8 Freq,                    // i - Known frequency of the input pulses [MHz]
                               //        [range 0.0001 .. 10.0 MHz]
   U4 RMS_ppb );               // i - Uncertainty (RMS value) of the known frequency [ppb]


// GN AGPS API Function to stop the frequency aiding computations.
// This function must be called while the input pulses are still enabled.
// On return from this function, the host should disable the pulses.

void GN_AGPS_Stop_Freq_Aiding( void );


// Comment
// =======
// For the period during which the frequency aiding pulses are enabled the host
// should not try to make use of the Fine Time input functionality, i.e. the
// host should not make calls to the GN_AGPS_Set_EFSP_Time() API function.
// That is, frequency aiding and precise time input are mutually exclusive.


//*****************************************************************************

#ifdef __cplusplus
   }     // extern "C"
#endif

#endif   // GN_AGPS_FREQAID_H
