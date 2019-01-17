/*************************************************************************
**    (C) 2008 - 2009 IXXAT Automation GmbH, all rights reserved
**************************************************************************
**
**       File: FIOerr.c
**    Summary: config file scanner/parser
**    Version: 1.01.01
**    Version: 1.01.01
**
**************************************************************************
**************************************************************************
**
**  Functions: FIO_GetErrStr
**
**
**   Compiler: Ansi-C
**    Remarks:
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
#include "PTP/PTPdef.h"
#include "FIO/FIOint.h"
#include "FIO/FIO.h"
/*************************************************************************
**    global variables
*************************************************************************/

/*************************************************************************
**    static constants, types, macros, variables, function prototypes
*************************************************************************/

/*************************************************************************
**    static function-prototypes
*************************************************************************/

/*************************************************************************
**    global functions
*************************************************************************/
#ifdef ERR_STR
/*************************************************************************
**
** Function    : FIO_GetErrStr
**
** Description : Resolves the error number to the according error string
**
** Parameters  : dw_errNmb (IN) - error number
**               
** Returnvalue : const CHAR*   - pointer to error string
** 
** Remarks     : Gets compiled with definition of ERR_STR.
**  
***********************************************************************/
const CHAR* FIO_GetErrStr(UINT32 dw_errNmb)
{
  const CHAR *pc_ret;
                    
  /* handle errors */
  switch(dw_errNmb)
  {
    case FIO_k_UNKWN_PARAM:
    {
      pc_ret = "Unknown parameter in file while reading";
      break;
    }
    case FIO_k_INV_STRLEN:
    {
      pc_ret = "Invalid string length in file while reading";
      break;
    }
    case FIO_k_INV_IDLEN:
    {
      pc_ret = "Invalid identifier length in file while reading";
      break;
    }
    case FIO_k_INV_IDX:
    {
      pc_ret = "Invalid index in file while reading";
      break;
    }
    case FIO_k_NUM_LEN:
    {
      pc_ret = "Invalid variable size of a numerical value";
      break;
    }
    case FIO_k_INV_NMB:
    {
      pc_ret = "Invalid number of values";
      break;
    }
    case FIO_k_INV_PDEF:
    {
      pc_ret = "Invalid parameter definition";
      break;
    }
    case FIO_k_WRG_TYPE:
    {
      pc_ret = "Other data type expected";
      break;
    }
    case FIO_k_ERR_UCMTBL:
    {
      pc_ret = "Ascii scanner detected problem in unicast master table";
      break;
    }
    case FIO_k_ERR_CFG:
    {
      pc_ret = "Ascii scanner detected problem in configuration file";
      break;
    }
    case FIO_k_ERR_FLT:
    {
      pc_ret = "Ascii scanner detected problem in filter file";
      break;
    }
    case FIO_k_ERR_CRCV:
    {
      pc_ret = "Ascii scanner detected problem in clock recovery file";
      break;
    }
    case FIO_k_OOR_ERR:
    {
      pc_ret = "Profile check - out of range error - value will be defaulted";
      break;
    }
    default:
    {
      pc_ret = "unknown error";
      break;
    }
  }
  return pc_ret;
}
#endif

/*************************************************************************
**    static functions
*************************************************************************/
