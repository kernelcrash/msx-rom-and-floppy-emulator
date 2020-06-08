msx-rom-and-floppy-emulator
===========================

What is it?
-----------

- Take a cheap STM32F407 board (US$10 or so). Wire it into an MSX cartridge slot..
- Make a FAT32 partition on a micro SD card and put rom and dsk images on it.
- Plug the micro SD into the STM32F4 board.
- The STM32F4 board presents a rom image in real time to the MSX/MSX2 computer such that it thinks a rom cartridge is attached.
- It also emulates a WD2793 floppy disk controller such that you can load disk images off the SD card.
- It also (optionally) partially emulates a RP5C01 RTC chip (enough of it to make an MSX2 computer boot if you don't have that chip). This is only really of use to me! Comment out the
 ENABLE_RTC_RAM_BANK_EMULATION line in the Makefile if you dont need this emulation.
- The code is still really 'proof of concept'. There is one button that allows you to cycle to the next rom or disk in a directory. Another button takes you to the previous rom or disk.

I used an Omega MSX2 computer to test this. I have not tested it on any other MSX or MSX2 computers.

Wiring it
---------

Using a STM32F407VET6 or STM32F407VGT6 board ('B' means the buffered pin on the slot connector)

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
   implemented Konami4 (ie. Konami without an SCC), Ascii8, Ascii16 and 
   what I would call 'generic' 16K and 32K carts (ie. no mapper)
 - You have to rename the ROMs to include the mapper type (yep, I don't have
   any fancy database lookup). So something like this won't work:

     Treasure-of-Usas.rom

   But this will:

     Treasure-of-usas.konami4

   So the rules are:

     - ends in .konami4 then the rom is treated as Konami4
     - ends in .ascii8  then the rom is treated as Ascii8
     - ends in .ascii16 then the rom is treated as Ascii16
     - anything else is treated as a generic 16 or 32K rom.

  - You are limited to 128KB carts. Bigger ones won't work as
    they are loaded to RAM on the stm32f407 board and it simply
    does not have enough RAM.

Disk images should end in .dsk and should be either 737280 or 368640 bytes
in size.

In order to load a disk, you need a diskrom. Put one disk rom in the root
of the SD card called 'disk.rom. I have only tested with the NMS8250 disk rom with MD5
0ed6dbd654da55b56dfb331dd3df82f0 . 

For the WD2793 floppy disk controller support:

 - Not all the chip is implemented. But most of the stuff you want is.
 - It pretends to be only one drive
 - I haven't worked out how to make it think the disk has changed after a warm
   boot, so you need to power off /on to get a new image to load. However, in
   a multi-disk game it generally works OK if you press NEXT while the computer 
   is running.

Copying the firmware to the stm32f407 board
-------------------------------------------

There are lots of options for writing firmware with the stm32 chips. There is 
an example of using dfu-util in the transfer.sh script. In order for this to 
work you would need to set the BOOT0 and or BOOT1 settings such that plugging
the board in via usb will show a DFU device. Then you can run transfer.sh. Remove
the BOOT0 or BOOT1 jumpers after you do this.


Technical
---------

PC2 will interrupt on the -ve edge of _MREQ. I've used a different technique in the past,
such that the _MREQ int is on the +ve edge. That former technique gives better timing
outcomes for the _MREQ processing at the expense of reducing the time available to the
main while() loop. MSX adds wait states such that an M1 is low for 560ns and a regular
memory access is 690ns. On the stm32f407 @ 240MHz, its around 100ns best case until the
first line of the interrupt handler, so we are not as pressed for time as some other Z80A
systems. ie. we can 'get away' with the interrupt on the -ve edge.

The interrupt handler makes heavy use of floating point registers that are used as
general purpose global constants and pointers. This means the interrupt handler does
not need to push any registers (On ARM, r0-r3 and r12 are automatically pushed 
whether you like it or not though). The fpu registers are effectively used as high speed RAM. 
Here's an example;

   // grab the pointer to GPIOA from s11
   vmov	r1,s11
   // s1 is preset to the value '1'. Send PA0 high
   vstr s1,[r1,ODR]

There are thirty two 32 bit fpu registers (s0 to s31). I have used most of them, though
I noticed that gcc tends to want to do weird things with s16, s17, s18 and s19 (d8 and d9),
so I tend to avoid those.

As far as the decision process for an _MREQ int;

 - work out if its a false int. Sometimes this happens as edges of signals from the Z80A
   can appear a bit rough to the stm32f4
 - see if the _SlotSelect signal is low. If its not then exit.
 - Next we need to figure out if we're simply emulating a normal ROM cart, or whether
   we're emulating a disk rom, and will be using disk images. A the moment its one or the
   other. If you want disks then you can't have other carts as the only ROM being emulated
   will be the disk ROM (which is a basic 16k ROM).
 - So if its a disk ROM, then we emulate the 16K disk ROM, and also enable access to the 
   emulated floppy disk controller at 0x7ff8-0x7fff (replcated at 0xbff8-0xbfff)



