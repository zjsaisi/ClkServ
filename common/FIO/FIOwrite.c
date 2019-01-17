/*************************************************************************
**    (C) 2008 - 2009 IXXAT Automation GmbH, all rights reserved
**************************************************************************
**
**       File: FIOwrite.c
**    Summary: config file writer
**    Version: 1.01.01
**    Version: 1.01.01
**
**************************************************************************
**************************************************************************
**             
**  Functions: FIO_WriteParam
**             str_copy
**             data_copy
**             dump
**             ll_to_a
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
#include "NIF/NIF.h"
#include "PTP/PTP.h"
#include "FIO/FIOint.h"

/*************************************************************************
**    global variables
*************************************************************************/

/*************************************************************************
**    static constants, types, macros, variables
*************************************************************************/
#define k_BLANKS "                                                              "
#define k_IDENT  (2u) /* number of blanks for ascii file identation */
#define MIN(a,b)     (((a) < (b)) ? (a) : (b) )
#define k_MAX_LL_STR (21u)  /* maximum length of a 64bit number 
                               represented as undotted string */
#define k_DGT_ZERO   (0x30) /* ascii representation of zero */
#define k_BASE_10    ((INT64)10)

/*************************************************************************
**    static function-prototypes
*************************************************************************/
static CHAR* str_copy(CHAR *pc_dest, const CHAR *pc_end, 
                      const CHAR *pc_src, INT32 l_cnt);
static CHAR* data_copy(CHAR *pc_dest, const CHAR *pc_end, 
                       const FIO_t_ParamDef *ps_param_def, UINT16 w_offs);
static CHAR* dump(CHAR *pc_pos, CHAR *pc_end,const FIO_t_ParamDef *ps_pDef, 
                  UINT16 w_offs, UINT16 w_lvl);
static CHAR* ll_to_a(CHAR *pc_dest, const CHAR *pc_end,INT64 ll_val);
/*************************************************************************
**    global functions
*************************************************************************/

/*************************************************************************
**
** Function    : FIO_WriteParam
**
** Description : Write config file so that it can be read again via
**               FIO_ReadParam.
**
** Parameters  : pc_dest     (OUT): pointer to the output ASCII stream for
**                                  the config file.
**               dw_buf_size  (IN): size of the buffer behind p_dest
**               ps_param_def (IN): Pointer to the definition table with
**                                  all config parameter names and attributes.
**
** Returnvalue : number of bytes to written or 0
**
*************************************************************************/
UINT32 FIO_WriteParam(CHAR *pc_dest, UINT32 dw_buf_size, 
                      const FIO_t_ParamDef *ps_param_def)
{
  CHAR *pc_end;
  pc_end = dump(pc_dest, pc_dest + (dw_buf_size - 2), ps_param_def, 0, 0);

  if(pc_end != NULL)
  {
    return (UINT32)(pc_end - pc_dest); /*lint !e946*/
  }
  return 0;
}

/*************************************************************************
**    static functions
*************************************************************************/

/***********************************************************************
**  
** Function    : str_copy
**  
** Description : Copies a string into buffer
**  
** Parameters  : pc_dest (IN) - buffer position to write 
**               pc_end  (IN) - buffer end postion 
**               pc_src  (IN) - source string to write 
**               l_cnt   (IN) - number of bytes to write
**               
** Returnvalue : new buffer write position (behind copied data)
** 
** Remarks     : -
**  
***********************************************************************/
static CHAR* str_copy(CHAR *pc_dest, const CHAR *pc_end, 
                      const CHAR *pc_src, INT32 l_cnt)
{
  if(l_cnt < 0)   /* unknown string length ? */
  {
    l_cnt = (INT32)PTP_BLEN(pc_src);
  }
  /* end of buffer reached ? */
  if(((pc_dest + l_cnt) >= pc_end) || (pc_dest == NULL))/*lint !e946*/
  {
    return NULL;
  }
  PTP_BCOPY(pc_dest, pc_src, l_cnt);
  return (pc_dest + l_cnt);
}    

