/*************************************************************************
**    (C) 2008 - 2009 IXXAT Automation GmbH, all rights reserved
**************************************************************************
**
**       File: FIO.h 
**    Summary: This unit implements file access methods.
**             The stack needs to maintain settings in Flash.
**             Therefore this unit implements the read/write
**             functions.
**              
**    Version: 1.01.01
**
**************************************************************************
**************************************************************************
**             
**  Functions: FIO_InitCfgDflt
**             FIO_ReadConfig
**             FIO_WriteConfig
**             FIO_CheckCfg
**             FIO_ReadClkRcvr
**             FIO_WriteClkRcvr
**             FIO_ReadUcMstTbl
**             FIO_WriteUcMstTbl
**             FIO_ReadFltCfg
**             FIO_WriteFltCfg
**             FIO_GetErrStr
**            
**   Compiler: Ansi-C
**    Remarks:
**
**************************************************************************
**    all rights reserved
*************************************************************************/
#ifndef __FIO_H__
#define __FIO_H__

#ifdef __cplusplus
extern "C"
{
#endif
/*************************************************************************
**    constants and macros
*************************************************************************/

/* maximum string length for network address representation as string */
#define k_MAX_NETADDR_STRLEN (64u)
/*************************************************************************
**    data types
*************************************************************************/

/************************************************************************/
/** FIO_t_FileDesc : 
            File descriptor, contains file index and file name
*/
typedef struct
{
  UINT32     dw_fIdx;   /* file index */
  const CHAR *pc_fName; /* file name */
}FIO_t_FileDesc;

/*************************************************************************
**    global variables
*************************************************************************/

/*************************************************************************
**    function prototypes
*************************************************************************/
/***********************************************************************
**  
** Function    : FIO_InitCfgDflt
**  
** Description : Initializes the configuration
**               to be used and written into ROM
**  
** Parameters  : ps_cfgRom (IN) - pointer to config File
**               
** Returnvalue : -
** 
** Remarks     : -
**  
***********************************************************************/
void FIO_InitCfgDflt( PTP_t_cfgRom *ps_cfgRom);

/***********************************************************************
**  
** Function    : FIO_ReadConfig
**  
** Description : Reads the config file out of ROM.
**               The appropriate GOE function returns either a ASCII file
**               or a binary file.
**               With returning FALSE, this means, that the 
**               file could not be read or that parts of it 
**               could not be recovered from the file stored
**               in non-volatile memory. In this cases, the
**               returned file should be written to fix the 
**               problem.
**  
** Parameters  : ps_cfgFile (IN) - pointer to config File
**               
** Returnvalue : TRUE  - success
**               FALSE - error reading the file - returned configuration is
**                       default.
** 
** Remarks     : -
**  
***********************************************************************/
BOOLEAN FIO_ReadConfig( PTP_t_cfgRom *ps_cfgFile );

/***********************************************************************
**  
** Function    : FIO_WriteConfig
**  
** Description : Writes the configuration file into readable ascii format
**               and in binary format. 
**  
** Parameters  : ps_cfgRom (IN) - configuration file to write
**               
** Returnvalue : TRUE           - function succeeded
**               FALSE          - function failed
** 
** Remarks     : -
**  
***********************************************************************/
BOOLEAN FIO_WriteConfig(const PTP_t_cfgRom *ps_cfgRom);

/***********************************************************************
**  
** Function    : FIO_CheckCfg
**  
** Description : Checks the configuration parameter with the
**               regarding ranges stored in the configuration.
**               Values out of range will be set to the 
**               default values.
**  
** Parameters  : ps_cfgRom (IN/OUT) - config File
**               
** Returnvalue : -
** 
** Remarks     : -
**  
***********************************************************************/
void FIO_CheckCfg( PTP_t_cfgRom *ps_cfgRom);

/***********************************************************************
**  
** Function    : FIO_ReadClkRcvr
**  
** Description : Reads the clock recovery file. This file is used
**               to store the clock drift at stopping the clock
**               a flag, if this drift is to be used at restart.
**               The returned file is filled with default values.
**               With returning FALSE, this means, that the 
**               file could not be read or that parts of it 
**               could not be recovered from the file stored
**               in non-volatile memory. In this cases, the
**               returned file should be written to fix the 
**               problem.
**  
** Parameters  : ps_clkRcvr (OUT) - clock recovery file
**               
** Returnvalue : TRUE             - function succeeded
**               FALSE            - function failed
** 
** Remarks     : -
**  
***********************************************************************/
BOOLEAN FIO_ReadClkRcvr(PTP_t_cfgClkRcvr *ps_clkRcvr);

/***********************************************************************
**  
** Function    : FIO_WriteClkRcvr
**  
** Description : Writes the clock recovery file. This file is used
**               to store the clock drift at stopping the clock
**               a flag, if this drift is to be used at restart.
**  
** Parameters  : ps_clkRcvr (IN) - configuration file to write
**               
** Returnvalue : TRUE            - function succeeded
**               FALSE           - function failed
** 
** Remarks     : -
**  
***********************************************************************/
BOOLEAN FIO_WriteClkRcvr(const PTP_t_cfgClkRcvr *ps_clkRcvr);

/***********************************************************************
**  
** Function    : FIO_ReadUcMstTbl
**  
** Description : Reads the unicast master table in ascii or binary format.
**               The function GOE_ReadFile returns the format of the
**               returned unicast master table.
**               With returning FALSE, this means, that the 
**               file could not be read or that parts of it 
**               could not be recovered from the file stored
**               in non-volatile memory. In this cases, the
**               returned file should be written to fix the 
**               problem.
**  
** Parameters  : ps_ucMstTbl (OUT) - unicast master table to read
**               
** Returnvalue : TRUE              - function succeeded
**               FALSE             - function failed
** 
** Remarks     : -
**  
***********************************************************************/
BOOLEAN FIO_ReadUcMstTbl(PTP_t_PortAddrQryTbl *ps_ucMstTbl );

/***********************************************************************
**  
** Function    : FIO_WriteUcMstTbl
**  
** Description : Writes the unicast master table in ascii AND binary format.
**               The function GOE_WriteFile writes the data in one or both
**               formats into the non-volatile storage of the device.
**               The function GOE_ReadFile returns one of the two formats.
**  
** Parameters  : ps_ucMstTbl (IN) - the unicast table to write
**               
** Returnvalue : TRUE             - function succeeded
**               FALSE            - function failed
** 
** Remarks     : -
**  
***********************************************************************/
BOOLEAN FIO_WriteUcMstTbl(const PTP_t_PortAddrQryTbl *ps_ucMstTbl);

/*************************************************************************
**
** Function    : FIO_ReadFltCfg
**
** Description : This function reads the filter configuration data 
**               out of the non-volatile storage. 
**               The data contains:
**               - used filter
**               - filter parameters
**               With returning FALSE, this means, that the 
**               file could not be read or that parts of it 
**               could not be recovered from the file stored
**               in non-volatile memory. In this cases, the
**               returned file should be written to fix the 
**               problem.
**
** Parameters  : ps_cfgFlt (OUT) - pointer to filter configuration file to
**                                 read in data out of non-volatile storage
**
** Returnvalue : TRUE            - function succeeded
**               FALSE           - function failed - no data available
**               
** Remarks     : -
**
*************************************************************************/
BOOLEAN FIO_ReadFltCfg( PTP_t_fltCfg *ps_cfgFlt );

/***********************************************************************
**  
** Function    : FIO_WriteFltCfg
**  
** Description : This function writes the filter configuration data 
**               into the non-volatile storage. 
**               The data contains:
**               - used filter
**               - filter parameters
**  
** Parameters  : ps_fltCfg (IN) - filter configuration file
**               
** Returnvalue : TRUE           - function succeeded
**               FALSE          - function failed
** 
** Remarks     : -
**  
***********************************************************************/
BOOLEAN FIO_WriteFltCfg(const PTP_t_fltCfg *ps_fltCfg);

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
const CHAR* FIO_GetErrStr(UINT32 dw_errNmb);
#endif /* #ifdef ERR_STR */

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* __FIO_H__ */
