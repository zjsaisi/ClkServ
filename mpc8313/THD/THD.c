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

FILE NAME    : THD.c

AUTHOR       : Jining Yang

DESCRIPTION  :

The functions in this file are used to run the threads in SoftClient.
	SC_GetRevInfo()
	SC_Init()
	SC_Run()
	SC_Shutdown()
	SC_Timer()


Revision control header:
$Id: THD/THD.c 1.17 2011/07/01 16:07:42PDT Daniel Brown (dbrown) Exp  $

******************************************************************************
*/



/*--------------------------------------------------------------------------*/
/*                         INCLUDE FILES                                    */
/*--------------------------------------------------------------------------*/
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>
#include "target.h"
#include "sc_types.h"
#include "PTP/PTPdef.h"
#include "NIF/NIF.h"
#include "PTP/PTP.h"
#include "SIS/SIS.h"
#include "sc_servo_api.h"
#include "DBG/DBG.h"
#include "GNS/GN_GPS_Task.h"
#include "GPS/GPS.h"
#include "TOD/tod.h"


/*--------------------------------------------------------------------------*/
/*                              defines                                     */
/*--------------------------------------------------------------------------*/
#define PTP_TASK_PRI            90      // highest
#define ONE_SEC_TASK_PRI        80      // lower than PTP_TASK_PRI
#define GPS_TASK_PRI            70      // lower than ONE_SEC_TASK_PRI
#define ONE_MIN_TASK_PRI        60      // lower than GPS_TASK_PRI

#define NSEC_PER_MSEC           1000000 // number of nsecs per millisecond

#define NSEC_PER_SEC            1000000000LL
#define NSEC_PER_100MS          100000000LL
/*--------------------------------------------------------------------------*/
/*                              types                                       */
/*--------------------------------------------------------------------------*/
static void *PtpTask(void *arg);
static void *OneSecTask(void *arg);
static void *AnalysisTask(void *arg);
#ifdef GPS_BUILD
static void *GpsTask(void *arg);
#endif


/*--------------------------------------------------------------------------*/
/*                              data                                        */
/*--------------------------------------------------------------------------*/
UINT32 l_timeResNsec = 4000000;
static BOOLEAN o_shutdown = FALSE;
static pthread_t s_ptpThread;
static pthread_t s_oneSecThread;
static pthread_t s_analysisThread;
#ifdef GPS_BUILD
static pthread_t s_gpsThread;
#endif
static sem_t semPtp;
static sem_t semOneSec;
static sem_t semAnalysis;
#ifdef GPS_BUILD
static sem_t semGps;
#endif
static BOOLEAN o_ptpRunComplete = TRUE;
static UINT32 l_ptpOverrunCount = 0;
static BOOLEAN o_oneSecRunComplete = TRUE;
#ifdef GPS_BUILD
static BOOLEAN o_gpsRunComplete = TRUE;
#endif
static UINT32 l_oneSecOverrunCount = 0;
#ifdef GPS_BUILD
static UINT32 l_gpsOverrunCount = 0;

/* GPS interval time [nsec], initialize to 1 sec in case GPS is disabled by
   user flag so that GPS task does not consume processor time. It reassigned
   by initialization function */
static UINT32 l_gpsIntervalTime = 1000000000;
#endif

/*--------------------------------------------------------------------------*/
/*                              functions                                   */
/*--------------------------------------------------------------------------*/
static UINT64 diff_nsec(const struct timespec start, const struct timespec end);



/*
----------------------------------------------------------------------------
                                THD_InitTimeRes()

Description:
This function creates the PTP thread, one second CLK thread and one minute
CLK thread. The also creates the semaphores.


Parameters:

Inputs
	w_rateMultiplier
	Clock rate multiplier


Outputs:
	None


Return value:
	 0: function succeeded
	-1: function failed
   -2: w_rateMultiplier is value of zero

-----------------------------------------------------------------------------
*/
int THD_InitTimeRes(UINT16 w_rateMultiplier)
{
    struct timespec res;
    if (clock_getres(CLOCK_REALTIME, &res) < 0)
        return -1;
    if (w_rateMultiplier == 0)
        return -2;
#if 0
    l_timeResNsec = res.tv_nsec * w_rateMultiplier;
#endif
    return 0;
}