/***********************************************************************
**  
** Function    : data_copy
**  
** Description : copies data to character buffer
**  
** Parameters  : pc_dest      (IN) - buffer position to write 
**               pc_end       (IN) - buffer end postion 
**               ps_param_def (IN) - configuration parameter definition
**               w_offs       (IN) - write offset in arrays 
**               
** Returnvalue : new buffer write position (behind copied data)
** 
** Remarks     : -
**  
***********************************************************************/
static CHAR* data_copy(CHAR *pc_dest, const CHAR *pc_end, 
                       const FIO_t_ParamDef *ps_param_def, UINT16 w_offs)
{
  UINT16 w_size;
  INT64  ll_nmb;
  UINT16 w_strLen;
  UINT8 *pb_src = &((UINT8*)ps_param_def->pv_ref)[w_offs];

  /* string copy */
  if(ps_param_def->s_type == FIO_k_STRING)
  {
    pc_dest  = str_copy(pc_dest, pc_end, "\"", -1);
    w_strLen = (UINT16)PTP_BLEN(pb_src);
    w_size   = MIN(ps_param_def->w_size, w_strLen);
    pc_dest  = str_copy(pc_dest, pc_end, (CHAR*)pb_src, w_size);/*lint !e713*/
    pc_dest  = str_copy(pc_dest, pc_end, "\"", -1);
  }
  else /* FIO_k_US_NMB || FIO_k_S_NMB */
  {
    /* assert enough space for printing the number */
    if(((pc_dest + (ps_param_def->w_size * 4)) >= pc_end) || /*lint !e946*/
        (pc_dest == NULL))
    {
      return NULL;
    }
    /* unsigned number */
    if( ps_param_def->s_type == FIO_k_US_NMB )
    {
      switch(ps_param_def->w_size)
      {
        case 1:
        {
          ll_nmb = (INT64)*(UINT8*)pb_src;
          break;
        }
        case 2:
        {
          ll_nmb = (INT64)*(UINT16*)pb_src;/*lint !e826*/
          break;
        }
        case 4:
        {
          ll_nmb = (INT64)*(UINT32*)pb_src;/*lint !e826*/
          break;
        }
        case 8:
        {
          ll_nmb = (INT64)(*(UINT64*)pb_src);/*lint !e826 !e740*/
          break;
        }
        default:
        {
          /* set error */
          PTP_SetError(k_FIO_ERR_ID,FIO_k_NUM_LEN,e_SEVC_NOTC);
          ll_nmb = (INT64)0;
          break;
        }
      }
    }
    /* FIO_k_S_NMB -> signed number */
    else
    {
      switch(ps_param_def->w_size)
      {
        case 1:
        {
          ll_nmb = *((INT8*)pb_src);
          break;
        }
        case 2:
        {
          ll_nmb = *((INT16*)pb_src);/*lint !e826*/
          break;
        }
        case 4:
        {
          ll_nmb = *((INT32*)pb_src);/*lint !e826*/
          break;
        }
        case 8:
        {
          ll_nmb = *((INT64*)pb_src);/*lint !e826 !e740*/
          break;
        }
        default:
        {
          /* set error */
          PTP_SetError(k_FIO_ERR_ID,FIO_k_NUM_LEN,e_SEVC_NOTC);
          ll_nmb = (INT64)0;
          break;
        }
      }
    }
    /* convert number into string */
    pc_dest = ll_to_a(pc_dest,pc_end,ll_nmb);   
  }
  return pc_dest;
}

