@echo off
setlocal

set arch=%1%
set program=%2%
set args=

:next_arg
if "%3%"=="" goto continue
set args=%args% %3%
shift
goto next_arg
:continue

set PATH=%PATH%;%VCINSTALLDIR%\Auxiliary\Build

if "%arch%"=="x86_64" (
  call vcvars64.bat > NUL
) else ( if "%arch%"=="i386" (
  rem Using the x86 64-bit cross-compiler to avoid conflicts on the pdb file with tup
  call vcvarsamd64_x86.bat > NUL
) else (
  echo Unknown Architecture: %arch%
  exit /b 42
))

"%program%" %args%
