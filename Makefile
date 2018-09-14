# file      Makefile
# author    $Author$
# version   $Rev$
# date      $Date$
# brief     a simple makefile to (cross) compile.
#           uses the openembedded provided sysroot and toolchain
# target    Linux on Apalis/Colibri modules
# caveats   -

CC ?= gcc
CPP ?= g++
APP_BINARY = aiiot_airkiss
VERSION = 0.1
PREFIX = /usr/local/bin

WARNINGS = -Wall
CFAGS_OPT-release = -O0 -g
CFAGS_OPT-debug = -O2
CFLAGS = -DLINUX -DVERSION=\"$(VERSION)\" $(WARNINGS) -I$(OECORE_TARGET_SYSROOT) -I./include

SRCS = aiiot_airkiss.c
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
LIBAIRKISS-debug = -L./libak -lairkiss_log
LIBAIRKISS-release = -L./libak -lairkiss
LIBCRC = -L./libcrc -lcrc
LIBS-debug = -lpthread -lm -L$(OECORE_TARGET_SYSROOT)/usr/lib $(LIBAIRKISS-debug) $(LIBIW) $(LIBTIMER) $(LIBCRC)
LIBS-release = -lpthread -lm -L$(OECORE_TARGET_SYSROOT)/usr/lib $(LIBAIRKISS-release) $(LIBIW) $(LIBTIMER) $(LIBCRC)

all: debug release

clean:
	@echo "Cleaning up directory."
	rm -f *.a $(OBJS) $(APP_BINARY) core *~ log errlog

install:
	install $(APP_BINARY) $(PREFIX)

# Applications:
debug: $(OBJS)
	$(CC) $(CFLAGS) $(CFAGS_OPT-debug) $(OBJS) $(LIBS-debug) -o $(APP_BINARY)

release: $(OBJS)
	$(CC) $(CFLAGS) $(CFAGS_OPT-release) $(OBJS) $(LIBS-release) -o $(APP_BINARY)

