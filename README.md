# ReaPack: Package manager for REAPER

[![Donate](https://img.shields.io/badge/donate-paypal-orange.svg)](https://www.paypal.com/cgi-bin/webscr?business=T3DEWBQJAV7WL&cmd=_donations&currency_code=CAD&item_name=ReaPack%3A+Package+manager+for+REAPER)

Visit the [ReaPack website](https://reapack.com/) for ready-to-use binaries
and the user guide.

## Compilation

The following section describes how to build ReaPack on your computer.
The build system is based on [Tup](http://gittup.org/tup/).
A modern compiler (gcc/clang/msvc) with C++17 support is needed.

By default the `tup` command triggers both the 64-bit and 32-bit builds.
Use `tup x64` or `tup x86` to select a single architecture. Run the test suite
with `make test`.

The 64-bit and 32-bit binaries (such as `reaper_reapack64.so`) are created in
`x64/bin` and `x86/bin` respectively. Copy or link the desired one into
`<REAPER resource path>/UserPlugins` and restart REAPER to use it.

Compiling ReaPack requires a few external libraries and files depending
on the operating system.

Put the REAPER extension SDK into the `vendor` directory:

- reaper_plugin.h from
  [reaper-oss/sws](https://github.com/reaper-oss/sws/raw/master/reaper/reaper_plugin.h)
- reaper_plugin_functions.h from the REAPER action
  "[developer] Write C++ API functions header"

Clone [WDL](http://www.cockos.com/wdl/): `git clone https://github.com/justinfrankel/WDL.git vendor/WDL`

### Linux

Install GCC/G++, tup, PHP and the development files for Boost (1.56 or later),
Catch2, libcurl (7.52 or later), SQLite3 and zlib matching the target architecture(s).

#### Custom compiler

Set the `CXX` environement variable to select a different compiler
(default is `gcc` if unset).

    CXX=gcc-8 tup x64

#### Custom libcurl

Set the `CURLSO` environment variable to override the system default libcurl
used for linking. Some older distributions still ship libcurl with the old
pre-7.16 SONAME. Consider using a libcurl built with `--disable-versioned-symbols`
to produce more compatible binaries (`libcurl-compat` on Arch Linux).

    CURLSO=:libcurl.so.3 tup

### macOS

Install Boost, tup and Xcode Command Line Tools. Using [Homebrew](http://brew.sh/):

    xcode-select --install
    brew cask install osxfuse
    brew install boost tup

Download the single header version of
[Catch2](https://github.com/catchorg/Catch2) (catch.hpp) into `vendor/catch`.

Apply these patches to WDL:

- [optimize-listview-setitemtext](https://github.com/cfillion/WDL/commit/a6d7f802762e5e9d9833829bab83696e0db50de6.patch)
- [resize-redraw-fix](https://github.com/cfillion/WDL/commit/45ca4c819d4aaaed98540b8e5125085c05044786.patch)
- [richtext-off](https://github.com/cfillion/WDL/commit/af9df173570edbb1d022045a7036d8d3296977b6.patch)
- [shellexecute-https](https://github.com/cfillion/WDL/commit/0424a87047470aefbeef98526622e5af5f919ac9.patch)

### Windows

Install tup ([explicit-variant](http://gittup.org/tup/win32/tup-explicit-variant-v0.7.3-45-gcf6a829.zip)
branch, see [this thread](https://groups.google.com/d/topic/tup-users/UNUSE15PQdA/discussion))
and [Visual Studio 2017 with C++ support](https://www.visualstudio.com/vs/community/).

Clone [vcpkg](https://github.com/Microsoft/vcpkg) into `vendor` and install
the build dependencies:

    git clone https://github.com/Microsoft/vcpkg.git vendor\vcpkg
    git apply --directory=vendor vendor\0001-vcpkg-curl-use-winssl.patch
    vendor\vcpkg\bootstrap-vcpkg.bat

    set /p vcpkg-deps=<vendor\vcpkg-deps.txt
    vendor\vcpkg\vcpkg install --triplet x64-windows-static %vcpkg-deps%
    vendor\vcpkg\vcpkg install --triplet x86-windows-static %vcpkg-deps%

Always run `tup` using the Developer Command Prompt for VS 2017.

## Support and feedback

Ask any ReaPack-releated questions in the
[ReaPack forum thread](https://forum.cockos.com/showthread.php?t=177978).
Questions related to development or packaging can be asked in the
[development thread](https://forum.cockos.com/showthread.php?t=169127).

Report bugs or request features in the [issue tracker](https://github.com/cfillion/reapack/issues).
