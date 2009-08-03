#*****************************************************************************
#
# Linux driver for Easylon ISA/PC-104/PCI Interfaces
#
# Copyright (c)2004 Gesytec GmbH, Volker Schober (info@gesytec.de)
# Licensend for use with Easylon ISA/PC-104/PCI Interfaces by Gesytec
#
#*****************************************************************************
 
KERNEL_VERSION = $(shell uname -r)
KERNEL_SOURCE ?= /lib/modules/$(KERNEL_VERSION)/build
KERNEL_HEADERS = $(KERNEL_SOURCE)/include

# define a unique major number for the character device (see /proc/devices)
EXTRA_CFLAGS += -DLPC_MAJOR=84

# remove "#" from line below if you want to enable debug messages
# EXTRA_CFLAGS += -DCONFIG_LPC_DEBUG


# if Rules.make exists in the kernel tree, we assume 2.4 style modules
# if it doesn't assume 2.5 / 2.6 style
OLDMAKE = $(wildcard $(KERNEL_SOURCE)/Rules.make)

ifeq (,$(OLDMAKE))
#*****************************************************************************
# 2.5 / 2.6 style modules, get the kernel makefiles to do the work
#*****************************************************************************

all: modules
 
obj-m	:= lpclinux.o

# Set to something different to install somewhere else:
# MOD_DIR := extra



.PHONY: modules install clean modules_add modules_install

#install : modules_add
install:
	@if [ ! -c "/dev/lon1" ]; then mknod /dev/lon1 c 84 0;	fi;	# (lon1 is the 1st ISA card)
	@if [ ! -c "/dev/lon2" ]; then mknod /dev/lon2 c 84 1;	fi;	# (lon2 is the 2nd ISA card)
	@if [ ! -c "/dev/lon3" ]; then mknod /dev/lon3 c 84 32;	fi;	# (lon3 is the 1st PCI card)
	@if [ ! -c "/dev/lon4" ]; then mknod /dev/lon4 c 84 33;	fi;	# (lon4 is the 2nd PCI card)
	@chmod 666 /dev/lon*
	@ls -lF /dev/lon*
	@echo
	$(MAKE) -C $(KERNEL_SOURCE) SUBDIRS=$(CURDIR) modules_install


modules modules_add modules_install clean:
	$(MAKE) -C $(KERNEL_SOURCE) $@ SUBDIRS=$(CURDIR)


else  # $(OLDMAKE)
#*****************************************************************************
# 2.4 style modules
#*****************************************************************************

#INCLUDEDIR = /usr/src/linux-$(KERNEL_VERSION)/include
#INCLUDEDIR = /usr/src/linux/include
#INCLUDEDIR = /usr/src/linux-2.4/include
ifneq (,$(wildcard /usr/src/linux/include))
	INCLUDEDIR = /usr/src/linux/include
else
	INCLUDEDIR = /usr/src/linux-2.4/include
endif
INCLUDEDIR = /lib/modules/$(KERNEL_VERSION)/build/include

CC = gcc
LD = ld
SHELL = sh
TARGET = lpclinux.o
CFILES = driverdata.c driverentry.c lpclinux.c #dynbuf.c 
HFILES = Makefile kdebug.h lpclinux.h #dynbuf.h

CFLAGS = -D__KERNEL__ -DMODULE -DLINUX -DEXPORT_SYMTAB
CFLAGS += $(EXTRA_CFLAGS)

# remove "#" from line below if kernel compiled with SMP support
# CFLAGS += -D__SMP__


CFLAGS += -I$(INCLUDEDIR) -I.

# check for module versioning configured in the kernel sources
CFLAGS += $(shell [ -f $(INCLUDEDIR)/linux/modversions.h ] && \
            echo -DMODVERSIONS -include $(INCLUDEDIR)/linux/modversions.h)
CFLAGS += -O2 -pipe

ifneq (,$(wildcard /lib/modules/$(KERNEL_VERSION)/kernel))
	INSTALLDIR = /lib/modules/$(KERNEL_VERSION)/kernel/drivers/pci
else
	INSTALLDIR = /lib/modules/$(KERNEL_VERSION)/pci
endif

all: modules

modules: lpclinux.o

lpclinux.o: $(CFILES) $(HFILES)
#	$(CC) $(CFLAGS) -c lpclinux.c -o lpclinux_.o 2>&1 | grep -v "warning: concatenation of string literals with __FUNCTION__ is deprecated"
#	$(CC) $(CFLAGS) -c lpclinux.c -o lpclinux.o
	$(CC) $(CFLAGS) -c lpclinux.c -o lpclinux_.o
	$(LD) -r -o lpclinux.o lpclinux_.o
	rm lpclinux_.o

install: lpclinux.o
	@if [ ! -c "/dev/lon1" ]; then mknod /dev/lon1 c 84 0;	fi;	# (lon1 is the 1st ISA card)
	@if [ ! -c "/dev/lon2" ]; then mknod /dev/lon2 c 84 1;	fi;	# (lon2 is the 2nd ISA card)
	@if [ ! -c "/dev/lon3" ]; then mknod /dev/lon3 c 84 32;	fi;	# (lon3 is the 1st PCI card)
	@if [ ! -c "/dev/lon4" ]; then mknod /dev/lon4 c 84 33;	fi;	# (lon4 is the 2nd PCI card)
	@chmod 666 /dev/lon*
	@ls -lF /dev/lon*
	@echo
	@echo INSTALLDIR=$(INSTALLDIR)
	mkdir -p $(INSTALLDIR)
	install -m 644 lpclinux.o $(INSTALLDIR)
	depmod -a -e

uninstall:
	$(SHELL) -c 'if [ -f $(INSTALLDIR)/lpclinux.o ]; then \
			rm $(INSTALLDIR)/lpclinux.o; \
	             fi'

clean:
	rm -f *.o core

endif  # $(OLDMAKE)
