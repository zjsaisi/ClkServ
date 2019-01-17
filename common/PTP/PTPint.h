/*************************************************************************
**    (C) 2008 - 2009 IXXAT Automation GmbH, all rights reserved
**************************************************************************
**
**       File: PTPint.h  
**    Summary: Internal definitions for the unit PTP
**
**    Version: 1.01.01
**
**************************************************************************
**************************************************************************
**
**  Functions:
**
**   Compiler: Ansi-C
**    Remarks: 
**
**
**************************************************************************
**    all rights reserved
*************************************************************************/

#ifndef __PTPINT_H__
#define __PTPINT_H__

#ifdef __cplusplus
extern "C"
{
#endif

/*************************************************************************
**    constants and macros
*************************************************************************/
/* error definitions */
#define PTP_k_MSGBOX_MNT    (0u) /* message box of unit MNT full */ 
#define PTP_k_MSGBOX_CTL    (1u) /* message box of unit CTL full */
#define PTP_k_ERR_INTV      (2u)  
#define PTP_k_ERR_CLIP_I64  (3u) /* clipping in addition function */ 

#define k_U48_OVRFLW_MSK ((UINT64)0xFFFF000000000000ULL)
#define k_I48_OVRFLW_MSK   ((INT64)0xFFFF800000000000LL)
#define k_I48_IS_NEG       ((INT64)0x8000000000000000LL)
#define k_MAX_NEG_I48      ((INT64)0x8000000000000000LL)
#define k_SCL_OVFL       ((UINT64)0xC000000000000000ULL)

/* error weight variables */
#define k_WGHT_ERR_ALRT (128u)
#define k_WGHT_ERR_CRIT  (64u)
#define k_WGHT_ERR_ERR   (32u)
#define k_WGHT_ERR_WARN   (1u)
/*************************************************************************
**    data types
*************************************************************************/

/*************************************************************************
**    global variables
*************************************************************************/

/*************************************************************************
**    function prototypes
*************************************************************************/

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* __PTPINT_H__ */

