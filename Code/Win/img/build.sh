#!/bin/sh
set -eu
nasm -f bin boot.asm -o boot.bin
nasm -f bin kernel.asm -o kernel.bin
python3 make_image.py
