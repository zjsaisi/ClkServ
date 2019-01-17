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
} TP500_alarm_table;

extern TP500_alarm_table tp500_alarm_lst[];

