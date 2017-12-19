@echo off
SETLOCAL
setlocal EnableDelayedExpansion

call "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" x86

devenv debug.exe

REM -arch:IA32, SSE, SSE2, AVX, AVX2
REM -Dname defines macro
REM -EHa- disables exceptions
REM -favor:blend, ATOM, AMD64, INTEL64
REM -fp:fast, strict, precise
REM -FC shows source file path
REM -Fmpath.map location to function map file
REM -GA better performance for .exe
REM -GL enables whole program optimization
REM -Gm- disable incremental build
REM -GR- disables runtine type information (needed for c++ OOP only)
REM -Idirectory include directory
REM -MT pack runtime library in exe
REM -nologo disable compiler infos text
REM -O2 enables optimizations
REM -Od disable optimizations
REM -Oi enables native asm functions (like sin cos)
REM -Ot optimizations
REM -Oy omits frame pointers
REM -wd4201 ignore warning 4201
REM -W4 enables warning up to level 4
REM -WX consider warnings as errors
REM -Zi produce debug information
REM -Z7 different debug format (better??)

REM /link
REM /link -subsystem:windows,5.1 windows xp support
REM -link -opt:ref remove not used functions
