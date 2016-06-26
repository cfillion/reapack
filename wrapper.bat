@echo off
setlocal enabledelayedexpansion

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

:: Windows XP support
:: https://blogs.msdn.microsoft.com/vcblog/2012/10/08/windows-xp-targeting-with-c-in-visual-studio-2012/
:: http://www.tripleboot.org/?p=423
set PF86=%ProgramFiles(x86)%
set INCLUDE=%PF86%\Microsoft SDKs\Windows\v7.1A\Include;%INCLUDE%
set PATH=%PF86%\Microsoft SDKs\Windows\v7.1A\Bin;%PATH%
set LIB=%PF86%\Microsoft SDKs\Windows\v7.1A\Lib;%LIB%
set CL=/D_WIN32_WINNT#0x0501 /D_USING_V110_SDK71_ /D_ATL_XP_TARGETING %CL%
if "%arch%"=="x86_64" (
  set LIB=!PF86!\Microsoft SDKs\Windows\v7.1A\Lib\x64;!LIB!
  set LINK=/SUBSYSTEM:CONSOLE,5.02 %LINK%
) else (
  set LINK=/SUBSYSTEM:CONSOLE,5.01 %LINK%
)

"%program%" %args%
