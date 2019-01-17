/*HEADER****************************************************************
************************************************************************
***
***
*** File: sc_alarms.c
***
*** Comments:  This file contains a list of alarm id with associated
*** text.
***
************************************************************************
*END*******************************************************************/

#include "sc_alarms.h"

SC_alarm_table sc_alarm_lst[] =
{
{0, "Holdover"},
{1, "Free-run(Power-on)"},
{2, "Bridging"},
{3, "Channel No Data"},
{4, "Channel Performance"},
{5, "Freq Channel Qualified"},
{6, "Freq Channel Selected"},
{7, "Time Channel Qualified"},
{8, "Time Channel Selected"},
{9, "Channel Not Valid"},
{10,"Timeline Changed"},
{11,"Phase Alignment"},
{12,"Excessive Frequency Correction Value"},
{13,"Incompatible Transport Type"},
{0, 0}
};