/*
----------------------------------------------------------------------------
                                THD_Start()

Description:
This function creates the PTP thread, one second CLK thread and analysis thread.
This also creates the semaphores.


Parameters:

Inputs
	None


Outputs:
	None


Return value:
	 0: function succeeded
	-1: function failed

-----------------------------------------------------------------------------
*/
int THD_Start(void)
{
    /* Initialize semaphores */
    if (sem_init(&semPtp, 0, 0) < 0)
    {
        debug_printf(UNMASK_PRT, "THD: Init PTP thread semaphore failed\n");
        return -1;
    }
    if (sem_init(&semOneSec, 0, 0) < 0)
    {
        debug_printf(UNMASK_PRT, "THD: Init one second thread semaphore failed\n");
        sem_destroy(&semPtp);
        return -1;
    }
    if (sem_init(&semAnalysis, 0, 0) < 0)
    {
        debug_printf(UNMASK_PRT, "THD: Init analysis thread semaphore failed\n");
        sem_destroy(&semPtp);
        sem_destroy(&semOneSec);
        return -1;
    }
#ifdef GPS_BUILD
    if (sem_init(&semGps, 0, 0) < 0)
    {
        debug_printf(UNMASK_PRT, "THD: Init GPS thread semaphore failed\n");
        sem_destroy(&semPtp);
        sem_destroy(&semOneSec);
        sem_destroy(&semAnalysis);
        return -1;
    }
#endif
    /* Start up thread */
    if (pthread_create(&s_ptpThread, NULL, PtpTask, NULL) < 0)
    {
        debug_printf(UNMASK_PRT, "THD: Creating ptp thread failed\n");
        sem_destroy(&semPtp);
        sem_destroy(&semOneSec);
        sem_destroy(&semAnalysis);
#ifdef GPS_BUILD
        sem_destroy(&semGps);
#endif
        return -1;
    }
    if (pthread_create(&s_oneSecThread, NULL, OneSecTask, NULL) < 0)
    {
        debug_printf(UNMASK_PRT, "THD: Creating one second thread failed\n");
        sem_destroy(&semPtp);
        sem_destroy(&semOneSec);
        sem_destroy(&semAnalysis);
#ifdef GPS_BUILD
        sem_destroy(&semGps);
#endif
        return -1;
    }
    if (pthread_create(&s_analysisThread, NULL, AnalysisTask, NULL) < 0)
    {
        debug_printf(UNMASK_PRT, "THD: Creating analysis thread failed\n");
        sem_destroy(&semPtp);
        sem_destroy(&semOneSec);
        sem_destroy(&semAnalysis);
#ifdef GPS_BUILD
        sem_destroy(&semGps);
#endif
        return -1;
    }
#ifdef GPS_BUILD
    if (pthread_create(&s_gpsThread, NULL, GpsTask, NULL) < 0)
    {
        debug_printf(UNMASK_PRT, "THD: Creating GPS thread failed\n");
        sem_destroy(&semPtp);
        sem_destroy(&semOneSec);
        sem_destroy(&semAnalysis);
        sem_destroy(&semGps);
        return -1;
    }
#endif

    /* Delay one tick to allow PTP stack to ready */
    usleep(l_timeResNsec/1000);

    return 0;
}

/*
----------------------------------------------------------------------------
                                THD_Run()

Description:
This function is called from the Timer ISR.  This function is used to
run the PTP stack, and must be call at a regular predetermined rate
(i.e every 4 milliseconds.)

Parameters:

	Inputs:
	None

	Outputs:
	None

Return value:
	 0: function succeeded
	-1: function failed

-----------------------------------------------------------------------------
*/
int THD_Run(void)
{
    if (!o_ptpRunComplete)
    {
        l_ptpOverrunCount++;
    }
    SIS_TimerServiceRoutine();
    if (sem_post(&semPtp) < 0)
    {
        return -1;
    }
    return 0;
}

