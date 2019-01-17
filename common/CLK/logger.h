//#define NO_RAM_LOG_ENTRIES 200
#define NO_RAM_LOG_ENTRIES 200

#define SIZE_OF_LOG_FLASH 0x3FFF
//#define SIZE_OF_LOG_FLASH 0x180
#define NUMBER_OF_LOGS    3
#define SIZE_OF_ENTRY 16 /*bytes*/
#define NUMBER_OF_ENTRIES_PER_LOG (SIZE_OF_LOG_FLASH/NUMBER_OF_LOGS/SIZE_OF_ENTRY)
#define SIZE_OF_LOG_BLOCK NUMBER_OF_ENTRIES_PER_LOG * SIZE_OF_ENTRY
#define FLASH_LOC_LOGGER (0x8000)
#define FLASH_LOC_SETTINGS1 (0x4000)
#define FLASH_LOC_SETTINGS2 (0x6000) 

#define LOG_IS_GOOD         0
#define LOG_IS_EMPTY        1
#define LOG_IS_OUT_OF_RANGE 2

#define LOG_CONFIGURATION 0
#define LOG_ALARM         1
#define LOG_DEBUG         2

#define LOG_TYPE_POWER_ON        1
#define LOG_TYPE_STATE_CHANGE    2
#define LOG_TYPE_ALARM 			 3
#define LOG_TYPE_CONFIG_CHANGE	 4
#define LOG_TYPE_TIME_SET		 5
#define LOG_TYPE_ALARM_EXIT		 6
#define LOG_TYPE_CHANGE_SETTING	 7
#define LOG_TYPE_LOGIN			 8
#define LOG_TYPE_LOGOUT			 9
#define LOG_TYPE_REF_CHANGE		 10
#define LOG_TYPE_EXCEPTION		 11 //GPZ added for debug level reporting
#define LOG_TYPE_RESTART_PHASE		 12

#define LOG_LOGIN_CLI			0
#define LOG_LOGIN_TELNET		1


/* list of settings to be set */
#define LOG_CH_SET_DHCP        0
#define LOG_CH_SET_ALM_LEVEL   1
#define LOG_CH_SET_ALM_DELAY   2
#define LOG_CH_SET_ALM_ENABLE  3
#define LOG_CH_SET_TIME 	   4
#define LOG_CH_SET_FDRIFT 	   5
#define LOG_CH_LOG_CLEAR       6
#define LOG_CH_LOG_PRESET      7
#define LOG_CH_SER_NUM         8
#define LOG_CH_EIA232_BAUD     9
#define LOG_CH_EIA232_PARITY   10
#define LOG_CH_EIA232_DATA     11
#define LOG_CH_HDB3			   12
#define LOG_CH_OUTPUT_TYPE	   13
#define LOG_CH_MAC	 		   14
#define LOG_CH_CLOCK_ID	  	   15
#define LOG_CH_ACC_MASTER	   16
#define LOG_CH_MASTER	  	   17
#define LOG_CH_ANNOUNCE_INTV   18
#define LOG_CH_SYNC_INTV  	   19
#define LOG_CH_DEL_INTV	  	   20
#define LOG_CH_LEASE_DUR  	   21
#define LOG_CH_INIT_SYNC_INTV  22
#define LOG_CH_IP	  	  	   23
#define LOG_CH_VLAN_ID_PRI 	   24
#define LOG_CH_VLAN_ENABLE 	   25
#define LOG_CH_LOGIN_PASSWD	   26
#define LOG_CH_FIREWALL_TELNET 27
#define LOG_CH_FIREWALL_FTP    28
#define LOG_CH_FIREWALL_ICMP   29
#define LOG_CH_OUTP_FREE_MODE  30
#define LOG_CH_OUTP_HOLD_MODE  31
#define LOG_CH_BRIDGE_TIME     32
#define LOG_CH_PTP_ENABLE_TYPE 33
#define LOG_CH_DOMAIN          34
#define LOG_CH_DSCP            35

#define LOG_CH_SET_ALM_ALL 0xFFFFFFFF

// Exception level reporting sub structure
#define LOG_EXC_NO_SYNC 0
#define LOG_EXC_SYNC_START 1
#define LOG_EXC_NO_DELAY 2
#define LOG_EXC_DELAY_START 3
#define LOG_EXC_FLL_START 4
#define LOG_EXC_FLL_COMPLETE 5
#define LOG_EXC_EXCESSIVE_DIFF_CHAN_ERR 6
#define LOG_EXC_SYNC_LOOP_PHASE_RANGE 7
#define LOG_EXC_DELAY_LOOP_PHASE_RANGE 8
#define LOG_EXC_RECENTER_QUIET_TIMEOUT 9
#define LOG_EXC_RECENTER_BRIDGING 10
#define LOG_EXC_RECENTER_RANGE 11
#define LOG_EXC_RESHAPE_USE_NEW_BASE 12
#define LOG_EXC_RESHAPE_USE_OLD_BASE 13
#define LOG_EXC_RECENTER_QUIET_RANGE 14
#define LOG_EXC_SYNC_POP_DETECTED 15
#define LOG_EXC_SYNC_RANGE_DETECTED 16
#define LOG_EXC_SYNC_SLEW_DETECTED 17
#define LOG_EXC_DELAY_POP_DETECTED 18
#define LOG_EXC_DELAY_RANGE_DETECTED 19
#define LOG_EXC_DELAY_SLEW_DETECTED 20
#define LOG_EXC_FIT_FREQ_EST 21
#define LOG_EXC_FLOOR_BIAS_EST 22
#define LOG_EXC_FLOOR_TDEV_EST 23
#define LOG_EXC_FLOOR_RTD_EST 24
#define LOG_EXC_TEMP_AVG_EST 25
#define LOG_EXC_FLOOR_MAFE_EST 26

typedef struct  
{
	union {
	UINT32 unsigned_32;
	INT32  signed_32;
	} sign;
} LOG_PARAM_UNION;

typedef struct  
{
	UINT32	timestamp;/* Time stamp in seconds */
	UINT16 type;     /* 2 bytes reserved      */
	UINT16 reserved; /* 2 bytes reserved      */
	UINT32	param1;	  /* first parameter       */
	UINT32	param2;   /* second parameter      */
} log_entry_type;

/*  This will be the structure kept in RAM, it is a circular queue..
 */
typedef struct	
{
	UINT16 NextRamIn;  /* next space to be written */
	UINT16 OldestRamOut; /* oldest entry */   	
	UINT16 NextFlashIn;  /* Ram index of next Flash to be saved*/
	UINT32 MinFlashAddress;
	UINT32 MaxFlashAddress;
	UINT32 CurrentFlashAddress; /* next Flash address to be written */
	UINT16 wrapped;	  	/*  1 == entries full, are wrapping around to 0 	*/
	log_entry_type entry[ NO_RAM_LOG_ENTRIES ];

} RamLog_Struct_Type;

typedef struct  {
RamLog_Struct_Type Log[NUMBER_OF_LOGS];
} Ram_Log_Type;

extern Ram_Log_Type Ram_Log;

extern UINT32 init_RamLog( void );
extern void Send_Log(UINT16 log_type, UINT16 log_entry_num, UINT32 param1, UINT32 param2, UINT16 reserved);
extern void Update_Logger(void);
extern UINT16 Get_Log(char *buf, UINT16 log, UINT16 entry_num);
extern INT16 erase_flash_log(void);
