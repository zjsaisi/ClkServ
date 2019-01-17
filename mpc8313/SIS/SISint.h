/*************************************************************************
**    (C) 2008 - 2009 IXXAT Automation GmbH, all rights reserved
**************************************************************************
**
**       File: SISint.h
**    Summary: internal header file 
**             of the Stack Internal 
**             Scheduling unit
**
**    Version: 1.01.01
**
**************************************************************************
**************************************************************************
**
**  Functions: SIS_EventInit
**             SIS_ErrorInit
**             SIS_MboxInit
**             SIS_MemPoolInit
**
**************************************************************************
**    all rights reserved
*************************************************************************/

/*************************************************************************
**    constants and macros
*************************************************************************/

/* task states: */
#define TASK_k_READY        1       /* task is ready for execution */
#define TASK_k_PENDING      0       /* task is waiting for an event */

#define SIS_NIL                 ((UINT16)(-1))
#define SIS_MEMCPY(s,d,n)       (memcpy((s),(d),(n)))
#define SIS_MEMSET(s,d,n)       (memset((void*)(s),(UINT8)(d),(INT32)(n)))

/*************************************************************************
**    data types
*************************************************************************/
/** t_Timer : definition of a task timer
**/
typedef struct
{
  UINT32   dw_time_val;  /* timer value */
}t_TIMER;


/*************************************************************************
**    global variables
*************************************************************************/
extern UINT8  SIS_ab_TaskSts[];
extern UINT16 SIS_w_RunningTask;

/*************************************************************************
**    function prototypes
*************************************************************************/

/*************************************************************************
**
**  Function    : SIS_EventInit
**
**  Description : Initialize the event structures. All events will
**                be reset.
**
**  Parameters  : -
**
**  Returnvalue : -
**
**  Remarks     : function is not reentrant
**
*************************************************************************/
void SIS_EventInit(void);

/*************************************************************************
**
** Function    : SIS_ErrorInit
**
** Description : Initialization of the error handling
**
** Parameters  : -
**
** Returnvalue : -
**
** Remarks     : function is not reentrant
**
*************************************************************************/
void SIS_ErrorInit(void);

/*************************************************************************
**
**  Function    : SIS_MboxInit
**
**  Description : Initialize the mailbox structures. All mailboxes will
**                be cleared.
**
**  Parameters  : -
**
**  Returnvalue : -
**
**  Remarks     : function is not reentrant
**
*************************************************************************/
void SIS_MboxInit(void);

/*************************************************************************
**
**  Function    : SIS_MemPoolInit
**
**  Description : Init the linked list of each memory pool
**
**  Parameters  : -
**
**  Returnvalue : -
**
**  Remarks     : function is not reentrant
**
*************************************************************************/
void SIS_MemPoolInit(void);