/*
----------------------------------------------------------------------------
                                THD_Shutdown()

Description:
This function initiates the shutdown process of all threads.

Parameters:

Inputs:
	None

Outputs:
	None

Return value:
	 0: function succeeded
	-1: function failed

-----------------------------------------------------------------------------
*/
int THD_Shutdown( void )
{
    int i_ret = 0;

    o_shutdown = TRUE;
    if (sem_post(&semPtp) < 0)
    {
        debug_printf(UNMASK_PRT, "%s: post semPtp failed", __FUNCTION__);
        i_ret = -1;
    }
    if (pthread_join(s_ptpThread, NULL) < 0)
    {
        debug_printf(UNMASK_PRT, "THD: Join ptp task failed\n");
        i_ret = -1;
    }
    if (pthread_join(s_oneSecThread, NULL) < 0)
    {
        debug_printf(UNMASK_PRT, "THD: Join one second task failed\n");
        i_ret = -1;
    }
    if (pthread_join(s_analysisThread, NULL) < 0)
    {
        debug_printf(UNMASK_PRT, "THD: Join analysis task failed\n");
        i_ret = -1;
    }
#ifdef GPS_BUILD
    if (pthread_join(s_gpsThread, NULL) < 0)
    {
        debug_printf(UNMASK_PRT, "THD: Join GPS task failed\n");
        i_ret = -1;
    }
#endif
    /* Destroy semophores on exit */
    if (sem_destroy(&semPtp) < 0)
    {
        debug_printf(UNMASK_PRT, "THD: Destroy PTP semaphore failed\n");
        i_ret = -1;
    }
    if (sem_destroy(&semOneSec) < 0)
    {
        debug_printf(UNMASK_PRT, "THD: Destroy one second semaphore failed\n");
        i_ret = -1;
    }
    if (sem_destroy(&semAnalysis) < 0)
    {
        debug_printf(UNMASK_PRT, "THD: Destroy Analysis semaphore failed\n");
        i_ret = -1;
    }
#ifdef GPS_BUILD
    if (sem_destroy(&semGps) < 0)
    {
        debug_printf(UNMASK_PRT, "THD: Destroy GPS semaphore failed\n");
        i_ret = -1;
    }
#endif
    return i_ret;
}

/*
----------------------------------------------------------------------------
                                PtpTask()

Description:
This function is the main thread that runs PTP_Main every tick.

Parameters:

Inputs:
	None

Outputs:
	None

Return value:
	 0: function succeeded
	-1: function failed

-----------------------------------------------------------------------------
*/
static void *PtpTask(void *arg)
{
  struct sched_param sched;
  UINT32 l_ptpRunCount = 0;

  //const UINT32 l_ticksPerSec = 1000000000L / l_timeResNsec;
#ifdef GPS_BUILD
  UINT32 l_gpsRunCount = 0;
  UINT32 l_ticksPerGpsInterval = 0;
#endif
  t_ptpTimeStampType  s_sysTime;
  INT16 d_utcOffset;
  UINT64 curr_time = 0;
  static BOOLEAN rollover_time_flag = 0;
  //    static UINT64 prev_time = 0;

  //variables to keep track of time usage for the CPU_BANDWIDTH_PRT debug flag
  //the "wall" variables are used to track how often to print and
  //use the unix "wall" clock. The other time variables are used to track
  //the time used by the thread and use SC_SystemTime time.
  t_ptpTimeStampType start_time = {0,0};
  t_ptpTimeStampType end_time = {0,0};
  UINT64 acc_time = 0ll;
  struct timespec wall_start_time = {0,0};
  struct timespec wall_end_time = {0,0};
  UINT64 acc_wall_time = 0ll;
  int acc_runs = 0;

  /* Set process priority level */
  sched.sched_priority = PTP_TASK_PRI;
  sched_setscheduler(0, SCHED_FIFO, &sched);

#ifdef GPS_BUILD
  /* Calculate tick interval for GPS task */
  l_ticksPerGpsInterval = l_gpsIntervalTime / l_timeResNsec;
#endif

  clock_gettime(CLOCK_MONOTONIC, &wall_start_time);

  o_shutdown = FALSE;
  while (1)
  {
    PTP_t_TmIntv s_tiActOffs;
    sem_wait(&semPtp);
    if (o_shutdown)
      break;

    //this must be the first block in the while loop for the thread
    //after it is released by the semaphore
    if(debug_NeedToCallPrintf(CPU_BANDWIDTH_PRT)) // are we printing CPU?, otherwise skip...
    {
      SC_SystemTime(&start_time, NULL, e_PARAM_GET);
      acc_runs++;
    }

    o_ptpRunComplete = FALSE;
    PTP_Main(&s_tiActOffs);
    o_ptpRunComplete = TRUE;
    l_ptpRunCount++;

#if 1

    SC_SystemTime(&s_sysTime, &d_utcOffset, e_PARAM_GET);
    curr_time = ((UINT64)s_sysTime.u48_sec * NSEC_PER_SEC ) + (UINT64)s_sysTime.dw_nsec;
    curr_time = curr_time % NSEC_PER_SEC;

    /* send semaphore when one second in SC_SystemTime() rolls over */
    if((curr_time < NSEC_PER_100MS) && (rollover_time_flag == TRUE))
    {
      if (!o_oneSecRunComplete)
      {
        l_oneSecOverrunCount++;
      }
      sem_post(&semOneSec);
      l_ptpRunCount = 0;
      rollover_time_flag = FALSE;
    }
    else if(curr_time > (NSEC_PER_100MS * 9))
    {
      rollover_time_flag = TRUE;
    }

#else
    if (l_ptpRunCount >= l_ticksPerSec)
    {
      if (!o_oneSecRunComplete)
      {
        l_oneSecOverrunCount++;
      }
      sem_post(&semOneSec);
      l_ptpRunCount = 0;
      //printf("+++ l_ptpOverrunCount= %d\n", l_ptpOverrunCount);
      //printf("+++ l_oneSecOverrunCount= %d\n", l_oneSecOverrunCount);
    }
#endif

#ifdef GPS_BUILD
    l_gpsRunCount++;
    if (l_gpsRunCount >= l_ticksPerGpsInterval)
    {
      if (!o_gpsRunComplete)
      {
        l_gpsOverrunCount++;
      }
      sem_post(&semGps);
      l_gpsRunCount = 0;
      //printf("+++ l_gpsOverrunCount= %d\n", l_gpsOverrunCount);
    }
#endif

    //this must be the last block in the while loop for the thread
    //calculate CPU usage and print (if needed)
    if(debug_NeedToCallPrintf(CPU_BANDWIDTH_PRT))
    {
      SC_SystemTime(&end_time, NULL, e_PARAM_GET);
      acc_time += ((end_time.u48_sec * NSEC_PER_SEC) + end_time.dw_nsec) - ((start_time.u48_sec * NSEC_PER_SEC) + start_time.dw_nsec);

      clock_gettime(CLOCK_MONOTONIC, &wall_end_time);
      acc_wall_time = diff_nsec(wall_start_time, wall_end_time);

      if(acc_wall_time >= k_Interval_CPU_BANDWIDTH_PRT_NS)
      {
        debug_printf(CPU_BANDWIDTH_PRT, "%s thread %lld ns used per %lld ns and ran %d times\n", __func__, acc_time, acc_wall_time, acc_runs);

        //reset counters and timers
        acc_runs = 0;
        acc_time = 0;
        clock_gettime(CLOCK_MONOTONIC, &wall_start_time);
      }
    }

  }
  /* stop PTP */
  PTP_Close();
#ifdef GPS_BUILD
  /* stop GPS task */
  sem_post(&semGps);
#endif
  /* stop servo task */
  sem_post(&semOneSec);

  return 0;
}

