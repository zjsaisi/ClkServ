/*************************************************************************
**    (C) 2008 - 2009 IXXAT Automation GmbH, all rights reserved
**************************************************************************
**
**       File: FIO.c 
**    Summary: This unit implements file access methods.
**             The stack needs to maintain settings in Flash.
**             Therefore this unit implements the read/write
**             functions.
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
**             ReadUcMstBin
**             WriteUcMstBin
**             InitFltDflt
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
#include "GOE/GOE.h"
#include "NIF/NIF.h"
#include "PTP/PTP.h"
#include "FIO/FIOint.h"
#include "FIO/FIO.h"
#include "API/sc_system.h"
#include "CLK/clk_private.h"
#include "CLK/ptp.h"
#include "CLK/CLKflt.h"

/*************************************************************************
**    global variables
*************************************************************************/

/* the local configuration file */
static PTP_t_cfgRom s_cfg;

/* parameter definition for the default data set config file, part of configuration file */
FIO_t_ParamDef as_defDsCfg[] = {
 /* type,        ident,         size,              num,     ref */
{ FIO_k_US_NMB, "ClockClass",         sizeof(PTP_t_clkClassEnum)  ,  1, &s_cfg.s_defDs.e_clkClass },
{ FIO_k_US_NMB, "ClockScaledVariance",sizeof(UINT16)              ,  1, &s_cfg.s_defDs.w_scldVar },
{ FIO_k_US_NMB, "ClockPriority1",     sizeof(UINT8)               ,  1, &s_cfg.s_defDs.b_prio1 },
{ FIO_k_US_NMB, "ClockPriority2",     sizeof(UINT8)               ,  1, &s_cfg.s_defDs.b_prio2 },
{ FIO_k_US_NMB, "ClockDomain",        sizeof(UINT8)               ,  1, &s_cfg.s_defDs.b_domNmb },
{ FIO_k_US_NMB, "ClockIsSlaveOnly",   sizeof(BOOLEAN)             ,  1, &s_cfg.s_defDs.o_slaveOnly },
{ FIO_k_NIL   , NULL                , 0                           , 0, NULL }
};

/* parameter definition for the TC default data set config file, part of configuration file */
FIO_t_ParamDef as_tcDefDsCfg[] ={
 /* type,        ident,                size,                        num,     ref */
{ FIO_k_US_NMB, "TcClockPrimaryDomain",sizeof(UINT8)               ,  1, &s_cfg.s_tcDefDs.b_TcprimDom },
{ FIO_k_US_NMB, "TcClockDelMech",      sizeof(PTP_t_delmEnum)      ,  1, &s_cfg.s_tcDefDs.e_TcDelMech },
{ FIO_k_NIL   , NULL                  , 0                          , 0, NULL }
};

FIO_t_ParamDef as_profCfg[] = {
 /* type,        ident,         size,              num,     ref */
{ FIO_k_S_NMB ,"AnncIntvRange",   sizeof(INT8) , k_RNG_SZE, &s_cfg.s_prof.ac_annIntRng[0]},
{ FIO_k_US_NMB,"AnncRcptTmoRange",sizeof(UINT8), k_RNG_SZE, &s_cfg.s_prof.ab_annRcpTmo[0]},
{ FIO_k_S_NMB ,"SyncIntvRange",   sizeof(INT8) , k_RNG_SZE, &s_cfg.s_prof.ac_synIntRng[0]},
{ FIO_k_S_NMB ,"DelmIntvRange",   sizeof(INT8) , k_RNG_SZE, &s_cfg.s_prof.ac_delMIntRng[0]},
{ FIO_k_US_NMB,"DomainNmbRange",  sizeof(UINT8), k_RNG_SZE, &s_cfg.s_prof.ab_domn[0]},
{ FIO_k_US_NMB,"Priority1Range",  sizeof(UINT8), k_RNG_SZE, &s_cfg.s_prof.ab_prio1[0]},
{ FIO_k_US_NMB,"Priority2Range",  sizeof(UINT8), k_RNG_SZE, &s_cfg.s_prof.ab_prio2[0]},
{ FIO_k_NIL   , NULL           ,0              ,  0       , NULL }
};

/* parameter definition for the port config file, part of configuration file */
FIO_t_ParamDef as_portCfg[] = {
   /* type,        ident,      size,                   num,     ref */
{ FIO_k_US_NMB, "DelMech", sizeof(PTP_t_clkClassEnum),  1, &s_cfg.as_pDs[0].e_delMech },
{ FIO_k_S_NMB , "DelrIntv",sizeof(INT8)              ,  1, &s_cfg.as_pDs[0].c_dlrqIntv },
{ FIO_k_S_NMB , "AnncIntv",sizeof(INT8)              ,  1, &s_cfg.as_pDs[0].c_AnncIntv },
{ FIO_k_US_NMB, "AnncTmo", sizeof(UINT8)             ,  1, &s_cfg.as_pDs[0].b_anncRcptTmo },
{ FIO_k_S_NMB , "SynIntv", sizeof(INT8)              ,  1, &s_cfg.as_pDs[0].c_syncIntv },
{ FIO_k_S_NMB , "PdelIntv",sizeof(INT8)              ,  1, &s_cfg.as_pDs[0].c_pdelReqIntv },
{ FIO_k_NIL   , NULL      ,0                         ,  0, NULL }
};

/* parameter definition for the configuration file */
FIO_t_ParamDef as_cfgRom[] = {
/*     type,        ident,               size,                  num,  ref */
{ FIO_k_STRING, "UserDescription", sizeof(s_cfg.ac_userDesc) ,  1, s_cfg.ac_userDesc },
{ FIO_k_STRUCT, "DefaultDataSet",  sizeof(PTP_t_cfgDefDs)    ,  1, as_defDsCfg },
{ FIO_k_STRUCT, "TcDefaultDataSet",sizeof(PTP_t_cfgTcDefDs)  ,  1, as_tcDefDsCfg},
{ FIO_k_STRUCT, "ProfileRanges",   sizeof(PTP_t_ProfRng)     ,  1, as_profCfg },
{ FIO_k_STRUCT, "PortDataSet",     sizeof(PTP_t_cfgPort),k_NUM_IF, as_portCfg },            
{ FIO_k_NIL   , NULL         ,         0                     ,  0, NULL }
};

