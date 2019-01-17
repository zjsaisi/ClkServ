/*************************************************************************
**    (C) 2008 - 2009 IXXAT Automation GmbH, all rights reserved
**************************************************************************
**
**       File: SISevent.c
**    Summary: Event passing implemention for the unit SIS.
**     Remark: !!! Auto generated source file, do not modify !!!
**
**************************************************************************
**************************************************************************
**
**  Functions:  SIS_EventInit
**              SIS_EventSet
**              SIS_EventGet
**              SIS_EventQuery
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

/*************************************************************************
**    static constants, types, macros, variables
*************************************************************************/
/* Event Variables */
static UINT32 dw_Events_0 = 0;
static UINT32 dw_Events_1 = 0;
static UINT32 dw_Events_2 = 0;
static UINT32 dw_Events_3 = 0;
static UINT32 dw_Events_4 = 0;
static UINT32 dw_Events_5 = 0;
static UINT32 dw_Events_6 = 0;
static UINT32 dw_Events_7 = 0;
static UINT32 dw_Events_8 = 0;
static UINT32 dw_Events_9 = 0;
static UINT32 dw_Events_10 = 0;
static UINT32 dw_Events_11 = 0;
static UINT32 dw_Events_12 = 0;
static UINT32 dw_Events_13 = 0;
static UINT32 dw_Events_14 = 0;
static UINT32 dw_Events_15 = 0;
static UINT32 dw_Events_16 = 0;
static UINT32 dw_Events_17 = 0;
static UINT32 dw_Events_18 = 0;

/*************************************************************************
**    static function-prototypes
*************************************************************************/