/*
----------------------------------------------------------------------------
                                OneSecTask()

Description:
This function is the one second thread that runs one second CLK task.

Parameters:

Inputs:
	arg: not used

Outputs:
	None

Return value:
	 0: function succeeded
	-1: function failed

-----------------------------------------------------------------------------
*/
static void *OneSecTask(void *arg)
{
    struct sched_param sched;
    UINT16 w_oneSecRunCount = 0;

    /* Set process priority level */
    sched.sched_priority = ONE_SEC_TASK_PRI;
    sched_setscheduler(0, SCHED_FIFO, &sched);

    while (1)
    {
        sem_wait(&semOneSec);
        if (o_shutdown)
            break;

        o_oneSecRunComplete = FALSE;
        SC_RunOneSecTask();
        o_oneSecRunComplete = TRUE;
        w_oneSecRunCount++;


        //Print_Timecode_T1();
        GenerateTimecode();

        if (w_oneSecRunCount >= 8)
        {
            sem_post(&semAnalysis);
            w_oneSecRunCount = 0;
        }
    }
    sem_post(&semAnalysis);

    return 0;
}

/*
----------------------------------------------------------------------------
                                AnalysisTask()

Description:
This function is the one second thread that runs one minute CLK task.

Parameters:

Inputs:
	arg: not used

Outputs:
	None

Return value:
	 0: function succeeded
	-1: function failed

-----------------------------------------------------------------------------
*/
static void *AnalysisTask(void *arg)
{
    struct sched_param sched;

    /* Set process priority level */
    sched.sched_priority = ONE_MIN_TASK_PRI;
    sched_setscheduler(0, SCHED_FIFO, &sched);

    while (1)
    {
        sem_wait(&semAnalysis);
        if (o_shutdown)
            break;

        SC_RunAnalysisTask();
    }

    return 0;
}

