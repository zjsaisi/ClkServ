/*HEADER****************************************************************
************************************************************************
***
*** File: tp500_alarms.h
***
*** Comments:  This file contains a structure that is used to list 
*** alarm id numbers and associated text.
***
************************************************************************
*END*******************************************************************/


typedef struct {
	unsigned char bit_position;
	char *dsc_str;
} SC_alarm_table;

extern SC_alarm_table sc_alarm_lst[];

#ifndef e_HOLDOVER
#define e_HOLDOVER	0
#define e_FREERUN	   1
#define e_BRIDGING	2
#define e_CHAN_NO_DATA			3
#define e_CHAN_PERFORMANCE		4
#define e_FREQ_CHAN_QUALIFIED	5
#define e_FREQ_CHAN_SELECTED	6
#define e_TIME_CHAN_QUALIFIED	7
#define e_TIME_CHAN_SELECTED	8
#define e_CHAN_NOT_VALID		9
#define e_TIMELINE_CHANGED		10
#define e_PHASE_ALIGNMENT		11
#define e_EXCESSIVE_FREQUENCY_CORR    12
#define e_INCOMPATIBLE_TRANSPORT_MODE 13
#endif
