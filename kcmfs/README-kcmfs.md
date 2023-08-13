KCMFS
=====

Native MSX based file selector that interacts with the kernelcrash msx-rom-and-floppy
emulator to let you choose rom or disk images

Usage
=====

Reset the stm32f4 board. It should load the menu.rom from the root of the SD card.

You get a file listing of whatever is in the msx directory of the SD card.

You see 20 files at a time. Press the number keys to see more (ie. press 2 for
the 2nd page, 3 for the 3rd page and so on).

Press a letter key to select a file and reboot the MSX system from what you 
selected.

Press ? if you want a brief help screen.

Assembly and Setup
==================

Requires asmz80 (part of asmx)  and srec_cat to build. 


$ ./build.sh menu.asm
blah/asmx/asmz80
blah/bin/srec_cat
Pass 1
Pass 2

00000 Total Error(s)

srec_cat: menu.asm.s37: 1: warning: no header record
srec_cat: menu.asm.s37: 26: warning: no execution start address record


That will produce a ROM image

Mount your SD card 

Copy menu.rom to the root of the SD card

Create an msx directory on the SD card. Copy rom and disk images to the msx
folder on the SD card

Limitations
===========

This is a very simple implementation. Currently you are limited to about 120 files
in the msx directory. Filenames must be less than 127 chars.
