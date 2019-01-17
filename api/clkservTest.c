#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <unistd.h>
#include <syslog.h>
#include <string.h>

#include "clkserv_api.h"

static	void	print_menu(void);
static	char *servo_str[] = {
"Unknown",
"Warmup",
"Unknown",
"Fasttrack",
"Normaltrack",
"Holdover",
"Bridging",
};

static char *fll_str[] = {
"Unknown",
"Warmup",
"Fasttrack",
"Normaltrack",
"Bridging",
"Holdover",
};

static int putsfile(const char *path);

/*******************************************************************************
 * function : main()
 * 
 * description: main test entry
 * 
 ******************************************************************************/
int	main(int argc, char *argv[])
{
    print_menu();
    printf("> ");
	
	while(1){
		char cmd;
		scanf("%c", &cmd);
		
		if(cmd == '1'){
			int32_t ret;
			uint8_t state;

			ret = clkserv_get_servo_state(&state);
			if(ret == 0){
				printf("Current Servo State: %s\n", servo_str[state]);
			}
			else{
				printf("Fail to get servo state\n");
			}
		}
		else if(cmd == '2'){
			int32_t ret;
			servo_statics_t cur_statics;

			ret = clkserv_get_servo_statics(&cur_statics);
			if(ret == 0){
				printf("     Current FLL State: %s\n", fll_str[cur_statics.state]);
				printf(" State Duration (mins): %u\n", cur_statics.cur_state_dur);
				printf("  Frequency Corr (ppb): %g\n", cur_statics.freq_corr);
			}
			else{
				printf("Fail to get servo state\n");
			}
		}
		else if(cmd == '3'){
			int32_t ret;
			char fname[32];

			ret = clkserv_get_servo_chan_config(fname);
			if(ret == 0){
				putsfile(fname);
			}
			else{
				printf("Fail to get chan config\n");
			}
		}	
		else if(cmd == '4'){
			int32_t ret;
			char fname[32];

			ret = clkserv_get_servo_chan_status(fname);
			if(ret == 0){
				putsfile(fname);
			}
			else{
				printf("Fail to get chan status\n");
			}
		}
		else if(cmd == '5'){
			int32_t ret;
			unsigned char active_ref;

			ret = clkserv_get_servo_active_reference(&active_ref);
			if(ret == 0){
				if(active_ref == 0)
					printf("Active Ref: GPS\n");
				else if(active_ref == 1)
					printf("Active Ref: Beidou\n");
				else if(active_ref == 2)
					printf("Active Ref: IRIG-B\n");
				else
					printf("No Active Reference\n");
			}
			else{
				printf("Fail to get chan status\n");
			}
		}	
		else if(cmd == '6'){
			int32_t ret;
			int32_t value;
			uint8_t chan;
			
			scanf("%d", &value);
			if(value == 0)
				chan =0;
			else if(value ==1)
				chan =1;
			else 
				chan =2;
			ret = clkserv_set_servo_active_reference(chan);
			if(ret == 0){
				printf("set chan = %d\n", chan);
			}
			else{
				printf("Fail to set active reference\n");
			}
		}	
		else if(cmd == '7'){
			int32_t ret;
			int64_t offset;

			ret = clkserv_get_servo_phase_offset(&offset);
			if(ret == 0){
				printf("phase offset %lld\n", offset);
			}		
		}
		else if(cmd == 'c'){
			int32_t ret;
			int64_t offset;
			scanf("%lld", &offset);
			ret = clkserv_set_servo_phase_offset(offset);
			if(ret == 0){
				printf("set phase offset %lld\n", offset);
			}		
		}
		else if(cmd == 'a'){
			int32_t ret;
			uint8_t chan;
			int32_t value;
			uint8_t valid;

			scanf("%d", &value);
			if(value == 0)
				chan =0;
			else if(value ==1)
				chan =1;
			else 
				chan =2;

			ret = clkserv_get_servo_chan_valid(chan,&valid);
			if(ret == 0){
				printf("chan %d %s\n", chan, valid? "valid" : "invalid");
			}
			else{
				printf("Fail to get chan valid status\n");
			}
		}
		else if(cmd == 'b'){
			int32_t ret;
			uint8_t chan, prio;
			uint8_t valid;
			printf("Please input chan index and priority->");
			scanf("%hhu %hhu", &chan, &prio);

			ret = clkserv_set_servo_chan_prio(chan, 1, prio);
			ret = clkserv_set_servo_chan_prio(chan, 0, prio);

			if(ret != 0){
				printf("Fail to set chan %hhu priority to %d\n",chan, prio);
			}
		}		
		else if(cmd == '9'){
			int32_t ret;
			int32_t value;
			uint8_t mode;
			
			scanf("%d", &value);
			if(value == 0)
				mode =0;
			else 
				mode =1;
			ret = clkserv_set_servo_chan_switch_mode(mode);
			if(ret == 0){
				printf("set mode = %d\n", mode);
			}
			else{
				printf("Fail to set servo switch mode\n");
			}
		}		
		else if(cmd == '8'){
			int32_t ret;
			uint8_t mode;

			ret = clkserv_get_servo_chan_switch_mode(&mode);
			if(ret == 0){
				printf("mode = %d\n", mode);
			}
			else{
				printf("Fail to get servo switch mode\n");
			}
		}		
		else if((cmd == 'x') || (cmd == 'X')){
			break;
		}
		else if((cmd == 'h') || (cmd == 'X')){
			print_menu();
		}
		else{
			printf("> ");
		}	
	}

	return 0;
}

static void print_menu(void)
{
	printf("Clkserv module test definition:\n");
#if 0
	printf("...Servo state : 1 (Warmup), 3 (Fasttrack), 4 (Normaltrack)...\n");
	printf(".................5 (Holdover), 6 (Bridge).....................\n");
	printf("..............................................................\n");
	printf("......FLL state: 1 (Warmup), 2 (Fast), 3 (Normal).............\n");
	printf(".................4 (Bridge), 5 (Holdover).....................\n");
#endif
    printf("Select from following commands :\n");
    printf("\t1                  			 -- get servo state\n");
    printf("\t2                 			 -- get FLL statics\n"); 
    printf("\t3                 			 -- get chan config\n"); 
    printf("\t4                 			 -- get chan status\n"); 
    printf("\t5                 			 -- get active reference\n"); 
    printf("\t6 <chan>            			 -- set active reference\n"); 
    printf("\t7                 			 -- get phase offset\n"); 
    printf("\t8                 			 -- get switch mode\n"); 
    printf("\t9 <mode>             			 -- set switch mode\n"); 
    printf("\ta                 			 -- get chan valid\n"); 
    printf("\tb <chan> <pri>             	 -- set chan priority\n"); 
    printf("\tc <value>                 	 -- set phase offset\n"); 
    printf("\tx                  			 -- exit\n");
    printf("\th                  			 -- print this list\n");
}

static int putsfile(const char *path)
{
	FILE *fd;
	int ret;
	char buf[512];

	memset(buf,0,sizeof(buf));
	if(access(path,F_OK)!=0)
		return -1;
	sprintf(buf,"cat %s",path);
	system(buf);
	return 0;
}

