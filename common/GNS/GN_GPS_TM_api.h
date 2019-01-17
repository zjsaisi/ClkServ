
//****************************************************************************
// GloNav GPS Technology
// Copyright (C) 2001-2008 GloNav Ltd.
// March House, London Rd, Daventry, Northants, UK.
// All rights reserved
//
// Filename  GN_GPS_RM_api.h
//
// $Header: GNS/GN_GPS_TM_api.h 1.1 2011/01/07 13:30:58PST Daniel Brown (dbrown) Exp  $
// $Locker:  $
//****************************************************************************

#ifndef GN_GPS_TM_API_H
#define GN_GPS_TM_API_H

#ifdef __cplusplus
   extern "C" {
#endif

//****************************************************************************
//
//   GloNav GPS Tracking Measurements External Interface API Definitions
//
//*****************************************************************************

#include "gps_ptypes.h"

//*****************************************************************************
//
//  GN_GPS Tracking Measurements External Interface API/Callback function
//  related enumerated data type and structure definitions.
//
//*****************************************************************************

#define GN_GPS_TMCH   14         // Number of GN_GPS Tracking Measurement Channels

// GN_GPS Time of Week Status enumerated data type.
// Used to define how accurately the current Time of Week is believed to be known.
// Note:  The elements of this enumerated data type have been specifically
// ordered so that those with incrementally higher values (i.e. further down
// the list) correspond to a better knowledge of GPS Time of Week.

typedef enum GN_GPS_TOW_Status   // GN_GPS Time of Week known Status
{
   GN_GPS_TOW_ST_NOT_SET,        // GPS TOW is completely unknown
   GN_GPS_TOW_ST_10_MIN,         // GPS TOW is known to better than  10 minutes (max)
   GN_GPS_TOW_ST_01_MIN,         // GPS TOW is known to better than   1 minute  (max)
   GN_GPS_TOW_ST_10_SEC,         // GPS TOW is known to better than  10 seconds (max)
   GN_GPS_TOW_ST_03_SEC,         // GPS TOW is known to better than   3 seconds (max)
   GN_GPS_TOW_ST_01_SEC,         // GPS TOW is known to better than   1 second  (max)
   GN_GPS_TOW_ST_300_MS,         // GPS TOW is known to better than 100 milliseconds (max)
   GN_GPS_TOW_ST_100_MS,         // GPS TOW is known to better than 100 milliseconds (max)
   GN_GPS_TOW_ST_030_MS,         // GPS TOW is known to better than  30 milliseconds (max)
   GN_GPS_TOW_ST_010_MS,         // GPS TOW is known to better than  10 milliseconds (max)
   GN_GPS_TOW_ST_003_MS,         // GPS TOW is known to better than   3 milliseconds (max)
   GN_GPS_TOW_ST_SUBMS           // GPS TOW is known to much better than 1 milliseconds

}  e_GN_GPS_TOW_St;              // GN_GPS Time of Week known Status


// GN_GPS Tracking Measurement Quality Indicator Enumerated data type.
// Note:  The order of the definitions below is important as the Tracking
// Measurement Quality gets better with increasing value.  For example,
// a signal which is carrier locked is implicity already frequency and
// code locked.

typedef enum GN_GPS_TMQI         // GN_GPS Tracking Measurement Quality Indicators
{
   GN_GPS_TMQI_IDLE,             // 0    : Channel Idle, No Measurement
   GN_GPS_TMQI_SEARCHING,        // 1    : Channel Searching, No Measurement
   GN_GPS_TMQI_VALIDATING,       // 2    : SV Found, but still validating Measurement
   GN_GPS_TMQI_REPEAT_SV,        // 3    : Repeated SV, not used for Navigation
   GN_GPS_TMQI_INVALID_1,        // 4    : Invalid Measurement, level 1
   GN_GPS_TMQI_INVALID_2,        // 5    : Invalid Measurement, level 2
   GN_GPS_TMQI_INVALID_3,        // 6    : Invalid Measurement, level 3
   GN_GPS_TMQI_INVALID_4,        // 7    : Invalid Measurement, level 4
   GN_GPS_TMQI_INVALID_5,        // 8    : Invalid Measurement, level 5
   GN_GPS_TMQI_RE_ACQ,           // 9    : Tracking dropped back into Re-Acquisition
   GN_GPS_TMQI_SUBMS_CODE,       // 10   : >= Wide Band Code Lock, but only the
                                 //           sub-millisecond part of the code is known
   GN_GPS_TMQI_WB_CODE_LK,       // >=11 : Wide Band Code Lock
   GN_GPS_TMQI_NB_CODE_LK,       // >=12 : Narrow Band Code Lock
   GN_GPS_TMQI_WB_FREQ_LK,       // >=13 : Wide Band Frequency Lock
   GN_GPS_TMQI_NB_FREQ_LK,       // >=14 : Narrow Band Frequency Lock
   GN_GPS_TMQI_HW_CARR_LK,       // >=15 : Half Wavelength Carrier Lock
   GN_GPS_TMQI_FI_CARR_LK,       // 16   : Full Wavelength Carrier Lock,
                                 //           Inverted, add 0.5 cycle to make in-phase
   GN_GPS_TMQI_FW_CARR_LK,       // 17   : Full Wavelength Carrier Lock,
                                 //           Was in-phase at lock on
   GN_GPS_TMQI_FH_CARR_LK        // 18   : Full Wavelength Carrier Lock,
                                 //           Half cycle applied to make in-phase

}  e_GN_GPS_TMQI;                // GPS Tracking Measurement Quality Indicators


// GN_GPS Tracking Measurements Structure.
// Notes:
//  1) No measurements should be used unless the QI[] >= GN_GPS_TM_QI_SUBMS_CODE.
//  2) If QI[] = GN_GPS_TM_QI_SUBMS_CODE then the Psueorange measurements will
//     be in the range 0..300km,  whereas for all other cases they will typically
//     be in the range 18,000km to 27,000km.
//  3) Pseudorange   measurements should only be used if CodeLock[] > 0.
//  4) Doppler       measurements should only be used if FreqLock[] > 0.
//  5) Carrier Phase measurements should only be used if CarrLock[] > 0.
//  6) If QI[] = Pseudorange measurements should only be used if CodeLock[] > 0.
//  7) The

typedef struct GN_GPS_Track_Meas // GN GPS Tracking Measurement Data
{
   U4 Local_TTag;                // Measurement local baseband millisecond time-tag
                                 //    [milliseconds]
   e_GN_GPS_TOW_St TOW_St;       // Measuremnt local GPS Time of Week Status
   BL TimeSyncOK;                // Is Time-Synchronisation OK ?
                                 //    (ie there is no millisecond ambiguity between
                                 //     the code measurements and the local TOW)
   U2 WeekNo;                    // Measurement local GPS Week Number including the
                                 //    1024 week rollovers.  Zero if not set.
   R8 TOW;                       // Measurement local GPS Time of Week [seconds]
   R8 EstClkBias;                // Estimated Local GPS Receiver Clock Bias [seconds],
                                 //    if known (for information only)
   U4 EFSP_TTag;                 // External Frame Sync Pulse local baseband millisecond
                                 //    time-tag [milliseconds].  Zero if no pulse.
   U2 EFSP_Phase;                // External Frame Sync Pulse local baseband time-tag
                                 //    sub-millisecond 16-bit phase [range 00..0xFFFF]
                                 //
   U1 SV[GN_GPS_TMCH];           // Satellite ID (PRN) on this channel
   U1 SNR[GN_GPS_TMCH];          // Signal-to-Noise Ratio  [range 0 to 60 dBHz]
   U1 CNO[GN_GPS_TMCH];          // Carrier-to-Noise Ratio [range 0 to 60 dBHz]
                                 //   (only set if there is a valid carrier phase lock)
   U1 JNR[GN_GPS_TMCH];          // Jammer-to-Noise Ratio  [range 0 to 60 dBHz]
                                 //   (includes Jammers due cross & auto correlations)
   e_GN_GPS_TMQI QI[GN_GPS_TMCH];// Channel measurement Quality Indicator
   U1 SF_Sync_St[GN_GPS_TMCH];   // Channel Subframe Synchronisation State ?
                                 //    0 None             [Code ZCNTS:CHIPS invalid]
                                 //    1 Approx Bit       [Code ZCNTS:CHIPS%20  +/- 3 mS]
                                 //    2 Approx Subframe  [Code ZCNTS:CHIPS +/- 3 mS]
                                 //    3 Exact            [Code ZCNTS:CHIPS exact]
   U2 CodeLock[GN_GPS_TMCH];     // Code Loop Lock counter          [0.01s, max 64000]
   U2 FreqLock[GN_GPS_TMCH];     // Frequency Lock Loop counter     [0.01s, max 64000]
   U2 CarrLock[GN_GPS_TMCH];     // Carrier Phase Loop Lock counter [0.01s, max 64000]
   R8 Pseudorange[GN_GPS_TMCH];  // Pseudorange measurements [m]
   R4 Doppler[GN_GPS_TMCH];      // Doppler frequency measurements [Hz]
   R8 CarrierPhase[GN_GPS_TMCH]; // Carrier Phase Measurements [cycles]

} s_GN_GPS_Track_Meas;           // GN GPS Tracking Measurement Data


//*****************************************************************************
//
// GN_GPS Tracking Measurement External Interface API function prototypes
//
//*****************************************************************************

// GN GPS API Function to Get the Tracking Measuremenmt data.
// Returns TRUE if the Tracking Measurement Data are valid (ie the strcture has
// been populated.  The Tracking Measurement Quality Indicatros and Lock counters
// need to be checked on a per measurememt basis.

BL GN_GPS_Get_Track_Meas(
   s_GN_GPS_Track_Meas  *p_TM ); // i - Pointer to where the host software can
                                 //        get the Tracking Measurement data from


//*****************************************************************************

#ifdef __cplusplus
   }     // extern "C"
#endif

#endif   // GN_GPS_TM_API_H
