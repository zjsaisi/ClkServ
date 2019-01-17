# ###########################################
#
# This is a very simple makefile
#
# ###########################################

ifndef scriptDir
$(error scriptDir must be defined, perhaps use setupenv script?)
endif

include ../../Arch.make

prodName = PtpGpsSe
cpuType = mpc8313
configFileName = "sc_config_ptp_gps_se.h"
targetLibDir = $(sandboxDir)/ClkServ/mpc8313/lib/

ifeq ($(prodName),Ptp)
TARGETS = ptp_lib_static ptp_lib_dynamic
else ifeq ($(prodName),PtpGpsSe)
TARGETS = ptp_gps_se_span_lib_static ptp_gps_se_span_lib_dynamic
GPS_FLAGS = -D GPS_BUILD=1
else ifeq ($(prodName),PtpGpsSeSpan)
TARGETS = ptp_gps_se_span_lib_static ptp_gps_se_span_lib_dynamic
GPS_FLAGS = -D GPS_BUILD=1
else ifeq ($(prodName),BcomKa)
TARGETS = bcom_lib_static bcom_lib_dynamic 
#BCOM_FLAGS = -D BCOM_BUILD=1
#SERVO_FLAGS = -D SERVO_ONLY=1
else ifeq ($(prodName),Servo)
TARGETS = servo_lib_static servo_lib_dynamic
SERVO_FLAGS = -D SERVO_ONLY=1
else ifeq ($(prodName),PtpSe)
TARGETS = ptp_se_lib_static ptp_se_lib_dynamic
else ifeq ($(prodName),PtpGpsSpan)
TARGETS = ptp_gps_span_lib_static ptp_gps_span_lib_dynamic
GPS_FLAGS = -D GPS_BUILD=1
else ifeq ($(prodName),PtpGpsSeSpanRed)
TARGETS = ptp_gps_se_span_red_lib_static ptp_gps_se_span_red_lib_dynamic
SERVO_FLAGS = -D GPS_BUILD=1
endif

ifeq ($(useVdt),TRUE)
VER_DATE=_$(vdt)
endif

PROD_REV = 1.0

all: $(TARGETS)

include $(scriptDir)/make.top.include

CC = $(CROSS_COMPILE)gcc
LD = $(CROSS_COMPILE)ld
AR = $(CROSS_COMPILE)ar
CC_FLAGS = -D configFileName=\"$(configFileName)\" -Wall -O0 -pthread -ggdb -fPIC -I. -I./include -I../common -I../common/include -I../../Include/ -I../../drivers/ \
           -D SC_REV=\"$(PROD_REV)\" -D SC_PRODUCT_NUM=\"$(prodNum)\" $(GPS_FLAGS) -D $(subst -,_,$(cpuType)) $(BCOM_FLAGS) \
           $(SERVO_FLAGS) -mfloat-abi=soft 

