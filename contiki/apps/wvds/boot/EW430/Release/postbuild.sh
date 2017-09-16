#!/bin/sh

SRCDIR=../..
ROLE=$(sed -n "/BOARD_CADRE1120_\([A-Z]*\)\s*1/s/^.*BOARD_CADRE1120_\([A-Z]*\)\s*1.*/\1/p" $SRCDIR/main.c)
cp Exe/boot.txt Exe/boot-${ROLE}.txt

[ "$ROLE" == "VD" ] && cp Exe/boot.txt ../../../device/EW430/Release/Exe/boot.txt
[ "$ROLE" == "RP" ] && cp Exe/boot.txt ../../../router/EW430/Release/Exe/boot.txt
[ "$ROLE" == "AP" ] && cp Exe/boot.txt ../../../gateway/EW430/Release/Exe/boot.txt
