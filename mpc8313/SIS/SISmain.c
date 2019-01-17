/*************************************************************************
**    (C) 2008 - 2009 IXXAT Automation GmbH, all rights reserved
**************************************************************************
**
**       File: SISmain.c
**    Summary: Stack Internal Scheduling (SIS) module.
**     Remark: !!! Auto generated source file, do not modify !!!
**
**************************************************************************
**************************************************************************
**
**  Functions:  SIS_Init
**              SIS_TaskExeReq
**              SIS_TimerStart
**              SIS_TimerStop
**              SIS_TimerStsRead
**              SIS_GetTime
**              SIS_Scheduler
**              SIS_TimerServiceRoutine
**
**************************************************************************
**    all rights reserved
*************************************************************************/

/*************************************************************************
**    compiler directives
*************************************************************************/

/*************************************************************************
**    include-files
*************************************************************************/
#include "target.h"
#include "SIS.h"
#include "SISint.h"

/*************************************************************************
**    global variables
*************************************************************************/
/* Task states, used by the SIS_Scheduler */
UINT8 SIS_ab_TaskSts[SIS_k_NO_TASKS+1]; /* last entry for abort condition */

/* variable shows the running task handle. If zero
   scheduler must be restarted via real OS services */
UINT16 SIS_w_RunningTask = 0;

/*************************************************************************
**    static constants, types, macros, variables
*************************************************************************/
#ifdef __cplusplus
extern "C"
{
#endif
/* definition of task prototypes */
extern void CTL_Task(UINT16 w_task_hdl);/*lint !e762 */
extern void SLV_DelayTask(UINT16 w_task_hdl);/*lint !e762 */
extern void SLV_SyncTask(UINT16 w_task_hdl);/*lint !e762 */
extern void SLV_SyncTask(UINT16 w_task_hdl);/*lint !e762 */
extern void SLV_SyncTask(UINT16 w_task_hdl);/*lint !e762 */
extern void MSTsyn_Task(UINT16 w_task_hdl);/*lint !e762 */
extern void MSTsyn_Task(UINT16 w_task_hdl);/*lint !e762 */
extern void MSTdel_Task(UINT16 w_task_hdl);/*lint !e762 */
extern void MSTdel_Task(UINT16 w_task_hdl);/*lint !e762 */
extern void MSTannc_Task(UINT16 w_task_hdl);/*lint !e762 */
extern void MSTannc_Task(UINT16 w_task_hdl);/*lint !e762 */
extern void MNT_Task(UINT16 w_task_hdl);/*lint !e762 */
extern void UCMann_Task(UINT16 w_task_hdl);/*lint !e762 */
extern void UCMsyn_Task(UINT16 w_task_hdl);/*lint !e762 */
extern void UCMdel_Task(UINT16 w_task_hdl);/*lint !e762 */
extern void UCD_Task(UINT16 w_task_hdl);/*lint !e762 */
extern void DIS_Task(UINT16 w_task_hdl);/*lint !e762 */
extern void DISnet_Task(UINT16 w_task_hdl);/*lint !e762 */
extern void DISnet_Task(UINT16 w_task_hdl);/*lint !e762 */
#ifdef __cplusplus
}
#endif

/* task table with function pointers */
static const SIS_t_TASK afp_TaskTbl[SIS_k_NO_TASKS+1] =
{
  CTL_Task,
  SLV_DelayTask,
  SLV_SyncTask,
  SLV_SyncTask,
  SLV_SyncTask,
  MSTsyn_Task,
  MSTsyn_Task,
  MSTdel_Task,
  MSTdel_Task,
  MSTannc_Task,
  MSTannc_Task,
  MNT_Task,
  UCMann_Task,
  UCMsyn_Task,
  UCMdel_Task,
  UCD_Task,
  DIS_Task,
  DISnet_Task,
  DISnet_Task,
  0
};

/* timer tables with timer values and timer states */
static t_TIMER  as_TimerVal[SIS_k_NO_TASKS];
/* last entry for abort condition */
static UINT8    ab_TimerSts[SIS_k_NO_TASKS+1];
/* SIS system timer */
static UINT32   dw_TimerTicks = 0;

/*************************************************************************
**    static function-prototypes
*************************************************************************/

/*************************************************************************
**    global functions
*************************************************************************/