/***********************************************************************
**  
** Function    : dump
**  
** Description : Dumps the configuration file contents into an ascii file.
**               The dump function is recursive. The parameter level (w_lvl)
**               shows the actual recursion depth.
**  
** Parameters  : pc_pos  (IN) - buffer position to write 
**               pc_end  (IN) - buffer end postion 
**               ps_pDef (IN) - parameter definition file
**               w_offs  (IN) - offset of the parameter to dump
**               w_lvl   (IN) - recursion depth
**               
** Returnvalue : new write position (behind dumped part)
** 
** Remarks     : -
**  
***********************************************************************/
static CHAR* dump(CHAR *pc_pos, CHAR *pc_end,const FIO_t_ParamDef *ps_pDef, 
                  UINT16 w_offs, UINT16 w_lvl)
{
  UINT32 dw_i;
  /* write position OK ? */
  if( pc_pos == NULL )
  {
    return NULL;
  }
  /* dump parameters out of struct into ascii file */
  while(ps_pDef->pc_ident != NULL)
  {
    pc_pos = str_copy(pc_pos, pc_end, k_BLANKS, w_lvl*k_IDENT);/*lint !e713*/
    pc_pos = str_copy(pc_pos, pc_end, ps_pDef->pc_ident, -1);
    pc_pos = str_copy(pc_pos, pc_end, " = ", -1);
    /* dump numbers and strings */
    if((ps_pDef->s_type == FIO_k_US_NMB) || /* unsigned number */
       (ps_pDef->s_type == FIO_k_S_NMB) ||  /* signed number */
       (ps_pDef->s_type == FIO_k_STRING))   /* string */
    {
      /* one number or a list ? */
      if(ps_pDef->w_num == 1)
      {
        pc_pos = data_copy(pc_pos, pc_end, ps_pDef, w_offs);
      }
      else
      {

        pc_pos = str_copy(pc_pos, pc_end, "[ ", -1);
        for( dw_i=0 ; dw_i < ps_pDef->w_num ; dw_i++)
        {
          pc_pos = data_copy(pc_pos, pc_end, ps_pDef, 
                             (UINT16)(w_offs + (ps_pDef->w_size * dw_i)));
          if( dw_i != (ps_pDef->w_num-1))
          {
            pc_pos = str_copy(pc_pos, pc_end, ", ", -1);
          }
        }
        pc_pos = str_copy(pc_pos, pc_end, " ]", -1);
      }
      pc_pos = str_copy(pc_pos, pc_end, "\n", -1);
    }
    /* dump of structs -> recursive part */
    else if(ps_pDef->s_type == FIO_k_STRUCT)
    {
      if(ps_pDef->w_num == 0)
      {
        pc_pos = str_copy(pc_pos, pc_end, "\n", -1);
      }
      else if(ps_pDef->w_num == 1)
      {
        pc_pos = str_copy(pc_pos, pc_end, "{\n", -1);
        pc_pos = dump(pc_pos, pc_end, 
                      (FIO_t_ParamDef*)ps_pDef->pv_ref, 
                      w_offs, 
                      w_lvl+1);
        pc_pos = str_copy(pc_pos, pc_end, k_BLANKS, w_lvl*k_IDENT);/*lint !e713*/
        pc_pos = str_copy(pc_pos, pc_end, "}\n", -1);
      }
      else
      {
        pc_pos = str_copy(pc_pos, pc_end, "[\n", -1);
        w_lvl++;
        for( dw_i=0 ; dw_i < ps_pDef->w_num ; dw_i++)
        {
          pc_pos = str_copy(pc_pos, pc_end, k_BLANKS, w_lvl*k_IDENT);/*lint !e713*/
          pc_pos = str_copy(pc_pos, pc_end, "{\n", -1);
          pc_pos = dump(pc_pos, pc_end, (FIO_t_ParamDef*)ps_pDef->pv_ref, 
                        (UINT16)(w_offs + (ps_pDef->w_size * dw_i)), 
                        (UINT16)(w_lvl+1));
          pc_pos = str_copy(pc_pos, pc_end, k_BLANKS, w_lvl*k_IDENT);/*lint !e713*/
          pc_pos = str_copy(pc_pos, pc_end, "}", -1);
          if( dw_i != (ps_pDef->w_num-1))
          {
            pc_pos = str_copy(pc_pos, pc_end, ",\n", -1);
          }
        }
        w_lvl--;
        pc_pos = str_copy(pc_pos, pc_end, "\n", -1);
        pc_pos = str_copy(pc_pos, pc_end, k_BLANKS, w_lvl*k_IDENT);/*lint !e713*/
        pc_pos = str_copy(pc_pos, pc_end, "]\n", -1);
      }
    }
    ps_pDef++;
  }
  return pc_pos;
}

/***********************************************************************
**  
** Function    : ll_to_a
**  
** Description : Converts a INT64 type to a CHAR string.
**               the length of the string should be at least
**               21 characters long for the maximum number size
**               plus negative sign.
**  
** Parameters  : pc_dest (IN) - buffer position to write 
**               pc_end  (IN) - buffer end postion 
**               ll_val  (IN) - value to convert
**               
** Returnvalue : new buffer write position (behind written number)
** 
** Remarks     : -
**  
***********************************************************************/
static CHAR* ll_to_a(CHAR *pc_dest, const CHAR *pc_end,INT64 ll_val)
{
  UINT32  dw_i;
  BOOLEAN o_isNeg = FALSE;
  INT64   ll_digit;
  INT64   ll_rest;
  INT32   l_tempLen;
  CHAR    ac_temp[k_MAX_LL_STR+1]={'\0'};
  INT32   l_len;

  /* convert in positive number and remember sign */
  if( ll_val < (INT64)0LL )
  {
    o_isNeg = TRUE;    
  }
  /* convert digit after digit */
  dw_i = k_MAX_LL_STR;
  /* initialize */
  ll_rest = ll_val;
  do
  {
    /* get least digit */
    ll_digit = PTP_ABS(INT64,(ll_rest % k_BASE_10));
    ll_rest  = PTP_ABS(INT64,(ll_rest / k_BASE_10));
    /* place it in string */
    ac_temp[dw_i] = ((CHAR)ll_digit) + k_DGT_ZERO;
    dw_i--;
  }while( ll_rest > 0);
  /* append negative sign if necessary */
  if( o_isNeg == TRUE )
  { 
    ac_temp[dw_i] = '-';
    dw_i--;
  }
  /* get length of string */
  l_tempLen = (INT32)(k_MAX_LL_STR - dw_i);
  /* get rest of file space */
  l_len = (INT32)(pc_end - pc_dest);/*lint !e946*/
  /* enough space ? */
  if( l_tempLen > l_len )
  {
    /* copy least significant digits */
    pc_dest = str_copy(pc_dest,pc_end,&ac_temp[(l_tempLen-l_len)+1],l_len-1);
  }
  else
  {
    /* copy temp value into return string */
    pc_dest = str_copy(pc_dest,pc_end,&ac_temp[dw_i+1],l_tempLen);
  }  
  return pc_dest;  
}





