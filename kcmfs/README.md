# Skeleton C project for MSX
Will create a "hello world" program running on MSX Z80 platform and emulated hardware. Following packages are required:

  * make
  * [hex2bin](http://sourceforge.net/projects/hex2bin/) (1)
  * [sdcc](http://sdcc.sourceforge.net/)

(1) To compile hex2bin on OSX, download the source. Extract the archive and delete the hex2bin, mot2bin and hex2bin.1 files (they are for Linux). Then go to the directory on the command line and rebuild the binaries for OS X by typing: ```make```
To do a system-wide install: ```make install MAN_DIR=/usr/local/share/man/man1```

## Compiling

```
make all      ;Compile and build
make compile  ;Just compile the project
make build    ;Build the final file (ROM|COM)
make emulator ;Launch the final file with openMSX
```

Check the makefile to select the correct startup file (ROM/COM). Current supported startup list:

- ROM 16kb (init: 0x4000)
- ROM 16kb (init: 0x8000)
- ROM 32Kb (init: 0x4000)
- MSX-DOS COM file (simple main)
- MSX-DOS COM file (main with arguments)

## Thanks
- [Avelino Herrera Morales](http://msx.atlantes.org/index_en.html) for providing the basic files needed to create a MSX-DOS executable.
- [Laurens Holst](http://map.grauw.nl/) for helping with hex2bin.
- [Alberto de Hoyo Nebot](http://albertodehoyonebot.blogspot.nl/p/how-to-create-msx-roms-with-sdcc.html)
