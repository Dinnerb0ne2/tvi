@echo off
setlocal enabledelayedexpansion

echo === Tiny VI Build Script ===
echo.

if not exist "build" mkdir build

:: compiler auto detect
set COMPILER=
where cl >nul 2>nul && set COMPILER=cl
if not defined COMPILER where gcc >nul 2>nul && set COMPILER=gcc
if not defined COMPILER (
    echo ERROR: cl ^(MSVC^) or gcc ^(MinGW^) not found.
    echo Install Visual Studio or MinGW and ensure its bin folder is in PATH.
    pause & exit /b 1
)

echo Using compiler: %COMPILER%

:: variables
set INC=-I include
set SRC_MAIN=src\main.c
set SRC_LIBS=src\libs\*.c
set OUT_EXE=build\tvi.exe

:: compiler
if "%COMPILER%"=="cl" (
    :: msvc
    cl /nologo /W3 /O2 %INC% /Fe:%OUT_EXE% %SRC_MAIN% %SRC_LIBS%
) else (
    :: mingw
    gcc -Wall -O2 %INC% -o %OUT_EXE% %SRC_MAIN% %SRC_LIBS% -lkernel32 -luser32 -static -std=c11
)

:: output
if %errorlevel% equ 0 (
    echo.
    echo Build succeeded! Executable: %OUT_EXE%
) else (
    echo.
    echo.
    echo Build Failed!
)