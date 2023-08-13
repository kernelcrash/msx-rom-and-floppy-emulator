msx-rom-and-floppy-emulator
===========================

More info at https://www.kernelcrash.com/blog/emulating-roms-and-floppy-drives-in-msx/

EXPERIMENTAL OLED DISPLAY EXAMPLE
=================================

This branch is meant to be an example of how you might incorporate some OLED display support

- I used one of those 128x64 SSD 1306 OLED screens with an I2C interface
- You need to wire VCC to +3.3V and GND to GND
- Wire SCL to PB10 and SDA to PB11
- Look for all the references to ENABLE_OLED_DISPLAY. In the Makefile there are some extra source files and the CFLAGS includes ENABLE_OLED_DISPLAY
- There are some points in main.c where the filename of the file loaded from SD is known. This is used as a starting point for what to display
- I just used some example code from https://stm32f4-discovery.net/2015/05/library-61-ssd1306-oled-i2c-lcd-for-stm32f4xx/. Most of the functions in oled.c and it2.c come from this example. It's been reduced to down to the minimum required. ie. there is only one font and some methods like drawing cricles are not included. 
- Two lines of text printed to the OLED display. Due to the font used you can only do about 17 characters per line. 
- As a lot of filenames are longer than 34 characters (2x17), some 'mangling' of the filename is done so that the disk number is shown at the very end of the 2nd line as #1 or #2 etc.



Overview
--------

