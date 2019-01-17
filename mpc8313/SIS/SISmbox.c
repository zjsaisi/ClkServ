/*************************************************************************
**    (C) 2008 - 2009 IXXAT Automation GmbH, all rights reserved
**************************************************************************
**
**       File: SISmbox.c
**    Summary: task related mailbox implemention for the unit SIS.
**     Remark: !!! Auto generated source file, do not modify !!!
**
**************************************************************************
**************************************************************************
**
**  Functions:  SIS_MboxInit
**              SIS_MboxPut
**              SIS_MboxWListAdd
**              SIS_MboxGet
**              SIS_MboxRelease
**              SIS_MboxQuery
**
**************************************************************************
**    all rights reserved
*************************************************************************/

/*
  mailbox relations:
  CTL_TSK:
      MNT_TSK
      SLS_TSK
      SLD_TSK
  SLD_TSK:
  SLS_TSK:
  SLSintTmo_TSK:
  SLSrcvTmo_TSK:
  MAS1_TSK:
  MAS2_TSK:
  MAD1_TSK:
  MAD2_TSK:
  MAA1_TSK:
  MAA2_TSK:
  MNT_TSK:
  UCMann_TSK:
  UCMsyn_TSK:
  UCMdel_TSK:
  UCD_TSK:
  DIS_TSK:
  DISnet_TSK:
  DISlat_TSK:
*/

/*************************************************************************
**    compiler directives
*************************************************************************/
/*lint --e{754} */
/*lint --e{830} */
/*************************************************************************
**    include-files
*************************************************************************/
#include "target.h"
#include "PTP/PTPdef.h"
#include "NIF/NIF.h"
#include "PTP/PTP.h"
#include "SIS.h"
#include "SISint.h"

/*************************************************************************
**    global variables
*************************************************************************/

/*************************************************************************
**    static constants, types, macros, variables
*************************************************************************/
/* task "waiting list" */
static BOOLEAN o_Waiting_0_0 = 0;
static BOOLEAN o_Waiting_0_1 = 0;
static BOOLEAN o_Waiting_0_2 = 0;

/* definition of mailbox 0 */
#define NO_ENTRIES0    10
/* size of mailbox entries is defined by the size */
/* of the type PTP_t_MboxMsg extended to 32-bit alignment */
typedef struct { UINT32 adw_tmp[(sizeof(PTP_t_MboxMsg)+3)/4]; }t_MBOX0;
static t_MBOX0 a_Mbx0[NO_ENTRIES0];
static t_MBOX0 *p_Put0 = a_Mbx0;
static t_MBOX0 *p_Get0 = a_Mbx0;
static volatile UINT16 ActNumEntries0 = 0;
static volatile UINT8 NestedCnt0 = 0;

/* definition of mailbox 1 */
#define NO_ENTRIES1    10
/* size of mailbox entries is defined by the size */
/* of the type PTP_t_MboxMsg extended to 32-bit alignment */
typedef struct { UINT32 adw_tmp[(sizeof(PTP_t_MboxMsg)+3)/4]; }t_MBOX1;
static t_MBOX1 a_Mbx1[NO_ENTRIES1];
static t_MBOX1 *p_Put1 = a_Mbx1;
static t_MBOX1 *p_Get1 = a_Mbx1;
static volatile UINT16 ActNumEntries1 = 0;
static volatile UINT8 NestedCnt1 = 0;

/* definition of mailbox 2 */
#define NO_ENTRIES2    10
/* size of mailbox entries is defined by the size */
/* of the type PTP_t_MboxMsg extended to 32-bit alignment */
typedef struct { UINT32 adw_tmp[(sizeof(PTP_t_MboxMsg)+3)/4]; }t_MBOX2;
static t_MBOX2 a_Mbx2[NO_ENTRIES2];
static t_MBOX2 *p_Put2 = a_Mbx2;
static t_MBOX2 *p_Get2 = a_Mbx2;
static volatile UINT16 ActNumEntries2 = 0;
static volatile UINT8 NestedCnt2 = 0;

/* definition of mailbox 3 */
#define NO_ENTRIES3    1
/* size of mailbox entries is defined by the size */
/* of the type PTP_t_MboxMsg extended to 32-bit alignment */
typedef struct { UINT32 adw_tmp[(sizeof(PTP_t_MboxMsg)+3)/4]; }t_MBOX3;
static t_MBOX3 a_Mbx3[NO_ENTRIES3];
static t_MBOX3 *p_Put3 = a_Mbx3;
static t_MBOX3 *p_Get3 = a_Mbx3;
static volatile UINT16 ActNumEntries3 = 0;
static volatile UINT8 NestedCnt3 = 0;

/* definition of mailbox 4 */
#define NO_ENTRIES4    1
/* size of mailbox entries is defined by the size */
/* of the type PTP_t_MboxMsg extended to 32-bit alignment */
typedef struct { UINT32 adw_tmp[(sizeof(PTP_t_MboxMsg)+3)/4]; }t_MBOX4;
static t_MBOX4 a_Mbx4[NO_ENTRIES4];
static t_MBOX4 *p_Put4 = a_Mbx4;
static t_MBOX4 *p_Get4 = a_Mbx4;
static volatile UINT16 ActNumEntries4 = 0;
static volatile UINT8 NestedCnt4 = 0;

