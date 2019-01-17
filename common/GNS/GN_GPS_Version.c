
//*****************************************************************************
// GPS IP Centre, ST-Ericsson (UK) Ltd.
// Copyright (c) 2009 ST-Ericsson (UK) Ltd.
// 15-16 Cottesbrooke Park, Heartlands Business Park, Daventry, NN11 8YL, UK.
// All rights reserved.
//
// Filename  GN_GPS_Version.c
//
// $Id: GNS/GN_GPS_Version.c 1.1 2011/01/07 13:31:05PST Daniel Brown (dbrown) Exp  $
//
//*****************************************************************************

#include "gps_ptypes.h"

#include "GN_GPS_api.h"

//*****************************************************************************
// Global constant defining the Software Version details of the host Linux gn_gps
// application.

const s_GN_GPS_Version gn_Version =   // Host GPS SW Version Details
{
   "ptp_gps",                    // Host Wrapper SW Version Name
   0,                            // Use GPS Core Library Major Version No
   0,                            // Use GPS Core Library Minor Version No
   "",                           // Host Wrapper SW Build Version Date
   __DATE__,                     // Host Wrapper SW Build Compiled Date
   __TIME__,                     // Host Wrapper SW Build Compiled Time
   0,                            // GNB ROM SW Version (Can not be changed)
   0,                            // GNB ROM SW Patch Checksum (Can not be changed)
   ""                            // GN GPS System Verrsion (Can not be changed here)
};

//*****************************************************************************