- Take a cheap STM32F407 board (US$10 or so). Wire it into an MSX cartridge slot..
- Make a FAT32 partition on a micro SD card and put rom and dsk images on it.
- Plug the micro SD into the STM32F4 board.
- The STM32F4 board presents a rom image in real time to the MSX/MSX2 computer such that it thinks a rom cartridge is attached.
- It also emulates a WD2793 floppy disk controller such that you can load disk images off the SD card. Two floppy drives are supported; A: and B:
- It also (optionally) partially emulates a RP5C01 RTC chip (enough of it to make an MSX2 computer boot if you don't have that chip). This is only really of use to me! Comment out the
 ENABLE_RTC_RAM_BANK_EMULATION line in the Makefile if you dont need this emulation.
- The code is still really 'proof of concept'. There is one button that allows you to cycle to the next rom or disk in a directory. Another button takes you to the previous rom or disk. However, I've added a simple boot menu mechanism where an MSX native program (kcmfs) presents a menu of files from the SD card and lets you boot from one of them.

I used an Omega MSX2 computer to test this. I have not tested it on any other MSX or MSX2 computers.

Wiring it
---------

Using a STM32F407VET6 or STM32F407VGT6 board ('B' means the buffered pin on the slot connector)
```
   PA1         - wire a button between it and GND. No need for a pullup. This is 'NEXT'
   PA2         - wire a button between it and GND. No need for a pullup. This is 'PREV'
   PE0 to PE15 - BA0 to BA15
   PD8 to PD15 - BD0 to BD7
   PC0         - _B_IORQ
   PC1         - _SLTSL1 or SLTSL2
   PC2         - _B_MREQ

   PC3         - _B_RD
   PC4         - _BUSDIR1 or 2 (only required if you are emulating the RP5C01 chip)

   GND         - GND
```
If you get a board with a microSD card slot, then this 'standard' wiring of the SD adapter
is fine.

I realise this is quite difficult as MSX has an edge connector socket, so you need to
either solder wires directly to the slot pins, or make up some kind of adapter with
an edge connector on it. I got around this by not soldering an edge connector at all 
in to my Omega MSX2 computer

Setting up the micro SD card
----------------------------

I tend to format a micro SD card with a smallish partition (less than 1GB) with 
FAT32. Create an msx directory in the root and add rom images and disk images 
to that directory. The order in which you copy the files to this directory
determines the order in which you can cycle through them (ie. its not 
alphabetical). 

Some limitations:

 - MSX has lots of different 'megarom mappers' in cartridges. I have only 
   implemented Konami4 (ie. Konami without an SCC), Konami5 (again without the SCC),
   Ascii8, Ascii16 and what I would call 'generic' 16K and 32K carts (ie. no mapper)
 - You have to rename the ROMs to include the mapper type (yep, I don't have
   any fancy database lookup). So something like this won't work:

     Treasure-of-Usas.rom

   But this will:

     Treasure-of-usas.konami4

   So the rules are:

     - ends in .konami4 then the rom is treated as Konami4
     - ends in .konami5 then the rom is treated as Konami5 (though there is no SCC emulated. This format is purely for carts that are in the KonamiWithSCC format and can operate with a PSG)
     - ends in .ascii8  then the rom is treated as Ascii8
     - ends in .ascii16 then the rom is treated as Ascii16
     - anything else is treated as a generic 16 or 32K rom.

  - You are limited to 128KB carts. Bigger ones won't work as
    they are loaded to RAM on the stm32f407 board and it simply
    does not have enough RAM.

Disk images should end in .dsk and should be either 737280 or 368640 bytes
in size. Normally you just copy .dsk images to the msx directory on the SD card.
You can mix these freely with the rom images.

However, the two drive support allows for some other options. If you 
create a subdirectory under the msx directory instead of just plonking a .dsk file
in, the subdirectory is treated as a collection of disk images. The subdirectory can
be called anything you like. Its name will show up in the kcmfs menu. If say you have a 
game and it has 'Game disk 1.dsk' and 'Game disk 2.dsk' then just copy them with their
original names into the subdirectory you made. Now when you select that subdirectory
from kcmfs, it will auto load the 'Game disk 1.dsk' into the A: drive as it is looking for
the text 'disk 1' or 'Disk 1' or 'disk1' or 'Disk1' in the text of the filename. The 2nd disk
'Game disk 2.dsk' isn't actually loaded in to the B: drive. I found most msx games don't seem
to want to look at the B: drive. Anyway, when the game asks for the 2nd disk, hit the NEXT button
and it will put 'Game disk 2.dsk' into the A: drive. Pressing PREV will switch it back to 
'Game Disk 1.dsk' . If you actually want to put something in to the B: drive it just has to be a 
file ending in '.dsk2'. If you had a 2 disk game and you had 'Game disk 2.dsk' in the A: drive, 
and then pressed NEXT, it will actually pop you out of the subdirectory and select the next
thing in the main list you see in kcmfs.

In order to use disks at all, you need a diskrom. Put one disk rom in the root
of the SD card called 'disk.rom'. I have only tested with the NMS8250 disk rom with MD5
0ed6dbd654da55b56dfb331dd3df82f0 . 

The WD2793 floppy disk controller support was based on WD1793.c in fMSX by Marat Fayzullin. Note that:

 - Not all the chip is implemented. But most of the stuff you want is.
 - It pretends to be only one drive
 - I haven't worked out how to make it think the disk has changed after a warm
   boot, so you need to power off /on to get a new image to load. However, in
   a multi-disk game it generally works OK if you press NEXT while the computer 
   is running.

There is also now a native MSX program (kcmfs) that presents a list of files on the
SD card, lets you select one and boot it. To use kcmfs you need to copy the menu.rom
file to the root of the SD card. After a reset of the STM32F4 board it will load
menu.rom instead of the first file in the msx directory. See the section on KCMFS 
for more information.

Here's an example of a mix of games on the SD card
```
  msx/SomeSimple32K.rom
  msx/AKomani4Rom.konami4
  msx/somegame.dsk
  msx/AKonami5Rom.konmai5
  msx/SomeGameDirectory
  msx/SomeGameDirectory/Blah Disk 1.dsk
  msx/SomeGameDirectory/Blah Disk 2.dsk
  msx/AnAscii8Rom.ascii8
  msx/SomeOtherGameDirectory
  msx/SomeOtherGameDirectory/Thing Disk1.dsk
  msx/SomeOtherGameDirectory/Thing Disk2.dsk
  msx/SomeOtherGameDirectory/Thing Disk3.dsk
  msx/SomeOtherGameDirectory/Thing Disk4.dsk
  msx/someinfocomgame
  msx/someinfocomgame/game.dsk
  msx/someinfocomgame/savedisk.dsk2
```

Compiling the firmware
----------------------

Normally I use the ARM GNU Toolchain from developer.arm.com, but on modern linux distros you can actually
use the packaged up ARM cross compiler (for example on debian/ubuntu you might 'apt install gcc-arm-none-eabi').
I only use linux for compiling, so have no idea how to do it on other platforms.

You will need the STM32F4DISCOVERY board firmware package from st.com. This is called STSW-STM32068. You need to
extract it to the directory 'above' where you git cloned this repo. eg. on some folder you would see
```
   ./STM32F4-Discovery_FW_V1.1.0
   ./msx-rom-and-floppy-emulator
```
To build
```
   cd msx-rom-and-floppy-emulator
   make
```
You should end up with a hex and bin file that you can then flash to the stm32f4 board. There's 
an example below re using USB DFU mode to transfer it.

Copying the firmware to the stm32f407 board
-------------------------------------------

There are lots of options for writing firmware with the stm32 chips. There is 
an example of using dfu-util in the transfer.sh script. In order for this to 
work you would need to set the BOOT0 and or BOOT1 settings such that plugging
the board in via usb will show a DFU device. Then you can run transfer.sh. Remove
the BOOT0 or BOOT1 jumpers after you do this.

If you are having trouble getting the board to appear as a DFU device, you can try
putting a pull down resistor between PA10 and GND (some value between 1K and 10K will
probably work).

KCMFS
-----

In the kcmfs directory is an sdcc based MSX program that presents a menu to
the user. It lists the files in the msx directory of the SD card and let's 
you select one, and consequently reboots off it. If you put kcmfs (menu.rom) in
the root of the SD card, it will always boot menu.rom  after a reset of the
STM32F4 board.

kcmfs is about as simple as I could go. No fancy graphics. On boot it
shows the first 20 files in the msx directory of the SD card. They are listed
with 'a' to 't' down the left hand side. You press a letter and your MSX
computer should reboot and start up what you selected. Assuming there are 
more than 20 files in the msx directory, you just press '2' for the 2nd
page of 20 files, '3' for the 3rd and so on. You can also press '?' to get
some help.

There is an ultra simple protocol used;

 - kcmfs writes an 0x80 to 0x8000. That triggers the main thread of the 
   smt32f4 board to do a directory listing of the msx directory. The 
   first filename in the direcory is written so that the MSX computer will
   see it at 0x8100, the 2nd file appears at 0x8180 and so on (ie. 128 byte's
   per filename)
 - kcmfs polls 0x8000 to see when bit 7 goes low. That means the file listing
   process has completed. The number of files retrieved is written to 0x8002.
   It can't be more than 126 or so, so its always a byte length, however I
   write a 0x00 to 0x8003 so that it can be used as a 16 bit int.
 - kcmfs lets a user select a file. The filename selected is copied in to
   0x8080 - 0x80ff. Then a 0x40 is written to 0x8000. That triggers the 
   main thread of the stm32f4 board to load the appropriate rom or disk.
   As soon as the 0x40 is written, the code triggers a reset of the MSX 
   computer. Technically I should be running the code to write the 0x40
   out of RAM , not slot memory since the loading of a new ROM will overwrite
   the menu.rom that is currently loaded. However, there is enough of a 
   delay between the write of the 0x40 and a ROM being loaded , that it
   does not seem to currently cause a problem.

   One other thing to note with the loading of a new ROM, is that the main
   thread actually steps through the msx directory (as if you were pressing 
   NEXT again and again). This seems silly, but the point is to allow 2 or
   3 disks games to work. ie. press NEXT in a game when it asks for the next
   disk. This is all dependent on you copying files to the SD card in the 
   right order.


Technical
---------

PC2 will interrupt on the -ve edge of _MREQ. I've used a different technique in the past,
such that the _MREQ int is on the +ve edge. That former technique gives better timing
outcomes for the _MREQ processing at the expense of reducing the time available to the
main while() loop. MSX adds wait states such that an M1 is low for 560ns and a regular
memory access is 690ns. On the stm32f407 @ 240MHz, its around 100ns best case until the
first line of the interrupt handler, so we are not as pressed for time as some other Z80A
systems. ie. we can 'get away' with the interrupt on the -ve edge. Note though that the 
interrupt handling code is written in ARM assembly.

The interrupt handler makes heavy use of floating point registers that are used as
general purpose global constants and pointers. This means the interrupt handler does
not need to push any registers (On ARM, r0-r3 and r12 are automatically pushed 
whether you like it or not though). The fpu registers are effectively used as high speed RAM. 
Here's an example;
```
   // grab the pointer to GPIOA from s11
   vmov	r1,s11
   // s1 is preset to the value '1'. Send PA0 high
   vstr s1,[r1,ODR]
```
There are thirty two 32 bit fpu registers (s0 to s31). I have used most of them, though
I noticed that gcc tends to want to do weird things with s16, s17, s18 and s19 (d8 and d9),
so I tend to avoid those.

As far as the decision process for an _MREQ int;

 - work out if its a false int. Sometimes this happens as edges of signals from the Z80A
   can appear a bit rough to the stm32f4
 - see if the _SlotSelect signal is low. If its not then exit.
 - Next we need to figure out if we're simply emulating a normal ROM cart, or whether
   we're emulating a disk rom, and will be using disk images. At the moment its one or the
   other. If you want disks then you can't have other carts as the only ROM being emulated
   will be the disk ROM (which is a basic 16k ROM).
 - So if its a disk ROM, then we emulate the 16K disk ROM, and also enable access to the 
   emulated floppy disk controller at 0x7ff8-0x7fff (replicated at 0xbff8-0xbfff)
 - If its a ROM cartridge, then we need to quickly translate any 'Mapper' bank selection
   such that the appropriate 8K block is presented. Conversely a 'write' allows the ROM
   to write to the Mapper bank registers (which differ depending on the Mapper type).



