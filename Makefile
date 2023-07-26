
# Put your stlink folder here so make burn will work.
STLINK=~/stlink.git

#SRCS=main.c system_stm32f4xx.c stm32f4xx_it.c
#SRCS=main.c system_stm32f4xx.c working-stm32f4xx_it.c

SRCS=main.c system_stm32f4xx.c util.c

# Library modules
SRCS += stm32f4xx_exti.c stm32f4xx_gpio.c stm32f4xx_rcc.c stm32f4xx_pwr.c stm32f4xx_syscfg.c
SRCS += stm32f4xx_tim.c misc.c 

# Add in stuff for SD access
SRCS += diskio.c  ff.c ffunicode.c stm32f4_discovery.c  stm32f4_discovery_sdio_sd.c system_stm32f4xx.c misc_handlers.c stm32f4xx_sdio.c stm32f4xx_dma.c

#SRCS += stm32f4_discovery.c

# Binaries will be generated with this name (.elf, .bin, .hex, etc)
PROJ_NAME=msx-rom-and-floppy-emulator

#######################################################################################

#STM_COMMON=../../..
# You need libs somewhere!
# I used the STM32F4DISCOVERY firmware at http://www.st.com/web/en/catalog/tools/PF257904# 
STM_COMMON=../STM32F4-Discovery_FW_V1.1.0

CC=arm-none-eabi-gcc
OBJCOPY=arm-none-eabi-objcopy

# NOTE: -mgeneral-regs-only does not work with gcc 7. I used gcc 9
#CFLAGS  =  -g -O2 -Wall -Tstm32_flash.ld -mno-unaligned-access -fsingle-precision-constant  -mgeneral-regs-only
#CFLAGS  =  -g -O2 -Wall -Tstm32_flash.ld -fsingle-precision-constant  -mgeneral-regs-only
## -ffunction-sections is meant to keep the literal pools closer to where they are referenced
CFLAGS  =  -g -O2 -Wall -Tstm32_flash.ld -ffunction-sections -fdata-sections -fomit-frame-pointer 
CFLAGS += -DUSE_STDPERIPH_DRIVER
#CFLAGS += -mlittle-endian -mthumb -mcpu=cortex-m4 -mthumb-interwork
CFLAGS += -mlittle-endian -mthumb -mcpu=cortex-m4 
CFLAGS += -mfloat-abi=hard -mfpu=fpv4-sp-d16
#CFLAGS += -mfloat-abi=softfp -mfpu=fpv4-sp-d16
#CFLAGS += -mfloat-abi=soft
CFLAGS += -I.
CFLAGS += --specs=nosys.specs
#CFLAGS += -specs=nano.specs -specs=rdimon.specs -lc -lrdimon
## Even though we specify these registers in main.c as global, still need to specify here to stop FATFS and anything else using though (though we luck out with any other libs linked in)
## This register list must match what is in defines.h
CFLAGS += -ffixed-s0 -ffixed-s1 -ffixed-s2 -ffixed-s3 -ffixed-s4 -ffixed-s5 -ffixed-s6 -ffixed-s7 -ffixed-s8 -ffixed-s9 -ffixed-s10 -ffixed-s11 -ffixed-s12 -ffixed-s13 -ffixed-s14 -ffixed-s15 -ffixed-s16 -ffixed-s17 -ffixed-s18 -ffixed-s19 -ffixed-s20 -ffixed-s21 -ffixed-s22 -ffixed-s23 -ffixed-s24 -ffixed-s25 -ffixed-s26 -ffixed-s27 -ffixed-s28 -ffixed-s29 -ffixed-s30 -ffixed-s31

CFLAGS += -ffixed-r10

#CFLAGS += -DENABLE_RTC_RAM_BANK_EMULATION
#CFLAGS += -DDEBUG_EXTI2_START
#CFLAGS += -DDEBUG_EXTI2_END
#CFLAGS += -DDEBUG_EXTI2_ROM_ACCESS
#CFLAGS += -DDEBUG_EXTI0_START
#CFLAGS += -DDEBUG_EXTI0_END
#CFLAGS += -DDEBUG_IO_00B4
#CFLAGS += -DDEBUG_IO_00B5
#CFLAGS += -DDEBUG_IO_00B5_READ
#CFLAGS += -DENABLE_SEMIHOSTING
#CFLAGS += -DDEBUG_FDC
#CFLAGS += -DDEBUG_FDC_STATUS_READ
#CFLAGS += -DENABLE_FDC_CCM_LOGGING
#CFLAGS += -DDEBUG_FILENAME
CFLAGS += -DDATABUS_READ_EXTRA_DELAY


# Include files from STM libraries
CFLAGS += -I$(STM_COMMON)/Utilities/STM32F4-Discovery
CFLAGS += -I$(STM_COMMON)/Libraries/CMSIS/Include 
CFLAGS += -I$(STM_COMMON)/Libraries/CMSIS/ST/STM32F4xx/Include
CFLAGS += -I$(STM_COMMON)/Libraries/STM32F4xx_StdPeriph_Driver/inc


# add startup file to build
SRCS += startup_stm32f4xx.s 
# You need to end asm files in capital S to get them to see preprocessor directives
SRCS += interrupt.S

OBJS = $(SRCS:.c=.o)

vpath %.c $(STM_COMMON)/Libraries/STM32F4xx_StdPeriph_Driver/src $(STM_COMMON)/Utilities/STM32F4-Discovery

.PHONY: proj

all: proj

proj: $(PROJ_NAME).elf

$(PROJ_NAME).elf: $(SRCS)
	$(CC) $(CFLAGS) $^ -o $@ 
	$(OBJCOPY) -O ihex $(PROJ_NAME).elf $(PROJ_NAME).hex
	$(OBJCOPY) -O binary $(PROJ_NAME).elf $(PROJ_NAME).bin

clean:
	rm -f *.o
	rm -f $(PROJ_NAME).elf
	rm -f $(PROJ_NAME).hex
	rm -f $(PROJ_NAME).bin


# Flash the STM32F4
#burn: proj
#	$(STLINK)/st-flash write $(PROJ_NAME).bin 0x8000000