SOURCES = \
     ../common/UCD/UCDmain.c \
     ../common/TCF/TCFmain.c \
     ../common/TCF/TCFsyn.c \
     ../common/TCF/TCFdel.c \
     ../common/TCF/TCFp2p.c \
     ../common/PTP/PTPmath.c \
     ../common/PTP/PTP.c \
     ../common/CTL/CTLbmc.c \
     ../common/CTL/CTLmain.c \
     ../common/CTL/CTLflt.c \
     ../common/CTL/CTLds.c \
     ../common/P2P/P2Presp.c \
     ../common/P2P/P2Pmain.c \
     ../common/P2P/P2Preq.c \
     ../common/CLK/sc_chan.c \
     ../common/CLK/sc_alarms.c \
     ../common/CLK/CLK.c \
     ../common/CLK/CLKflt.c \
     ../common/CLK/clk_ana.c \
     ../common/CLK/clk_acq.c \
     ../common/CLK/clk_input.c \
     ../common/CLK/clk_main.c \
     ../common/CLK/temperature.c \
     ../common/CLK/sc_ptp_servo.c \
     ../common/SLV/SLVdel.c \
     ../common/SLV/SLVdel_queue.c \
     ../common/SLV/SLVsyn.c \
     ../common/SLV/SLVmain.c \
     ../common/APH/APH.c \
     ../common/MNT/MNTget.c \
     ../common/MNT/MNTmain.c \
     ../common/MNT/MNTapi.c \
     ../common/MNT/MNTmsg.c \
     ../common/NIF/NIFmain.c \
     ../common/NIF/NIFrx.c \
     ../common/NIF/NIFtx.c \
     ../common/UCM/UCMannc.c \
     ../common/UCM/UCMmain.c \
     ../common/UCM/UCMsyn.c \
     ../common/UCM/UCMdel.c \
     ../common/DIS/DISmain.c \
     ../common/DIS/DISnet.c \
     ../common/DIS/DISrx.c \
     ../common/DIS/DIStx.c \
     ../common/MST/MSTmain.c \
     ../common/MST/MSTsyn.c \
     ../common/MST/MSTannc.c \
     ../common/MST/MSTdel.c \
     ../common/CIF/CIFmain.c \
     ../common/TSU/TSUmain.c \
     ../common/FIO/FIO.c \
     ../common/FIO/FIOerr.c \
     ../common/FIO/FIOread.c \
     ../common/FIO/FIOwrite.c \
     ../common/FLT/FLTest.c \
     ../common/FLT/FLTmin.c \
     ../common/FLT/FLTno.c \
     ../common/API/sc_log.c \
     ../common/API/sc_system.c \
     ../common/API/sc_ptp_mgmt.c \
     ../common/TOD/tod.c \
     ../common/DBG/DBG.c \
     ../common/shared_alarms/tp500_alarms.c \
     API/sc_mnt.c \
     API/sc_util.c \
     TGT/TGT.c \
     SIS/SISmpool.c \
     SIS/SISmain.c \
     SIS/SISevent.c \
     SIS/SISmbox.c \
     GOE/GOE.c \
     THD/THD.c

SE_SOURCES = \
     ../common/SE/SE.c

E1_SOURCES = \
     ../common/E1/E1.c

T1_SOURCES = \
     ../common/T1/T1.c

RED_SOURCES = \
     ../common/RED/Red.c

GPS_SOURCES = \
     ../common/API/sc_gps.c \
     ../common/GNS/GN_GPS_api_calls.c \
     ../common/GNS/GN_GPS_Task.c \
     ../common/GNS/GN_GPS_Time.c \
     ../common/GNS/GN_GPS_HAL.c \
     ../common/GNS/GN_GPS_GNB_Patch_205.c \
     ../common/GNS/GN_GPS_GNB_Patch_301.c \
     ../common/GNS/GN_GPS_GNB_Patch_502.c \
     ../common/GNS/GN_GPS_GNB_Patch_506.c \
     ../common/GNS/GN_GPS_GNB_Patch_510.c \
     ../common/GNS/GN_GPS_GNB_Patch.c \
     ../common/GNS/GN_GPS_Version.c \
     ../common/GPS/GPS.c

LIB_INCLUDE_DIR = ./lib_include
LIB_OBJ_DIR = lib_obj
GN_GPS_LIB = GN_GPS_Lib_iNS_powerpc-e300c3_fPIC.a
OBJECTS = $(SOURCES:.c=.o)
SHARED_OBJECTS_GPS = $(SOURCES:.c=_gps.o)
GPS_OBJECTS = $(GPS_SOURCES:.c=.o)
SE_OBJECTS = $(SE_SOURCES:.c=.o)
E1_OBJECTS = $(E1_SOURCES:.c=.o)
T1_OBJECTS = $(T1_SOURCES:.c=.o)
RED_OBJECTS = $(RED_SOURCES:.c=.o)
SERVO_OBJECTS = \
	../common/CLK/CLK.o \
	../common/CLK/clk_acq.o \
	../common/CLK/clk_ana.o \
	../common/CLK/clk_input.o \
	../common/CLK/clk_main.o \
	../common/CLK/CLKflt.o \
	../common/CLK/sc_chan.o \
	../common/CLK/sc_alarms.o \
	../common/CLK/sc_ptp_servo.o \
	../common/DBG/DBG.o

BCOM_OBJECTS = $(SERVO_OBJECTS:.o=_bcom.o)