/*************************************************************************
**
** Function    : SIS_Init
**
** Description : Initialize scheduler structures.
**               All task registrations will be deleted, all mailboxes
**               are cleared and all running task timers are stopped.
**
** Parameters  : -
**
** Returnvalue : -
**
** Remarks     : function is not reentrant
**
*************************************************************************/
void SIS_Init(void)
{
  UINT32 i;

  /*
  ** init task status table
  */
  for (i=0;i<SIS_k_NO_TASKS;i++)
  {
    SIS_ab_TaskSts[i] = TASK_k_PENDING;
  }
  /* abort condition for the SIS_Scheduler while loop */
  SIS_ab_TaskSts[SIS_k_NO_TASKS] = TASK_k_READY;
  /*
  ** init timer register table
  */
  for(i=0;i<SIS_k_NO_TASKS;i++)
  {
    as_TimerVal[i].dw_time_val = 0;
    ab_TimerSts[i]             = SIS_k_TIMER_STOPPED;
  }
  /* abort condition for the SIS_TimerServiceRoutine while loop */
  ab_TimerSts[SIS_k_NO_TASKS] = SIS_k_TIMER_RUNNING;

  dw_TimerTicks = 0;
  /* init event handling */
  SIS_EventInit();
  /* init pool of mailbox buffers */
  SIS_MboxInit();
  /* init memory pool management */
  SIS_MemPoolInit();
}

/*************************************************************************
**
** Function    : SIS_TaskExeReq
**
** Description : Set a request for task execution
**               Function can be used from SIS task and ISR level
**
** Parameters  : w_task_hdl (IN)  - task handle (0..n)
**
** Returnvalue : -
**
** Remarks     : function is reentrant
**
*************************************************************************/
void SIS_TaskExeReq(UINT16 w_task_hdl)
{
  /* check parameter */
  assert(w_task_hdl < SIS_k_NO_TASKS);
  /* set task state to READY */
  SIS_ab_TaskSts[w_task_hdl] = TASK_k_READY;
}

/*************************************************************************
**
** Function    : SIS_TimerStart
**
** Description : Start/restart a task timer by the given value in ticks.
**               After the timer is expired, the timer will be stopped
**               and the task started. With this feature, a task can
**               reactivate itself after a certain amount of time for
**               e. g. timeout supervision.
**               Function can be used from SIS task level only !
**
** Parameters  : w_task_hdl (IN)  - task handle (0..n)
**               dw_ticks   (IN)  - delay time in timer ticks (2...65535)
**
** Returnvalue : -
**
** Remarks     : function is not reentrant
**
*************************************************************************/
void SIS_TimerStart(UINT16 w_task_hdl, UINT32 dw_ticks)
{
  /* check parametes */
  assert(w_task_hdl < SIS_k_NO_TASKS);
  assert(dw_ticks > 0);
  /* stop running instead of disabling the timer ISR */
  ab_TimerSts[w_task_hdl]             = SIS_k_TIMER_STOPPED;
  as_TimerVal[w_task_hdl].dw_time_val = dw_TimerTicks + dw_ticks;
  ab_TimerSts[w_task_hdl]             = SIS_k_TIMER_RUNNING;
}


/*************************************************************************
**
** Function    : SIS_TimerStop
**
** Description : Stop the running timer, so that the task will not
**               be started.
**
** Parameters  : w_task_hdl (IN)  - handle of the corresponding task (0..n)
**
** Returnvalue : -
**
** Remarks     : function is not reentrant
**
*************************************************************************/
void SIS_TimerStop(UINT16 w_task_hdl)
{
  /* check parameter */
  assert(w_task_hdl < SIS_k_NO_TASKS);
  /*  reset timer status */
  ab_TimerSts[w_task_hdl] = SIS_k_TIMER_STOPPED;
}

