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

FILE NAME    : GPS.c

AUTHOR       : Daniel Brown

DESCRIPTION  : 

Generic GPS layer that to interface between SoftClient and Specific GPS 
engine (eg, Furuno, GNS7560)

Revision control header:
$Id: GPS/GPS.c 1.6 2012/04/23 15:18:14PDT German Alvarez (galvarez) Exp  $

******************************************************************************
*/



/*--------------------------------------------------------------------------*/
/*                         INCLUDE FILES                                    */
/*--------------------------------------------------------------------------*/
#include <stdlib.h>
#include <string.h>
#include "GNS/GN_GPS_Task.h"
#include "GPS/GPS.h"
#include "THD/THD.h"
#include "sc_api.h"


/*--------------------------------------------------------------------------*/
/*                              defines                                     */
/*--------------------------------------------------------------------------*/

/* maximum number of GPS engines defined */
#define NUM_GPS_ENGINES 2

/* GPS engine status */
#define k_GPS_STS_STOP  (0u) /* stopped status */
#define k_GPS_STS_INIT  (1u) /* initializing status */
#define k_GPS_STS_RUN   (2u) /* running status */

/*--------------------------------------------------------------------------*/
/*                              types                                       */
/*--------------------------------------------------------------------------*/


/*--------------------------------------------------------------------------*/
/*                              data                                        */
/*--------------------------------------------------------------------------*/

/* gps engine status variable */
static UINT8  GPS_b_engSts = k_GPS_STS_STOP;

/* gps engine initialization function pointer array */
static BOOLEAN (*pfa_gpsEngInit[NUM_GPS_ENGINES])(void *, UINT16) = 
   {
   NULL,                                        /* Furuno GPS initialization function */
   (BL (*)(void *, UINT16))&GN_GPS_Task_Start   /* GNS 7560 GPS initialization function */
   };

/* gps engine shutdown function pointer array */
static BOOLEAN (*pfa_gpsEngStop[NUM_GPS_ENGINES])(void) = 
   {
   NULL,                               /* Furuno GPS shutdown function */
   (BL (*)(void))&GN_GPS_Task_Stop     /* GNS 7560 GPS shutdown function */
   };

/* gps engine run function pointer array */
static BOOLEAN (*pfa_gpsEngRun[NUM_GPS_ENGINES])(void) = 
   {
   NULL,                               /* Furuno GPS run function */
   (BL (*)(void))&GN_GPS_Task_Run      /* GNS 7560 GPS run function */
   };

/* gps engine status function pointer array */
static BOOLEAN (*pfa_gpsEngStat[NUM_GPS_ENGINES])(void *) = 
   {
   NULL,                               /* Furuno GPS status function */
   (BL (*)(void *))&GN_GPS_Stat        /* GNS 7560 GPS status function */
   };

/* gps engine status function pointer array */
static BOOLEAN (*pfa_gpsEngValid[NUM_GPS_ENGINES])(void) = 
   {
   NULL,                               /* Furuno GPS status function */
   (BL (*)(void))&GN_GPS_Valid         /* GNS 7560 GPS status function */
   };

/* array of GPS engine task interval times in msec units */
static UINT32 wa_gpsInterval[NUM_GPS_ENGINES] = 
   {
   1000,                /* Furuno GPS task time, 1 sec */
   50                   /* GNS 7560 task time, 50 msecs */
   };

/* gps engine shutdown and run function pointers */
static BL (*pf_gpsEngInit)(void *, UINT16)   = NULL;
static BL (*pf_gpsEngStop)(void)             = NULL;
static BL (*pf_gpsEngRun)(void)              = NULL;
static BL (*pf_gpsEngStat)(void *)           = NULL;
static BL (*pf_gpsEngValid)(int chan)            = NULL;

/*--------------------------------------------------------------------------*/
/*                              functions                                   */
/*--------------------------------------------------------------------------*/