/* local filter configuration file */
static PTP_t_fltCfg s_flt;

/* parameter definition for the minimum filter */
FIO_t_ParamDef as_minFltPrm[] = {
  /* type,        ident,         size,           num,     ref */
{ FIO_k_US_NMB, "FilterLenght", sizeof(UINT32),  1, &s_flt.s_filtMin.dw_filtLen },
{ FIO_k_NIL   , NULL          ,  0            ,  0, NULL }
};

/* parameter definition for the estimator filter */
FIO_t_ParamDef as_estFltPrm[] = {
  /* type,        ident,         size,             num,     ref */
{ FIO_k_US_NMB, "HistoryLenght"  , sizeof(UINT8) , 1, &s_flt.s_filtEst.b_histLen },
{ FIO_k_US_NMB, "WhiteNoiseFloor", sizeof(UINT64), 1, &s_flt.s_filtEst.ddw_WhtNoise },
{ FIO_k_US_NMB, "SuccessiveCorr" , sizeof(UINT8) , 1, &s_flt.s_filtEst.b_amntFlt },
{ FIO_k_NIL   , NULL            , 0              , 0, NULL }
};


/* parameter definition for the filter configuration file */
FIO_t_ParamDef as_fltParam[] = {
/*      type,        ident,       size,                    num,  ref */
{ FIO_k_US_NMB, "UsedFilter"    ,sizeof(PTP_t_fltTypeEnum), 1, &s_flt.e_usedFilter },
{ FIO_k_STRUCT, "MinimumFilter" , sizeof(PTP_t_filtMin)   , 1, as_minFltPrm },
{ FIO_k_STRUCT, "EstimatorFilter",sizeof(PTP_t_filtEst)   , 1, as_estFltPrm },
{ FIO_k_NIL   , NULL             , 0                      , 0, NULL }
};

/* local clock recovery file */
static PTP_t_cfgClkRcvr s_clkRcvr;

/* parameter definition for clock recovery config file, part of configuration file */
FIO_t_ParamDef as_clkRcrv[] = {
 /* type,        ident,         size,              num,     ref */
{ FIO_k_S_NMB , "ClockDrift"       ,sizeof(INT64)  , 1, &s_clkRcvr.ll_drift_ppb },
{ FIO_k_US_NMB, "UseDriftAtStartup",sizeof(BOOLEAN), 1, &s_clkRcvr.o_drift_use },
{ FIO_k_S_NMB , "MeanE2EDelay"     ,sizeof(INT64)  , 1, &s_clkRcvr.ll_E2EmeanPDel },
{ FIO_k_S_NMB , "MeanP2PDelay",sizeof(INT64), k_NUM_IF,&s_clkRcvr.all_P2PmeanPDel[0]},
{ FIO_k_NIL   , NULL               ,0              , 0, NULL }
};

/* constants that may be used in the configuration files */
FIO_t_ConstDef as_const[] = {
  { "TRUE" , 1 },
  { "FALSE", 0 },
  { "E2E"  , 1 },
  { "P2P"  , 2 },
  { "IPV4" , 1 },
  { NULL   , 0}
};
/************************************************************************/
/** t_netaddr : 
            Network address representation for the ascii file
            parameter table
*/
typedef struct
{
  PTP_t_nwProtEnum e_netwProt; /* network protocol enumeration */
  UINT16           w_AddrLen;  /* length of the following 
                                  address array */
  CHAR             ac_addrStr[k_MAX_NETADDR_STRLEN];
}t_netaddr;

/************************************************************************/
/** t_ucTblAscii : 
            Unicast master table representation for the ascii file
            parameter table
*/
typedef struct
{
  INT8      c_logQryIntv;        /* log2 of query interval */
  UINT16    w_actTblSze;         /* actual table size */
  t_netaddr as_Addr[k_MAX_UC_MST_TBL_SZE]; 
}t_ucTblAscii;

/* local unicast master table (ascii) for r/w access to the ascii file in ROM */
static t_ucTblAscii s_lclUcMstTbl;

/* parameter definition unicast master entry in the unicast master table */
FIO_t_ParamDef as_ucMst[] = {
  /* type,        ident,         size,                      num,     ref */
{ FIO_k_US_NMB, "NetwProt",sizeof(PTP_t_nwProtEnum), 1 , &s_lclUcMstTbl.as_Addr[0].e_netwProt },
{ FIO_k_US_NMB, "NetwAddrLen",  sizeof(UINT16)         , 1 , &s_lclUcMstTbl.as_Addr[0].w_AddrLen },
{ FIO_k_STRING, "NetwAddr", k_MAX_NETADDR_STRLEN   , 1 , s_lclUcMstTbl.as_Addr[0].ac_addrStr },
{ FIO_k_NIL   , NULL            ,  0                      , 0, NULL }
};


/* parameter definition for the unicast master table (ascii representation) in ROM */
FIO_t_ParamDef as_ucMstTbl[] = {
/* type,        ident,                       size,    num,  ref */
{ FIO_k_S_NMB , "Log2QueryInterval",sizeof(INT8)      , 1, &s_lclUcMstTbl.c_logQryIntv },
{ FIO_k_US_NMB, "ActualTableSize"  ,sizeof(UINT16)    , 1, &s_lclUcMstTbl.w_actTblSze },
{ FIO_k_STRUCT, "UnicastMasters"   ,sizeof(t_netaddr) , k_MAX_UC_MST_TBL_SZE, as_ucMst },
{ FIO_k_NIL   , NULL               ,  0                , 0, NULL }
};

/*************************************************************************
**    static constants, types, macros, variables
*************************************************************************/

