#
# File : mlanconfig/Makefile
#
# Copyright (C) 2008-2011, Marvell International Ltd. All Rights Reserved

# Path to the top directory of the mlandriver distribution
PATH_TO_TOP = ../..

# Determine how we should copy things to the install directory
ABSPATH := $(filter /%, $(INSTALLDIR))
RELPATH := $(filter-out /%, $(INSTALLDIR))
INSTALLPATH := $(ABSPATH)
ifeq ($(strip $(INSTALLPATH)),)
INSTALLPATH := $(PATH_TO_TOP)/$(RELPATH)
endif

# Override CFLAGS for application sources, remove __ kernel namespace defines
CFLAGS := $(filter-out -D__%, $(EXTRA_CFLAGS))

#
# List of application executables to create
#
libobjs:= mlanconfig.o mlanhostcmd.o mlanmisc.o
exectarget=mlanconfig
TARGETS := $(exectarget)

#
# Make target rules
#

# All rule compiles list of TARGETS using builtin program target from src rule
all :
$(exectarget): $(libobjs)
	$(CC) $(CFLAGS) $(libobjs) -o $(exectarget)

# Update any needed TARGETS and then copy to the install path
build install: $(TARGETS)
	@cp -f $(exectarget) $(INSTALLPATH)
	@cp -rf config $(INSTALLPATH)

clean:
	@rm -f $(exectarget)
	@rm -f *.o

distclean: clean
	@rm -f *~ core
	@rm -f tags
