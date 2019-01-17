
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

FILE NAME    : temperature.c

AUTHOR       : Ken Ho

DESCRIPTION  : 


Revision control header:
$Id: CLK/temperature.c 1.4 2011/02/07 14:03:41PST Kenneth Ho (kho) Exp  $

******************************************************************************
*/



/*--------------------------------------------------------------------------*/
/*                         INCLUDE FILES                                    */
/*--------------------------------------------------------------------------*/
#include "target.h"
#include "sc_types.h"
#include "sc_servo_api.h"
#include "PTP/PTPdef.h"
#include <ctype.h>
#include <string.h>
#include <math.h>
#include "CLK/clk_private.h"
#include "CLK/ptp.h"
#include "CLK/CLKflt.h"
#include "DBG/DBG.h"
#include "logger.h"
#include "sc_ptp_servo.h"
#include "temperature.h"


/*--------------------------------------------------------------------------*/
/*                              defines                                     */
/*--------------------------------------------------------------------------*/



/*--------------------------------------------------------------------------*/
/*                              types                                       */
/*--------------------------------------------------------------------------*/


/*--------------------------------------------------------------------------*/
/*                              data                                        */
/*--------------------------------------------------------------------------*/
TEMPERATURE_STRUCT temp_stats;
static UINT8 temperature_has_been_touched = FALSE;



/*--------------------------------------------------------------------------*/
/*                              functions                                   */
/*--------------------------------------------------------------------------*/
extern UINT8 Get_Temp_Reading(float *reading);
extern UINT8 Get_Current_Temp(float *reading);
#if 0
void temperature_calc()
{
	float ctemp;
	
	
//	TODO make 600 when calling analysis as separate function
	if((temp_stats.tc_indx) < 600) //five minute collection interval
	{
		temp_stats.tc_indx++;
		Get_Current_Temp( &ctemp);	
		temp_stats.tes[temp_stats.t_indx].t_sum +=(double)(ctemp);
		if(ctemp > (temp_stats.tes[temp_stats.t_indx].t_max))temp_stats.tes[temp_stats.t_indx].t_max= (double)ctemp;		
		else if(ctemp < (temp_stats.tes[temp_stats.t_indx].t_min))temp_stats.tes[temp_stats.t_indx].t_min= (double)ctemp;		
//		debug_printf(GEORGE_PTP_PRT, "temperature collection: temp= %le, count=%ld \n",
//		(double)ctemp,temp_stats.tc_indx);
		
	}
}
#endif
///////////////////////////////////////////////////////////////////////
// This function initializes Temperature data
///////////////////////////////////////////////////////////////////////
void init_temperature()
{
	UINT8 i;

	temperature_has_been_touched = FALSE;

	temp_stats.v5min_flag=0;
	temp_stats.v60min_flag=0;
	temp_stats.tc_indx=0;		
	temp_stats.t_indx= 0;
	temp_stats.t_var_5min= 0.25;
	temp_stats.t_var_60min=	0.25;	
	for(i=0;i<128;i++)
	{	
		temp_stats.tes[i].t_max= MINTEMP;
		temp_stats.tes[i].t_min= MAXTEMP;
		temp_stats.tes[i].t_max_hold= MINTEMP;
		temp_stats.tes[i].t_min_hold= MAXTEMP;
		temp_stats.tes[i].t_avg= ZEROF;
		temp_stats.tes[i].t_sum= ZEROF;
	}	
	
}
#if 0
void temperature_analysis()
{
#if 1 //skip until testing complete
	double dtemp;
	UINT8 vindx;
	
	if((temp_stats.tc_indx) >299 ) //five minute collection interval
//	if((temp_stats.tc_indx) >7 ) //hyper fast analysis for debug
	{
		temperature_has_been_touched = TRUE;

		temp_stats.tes[temp_stats.t_indx].t_avg=temp_stats.tes[temp_stats.t_indx].t_sum/(double)(temp_stats.tc_indx);
		temp_stats.tc_indx=0;
		temp_stats.tes[temp_stats.t_indx].t_sum= ZEROF;
		//TODO update stability stats
		if(temp_stats.v5min_flag)
		{
			vindx= (temp_stats.t_indx+127)%128;
			dtemp= 	temp_stats.tes[temp_stats.t_indx].t_avg;
			dtemp= (dtemp - temp_stats.tes[vindx].t_avg)/2.0;	
			temp_stats.t_var_5min= (0.9*temp_stats.t_var_5min) + (0.1* dtemp*dtemp);
		}
		if(temp_stats.v60min_flag)
		{
			vindx= (temp_stats.t_indx+116)%128;
			dtemp= 	temp_stats.tes[temp_stats.t_indx].t_avg;
			dtemp= (dtemp - temp_stats.tes[vindx].t_avg)/2.0;	
			temp_stats.t_var_60min= (0.98*temp_stats.t_var_60min) + (0.02* dtemp*dtemp);
		}
		temp_stats.tes[temp_stats.t_indx].t_max_hold=temp_stats.tes[temp_stats.t_indx].t_max;
		temp_stats.tes[temp_stats.t_indx].t_min_hold=temp_stats.tes[temp_stats.t_indx].t_min;
		debug_printf(GEORGE_PTP_PRT, "temperature data: %ld,indx:%ld, max:%le, min:%le, avg:%le, 5_min_var:%le, 60_min_var:%le  \n",
//	        FLL_Elapse_Sec_Count,
	        temp_stats.t_indx,
	        temp_stats.tes[temp_stats.t_indx].t_max,
	        temp_stats.tes[temp_stats.t_indx].t_min,
	        temp_stats.tes[temp_stats.t_indx].t_avg,
			temp_stats.t_var_5min,
			temp_stats.t_var_60min				        
	        );
		//clear max and min and average of next point
		if(temp_stats.t_indx<128)
		{
			temp_stats.t_indx= ((temp_stats.t_indx+1)%128);
			if(temp_stats.t_indx>12)
			{
				temp_stats.v60min_flag=1;
			}
		}
		else
		{
			temp_stats.t_indx= 0;
		}	
		
		temp_stats.tes[temp_stats.t_indx].t_max= MINTEMP;
		temp_stats.tes[temp_stats.t_indx].t_min= MAXTEMP;
		temp_stats.v5min_flag=1;
		
	}	
#endif
}
#endif

UINT8 Temperature_data_ready(void)
{
	return temperature_has_been_touched;
}
