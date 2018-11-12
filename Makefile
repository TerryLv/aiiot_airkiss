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

PROG = aiiot_airkiss
VERSION = 0.1
PREFIX = /usr/local/bin

WARNINGS = -Wall
HOST_ROOTFS ?= $(OECORE_TARGET_SYSROOT)
ifeq ($(DEBUG), 1)
	CFLAGS = -DLINUX -DVERSION=\"$(VERSION)\" -O0 -g $(WARNINGS) -I$(HOST_ROOTFS) -I./include
else
	CFLAGS = -DLINUX -DVERSION=\"$(VERSION)\" -O2 $(WARNINGS) -I$(HOST_ROOTFS) -I./include
endif

SRCS = aiiot_airkiss.c
SRCS += \
		aircrack-osdep/common.c \
		aircrack-osdep/osdep.c \
		aircrack-osdep/linux.c \
		aircrack-osdep/file.c \
		aircrack-osdep/network.c \
		aircrack-osdep/radiotap/radiotap.c
SRCS += wifi_scan.c

# Sets the output filename and object files
OBJS= $(SRCS:.c=$(BIN_POSTFIX).o)
DEPS= $(OBJS:.o=.o.d)

LIBIW = -liw
LIBTIMER = -lrt
ifeq ($(DEBUG), 1)
LIBAIRKISS = -L./libak -lairkiss_log
else
LIBAIRKISS = -L./libak -lairkiss
endif
LIBCRC = -L./libcrc -lcrc
LIBS = -lpthread -lm -L$(HOST_ROOTFS)/usr/lib $(LIBAIRKISS) $(LIBIW) $(LIBTIMER) $(LIBCRC)

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