/* definition of mailbox 5 */
#define NO_ENTRIES5    10
/* size of mailbox entries is defined by the size */
/* of the type PTP_t_MboxMsg extended to 32-bit alignment */
typedef struct { UINT32 adw_tmp[(sizeof(PTP_t_MboxMsg)+3)/4]; }t_MBOX5;
static t_MBOX5 a_Mbx5[NO_ENTRIES5];
static t_MBOX5 *p_Put5 = a_Mbx5;
static t_MBOX5 *p_Get5 = a_Mbx5;
static volatile UINT16 ActNumEntries5 = 0;
static volatile UINT8 NestedCnt5 = 0;

/* definition of mailbox 6 */
#define NO_ENTRIES6    10
/* size of mailbox entries is defined by the size */
/* of the type PTP_t_MboxMsg extended to 32-bit alignment */
typedef struct { UINT32 adw_tmp[(sizeof(PTP_t_MboxMsg)+3)/4]; }t_MBOX6;
static t_MBOX6 a_Mbx6[NO_ENTRIES6];
static t_MBOX6 *p_Put6 = a_Mbx6;
static t_MBOX6 *p_Get6 = a_Mbx6;
static volatile UINT16 ActNumEntries6 = 0;
static volatile UINT8 NestedCnt6 = 0;

/* definition of mailbox 7 */
#define NO_ENTRIES7    10
/* size of mailbox entries is defined by the size */
/* of the type PTP_t_MboxMsg extended to 32-bit alignment */
typedef struct { UINT32 adw_tmp[(sizeof(PTP_t_MboxMsg)+3)/4]; }t_MBOX7;
static t_MBOX7 a_Mbx7[NO_ENTRIES7];
static t_MBOX7 *p_Put7 = a_Mbx7;
static t_MBOX7 *p_Get7 = a_Mbx7;
static volatile UINT16 ActNumEntries7 = 0;
static volatile UINT8 NestedCnt7 = 0;

/* definition of mailbox 8 */
#define NO_ENTRIES8    10
/* size of mailbox entries is defined by the size */
/* of the type PTP_t_MboxMsg extended to 32-bit alignment */
typedef struct { UINT32 adw_tmp[(sizeof(PTP_t_MboxMsg)+3)/4]; }t_MBOX8;
static t_MBOX8 a_Mbx8[NO_ENTRIES8];
static t_MBOX8 *p_Put8 = a_Mbx8;
static t_MBOX8 *p_Get8 = a_Mbx8;
static volatile UINT16 ActNumEntries8 = 0;
static volatile UINT8 NestedCnt8 = 0;

/* definition of mailbox 9 */
#define NO_ENTRIES9    10
/* size of mailbox entries is defined by the size */
/* of the type PTP_t_MboxMsg extended to 32-bit alignment */
typedef struct { UINT32 adw_tmp[(sizeof(PTP_t_MboxMsg)+3)/4]; }t_MBOX9;
static t_MBOX9 a_Mbx9[NO_ENTRIES9];
static t_MBOX9 *p_Put9 = a_Mbx9;
static t_MBOX9 *p_Get9 = a_Mbx9;
static volatile UINT16 ActNumEntries9 = 0;
static volatile UINT8 NestedCnt9 = 0;

/* definition of mailbox 10 */
#define NO_ENTRIES10    10
/* size of mailbox entries is defined by the size */
/* of the type PTP_t_MboxMsg extended to 32-bit alignment */
typedef struct { UINT32 adw_tmp[(sizeof(PTP_t_MboxMsg)+3)/4]; }t_MBOX10;
static t_MBOX10 a_Mbx10[NO_ENTRIES10];
static t_MBOX10 *p_Put10 = a_Mbx10;
static t_MBOX10 *p_Get10 = a_Mbx10;
static volatile UINT16 ActNumEntries10 = 0;
static volatile UINT8 NestedCnt10 = 0;

/* definition of mailbox 11 */
#define NO_ENTRIES11    10
/* size of mailbox entries is defined by the size */
/* of the type PTP_t_MboxMsg extended to 32-bit alignment */
typedef struct { UINT32 adw_tmp[(sizeof(PTP_t_MboxMsg)+3)/4]; }t_MBOX11;
static t_MBOX11 a_Mbx11[NO_ENTRIES11];
static t_MBOX11 *p_Put11 = a_Mbx11;
static t_MBOX11 *p_Get11 = a_Mbx11;
static volatile UINT16 ActNumEntries11 = 0;
static volatile UINT8 NestedCnt11 = 0;

/* definition of mailbox 12 */
#define NO_ENTRIES12    3
/* size of mailbox entries is defined by the size */
/* of the type PTP_t_MboxMsg extended to 32-bit alignment */
typedef struct { UINT32 adw_tmp[(sizeof(PTP_t_MboxMsg)+3)/4]; }t_MBOX12;
static t_MBOX12 a_Mbx12[NO_ENTRIES12];
static t_MBOX12 *p_Put12 = a_Mbx12;
static t_MBOX12 *p_Get12 = a_Mbx12;
static volatile UINT16 ActNumEntries12 = 0;
static volatile UINT8 NestedCnt12 = 0;

/* definition of mailbox 13 */
#define NO_ENTRIES13    3
/* size of mailbox entries is defined by the size */
/* of the type PTP_t_MboxMsg extended to 32-bit alignment */
typedef struct { UINT32 adw_tmp[(sizeof(PTP_t_MboxMsg)+3)/4]; }t_MBOX13;
static t_MBOX13 a_Mbx13[NO_ENTRIES13];
static t_MBOX13 *p_Put13 = a_Mbx13;
static t_MBOX13 *p_Get13 = a_Mbx13;
static volatile UINT16 ActNumEntries13 = 0;
static volatile UINT8 NestedCnt13 = 0;

