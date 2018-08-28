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

#CFLAGS = -std=gnu99 -O2 -DLINUX -DVERSION=\"$(VERSION)\" $(WARNINGS)
CFLAGS = -O0 -g -DLINUX -DVERSION=\"$(VERSION)\" $(WARNINGS) -I$(OECORE_TARGET_SYSROOT) -I./include
CPPFLAGS = $(CFLAGS)

SRCS = aiiot_airkiss_demon.c
OBJS = $(SRCS:.c=$(BIN_POSTFIX).o)

LIBS = -lpthread -lm -L$(OECORE_TARGET_SYSROOT)/usr/lib


all:   $(APP_BINARY) 

clean:
	@echo "Cleaning up directory."
	rm -f *.a $(OBJS) $(APP_BINARY) core *~ log errlog

install:
	install $(APP_BINARY) $(PREFIX)

# Applications:
$(APP_BINARY): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) $(LIBS) -o $(APP_BINARY)
