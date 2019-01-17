#*****************************************************************************
#
# FILE NAME    : Makefile
#
#******************************************************************************

ifndef scriptDir
$(error scriptDir must be defined, perhaps use setupenv script?)
endif

include ../../../Arch.make

#TARGET = $(prodName)
TARGET = PtpGpsSe
prodName = PtpGpsSe
configFileName = "sc_config_ptp_gps_se.h"
LIB_NAME = libsymm_mpc8313_$(prodName).a

all: $(TARGET)

EXE_NAME = ../../clkservd

EXPORT_LIB = ../../libzss_clkserv.so

TEST_UTIL = ../../clkservTest

include $(scriptDir)/make.top.include

CC = $(CROSS_COMPILE)gcc
LD = $(CROSS_COMPILE)ld
CC_FLAGS = -D configFileName=\"$(configFileName)\" -Wall -O3 -pthread -ggdb -mfloat-abi=soft -I. -I../common -I../common/include -I../../api/ -I../../../Mstate -I../../../Clock 
CC_FLAGS_GPS = -D configFileName=\"$(configFileName)\" -Wall -O3 -pthread -ggdb -mfloat-abi=soft -I. -I../common -I../common/include -I../../api/ -I../../../Include/ -I../../../Mstate -I../../../Clock -D GPS_BUILD=1
LD_FLAGS = ../common/lib/$(LIB_NAME) -L../../../Mstate -lzss_mstate -L../../../Clock -lzss_clock \
           -lpthread -lrt -lm -Wl,-Map,$(EXE_NAME).map

SOURCES := \
	./natsemi_util/epl_1588.c \
	./timestamper_if/sc_timestamp.c \
	./clock_if/sc_clock_if.c \
	../common/network_if/sc_network_if.c \
	../common/log/sc_log.c \
	../common/user_if/sc_ui.c \
	../common/config_if/sc_readconfig.c \
	../common/chan_processing_if/sc_SSM_handling.c \
	../common/main.c

SE_SOURCES = \
	../common/se_if/sc_se_if.c

E1_SOURCES = \
	../common/e1_if/sc_e1_if.c

T1_SOURCES = \
	../common/t1_if/sc_t1_if.c

GPS_SOURCES = \
	../common/gps_if/sc_gps_if.c

RED_SOURCES = \
	../common/redundancy_if/sc_redundancy_if.c

API_SOURCES = \
	../../api/clkserv_api.c

TEST_SOURCES = \
	../../api/clkservTest.c

OBJECTS := $(SOURCES:.c=.o)
SHARED_OBJECTS_GPS := $(SOURCES:.c=_gps.o)
SE_OBJECTS := $(SE_SOURCES:.c=.o)
E1_OBJECTS := $(E1_SOURCES:.c=.o)
T1_OBJECTS := $(T1_SOURCES:.c=.o)
GPS_OBJECTS := $(GPS_SOURCES:.c=.o)
RED_OBJECTS := $(RED_SOURCES:.c=.o)
API_OBJECTS := $(API_SOURCES:.c=.o)
TEST_OBJECTS := $(TEST_SOURCES:.c=.o)

%.o : %.c  $(HEADERS)
	$(CC) -c $(CC_FLAGS) -o $@ $<
	@echo

%_gps.o : %.c  $(HEADERS)
	$(CC) -c $(CC_FLAGS_GPS) -o $@ $<
	@echo

$(EXPORT_LIB): $(API_OBJECTS)
	$(CC) -shared -o $@ $<

$(API_OBJECTS): $(API_SOURCES)
	$(CC) -fPIC $(CC_FLAGS_GPS) -o $@ -c $<

$(TEST_OBJECTS): $(TEST_SOURCES)
	$(CC) -c $(CC_FLAGS_GPS) -o $@ $<

lib: $(EXPORT_LIB)

default: $(TARGET)

Ptp: $(OBJECTS) ../common/lib/$(LIB_NAME) Makefile
	$(CC) $(OBJECTS) $(LD_FLAGS) -o $(EXE_NAME) 
	@echo

PtpGpsSe: $(SHARED_OBJECTS_GPS) $(SE_OBJECTS) $(GPS_OBJECTS) ../common/lib/$(LIB_NAME) $(EXPORT_LIB) $(TEST_OBJECTS) Makefile
	$(CC) $(SHARED_OBJECTS_GPS) $(SE_OBJECTS) $(GPS_OBJECTS) $(LD_FLAGS) -o $(EXE_NAME) 
	$(CC)  -L../../ -lzss_clkserv $(TEST_OBJECTS) -o $(TEST_UTIL)
	@echo

PtpGpsSeSpan: $(SHARED_OBJECTS_GPS) $(SE_OBJECTS) $(E1_OBJECTS) $(T1_OBJECTS) $(GPS_OBJECTS) ../common/lib/$(LIB_NAME) Makefile
	$(CC) $(SHARED_OBJECTS_GPS) $(SE_OBJECTS) $(E1_OBJECTS) $(T1_OBJECTS) $(GPS_OBJECTS) $(LD_FLAGS) -o $(EXE_NAME) 
	@echo

PtpSe: $(OBJECTS) $(SE_OBJECTS) ../common/lib/$(LIB_NAME) Makefile
	$(CC) $(OBJECTS) $(SE_OBJECTS) $(LD_FLAGS) -o $(EXE_NAME) 
	@echo

PtpGpsSpan: $(SHARED_OBJECTS_GPS) $(E1_OBJECTS) $(T1_OBJECTS) $(GPS_OBJECTS) ../common/lib/$(LIB_NAME) Makefile
	$(CC) $(SHARED_OBJECTS_GPS) $(E1_OBJECTS) $(T1_OBJECTS) $(GPS_OBJECTS) $(LD_FLAGS) -o $(EXE_NAME) 
	@echo

PtpGpsSeSpanRed: $(SHARED_OBJECTS_GPS) $(SE_OBJECTS) $(E1_OBJECTS) $(T1_OBJECTS) $(GPS_OBJECTS) $(RED_OBJECTS) ../common/lib/$(LIB_NAME) Makefile
	$(CC) $(SHARED_OBJECTS_GPS) $(SE_OBJECTS) $(E1_OBJECTS) $(T1_OBJECTS) $(GPS_OBJECTS) $(RED_OBJECTS) $(LD_FLAGS) -o $(EXE_NAME) 
	@echo

BcomKa: $(OBJECTS) $(RED_OBJECTS) ../common/lib/$(LIB_NAME) Makefile
	$(CC) $(OBJECTS) $(RED_OBJECTS) $(LD_FLAGS) -o $(EXE_NAME) 
	@echo

sc_prng_test: ../common/sc_prng_test.o ../common/lib/libsymm_mpc8313_Servo.a
	$(CC)  ../common/sc_prng_test.o ../common/lib/libsymm_mpc8313_Servo.a -lpthread -lrt -lm -Wl -o  ../common/sc_prng_test

clean:
	rm -f $(OBJECTS)
	rm -f $(SHARED_OBJECTS_GPS)
	rm -f $(SE_OBJECTS)
	rm -f $(E1_OBJECTS)
	rm -f $(T1_OBJECTS)
	rm -f $(GPS_OBJECTS)
	rm -f $(RED_OBJECTS)
	rm -f $(API_OBJECTS)
	rm -f $(TEST_OBJECTS)
	rm -f $(EXE_NAME)
	rm -f $(EXPORT_LIB)
	rm -f $(TEST_UTIL)
	rm -f $(EXE_NAME).map
	rm -f ../common/lib/libsymm_mpc8313_$(prodName)*

.SUFFIXES: .c .o .co

.PHONY: clean
