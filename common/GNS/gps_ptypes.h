
//****************************************************************************
// GloNav GPS Technology
// Copyright (C) 2001-2008 GloNav Ltd.
// March House, London Rd, Daventry, Northants, UK.
//
// Filename gps_ptypes.h
//
// $Header: GNS/gps_ptypes.h 1.1 2011/01/07 13:31:12PST Daniel Brown (dbrown) Exp  $
// $Locker:  $
//****************************************************************************
//
// ARM 32-bit RISC platform primitive typedefs


#ifndef GPS_PTYPES_H
#define GPS_PTYPES_H


//****************************************************************************
// GPS 4500 Build-specific option definitions


//****************************************************************************
// If the Nav Debug data is expected to be viewed in real-time, for example
// in ProComm, the following Terminal Control characters may best be defined.

#define _HOME_  ""
#define _CLEAR_ ""
#define _ERASE_ ""


//****************************************************************************
// On this target va_arg() needs I4 type for CH arguments.

#define VA_ARG_NEEDS_I4_FOR_CH


//****************************************************************************
// If there is no sign of valid Comms data from the GPS ME, the High-Level
// software will repeatedly send a Wake-Up command. The following parameter
// defines the interval between these commands. If a value of 0 is defined,
// a wake-up command will be sent every time the GN_GPS_Update function is
// called by the host.

#define WAKEUP_TX_INT      0              // 20 * 50 ms = 1 s


//****************************************************************************
// Define the Default Debug Enabled flags

#define DEFAULT_NAV_DEBUG   0x0001        // Default basic debug level
#define DEFAULT_GNB_DEBUG   0x0001        // Default basic debug level
#define DEFAULT_EVENT_LOG   0x0001        // Default basic debug level


//****************************************************************************


//--------------------------------------------------------------------------

// Unsigned integer types
typedef unsigned char   U1;         // 1 byte unsigned numerical value
typedef unsigned short  U2;         // 2 byte unsigned numerical value
typedef unsigned int    U4;         // 4 byte unsigned numerical value

// Signed integer types
typedef signed   char   I1;         // 1 byte signed numerical value
typedef signed   short  I2;         // 2 byte signed numerical value
typedef signed   int    I4;         // 4 byte signed numerical value

// Real floating point number, single and double precision
typedef float           R4;         // 4 byte floating point value
typedef double          R8;         // 8 byte floating point value

// Define ASCII character type
typedef char            CH;         // ASCII character

// Boolean / Logical type
typedef unsigned char   BL;         // Boolean logical (TRUE or FALSE only)
typedef unsigned char   L1;         // 1 byte logical  (TRUE or FALSE only)
typedef unsigned short  L2;         // 2 byte logical  (TRUE or FALSE only)
typedef unsigned int    L4;         // 4 byte logical  (TRUE or FALSE only)


//-----------------------------------------------------------------------------

// Ensure TRUE and FALSE are defined
#ifndef TRUE
   #define TRUE  1
#endif

#ifndef FALSE
   #define FALSE 0
#endif

// Ensure NULL is defined
#ifndef NULL
   #define NULL (void*)0
#endif


//-----------------------------------------------------------------------------

// Define the use of inline function types as being possible
#define INLINE __inline


//*****************************************************************************

#endif   // GPS_PTYPES_H

// end of file
