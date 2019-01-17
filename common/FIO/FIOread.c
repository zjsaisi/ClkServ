/*************************************************************************
**    (C) 2008 - 2009 IXXAT Automation GmbH, all rights reserved
**************************************************************************
**
**       File: FIOread.c
**    Summary: config file scanner/parser
**    Version: 1.01.01
**    Version: 1.01.01
**
**************************************************************************
**************************************************************************
**
**  Functions: FIO_ReadParam
**
**             getChar
**             ungetChar
**             getvalue
**             const_lookup
**             scanner_init
**             scanner
**             copy
**             match
**             skip_chunk
**             var_lookup
**             assign_block
**             assign_list
**             assignment
**             expression
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
**    static constants, types, macros, variables, function prototypes
*************************************************************************/

#define k_BSIZE   256               /* max token buffer size */
#define k_NONE    0xffffffff
#define k_EOS     '\0'

/* token types */
#define k_NUM     256     /* number */
#define k_ID      257     /* Identifier (variable)*/
#define k_STR     258     /* string */
#define k_ERR     259     /* error */
#define k_DONE    999     /* end of file */


#define is_hex(c)             ((((c) >= 'a') && ((c <= 'f'))) || \
                                (((c) >= 'A') && ((c <= 'F'))))
#define is_special_char(c)    (((c) == '_') || ((c) == '.'))
#define is_digit(c)           ((((c) >= '0') && ((c) <= '9')))
#define is_alpha(c)           ((((c) >= 'A') && ((c) <= 'Z')) || \
                               (((c) >= 'a') && ((c) <= 'z')))
#define is_alnum(c)           (is_alpha(c) || is_digit(c))

/************************************************************************/
/** t_ScanParam : 
            Scanner parameters structure used to scan a file
*/
typedef struct
{
  UINT16         w_line_no;          /* line number of the config file */
  UINT16         w_lookahead;        /* next token */
  INT64          ll_tok_val;          /* numeric value of the current token */
  CHAR           ac_tok_buf[k_BSIZE];/* identifier/string of the current token */
  CHAR           *pc_stream;         /* input data stream */
  FIO_t_ConstDef *ps_const;          /* pointer to the constant data definition */
}t_ScanParam;

/*************************************************************************
**    static function-prototypes
*************************************************************************/
/* Scanner */
static CHAR getChar(t_ScanParam *ps_scan);
static void ungetChar(t_ScanParam *ps_scan);
static INT64 getvalue(t_ScanParam *ps_scan);
static FIO_t_ConstDef *const_lookup(const CHAR *pc_ident, FIO_t_ConstDef *ps_const);
static void scanner_init(t_ScanParam *ps_scan, CHAR *s, FIO_t_ConstDef *ps_const);
static void scanner(t_ScanParam *ps_scan);
/* Parser */
static BOOLEAN copy(void *pv_dest, INT64 ll_data, UINT16 w_size);
static BOOLEAN match(UINT16 w_tok, t_ScanParam *ps_scan);
static void skip_chunk(UINT16 w_stop_token, t_ScanParam *ps_scan);
static FIO_t_ParamDef* var_lookup(const CHAR *pc_ident, FIO_t_ParamDef *ps_def);
static BOOLEAN assign_block(t_ScanParam *ps_scan, FIO_t_ParamDef *p_def,
                            UINT16 w_offs);
static BOOLEAN assign_list(t_ScanParam *ps_scan, FIO_t_ParamDef *p_def,
                           UINT16 w_offs);
static BOOLEAN assignment(t_ScanParam *ps_scan, FIO_t_ParamDef *p_def,
                          UINT16 w_offs);
static BOOLEAN expression(t_ScanParam *ps_scan, FIO_t_ParamDef *p_def,
                          UINT16 w_offs);