TARGET_FILE_SUFFIX = $(cpuType)_$(prodName)$(VER_DATE)

%.o : %.c
	$(CC) -c $(CC_FLAGS) -o $@ $<
	@echo

%_gps.o : %.c
	$(CC) -c $(CC_FLAGS) -o $@ $<
	@echo

%_bcom.o : %.c
	$(CC) -c $(CC_FLAGS) -o $@ $<
	@echo

ptp_lib_static: $(OBJECTS) $(SE_OBJECTS) $(E1_OBJECTS) $(T1_OBJECTS)
	$(AR) rcs $(targetLibDir)/libsymm_$(TARGET_FILE_SUFFIX).a $(OBJECTS) $(SE_OBJECTS) $(E1_OBJECTS) $(T1_OBJECTS)
	@echo

ptp_lib_dynamic: $(OBJECTS) $(SE_OBJECTS) $(E1_OBJECTS) $(T1_OBJECTS)
	$(CC) -shared $(OBJECTS) $(SE_OBJECTS) $(E1_OBJECTS) $(T1_OBJECTS) -o $(targetLibDir)/libsymm_$(TARGET_FILE_SUFFIX).so
	@echo

servo_lib_static: $(SERVO_OBJECTS)
	$(AR) rcs $(targetLibDir)/libsymm_$(TARGET_FILE_SUFFIX).a $(SERVO_OBJECTS)
	@echo

servo_lib_dynamic: $(SERVO_OBJECTS)
	$(CC) -shared $(SERVO_OBJECTS) -o $(targetLibDir)/libsymm_$(TARGET_FILE_SUFFIX).so
	@echo

bcom_lib_static: $(OBJECTS) $(RED_OBJECTS)
	$(AR) rcs $(targetLibDir)/libsymm_$(TARGET_FILE_SUFFIX).a $(OBJECTS) $(RED_OBJECTS)
	@echo

bcom_lib_dynamic: $(OBJECTS) $(RED_OBJECTS)
	$(CC) -shared $(OBJECTS) $(RED_OBJECTS) -o $(targetLibDir)/libsymm_$(TARGET_FILE_SUFFIX).so
	@echo

gn_gps_lib:
#ifeq ($(cpuType),mpc8313)
#	@if ! [ -d $(LIB_INCLUDE_DIR)/$(LIB_OBJ_DIR) ] ; then mkdir $(LIB_INCLUDE_DIR)/$(LIB_OBJ_DIR) ; fi
#	@cd $(LIB_INCLUDE_DIR)/$(LIB_OBJ_DIR) ; $(AR) x $(sandboxDir)/ClkServ/mpc8313/$(LIB_INCLUDE_DIR)/$(GN_GPS_LIB)
#endif

ptp_gps_se_span_lib_static: $(SHARED_OBJECTS_GPS) $(GPS_OBJECTS) $(SE_OBJECTS) $(E1_OBJECTS) $(T1_OBJECTS) gn_gps_lib
ifeq ($(cpuType),mpc8313)
	$(AR) rcs $(targetLibDir)/libsymm_$(TARGET_FILE_SUFFIX).a $(SHARED_OBJECTS_GPS) $(GPS_OBJECTS) $(SE_OBJECTS) $(E1_OBJECTS) $(T1_OBJECTS) 
#$(LIB_INCLUDE_DIR)/$(LIB_OBJ_DIR)/*.o 
	@echo
	cp -f $(targetLibDir)/libsymm_$(TARGET_FILE_SUFFIX).a ../ptp/common/lib/
endif

ptp_gps_se_span_lib_dynamic: $(SHARED_OBJECTS_GPS) $(GPS_OBJECTS) $(SE_OBJECTS) $(E1_OBJECTS) $(T1_OBJECTS) gn_gps_lib
ifeq ($(cpuType),mpc8313)
	$(CC) -shared $(SHARED_OBJECTS_GPS) $(GPS_OBJECTS) $(SE_OBJECTS) $(E1_OBJECTS) $(T1_OBJECTS) -o $(targetLibDir)/libsymm_$(TARGET_FILE_SUFFIX).so
	cp -f $(targetLibDir)/libsymm_$(TARGET_FILE_SUFFIX).so ../ptp/common/lib/
