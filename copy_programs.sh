#!/usr/bin/env bash

rm -rf loop
mkdir -p loop
sudo mount -t minix -o loop,rw,noexec hdd1.dsk loop
mkdir -p loop/programs
rm -f loop/programs/*
for f in user/*.elf; do
    cp $f loop/programs/$(basename $f ".bin.elf")
done
sudo umount loop
rm -r loop
