@echo off
gcc -std=c11 -Wall -Wextra -Wpedantic -O2 -I..\kernel\include ..\kernel\src\main.c ..\kernel\src\shell.c ..\kernel\src\fs.c ..\kernel\src\error.c -o kedo.exe
if errorlevel 1 exit /b 1
echo Built kedo.exe