#$(LIB_INCLUDE_DIR)/$(LIB_OBJ_DIR)/*.o
	@echo
endif

ptp_se_lib_static: $(OBJECTS) $(SE_OBJECTS)
ifeq ($(cpuType),mpc8313)
	$(AR) rcs $(targetLibDir)/libsymm_$(TARGET_FILE_SUFFIX).a $(OBJECTS) $(SE_OBJECTS)
	@echo
endif

ptp_se_lib_dynamic: $(OBJECTS) $(SE_OBJECTS)
ifeq ($(cpuType),mpc8313)
	$(CC) -shared $(OBJECTS) $(SE_OBJECTS) -o $(targetLibDir)/libsymm_$(TARGET_FILE_SUFFIX).so
	@echo
endif

ptp_gps_span_lib_static: $(SHARED_OBJECTS_GPS) $(GPS_OBJECTS) $(E1_OBJECTS) $(T1_OBJECTS) gn_gps_lib
ifeq ($(cpuType),mpc8313)
	$(AR) rcs $(targetLibDir)/libsymm_$(TARGET_FILE_SUFFIX).a $(SHARED_OBJECTS_GPS) $(GPS_OBJECTS) $(E1_OBJECTS) $(T1_OBJECTS) $(LIB_INCLUDE_DIR)/$(LIB_OBJ_DIR)/*.o 
	@echo
endif

ptp_gps_span_lib_dynamic: $(SHARED_OBJECTS_GPS) $(GPS_OBJECTS) $(E1_OBJECTS) $(T1_OBJECTS) gn_gps_lib
ifeq ($(cpuType),mpc8313)
	$(CC) -shared $(SHARED_OBJECTS_GPS) $(GPS_OBJECTS) $(E1_OBJECTS) $(T1_OBJECTS) $(LIB_INCLUDE_DIR)/$(LIB_OBJ_DIR)/*.o -o $(targetLibDir)/libsymm_$(TARGET_FILE_SUFFIX).so
	@echo
endif

ptp_gps_se_span_red_lib_static: $(SHARED_OBJECTS_GPS) $(GPS_OBJECTS) $(SE_OBJECTS) $(E1_OBJECTS) $(T1_OBJECTS)  $(RED_OBJECTS) gn_gps_lib
ifeq ($(cpuType),mpc8313)
	$(AR) rcs $(targetLibDir)/libsymm_$(TARGET_FILE_SUFFIX).a $(SHARED_OBJECTS_GPS) $(GPS_OBJECTS) $(SE_OBJECTS) $(E1_OBJECTS) $(T1_OBJECTS) $(RED_OBJECTS) $(LIB_INCLUDE_DIR)/$(LIB_OBJ_DIR)/*.o 
	@echo
endif

ptp_gps_se_span_red_lib_dynamic: $(SHARED_OBJECTS_GPS) $(GPS_OBJECTS) $(SE_OBJECTS) $(E1_OBJECTS) $(T1_OBJECTS)  $(RED_OBJECTS) gn_gps_lib
ifeq ($(cpuType),mpc8313)
	$(CC) -shared $(SHARED_OBJECTS_GPS) $(GPS_OBJECTS) $(SE_OBJECTS) $(E1_OBJECTS) $(T1_OBJECTS) $(RED_OBJECTS) $(LIB_INCLUDE_DIR)/$(LIB_OBJ_DIR)/*.o -o $(targetLibDir)/libsymm_$(TARGET_FILE_SUFFIX).so
	@echo
endif

gn_gps_lib_clean:
	@if [ -d $(LIB_INCLUDE_DIR)/$(LIB_OBJ_DIR) ] ; then rm -R $(LIB_INCLUDE_DIR)/$(LIB_OBJ_DIR) ; fi

clean: gn_gps_lib_clean
	find ../common -name "*.o" -print -exec rm {} \;
	find . -name "*.o" -print -exec rm {} \;
	find $(targetLibDir) -name "*.a" -print -exec rm {} \;
	find $(targetLibDir) -name "*.so" -print -exec rm {} \;

.SUFFIXES: .c .o .co

.PHONY: clean gn_gps_lib gn_gps_lib_clean
