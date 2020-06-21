CC = /usr/libexec/sdcc/sdcc
ASM = /usr/libexec/sdcc/sdasz80
PLATFORM = -mz80
EMULATOR = /Applications/openMSX.app/Contents/MacOS/openmsx -machine Philips_NMS_8255 -ext msxdos2 -diska emulation/msx-dos2/ -script emulation/boot.tcl -diskb bin/
#EMULATOR = /Applications/openMSX.app/Contents/MacOS/openmsx -machine C-BIOS_MSX2 -carta
HEXBIN = hex2bin

STARTUPDIR = startups
INCLUDEDIR = includes
LIBDIR = libs
SRCDIR = src

# See startup files for the correct ADDR_CODE and ADDR_DATA
CRT0 = crt0msx.32k.4000.s
ADDR_CODE = 0x4020
ADDR_DATA = 0xc000

VERBOSE = -V
CCFLAGS = $(VERBOSE) $(PLATFORM) --code-loc $(ADDR_CODE) --data-loc $(ADDR_DATA) \
          --no-std-crt0 --opt-code-size --out-fmt-ihx
OBJECTS = $(CRT0)
SOURCES = menu.c
ROMFILE = menu.rom

.PHONY: all compile build clean emulator

all: clean compile build

compile: $(OBJECTS) $(SOURCES)

$(CRT0):
		@echo "Compiling $(CRT0)"
		@$(ASM) -o $(notdir $(@:.s=.rel)) $(STARTUPDIR)/$(CRT0)
%.s:
		@echo "Compiling $@"
		@[ -f $(LIBDIR)/$@ ] && $(ASM) -o $(notdir $(@:.s=.rel)) $(LIBDIR)/$@ || true
		@[ -f $(SRCDIR)/$@ ] && $(ASM) -o $(notdir $(@:.s=.rel)) $(SRCDIR)/$@ || true
%.c:
		@echo "Compiling $@"
		@[ -f $(LIBDIR)/$@ ] && $(CC) $(VERBOSE) $(PLATFORM) -I$(INCLUDEDIR) -c -o $(notdir $(@:.c=.rel)) $(LIBDIR)/$@ || true
		@[ -f $(SRCDIR)/$@ ] && $(CC) $(VERBOSE) $(PLATFORM) -I$(INCLUDEDIR) -c -o $(notdir $(@:.c=.rel)) $(SRCDIR)/$@ || true

$(SOURCES):
		$(CC) -I$(INCLUDEDIR) $(CCFLAGS) \
				$(addsuffix .rel, $(basename $(notdir $(OBJECTS)))) \
				$(SRCDIR)/$(SOURCES)

build: menu.ihx
		@echo "Building $(OUTFILE)..."
		@$(HEXBIN) menu.ihx
		@dd if=menu.bin of=${ROMFILE} bs=1 count=32768
		@echo "Done."

clean:
		@echo "Cleaning ...."
		rm -f *.asm *.bin *.cdb *.ihx *.lk *.lst *.map *.mem *.omf *.rst *.rel *.sym *.noi *.hex *.lnk *.dep
		rm -f ${ROMFILE}

emulator: $(OUTFILE)
		$(EMULATOR) &
		#For ROM use:
		#$(EMULATOR) $(OUTFILE) &