/*************************************************************************
**    global functions
*************************************************************************/
/*************************************************************************
**
** Function    : FIO_ReadParam
**
** Description : Read config file and copy data to the via 'p_param_def'
**               specified location.
**
** Parameters  : pc_src        (IN): pointer to the input ASCII stream of
**                                  the config file.
**               ps_param_def  (IN): Pointer to the definition table with
**                                  all config parameter names and attributes.
**               ps_const_def  (IN): Optional additional table with constant
**                                  definitions. (NULL if not used)
**
** Returnvalue : number of occured errors
**
*************************************************************************/
UINT32 FIO_ReadParam(CHAR *pc_src, FIO_t_ParamDef *ps_param_def,
                     FIO_t_ConstDef *ps_const_def)
{
  t_ScanParam s_scan;
  BOOLEAN     o_res = TRUE;
  UINT32      dw_err = 0;
  /* initialize the scanner */
  scanner_init(&s_scan, pc_src, ps_const_def);
  scanner(&s_scan);
  while(s_scan.w_lookahead != k_DONE)
  {
    o_res = assignment(&s_scan, ps_param_def, 0);
    if(o_res == FALSE)
    {
        dw_err++;
    }
  }
  return dw_err;
}

/*************************************************************************
**    static functions
*************************************************************************/

/*************************************************************************
**    Scanner
*************************************************************************/

/*************************************************************************
**
** Function    : getChar
**
** Description : Read next character from the input data stream (config file).
**
** Parameters  : ps_scan (IN): pointer to scanner struct
**
** Returnvalue : numeric value
**
*************************************************************************/
static CHAR getChar(t_ScanParam *ps_scan)
{
  if(*ps_scan->pc_stream)
  {
    return *ps_scan->pc_stream++;
  }
  return EOF;
}

/*************************************************************************
**
** Function    : ungetChar
**
** Description : Unread last read character from the input data stream
**               (config file).
**
** Parameters  : ps_scan (IN): pointer to scanner struct
**
** Returnvalue : -
**
*************************************************************************/
static void ungetChar(t_ScanParam *ps_scan)
{
  ps_scan->pc_stream--;
}

/*************************************************************************
**
** Function    : getvalue
**
** Description : convert given string to the data type INT64
**
** Parameters  : ps_scan  (IN): string with numeric value (dec, hex)
**
** Returnvalue : numeric value
**
*************************************************************************/
static INT64 getvalue(t_ScanParam *ps_scan)
{
  INT64 ll_n;
  INT64 ll_basis;
  INT64 ll_val;
  BOOLEAN        o_loop = TRUE;

  ll_val = 0;

  if((ps_scan->pc_stream[1] == 'x') || (ps_scan->pc_stream[1] == 'X'))
  {
    ll_basis = (INT64)16;
    ps_scan->pc_stream = ps_scan->pc_stream + 2;
  }
  else
  {
    ll_basis = (INT64)10;
  }

  while(o_loop == TRUE)
  {
    ll_n = 0;
    if((is_digit(*ps_scan->pc_stream)) || (is_hex(*ps_scan->pc_stream)))
    {
      if((*ps_scan->pc_stream >= 'A') && (*ps_scan->pc_stream <= 'F'))
      {
        ll_n = *ps_scan->pc_stream - 55; /*lint !e732 */
      }
      if((*ps_scan->pc_stream >= 'a') && (*ps_scan->pc_stream <= 'f'))
      {
        ll_n = *ps_scan->pc_stream - 87;/*lint !e732 */
      }
      if((*ps_scan->pc_stream >= '0') && (*ps_scan->pc_stream <= '9'))
      {
        ll_n = *ps_scan->pc_stream - 48;/*lint !e732 */
      }
      if( ll_n >= ll_basis)
      {
        o_loop = FALSE;
      }
      else
      {
        ll_val = (ll_val * ll_basis) + ll_n;
        ps_scan->pc_stream++;
      }
    }
    else
    {
      o_loop = FALSE;
    }
  }
  return ll_val;
}

