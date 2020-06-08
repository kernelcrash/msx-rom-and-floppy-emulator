ELF=msx-rom-and-floppy-emulator.elf
arm-none-eabi-objdump -dS $ELF >asm.out
arm-none-eabi-objdump -x $ELF | grep ^[12]|sort >vars.out
