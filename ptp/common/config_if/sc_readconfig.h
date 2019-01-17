/******************************************************************************
$Id: config_if/sc_readconfig.h 1.10 2012/02/01 16:04:45PST German Alvarez (galvarez) Exp  $

Copyright (C) 2009, 2010, 2011, 2012 Symmetricom, Inc., all rights reserved
This software contains proprietary and confidential information of Symmetricom.                                                             *
It may not be reproduced, used, or disclosed to others for any purpose
without the written authorization of Symmetricom.


Original author: German Alvarez

Last revision by: $Author: German Alvarez (galvarez) $

File name: $Source: config_if/sc_readconfig.h $

Functionality:

This file is used to initialize the soft clock
reading a configuration file.

Log:
$Log: config_if/sc_readconfig.h  $
Revision 1.10 2012/02/01 16:04:45PST German Alvarez (galvarez) 
added HIGH_JITTER_ACCESS configuration
Revision 1.9 2011/10/07 09:59:56PDT German Alvarez (galvarez) 

Revision 1.8 2011/09/20 12:25:21PDT German Alvarez (galvarez) 
Implement user side of the redundancy code.
Revision 1.7 2011/08/24 17:04:33PDT Kenneth Ho (kho) 
Added Ext reference configurations
Revision 1.6 2011/02/20 11:35:24PST Kenneth Ho (kho) 
Added functions to get channel number for specific types of channels
Revision 1.5 2011/02/14 09:52:11PST Daniel Brown (dbrown) 
Updates to implement consistent naming of "b_ChanIndex" and "b_ChanCount".
Revision 1.4 2011/02/12 15:47:08PST Kenneth Ho (kho) 
Get the GPS channel number
Revision 1.3 2011/02/09 17:27:04PST German Alvarez (galvarez) 
simulate the SSM implemented int SC_ChanSSM(UINT8 b_ChanIndex, UINT8 *pb_ssm, BOOLEAN *po_Valid)
with an array of values.
Revision 1.2 2011/02/07 17:57:14PST German Alvarez (galvarez)

Revision 1.1 2011/02/01 16:25:25PST German Alvarez (galvarez)
Initial revision
Member added to project e:/MKS_Projects/PTPSoftClientDevKit/SCRDB_MPC8313/ptp/common/project.pj

******************************************************************************/
#ifndef SC_READCONFIG_H_
#define SC_READCONFIG_H_

/*--------------------------------------------------------------------------*/
//              include
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
//              defines
/*--------------------------------------------------------------------------*/
#define EXTREF_B 0
#define EXTREF_A 1

/*--------------------------------------------------------------------------*/
//              types
/*--------------------------------------------------------------------------*/
typedef struct
{
  SC_t_Chan_Type chan;
  char *name;
} t_ChanTypeName;

typedef struct
{
  t_ptpAccessTransportEnum transport;
  char *name;
} t_AccessTransportName;

/*--------------------------------------------------------------------------*/
//              data
/*--------------------------------------------------------------------------*/
extern t_AccessTransportName kAccessTransportName[];

/*--------------------------------------------------------------------------*/
//              function prototypes
/*--------------------------------------------------------------------------*/
extern int readConfigFromFile();
extern int chanTypeToString(char *s, const int bufSize, SC_t_Chan_Type chan);
extern UINT8 GetGpsChan(void);
extern UINT8 GetSe1Chan(void);
extern UINT8 GetSe2Chan(void);
extern UINT8 GetT1Chan(void);
extern UINT8 GetE1Chan(void);
extern UINT8 GetExtAChan(void);
extern UINT8 GetExtBChan(void);
extern UINT8 GetEth0Chan(void);
extern UINT8 GetEth1Chan(void);
extern UINT8 GetRedundancyChan(void);
extern UINT8 GetFreqOnlyChan(void);
extern int stringToAccessTransportType(const char *s, t_ptpAccessTransportEnum *transport);
extern int accessTransportTypeToString(const t_ptpAccessTransportEnum transport, const int bufSize, char *s);

#endif /* SC_READCONFIG_H_ */
