/*************************************************************************
**    (C) 2008 - 2009 IXXAT Automation GmbH, all rights reserved
**************************************************************************
**
**       File: TSUint.h 
**    Summary: Declares the internal types for the Unit TSU.
**             The timestamping functions are the most time-critical 
**             functions in the hole protocol-stack. 
** 
**    Version: 1.01.01
**
**************************************************************************
**************************************************************************
**
**  Functions: -
**
**   Compiler: gcc 3.3.2
**    Remarks: 
** 
**************************************************************************
**    all rights reserved
*************************************************************************/

#ifndef __TSUINT_H__
#define __TSUINT_H__

#ifdef __cplusplus
extern "C"
{
#endif

/*************************************************************************
**    constants and macros
*************************************************************************/
/* errors*/
#define TSU_k_INITERR      (0)
#define TSU_k_ERR_RXTS     (1)
#define TSU_k_ERR_TXTS     (2)

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
#endif /* __TSUINT_H__ */
