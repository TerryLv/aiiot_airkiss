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
RM = rm -f
STRIP ?= strip

PROG = aiiot_airkiss_demon
VERSION = 0.1
PREFIX=/usr/local/bin

WARNINGS = -Wall
HOST_ROOTFS ?= $(OECORE_TARGET_SYSROOT)
ifeq ($(DEBUG), 1)
	CFLAGS = -DLINUX -DVERSION=\"$(VERSION)\" -O0 -g $(WARNINGS) -I$(HOST_ROOTFS) -I./include
else
	CFLAGS = -DLINUX -DVERSION=\"$(VERSION)\" -O2 $(WARNINGS) -I$(HOST_ROOTFS) -I./include
endif

# Sets the output filename and object files
SRCS = aiiot_airkiss_demon.c
OBJS = $(SRCS:.c=$(BIN_POSTFIX).o)
DEPS= $(OBJS:.o=.d)

LIBS = -lpthread -lm -L$(HOST_ROOTFS)/usr/lib

# pull in dependency info for *existing* .o files
-include $(DEPS)

all: $(PROG)

$(PROG): $(OBJS) Makefile
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LIBS) $(LDFLAGS)
	#$(STRIP) $@

%$(BIN_POSTFIX).o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<
	$(CC) -MM $(CFLAGS) $< > $@.d

clean:
	@echo "Cleaning up directory."
	$(RM) $(OBJS) $(PROG) $(DEPS)

install:
	install $(PROG) $(PREFIX)