#ifdef GPS_BUILD
/*
----------------------------------------------------------------------------
                                GpsTask()

Description:
This function is the one second thread that runs the GPS Task.
Run interval is determined at config time by the type of GPS
engine configured by the user.

Parameters:

Inputs:
	arg: not used

Outputs:
	None

Return value:
	 0: function succeeded
	-1: function failed

-----------------------------------------------------------------------------
*/
static void *GpsTask(void *arg)
{
  struct sched_param sched;

  //variables to keep track of time usage for the CPU_BANDWIDTH_PRT debug flag
  //the "wall" variables are used to track how often to print and
  //use the unix "wall" clock. The other time variables are used to track
  //the time used by the thread and use SC_SystemTime time.
  t_ptpTimeStampType start_time = {0,0};
  t_ptpTimeStampType end_time = {0,0};
  UINT64 acc_time = 0ll;
  struct timespec wall_start_time = {0,0};
  struct timespec wall_end_time = {0,0};
  UINT64 acc_wall_time = 0ll;
  int acc_runs = 0;


  /* Set process priority level */
  sched.sched_priority = GPS_TASK_PRI;
  sched_setscheduler(0, SCHED_FIFO, &sched);

  clock_gettime(CLOCK_MONOTONIC, &wall_start_time);

  while (1)
  {
    sem_wait(&semGps);
    if (o_shutdown)
      break;

    //this must be the first block in the while loop for the thread
    //after it is released by the semaphore
    if(debug_NeedToCallPrintf(CPU_BANDWIDTH_PRT)) // are we printing CPU?, otherwise skip...
    {
      SC_SystemTime(&start_time, NULL, e_PARAM_GET);
      acc_runs++;
    }


    o_gpsRunComplete = FALSE;
    GPS_Main();
    o_gpsRunComplete = TRUE;

    //this must be the last block in the while loop for the thread
    //calculate CPU usage and print (if needed)
    if(debug_NeedToCallPrintf(CPU_BANDWIDTH_PRT))
    {
      SC_SystemTime(&end_time, NULL, e_PARAM_GET);
      acc_time += ((end_time.u48_sec * NSEC_PER_SEC) + end_time.dw_nsec) - ((start_time.u48_sec * NSEC_PER_SEC) + start_time.dw_nsec);

      clock_gettime(CLOCK_MONOTONIC, &wall_end_time);
      acc_wall_time = diff_nsec(wall_start_time, wall_end_time);

      if(acc_wall_time >= k_Interval_CPU_BANDWIDTH_PRT_NS)
      {
        debug_printf(CPU_BANDWIDTH_PRT, "%s thread %lld ns used per %lld ns and ran %d times\n", __func__, acc_time, acc_wall_time, acc_runs);

        //reset counters and timers
        acc_runs = 0;
        acc_time = 0;
        clock_gettime(CLOCK_MONOTONIC, &wall_start_time);
      }
    }

  }

  return 0;
}

/*
----------------------------------------------------------------------------
                                THD_InitGpsInterval()

Description:
This function initializes the run interval time of the GPS Task

Parameters:

Inputs:
	dw_gpsInterval: GPS Task interval time in msec

Outputs:
	None

Return value:
	 0: function succeeded
	-1: function failed

-----------------------------------------------------------------------------
*/
void THD_InitGpsInterval(UINT32 dw_gpsInterval)
{
    /* Assign GPS interval, convert from msec to nsec */
    l_gpsIntervalTime = dw_gpsInterval * NSEC_PER_MSEC;

    return;
}
#endif /* GPS_BUILD */

/*
----------------------------------------------------------------------------
                                diff_nsec()

Description:
Calculate the difference between start and end.
End must be greater than end, otherwise 0 is returned.

Parameters:

Inputs
  start, end

Outputs
  None

Return value:
  difference in nanoseconds

Global variables affected and border effects:
  Read
    none
  Write
    none

-----------------------------------------------------------------------------
*/
static UINT64 diff_nsec(const struct timespec start, const struct timespec end)
{
  struct timespec result ;

  /* Subtract the second time from the first. */
  if ((end.tv_sec < start.tv_sec) || ((end.tv_sec == start.tv_sec) &&
     (end.tv_nsec <= start.tv_nsec)))
  { /* end <= start? */
    result.tv_sec = result.tv_nsec = 0;
  }
  else
  { /* end > start */
    result.tv_sec = end.tv_sec - start.tv_sec ;
    if (end.tv_nsec < start.tv_nsec)
    { /* Borrow a second. */
      result.tv_nsec = end.tv_nsec + NSEC_PER_SEC - start.tv_nsec;
      result.tv_sec--;
    }
    else
    {
      result.tv_nsec = end.tv_nsec - start.tv_nsec;
    }
  }

  return (result.tv_sec * NSEC_PER_SEC) + (result.tv_nsec);
}


