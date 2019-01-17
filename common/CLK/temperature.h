
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

FILE NAME    : temperature.h

AUTHOR       : Ken Ho

DESCRIPTION  : 

High level description here..,

Revision control header:
$Id: CLK/temperature.h 1.1 2010/05/26 15:00:25PDT Kenneth Ho (kho) Exp  $

******************************************************************************
*/


/* Prevent file from being included multiple times. */
#ifndef H_temperature_h
#define H_temperature_h


/*****************************************************************************/
/*                       ***INCLUDES Definitions***                          */
/*  Nested includes should not be used as a general practice.  If there is a */
/*  good reason to include an include within an include, do so here. 	     */
/*****************************************************************************/
#define MAXTEMP 150.0
#define MINTEMP -55.0
#define ZEROF (0.0) 



/*****************************************************************************/
/*                       ***CONSTANT Definitions***                          */
/*  This section should be used to define constants (#defines) and macros    */
/*  that can be used by any module that includes this file.                  */
/*****************************************************************************/

/*****************************************************************************/
/*                       ***Data Type Specifications***                      */
/*  This section should be used to specify data types including structures,  */
/*  enumerations, unions, and redefining individual data types.              */
/*****************************************************************************/
typedef struct 
{
	double t_avg;  
	double t_min,t_min_hold;
	double t_max,t_max_hold;
	double t_sum;
}
 TEMPERATURE_ELEMENT_STRUCT;

typedef struct 
{
	double	t_var_5min; //5 min temperature stability estimate
	double	t_var_60min;// 1 hour stability estimate
	UINT8  t_indx; // current index in temperature circular queue
	UINT16 tc_indx; // 5 minute 0 to 299 temperature data collection index
	UINT8 	v5min_flag;
	UINT8 	v60min_flag;
	TEMPERATURE_ELEMENT_STRUCT tes[128]; //temperature stability queue	
}
 TEMPERATURE_STRUCT;

/*****************************************************************************/
/*                       ***Public Data Section***                           */
/*  Global variables, if used, should be defined in a ".c" source file.      */
/*  The scope of a global may be extended to other modules by declaring it as*/
/*  as an extern here. 			                                     */
/*****************************************************************************/
extern TEMPERATURE_STRUCT temp_stats;

/*****************************************************************************/
/*                       ***Function Prototype Section***                    */
/*  This section to be used to prototype public functions.                   */
/*                                                                           */
/*  Descriptions of parameters and return code should be given for each      */
/*  function.                                                                */
/*****************************************************************************/

extern UINT8 Get_Temp_Reading(float *reading);
extern UINT8 Get_Current_Temp(float *reading);
extern void temperature_calc();
extern void init_temperature();
extern void temperature_analysis();
extern UINT8 Temperature_data_ready(void);
#endif /*  H_temperature_h */