/*************************************************************************
**
** Function    : const_lookup
**
** Description : Return pointer to the constant definition with the given
**               name.
**
** Parameters  : pc_ident (IN): constant name
**               ps_const (IN): pointer to the table with constant definitions
**
** Returnvalue : pointer to the found entry or NULL
**
*************************************************************************/
static FIO_t_ConstDef *const_lookup(const CHAR *pc_ident, FIO_t_ConstDef *ps_const)
{
  if(ps_const == NULL)
  {
    return NULL;
  }
  while(PTP_STRCMP(ps_const->pc_ident, pc_ident) != 0)
  {
    ps_const++;
    if(ps_const->pc_ident == NULL)
    {
      return NULL;
    }
  }
  return ps_const;
}

/*************************************************************************
**
** Function    : scanner_init
**
** Description : Init the scanner
**
** Parameters  : ps_scan (OUT): pointer to a struct for the scanner data.
**               pc_data  (IN): input ASCII data stream from the config file.
**               ps_const  (IN): pointer to a table with constant definitions.
**
** Returnvalue : -
**
*************************************************************************/
static void scanner_init(t_ScanParam *ps_scan, CHAR *pc_data, FIO_t_ConstDef *ps_const)
{
  ps_scan->pc_stream   = pc_data;
  ps_scan->ps_const    = ps_const;
  ps_scan->w_line_no   = 1;
  ps_scan->ll_tok_val  = (INT64)k_NONE;
  ps_scan->w_lookahead = (UINT16)pc_data[0];/*lint !e571*/
}

/*************************************************************************
**
** Function    : scanner
**
** Description : Read next token
**
** Parameters  : ps_scan (IN/OUT): pointer to a struct for the scanner data.
**
** Returnvalue : -
**
*************************************************************************/
static void scanner(t_ScanParam *ps_scan)
{
  INT16          i_tok;
  BOOLEAN        o_loop;
  UINT16         w_idx;
  FIO_t_ConstDef *ps_const;

  while(1) /*lint !e716*/
  {
    i_tok = getChar(ps_scan);
    /* file end */
    if((CHAR)i_tok == (CHAR)EOF)
    {
      ps_scan->ac_tok_buf[0] = k_EOS;
      ps_scan->w_lookahead =  k_DONE;
      ps_scan->ll_tok_val = (INT64)k_NONE;
      return;
    }
    /* white spaces */
    else if((i_tok == ' ') || (i_tok == '\t'))
    {
      ;  /* skip blanks */
    }
    /* new line */
    else if(i_tok == '\n')
    {
      ps_scan->w_line_no++;
    }
    else if(i_tok == '\r')
    {
      ; /* skip line feed */
    }
    /* comment line  */
    else if (i_tok == '#')               
    {
      o_loop = TRUE;
      while(o_loop == TRUE)
      {
        i_tok = getChar(ps_scan);
        if((i_tok == '\n') || (i_tok == EOF))
        {
          ps_scan->w_line_no++;
          o_loop = FALSE;
        }
      }
    }
    /* a numerical value */
    else if(is_digit(i_tok))
    {
      ungetChar(ps_scan);
      ps_scan->ll_tok_val   = getvalue(ps_scan);
      ps_scan->w_lookahead = k_NUM;
      return;
    }
    /* a string */
    else if(i_tok == '"')
    {
      w_idx = 0;
      i_tok = getChar(ps_scan);
      while((i_tok != '"') && (i_tok != EOF))
      {
        ps_scan->ac_tok_buf[w_idx] = (CHAR)i_tok;
        i_tok = getChar(ps_scan);
        w_idx++;
        if(w_idx >= k_BSIZE)
        {
          /* set error - invalid string lenght */
          PTP_SetError(k_FIO_ERR_ID,FIO_k_INV_STRLEN,e_SEVC_NOTC);
          ps_scan->ac_tok_buf[0] = k_EOS;
          ps_scan->w_lookahead = k_ERR;
          return;
        }
      }
      ps_scan->ac_tok_buf[w_idx] = k_EOS;
      ps_scan->w_lookahead = k_STR;
      return;
    }
    /* other tokens */
    else if(is_alpha(i_tok) || is_special_char(i_tok))
    {
      w_idx = 0;
      while(is_alnum(i_tok) || is_special_char(i_tok))
      {
        ps_scan->ac_tok_buf[w_idx] = (CHAR)i_tok;
        i_tok = getChar(ps_scan);
        w_idx++;
        if(w_idx >= k_BSIZE)
        {
          /* set error - invalid string lenght */
          PTP_SetError(k_FIO_ERR_ID,FIO_k_INV_IDLEN,e_SEVC_NOTC);
          ps_scan->ac_tok_buf[0] = k_EOS;
          ps_scan->w_lookahead = k_ERR;
          return;
        }
      }
      ps_scan->ac_tok_buf[w_idx] = k_EOS;
      if(i_tok != EOF)
      {
        ungetChar(ps_scan);
      }
      if((ps_const = const_lookup(ps_scan->ac_tok_buf, ps_scan->ps_const)) != NULL)
      {
        ps_scan->ac_tok_buf[0] = k_EOS;
        ps_scan->w_lookahead   = k_NUM;  
        ps_scan->ll_tok_val    = ps_const->ll_val;
        return;
      }
      ps_scan->w_lookahead = k_ID;
      return;
    }
    else
    {
      ps_scan->ac_tok_buf[0] = k_EOS;
      ps_scan->w_lookahead   = (UINT16)i_tok;
      ps_scan->ll_tok_val    = (INT64)k_NONE;
      return;
    }
  }
}


