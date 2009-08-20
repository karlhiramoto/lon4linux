#*****************************************************************************
#
# Linux driver for Easylon ISA/PC-104/PCI Interfaces
#
# Copyright (c)2004 Gesytec GmbH, Volker Schober (info@gesytec.de)
# Licensend for use with Easylon ISA/PC-104/PCI Interfaces by Gesytec
#
#*****************************************************************************
 
MODULE=lpclinux
KERNEL_VERSION = $(shell uname -r)
KERNEL_SOURCE ?= /lib/modules/$(KERNEL_VERSION)/build
KERNEL_HEADERS = $(KERNEL_SOURCE)/include
UDEV_RULES_PATH = /etc/udev/rules.d
UDEV_RULES = 25-easylon.rules

# define a unique major number for the character device (see /proc/devices)
EXTRA_CFLAGS += -DLPC_MAJOR=84

# remove "#" from line below if you want to enable debug messages
# EXTRA_CFLAGS += -DCONFIG_LPC_DEBUG

# remove "#" from line below if you want only a ISA/PC-104 driver
# EXTRA_CFLAGS += -DNO_PCI

# use deferred interrupt handling
EXTRA_CFLAGS += -DUSE_TASKLET 

# if Rules.make exists in the kernel tree, we assume 2.4 style modules
# if it doesn't assume 2.5 / 2.6 style
OLDMAKE = $(findstring 2.4,$(KERNEL_VERSION))

ifeq (,$(OLDMAKE))
#*****************************************************************************
# 2.5 / 2.6 style modules, get the kernel makefiles to do the work
#*****************************************************************************

all: modules
 
obj-m	:= $(MODULE).o

# Set to something different to install somewhere else:
# MOD_DIR := extra



.PHONY: help debug modules install clean modules_add modules_install

debug: modules
	-modprobe -r $(MODULE)
	insmod $(MODULE).ko debug=0xFF

#install : modules_add
install:
#	@if [ ! -c "/dev/lon1" ]; then mknod /dev/lon1 c 84 0;	fi;	# (lon1 is the 1st ISA card)
#	@if [ ! -c "/dev/lon2" ]; then mknod /dev/lon2 c 84 1;	fi;	# (lon2 is the 2nd ISA card)
#	@if [ ! -c "/dev/lon3" ]; then mknod /dev/lon3 c 84 32;	fi;	# (lon3 is the 1st PCI card)
#	@if [ ! -c "/dev/lon4" ]; then mknod /dev/lon4 c 84 33;	fi;	# (lon4 is the 2nd PCI card)
#	@chmod 666 /dev/lon*
	@-ls -lF /dev/lon*
	@echo
	$(MAKE) -C $(KERNEL_SOURCE) SUBDIRS=$(CURDIR) modules_install
	@if ! `cmp -s ./$(UDEV_RULES) $(UDEV_RULES_PATH)/$(UDEV_RULES)` ;  then cp -vi $(UDEV_RULES) $(UDEV_RULES_PATH)/; fi
	-modprobe -r $(MODULE)
	depmod -a -e
	modprobe $(MODULE) debug=0xFF


modules modules_add modules_install help:
	$(MAKE) -C $(KERNEL_SOURCE) $@ SUBDIRS=$(CURDIR)

clean:
	$(MAKE) -C $(KERNEL_SOURCE) $@ SUBDIRS=$(CURDIR)
	-rm -f lpctest

modules_uninstall:
	-modprobe -r $(MODULE)

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

SHELL = sh
TARGET = $(MODULE).o
CFILES = driverdata.c driverentry.c $(MODULE).c #dynbuf.c 
HFILES = Makefile $(MODULE).h sysdep.h kdebug.h #dynbuf.h

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

debug: modules
	-modprobe -r $(MODULE)
	insmod $(MODULE).o debug=0xFF

modules: $(MODULE).o

$(MODULE).o: $(CFILES) $(HFILES)
#	$(CC) $(CFLAGS) -c $(MODULE).c -o $(MODULE)_.o 2>&1 | grep -v "warning: concatenation of string literals with __FUNCTION__ is deprecated"
#	$(CC) $(CFLAGS) -c $(MODULE).c -o $(MODULE).o
	$(CC) $(CFLAGS) -c $(MODULE).c -o $(MODULE)_.o
	$(LD) -r -o $(MODULE).o $(MODULE)_.o
	rm $(MODULE)_.o

install: $(MODULE).o
	@if [ ! -c "/dev/lon1" ]; then mknod /dev/lon1 c 84 0;	fi;	# (lon1 is the 1st ISA card)
	@if [ ! -c "/dev/lon2" ]; then mknod /dev/lon2 c 84 1;	fi;	# (lon2 is the 2nd ISA card)
	@if [ ! -c "/dev/lon3" ]; then mknod /dev/lon3 c 84 32;	fi;	# (lon3 is the 1st PCI card)
	@if [ ! -c "/dev/lon4" ]; then mknod /dev/lon4 c 84 33;	fi;	# (lon4 is the 2nd PCI card)
	@chmod 666 /dev/lon*
	@ls -lF /dev/lon*
	@echo
	@echo INSTALLDIR=$(INSTALLDIR)
	mkdir -p $(INSTALLDIR)
	install -m 644 $(MODULE).o $(INSTALLDIR)
	-modprobe -r $(MODULE)
	depmod -a -e
	modprobe $(MODULE) debug=0xFF

uninstall:
	-modprobe -r $(MODULE)
	$(SHELL) -c 'if [ -f $(INSTALLDIR)/$(MODULE).o ]; then \
			rm $(INSTALLDIR)/$(MODULE).o; \
	             fi'

clean:
	-rm -f *.o core
	-rm -f lpctest

endif  # $(OLDMAKE)


##################################################################
#
# Userland tools
#
##################################################################

#CFLAGS += -Wall -O2 -pipe

lpctest: lpctest.c
	gcc -o lpctest $^

tools: lpctest



test:
	@echo INSTALLDIR=$(INSTALLDIR)
	@echo OLDMAKE=$(OLDMAKE)
	@echo KERNEL_SOURCE=$(KERNEL_SOURCE)
	@echo KERNEL_VERSION=$(KERNEL_VERSION)
