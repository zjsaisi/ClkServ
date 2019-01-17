/*************************************************************************
**    (C) 2008 - 2009 IXXAT Automation GmbH, all rights reserved
**************************************************************************
**
**       File: FIOint.h
**    Summary: config file scanner/parser
**    Version: 1.01.01
**    Version: 1.01.01
**
**************************************************************************
**************************************************************************
**
**  Functions: FIO_ReadParam
**             FIO_WriteParam
**
**   Compiler: Ansi-C
**    Remarks:
**
**************************************************************************
**    all rights reserved
*************************************************************************/

#ifndef __FIO_INT_H__
#define __FIO_INT_H__

#ifdef __cplusplus
extern "C"
{
#endif
/*************************************************************************
**    constants and macros
*************************************************************************/
/* error definitions */
#define FIO_k_UNKWN_PARAM (0u) /* unknown parameter in configuration file */
#define FIO_k_INV_STRLEN  (1u) /* invalid string length */
#define FIO_k_INV_IDLEN   (2u) /* invalid identifier length */
#define FIO_k_INV_IDX     (3u) /* invalid index */
#define FIO_k_NUM_LEN     (4u) /* invalid variable size of a numerical value */
#define FIO_k_INV_NMB     (5u) /* invalid number of values */
#define FIO_k_INV_PDEF    (6u) /* invalid parameter definition */
#define FIO_k_WRG_TYPE    (7u) /* other data type expected */
#define FIO_k_ERR_UCMTBL  (8u) /* Ascii scanner detected problem 
                                  in unicast master table */            
#define FIO_k_ERR_CFG     (9u) /* Ascii scanner detected problem 
                                  in configuration file */ 
#define FIO_k_ERR_FLT    (10u) /* Ascii scanner detected problem 
                                  in filter file */ 
#define FIO_k_ERR_CRCV   (11u) /* Ascii scanner detected problem 
                                  in clock recovery file */ 
#define FIO_k_OOR_ERR    (12u) /* Profile check - out of range error - 
                                  value will be defaulted */
/*************************************************************************
**    data types
*************************************************************************/

/************************************************************************/
/** FIO_e_Types : 
            Parameter definition types 
*/
typedef enum
{
  FIO_k_NIL     = 0, /* no type */
  FIO_k_US_NMB  = 1, /* a unsigned integer number */
  FIO_k_S_NMB   = 2, /* a signed integer number */
  FIO_k_STRING  = 3, /* a string */
  FIO_k_STRUCT  = 4  /* a structure */
}FIO_e_Types;

/************************************************************************/
/** FIO_t_ParamDef : 
            Scanner 
*/
typedef struct
{
    FIO_e_Types s_type;   /* variable type */
    const CHAR  *pc_ident;/* variable name */
    UINT16      w_size;   /* variable size */
    UINT16      w_num;    /* ARR: number of elements */
    void        *pv_ref;  /* pointer to a sub-struct or
                             to the variable destination */
}FIO_t_ParamDef;

/************************************************************************/
/** FIO_t_ConstDef : 
            Definition of constants
*/
typedef struct
{
  const CHAR *pc_ident;        /* constant identifier */
  INT64      ll_val;  /* constant value */
}FIO_t_ConstDef;

/*************************************************************************
**    global variables
*************************************************************************/

/*************************************************************************
**    function prototypes
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
                     FIO_t_ConstDef *ps_const_def);

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
                      const FIO_t_ParamDef *ps_param_def);


#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* __FIO_INT_H__ */