/*************************************************************************
**    Parser
*************************************************************************/

/*************************************************************************
**
** Function    : copy
**
** Description : Copy data from p_data to p_dest
**
** Parameters  : pv_dest (IN): destination data pointer
**               ll_data (IN): source data 
**               w_size  (IN): Number of bytes (1,2,4,8)
**
** Returnvalue : pointer to s
**
*************************************************************************/
static BOOLEAN copy(void *pv_dest, INT64 ll_data, UINT16 w_size)
{
  BOOLEAN o_res = TRUE;
  
  switch(w_size)
  {
    case 1:                    /* 8-bit value */
    {
      *(UINT8*)pv_dest = (UINT8)ll_data;
      break;
    }
    case 2:                     /* 16-bit value */
    {
      *(UINT16*)pv_dest = (UINT16)ll_data;
      break;
    }
    case 4:                    /* 32-bit value */
    {
      *(UINT32*)pv_dest = (UINT32)ll_data;
      break;
    }
    case 8:                     /* 64-bit value */
    {
      *(INT64*)pv_dest = ll_data;
      break;
    }
    default:                    /* undefined */
    {
      o_res = FALSE;
      break;
    }
  }
  return o_res;
}

/*************************************************************************
**
** Function    : match
**
** Description : Test lookahead token against the given value and read
**               next token.
**
** Parameters  : w_tok   (IN): expected token
**               ps_scan (IN): scanner data set
**
** Returnvalue : TRUE/FALSE
**
*************************************************************************/
static BOOLEAN match(UINT16 w_tok, t_ScanParam *ps_scan)
{
  if( ps_scan->w_lookahead != k_DONE )
  {
    if(w_tok != ps_scan->w_lookahead)
    {
      /* scan and check, if done */
      scanner(ps_scan);
      if( ps_scan->w_lookahead != k_DONE )
      {
        /* set error */
        PTP_SetError(k_FIO_ERR_ID,FIO_k_WRG_TYPE,e_SEVC_NOTC);
        return FALSE;
      }
      else
      {
        return TRUE;
      }
    }
    scanner(ps_scan);
  }
  return TRUE;
}

/*************************************************************************
**
** Function    : skip_chunk
**
** Description : Try to skip all token up to the next assignment.
**
** Parameters  : w_stop_token  (IN): expected last token
**               ps_scan       (IN): scanner data set
**
** Returnvalue : -
**
*************************************************************************/
static void skip_chunk(UINT16 w_stop_token, t_ScanParam *ps_scan)
{
  while((ps_scan->w_lookahead != w_stop_token) && 
        (ps_scan->w_lookahead != k_DONE))
  {
    if(ps_scan->w_lookahead == '[')
    {
      match('[', ps_scan);/*lint !e534*/
      skip_chunk(']', ps_scan);
    }
    else if(ps_scan->w_lookahead == '{')
    {
      match('{', ps_scan);/*lint !e534*/
      skip_chunk('}', ps_scan);
    }
    else
    {
      match(ps_scan->w_lookahead, ps_scan); /*lint !e534*/
    }
  }
  match(ps_scan->w_lookahead, ps_scan);/*lint !e534*/
}


