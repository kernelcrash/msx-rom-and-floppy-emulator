#!/bin/bash
# Requries asmz80 in your path
# Also srec_cat needs to be installed in your path
which asmz80 
if [ $? -ne 0 ];then
	echo "You need to install asmz80 in your PATH"
	exit 1
fi
which srec_cat
if [ $? -ne 0 ];then
	echo "You need to install srec_cat in your PATH"
	exit 1
fi
if [ "$1" = "" ];then
	echo "pass the asm file as the first argument"
	exit 1
fi


F=$(echo $1|sed 's/\.asm$//')

asmz80 -e -w -l -o -s37 -- $F.asm
srec_cat $F.asm.s37 -offset -16384 -o $F.asm.bin -binary
LEN=$(ls -l $F.asm.bin |awk '{print $5}') 
if [ $LEN -lt 32768 ];then
  dd if=/dev/zero bs=1 count=$((32768-LEN)) of=blank
  cat $F.bin blank >$F.rom
else
  cp $F.asm.bin $F.rom
fi



