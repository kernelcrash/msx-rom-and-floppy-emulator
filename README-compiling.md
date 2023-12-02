Compiling the firmware
======================

There are multiple ways of compiling the firmware. If you are on linux then
it is relatively easy. There are pre-requisites mentioned in the README files.

If you are on other platforms like Mac or Window you have plenty of options as
well. You could

- Run virtualisation and install Debian or Ubuntu into a virtual machine
then compile it inside the virtual machine.
- If you are on Windows, you could download a 'live USB' for Debian
and  reboot your PC off the live USB. Because it's  a 'live USB' you don't
have to install anything to your hard drive. You just download the 
pre-requisites and compile the firmware while its running in RAM.

But perhaps the easiest way, which will work much the same on Mac,
Windows or Linux is to use docker to build the firmware. On linux
I am just using docker-ce. I think on Windows and Mac you need to 
install 'Docker Desktop' first.

But regardless you first need to download a zip file from st.com. I 
won't put a link as it may change over time. It's called the 
"STM32F4DISCOVERY board firmware package". It should have the name STSW-STM32068
 (you can probably search for it on st.com). You should end up with a file called
 en.stsw-stm32068.zip with an MD5 sum of 6063f18dff8b5f1ddfafa77d1ab72ad9. 

Then you do something like this

```
mkdir work
cd work
## Get the en.stsw-stm32068.zip file into the the current directory
## (ie. the work directory that was just created)

## This is how I run a 'docker container' on linux. The expectation is that
## you would do the same from some sort of terminal or command prompt on
## Windows or Mac. On some versions of docker you might have to change this
## line to    docker run -v ${PWD}:/work -it debian:bullseye-slim

docker run -v .:/work -it debian:bullseye-slim

 apt update
 apt install -y gcc-arm-none-eabi git unzip make gcc srecord wget
 cd /work
 git clone https://github.com/kernelcrash/msx-rom-and-floppy-emulator.git
 unzip en.stsw-stm32068.zip 

 wget http://xi6.com/files/asmx-1.8.2.zip

 mkdir asmx
 cd asmx
 unzip ../asmx-1.8.2.zip
 gcc -O1 -o asmz80 asmz80.c
 cp asmz80 /bin

 cd ../msx-rom-and-floppy-emulator
 make
#arm-none-eabi-gcc -g -O2 -Wall -Tstm32_flash.ld 
#...
#arm-none-eabi-objcopy -O ihex msx-rom-and-floppy-emulator.elf msx-rom-and-floppy-emulator.hex
#arm-none-eabi-objcopy -O binary msx-rom-and-floppy-emulator.elf msx-rom-and-floppy-emulator.bin

 cd kcmfs
## Assemble the menu.rom . You will see some warning messages. That's OK.
./build.sh menu.asm

# Check that it assembled and produced the menu.rom file. You should see a 32K menu.rom file
# ie. 32768 bytes
ls -l menu.rom
exit

## Now you will be back at the command prompt of whatever OS you are on. You should see a
## msx-rom-and-floppy-emulator directory. It will contain the
##  msx-rom-and-floppy-emulator.hex and msx-rom-and-floppy-emulator.bin files
## These are just 'the firmware' in two different file formats ; hex and bin. 
## Inside the msx-rom-and-floppy-emulator directory you should have the kcmfs directory now containing the
## menu.rom file.

## You'll need a firmware flashing tool to get the firmware on to the stm32f407 board. I have no idea
## about doing it on Windows or Mac. On linux it is easy, and the transfer.sh script is a one line
## command to do it ... if you have the dfu-util tool installed. STM32 boards have several 
## different ways to flash them (ie. you have a choice) .. and it's up to the reader to research
## those and figure it out themselves.

## The menu.rom file just needs to go in the root of the SD card.
```
If you made a mistake with any of that, you may find it easier to delete that whole 'work' directory and try
again, otherwise you will get different behaviour due to certain files and directories already existing. 

Also, if you are not on docker and you are using Debian, all the commands entered in to the docker container
(ie. from the 'apt update' onwards) should work if you had a Debian 'Bullseye' based linux system (eg.
Virtual Machine or you booted off a live USB). Debian Bullseye is version 11 one (11.8 was the last
version of Bullseye  I think).

