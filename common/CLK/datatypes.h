/**************************************/
/* Type definitions                   */
/**************************************/

#ifndef __INCLUDE_OPENTCP_DATATYPES_H__
#define __INCLUDE_OPENTCP_DATATYPES_H__

#include "sc_types.h"

#define BYTE 	unsigned char		//8 bit unsigned
#define WORD 	unsigned short int	//16 bit unsigned
#define LWORD	unsigned long int	//32 bit unsigned

#ifndef UINT8
#define UINT8	unsigned char
#endif
#ifndef INT8
#define INT8	signed char
#endif
#ifndef UINT16
#define	UINT16	unsigned short int
#endif
#ifndef INT16
#define INT16	signed short int
#endif
#ifndef UINT32 
#define UINT32	unsigned long int
#endif
#ifndef INT32
#define INT32 	signed long
#endif
#ifndef UINT64 
#define UINT64 	unsigned long long int
#endif
#ifndef INT64 
#define INT64 	signed long long int
#endif
#ifndef FLOAT
#define FLOAT 	float
#endif
#ifndef FLOAT32
#define FLOAT32	float
#endif
#ifndef boolean
#define boolean unsigned char
#endif
#ifndef BOOLEAN
#define BOOLEAN boolean
#endif
#ifndef uint_32
#define uint_32 unsigned long int
#endif
#ifndef int_32
#define int_32 long
#endif
#ifndef uint_16
#define uint_16 unsigned short int
#endif
#ifndef int_16
#define int_16 short int
#endif
#ifndef uint_8
#define uint_8 unsigned char
#endif
#ifndef int_8
#define int_8 char
#endif
#define __u8 unsigned char
#define __u16 unsigned short int
#define __u32 unsigned long int

#define u8 unsigned char
#define u16 unsigned short int
#define u32 unsigned long int

//#ifndef DEFINED_uint32
//typedef unsigned char		uint8;  /*  8 bits */
//typedef unsigned short int	uint16; /* 16 bits */
//typedef unsigned long int	uint32; /* 32 bits */
//#define DEFINED_uint32
//#endif
	
#endif
