/*HEADER****************************************************************
************************************************************************
***
***
*** File: tp500_alarms.c
***
*** Comments:  This file contains a list of alarm id with associated
*** text.
***
************************************************************************
*END*******************************************************************/

#include "tp500_alarms.h"

TP500_alarm_table tp500_alarm_lst[] =
{
{0, "Holdover"},
{1, "Free-run(Power-on)"},
{2, "Bridging"},
{3, "FPGA"},
{4, "10MHz LOS"},
{5, "LINK loss of signal"},
{6, "Outputs disabled"},
{7, "PTP disabled"},

/* alarms for low packet rates */
{8, "Sync packet rate low(GM1)"},
{9, "Sync packet rate low(GM2)"},
{10,"Delay packet rate low"},

/* alarms for announce packer Timeouts */
{11, "Announce timeout(GM1)"},
{12, "Announce timeout(GM2)"},

/* alarms for signaling timeouts on ACK */
{13, "Signaling ack timeout(GM1)"},
{14, "Signaling ack timeout(GM2)"},

{15, "Firmware upgrade"},
{16, "Power Supply A"},
{17, "Power Supply B"},
{18, "Packet RTD too large"},
{19, "Excessive transients"},
{0, 0}
};