/* definition of mailbox 14 */
#define NO_ENTRIES14    3
/* size of mailbox entries is defined by the size */
/* of the type PTP_t_MboxMsg extended to 32-bit alignment */
typedef struct { UINT32 adw_tmp[(sizeof(PTP_t_MboxMsg)+3)/4]; }t_MBOX14;
static t_MBOX14 a_Mbx14[NO_ENTRIES14];
static t_MBOX14 *p_Put14 = a_Mbx14;
static t_MBOX14 *p_Get14 = a_Mbx14;
static volatile UINT16 ActNumEntries14 = 0;
static volatile UINT8 NestedCnt14 = 0;

/* definition of mailbox 15 */
#define NO_ENTRIES15    1
/* size of mailbox entries is defined by the size */
/* of the type PTP_t_MboxMsg extended to 32-bit alignment */
typedef struct { UINT32 adw_tmp[(sizeof(PTP_t_MboxMsg)+3)/4]; }t_MBOX15;
static t_MBOX15 a_Mbx15[NO_ENTRIES15];
static t_MBOX15 *p_Put15 = a_Mbx15;
static t_MBOX15 *p_Get15 = a_Mbx15;
static volatile UINT16 ActNumEntries15 = 0;
static volatile UINT8 NestedCnt15 = 0;

/* definition of mailbox 16 */
#define NO_ENTRIES16    20
/* size of mailbox entries is defined by the size */
/* of the type PTP_t_MboxMsg extended to 32-bit alignment */
typedef struct { UINT32 adw_tmp[(sizeof(PTP_t_MboxMsg)+3)/4]; }t_MBOX16;
static t_MBOX16 a_Mbx16[NO_ENTRIES16];
static t_MBOX16 *p_Put16 = a_Mbx16;
static t_MBOX16 *p_Get16 = a_Mbx16;
static volatile UINT16 ActNumEntries16 = 0;
static volatile UINT8 NestedCnt16 = 0;

/* definition of mailbox 17 */
#define NO_ENTRIES17    1
/* size of mailbox entries is defined by the size */
/* of the type PTP_t_MboxMsg extended to 32-bit alignment */
typedef struct { UINT32 adw_tmp[(sizeof(PTP_t_MboxMsg)+3)/4]; }t_MBOX17;
static t_MBOX17 a_Mbx17[NO_ENTRIES17];
static t_MBOX17 *p_Put17 = a_Mbx17;
static t_MBOX17 *p_Get17 = a_Mbx17;
static volatile UINT16 ActNumEntries17 = 0;
static volatile UINT8 NestedCnt17 = 0;

/* definition of mailbox 18 */
#define NO_ENTRIES18    1
/* size of mailbox entries is defined by the size */
/* of the type PTP_t_MboxMsg extended to 32-bit alignment */
typedef struct { UINT32 adw_tmp[(sizeof(PTP_t_MboxMsg)+3)/4]; }t_MBOX18;
static t_MBOX18 a_Mbx18[NO_ENTRIES18];
static t_MBOX18 *p_Put18 = a_Mbx18;
static t_MBOX18 *p_Get18 = a_Mbx18;
static volatile UINT16 ActNumEntries18 = 0;
static volatile UINT8 NestedCnt18 = 0;


/*************************************************************************
**    static function-prototypes
*************************************************************************/