/* ROM file names  */
#define k_FLNME_CFG             ("/active/config/Config")
#define k_FLNME_UC_MST_TBL      ("/active/config/UcMstTbl")
#define k_FLNME_FLTCFG          ("/active/config/FilterCfg")
#define k_FLNME_CRCVR           ("/active/config/ClockRcvr")
/* ROM file indices */
#define k_FLIDX_CFG             (0u) /* file index for configuration file */
#define k_FLIDX_UC_MST_TBL      (1u) /* file index for unicast master table */
#define k_FLIDX_FLT             (2u) /* file index for filter settings */
#define k_FLIDX_CRCVR           (3u) /* file index for clock recovery file */

/* max. files for I/O in Rom */
#define k_CFG_FILE_SZE_ASCII  ( 1024u + ( 192u * k_NUM_IF ))
#define k_FLT_FILE_SZE        ( 256u )
#define k_UCM_FILE_SZE        ( 100u + ( 100u * k_MAX_UC_MST_TBL_SZE ))
#define k_CRCVR_FILE_SZE      ( 256u )

/*************************************************************************
**    static function-prototypes
*************************************************************************/
#if 0
static BOOLEAN ReadUcMstBin( PTP_t_PortAddrQryTbl *ps_ucMstTbl,
                             const UINT8 *ab_data,
                             UINT32 dw_fSze);
static INT32 WriteUcMstBin(CHAR   *pc_binFile,
                           UINT32 dw_fileSize,
                           const PTP_t_PortAddrQryTbl *ps_ucMstTbl);
