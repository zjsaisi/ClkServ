/*************************************************************************
**    (C) 2008 - 2009 IXXAT Automation GmbH, all rights reserved
**************************************************************************
**
**       File: SIS.h
**    Summary: SIS - Stack Internal Scheduling
**             A Light Weight "Operating System" Running In One Task.
**             The SIS modul is a minimal operation system, which schedules
**             the stack internal tasks. It can be implemented as one real
**             OS task, but also used from a superloop system without OS.
**
**             SIS has the following functionality:
**             - A scheduler which calls the SIS tasks in order of their
**               priorities
**             - A mailbox mechanism for inter-task communication
**             - An event mechanism for inter-task communication
**             - A timer for each task for e.g. timeout supervision
**             - An error handling modul for capturing and forwarding of
**               occurred errors
**             - A memory manamement unit for allocating/freeing of memory
**               blocks (memory pools)
**
**             The SIS scheduler works in a cooperative and non-preemptive
**             mode. Therefore all tasks must be implemented as normal
**             'C' functions, which will be called from the scheduler one
**             after the other. So each task must run to completion, blocking
**             tasks (endless loops) are not allowed.
**
**    Version: 1.01.01
**
**************************************************************************
**************************************************************************
**
**  Functions: SIS_Init
**             SIS_TaskExeReq
**             SIS_TimerStart
**             SIS_TimerStop
**             SIS_TimerStsRead
**             SIS_GetTime
**             SIS_EventSet
**             SIS_EventGet
**             SIS_EventQuery
**             SIS_MboxPut
**             SIS_MboxWListAdd
**             SIS_MboxGet
**             SIS_MboxRelease
**             SIS_MboxQuery
**             SIS_AllocDbg
**             SIS_AllocHdlDbg
**             SIS_FreeDbg
**             SIS_Alloc
**             SIS_AllocHdl
**             SIS_Free
**             SIS_MemPoolQuery
**             SIS_MemPoolDebugPrint
**             SIS_Scheduler
**             SIS_TimerServiceRoutine
**
**************************************************************************
**    all rights reserved
*************************************************************************/
#ifndef __SIS_H__
#define __SIS_H__

/*************************************************************************
**    compiler directives
*************************************************************************/

/*************************************************************************
**    include-files
*************************************************************************/

#include "SIShdls.h"

/*************************************************************************
**    constants and macros
*************************************************************************/

/* changes SIS functions to debug version that collects file names and lines */
#if( SIS_k_ALLOC_DEBUG_VERS == TRUE )
#define SIS_Alloc(x)      SIS_AllocDbg(x, __FILE__, (UINT32) __LINE__)
#define SIS_AllocHdl(x)   SIS_AllocHdlDbg(x, __FILE__, (UINT32) __LINE__)
#define SIS_Free(x)       SIS_FreeDbg(x, __FILE__, (UINT32) __LINE__)
#endif

/* task timer states: */
#define SIS_k_TIMER_STOPPED   0
#define SIS_k_TIMER_RUNNING   1
#define SIS_k_TIMER_EXPIRED   2

/** SIS_TIME_OVER
**  macro returns TRUE, if time is over.
**  Example::
**
**    dest_time = SIS_GetTime() + TIMEOUT;
**    while(!SIS_TIME_OVER(dest_time))
**    { ... }
**
**  Warning:
**      Timeout must be smaller then 0x80000000 !!!
*/
#define SIS_TIME_OVER(t)   ((UINT32)(SIS_GetTime() - (t)) < 0x80000000UL)

/*************************************************************************
**    data types
*************************************************************************/

/** SIS_t_TASK:
    Prototype of a SIS task.
    A SIS task has the following parameter:
    - b_task_hdl (IN):  a unique number to identify each tasks. */
typedef void (*SIS_t_TASK)(UINT16 w_task_hdl);

/*************************************************************************
**    global variables
*************************************************************************/


/*************************************************************************
**    function prototypes
*************************************************************************/

