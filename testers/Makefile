#$Id: Makefile,v 1.2 2004/05/01 14:53:48 morsiani Exp morsiani $
# Makefile for mipsel-linux
#
ifneq ($(wildcard /usr/bin/umps3),)
	UMPS3_DIR_PREFIX = /usr
else
	UMPS3_DIR_PREFIX = /usr/local
endif

SUPDIR = $(UMPS3_DIR_PREFIX)/share/umps3
UMPS3_DATA_DIR = $(UMPS3_DIR_PREFIX)/share/umps3
UMPS3_INCLUDE_DIR = $(UMPS3_DIR_PREFIX)/include/umps3

CFLAGS = -ffreestanding -c -mips1 -mabi=32 -mfp32 -mno-gpopt -G 0 -fno-pic -mno-abicalls -I$(UMPS3_INCLUDE_DIR) -Wall -MMD -MP

LDAOUTFLAGS = -G 0 -nostdlib -T $(SUPDIR)/umpsaout.ldscript
XT_PRG_PREFIX ?= mipsel-linux-gnu-
CC = $(XT_PRG_PREFIX)gcc
LD = $(XT_PRG_PREFIX)ld

EF = umps3-elf2umps
UDEV = umps3-mkdev

# Add the location of crt*.S to the search path
VPATH = $(UMPS3_DATA_DIR)

TESTS = todTest terminalTest1 terminalTest2 terminalTest3 terminalTest4 fibEight fibEleven printerTest
OBJECTS = $(TESTS:=.o) print.o crti.o libumps.o

.PHONY: all clean
all: $(TESTS:=.umps)

.SECONDARY: $(OBJECTS) $(TESTS:=.t) $(TESTS:=.t.aout.umps)

# Pattern rule for assembly modules
%.o : %.S
	$(CC) $(CFLAGS) -c -o $@ $<

%.t: %.o print.o crti.o libumps.o
	$(LD) $(LDAOUTFLAGS) crti.o $< print.o libumps.o -o $@

%.t.aout.umps: %.t
	$(EF) -a $<

%.umps: %.t.aout.umps
	$(UDEV) -f $@ $<

clean:
	rm -f *.o *.d *.t *.umps

-include $(OBJECTS:.o=.d)