static void InitFltDflt( PTP_t_fltCfg *ps_fltCfg);
#endif // #if 0
/*************************************************************************
**    global functions
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
void FIO_InitCfgDflt( PTP_t_cfgRom *ps_cfgRom)
{
  UINT16 w_ifIdx;
  UINT32 dw_userDescSze;
  
  /* init type PTP_text */
  /* get lenght of string in target.h */
  dw_userDescSze = (UINT8)PTP_BLEN(k_CLKDES_USER);
  if( dw_userDescSze > k_MAX_USRDESC_SZE )/*lint !e774*/
  {
    dw_userDescSze = k_MAX_USRDESC_SZE;
  }
  /* copy user description string */
  PTP_STRNCPY(ps_cfgRom->ac_userDesc,
              k_CLKDES_USER,
              dw_userDescSze);
  ps_cfgRom->ac_userDesc[dw_userDescSze] = '\0';
  /* initialize it with default values */
  /* values for default data set */
  ps_cfgRom->s_defDs.e_clkClass      = k_CLK_CLASS;                
  /* estimate scaled log variance on basis of timestamp granularity */
  ps_cfgRom->s_defDs.w_scldVar       = PTP_scaleVar((UINT64)k_TS_GRANULARITY_NS * 
                                            (UINT64)k_TS_GRANULARITY_NS);
  ps_cfgRom->s_defDs.b_prio1         = k_PRIO1_DEF;
  ps_cfgRom->s_defDs.b_prio2         = k_PRIO2_DEF;
  ps_cfgRom->s_defDs.b_domNmb        = k_DOMN_DEF;
  ps_cfgRom->s_defDs.o_slaveOnly     = k_IMPL_SLAVE_ONLY;
  /* values for TC default data set */
  ps_cfgRom->s_tcDefDs.b_TcprimDom     = k_DOMN_DEF;
  if( k_CLK_DEL_E2E == TRUE ) /*lint !e506 !e774 !e831*/
  {
    ps_cfgRom->s_tcDefDs.e_TcDelMech = e_DELMECH_E2E;
  }
  else if( k_CLK_DEL_P2P == TRUE ) /*lint !e506 !e774 !e831*/
  {
    ps_cfgRom->s_tcDefDs.e_TcDelMech = e_DELMECH_P2P;
  }
  else
  {
    ps_cfgRom->s_tcDefDs.e_TcDelMech = e_DELMECH_DISBl;
  }
  /* profile ranges */
  /* announce interval range */
  ps_cfgRom->s_prof.ac_annIntRng[k_RNG_MIN] = k_ANNC_INTV_MIN;
  ps_cfgRom->s_prof.ac_annIntRng[k_RNG_DEF] = k_ANNC_INTV_DEF;
  ps_cfgRom->s_prof.ac_annIntRng[k_RNG_MAX] = k_ANNC_INTV_MAX;
  /* announce receipt timeout range */
  ps_cfgRom->s_prof.ab_annRcpTmo[k_RNG_MIN] = k_ANNC_RTMO_MIN;
  ps_cfgRom->s_prof.ab_annRcpTmo[k_RNG_DEF] = k_ANNC_RTMO_DEF;
  ps_cfgRom->s_prof.ab_annRcpTmo[k_RNG_MAX] = k_ANNC_RTMO_MAX;
  /* sync interval range */
  ps_cfgRom->s_prof.ac_synIntRng[k_RNG_MIN] = k_SYN_INTV_MIN;
  ps_cfgRom->s_prof.ac_synIntRng[k_RNG_DEF] = k_SYN_INTV_DEF;
  ps_cfgRom->s_prof.ac_synIntRng[k_RNG_MAX] = k_SYN_INTV_MAX;
  /* delay mechanism interval range (E2E AND P2P) */
  ps_cfgRom->s_prof.ac_delMIntRng[k_RNG_MIN] = k_DELM_INTV_MIN;
  ps_cfgRom->s_prof.ac_delMIntRng[k_RNG_DEF] = k_DELM_INTV_DEF;
  ps_cfgRom->s_prof.ac_delMIntRng[k_RNG_MAX] = k_DELM_INTV_MAX;
  /* domain number range */
  ps_cfgRom->s_prof.ab_domn[k_RNG_MIN] = k_DOMN_MIN;
  ps_cfgRom->s_prof.ab_domn[k_RNG_DEF] = k_DOMN_DEF;
  ps_cfgRom->s_prof.ab_domn[k_RNG_MAX] = k_DOMN_MAX;
  /* priority 1 range */
  ps_cfgRom->s_prof.ab_prio1[k_RNG_MIN] = k_PRIO1_MIN;
  ps_cfgRom->s_prof.ab_prio1[k_RNG_DEF] = k_PRIO1_DEF;
  ps_cfgRom->s_prof.ab_prio1[k_RNG_MAX] = k_PRIO1_MAX;
  /* priority 2 range */
  ps_cfgRom->s_prof.ab_prio2[k_RNG_MIN] = k_PRIO2_MIN;
  ps_cfgRom->s_prof.ab_prio2[k_RNG_DEF] = k_PRIO2_DEF;
  ps_cfgRom->s_prof.ab_prio2[k_RNG_MAX] = k_PRIO2_MAX;

  /* values for port data sets */
  for( w_ifIdx = 0 ; w_ifIdx < k_NUM_IF ; w_ifIdx++ )
  {
    if( k_CLK_DEL_E2E == TRUE ) /*lint !e506 !e774 !e831*/
    {
      ps_cfgRom->as_pDs[w_ifIdx].e_delMech    = e_DELMECH_E2E;
    }
    else if( k_CLK_DEL_P2P == TRUE ) /*lint !e506 !e774 !e831*/
    {
      ps_cfgRom->as_pDs[w_ifIdx].e_delMech    = e_DELMECH_P2P;
    }
    else
    {
      ps_cfgRom->as_pDs[w_ifIdx].e_delMech    = e_DELMECH_DISBl;
    }
    ps_cfgRom->as_pDs[w_ifIdx].c_dlrqIntv     = k_DELM_INTV_DEF;
    ps_cfgRom->as_pDs[w_ifIdx].c_AnncIntv     = k_ANNC_INTV_DEF;
    ps_cfgRom->as_pDs[w_ifIdx].b_anncRcptTmo  = k_ANNC_RTMO_DEF;
    ps_cfgRom->as_pDs[w_ifIdx].c_syncIntv     = k_SYN_INTV_DEF;
    ps_cfgRom->as_pDs[w_ifIdx].c_pdelReqIntv  = k_DELM_INTV_DEF;
  }
}

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
BOOLEAN FIO_ReadConfig( PTP_t_cfgRom *ps_cfgFile )
{
#if 1
  return SC_GetConfig(ps_cfgFile);
#else
  UINT8   ab_data[k_CFG_FILE_SZE_ASCII]={'\0'};
  UINT32  dw_bufSze = k_CFG_FILE_SZE_ASCII;
  UINT8   b_fileMode;
  BOOLEAN o_ret = FALSE;
  FIO_t_FileDesc s_cfgDesc = {k_FLIDX_CFG,k_FLNME_CFG};
  
  /* preinit the local file to default values */
  FIO_InitCfgDflt(&s_cfg);
  /* try to read the configuration file out of ROM */
  if( GOE_ReadFile(s_cfgDesc.dw_fIdx,
                   s_cfgDesc.pc_fName,
                   ab_data,
                   &dw_bufSze,
                   &b_fileMode) == FALSE )
  {
    /* return error */
    o_ret = FALSE;
  }
  else
  {
    /* interprete data */
    if( b_fileMode == k_FILE_ASCII )
    {
      /* ensure, input data are terminated */
      ab_data[dw_bufSze] = '\0';
      /* read variables into config file */
      if( FIO_ReadParam((CHAR*)ab_data,as_cfgRom,as_const) > 0 )
      {
        /* set notification */
        PTP_SetError(k_FIO_ERR_ID,FIO_k_ERR_CFG,e_SEVC_NOTC);
        /* return FALSE - new file will be written */
        o_ret = FALSE;
      }
      else
      {
        o_ret = TRUE;
      }
    }
    else
    {
      /* deserialize binary file */
      if( dw_bufSze != sizeof(PTP_t_cfgRom))
      {
        /* return FALSE - new file will be written */
        o_ret = FALSE;
      }
      else
      {
        /* copy data into local struct */
        PTP_BCOPY(&s_cfg,ab_data,dw_bufSze);
        o_ret = TRUE;
      }
    }
  }  
  /* copy resulting config file */
  *ps_cfgFile = s_cfg;
  return o_ret;
#endif
}

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
BOOLEAN FIO_WriteConfig(const PTP_t_cfgRom *ps_cfgRom)
{
#if 1
  return TRUE;
#else
  UINT32  dw_fSizeAsc;
  CHAR    ac_cfgFileAsc[k_CFG_FILE_SZE_ASCII]={'\0'};
  FIO_t_FileDesc s_cfgDesc = {k_FLIDX_CFG,k_FLNME_CFG};

  /* copy configuration to local file to write */
  s_cfg = *ps_cfgRom;
  /* write the data into the file according to the parameter definitions */
  dw_fSizeAsc = FIO_WriteParam(ac_cfgFileAsc,k_CFG_FILE_SZE_ASCII,as_cfgRom);
  /* write the files */
  return GOE_WriteFile(s_cfgDesc.dw_fIdx,
                       s_cfgDesc.pc_fName,
                       ac_cfgFileAsc,
                       dw_fSizeAsc,
                       (UINT8*)ps_cfgRom, /*lint !e740*/
                       sizeof(PTP_t_cfgRom));
#endif
}

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
void FIO_CheckCfg( PTP_t_cfgRom *ps_cfgRom)
{
  UINT16 w_ifIdx;
  PTP_t_cfgPort       *ps_cfgPort;
  PTP_t_cfgDefDs      *ps_cfgDefDs;
  const PTP_t_ProfRng *ps_prof;

  /* get profile ranges */
  ps_prof = &ps_cfgRom->s_prof;
  /* port data set */
  for( w_ifIdx = 0 ; w_ifIdx < k_NUM_IF ; w_ifIdx++ )
  {
    /* get pointer to port configurarion */
    ps_cfgPort = &ps_cfgRom->as_pDs[w_ifIdx];
    /* announce interval */
    if( PTP_CHK_RNG(INT8,
                    ps_cfgPort->c_AnncIntv,
                    ps_prof->ac_annIntRng[k_RNG_MIN],
                    ps_prof->ac_annIntRng[k_RNG_MAX]) 
                    == FALSE )/*lint !e731*/
    {
      /* set to default value */
      ps_cfgPort->c_AnncIntv = ps_prof->ac_annIntRng[k_RNG_DEF];
      /* set error */
      PTP_SetError(k_FIO_ERR_ID,FIO_k_OOR_ERR,e_SEVC_NOTC);
    }
    /* announce receipt timeout  */
    if( PTP_CHK_RNG(UINT8,
                    ps_cfgPort->b_anncRcptTmo,
                    ps_prof->ab_annRcpTmo[k_RNG_MIN],
                    ps_prof->ab_annRcpTmo[k_RNG_MAX]) 
                    == FALSE )/*lint !e731*/
    {
      /* set to default value */
      ps_cfgPort->b_anncRcptTmo = ps_prof->ab_annRcpTmo[k_RNG_DEF];
      /* set error */
      PTP_SetError(k_FIO_ERR_ID,FIO_k_OOR_ERR,e_SEVC_NOTC);
    }
    /* sync interval */
    if( PTP_CHK_RNG(INT8,
                    ps_cfgPort->c_syncIntv,
                    ps_prof->ac_synIntRng[k_RNG_MIN],
                    ps_prof->ac_synIntRng[k_RNG_MAX]) 
                    == FALSE )/*lint !e731*/
    {
      /* set to default value */
      ps_cfgPort->c_syncIntv = ps_prof->ac_synIntRng[k_RNG_DEF];
      /* set error */
      PTP_SetError(k_FIO_ERR_ID,FIO_k_OOR_ERR,e_SEVC_NOTC);
    }
    /* delay mechanism (E2E or P2P) interval */
    if( PTP_CHK_RNG(INT8,
                    ps_cfgPort->c_dlrqIntv,
                    ps_prof->ac_delMIntRng[k_RNG_MIN],
                    ps_prof->ac_delMIntRng[k_RNG_MAX]) 
                    == FALSE )/*lint !e731*/
    {
      /* set to default value */
      ps_cfgPort->c_dlrqIntv = ps_prof->ac_delMIntRng[k_RNG_DEF];
      /* set error */
      PTP_SetError(k_FIO_ERR_ID,FIO_k_OOR_ERR,e_SEVC_NOTC);
    }
    ps_cfgPort->c_pdelReqIntv = ps_cfgPort->c_dlrqIntv;
  }
  /* default data set */
  ps_cfgDefDs = &ps_cfgRom->s_defDs;
  /* domain number */
  if( PTP_CHK_RNG(UINT8,
                  ps_cfgDefDs->b_domNmb,
                  ps_prof->ab_domn[k_RNG_MIN],
                  ps_prof->ab_domn[k_RNG_MAX]) 
                  == FALSE )/*lint !e731*/
  {
    /* set to default value */
    ps_cfgDefDs->b_domNmb = ps_prof->ab_domn[k_RNG_DEF];
    /* set error */
    PTP_SetError(k_FIO_ERR_ID,FIO_k_OOR_ERR,e_SEVC_NOTC);
  }
  /* priority 1 */
  if( PTP_CHK_RNG(UINT8,
                  ps_cfgDefDs->b_prio1,
                  ps_prof->ab_prio1[k_RNG_MIN],
                  ps_prof->ab_prio1[k_RNG_MAX]) 
                  == FALSE )/*lint !e731*/
  {
    /* set to default value */
    ps_cfgDefDs->b_prio1 = ps_prof->ab_prio1[k_RNG_DEF];
    /* set error */
    PTP_SetError(k_FIO_ERR_ID,FIO_k_OOR_ERR,e_SEVC_NOTC);
  }
  /* priority 2 */
  if( PTP_CHK_RNG(UINT8,
                  ps_cfgDefDs->b_prio2,
                  ps_prof->ab_prio2[k_RNG_MIN],
                  ps_prof->ab_prio2[k_RNG_MAX]) 
                  == FALSE )/*lint !e731*/
  {
    /* set to default value */
    ps_cfgDefDs->b_prio2 = ps_prof->ab_prio2[k_RNG_DEF];
    /* set error */
    PTP_SetError(k_FIO_ERR_ID,FIO_k_OOR_ERR,e_SEVC_NOTC);
  }
}

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
BOOLEAN FIO_ReadClkRcvr(PTP_t_cfgClkRcvr *ps_clkRcvr)
{
#if 1
  return SC_GetClkRcvr(ps_clkRcvr);
#else
  UINT8   ab_data[k_CRCVR_FILE_SZE]={'\0'};
  UINT32  dw_bufSze = k_CRCVR_FILE_SZE;
  UINT8   b_fileMode;
  BOOLEAN o_ret = FALSE;
  UINT16  w_ifIdx;
  FIO_t_FileDesc s_cRcvrDesc = {k_FLIDX_CRCVR,k_FLNME_CRCVR};
  
  /* preinit the local file to default values */
  s_clkRcvr.ll_drift_ppb      = 0;
  s_clkRcvr.o_drift_use       = k_USE_STRD_DRIFT; 
  s_clkRcvr.ll_E2EmeanPDel = 0;
  for( w_ifIdx = 0 ; w_ifIdx < k_NUM_IF ; w_ifIdx++ )
  {
    s_clkRcvr.all_P2PmeanPDel[w_ifIdx] = 0;
  }
  /* try to read the configuration file out of ROM */
  if( GOE_ReadFile(s_cRcvrDesc.dw_fIdx,
                   s_cRcvrDesc.pc_fName,
                   ab_data,
                   &dw_bufSze,
                   &b_fileMode) == FALSE )
  {
    /* return error */
    o_ret = FALSE;
  }
  else
  {
    /* interprete data */
    if( b_fileMode == k_FILE_ASCII )
    {
      /* ensure, input data are terminated */
      ab_data[dw_bufSze] = '\0';
      /* read variables into config file */
      if( FIO_ReadParam((CHAR*)ab_data,as_clkRcrv,as_const) > 0 )
      {
        /* set notification */
        PTP_SetError(k_FIO_ERR_ID,FIO_k_ERR_CRCV,e_SEVC_NOTC);
        /* return FALSE - new file will be written */
        o_ret = FALSE;
      }
      else
      {
        o_ret = TRUE;
      }
    }
    else
    {
      /* deserialize binary file */
      if( dw_bufSze != sizeof(PTP_t_cfgClkRcvr))
      {
        /* return FALSE - new file will be written */
        o_ret = FALSE;
      }
      else
      {
        /* copy data into local struct */
        PTP_BCOPY(&s_clkRcvr,ab_data,dw_bufSze);
        o_ret = TRUE;
      }
    }
  }
  /* copy resulting config file */
  *ps_clkRcvr = s_clkRcvr;
  return o_ret;
#endif
}

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
BOOLEAN FIO_WriteClkRcvr(const PTP_t_cfgClkRcvr *ps_clkRcvr)
{
#if 1
  return TRUE;
#else
  UINT32  dw_fSizeAsc;
  CHAR    ac_fileAsc[k_CRCVR_FILE_SZE]={'\0'};
  FIO_t_FileDesc s_fDesc = {k_FLIDX_CRCVR,k_FLNME_CRCVR};

  /* copy new file to local file to write */
  s_clkRcvr = *ps_clkRcvr;
  /* write the data into the file according to the parameter definitions */
  dw_fSizeAsc = FIO_WriteParam(ac_fileAsc,k_CRCVR_FILE_SZE,as_clkRcrv);
  /* write the files */
  return GOE_WriteFile(s_fDesc.dw_fIdx,
                       s_fDesc.pc_fName,
                       ac_fileAsc,
                       dw_fSizeAsc,
                       (UINT8*)ps_clkRcvr,/*lint !e740*/
                       sizeof(PTP_t_cfgClkRcvr));
#endif
}

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
BOOLEAN FIO_ReadUcMstTbl(PTP_t_PortAddrQryTbl *ps_ucMstTbl )
{
#if 1
  return SC_GetUcMstTbl(ps_ucMstTbl);
#else
  UINT8   ab_data[k_UCM_FILE_SZE]={'\0'};
  UINT32  dw_bufSze = k_UCM_FILE_SZE;
  UINT8   b_fileMode;
  BOOLEAN o_ret = FALSE;
  UINT32  dw_i,dw_ucMst;
  FIO_t_FileDesc s_ucMstDesc = {k_FLIDX_UC_MST_TBL,k_FLNME_UC_MST_TBL};
  
  /* preinit the local file */
  s_lclUcMstTbl.c_logQryIntv = k_INTV_UC_MST_QRY;
  s_lclUcMstTbl.w_actTblSze  = 0;
  for( dw_i = 0 ; dw_i < k_MAX_UC_MST_TBL_SZE ; dw_i++ )
  {
    s_lclUcMstTbl.as_Addr[dw_i].e_netwProt = e_UNKNOWN;
  }  
  /* try to read the configuration file out of ROM */
  if( GOE_ReadFile(s_ucMstDesc.dw_fIdx,
                   s_ucMstDesc.pc_fName,
                   ab_data,
                   &dw_bufSze,
                   &b_fileMode) == FALSE )
  {
    /* if no file -> initialize to default values and return */
    ps_ucMstTbl->c_logQryIntv = k_INTV_UC_MST_QRY;
    ps_ucMstTbl->w_actTblSze  = 0;
    o_ret = FALSE;
  }
  else
  {
    /* interprete data */
    if( b_fileMode == k_FILE_ASCII )
    {
      /* ensure, input data are terminated */
      ab_data[dw_bufSze] = '\0';
      /* read variables into config file and ignore errors */
      if( FIO_ReadParam((CHAR*)ab_data,as_ucMstTbl,as_const) > 0 )
      {
        /* set notification */
        PTP_SetError(k_FIO_ERR_ID,FIO_k_ERR_UCMTBL,e_SEVC_NOTC);
        /* return FALSE - corrected file will be written */
        o_ret = FALSE;
      }
      else
      {
        o_ret = TRUE;
      }
      /* copy resulting config file */
      ps_ucMstTbl->c_logQryIntv = s_lclUcMstTbl.c_logQryIntv;
      ps_ucMstTbl->w_actTblSze  = s_lclUcMstTbl.w_actTblSze;
      for((dw_i = 0),( dw_ucMst = 0); dw_i < k_MAX_UC_MST_TBL_SZE ; dw_i++ )
      {
        /* if a correct network protocol is used, try to copy */
        if((s_lclUcMstTbl.as_Addr[dw_i].e_netwProt != e_UNKNOWN) &&
           (ps_ucMstTbl->aps_pAddr[dw_ucMst] != NULL ))
        {
          /* copy this entry */
          ps_ucMstTbl->aps_pAddr[dw_ucMst]->e_netwProt = 
                                 s_lclUcMstTbl.as_Addr[dw_i].e_netwProt;
          ps_ucMstTbl->aps_pAddr[dw_ucMst]->w_AddrLen = 
                                 s_lclUcMstTbl.as_Addr[dw_i].w_AddrLen;
          if( GOE_NetStrToAddr(ps_ucMstTbl->aps_pAddr[dw_ucMst]->e_netwProt,
                               ps_ucMstTbl->aps_pAddr[dw_ucMst]->ab_Addr,                               
                               ps_ucMstTbl->aps_pAddr[dw_ucMst]->w_AddrLen,
                               s_lclUcMstTbl.as_Addr[dw_i].ac_addrStr) 
                               == TRUE )
          {
            /* next master */
            dw_ucMst++;
          }
        }
      }
      /* correct the actual table size to the correct value */
      ps_ucMstTbl->w_actTblSze  = (UINT16)dw_ucMst;
      o_ret &= TRUE;
    }
    else
    {
      /* read binary */
      o_ret = ReadUcMstBin(ps_ucMstTbl,ab_data,dw_bufSze ); 
    }
  }
  return o_ret;
#endif
}

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
BOOLEAN FIO_WriteUcMstTbl(const PTP_t_PortAddrQryTbl *ps_ucMstTbl)
{
#if 1
  return TRUE;
#else
  UINT32  dw_fSzeAscii;
  INT32   l_fSzeBin;
  CHAR    ac_ucmFileAscii[k_UCM_FILE_SZE]={'\0'};
  CHAR    ac_ucmFileBin[k_UCM_FILE_SZE]={'\0'};
  UINT32  dw_i,dw_ucMst;
  FIO_t_FileDesc s_fDesc = {k_FLIDX_UC_MST_TBL,k_FLNME_UC_MST_TBL};
  
  /* copy configuration to write */
  s_lclUcMstTbl.c_logQryIntv = ps_ucMstTbl->c_logQryIntv;
  s_lclUcMstTbl.w_actTblSze  = ps_ucMstTbl->w_actTblSze;
  for((dw_i = 0),(dw_ucMst = 0); dw_i < ps_ucMstTbl->w_actTblSze ; dw_i++ )
  {
    /* if master table entry OK */
    if( (ps_ucMstTbl->aps_pAddr[dw_i] != NULL ) &&
        ( dw_i < ps_ucMstTbl->w_actTblSze))
    {
      /* copy this entry */
      s_lclUcMstTbl.as_Addr[dw_i].e_netwProt =
                    ps_ucMstTbl->aps_pAddr[dw_i]->e_netwProt;
      s_lclUcMstTbl.as_Addr[dw_i].w_AddrLen = 
                    ps_ucMstTbl->aps_pAddr[dw_i]->w_AddrLen;
      if( GOE_NetAddrToStr(s_lclUcMstTbl.as_Addr[dw_i].e_netwProt,
                           s_lclUcMstTbl.as_Addr[dw_i].ac_addrStr,
                           k_MAX_NETADDR_STRLEN,
                           ps_ucMstTbl->aps_pAddr[dw_i]->ab_Addr,                               
                           s_lclUcMstTbl.as_Addr[dw_i].w_AddrLen)==TRUE)
      {
        /* next master */
        dw_ucMst++;
      }
    }
  }
  /* change size in parameter definition file temporarily */
  as_ucMstTbl[2].w_num = (UINT16)dw_ucMst;
  /* correct unicast master table size */
  s_lclUcMstTbl.w_actTblSze = (UINT16)dw_ucMst;
  /* compile the ascii file with the given parameter array */
  dw_fSzeAscii = FIO_WriteParam(ac_ucmFileAscii,k_UCM_FILE_SZE,as_ucMstTbl);
  /* reset size in parameter definition file */
  as_ucMstTbl[2].w_num = (UINT16)dw_ucMst;
  
  /* serialize binary */
  l_fSzeBin   = WriteUcMstBin(ac_ucmFileBin,k_UCM_FILE_SZE,ps_ucMstTbl);
  /* write the files */
  return GOE_WriteFile(s_fDesc.dw_fIdx,
                        s_fDesc.pc_fName,
                        ac_ucmFileAscii,
                        dw_fSzeAscii,
                        (UINT8*)ac_ucmFileBin,
                        (UINT32)l_fSzeBin);
#endif
}

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
BOOLEAN FIO_ReadFltCfg( PTP_t_fltCfg *ps_cfgFlt )
{
#if 1
  return SC_GetFltCfg((t_fltCfg*)ps_cfgFlt);
#else
  UINT8   ab_data[k_FLT_FILE_SZE]={'\0'};
  UINT32  dw_bufSze = k_FLT_FILE_SZE;
  UINT8   b_fileMode;
  BOOLEAN o_ret = FALSE;
  FIO_t_FileDesc s_cfgDesc = {k_FLIDX_FLT,k_FLNME_FLTCFG};

  /* preinit the local file to the default values */
  InitFltDflt(&s_flt);
  /* try to read the configuration file out of ROM */
  if(( o_ret = GOE_ReadFile(s_cfgDesc.dw_fIdx,
                            s_cfgDesc.pc_fName,
                            ab_data,
                            &dw_bufSze,
                            &b_fileMode)) == TRUE )
  {
    /* interprete data */
    if( b_fileMode == k_FILE_ASCII )
    {
      /* ensure, input data are terminated */
      ab_data[dw_bufSze] = '\0';
      /* read variables into config file */
      if( FIO_ReadParam((CHAR*)ab_data,as_fltParam,as_const) > 0 )
      {
        /* return FALSE - new file will be written */
        o_ret = FALSE;
        /* set notification */
        PTP_SetError(k_FIO_ERR_ID,FIO_k_ERR_FLT,e_SEVC_NOTC);
       
      }
      else
      {
        o_ret = TRUE;
      }
    }
    else
    {
      /* read binary */
      if( dw_bufSze != sizeof(PTP_t_fltCfg))
      {
        /* return FALSE - new file will be written */
        o_ret = FALSE;
      }
      else
      {
        /* copy data into struct */
        PTP_BCOPY(&s_flt,ab_data,dw_bufSze);
        o_ret = TRUE;
      }     
    }
  }
  /* copy local struct into returned struct */
  *ps_cfgFlt = s_flt;
  return o_ret;
#endif
}

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
BOOLEAN FIO_WriteFltCfg(const PTP_t_fltCfg *ps_fltCfg)
{
#if 1
  return TRUE;
#else
  CHAR    ac_fltFileAsc[k_FLT_FILE_SZE]={'\0'};
  UINT32  dw_fSzeAsc;
  FIO_t_FileDesc s_fltDesc = {k_FLIDX_FLT,k_FLNME_FLTCFG};
 
  /* copy data to write into local struct */
  s_flt = *ps_fltCfg;
  /* write the data into readable ascii format */
  dw_fSzeAsc = FIO_WriteParam(ac_fltFileAsc,k_FLT_FILE_SZE,as_fltParam);
  /* write the files */
  return GOE_WriteFile(s_fltDesc.dw_fIdx,
                       s_fltDesc.pc_fName,
                       ac_fltFileAsc,
                       dw_fSzeAsc,
                       (UINT8*)ps_fltCfg,/*lint !e740*/
                        sizeof(PTP_t_fltCfg));
#endif
}