/***********************************************************************
** 
** Function    : GPS_Init
**
** Description : This function initializes the GPS engine.
**
** Parameters  : -
**
** Returnvalue : TRUE   - function succeeded
**               FALSE  - function failed
**
** Remarks     : -
**
***********************************************************************/
BOOLEAN GPS_Init (SC_t_GPS_GeneralConfig *ps_gpsGenCfg)
{
   /* if GPS engine is not already running */
   if (GPS_b_engSts != k_GPS_STS_RUN)
   {
      /* set status to INIT */
      GPS_b_engSts = k_GPS_STS_INIT;

      /* Check whether user supplied receiver type is a valid array index */
      if ((ps_gpsGenCfg->e_rcvrType) >= 0 && (ps_gpsGenCfg->e_rcvrType) < NUM_GPS_ENGINES)
      {
         /* Check whether user supplied receiver type has a valid function pointer */
         if (pfa_gpsEngInit[ps_gpsGenCfg->e_rcvrType] != NULL)
         {
            /* assign GPS engine-specific init function pointer */
            pf_gpsEngInit = pfa_gpsEngInit[ps_gpsGenCfg->e_rcvrType];

            // Callback to SC API user function
            if (SC_InitGpsIf() >= 0)
            {
               /* Call GPS engine-specific init function and check return value */
               if ((*pf_gpsEngInit)(ps_gpsGenCfg, wa_gpsInterval[ps_gpsGenCfg->e_rcvrType]) == TRUE)
               {
                  /* assign GPS engine-specific shutdown function pointer */
                  pf_gpsEngStop = pfa_gpsEngStop[ps_gpsGenCfg->e_rcvrType];

                  /* assign GPS engine-specific run function pointer */
                  pf_gpsEngRun = pfa_gpsEngRun[ps_gpsGenCfg->e_rcvrType];

                  /* assign GPS engine-specific status function pointer */
                  pf_gpsEngStat = pfa_gpsEngStat[ps_gpsGenCfg->e_rcvrType];

                  /* assign GPS engine-specific valid function pointer */
                  pf_gpsEngValid = pfa_gpsEngValid[ps_gpsGenCfg->e_rcvrType];

                  /* initialize the GPS task interval rate in msecs */
                  THD_InitGpsInterval(wa_gpsInterval[ps_gpsGenCfg->e_rcvrType]);

                  /* set engine status to RUN */
                  GPS_b_engSts = k_GPS_STS_RUN;
               }
               else
               {
                  /* Close the interface */
                  SC_CloseGpsIf();
                  /* error  out */
                  return(FALSE);
               }
            }
            else
            {
               return(FALSE);
            }   
         }
         else
         {
            return (FALSE);
         }
      }
      else
      {
         return (FALSE);
      }
   }
   else
   {
      return (FALSE);
   }

   return (TRUE);
} /* end function GPS_Init */

/***********************************************************************
** 
** Function    : GPS_Close
**
** Description : This function stops the GPS engine.
**
** Parameters  : -
**
** Returnvalue : -
**
** Remarks     : -
**
***********************************************************************/
void GPS_Close (void)
{
   /* check current status */
   if( (GPS_b_engSts == k_GPS_STS_RUN) || (GPS_b_engSts == k_GPS_STS_INIT) )
   {
      /* Check whether user supplied receiver type has a valid function pointer */
      if (pf_gpsEngStop != NULL)
      {
         /* Call GPS engine-specific close function */
         (*pf_gpsEngStop)();

         /* Call user API function */
         SC_CloseGpsIf();
      }

      /* set gps engine status to stop */
      GPS_b_engSts = k_GPS_STS_STOP;
   }
} /* end function GPS_Close */

/***********************************************************************
**  
** Function    : GPS_Main
**  
** Description : The main function of the GPS engine. Its call interval
**               is set by its calling functions and is determined at 
**               configuration time by the type of GPS engine.
**  
** Parameters  : None
**               
** Returnvalue : TRUE          - GPS engine is still running
**               FALSE         - GPS engine stopped
** 
** Remarks     : Function is not reentrant.
**  
***********************************************************************/
BOOLEAN GPS_Main (void)
{
   BOOLEAN o_ret = FALSE;

   /* check current status */  
   if( GPS_b_engSts == k_GPS_STS_RUN )
   {
      /* Check whether user supplied receiver type has a valid function pointer */
      if (pf_gpsEngRun != NULL)
      {
         /* Call GPS engine-specific init function and check return value */
         o_ret = (*pf_gpsEngRun)();   
      }
   }
   return o_ret;
} /* end function GPS_Main */

/***********************************************************************
** 
** Function    : GPS_Stat
**
** Description : This function gets status GPS engine.
**
** Parameters  : -
**
** Returnvalue : TRUE   - function succeeded
**               FALSE  - function failed
**
** Remarks     : -
**
***********************************************************************/
BOOLEAN GPS_Stat (SC_t_GPS_Status *ps_gpsStatus)
{
   BOOLEAN o_ret = FALSE;

   /* check current status */  
   if( GPS_b_engSts == k_GPS_STS_RUN )
   {
      /* Check whether user supplied receiver type has a valid function pointer */
      if (pf_gpsEngStat != NULL)
      {
         /* Call GPS engine-specific status function and check return value */
         o_ret = (*pf_gpsEngStat)(ps_gpsStatus);   
      }
   }
   return o_ret;
} /* end function GPS_Stat */

/***********************************************************************
** 
** Function    : GPS_Valid
**
** Description : This function gets the validity status of the GPS engine (typically
**               whether the engine is tracking or not.
**
** Parameters  : -
**
** Returnvalue : TRUE   - GPS status is valid
**               FALSE  - GPS status is NOT valid
**
** Remarks     : -
**
***********************************************************************/
BOOLEAN GPS_Valid(int chan)
{
   BOOLEAN o_ret = FALSE;
   /* check current status */  
   if( GPS_b_engSts == k_GPS_STS_RUN )
   {
      /* Check whether user supplied receiver type has a valid function pointer */
      if (pf_gpsEngValid != NULL)
      {
         /* Call GPS engine-specific valid function and check return value */
         o_ret = (*pf_gpsEngValid)(chan);   
      }
   }

   return o_ret;
} /* end function GPS_Valid */





