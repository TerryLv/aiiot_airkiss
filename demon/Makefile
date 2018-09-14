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
APP_BINARY=aiiot_airkiss_demon
VERSION = 0.1
PREFIX=/usr/local/bin

WARNINGS = -Wall
CFAGS_OPT-release = -O0 -g
CFAGS_OPT-debug = -O2
CFLAGS = -DLINUX -DVERSION=\"$(VERSION)\" $(WARNINGS) -I$(OECORE_TARGET_SYSROOT)

SRCS = aiiot_airkiss_demon.c
OBJS = $(SRCS:.c=$(BIN_POSTFIX).o)

LIBS = -lpthread -lm -L$(OECORE_TARGET_SYSROOT)/usr/lib


all:   debug release 

clean:
	@echo "Cleaning up directory."
	rm -f *.a $(OBJS) $(APP_BINARY) core *~ log errlog

install:
	install $(APP_BINARY) $(PREFIX)

# Applications:
debug: $(OBJS)
	$(CC) $(CFLAGS) $(CFAGS_OPT-debug) $(OBJS) $(LIBS) -o $(APP_BINARY)

release: $(OBJS)
	$(CC) $(CFLAGS) $(CFAGS_OPT-release) $(OBJS) $(LIBS-release) -o $(APP_BINARY)