/*************************************************************************
**    static functions
*************************************************************************/

#if 0 
/*************************************************************************
**
** Function    : ReadUcMstBin
**
** Description : This function reads the unicast master table in binary format. 
**               This table contains the amount of 
**               masters and their unique port/interface addresses. Due 
**               to the dynamic behavior of this table during stack 
**               operation, the amount of masters varies. The parameter 
**               ps_ucMstTbl points to the stack internal structure of the 
**               unicast master table. This table provides free space for 
**               the maximum amount of table entries to copy all data out 
**               of non-volatile storage.
**
** Parameters  : ps_ucMstTbl (OUT) - pointer to unicast master table to read
**                                   in data out of non-volatile storage.
**               ab_data      (IN) - unicast master table in binary format
**               dw_fSze      (IN) - file size of the binary unicast table
**
** Returnvalue : TRUE              - function succeeded
**               FALSE             - function failed - no data available
**
** Remarks     : -
**
*************************************************************************/
static BOOLEAN ReadUcMstBin( PTP_t_PortAddrQryTbl *ps_ucMstTbl,
                             const UINT8 *ab_data,
                             UINT32 dw_fSze)
{
  UINT32 dw_sze;
  UINT32 dw_offs = 0;
  UINT16 w_mst;
 
  /* query interval */
  dw_sze = sizeof(ps_ucMstTbl->c_logQryIntv);
  PTP_BCOPY( &ps_ucMstTbl->c_logQryIntv,ab_data,dw_sze);
  dw_offs = dw_sze;
  /* actual table size */
  dw_sze = sizeof(ps_ucMstTbl->w_actTblSze);
  PTP_BCOPY( &ps_ucMstTbl->w_actTblSze ,&ab_data[dw_offs],dw_sze);
  dw_offs += dw_sze;
  /* unicast master port addresses */
  for( w_mst = 0 ; w_mst < ps_ucMstTbl->w_actTblSze ; w_mst++ )
  {
    dw_sze = sizeof(PTP_t_PortAddr);
    PTP_BCOPY(ps_ucMstTbl->aps_pAddr,&ab_data[dw_offs],dw_sze);
    dw_offs += dw_sze;
  }
  /* check, if the copy was out of range */
  if( dw_offs > dw_fSze )
  {
    return FALSE;
  }
  else
  {
    return TRUE;
  }
}