/*************************************************************************
**    global functions
*************************************************************************/

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
void SIS_MboxInit(void)
{
  /* init mailbox 0 */
  p_Put0 = a_Mbx0;
  p_Get0 = a_Mbx0;
  ActNumEntries0 = 0;
  NestedCnt0 = 0;
  /* init mailbox 1 */
  p_Put1 = a_Mbx1;
  p_Get1 = a_Mbx1;
  ActNumEntries1 = 0;
  NestedCnt1 = 0;
  /* init mailbox 2 */
  p_Put2 = a_Mbx2;
  p_Get2 = a_Mbx2;
  ActNumEntries2 = 0;
  NestedCnt2 = 0;
  /* init mailbox 3 */
  p_Put3 = a_Mbx3;
  p_Get3 = a_Mbx3;
  ActNumEntries3 = 0;
  NestedCnt3 = 0;
  /* init mailbox 4 */
  p_Put4 = a_Mbx4;
  p_Get4 = a_Mbx4;
  ActNumEntries4 = 0;
  NestedCnt4 = 0;
  /* init mailbox 5 */
  p_Put5 = a_Mbx5;
  p_Get5 = a_Mbx5;
  ActNumEntries5 = 0;
  NestedCnt5 = 0;
  /* init mailbox 6 */
  p_Put6 = a_Mbx6;
  p_Get6 = a_Mbx6;
  ActNumEntries6 = 0;
  NestedCnt6 = 0;
  /* init mailbox 7 */
  p_Put7 = a_Mbx7;
  p_Get7 = a_Mbx7;
  ActNumEntries7 = 0;
  NestedCnt7 = 0;
  /* init mailbox 8 */
  p_Put8 = a_Mbx8;
  p_Get8 = a_Mbx8;
  ActNumEntries8 = 0;
  NestedCnt8 = 0;
  /* init mailbox 9 */
  p_Put9 = a_Mbx9;
  p_Get9 = a_Mbx9;
  ActNumEntries9 = 0;
  NestedCnt9 = 0;
  /* init mailbox 10 */
  p_Put10 = a_Mbx10;
  p_Get10 = a_Mbx10;
  ActNumEntries10 = 0;
  NestedCnt10 = 0;
  /* init mailbox 11 */
  p_Put11 = a_Mbx11;
  p_Get11 = a_Mbx11;
  ActNumEntries11 = 0;
  NestedCnt11 = 0;
  /* init mailbox 12 */
  p_Put12 = a_Mbx12;
  p_Get12 = a_Mbx12;
  ActNumEntries12 = 0;
  NestedCnt12 = 0;
  /* init mailbox 13 */
  p_Put13 = a_Mbx13;
  p_Get13 = a_Mbx13;
  ActNumEntries13 = 0;
  NestedCnt13 = 0;
  /* init mailbox 14 */
  p_Put14 = a_Mbx14;
  p_Get14 = a_Mbx14;
  ActNumEntries14 = 0;
  NestedCnt14 = 0;
  /* init mailbox 15 */
  p_Put15 = a_Mbx15;
  p_Get15 = a_Mbx15;
  ActNumEntries15 = 0;
  NestedCnt15 = 0;
  /* init mailbox 16 */
  p_Put16 = a_Mbx16;
  p_Get16 = a_Mbx16;
  ActNumEntries16 = 0;
  NestedCnt16 = 0;
  /* init mailbox 17 */
  p_Put17 = a_Mbx17;
  p_Get17 = a_Mbx17;
  ActNumEntries17 = 0;
  NestedCnt17 = 0;
  /* init mailbox 18 */
  p_Put18 = a_Mbx18;
  p_Get18 = a_Mbx18;
  ActNumEntries18 = 0;
  NestedCnt18 = 0;
}

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
BOOLEAN SIS_MboxPut(UINT16 w_task_hdl, const void *pv_msg)
{
  void *ptr;

  switch(w_task_hdl)
  {
    case CTL_TSK:
    {
      /* free entry available ? */
      if(ActNumEntries0 < NO_ENTRIES0)
      {
        /* mark entry as occupied */
        ActNumEntries0++;
        NestedCnt0++;
        ptr = p_Put0;
        p_Put0++;
        if(p_Put0 == &a_Mbx0[NO_ENTRIES0])
        {
          p_Put0 = a_Mbx0;
        }
        /* copy message into mailbox */
        *(t_MBOX0*)ptr = *(t_MBOX0*)pv_msg;
        NestedCnt0--;
        SIS_TaskExeReq(w_task_hdl);
        return TRUE;
      }
      break;
    }
    case SLD_TSK:
    {
      /* free entry available ? */
      if(ActNumEntries1 < NO_ENTRIES1)
      {
        /* mark entry as occupied */
        ActNumEntries1++;
        NestedCnt1++;
        ptr = p_Put1;
        p_Put1++;
        if(p_Put1 == &a_Mbx1[NO_ENTRIES1])
        {
          p_Put1 = a_Mbx1;
        }
        /* copy message into mailbox */
        *(t_MBOX1*)ptr = *(t_MBOX1*)pv_msg;
        NestedCnt1--;
        SIS_TaskExeReq(w_task_hdl);
        return TRUE;
      }
      break;
    }
    case SLS_TSK:
    {
      /* free entry available ? */
      if(ActNumEntries2 < NO_ENTRIES2)
      {
        /* mark entry as occupied */
        ActNumEntries2++;
        NestedCnt2++;
        ptr = p_Put2;
        p_Put2++;
        if(p_Put2 == &a_Mbx2[NO_ENTRIES2])
        {
          p_Put2 = a_Mbx2;
        }
        /* copy message into mailbox */
        *(t_MBOX2*)ptr = *(t_MBOX2*)pv_msg;
        NestedCnt2--;
        SIS_TaskExeReq(w_task_hdl);
        return TRUE;
      }
      break;
    }
    case SLSintTmo_TSK:
    {
      /* free entry available ? */
      if(ActNumEntries3 < NO_ENTRIES3)
      {
        /* mark entry as occupied */
        ActNumEntries3++;
        NestedCnt3++;
        ptr = p_Put3;
        p_Put3++;
        if(p_Put3 == &a_Mbx3[NO_ENTRIES3])
        {
          p_Put3 = a_Mbx3;
        }
        /* copy message into mailbox */
        *(t_MBOX3*)ptr = *(t_MBOX3*)pv_msg;
        NestedCnt3--;
        SIS_TaskExeReq(w_task_hdl);
        return TRUE;
      }
      break;
    }
    case SLSrcvTmo_TSK:
    {
      /* free entry available ? */
      if(ActNumEntries4 < NO_ENTRIES4)
      {
        /* mark entry as occupied */
        ActNumEntries4++;
        NestedCnt4++;
        ptr = p_Put4;
        p_Put4++;
        if(p_Put4 == &a_Mbx4[NO_ENTRIES4])
        {
          p_Put4 = a_Mbx4;
        }
        /* copy message into mailbox */
        *(t_MBOX4*)ptr = *(t_MBOX4*)pv_msg;
        NestedCnt4--;
        SIS_TaskExeReq(w_task_hdl);
        return TRUE;
      }
      break;
    }
    case MAS1_TSK:
    {
      /* free entry available ? */
      if(ActNumEntries5 < NO_ENTRIES5)
      {
        /* mark entry as occupied */
        ActNumEntries5++;
        NestedCnt5++;
        ptr = p_Put5;
        p_Put5++;
        if(p_Put5 == &a_Mbx5[NO_ENTRIES5])
        {
          p_Put5 = a_Mbx5;
        }
        /* copy message into mailbox */
        *(t_MBOX5*)ptr = *(t_MBOX5*)pv_msg;
        NestedCnt5--;
        SIS_TaskExeReq(w_task_hdl);
        return TRUE;
      }
      break;
    }
    case MAS2_TSK:
    {
      /* free entry available ? */
      if(ActNumEntries6 < NO_ENTRIES6)
      {
        /* mark entry as occupied */
        ActNumEntries6++;
        NestedCnt6++;
        ptr = p_Put6;
        p_Put6++;
        if(p_Put6 == &a_Mbx6[NO_ENTRIES6])
        {
          p_Put6 = a_Mbx6;
        }
        /* copy message into mailbox */
        *(t_MBOX6*)ptr = *(t_MBOX6*)pv_msg;
        NestedCnt6--;
        SIS_TaskExeReq(w_task_hdl);
        return TRUE;
      }
      break;
    }
    case MAD1_TSK:
    {
      /* free entry available ? */
      if(ActNumEntries7 < NO_ENTRIES7)
      {
        /* mark entry as occupied */
        ActNumEntries7++;
        NestedCnt7++;
        ptr = p_Put7;
        p_Put7++;
        if(p_Put7 == &a_Mbx7[NO_ENTRIES7])
        {
          p_Put7 = a_Mbx7;
        }
        /* copy message into mailbox */
        *(t_MBOX7*)ptr = *(t_MBOX7*)pv_msg;
        NestedCnt7--;
        SIS_TaskExeReq(w_task_hdl);
        return TRUE;
      }
      break;
    }
    case MAD2_TSK:
    {
      /* free entry available ? */
      if(ActNumEntries8 < NO_ENTRIES8)
      {
        /* mark entry as occupied */
        ActNumEntries8++;
        NestedCnt8++;
        ptr = p_Put8;
        p_Put8++;
        if(p_Put8 == &a_Mbx8[NO_ENTRIES8])
        {
          p_Put8 = a_Mbx8;
        }
        /* copy message into mailbox */
        *(t_MBOX8*)ptr = *(t_MBOX8*)pv_msg;
        NestedCnt8--;
        SIS_TaskExeReq(w_task_hdl);
        return TRUE;
      }
      break;
    }
    case MAA1_TSK:
    {
      /* free entry available ? */
      if(ActNumEntries9 < NO_ENTRIES9)
      {
        /* mark entry as occupied */
        ActNumEntries9++;
        NestedCnt9++;
        ptr = p_Put9;
        p_Put9++;
        if(p_Put9 == &a_Mbx9[NO_ENTRIES9])
        {
          p_Put9 = a_Mbx9;
        }
        /* copy message into mailbox */
        *(t_MBOX9*)ptr = *(t_MBOX9*)pv_msg;
        NestedCnt9--;
        SIS_TaskExeReq(w_task_hdl);
        return TRUE;
      }
      break;
    }
    case MAA2_TSK:
    {
      /* free entry available ? */
      if(ActNumEntries10 < NO_ENTRIES10)
      {
        /* mark entry as occupied */
        ActNumEntries10++;
        NestedCnt10++;
        ptr = p_Put10;
        p_Put10++;
        if(p_Put10 == &a_Mbx10[NO_ENTRIES10])
        {
          p_Put10 = a_Mbx10;
        }
        /* copy message into mailbox */
        *(t_MBOX10*)ptr = *(t_MBOX10*)pv_msg;
        NestedCnt10--;
        SIS_TaskExeReq(w_task_hdl);
        return TRUE;
      }
      break;
    }
    case MNT_TSK:
    {
      /* free entry available ? */
      if(ActNumEntries11 < NO_ENTRIES11)
      {
        /* mark entry as occupied */
        ActNumEntries11++;
        NestedCnt11++;
        ptr = p_Put11;
        p_Put11++;
        if(p_Put11 == &a_Mbx11[NO_ENTRIES11])
        {
          p_Put11 = a_Mbx11;
        }
        /* copy message into mailbox */
        *(t_MBOX11*)ptr = *(t_MBOX11*)pv_msg;
        NestedCnt11--;
        SIS_TaskExeReq(w_task_hdl);
        return TRUE;
      }
      break;
    }
    case UCMann_TSK:
    {
      /* free entry available ? */
      if(ActNumEntries12 < NO_ENTRIES12)
      {
        /* mark entry as occupied */
        ActNumEntries12++;
        NestedCnt12++;
        ptr = p_Put12;
        p_Put12++;
        if(p_Put12 == &a_Mbx12[NO_ENTRIES12])
        {
          p_Put12 = a_Mbx12;
        }
        /* copy message into mailbox */
        *(t_MBOX12*)ptr = *(t_MBOX12*)pv_msg;
        NestedCnt12--;
        SIS_TaskExeReq(w_task_hdl);
        return TRUE;
      }
      break;
    }
    case UCMsyn_TSK:
    {
      /* free entry available ? */
      if(ActNumEntries13 < NO_ENTRIES13)
      {
        /* mark entry as occupied */
        ActNumEntries13++;
        NestedCnt13++;
        ptr = p_Put13;
        p_Put13++;
        if(p_Put13 == &a_Mbx13[NO_ENTRIES13])
        {
          p_Put13 = a_Mbx13;
        }
        /* copy message into mailbox */
        *(t_MBOX13*)ptr = *(t_MBOX13*)pv_msg;
        NestedCnt13--;
        SIS_TaskExeReq(w_task_hdl);
        return TRUE;
      }
      break;
    }
    case UCMdel_TSK:
    {
      /* free entry available ? */
      if(ActNumEntries14 < NO_ENTRIES14)
      {
        /* mark entry as occupied */
        ActNumEntries14++;
        NestedCnt14++;
        ptr = p_Put14;
        p_Put14++;
        if(p_Put14 == &a_Mbx14[NO_ENTRIES14])
        {
          p_Put14 = a_Mbx14;
        }
        /* copy message into mailbox */
        *(t_MBOX14*)ptr = *(t_MBOX14*)pv_msg;
        NestedCnt14--;
        SIS_TaskExeReq(w_task_hdl);
        return TRUE;
      }
      break;
    }
    case UCD_TSK:
    {
      /* free entry available ? */
      if(ActNumEntries15 < NO_ENTRIES15)
      {
        /* mark entry as occupied */
        ActNumEntries15++;
        NestedCnt15++;
        ptr = p_Put15;
        p_Put15++;
        if(p_Put15 == &a_Mbx15[NO_ENTRIES15])
        {
          p_Put15 = a_Mbx15;
        }
        /* copy message into mailbox */
        *(t_MBOX15*)ptr = *(t_MBOX15*)pv_msg;
        NestedCnt15--;
        SIS_TaskExeReq(w_task_hdl);
        return TRUE;
      }
      break;
    }
    case DIS_TSK:
    {
      /* free entry available ? */
      if(ActNumEntries16 < NO_ENTRIES16)
      {
        /* mark entry as occupied */
        ActNumEntries16++;
        NestedCnt16++;
        ptr = p_Put16;
        p_Put16++;
        if(p_Put16 == &a_Mbx16[NO_ENTRIES16])
        {
          p_Put16 = a_Mbx16;
        }
        /* copy message into mailbox */
        *(t_MBOX16*)ptr = *(t_MBOX16*)pv_msg;
        NestedCnt16--;
        SIS_TaskExeReq(w_task_hdl);
        return TRUE;
      }
      break;
    }
    case DISnet_TSK:
    {
      /* free entry available ? */
      if(ActNumEntries17 < NO_ENTRIES17)
      {
        /* mark entry as occupied */
        ActNumEntries17++;
        NestedCnt17++;
        ptr = p_Put17;
        p_Put17++;
        if(p_Put17 == &a_Mbx17[NO_ENTRIES17])
        {
          p_Put17 = a_Mbx17;
        }
        /* copy message into mailbox */
        *(t_MBOX17*)ptr = *(t_MBOX17*)pv_msg;
        NestedCnt17--;
        SIS_TaskExeReq(w_task_hdl);
        return TRUE;
      }
      break;
    }
    case DISlat_TSK:
    {
      /* free entry available ? */
      if(ActNumEntries18 < NO_ENTRIES18)
      {
        /* mark entry as occupied */
        ActNumEntries18++;
        NestedCnt18++;
        ptr = p_Put18;
        p_Put18++;
        if(p_Put18 == &a_Mbx18[NO_ENTRIES18])
        {
          p_Put18 = a_Mbx18;
        }
        /* copy message into mailbox */
        *(t_MBOX18*)ptr = *(t_MBOX18*)pv_msg;
        NestedCnt18--;
        SIS_TaskExeReq(w_task_hdl);
        return TRUE;
      }
      break;
    }
    default:
    {
      assert(0);
    }
  }
  return FALSE;
}

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
void SIS_MboxWListAdd(UINT16 w_tx_task_hdl, UINT16 w_rx_task_hdl)
{
  switch(w_rx_task_hdl)
  {
    case CTL_TSK:
    {
      /* store task in waiting list */
      switch(w_tx_task_hdl)
      {
        case MNT_TSK: o_Waiting_0_0 = 1; break;
        case SLS_TSK: o_Waiting_0_1 = 1; break;
        case SLD_TSK: o_Waiting_0_2 = 1; break;
        default: assert(0);
      }
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
void *SIS_MboxGet( void )
{
  switch(SIS_w_RunningTask)
  {
    case CTL_TSK:
    {
      /* mailbox entry available and already valid */
      if((ActNumEntries0 != 0) && (NestedCnt0 == 0))
      {
        return p_Get0;
      }
      break;
    }
    case SLD_TSK:
    {
      /* mailbox entry available and already valid */
      if((ActNumEntries1 != 0) && (NestedCnt1 == 0))
      {
        return p_Get1;
      }
      break;
    }
    case SLS_TSK:
    {
      /* mailbox entry available and already valid */
      if((ActNumEntries2 != 0) && (NestedCnt2 == 0))
      {
        return p_Get2;
      }
      break;
    }
    case SLSintTmo_TSK:
    {
      /* mailbox entry available and already valid */
      if((ActNumEntries3 != 0) && (NestedCnt3 == 0))
      {
        return p_Get3;
      }
      break;
    }
    case SLSrcvTmo_TSK:
    {
      /* mailbox entry available and already valid */
      if((ActNumEntries4 != 0) && (NestedCnt4 == 0))
      {
        return p_Get4;
      }
      break;
    }
    case MAS1_TSK:
    {
      /* mailbox entry available and already valid */
      if((ActNumEntries5 != 0) && (NestedCnt5 == 0))
      {
        return p_Get5;
      }
      break;
    }
    case MAS2_TSK:
    {
      /* mailbox entry available and already valid */
      if((ActNumEntries6 != 0) && (NestedCnt6 == 0))
      {
        return p_Get6;
      }
      break;
    }
    case MAD1_TSK:
    {
      /* mailbox entry available and already valid */
      if((ActNumEntries7 != 0) && (NestedCnt7 == 0))
      {
        return p_Get7;
      }
      break;
    }
    case MAD2_TSK:
    {
      /* mailbox entry available and already valid */
      if((ActNumEntries8 != 0) && (NestedCnt8 == 0))
      {
        return p_Get8;
      }
      break;
    }
    case MAA1_TSK:
    {
      /* mailbox entry available and already valid */
      if((ActNumEntries9 != 0) && (NestedCnt9 == 0))
      {
        return p_Get9;
      }
      break;
    }
    case MAA2_TSK:
    {
      /* mailbox entry available and already valid */
      if((ActNumEntries10 != 0) && (NestedCnt10 == 0))
      {
        return p_Get10;
      }
      break;
    }
    case MNT_TSK:
    {
      /* mailbox entry available and already valid */
      if((ActNumEntries11 != 0) && (NestedCnt11 == 0))
      {
        return p_Get11;
      }
      break;
    }
    case UCMann_TSK:
    {
      /* mailbox entry available and already valid */
      if((ActNumEntries12 != 0) && (NestedCnt12 == 0))
      {
        return p_Get12;
      }
      break;
    }
    case UCMsyn_TSK:
    {
      /* mailbox entry available and already valid */
      if((ActNumEntries13 != 0) && (NestedCnt13 == 0))
      {
        return p_Get13;
      }
      break;
    }
    case UCMdel_TSK:
    {
      /* mailbox entry available and already valid */
      if((ActNumEntries14 != 0) && (NestedCnt14 == 0))
      {
        return p_Get14;
      }
      break;
    }
    case UCD_TSK:
    {
      /* mailbox entry available and already valid */
      if((ActNumEntries15 != 0) && (NestedCnt15 == 0))
      {
        return p_Get15;
      }
      break;
    }
    case DIS_TSK:
    {
      /* mailbox entry available and already valid */
      if((ActNumEntries16 != 0) && (NestedCnt16 == 0))
      {
        return p_Get16;
      }
      break;
    }
    case DISnet_TSK:
    {
      /* mailbox entry available and already valid */
      if((ActNumEntries17 != 0) && (NestedCnt17 == 0))
      {
        return p_Get17;
      }
      break;
    }
    case DISlat_TSK:
    {
      /* mailbox entry available and already valid */
      if((ActNumEntries18 != 0) && (NestedCnt18 == 0))
      {
        return p_Get18;
      }
      break;
    }
    default:
    {
      assert(0);
    }
  }
  return NULL;
}

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
void SIS_MboxRelease( void )
{
  switch(SIS_w_RunningTask)
  {
    case CTL_TSK:
    {
      /* release entry */
      p_Get0++;
      if(p_Get0 == &a_Mbx0[NO_ENTRIES0])
      {
        p_Get0 = a_Mbx0;
      }
      ActNumEntries0--;
      /* delete waiting task from list */
      if(o_Waiting_0_0)
      {
        o_Waiting_0_0 = 0;
        SIS_ab_TaskSts[MNT_TSK] = TASK_k_READY;
      }
      else if(o_Waiting_0_1)
      {
        o_Waiting_0_1 = 0;
        SIS_ab_TaskSts[SLS_TSK] = TASK_k_READY;
      }
      else if(o_Waiting_0_2)
      {
        o_Waiting_0_2 = 0;
        SIS_ab_TaskSts[SLD_TSK] = TASK_k_READY;
      }
      break;
    }
    case SLD_TSK:
    {
      /* release entry */
      p_Get1++;
      if(p_Get1 == &a_Mbx1[NO_ENTRIES1])
      {
        p_Get1 = a_Mbx1;
      }
      ActNumEntries1--;
      break;
    }
    case SLS_TSK:
    {
      /* release entry */
      p_Get2++;
      if(p_Get2 == &a_Mbx2[NO_ENTRIES2])
      {
        p_Get2 = a_Mbx2;
      }
      ActNumEntries2--;
      break;
    }
    case SLSintTmo_TSK:
    {
      /* release entry */
      p_Get3++;
      if(p_Get3 == &a_Mbx3[NO_ENTRIES3])
      {
        p_Get3 = a_Mbx3;
      }
      ActNumEntries3--;
      break;
    }
    case SLSrcvTmo_TSK:
    {
      /* release entry */
      p_Get4++;
      if(p_Get4 == &a_Mbx4[NO_ENTRIES4])
      {
        p_Get4 = a_Mbx4;
      }
      ActNumEntries4--;
      break;
    }
    case MAS1_TSK:
    {
      /* release entry */
      p_Get5++;
      if(p_Get5 == &a_Mbx5[NO_ENTRIES5])
      {
        p_Get5 = a_Mbx5;
      }
      ActNumEntries5--;
      break;
    }
    case MAS2_TSK:
    {
      /* release entry */
      p_Get6++;
      if(p_Get6 == &a_Mbx6[NO_ENTRIES6])
      {
        p_Get6 = a_Mbx6;
      }
      ActNumEntries6--;
      break;
    }
    case MAD1_TSK:
    {
      /* release entry */
      p_Get7++;
      if(p_Get7 == &a_Mbx7[NO_ENTRIES7])
      {
        p_Get7 = a_Mbx7;
      }
      ActNumEntries7--;
      break;
    }
    case MAD2_TSK:
    {
      /* release entry */
      p_Get8++;
      if(p_Get8 == &a_Mbx8[NO_ENTRIES8])
      {
        p_Get8 = a_Mbx8;
      }
      ActNumEntries8--;
      break;
    }
    case MAA1_TSK:
    {
      /* release entry */
      p_Get9++;
      if(p_Get9 == &a_Mbx9[NO_ENTRIES9])
      {
        p_Get9 = a_Mbx9;
      }
      ActNumEntries9--;
      break;
    }
    case MAA2_TSK:
    {
      /* release entry */
      p_Get10++;
      if(p_Get10 == &a_Mbx10[NO_ENTRIES10])
      {
        p_Get10 = a_Mbx10;
      }
      ActNumEntries10--;
      break;
    }
    case MNT_TSK:
    {
      /* release entry */
      p_Get11++;
      if(p_Get11 == &a_Mbx11[NO_ENTRIES11])
      {
        p_Get11 = a_Mbx11;
      }
      ActNumEntries11--;
      break;
    }
    case UCMann_TSK:
    {
      /* release entry */
      p_Get12++;
      if(p_Get12 == &a_Mbx12[NO_ENTRIES12])
      {
        p_Get12 = a_Mbx12;
      }
      ActNumEntries12--;
      break;
    }
    case UCMsyn_TSK:
    {
      /* release entry */
      p_Get13++;
      if(p_Get13 == &a_Mbx13[NO_ENTRIES13])
      {
        p_Get13 = a_Mbx13;
      }
      ActNumEntries13--;
      break;
    }
    case UCMdel_TSK:
    {
      /* release entry */
      p_Get14++;
      if(p_Get14 == &a_Mbx14[NO_ENTRIES14])
      {
        p_Get14 = a_Mbx14;
      }
      ActNumEntries14--;
      break;
    }
    case UCD_TSK:
    {
      /* release entry */
      p_Get15++;
      if(p_Get15 == &a_Mbx15[NO_ENTRIES15])
      {
        p_Get15 = a_Mbx15;
      }
      ActNumEntries15--;
      break;
    }
    case DIS_TSK:
    {
      /* release entry */
      p_Get16++;
      if(p_Get16 == &a_Mbx16[NO_ENTRIES16])
      {
        p_Get16 = a_Mbx16;
      }
      ActNumEntries16--;
      break;
    }
    case DISnet_TSK:
    {
      /* release entry */
      p_Get17++;
      if(p_Get17 == &a_Mbx17[NO_ENTRIES17])
      {
        p_Get17 = a_Mbx17;
      }
      ActNumEntries17--;
      break;
    }
    case DISlat_TSK:
    {
      /* release entry */
      p_Get18++;
      if(p_Get18 == &a_Mbx18[NO_ENTRIES18])
      {
        p_Get18 = a_Mbx18;
      }
      ActNumEntries18--;
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
UINT16 SIS_MboxQuery(UINT16 w_task_hdl)
{
  switch(w_task_hdl)
  {
    case CTL_TSK:
    {
      return NO_ENTRIES0 - ActNumEntries0;
    }
    case SLD_TSK:
    {
      return NO_ENTRIES1 - ActNumEntries1;
    }
    case SLS_TSK:
    {
      return NO_ENTRIES2 - ActNumEntries2;
    }
    case SLSintTmo_TSK:
    {
      return NO_ENTRIES3 - ActNumEntries3;
    }
    case SLSrcvTmo_TSK:
    {
      return NO_ENTRIES4 - ActNumEntries4;
    }
    case MAS1_TSK:
    {
      return NO_ENTRIES5 - ActNumEntries5;
    }
    case MAS2_TSK:
    {
      return NO_ENTRIES6 - ActNumEntries6;
    }
    case MAD1_TSK:
    {
      return NO_ENTRIES7 - ActNumEntries7;
    }
    case MAD2_TSK:
    {
      return NO_ENTRIES8 - ActNumEntries8;
    }
    case MAA1_TSK:
    {
      return NO_ENTRIES9 - ActNumEntries9;
    }
    case MAA2_TSK:
    {
      return NO_ENTRIES10 - ActNumEntries10;
    }
    case MNT_TSK:
    {
      return NO_ENTRIES11 - ActNumEntries11;
    }
    case UCMann_TSK:
    {
      return NO_ENTRIES12 - ActNumEntries12;
    }
    case UCMsyn_TSK:
    {
      return NO_ENTRIES13 - ActNumEntries13;
    }
    case UCMdel_TSK:
    {
      return NO_ENTRIES14 - ActNumEntries14;
    }
    case UCD_TSK:
    {
      return NO_ENTRIES15 - ActNumEntries15;
    }
    case DIS_TSK:
    {
      return NO_ENTRIES16 - ActNumEntries16;
    }
    case DISnet_TSK:
    {
      return NO_ENTRIES17 - ActNumEntries17;
    }
    case DISlat_TSK:
    {
      return NO_ENTRIES18 - ActNumEntries18;
    }
    default:
    {
      assert(0);
    }
  }
  return 0;/*lint !e527*/
}

/*************************************************************************
**    static functions
*************************************************************************/

