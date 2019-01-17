//****************************************************************************
// epl_types.h
// 
// Copyright (c) 2007 National Semiconductor Corporation.
// All Rights Reserved
// 
// This is the EPL base type definitions header file.
//****************************************************************************


// Note: On platforms where the natural integer size is less then 32-bits
// in size (eg 16-bit platforms), NS_UINT and NS_SINT must be defined as a 
// data type no less than 32-bits in size.
typedef void                NS_VOID;
typedef unsigned int        NS_UINT;    // unsigned variable sized 
typedef int                 NS_SINT;    // signed variable sized 
typedef unsigned char       NS_UINT8;   // unsigned 8-bit fixed 
typedef char                NS_SINT8;   // signed 8-bit fixed 
typedef unsigned short int  NS_UINT16;  // unsigned 16-bit fixed 
typedef short int           NS_SINT16;  // signed 16-bit fixed 
typedef unsigned long int   NS_UINT32;  // unsigned 32-bit fixed
typedef long int            NS_SINT32;  // signed 32-bit fixed
typedef unsigned char       NS_CHAR;
typedef NS_UINT             NS_BOOL;    // TRUE or FALSE

#ifndef TRUE
#define TRUE    1
#endif
#ifndef FALSE
#define FALSE   0
#endif
#ifndef IN
#define IN
#endif
#ifndef OUT
#define OUT
#endif
#ifndef NULL
#define NULL ((void*)0)
#endif