/*************************************************************************
**    global functions
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
void SIS_EventInit(void)
{
  dw_Events_0 = 0;
  dw_Events_1 = 0;
  dw_Events_2 = 0;
  dw_Events_3 = 0;
  dw_Events_4 = 0;
  dw_Events_5 = 0;
  dw_Events_6 = 0;
  dw_Events_7 = 0;
  dw_Events_8 = 0;
  dw_Events_9 = 0;
  dw_Events_10 = 0;
  dw_Events_11 = 0;
  dw_Events_12 = 0;
  dw_Events_13 = 0;
  dw_Events_14 = 0;
  dw_Events_15 = 0;
  dw_Events_16 = 0;
  dw_Events_17 = 0;
  dw_Events_18 = 0;
}

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
void SIS_EventSet(UINT16 w_task_hdl, UINT32 dw_evt)
{
  switch(w_task_hdl)
  {
    case CTL_TSK:
    {
      dw_Events_0 |= dw_evt; /* OR event mask */
      SIS_TaskExeReq(w_task_hdl);
      break;
    }
    case SLD_TSK:
    {
      dw_Events_1 |= dw_evt; /* OR event mask */
      SIS_TaskExeReq(w_task_hdl);
      break;
    }
    case SLS_TSK:
    {
      dw_Events_2 |= dw_evt; /* OR event mask */
      SIS_TaskExeReq(w_task_hdl);
      break;
    }
    case SLSintTmo_TSK:
    {
      dw_Events_3 |= dw_evt; /* OR event mask */
      SIS_TaskExeReq(w_task_hdl);
      break;
    }
    case SLSrcvTmo_TSK:
    {
      dw_Events_4 |= dw_evt; /* OR event mask */
      SIS_TaskExeReq(w_task_hdl);
      break;
    }
    case MAS1_TSK:
    {
      dw_Events_5 |= dw_evt; /* OR event mask */
      SIS_TaskExeReq(w_task_hdl);
      break;
    }
    case MAS2_TSK:
    {
      dw_Events_6 |= dw_evt; /* OR event mask */
      SIS_TaskExeReq(w_task_hdl);
      break;
    }
    case MAD1_TSK:
    {
      dw_Events_7 |= dw_evt; /* OR event mask */
      SIS_TaskExeReq(w_task_hdl);
      break;
    }
    case MAD2_TSK:
    {
      dw_Events_8 |= dw_evt; /* OR event mask */
      SIS_TaskExeReq(w_task_hdl);
      break;
    }
    case MAA1_TSK:
    {
      dw_Events_9 |= dw_evt; /* OR event mask */
      SIS_TaskExeReq(w_task_hdl);
      break;
    }
    case MAA2_TSK:
    {
      dw_Events_10 |= dw_evt; /* OR event mask */
      SIS_TaskExeReq(w_task_hdl);
      break;
    }
    case MNT_TSK:
    {
      dw_Events_11 |= dw_evt; /* OR event mask */
      SIS_TaskExeReq(w_task_hdl);
      break;
    }
    case UCMann_TSK:
    {
      dw_Events_12 |= dw_evt; /* OR event mask */
      SIS_TaskExeReq(w_task_hdl);
      break;
    }
    case UCMsyn_TSK:
    {
      dw_Events_13 |= dw_evt; /* OR event mask */
      SIS_TaskExeReq(w_task_hdl);
      break;
    }
    case UCMdel_TSK:
    {
      dw_Events_14 |= dw_evt; /* OR event mask */
      SIS_TaskExeReq(w_task_hdl);
      break;
    }
    case UCD_TSK:
    {
      dw_Events_15 |= dw_evt; /* OR event mask */
      SIS_TaskExeReq(w_task_hdl);
      break;
    }
    case DIS_TSK:
    {
      dw_Events_16 |= dw_evt; /* OR event mask */
      SIS_TaskExeReq(w_task_hdl);
      break;
    }
    case DISnet_TSK:
    {
      dw_Events_17 |= dw_evt; /* OR event mask */
      SIS_TaskExeReq(w_task_hdl);
      break;
    }
    case DISlat_TSK:
    {
      dw_Events_18 |= dw_evt; /* OR event mask */
      SIS_TaskExeReq(w_task_hdl);
      break;
    }
    default:
    {
      assert(0);
    }
  }
}

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
UINT32 SIS_EventGet(UINT32 dw_evt_mask)
{
  UINT32 dw_evt = 0;

  switch(SIS_w_RunningTask)
  {
    case CTL_TSK:
    {
      dw_evt = dw_Events_0 & dw_evt_mask;
      dw_Events_0 &= ~dw_evt_mask;
      break;
    }
    case SLD_TSK:
    {
      dw_evt = dw_Events_1 & dw_evt_mask;
      dw_Events_1 &= ~dw_evt_mask;
      break;
    }
    case SLS_TSK:
    {
      dw_evt = dw_Events_2 & dw_evt_mask;
      dw_Events_2 &= ~dw_evt_mask;
      break;
    }
    case SLSintTmo_TSK:
    {
      dw_evt = dw_Events_3 & dw_evt_mask;
      dw_Events_3 &= ~dw_evt_mask;
      break;
    }
    case SLSrcvTmo_TSK:
    {
      dw_evt = dw_Events_4 & dw_evt_mask;
      dw_Events_4 &= ~dw_evt_mask;
      break;
    }
    case MAS1_TSK:
    {
      dw_evt = dw_Events_5 & dw_evt_mask;
      dw_Events_5 &= ~dw_evt_mask;
      break;
    }
    case MAS2_TSK:
    {
      dw_evt = dw_Events_6 & dw_evt_mask;
      dw_Events_6 &= ~dw_evt_mask;
      break;
    }
    case MAD1_TSK:
    {
      dw_evt = dw_Events_7 & dw_evt_mask;
      dw_Events_7 &= ~dw_evt_mask;
      break;
    }
    case MAD2_TSK:
    {
      dw_evt = dw_Events_8 & dw_evt_mask;
      dw_Events_8 &= ~dw_evt_mask;
      break;
    }
    case MAA1_TSK:
    {
      dw_evt = dw_Events_9 & dw_evt_mask;
      dw_Events_9 &= ~dw_evt_mask;
      break;
    }
    case MAA2_TSK:
    {
      dw_evt = dw_Events_10 & dw_evt_mask;
      dw_Events_10 &= ~dw_evt_mask;
      break;
    }
    case MNT_TSK:
    {
      dw_evt = dw_Events_11 & dw_evt_mask;
      dw_Events_11 &= ~dw_evt_mask;
      break;
    }
    case UCMann_TSK:
    {
      dw_evt = dw_Events_12 & dw_evt_mask;
      dw_Events_12 &= ~dw_evt_mask;
      break;
    }
    case UCMsyn_TSK:
    {
      dw_evt = dw_Events_13 & dw_evt_mask;
      dw_Events_13 &= ~dw_evt_mask;
      break;
    }
    case UCMdel_TSK:
    {
      dw_evt = dw_Events_14 & dw_evt_mask;
      dw_Events_14 &= ~dw_evt_mask;
      break;
    }
    case UCD_TSK:
    {
      dw_evt = dw_Events_15 & dw_evt_mask;
      dw_Events_15 &= ~dw_evt_mask;
      break;
    }
    case DIS_TSK:
    {
      dw_evt = dw_Events_16 & dw_evt_mask;
      dw_Events_16 &= ~dw_evt_mask;
      break;
    }
    case DISnet_TSK:
    {
      dw_evt = dw_Events_17 & dw_evt_mask;
      dw_Events_17 &= ~dw_evt_mask;
      break;
    }
    case DISlat_TSK:
    {
      dw_evt = dw_Events_18 & dw_evt_mask;
      dw_Events_18 &= ~dw_evt_mask;
      break;
    }
    default:
    {
      assert(0);
    }
  }
  return dw_evt;
}

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
UINT32 SIS_EventQuery(void)
{
  UINT32 dw_evt = 0;

  switch(SIS_w_RunningTask)
  {
    case CTL_TSK:
    {

      dw_evt = dw_Events_0;
      break;
    }
    case SLD_TSK:
    {

      dw_evt = dw_Events_1;
      break;
    }
    case SLS_TSK:
    {

      dw_evt = dw_Events_2;
      break;
    }
    case SLSintTmo_TSK:
    {

      dw_evt = dw_Events_3;
      break;
    }
    case SLSrcvTmo_TSK:
    {

      dw_evt = dw_Events_4;
      break;
    }
    case MAS1_TSK:
    {

      dw_evt = dw_Events_5;
      break;
    }
    case MAS2_TSK:
    {

      dw_evt = dw_Events_6;
      break;
    }
    case MAD1_TSK:
    {

      dw_evt = dw_Events_7;
      break;
    }
    case MAD2_TSK:
    {

      dw_evt = dw_Events_8;
      break;
    }
    case MAA1_TSK:
    {

      dw_evt = dw_Events_9;
      break;
    }
    case MAA2_TSK:
    {

      dw_evt = dw_Events_10;
      break;
    }
    case MNT_TSK:
    {

      dw_evt = dw_Events_11;
      break;
    }
    case UCMann_TSK:
    {

      dw_evt = dw_Events_12;
      break;
    }
    case UCMsyn_TSK:
    {

      dw_evt = dw_Events_13;
      break;
    }
    case UCMdel_TSK:
    {

      dw_evt = dw_Events_14;
      break;
    }
    case UCD_TSK:
    {

      dw_evt = dw_Events_15;
      break;
    }
    case DIS_TSK:
    {

      dw_evt = dw_Events_16;
      break;
    }
    case DISnet_TSK:
    {

      dw_evt = dw_Events_17;
      break;
    }
    case DISlat_TSK:
    {

      dw_evt = dw_Events_18;
      break;
    }
    default:
    {
      assert(0);
    }
  }
  return dw_evt;
}


/*************************************************************************
**    static functions
*************************************************************************/

