# file      Makefile
# author    $Author$
# version   $Rev$
# date      $Date$
# brief     a simple makefile to (cross) compile.
#           uses the openembedded provided sysroot and toolchain
# target    Linux on Apalis/Colibri modules
# caveats   -

#CC=gcc
#CPP=g++
APP_BINARY=aiiot_airkiss
VERSION = 0.1
PREFIX=/usr/local/bin

WARNINGS = -Wall

#CFLAGS = -std=gnu99 -O2 -DLINUX -DVERSION=\"$(VERSION)\" $(WARNINGS)
CFLAGS = -O0 -g -DLINUX -DVERSION=\"$(VERSION)\" $(WARNINGS) -I$(OECORE_TARGET_SYSROOT) -I./include
CPPFLAGS = $(CFLAGS)

SRCS = aiiot_airkiss.c
#SRCS += capture/common.c capture/osdep.c capture/linux.c capture/radiotap/radiotap-parser.c
SRCS += \
		aircrack-osdep/common.c \
		aircrack-osdep/osdep.c \
		aircrack-osdep/linux.c \
		aircrack-osdep/file.c \
		aircrack-osdep/network.c \
		aircrack-osdep/radiotap/radiotap.c
SRCS += wifi_scan.c
OBJS = $(SRCS:.c=$(BIN_POSTFIX).o)

LIBIW = -liw
LIBTIMER = -lrt
LIBAIRKISS = -L./libak -lairkiss_log
LIBCRC = -L./libcrc -lcrc
LIBS = -lpthread -lm -L$(OECORE_TARGET_SYSROOT)/usr/lib $(LIBAIRKISS) $(LIBIW) $(LIBTIMER) $(LIBCRC)


all:   $(APP_BINARY) 

clean:
	@echo "Cleaning up directory."
	rm -f *.a $(OBJS) $(APP_BINARY) core *~ log errlog

install:
	install $(APP_BINARY) $(PREFIX)

# Applications:
$(APP_BINARY): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) $(LIBS) -o $(APP_BINARY)