/*************************************************************************
**
** Function    : SIS_TimerStsRead
**
** Description : Read the timer status. If timer is expired, function
**               will return SIS_k_TIMER_EXPIRED and additionaly
**               set the timer state to SIS_k_TIMER_STOPPED.
**
** Parameters  : w_task_hdl (IN)  - handle of the corresponding task (0..n)
**
** Returnvalue : SIS_k_TIMER_STOPPED - timer is free/stopped
**               SIS_k_TIMER_RUNNING - timer is still busy/running
**               SIS_k_TIMER_EXPIRED - timer is expired

** Remarks     : function is not reentrant
**
*************************************************************************/
UINT8 SIS_TimerStsRead(UINT16 w_task_hdl)
{
  UINT8 b_tmp;

  /* check parameter */
  assert(w_task_hdl < SIS_k_NO_TASKS);
  /* return timer status */
  b_tmp = ab_TimerSts[w_task_hdl];
  if(b_tmp == SIS_k_TIMER_EXPIRED)
  {
    ab_TimerSts[w_task_hdl] = SIS_k_TIMER_STOPPED;  /* reset state */
  }
  return b_tmp;
}


/*************************************************************************
**
** Function    : SIS_GetTime
**
** Description : Function returns the SIS system time in ticks.
**
** Parameters  : -
**
** Returnvalue : timer value in ticks
**
** Remarks     : function is reentrant
**
*************************************************************************/
UINT32 SIS_GetTime(void)
{
  #ifdef __32BIT_CPU
    return dw_TimerTicks;
  #else
    UINT8 tmp;
    UINT32 dw_time;
    dw_time = dw_TimerTicks;
    return dw_time;
  #endif
}

/*************************************************************************
**
**  Function    : SIS_Scheduler
**
**  Description : SIS-Task scheduler. Checks the status of all tasks and
**                calls the corresponding tasks.
**
**  Parameters  : -
**
**  Returnvalue : -
**
**  Remarks     : function must be called from real OS task
**
*************************************************************************/
void SIS_Scheduler(void)
{
  UINT16  w_hdl;            /* task handle */
  UINT8   *pb_sts;         /* pointer to the task state */

  w_hdl   = 0;
  pb_sts = &SIS_ab_TaskSts[0];

  while(1)/*lint !e716*/
  {
    /* looks funny, but this is speed optimized C code ! */
    while(*pb_sts++ == TASK_k_PENDING)  /* while PENDING */
    {
      w_hdl++;
    }
    if(w_hdl >= SIS_k_NO_TASKS)     /* end of table reached ? */
    {
      break;                        /* abort searching */
    }
    SIS_w_RunningTask = w_hdl;
    /* reset task request status */
    SIS_ab_TaskSts[w_hdl] = TASK_k_PENDING;
    /* call blocked task */
    assert(afp_TaskTbl[w_hdl] != NULL);
    afp_TaskTbl[w_hdl](w_hdl);
    /* start searching at the beginning again */
    w_hdl   = 0;
    pb_sts = &SIS_ab_TaskSts[0];
    SIS_w_RunningTask = 0;
  }
  return;
}


/*************************************************************************
**
** Function    : SIS_TimerServiceRoutine
**
** Description : Routine for message timer handling.
**               Must be called cyclically from e.g. timer-ISR.
**               The cycle frequency is the basis for the timer
**               resolution (timer ticks).
**
** Parameters  : -
**
** Returnvalue : -
**
** Remarks     : function is not reentrant
**
*************************************************************************/
void SIS_TimerServiceRoutine(void)
{
  UINT16 w_hdl;   /* task/timer handle */
  UINT8  *pb_sts; /* pointer to the timer state */

  dw_TimerTicks ++;
  w_hdl   = 0;
  pb_sts = &ab_TimerSts[0];

  /*
  ** check all entries marked with SIS_k_TIMER_RUNNING, if the
  ** destination time is reached and if, set state to SIS_k_TIMER_EXPIRED
  */
  while(1)/*lint !e716*/
  {
    /* looks funny, but is speed optimized C code ! */
    while(*pb_sts++ != SIS_k_TIMER_RUNNING) /* while not running */
    {
      w_hdl++;
    }
    if(w_hdl >= SIS_k_NO_TASKS)        /* end of table reached ? */
    {
      break;
    }
    if(as_TimerVal[w_hdl].dw_time_val == dw_TimerTicks) /* time over ? */
    {
      ab_TimerSts[w_hdl]     = SIS_k_TIMER_EXPIRED;
      SIS_ab_TaskSts[w_hdl]  = TASK_k_READY;
    }
    w_hdl++;
  }
}

/*************************************************************************
**    static functions
*************************************************************************/

