/*************************************************************************
**    (C) 2008 - 2009 IXXAT Automation GmbH, all rights reserved
**************************************************************************
**
**       File: SIShdls.h
**    Summary: SIS header file to declare eventhandles, taskhandles
**             and memorypoolhandles of the Stack Internal
**             Scheduling unit
**
**    Version:
**
**************************************************************************
**************************************************************************
**
**  Functions: -
**
**************************************************************************
**    all rights reserved
*************************************************************************/

/*************************************************************************
**    constants and macros
*************************************************************************/

/* number of SIS tasks */
#define SIS_k_NO_TASKS  19

/* SIS task handles for task scheduling */
#define CTL_TSK      ((UINT8)0)
#define SLD_TSK      ((UINT8)1)
#define SLS_TSK      ((UINT8)2)
#define SLSintTmo_TSK ((UINT8)3)
#define SLSrcvTmo_TSK ((UINT8)4)
#define MAS1_TSK     ((UINT8)5)
#define MAS2_TSK     ((UINT8)6)
#define MAD1_TSK     ((UINT8)7)
#define MAD2_TSK     ((UINT8)8)
#define MAA1_TSK     ((UINT8)9)
#define MAA2_TSK     ((UINT8)10)
#define MNT_TSK      ((UINT8)11)
#define UCMann_TSK   ((UINT8)12)
#define UCMsyn_TSK   ((UINT8)13)
#define UCMdel_TSK   ((UINT8)14)
#define UCD_TSK      ((UINT8)15)
#define DIS_TSK      ((UINT8)16)
#define DISnet_TSK   ((UINT8)17)
#define DISlat_TSK   ((UINT8)18)

/* SIS event handles */
/* Events for task CTL_TSK */
#define k_EV_INIT    ((UINT16)(1<<0))
#define k_EV_SYN_TMT ((UINT16)(1<<1))
#define k_EV_SC_SLV  ((UINT16)(1<<2))
#define k_EV_SC_PSV  ((UINT16)(1<<3))
#define k_EV_SC_MST  ((UINT16)(1<<4))
#define k_EV_SC_FLT  ((UINT16)(1<<5))
#define k_EV_SC_DSBL ((UINT16)(1<<6))
#define k_EV_SC_ENBL ((UINT16)(1<<7))
#define k_EV_SETPREF ((UINT16)(1<<8))
#define k_EV_CLRPREF ((UINT16)(1<<9))
#define k_EV_DONE    ((UINT16)(1<<10))
#define k_EV_SYNC    ((UINT16)(1<<11))
#define k_EV_FLT_CLRD ((UINT16)(1<<12))

/* memory pool handles for memory management */
#define k_POOL_64    ((UINT16)0)
#define k_POOL_128   ((UINT16)1)
#define k_POOL_256   ((UINT16)2)
#define k_POOL_1536  ((UINT16)3)
