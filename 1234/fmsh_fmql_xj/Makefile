# Makefile - make rules for target/config/fmsh_fmql BSP
#
# Copyright (c) 2011, 2012 Wind River Systems, Inc.
# 
# The right to copy, distribute, modify or otherwise make use
# of this software may be licensed only pursuant to the terms
# of an applicable Wind River license agreement.
#
# modification history
# --------------------
# 010,11jul19,l_l  created.
#
# DESCRIPTION
# This file contains rules for building VxWorks for the FMSH FMQL A7x4 board
# fitted with an ARM Cotex-A7 MPCore module.
#
# INCLUDES
#     makeTarget

CPU           = ARMARCH7
TOOL          = diab

#EXTRA_DEFINE  = -DCPU_CORTEXA8 -DARMMMU=ARMMMU_CORTEXA8 \
#                -DARMCACHE=ARMCACHE_CORTEXA8
# jc
EXTRA_DEFINE  = -DCPU_CORTEXA8 -DARMMMU=ARMMMU_CORTEXA8 \
                -DARMCACHE=ARMCACHE_CORTEXA15
                
VFPVARIANT    = VFPv3

TGT_DIR = $(WIND_BASE)/target

include $(TGT_DIR)/h/make/defs.bsp

## Only redefine make definitions below this point, or your definitions will
## be overwritten by the makefile stubs above.

TARGET_DIR    = fmsh_fmql
VENDOR        = FMSH
BOARD         = FMSH_FMQL

RELEASE_CMD   += bootrom.bin

#
# The constants ROM_TEXT_ADRS and ROM_SIZE are defined
# in config.h and Makefile.
# All definitions for these constants must be identical.
#

ROM_TEXT_ADRS  = 0fc00000 # ROM entry address
ROM_SIZE       = 00200000 # number of bytes of ROM space

RAM_LOW_ADRS   = 00200000 # RAM text/data address
RAM_HIGH_ADRS  = 01000000 # RAM text/data address

VMA_START    = 0x$(ROM_TEXT_ADRS)

# Binary version of VxWorks ROM images, suitable for programming
# into Flash using tools provided by ARM.  If other ROM images need to
# be put into Flash, add similar rules here.

bootrom.bin: bootrom
	- @ $(RM) $@
	$(EXTRACT_BIN) -O binary bootrom $@

bootrom_res.bin: bootrom_res
	- @ $(RM) $@
	$(EXTRACT_BIN) -O binary bootrom_res $@

bootrom_uncmp.bin: bootrom_uncmp
	- @ $(RM) $@
	$(EXTRACT_BIN) -O binary bootrom_uncmp $@

vxWorks_rom.bin: vxWorks_rom
	- @ $(RM) $@
	$(EXTRACT_BIN) -O binary vxWorks_rom $@

vxWorks.st_rom.bin: vxWorks.st_rom
	- @ $(RM) $@
	$(EXTRACT_BIN) -O binary vxWorks.st_rom $@

vxWorks.res_rom.bin: vxWorks.res_rom
	- @ $(RM) $@
	$(EXTRACT_BIN) -O binary vxWorks.res_rom $@

vxWorks.res_rom_nosym.bin: vxWorks.res_rom_nosym
	- @ $(RM) $@
	$(EXTRACT_BIN) -O binary vxWorks.res_rom_nosym $@

## Only redefine make definitions above this point, or the expansion of
## makefile target dependencies may be incorrect.

include $(TGT_DIR)/h/make/rules.bsp
