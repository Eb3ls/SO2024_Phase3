# Cross toolchain variables
# If these are not in your path, you can make them absolute.
XT_PRG_PREFIX ?= mipsel-linux-gnu-
CC = $(XT_PRG_PREFIX)gcc
LD = $(XT_PRG_PREFIX)ld
HOST_CC ?= cc

# uMPS3-related paths

# Simplistic search for the umps3 installation prefix.
# If you have umps3 installed on some weird location, set UMPS3_DIR_PREFIX by hand.
ifneq ($(wildcard /usr/bin/umps3),)
	UMPS3_DIR_PREFIX = /usr
else
	UMPS3_DIR_PREFIX = /usr/local
endif

UMPS3_DATA_DIR = $(UMPS3_DIR_PREFIX)/share/umps3
UMPS3_INCLUDE_DIR = $(UMPS3_DIR_PREFIX)/include/umps3

# Compiler options
CFLAGS_LANG = -ffreestanding # -ansi
CFLAGS_MIPS = -mips1 -mabi=32 -mno-gpopt -G 0 -mno-abicalls -fno-pic -mfp32
CFLAGS = $(CFLAGS_LANG) $(CFLAGS_MIPS) -I$(UMPS3_INCLUDE_DIR) -Wall -O0 -MMD -MP

# Linker options
LDFLAGS = -G 0 -nostdlib -T $(UMPS3_DATA_DIR)/umpscore.ldscript

# Add the location of crt*.S to the search path
VPATH = $(UMPS3_DATA_DIR)

.PHONY : all clean testers test

all : kernel.core.umps testers

testers :
	$(MAKE) -C testers UMPS3_DIR_PREFIX="$(UMPS3_DIR_PREFIX)" XT_PRG_PREFIX="$(XT_PRG_PREFIX)"

test :
	HOST_CC="$(HOST_CC)" UMPS3_INCLUDE_DIR="$(UMPS3_INCLUDE_DIR)" sh tests/run.sh

kernel.core.umps : kernel
	umps3-elf2umps -k $<

OBJECTS = phase1/msg.o phase1/pcb.o phase2/initial.o phase2/scheduler.o phase2/SSI.o phase2/interrupts.o phase2/utils.o phase2/syscall.o phase3/initProc.o phase3/sst.o phase3/sysSupport.o phase3/vmSupport.o phase3/utils_phase3.o phase3/sst_utils.o crtso.o libumps.o

kernel : $(OBJECTS)
	$(LD) -o $@ $^ $(LDFLAGS)

clean :
	rm -f $(OBJECTS) $(OBJECTS:.o=.d) kernel kernel.*.umps
	$(MAKE) -C testers clean

# Pattern rule for assembly modules
%.o : %.S
	$(CC) $(CFLAGS) -c -o $@ $<

-include $(OBJECTS:.o=.d)