/*************************************************************************
**
** Function    : WriteUcMstBin
**
** Description : This function writes the unicast master table into a buffer
**               in binary format to write it into 
**               non-volatile storage. This table contains the amount of 
**               masters and their unique port addresses. Due to the 
**               dynamic behaviour of this table during stack operation, 
**               the amount of masters varies. Therefore memory pools are 
**               used to save these unique port addresses and only pointers 
**               to those buffers are stored within the master table. As a 
**               result, all pointers of the master table could be discarded 
**               and only the content of all dynamic allocated members must 
**               be saved within a static structure in non-volatile storage. 
**
**
** Parameters  : pc_binFile  (OUT) - binary file to write
**               dw_fileSize  (IN) - size of binary file
**               ps_ucMstTbl  (IN) - pointer to unicast master table to write
**
** Returnvalue : TRUE  - function succeeded, table written
**               FALSE - function failed, table not written
**
** Remarks     : -
**
*************************************************************************/
static INT32 WriteUcMstBin(CHAR   *pc_binFile,
                           UINT32 dw_fileSize,
                           const PTP_t_PortAddrQryTbl *ps_ucMstTbl)
{
  UINT32 dw_sze;
  UINT32 dw_offs = 0;
  UINT16 w_mst;
 
  /* query interval */
  dw_sze = sizeof(ps_ucMstTbl->c_logQryIntv);
  if( dw_fileSize < (dw_sze + dw_offs)) 
  {
    return -1;
  }
  PTP_BCOPY( pc_binFile,&ps_ucMstTbl->c_logQryIntv,dw_sze);
  dw_offs = dw_sze;
  /* actual table size */
  dw_sze = sizeof(ps_ucMstTbl->w_actTblSze);
  if( dw_fileSize < (dw_sze + dw_offs)) 
  {
    return -1;
  }  
  PTP_BCOPY( &pc_binFile[dw_offs],&ps_ucMstTbl->w_actTblSze ,dw_sze);
  dw_offs += dw_sze;
  /* unicast master port addresses */
  for( w_mst = 0 ; w_mst < ps_ucMstTbl->w_actTblSze ; w_mst++ )
  {
    dw_sze = sizeof(PTP_t_PortAddr);
    if( dw_fileSize < (dw_sze + dw_offs)) 
    {
      return -1;
    }
    PTP_BCOPY(&pc_binFile[dw_offs],ps_ucMstTbl->aps_pAddr,dw_sze);
    dw_offs += dw_sze;
  }
  return (INT32)dw_offs;
}



/***********************************************************************
**  
** Function    : InitFltDflt
**  
** Description : Initializes the filter configuration to default values 
**  
** Parameters  : ps_fltCfg (OUT) - the filter configuration file
**               
** Returnvalue : -
** 
** Remarks     : -
**  
***********************************************************************/
static void InitFltDflt( PTP_t_fltCfg *ps_fltCfg)
{
  /* no data available - initialize it with default values */
  /* set usage of 'No Filter' */
  ps_fltCfg->e_usedFilter           = e_FILT_NO;
  /* set parameters for minimum filter */
  ps_fltCfg->s_filtMin.dw_filtLen   = 4;
  /* set parameters for estimation filter */
  ps_fltCfg->s_filtEst.b_amntFlt    = 1;
  ps_fltCfg->s_filtEst.b_histLen    = 10;
  ps_fltCfg->s_filtEst.ddw_WhtNoise = 1500;
}
#endif // #if 0