/*************************************************************************
**
** Function    : var_lookup
**
** Description : Return pointer to the variable definition with the given
**               name.
**
** Parameters  : pc_ident (IN): variable name
**               ps_def   (IN): pointer to the table with var definitions
**
** Returnvalue : pointer to the found entry or NULL
**
*************************************************************************/
static FIO_t_ParamDef* var_lookup(const CHAR *pc_ident, FIO_t_ParamDef *ps_def)
{
  while(PTP_STRCMP(ps_def->pc_ident, pc_ident) != 0)
  {
    ps_def++;
    if(ps_def->pc_ident == NULL)
    {
      return NULL;
    }
  }
  return ps_def;
}

/*************************************************************************
**
** Function    : assign_block
**
** Description : Parser function with the following BNF::
**                 assign_block:
**                   = '{' { assignment } '}'
**
** Parameters  : ps_scan  (IN): scanner data set
**               ps_def   (IN): pointer to the table with var definitions
**               w_offs   (IN): offset in variable array
**
** Returnvalue : TRUE/FALSE
**
*************************************************************************/
static BOOLEAN assign_block(t_ScanParam *ps_scan, FIO_t_ParamDef *ps_def,
                            UINT16 w_offs)
{
  BOOLEAN o_res = TRUE;
      
  o_res &= match('{', ps_scan);
  while((ps_scan->w_lookahead != '}') && (ps_scan->w_lookahead != k_DONE))
  {
    o_res &= assignment(ps_scan, ps_def, w_offs);    
  }
  o_res &= match('}', ps_scan);
  return o_res;
}


/*************************************************************************
**
** Function    : assign_list
**
** Description : Parser function with the following BNF::
**                 assign_list:
**                   = '[' expression { ',' expression } ']'
**
** Parameters  : ps_scan  (IN): scanner data set
**               ps_def   (IN): pointer to the table with var definitions
**               w_offs   (IN): offset in variable array
**
** Returnvalue : TRUE/FALSE
**
*************************************************************************/
static BOOLEAN assign_list(t_ScanParam *ps_scan, FIO_t_ParamDef *ps_def,
                           UINT16 w_offs)
{
  UINT16  w_idx = 0;
  UINT16  w_temp;
  BOOLEAN o_res = TRUE;
  BOOLEAN o_loop = TRUE;
  
  o_res &= match('[', ps_scan);
  while(o_loop == TRUE)
  {
    w_temp = (UINT16)(w_offs + (w_idx * ps_def->w_size));  /* new offset */
    o_res &= expression(ps_scan, ps_def, w_temp);
    /* check, if list is ready or if scan is done */
    if((ps_scan->w_lookahead == k_DONE )||(ps_scan->w_lookahead == ']'))
    {
      /* stop loop */
      o_loop = FALSE;
    }
    else 
    {
      o_res &=  match(',', ps_scan);
      w_idx++;
      if(w_idx >= ps_def->w_num)
      {
        /* set error */
        PTP_SetError(k_FIO_ERR_ID,FIO_k_INV_NMB,e_SEVC_NOTC);
        o_res = FALSE;
        /* stop loop */
        o_loop = FALSE;
      }
    }
  }
  o_res &= match(']', ps_scan);
  if((w_idx + 1) != ps_def->w_num)
  {
    /* set error */
    PTP_SetError(k_FIO_ERR_ID,FIO_k_INV_NMB,e_SEVC_NOTC);
    o_res = FALSE;
  }
  return o_res;
}