#ifdef __cplusplus
extern "C"
{
#endif

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
void SIS_Init(void);

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
void SIS_TaskExeReq(UINT16 w_task_hdl);

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
void SIS_TimerStart(UINT16 w_task_hdl, UINT32 dw_ticks);

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
void SIS_TimerStop(UINT16 w_task_hdl);

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
UINT8 SIS_TimerStsRead(UINT16 w_task_hdl);

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
UINT32 SIS_GetTime(void);

/*************************************************************************
**
** Function    : SIS_IntDisable
**
** Description : Function disables all interrupts and returns the
**               old interrupt state for SIS_IntEnable.
**
** Parameters  : -
**
** Returnvalue : (UINT8) old interrupt state
**
** Remarks     : function is reentrant
**
*************************************************************************/
#define SIS_IntDisable()    GOE_IntDisable()

/*************************************************************************
**
** Function    : SIS_IntEnable
**
** Description : Function sets the interrupt state to the given value
**               (normally enables the interrupt again).
**
** Parameters  : (UINT8) val (IN) - old interrupt value
**
** Returnvalue : timer value in ticks
**
** Remarks     : function is reentrant
**
*************************************************************************/
#define SIS_IntEnable(v)    GOE_IntEnable(v)

/*************************************************************************
**
** Function    : SIS_Continue
**
** Description : Continue program execution on given environment address.
**               Together with SIS_Break, the macro is part of a
**               special kind of lightweight multitasking.
**
** Parameters  : w_sts (IN)  - necessary CPU environment
**
** Returnvalue : -
**
** Remarks     : implemented in GOE !!!
**
*************************************************************************/
#define SIS_Continue(w_sts)  static UINT32 dw_state[SIS_k_NO_TASKS]={0}; \
                             switch(dw_state[w_sts]) { case 0:

/*************************************************************************
**
** Function    : SIS_Break
**
** Description : Interrupt program execution and store CPU environment
**               in given variable.
**               Together with SIS_Continue, the macro is part of a
**               special kind of lightweight multitasking.
**
** Parameters  : w_sts (IN) - necessary CPU environment
**
** Returnvalue : -
**
** Remarks     : implemented in GOE !!!
**
*************************************************************************/
#define SIS_Break(w_sts,i)  do { dw_state[w_sts]= i ; return;   case i:; } while (0)  

/*************************************************************************
**
** Function    : SIS_Return
**
** Description : Interrupt program execution with given return value 'x'
**               and store CPU environment in parameter 'env'.
**               Together with SIS_Continue, the macro is part of a
**               special kind of lightweight multitasking.
**
** Parameters  : w_sts (IN) - necessary CPU environment
**               x     (IN) - caller return value
**
** Returnvalue : -
**
** Remarks     : implemented in GOE !!!
**
*************************************************************************/
#define SIS_Return(w_sts, x)     } dw_state[w_sts]=0;

/*************************************************************************
**
** Function    : SIS_Delay
**
** Description : Interrupt program execution for the given number of ticks.
**               Together with SIS_Continue, the macro is part of a
**               special kind of lightweight multitasking.
**
** Parameters  : hdl   (IN) - task handle
**               env  (OUT) - necessary CPU environment
**               ticks (IN) - delay time in ticks
**
** Returnvalue : -
**
** Remarks     : implemented as macro !!!
**
*************************************************************************/
#define SIS_Delay(hdl, env, ticks)     SIS_TimerStart(hdl, ticks);    \
                                       while(SIS_TimerStsRead(hdl) != \
                                       SIS_k_TIMER_EXPIRED)           \
                                       { SIS_Break(env); }

/*************************************************************************
**
** Function    : SIS_EventSet
**
** Description : Set the Event for the given receive task.
**
** Parameters  : w_task_hdl (IN)  - task handle of the receiving task
**               dw_evt     (IN)  - event handle (mask value)
**
** Returnvalue : -
**
** Remarks     : function is reentrant
**
*************************************************************************/
void SIS_EventSet(UINT16 w_task_hdl, UINT32 dw_evt);

/*************************************************************************
**
** Function    : SIS_EventGet
**
** Description : Function returns the accumulated events and
**               clears them additionally. But only the masked event bits
**               are returned (mask bit must be set), others keep 
**               untouched and can read later on.
**
** Parameters  : dw_evt_mask (IN)  - bit = 1: clear and return assigned event
**                                   bit = 0: keep event untouched
**
** Returnvalue : event flags (bit masked)
**
** Remarks     : function is reentrant
**
*************************************************************************/
UINT32 SIS_EventGet(UINT32 dw_evt_mask);

/*************************************************************************
**
** Function    : SIS_EventQuery
**
** Description : Function returns the accumulated events without
**               clearing them.
**
** Parameters  : -
**
** Returnvalue : event flags (bit masked)
**
** Remarks     : function is reentrant
**
*************************************************************************/
UINT32 SIS_EventQuery(void);

/*************************************************************************
**
** Function    : SIS_MboxPut
**
** Description : Send a message to the via handle specified task.
**               The message will be stored in a task related mailbox,
**               that means, the message behind pv_msg will be copied.
**               Function can be called from ISRs and also from other
**               execution contexts.
**
** Parameters  : w_task_hdl (IN)  - task handle of the receiving task
**               pv_msg     (IN)  - pointer to the message
**
** Returnvalue : TRUE   - success
**               FALSE  - mailbox was full (see SIS_MboxWListAdd)
**
** Remarks     : function is reentrant
**
*************************************************************************/
BOOLEAN SIS_MboxPut(UINT16 w_task_hdl,const void *pv_msg);

/*************************************************************************
**
** Function    : SIS_MboxWListAdd
**
**  Description : Add handle of the running task to the waiting list of
**                the given mailbox. If a mailbox entry becomes available,
**                the task will be reactivated again.
**
**  Parameters  : w_tx_task_hdl (IN) - task handle of the calling task
**                w_rx_task_hdl (IN) - task handle of the receiving task
**                                     (corresponds to waiting list handle)
**
**  Returnvalue : -
**
**  Remarks     : function is not reentrant
**
*************************************************************************/
void SIS_MboxWListAdd(UINT16 w_tx_task_hdl, UINT16 w_rx_task_hdl);

/*************************************************************************
**
** Function    : SIS_MboxGet
**
** Description : Function returns a pointer to the mailbox message or NULL.
**               After processing of the message. the task must call
**               SIS_MsgRelease to free the still occupied mailbox entry.
**               If a another task is waiting on the mailbox because of
**               no available mailbox entries, the task will be reactivated
**               again (task state is set to READY).
**
** Parameters  : -
**
** Returnvalue : Pointer to the message or
**               NULL, if mailbox empty
**
** Remarks     : function is not reentrant
**
*************************************************************************/
void *SIS_MboxGet( void );

/*************************************************************************
**
** Function    : SIS_MboxRelease
**
** Description : Function releases the occupied mailbox entry.
**               Function must be called after SIS_MbxGet.
**
** Parameters  : -
**
** Returnvalue : -
**
** Remarks     : function is not reentrant
**
*************************************************************************/
void SIS_MboxRelease( void );

/*************************************************************************
**
** Function    : SIS_MboxQuery
**
** Description : Function returns the number of free mailbox entries
**               of the given mailbox.
**
** Parameters  : w_task_hdl (IN)  - task handle of the mailbox task
**
** Returnvalue : number of free mailbox entries
**
** Remarks     : function is reentrant
**
*************************************************************************/
UINT16 SIS_MboxQuery(UINT16 w_task_hdl);

#if( SIS_k_ALLOC_DEBUG_VERS == TRUE )
/*************************************************************************
**
** Function    : SIS_AllocDbg
**
** Description : Function allocates a buffer with the given size.
**               The function returns a pointer to a buffer from ther pool,
**               which fits mostly to the given size.
**               (buffer_size >= given_size)
**               If the requested buffer size is a constant value, it is
**               recommended to use the more efficient function
**               SIS_AllocHdl().
**               To free the given buffer, SIS_Free() must be called.
**
**               This is the debug version of SIS_Alloc and stores the
**               file and line where it is called.
**
** See Also    : SIS_Free(), SIS_AllocHdlDbg()
**
** Parameters  : w_buf_size (IN) - memory block size in bytes (1..65535)
**               pc_file    (IN) - string contains file name of function call
**               dw_line    (IN) - contains line of function call in file
**
** Returnvalue : pointer to the allocated memory buffer or
**               NULL if no buffer is available.
**
** Remarks     : function is reentrant
**
*************************************************************************/
void* SIS_AllocDbg(UINT16 w_buf_size,const CHAR *pc_file,UINT32 dw_line);

/*************************************************************************
**
**  Function    : SIS_AllocHdlDbg
**
**  Description : Function allocates a buffer from the a memory pool.
**                The memory pool is specified by means of the given
**                pool handle. See "SIShdls.h" for possible pool handles.
**                The function returns a pointer to a buffer.
**                To free the given buffer, SIS_Free() must be called.
**
**                This is the debug version of SIS_AllocHdl and stores the
**                file and line where it is called.
**
**  Parameters  : w_pool_hdl (IN)  - pool handle
**                pc_file    (IN) - string contains file name of function call
**                dw_line    (IN) - contains line of function call in file
**
**  Returnvalue : pointer to the allocated memory buffer or
**                NULL if no buffer is available.
**
**  Remarks     : function is reentrant
**
*************************************************************************/
void* SIS_AllocHdlDbg(UINT16 w_pool_hdl,const CHAR *pc_file,UINT32 dw_line);

/*************************************************************************
**
**  Function    : SIS_FreeDbg
**
**  Description : Function frees a previously allocated memory buffer.
**
**                This is the debug version of SIS_Free and stores the
**                file and line where it is called.
**
**  Parameters  : pv_buff (IN) - pointer to the memory buffer
**                pc_file (IN) - string contains file name of function call
**                dw_line (IN) - contains line of function call in file
**
**  Returnvalue : -
**
**  Remarks     : function is reentrant
**
*************************************************************************/
void SIS_FreeDbg(const void *pv_buff,const CHAR *pc_file,UINT32 dw_line);
#else
/*************************************************************************
**
** Function    : SIS_Alloc
**
** Description : Function allocates a buffer with the given size.
**               The function returns a pointer to a buffer from ther pool,
**               which fits mostly to the given size.
**               (buffer_size >= given_size)
**               If the requested buffer size is a constant value, it is
**               recommended to use the more efficient function
**               SIS_AllocHdl().
**               To free the given buffer, SIS_Free() must be called.
**
** See Also    : SIS_Free(), SIS_AllocHdl()
**
** Parameters  : w_buf_size (IN)  - memory block size in bytes (1..65535)
**
** Returnvalue : pointer to the allocated memory buffer or
**               NULL if no buffer is available.
**
** Remarks     : function is reentrant
**
*************************************************************************/
void* SIS_Alloc(UINT16 w_buf_size);

/*************************************************************************
**
** Function    : SIS_AllocHdl
**
** Description : Function allocates a buffer from the a memory pool.
**               The memory pool is specified by means of the given
**               pool handle. See "SIShdls.h" for possible pool handles.
**               The function returns a pointer to a buffer.
**               To free the given buffer, SIS_Free() must be called.
**
** See Also    : SIS_Free()
**
** Parameters  : w_pool_hdl (IN)  - pool handle
**
** Returnvalue : pointer to the allocated memory buffer or
**               NULL if no buffer is available.
**
** Remarks     : function is reentrant
**
*************************************************************************/
void* SIS_AllocHdl(UINT16 w_pool_hdl);

/*************************************************************************
**
** Function    : SIS_Free
**
** Description : Function frees a previously allocated memory buffer.
**
** See Also    : SIS_Alloc(), SIS_AllocHdl()
**
** Parameters  : pv_buff (IN)  - pointer to the memory buffer
**
** Returnvalue : -
**
** Remarks     : function is reentrant
**
*************************************************************************/
void SIS_Free(const void *pv_buff);
#endif /* #if( SIS_k_ALLOC_DEBUG_VERS == TRUE ) */

/*************************************************************************
**
** Function    : SIS_MemPoolQuery
**
** Description : Function returns the available number of memory buffers
**               in the given pool.
**
** See Also    : SIS_Alloc(), SIS_AllocHdl()
**
** Parameters  : w_pool_hdl (IN)  - pool handle
**
** Returnvalue : number of available memory buffers or zero
**
** Remarks     : function is reentrant
**
*************************************************************************/
UINT16 SIS_MemPoolQuery(UINT16 w_pool_hdl);

/*************************************************************************
**
** Function    : SIS_MemPoolDebugPrint
**
** Description : Only useful for debugging purposes!
**               Function emits all available memory buffer indexes
**               of the given pool to stdout.
**
** Parameters  : w_pool_hdl (IN)  - pool handle
**
** Returnvalue : -
**
** Remarks     : function is useful for debugging purposes.
**               function is NOT reentrant.
**
*************************************************************************/
void SIS_MemPoolDebugPrint(UINT16 w_pool_hdl);

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
void SIS_Scheduler(void);

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
void SIS_TimerServiceRoutine(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* __SIS_H__ */

