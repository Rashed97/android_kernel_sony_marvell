# File: Makefile
# Copyright (C) 2007-2012, Marvell International Ltd.
#
# This software file (the "File") is distributed by Marvell International
# Ltd. under the terms of the GNU General Public License Version 2, June 1991
# (the "License").  You may use, redistribute and/or modify this File in
# accordance with the terms and conditions of the License, a copy of which
# is available by writing to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA or on the
# worldwide web at http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt.
#
# A copy of the GPL is available in file gpl-2.0.txt accompanying in this
# deliverables.
#
# THE FILE IS DISTRIBUTED AS-IS, WITHOUT WARRANTY OF ANY KIND, AND THE
# IMPLIED WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE
# ARE EXPRESSLY DISCLAIMED.  The License provides additional details about
# this warranty disclaimer.

#############################################################################
# Configuration Options
#############################################################################

# Debug Option
# DEBUG LEVEL n/1/2:
# n: NO DEBUG
# 1: PRINTM(MSG,...), PRINTM(FATAL,...), PRINTM(WARN,...) and PRINTM(INFO,...)
# 2: All PRINTM()
CONFIG_DEBUG=1

# SDIO suspend/resume
CONFIG_SDIO_SUSPEND_RESUME=y

#############################################################################
# Select Platform Tools
#############################################################################
MODEXT = ko
EXTRA_CFLAGS += -I$(srctree)/include
EXTRA_CFLAGS += -I$(srctree)/arch/arm/mach-mv88de3100/modules/bt_sd8787/bt
EXTRA_CFLAGS += -I$(srctree)/arch/arm/mach-mv88de3100/include/platform_setting/$(PLATFORM)

LD += -S

#############################################################################
# Compiler Flags
#############################################################################
	EXTRA_CFLAGS += -DFPNUM='"66"'

ifeq ($(CONFIG_DEBUG),1)
	EXTRA_CFLAGS += -DDEBUG_LEVEL1
endif

ifeq ($(CONFIG_DEBUG),2)
	EXTRA_CFLAGS += -DDEBUG_LEVEL1
	EXTRA_CFLAGS += -DDEBUG_LEVEL2
	DBG=	-dbg
endif

ifeq ($(CONFIG_SDIO_SUSPEND_RESUME),y)
	EXTRA_CFLAGS += -DSDIO_SUSPEND_RESUME
endif

#############################################################################
# Make Targets
#############################################################################

BTOBJS = bt/bt_main.o bt/bt_sdiommc.o bt/bt_proc.o
BTOBJS += bt/bt_init.o

obj-m := bt8787.o
bt8787-objs := $(BTOBJS)

# End of file;