/*************************************************************************
**
** Function    : assignment
**
** Description : Parser function with the following BNF::
**                 assignment:
**                   = ident [ '[' num ']' ] '=' expression
**
** Parameters  : ps_scan  (IN): scanner data set
**               ps_def   (IN): pointer to the table with var definitions
**               w_offs   (IN): offset in variable array
**
** Returnvalue : TRUE/FALSE
**
*************************************************************************/
static BOOLEAN assignment(t_ScanParam *ps_scan, FIO_t_ParamDef *ps_def,
                          UINT16 w_offs)
{
  FIO_t_ParamDef *p_pos =  NULL;
  BOOLEAN o_res = TRUE;
  UINT16 w_idx = 0;

  p_pos = var_lookup(ps_scan->ac_tok_buf, ps_def);
  if(p_pos == NULL)
  {
    /* set warning */
    PTP_SetError(k_FIO_ERR_ID,FIO_k_UNKWN_PARAM,e_SEVC_NOTC);
    skip_chunk(k_NUM, ps_scan);
    return FALSE;
  }
  o_res &= match(k_ID, ps_scan);
  if(ps_scan->w_lookahead == '[')
  {
    if(p_pos->w_num <= 1)
    {
      /* set error */
      PTP_SetError(k_FIO_ERR_ID,FIO_k_INV_IDX,e_SEVC_NOTC);
      match('[', ps_scan);/*lint !e534*/
      /* skip the rest of the expression up to the end of the list */
      skip_chunk(']', ps_scan);
      o_res = FALSE;
    }
    o_res &= match('[', ps_scan);
    w_idx = (UINT16)ps_scan->ll_tok_val;
    o_res &= match(k_NUM, ps_scan);
    o_res &= match(']', ps_scan);
  }
  else
  {
    w_idx = 0;
  }
  if(o_res)
  {
    o_res &= match('=', ps_scan);
    if(w_idx < p_pos->w_num)
    {
      w_offs = (UINT16)(w_offs + (p_pos->w_size * w_idx));
      o_res &= expression(ps_scan, p_pos, w_offs);
    }
    else
    {
      o_res = FALSE;
    }
  }
  return o_res;
}


/*************************************************************************
**
** Function    : expression
**
** Description : Parser function with the following BNF::
**                 expression:
**                   = num                  
**                   | string               
**                   | assign_list          
**                   | assign_block
**
** Parameters  : ps_scan  (IN): scanner data set
**               ps_def   (IN): pointer to the table with var definitions
**               w_offs   (IN): offset in variable array
**
** Returnvalue : TRUE/FALSE
**
*************************************************************************/
static BOOLEAN expression(t_ScanParam *ps_scan, FIO_t_ParamDef *ps_def,
                          UINT16 w_offs)
{
  BOOLEAN o_res = TRUE;
  
  if((ps_def->w_num > 1) && (ps_scan->w_lookahead == '['))
  {
    /* call list assignment function */
    o_res &= assign_list(ps_scan, ps_def, w_offs);
  }
  else if(ps_def->s_type == FIO_k_STRING)
  {
    PTP_BCOPY(((UINT8*)ps_def->pv_ref) + w_offs, ps_scan->ac_tok_buf,
             ps_def->w_size);
    o_res &= match(k_STR, ps_scan);
  }
  else if((ps_def->s_type == FIO_k_US_NMB)||(ps_def->s_type == FIO_k_S_NMB))
  {
    if(ps_scan->w_lookahead == '-')
    {
      match('-', ps_scan);/*lint !e534*/
      ps_scan->ll_tok_val = -ps_scan->ll_tok_val;
    }
    if(copy(((UINT8*)ps_def->pv_ref + w_offs), ps_scan->ll_tok_val,
              ps_def->w_size) != TRUE)
    {
      /* set error */
      PTP_SetError(k_FIO_ERR_ID,FIO_k_INV_PDEF,e_SEVC_NOTC);
    }
    o_res &= match(k_NUM, ps_scan);
  }
  else if(ps_def->s_type == FIO_k_STRUCT)
  {
    /* call block assignment function */
    o_res &= assign_block(ps_scan,(FIO_t_ParamDef*)ps_def->pv_ref, w_offs);
  }
  else
  {
    /* set error */
    PTP_SetError(k_FIO_ERR_ID,FIO_k_INV_PDEF,e_SEVC_NOTC);
  }
  return o_res;
}





