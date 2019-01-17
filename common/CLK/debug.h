/*********************************************************************************
File:			debug.h
*********************************************************************************/
#define LAN_DEBUG_PRT 	0x1  /* channel 0 */
#define GEORGE_PTP_PRT 	0x2  /* channel 1 */
#define FLASH_DEBUG_PRT 0x4  /* channel 2 */
#define GEORGE_PTPD_PRT 0x8  /* channel 3 */
#define GEORGE_FAST_PRT 0x10  /* channel 4 */
#define KEN_PTP_PRT 	0x40  /* channel 6 */
#define PPS_TICK_PRT 	0x20  /* channel 5 */
#define TIME_PRT 		0x80  /* channel 7 */
#define PLL_PRT 		0x100  /* channel 8 */
#define REDUNDANCY_PRT 	0x200  /* channel 9 */
#define SELECT_DEBUG_PRT 0x400  /* channel 10 */
#define PKT_DEBUG_PRT 0x800  /* channel 11 */
#define DHCP_PRT 0x1000  /* channel 12 */
#define MMFR_PRT 0x2000  /* channel 13 */
#define SEQ_PRT 0x4000  /* channel 14 */
#define KEN_PRT 0x8000  /* channel 15 */
#define CHAN_STAT_PRT 0x10000  /* channel 16 */

extern int Set_Debug_Message(int mess_num);
extern int Get_Debug_Message(void);
extern int debug_printf(int message_num, const char *fmt_ptr, ... );


