@echo off
nasm -f bin boot.asm -o boot.bin || exit /b 1
nasm -f bin kernel.asm -o kernel.bin || exit /b 1
python make_image.py
