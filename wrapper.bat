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

if defined VCtupINSTALLDIR (
  set ENVTOOLS=%VCINSTALLDIR%\Auxiliary\Build
) else (
  rem https://github.com/Microsoft/vswhere
  for /f "usebackq tokens=*" %%i in (
    `"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
    -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64
    -property installationPath`) do (
      set ENVTOOLS=%%i\VC\Auxiliary\Build
  )
)

if "%arch%"=="x86_64" (
  call "%ENVTOOLS%\vcvars64.bat" > NUL
) else ( if "%arch%"=="i386" (
  rem Using the x86 64-bit cross-compiler to avoid conflicts on the pdb file with tup
  call "%ENVTOOLS%\vcvarsamd64_x86.bat" > NUL
) else (
  echo Unknown Architecture: %arch%
  exit /b 42
))

"%program%" %args%
