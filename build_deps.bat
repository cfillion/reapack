@echo off
setlocal

set self=%~dp0
set vendor=%self%\vendor
set openssl=%vendor%\openssl
set curl=%vendor%\curl

if "%1%"=="openssl32" (
  cd "%openssl%"
  perl Configure VC-WIN32 no-asm --prefix=%vendor%\deps
  cmd /c ms\do_ms
  perl -pi.bak -e s/\/Zi//g ms\nt.mak
  nmake /f ms\nt.mak clean
  nmake /f ms\nt.mak
  nmake /f ms\nt.mak install
  exit /b
)
if "%1%"=="openssl64" (
  cd "%openssl%"
  perl Configure VC-WIN64A no-asm --prefix=%vendor%\deps
  cmd /c ms\do_win64a
  perl -pi.bak -e s/\/Zi//g ms\nt.mak
  nmake /f ms\nt.mak clean
  nmake /f ms\nt.mak
  nmake /f ms\nt.mak install
  exit /b
)

if "%1%"=="curl32" (
  cd "%curl%\builds"
  if %errorlevel% neq 0 exit /b %errorlevel%
  del * /Q /S
  cd "%curl%\winbuild"
  nmake /f Makefile.vc mode=static RTLIBCFG=static WITH_SSL=static ENABLE_IDN=no MACHINE=x86
  xcopy /y /s %curl%\builds\libcurl-vc-x86-release-static-ssl-static-ipv6-sspi %vendor%\libcurl32\
  exit /b
)
if "%1%"=="curl64" (
  cd "%curl%\builds"
  if %errorlevel% neq 0 exit /b %errorlevel%
  del * /Q /S
  cd "%curl%\winbuild"
  nmake /f Makefile.vc mode=static RTLIBCFG=static WITH_SSL=static ENABLE_IDN=no MACHINE=x64
  xcopy /y /s %curl%\builds\libcurl-vc-x64-release-static-ssl-static-ipv6-sspi %vendor%\libcurl64\
  exit /b
)

call %self%\wrapper i386 cmd /c %self%\build_deps openssl32
call %self%\wrapper i386 cmd /c %self%\build_deps curl32

call %self%\wrapper x86_64 cmd /c %self%\build_deps openssl64
call %self%\wrapper x86_64 cmd /c %self%\build_deps curl64
