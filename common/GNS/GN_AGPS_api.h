
//****************************************************************************
// GloNav GPS Technology
// Copyright (C) 2001-2008 GloNav Ltd.
// March House, London Rd, Daventry, Northants, UK.
// All rights reserved
//
// Filename  GN_AGPS_api.h
//
// $Header: GNS/GN_AGPS_api.h 1.1 2011/01/07 13:30:44PST Daniel Brown (dbrown) Exp  $
// $Locker:  $
//****************************************************************************

#ifndef GN_AGPS_API_H
#define GN_AGPS_API_H

#ifdef __cplusplus
   extern "C" {
#endif

//****************************************************************************
//
//   GloNav A-GPS External Interface API Definitions
//
//*****************************************************************************

#include "gps_ptypes.h"

//*****************************************************************************
//
//  GN GPS External Interface API/Callback function related enumerated data
//  type and structure definitions.
//
//*****************************************************************************

// GN A-GPS GAD (Geographical Area Description) Position & Velocity Data.
// (See 3GPP TS 23.032 for a full description of the GAD fields.)

typedef struct GN_AGPS_GAD_Data  // GN A-GPS GAD (Geographical Area Description)
{
   U1 latitudeSign;           // Latitude sign [0=North, 1=South]
   U4 latitude;               // Latitude  [range 0..8388607 for 0..90 degrees]
   I4 longitude;              // Longitude [range -8388608..8388607 for -180..+180 degrees]
   I1 altitudeDirection;      // Altitude Direction  [0=Height, 1=Depth, -1=Not present]
   U2 altitude;               // Altitude  [range 0..32767 metres]
   U1 uncertaintySemiMajor;   // Horizontal Uncertainty Semi-Major Axis [range 0..127, 255=Unknown]
   U1 uncertaintySemiMinor;   // Horizontal Uncertainty Semi-Minor Axis [range 0..127, 255=Unknown]
   U1 orientationMajorAxis;   // Orientation of the Semi-Major Axis [range 0..89 x 2 degrees, 255=Unknown]
   U1 uncertaintyAltitude;    // Altitude Uncertainty  [range 0..127,  255=Unknown]
   U1 confidence;             // Position Confidence   [range 1..100%, 255=Unknown]

   I1 verdirect;              // Vertical direction [0=Upwards, 1=Downwards, -1=Not present]
   I2 bearing;                // Bearing            [range 0..359 degrees,   -1=Not present]
   I2 horspeed;               // Horizontal Speed   [range 0..65535 km/hr,   -1=Not present]
   I1 verspeed;               // Vertical Speed     [range 0..255 km/hr,     -1=Not present]
   U1 horuncertspeed;         // Horizontal Speed Uncertainty [range 0..254 km/hr, 255=Unknown]
   U1 veruncertspeed;         // Vertical Speed Uncertainty   [range 0..254 km/hr, 255=Unknown]

}  s_GN_AGPS_GAD_Data;           // GN A-GPS GAD (Geographical Area Description)

// GN_A-GPS Required "Quality of Position" criteria structure

typedef struct GN_AGPS_QoP       // GN A-GPS Required "Quality of Position" criteria
{
   U1 Horiz_Accuracy;         // Horizontal Accuracy Required [range 0..127, 255=Undefined]
   U1 Vert_Accuracy;          // Vertical   Accuracy Required [range 0..127, 255=Undefined]
   U4 Deadline_OS_Time_ms;    // "Position required by" deadline OS Time [ms]

}  s_GN_AGPS_QoP;                // GN A-GPS Required "Quality of Position" criteria

// GN A-GPS Satellite Measurement Report structures.

typedef struct GN_AGPS_Meas_El    // GN A-GPS Measurement Element for one SV
{
   U1 SatID;                  // Satellite ID (PRN) Number  [range 1..32]
   U1 SNR;                    // Satellite Signal to Noise Ratio [range 0..63 dBHz]
   I2 Doppler;                // Satellite Doppler [range -32768..32767 x 0.2 Hz]
   U2 Whole_Chips;            // Satellite Code Phase Whole Chips [range 0..1022 chips]
   U2 Fract_Chips;            // Satellite Code Phase Fractional Chips [range 0..1023 x 2^-10 chips]
   U1 MPath_Ind;              // Pseudorange Multipath Indicator [range 0..3], representing:
                              //    { Not measured, Low (<5m), Medium (<43m), High (>43m) }
   U1 PR_RMS_Err;             // Pseudorange RMS Error [range 0..63] consisting of:
                              //    3 bit Mantissa 'x' & 3 bit Exponent 'y' where:
                              //    RMS Error =  0.5 * (1 + x/8) * 2^y  metres

} s_GN_AGPS_Meas_El;             // GN A-GPS Measurement Element for one SV

typedef struct GN_AGPS_Meas      // GN A-GPS Measurement report
{
   U4 Meas_GPS_TOW;           // Measurement GPS Time of Week [range 0..604799999 ms]
   U1 Meas_GPS_TOW_Unc;       // Measurement GPS Time of Week Uncertainty (K) [range 0..127],
                              //    where Uncertainty =  0.0022*((1.18^K) - 1) [microseconds]
   I1 Delta_TOW;              // Difference in milliseconds [range 0..127] between
                              //    Meas_GPS_TOW and the millisecond part of the "SV time"
                              //    "tsv_1" of the first entry in the Satellite Measurement
                              //    Element array.  See 3GPP TS 44.031 for a full description.
                              //    Set to -1 = "Unknown", if it is not known exactly.
   I4 EFSP_dT_us;             // Delta-Time [us] to the last received External Frame
                              //    Sync Pulse,  (0x7FFFFFFF = No Pulse).
   U1 Num_Meas;               // Number of measurement elements to follow [range 0..16]
   U1 Quality;                // A GPS measurement set quality metric [range 0..255]
                              //    If the metric drops, stay with an older recent set.
                              //    If the metric reaches the maximum 255 then the
                              //    caller should not wait for anything better.
   s_GN_AGPS_Meas_El Meas[16];// Satellite Measurement Elements array

} s_GN_AGPS_Meas;                // GN A-GPS Measurement report

// GN A-GPS Assistance Data Requirements structure.

typedef struct GN_AGPS_Assist_Req   // GN A-GPS Assistance Data Requirements
{
   BL Ref_Time_Req;           // Is Reference Time assistance required ?
   BL Ref_Pos_Req;            // Is Reference Position assistance required ?
   BL Ion_Req;                // Is Ionosphere data assistance required ?
   BL UTC_Req;                // Is UTC mode data assistance required ?
   BL SV_Health_Req;          // Is the SV Health assistance required ?
   BL Bad_SV_List_Req;        // Is the Bad SV List assistance required ?
   BL Alm_Req;                // Is Almanac Assistance data required ?
   BL Eph_Req;                // Is Ephemeris Assistance data required ?

                              // The following can only be specified if navigating
                              // and only an ephemeris top up is being requested.

   U2 gpsWeek;                // Ephemeris target week [range 0..1023 weeks]
   U1 Toe;                    // Ephemeris target Toe  [range 0..167 hours]
   U1 Toe_Limit;              // Age Limit on the Ephemeris Toe [range 0..10]
   U1 Num_Sat;                // Number of Satellites in use [range 0..32]
   U1 SatID[32];              // Satellite ID's in use [range 1..32]
   U1 IODE[32];               // Ephemeris IODE for the satellites in use [range 0.255]

   BL Approx_Time_Known;      ///< Is Approximate Time Known to better than +/- 3s ?
   BL Approx_Pos_Known;       ///< Is Approximate Position Known to better than +/- 30km ?
   BL Ion_Known;              ///< Is Ionosphere data Known (but it may not be up to date) ?
   BL UTC_Known;              ///< Is UTC model data Known (but it may not be up to date) ?
   U1 Num_Sat_Alm;            ///< Number of Satellites with valid Almanacs (but it may not be up to date) ?
   U1 Num_Sat_Eph;            ///< Number of Satellites with valid Ephemeris (but it may not be up to date) ?

}  s_GN_AGPS_Assist_Req;         // GN A-GPS Assistance Data Requirements

// GN A-GPS Satellite Ephemeris Subframe data (see ICD-GPS-200) structure.

typedef struct GN_AGPS_Eph       // GN A-GPS Satellite Ephemeris Subframe Data
{
   U4 ZCount;                 // Reference Z-Count the Subframe was decoded,
                              // 0 if Unknown.  (See Note 1 below).

   U1 word[4+8+8][3];         // GPS Ephemeris & Clock Binary words from :
                              //   Subframe 1, 4 x 24 bit words,  3 & 8-10,
                              //   Subframe 2, 8 x 24 bit words,  3-10,
                              //   Subframe 3, 8 x 24 bit words,  3-10.
                              // The 24 bits of data have been shifted right
                              // 6 places to remove the GPS parity bits,
                              // and are now stored as 3 adjacent bytes.

}  s_GN_AGPS_Eph;                // GN A-GPS Satellite Ephemeris Subframe Data

// GN A-GPS Satellite Ephemeris Data Elements (see ICD-GPS-200) structure.

typedef struct GN_AGPS_Eph_El    // GN A-GPS Satellite Ephemeris Data Elements
{
   U1 SatID;                  // Satellite ID (PRN) Number                :  6 bits [1..32]
   U1 CodeOnL2;               // C/A or P on L2                           :  2 bits [0..3]
   U1 URA;                    // User Range Accuracy Index                :  4 bits [0..15]
   U1 SVHealth;               // Satellite Health Bits                    :  6 bits [0..63]
   U1 FitIntFlag;             // Fit Interval Flag                        :  1 bit  [0=4hrs, 1=6hrs]
   U1 AODA;                   // Age Of Data Offset                       :  5 bits [x 900 sec]
   I1 L2Pflag;                // L2 P Data Flag                           :  1 bit  [0..1]
   I1 TGD;                    // Total Group Delay                        :  8 bits [x 2^-31 sec]
   I1 af2;                    // SV Clock Drift Rate                      :  8 bits [x 2^-55 sec/sec2]
   U2 Week;                   // GPS Reference Week Number                : 10 bits [0..1023]
   U2 toc;                    // Clock Reference Time of Week             : 16 bits [x 2^4 sec]
   U2 toe;                    // Ephemeris Reference Time of Week         : 16 bits [x 2^4 sec]
   U2 IODC;                   // Issue Of Data Clock                      : 10 bits [0..1023]
   I2 af1;                    // SV Clock Drift                           : 16 bits [x 2^-43 sec/sec]
   I2 dn;                     // Delta n                                  : 16 bits [x 2^-43 semi-circles/sec]
   I2 IDot;                   // Rate of Inclination Angle                : 14 bits [x 2^-43 semi-circles/sec]
   I2 Crs;                    // Coefficient-Radius-sine                  : 16 bits [x 2^-5 meters]
   I2 Crc;                    // Coefficient-Radius-cosine                : 16 bits [x 2^-5 meters]
   I2 Cus;                    // Coefficient-Argument_of_Latitude-sine    : 16 bits [x 2^-29 radians]
   I2 Cuc;                    // Coefficient-Argument_of_Latitude-cosine  : 16 bits [x 2^-29 radians]
   I2 Cis;                    // Coefficient-Inclination-sine             : 16 bits [x 2^-29 radians]
   I2 Cic;                    // Coefficient-Inclination-cosine           : 16 bits [x 2^-29 radians]
   I4 af0;                    // SV Clock Bias                            : 22 bits [x 2^-31 sec]
   I4 M0;                     // Mean Anomaly                             : 32 bits [x 2^-31 semi-circles]
   U4 e;                      // Eccentricity                             : 32 bits [x 2^-33]
   U4 APowerHalf;             // (Semi-Major Axis)^1/2                    : 32 bits [x 2^-19 metres^1/2]
   I4 Omega0;                 // Longitude of the Ascending Node          : 32 bits [x 2^-31 semi-circles]
   I4 i0;                     // Inclination angle                        : 32 bits [x 2^-31 semi-circles]
   I4 w;                      // Argument of Perigee                      : 32 bits [x 2^-31 meters]
   I4 OmegaDot;               // Rate of Right Ascension                  : 24 bits [x 2^-43 semi-circles/sec]

}  s_GN_AGPS_Eph_El;             // GN A-GPS Satellite Ephemeris Data Elements

// GN A-GPS Satellite Almanac Subframe data (see ICD-GPS-200) structure.

typedef struct GN_AGPS_Alm       // GN A-GPS Satellite Almanac Subframe Data
{
   U4 ZCount;                 // Reference Z-Count the Subframe was decoded,
                              // 0 if Unknown.  (See Note 1 below).

   U2 WeekNo;                 // Almanac Reference Week.  (if known,
                              //    include the GPS 1024 week rollovers)
   U1 word[8][3];             // 8 GPS Almanac 24 bit words from words 3-10 of
                              //   either Subframe 4 pages 2-10,
                              //   or     Subframe 5 pages 1-24.
                              // The 24 bits of data have been shifted right
                              // 6 places to remove the GPS parity bits,
                              // and are now stored as 3 adjacent bytes.


}  s_GN_AGPS_Alm;                // GN A-GPS Satellite Almanac Subframe Data

// GN A-GPS Satellite Almanac Data Elements (see ICD-GPS-200) structure.

typedef struct GN_AGPS_Alm_El    // GN A-GPS Satellite Almanac Data Elements
{
   U1 WNa;                    // Almanac Reference Week      8 bits  [0..255]
   U1 SatID;                  // Satellite ID (PRN) Number   6 bits  [1..32]
   U1 SVHealth;               // Satellite Health Bits       8 bits  [0..255]
   U1 toa;                    // Reference Time of Week      8 bits  [x 2^12 sec]
   I2 af0;                    // SV Clock Bias              11 bits  [x 2^-20 seconds]
   I2 af1;                    // SV Clock Drift             11 bits  [x 2^-38 sec/sec]
   U2 e;                      // Eccentricity               16 bits  [x 2^-21]
   I2 delta_I;                // Delta Inclination Angle    16 bits  [x 2-19 semi-circles]
   I2 OmegaDot;               // Rate of Right Ascension    16 bits  [x 2^-38 semi-circles/sec]
   U4 APowerHalf;             // (Semi-Major Axis)^1/2      24 bits  [x 2^-11 meters^1/2]
   I4 Omega0;                 // Longitude of Asc. Node     24 bits  [x 2^-23 semi-circles]
   I4 w;                      // Argument of Perigee        24 bits  [x 2^-23 semi-circles]
   I4 M0;                     // Mean Anomaly               24 bits  [x 2^-23 semi-circles]

} s_GN_AGPS_Alm_El;              // GN A-GPS Satellite Almanac Data Elements

// GN A-GPS Klobuchar Ionospheric Model data (see ICD-GPS-200) structure.

typedef struct GN_AGPS_Ion       // GN A-GPS Klobuchar Ionospheric model terms
{
   U4 ZCount;                 // Reference Z-Count the Subframe was decoded,
                              // 0 if Unknown.  (See Note 1 below).

   I1 a0;                     // Klobuchar - alpha 0  (seconds)           / (2^-30)
   I1 a1;                     // Klobuchar - alpha 1  (sec/semi-circle)   / (2^-27/PI)
   I1 a2;                     // Klobuchar - alpha 2  (sec/semi-circle^2) / (2^-24/PI^2)
   I1 a3;                     // Klobuchar - alpha 3  (sec/semi-circle^3) / (2^-24/PI^3)
   I1 b0;                     // Klobuchar - beta 0   (seconds)           / (2^11)
   I1 b1;                     // Klobuchar - beta 1   (sec/semi-circle)   / (2^14/PI)
   I1 b2;                     // Klobuchar - beta 2   (sec/semi-circle^2) / (2^16/PI^2)
   I1 b3;                     // Klobuchar - beta 3   (sec/semi-circle^3) / (2^16/PI^3)

}  s_GN_AGPS_Ion;                // GN A-GPS Klobuchar Ionospheric model data

// GN A-GPS UTC (Universal Time Coordinated) Correction Model data
// (see ICD-GPS-200) structure.

typedef struct GN_AGPS_UTC       // GN A-GPS UTC Correction data
{
   U4 ZCount;                 // Reference Z-Count the Subframe was decoded,
                              // 0 if Unknown.  (See Note 1 below).

   I4 A1;                     // UTC parameter A1 (seconds/second)/(2^-50)
   I4 A0;                     // UTC parameter A0 (seconds)/(2^-30)
   U1 Tot;                    // UTC reference time of week (seconds)/(2^12)
   U1 WNt;                    // UTC reference week number (weeks)
   I1 dtLS;                   // UTC time difference due to leap seconds
                              //     before event (seconds)
   U1 WNLSF;                  // UTC week number when next leap second event
                              //     occurs (weeks)
   U1 DN;                     // UTC day of week when next leap second event
                              //     occurs (days)
   I1 dtLSF;                  // UTC time difference due to leap seconds after
                              //     event (seconds)

}  s_GN_AGPS_UTC;                // GN A-GPS UTC Correction data


// Note:
//
// 1.  U4 ZCount:  The 19 least significant bits of the Z-Count can be decoded
//     from the HOW (Handover Word) which is Word 2 of every Subframe.  This
//     gives the GPS Time of Week in units of 1.5 seconds.
//     The full 29 bit Z-Count has the 10-bit GPS Week Number as the 10 most
//     significant bits.  (See GPS-ICD-200 for a full description).
//     If the GPS Week Number portion is not known then GPS Time of Week
//     portion should still be input.
//     If neither portion are known then ZCount should be input as zero, but
//     this can limit the GN GPS Library's ability to time-out the data.


// GN A-GPS Reference GPS Time.

typedef struct GN_AGPS_Ref_Time    // GN A-GPS Reference GPS Time
{
   U4 OS_Time_ms;             // OS Time [milliseconds] when the Reference
                              //    GPS Time was obtained
   U4 TOW_ms;                 // Reference GPS Time of Week [milliseconds]
   U2 WeekNo;                 // Reference GPS Week Number,  (if known,
                              //    include the GPS 1024 week rollovers)
   U2 RMS_ms;                 // Reference RMS Accuracy [milliseconds]

}  s_GN_AGPS_Ref_Time;             // GN A-GPS Reference GPS Time


// GN A-GPS External Frame Sync Pulse Time for the leading edge of the pulse
// provided to the GPS Baseband EXT_FRAME_SYNC pin.

typedef struct GN_AGPS_EFSP_Time    // GN A-GPS External Frame Sync Pulse Time
{
   R8 TOW;                    // Frame Sync Pulse GPS Time of Week [seconds]
   U2 WeekNo;                 // Frame Sync Pulse GPS Week Number,  (if known,
                              //    include the GPS 1024 week rollovers)
   U4 Abs_RMS_us;             // Frame Sync Absolute Pulse RMS Accuracy [microseconds],
                              //    0xFFFFFFFF = Absolute Time is not known.
   U4 Rel_RMS_ns;             // Frame Sync Relative Pulse RMS Accuracy [nanoseconds],
                              //    0xFFFFFFFF = Relative Time to previous recent
                              //    pulses should not be used.
   U4 OS_Time_ms;             // OS Time [milliseconds] corresponding to the
                              //    approximate time of the pulse.
   BL MultiplePulses;         // Do we expect to see multiple pulses and messages?

}  s_GN_AGPS_EFSP_Time;             // GN A-GPS External Frame Sync Pulse Time

// Note:
//
// 2.  The 'MultiplePulses' flag is important, since it's used to decide if it's
//     necessary to try to make sure that the time data is associated with the correct
//     pulse. If only a single pulse and message are provided, or if the interval
//     between each pulse is greater than a few seconds, then there is very little
//     danger of the wrong set of time data being associated with the pulse.


// GN A-GPS Time Of Week Assistance Data.

typedef struct GN_AGPS_TOW_Assist_El  // GN A-GPS Time of Week Assistance Element
{
   U1 SatID;                  // Satellite ID (PRN) Number  [range 1..32]
   U1 TLM_Reserved;           // Satellite Subframe Word 1, Telemetry Message Reserved
                              //    2 bits  [range 0..3]
   U2 TLM_Word;               // Satellite Subframe Word 1, Telemetry Message Word
                              //    14 bits  [range 0..16383]
   U1 Anti_Spoof_flag;        // Satellite Subframe Word 2, Anti-Spoof flag  [range 0..1]
   U1 Alert_flag;             // Satellite Subframe Word 2, Alert flag  [range 0..1]

}  s_GN_AGPS_TOW_Assist_El;         // GN A-GPS Time of Week Assistance Element

typedef struct GN_AGPS_TOW_Assist   // GN A-GPS Time of Week Assistance Element
{
   U4 TOW_ms;                 // Reference GPS Time of Week [milliseconds]
   U1 Num_TOWA;               // Number of TOW Assist Elements to follow  [range 1..24]
   s_GN_AGPS_TOW_Assist_El TOWA[24];// Per Satellite TOW Assistance Elements

}  s_GN_AGPS_TOW_Assist;            // GN A-GPS Time of Week Assistance Element


// GN A-GPS WGS84 Clock Frequency Calibration data

typedef struct GN_AGPS_ClkFreq   // GN A-GPS WGS84 Geodetic Position data
{
   U4 OS_Time_ms;             // OS Time [milliseconds] when the Clock
                              //    Frequency Calibration was obtained
   R4 Calibration;            // Clock Frequency Calibration value [ppb]
   U4 RMS_ppb;                // Clock Frequency Calibration RMS Accuracy [ppb]

}  s_GN_AGPS_ClkFreq;            // GN A-GPS WGS84 Geodetic Position data


// GN A-GPS WGS84 Geodetic Reference Position data

typedef struct GN_AGPS_Ref_Pos   // GN A-GPS WGS84 Geodetic Reference Position data
{
   R4 Latitude;               // WGS84 Geodetic Latitude  [degrees]
   R4 Longitude;              // WGS84 Geodetic Longitude [degrees]
   R4 RMS_SMaj;               // Horizontal RMS Accuracy, Semi-Major axis [metres]
   R4 RMS_SMin;               // Horizontal RMS Accuracy, Semi-Minor axis [metres]
   I2 RMS_SMajBrg;            // Horizontal RMS Accuracy, Semi-Major axis Bearing [deg]
   BL Height_OK;              // Is the Height component available & OK to use ?
   R4 Height;                 // WGS84 Geodetic Ellipsoidal Height [metres]
   R4 RMS_Height;             // Height RMS Accuracy [metres]

}  s_GN_AGPS_Ref_Pos;            // GN A-GPS WGS84 Geodetic Reference Position data


// GN A-GPS Satellite Acquisition Assistance data set structures.

typedef struct GN_AGPS_AA_El     // GN A-GPS Acquisition Assistance Element for one SV
{
   U1 SatID;                  // Satellite ID (PRN) Number  [range 1..32]
   I2 Doppler_0;              // Satellite Doppler 0th order term [range -2048..2047 x 2.5 Hz]
   U1 Doppler_1;              // Satellite Doppler 1st order term [range 0..63 x 1/42 Hz/s]
                              //    offset by -42 to give values in the range -1.0 .. 0.5 Hz.
                              //    (Set to zero if unknown)
   U1 Doppler_Unc;            // Satellite Doppler Uncertainty [range 0..4] representing:
                              //    +/- { <=200,  <=100,  <=50,  <=25,  <=12.5 } Hz
                              //    (Set to zero if unknown)
   U2 Code_Phase;             // Satellite Code Phase [range 0..1022 chips]
   U1 Int_Code_Phase;         // Satellite Integer Code Phase since the last GPS bit edge
                              //    boundary [range 0..19 ms]
   U1 GPS_Bit_Num;            // Satellite GPS Bit Number (modulo 4) relative to GPS_TOW
   U1 Code_Window;            // Satellite Code Phase Search Window [range 0..15], representing:
                              //    +/- { 512,   1,   2,   3,   4,   6,    8,  12,
                              //           16,  24,  32,  48,  64,  96,  128, 196 } chips.
   U1 Azimuth;                // Satellite Azimuth   [range 0..31 x 11.25 degrees]
                              //    (Set to zero if unknown)
   U1 Elevation;              // Satellite Elevation [range 0..7  x 11.25 degrees]
                              //    (Set to zero if unknown)

} s_GN_AGPS_AA_El;               // GN A-GPS Acquisition Assistance Element for one SV

typedef struct GN_AGPS_Acq_Ass   // GN A-GPS Acquisition Assistance data set
{
   U4 Ref_GPS_TOW;            // Acq Assist Reference GPS Time of Week [range 0..604799999 ms]
   U1 Num_AA;                 // Number of Acq Assist elements to follow [range 0..16]
   s_GN_AGPS_AA_El AA[16];    // Satellite Acquisition Assistance Elements array

} s_GN_AGPS_Acq_Ass;             // GN A-GPS Acquisition Assistance data set



//*****************************************************************************
//
// GN A-GPS External Interface API function prototypes
//
//*****************************************************************************

//*****************************************************************************
// API Functions that may be called by the Host platform software.

// GN GPS API Function to request the latest GAD (Geographical Area Description)
// format Position and Velocity Data.
// (See 3GPP TS 23.032 for a full description of the GAD fields.)
// Returns TRUE if there is a new GPS Position Fix available.
// Returns FALSE if the current Position fix is not a GPS fix (eg is the input
// A-GPS Reference Position), or is still for the 'Prev_OS_Time_ms' passed in.

BL GN_AGPS_Get_GAD_Data(
   U4 *Prev_OS_Time_ms,             // io - OS Time [ms] for the previous data got, to
                                    //         avoid being given the same data again.
                                    //         A NULL pointer pointer disables this feature.
   U4 *GAD_Ref_TOW,                 //  o - GAD Data Reference GPS Time of Week [ms]
   I2 *GAD_Ref_TOW_Subms,           //  o - GAD Data Reference GPS Time of Week Sub-millisecond
                                    //         part [range 0..9999 x 0.0001 ms,  -1 = Unknown]
   I4 *EFSP_dT_us,                  //  o - Delta-Time [us] to the last received
                                    //         External Frame Sync Pulse,
                                    //         (0x7FFFFFFF = No Pulse).
   s_GN_AGPS_GAD_Data *p_GAD );     //  o - Pointer to the output GAD Data.


// GN A-GPS API Function to determine if the current published position fix can
// be treated as "Qualified" with respect to the given "Quality of Position", QoP.
// criteria.  If both the Horizontal and Vertical Accuracy QoP are "Unspecified"
// then the library internal default values will be used.  However, if the
// Horizontal Accuracy QoP is specified but the Vertical Accuracy QoP is not,
// then only the Horizontal component will be qualified.
// Returns TRUE if current position fix meets the "Quality of Position" criteria.

BL GN_AGPS_Qual_Pos(
   U4 *Prev_OS_Time_ms,             // io - OS Time [ms] for the previous data got, to
                                    //         avoid being given the same data again.
                                    //         A NULL pointer input disables this feature.
   s_GN_AGPS_QoP *p_QoP );          // i  - Required "Quality of Position" criteria.


// GN A-GPS API Function to Get Measurement Report data for cellular A-GPS
// MS-Assisted mode via the RRLP or RRC protocols.
// Returns TRUE if the Get was successful.
BL GN_AGPS_Get_Meas(
   U4 *Prev_OS_Time_ms,             // io - OS Time [ms] for the previous data got, to
                                    //         avoid being given the same data again.
                                    //         A NULL pointer input disables this feature.
   s_GN_AGPS_Meas *p_Meas );        // i  - Pointer to where the GPS Core software
                                    //         can get the Measurement data from.

// GN A-GPS API Function to Get a details of the GPS Libraries Assistance Data
// Requirements.  If a NULL pointer is provided as the input argument then this
// API will not provide the list of items requested in this call.
// Returns TRUE if A-GPS Assistance Data is Required.
BL GN_AGPS_Get_Assist_Req(
   s_GN_AGPS_Assist_Req *p_AR );    //  o - A-GPS Assistance Requirements.


// GN A-GPS API Function to Set (ie input) Ephemeris Subframe data for the
// specified satellite.  This ephemeris will only be used if a valid current
// broadcast ephemeris has not recently been obtained from the satellite itself.
// Returns TRUE if the Set was successful.
BL GN_AGPS_Set_Eph(
   U1 SV,                           // i  - Satellite identification (PRN) number
   s_GN_AGPS_Eph *p_Eph );          // i  - Pointer to where the GPS Core software
                                    //         can get the Ephemeris data from

// GN A-GPS API Function to Get Ephemeris Subframe data for the specified
// satellite.  The data returned is what is actually being used internally
// and could have recently been decoded from a live satellite signal.
// Returns TRUE if the Get was successfully.
BL GN_AGPS_Get_Eph(
   U1 SV,                           // i  - Satellite identification (PRN) number
   s_GN_AGPS_Eph *p_Eph );          //  o - Pointer to where the Host software
                                    //         can get the Ephemeris data from


// GN A-GPS API Function to Set (ie input) an A-GPS Ephemeris Data Elements record.
// This ephemeris will only be used if a valid current broadcast ephemeris has
// not recently been obtained from the satellite itself.
// Returns TRUE if the Set was successful.
BL GN_AGPS_Set_Eph_El(
   s_GN_AGPS_Eph_El *p_Eph_El );    // i  - Pointer to where the GPS Core software
                                    //         can get the Ephemeris data from


// GN A-GPS API Function to Get (ie output) Ephemeris Data Elements for the
// specified satellite.  The data returned is what is actually being used
// internally and could have recently been decoded from a live satellite signal.
// Returns TRUE if the Set was successful.
BL GN_AGPS_Get_Eph_El(
   U1 SV,                           // i  - Satellite identification (PRN) number
   s_GN_AGPS_Eph_El *p_Eph_El );    // o  - Pointer to where the Host software
                                    //         can get the Ephemeris data from


// GN A-GPS API Function to Set (ie input) the 4 Ephemeris Reserved words from
// Subframe 1, words 4,5,6 & 7.  This ephemeris will only be used if a valid current
// broadcast ephemeris has not recently been obtained from the satellite itself.
// Returns TRUE if the Set was successful.
BL GN_AGPS_Set_Eph_Res(
   U1 SV,                           // i  - Satellite identification (PRN) number
   U4 Eph_Res[4] );                 // i  - Ephemeris Reserved Words from Subframe 1
                                    //         [0] = 23 bits from Subframe 1, Word 4
                                    //         [1] = 24 bits from Subframe 1, Word 5
                                    //         [2] = 24 bits from Subframe 1, Word 6
                                    //         [3] = 16 bits from Subframe 1, Word 7


// GN A-GPS API Function to Get (ie output) the 4 Ephemeris Reserved words from
// Subframe 1, words 4,5,6 & 7 for the specified satellite.  The data returned is
// what is actually being used internally and could have recently been decoded
// from a live satellite signal.
// Returns TRUE if the Set was successful.
BL GN_AGPS_Get_Eph_Res(
   U1 SV,                           // i  - Satellite identification (PRN) number
   U4 Eph_Res[4] );                 // i  - Ephemeris Reserved Words from Subframe 1
                                    //         [0] = 23 bits from Subframe 1, Word 4
                                    //         [1] = 24 bits from Subframe 1, Word 5
                                    //         [2] = 24 bits from Subframe 1, Word 6
                                    //         [3] = 16 bits from Subframe 1, Word 7


// GN A-GPS API Function to Set (ie input) Almanac Subframe data for the
// specified satellite.  This almanac will only be used if a valid current
// broadcast almanac has not recently been obtained from the satellite itself.
// Returns TRUE if the Set was successful.
BL GN_AGPS_Set_Alm(
   U1 SV,                           // i  - Satellite identification (PRN) number
   s_GN_AGPS_Alm *p_Alm );          // i  - Pointer to where the GPS Core software
                                    //         can get the Almanac data from


// GN A-GPS API Function to Get Almanac Subframe data for the specified
// satellite.  The data returned is what is actually being used internally
// and could have recently been decoded from a live satellite signal.
// Returns TRUE if the Get was successfully.
BL GN_AGPS_Get_Alm(
   U1 SV,                           // i  - Satellite identification (PRN) number
   s_GN_AGPS_Alm *p_Alm );          //  o - Pointer to where the Host software
                                    //         can get the Almanac data from


// GN A-GPS API Function to Set (ie input) an A-GPS Almanac Data Elements record.
// This almanac will only be used if a valid current broadcast almanac has not
// recently been obtained from the satellite itself.
// Returns TRUE if the Set was successful.
BL GN_AGPS_Set_Alm_El(
   s_GN_AGPS_Alm_El *p_Alm_El );    // i  - Pointer to where the GPS Core software
                                    //         can get the Almanac data from


// GN A-GPS API Function to Get (ie output) Almanac Data Elements for the
// specified satellite.  The data returned is what is actually being used
// internally and could have recently been decoded from a live satellite signal.
// Returns TRUE if the Get was successful.
BL GN_AGPS_Get_Alm_El(
   U1 SV,                           // i  - Satellite identification (PRN) number
   s_GN_AGPS_Alm_El *p_Alm_El );    // o  - Pointer to where the Host software
                                    //         can get the Almanac data from


// GN A-GPS API Function to Set (ie input) Klobuchar Ionospheric Delay Model
// data.  This data will only be used if a valid current broadcast version has
// not recently been obtained from one of the tracked satellites.
// Returns TRUE if the Set was successful.
BL GN_AGPS_Set_Ion(
   s_GN_AGPS_Ion *p_Ion );          // i  - Pointer to where the GPS Core software
                                    //         can get the Ionosphere data from


// GN A-GPS API Function to Get Klobuchar Ionospheric Delay Model data.
// The data returned is what is actually being used internally and could have
// recently been decoded from a live satellite signal.
// Returns TRUE if the Get was successful.
BL GN_AGPS_Get_Ion(
   s_GN_AGPS_Ion *p_Ion );          // i  - Pointer to where the Host software
                                    //         can get the Ionosphere data from


// GN A-GPS API Function to Set (ie input) UTC Correction Model data.
// This data will only be used if a valid current broadcast version has not
// recently been obtained from one of the tracked satellites.
// Returns TRUE if the Set was successful.
BL GN_AGPS_Set_UTC(
   s_GN_AGPS_UTC *p_UTC );          // i  - Pointer to where the GPS Core software
                                    //         can get the UTC Correction data from


// GN A-GPS API Function to Get UTC Correction Model data.
// The data returned is what is actually being used internally and could have
// recently been decoded from a live satellite signal.
// Returns TRUE if the Get was successful.
BL GN_AGPS_Get_UTC(
   s_GN_AGPS_UTC *p_UTC );          // i  - Pointer to where the Host software
                                    //         can get the UTC Correction data from


// GN A-GPS API Function to Set (ie input) Satellite Health data.
// This input is treated like the other sources of satellite health data in
// that it only takes effect until a newer source of satellite health data is
// received, either off-air from the GPS satellites, or via an A-GPS API input.
// Returns TRUE if the Set was successful.
BL GN_AGPS_Set_SV_Health(
   U4 SV_Health_Mask );             // i  - Satellite Health bit Mask, where:
                                    //         bit 0..31  for  SV ID's 1..32
                                    //         1=Healthy, 0=Unhealthy.


// GN A-GPS API Function to Get Satellite Health data.
// The data returned is what is actually being used internally and could have
// recently been decoded from a live satellite signal.
// Returns TRUE if the Get was successful.
BL GN_AGPS_Get_SV_Health(
   U4 *SV_Health_Mask );            // i  - Satellite Health bit Mask, where:
                                    //         bit 0..31  for  SV ID's 1..32
                                    //         1=Healthy, 0=Unhealthy.


// GN A-GPS API Function to Set (ie input) a list of "bad" satellites which
// should not be used as part of the Navigation Solution.
// Each new Bad Satellite List provided via this API supercedes any previous
// list provided.  Therefore, and empty list implicitly sets all the satellites
// to be "Not known to be Bad".  As such, this list cannot be used to enable the
// use of a satellite which is marked as "Unhealthy" by the GPS constellation.
// Returns TRUE if the Set was successful.
BL GN_AGPS_Set_Bad_SV_List(
   U1 Num_Bad_SV,                   // i  - Number of "bad" satellites
   U1 Bad_SV_List[] );              // i  - List of "bad" Satellites ID (PRN)
                                    //         Numbers [1..32]


// GN A-GPS API Function to Set (ie input) the GPS Reference Time.
// Returns TRUE if the Set was successful.
BL GN_AGPS_Set_Ref_Time(
   s_GN_AGPS_Ref_Time *p_RTime );   // i  - Pointer to where the GPS Core software
                                    //         can get the Reference GPS Time
                                    //         data from.


// GN A-GPS API Function to Set (ie input) the precise GPS time corresponding
// to the leading edge of the last External Frame Sync Pulse input to the GPS
// Baseband EXT_FRAME_SYNC pin.
// Returns TRUE if the Set was successful.
BL GN_AGPS_Set_EFSP_Time(
   s_GN_AGPS_EFSP_Time *p_EFSPT );  // i  - Pointer to where the GPS Core software
                                    //         can get the Frame Counter Pulse
                                    //         GPS Time from.


// GN A-GPS API Function to Set (ie input) the GPS Time of Week Assistance Data.
// Returns TRUE if the Set was successful.
BL GN_AGPS_Set_TOW_Assist(
   s_GN_AGPS_TOW_Assist *p_TOWA );  // i  - Pointer to where the GPS Core software
                                    //         software can get the GPS Time of Week
                                    //         Assistance data from.


// GN A-GPS API Function to Set (ie input) the GPS Reference Clock Frequency
// Calibration data.
// Returns TRUE if the Set was successful.
BL GN_AGPS_Set_ClkFreq(
   s_GN_AGPS_ClkFreq *p_ClkFreq );  // i  - Pointer to where the GPS Core software
                                    //         can get the Clock Frequency
                                    //         Calibration data from


// GN A-GPS API Function to Set (ie input) WGS84 Geodetic Reference Position
// Information.
// Returns TRUE if the Set was successful.
BL GN_AGPS_Set_Ref_Pos(
   s_GN_AGPS_Ref_Pos *p_RPos );     // i  - Pointer to where the GPS Core software
                                    //         can get the Reference Position info from


// GN A-GPS API Function to Set (ie input) a GAD (Geographical Area Description)
// format WGS84 Reference Position.
// (See 3GPP TS 23.032 for a full description of the GAD fields.)
// Returns TRUE if the Set was successful.
BL GN_AGPS_Set_GAD_Ref_Pos(
   s_GN_AGPS_GAD_Data *p_RPos );    // i  - Pointer to where the GPS Core software
                                    //         can get the GAD Reference Position from.


// GN A-GPS API Function to Set (ie input) Acquisition Assistance data for
// cellular A-GPS MS-Assisted mode via the RRLP or RRC protocols.
// Returns TRUE if the Set was successful.
BL GN_AGPS_Set_Acq_Ass(
   s_GN_AGPS_Acq_Ass *p_AA );       // i  - Pointer to where the GPS Core software
                                    //         can get the Acq Assist data from.


//*****************************************************************************

#ifdef __cplusplus
   }     // extern "C"
#endif

#endif   // GN_AGPS_API_H
