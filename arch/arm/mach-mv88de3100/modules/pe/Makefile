
# __LINUX_KERNEL__ is used in OS-independent header files
MV_DEFINE := -DBERLIN -DSoC_Galois -D__LINUX_KERNEL__ -D__CODE_LINK__=0 -DCPUINDEX=0
MV_DEFINE += -DBERLIN_B_0=0x1000 -DBERLIN_C_0=0x2000 -DBERLIN_BG2=0x3000 -DBERLIN_BG2_Z2=0x3010 -DBERLIN_BG2_A0=0x3020 -DBERLIN_BG2_CT=0x3050

# define BERLIN_CHIP_VERSION
ifeq ($(CONFIG_MV88DE3100_BG2_Z1),y)
        MV_DEFINE += -DBERLIN_CHIP_VERSION=BERLIN_BG2
endif
ifeq ($(CONFIG_MV88DE3100_BG2_Z2),y)
        MV_DEFINE += -DBERLIN_CHIP_VERSION=BERLIN_BG2_Z2
endif
ifeq ($(CONFIG_MV88DE3100_BG2_A0),y)
        MV_DEFINE += -DBERLIN_CHIP_VERSION=BERLIN_BG2_A0
endif
ifeq ($(CONFIG_MV88DE3100_BG2_CT),y)
        MV_DEFINE += -DBERLIN_CHIP_VERSION=BERLIN_BG2_CT
endif

HEADER_PATH +=  \
	-I$(srctree)/arch/arm/mach-mv88de3100/modules/shm \
	-I$(srctree)/arch/arm/mach-mv88de3100/modules/cc  \
	-I$(srctree)/arch/arm/mach-mv88de3100/modules/pe/gsinc

ccflags-y	+= $(MV_DEFINE) $(HEADER_PATH)

ccflags-y	+= -DBERLIN_SINGLE_CPU

obj-$(CONFIG_MV88DE3100_PE_MODULE) += galois_pe.o
galois_pe-objs := pe_driver.o avio_dhub_drv.o pe_agent_driver.o

