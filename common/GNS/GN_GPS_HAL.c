//*****************************************************************************
// GPS IP Centre, ST-Ericsson (UK) Ltd.
// Copyright (c) 2009 ST-Ericsson (UK) Ltd.
// 15-16 Cottesbrooke Park, Heartlands Business Park, Daventry, NN11 8YL, UK.
// All rights reserved.
//
// Filename  GN_GNS_HAL.c
//
// $Id: GNS/GN_GPS_HAL.c 1.3 2011/04/01 15:08:08PDT Daniel Brown (dbrown) Exp  $
//
//*****************************************************************************

//*****************************************************************************
//
//  Target specific GN GNS???? Chip/Module Hardware Abstraction Layer.
//
//  For this example distribution source code it is assumed that the GNS????
//  Chip/Module hardware is an externally connected Demonstration Kit, so
//  Power Control is only  possible if the comm port RTS line has been enabled
//  for this purpose, and the "-r" / "--rts" command line argument has been
//  specified.
//
//  TODO:  Modify this module to suit the final target implementation
//
//*****************************************************************************

#include "GN_GPS_Task.h"
#include "sc_api.h"

//*****************************************************************************
//  GN_GNS_HAL_Reset:  Function to "Reset" the GNS???? Chip / Module.
//  Return TRUE:  If "Reset" Control is supported and was achieved.

BL GN_GNS_HAL_Reset( void )
{
#ifdef GNS_DEBUG
   printf( "GN_GNS_Hal_Reset:  By Reset.\n" );
#endif
   /* Call SCi GPS Reset API function */
   SC_GpsReset();
   /* return success */
   return(TRUE);
}


//*****************************************************************************
