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

if "%arch%"=="x86_64" (
  call "%VCINSTALLDIR%\vcvarsall.bat" x86_amd64
) else ( if "%arch%"=="i386" (
  call "%VCINSTALLDIR%\vcvarsall.bat" x86
) else (
  echo Unknown Architecture: %arch%
  exit /b 42
))

"%program%" %args%
