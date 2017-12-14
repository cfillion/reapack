# ReaPack: Package manager for REAPER

[![Donate](https://www.paypalobjects.com/webstatic/en_US/btn/btn_donate_74x21.png)](https://www.paypal.com/cgi-bin/webscr?business=T3DEWBQJAV7WL&cmd=_donations&currency_code=CAD&item_name=ReaPack%3A+Package+manager+for+REAPER)

## User Guide

https://reapack.com/user-guide

## Build Setup

Download these files into the `vendor` directory:

- catch.hpp from
  [philsquared/Catch](https://github.com/philsquared/Catch/raw/master/single_include/catch.hpp)
- reaper_plugin.h from
  [reaper-oss/sws](https://github.com/reaper-oss/sws/raw/master/reaper/reaper_plugin.h)
- reaper_plugin_functions.h from the REAPER action
- [WDL](http://www.cockos.com/wdl/) from Cockos:
  `git clone https://github.com/justinfrankel/WDL.git vendor/WDL`
  "[developer] Write C++ API functions header"

The `vendor` directory structure should be as follow:

```
reapack> tree vendor
vendor
├── WDL/
│   └── WDL/
│       ├── MersenneTwister.h
│       ├── adpcm_decode.h
│       ├── adpcm_encode.h
│       ├── assocarray.h
│       └── ...
├── catch.hpp
├── reaper_plugin.h
└── reaper_plugin_functions.h
```

Follow the following instructions to install prerequisites and build both 64-bit
and 32-bit versions. Use `rake 'default[x64]'` or `rake 'default[x86]'` instead
of `rake` to build just one.

### Linux

1. Install gcc-multilib (and g++-multilib depending on your vendor), tup, Ruby and PHP
2. Install Boost, curl and SQLite3 development files (64-bit and 32-bit versions if building both)
3. Build using `rake`
4. Copy or link `x64/bin/reaper_reapack64.so` or `x86/bin/reaper_reapack32.so`
  to `~/.config/REAPER/UserPlugins`

### macOS

1. Install [Homebrew](http://brew.sh/) and Xcode Command Line Tools
2. Install [tup](http://gittup.org/tup/) and [Boost](http://www.boost.org/):
  `brew tap homebrew/fuse && brew install tup boost`
3. Apply these patches to WDL:
    - [optimize-listview-setitemtext](https://github.com/cfillion/WDL/commit/a6d7f802762e5e9d9833829bab83696e0db50de6.patch)
    - [resize-redraw-fix](https://github.com/cfillion/WDL/commit/45ca4c819d4aaaed98540b8e5125085c05044786.patch)
    - [richtext-off](https://github.com/cfillion/WDL/commit/af9df173570edbb1d022045a7036d8d3296977b6.patch)
    - [shellexecute-https](https://github.com/cfillion/WDL/commit/0424a87047470aefbeef98526622e5af5f919ac9.patch)
3. Build using `rake`
5. Copy or link `x64/bin/reaper_reapack64.dylib` or `x86/bin/reaper_reapack32.dylib`
  to REAPER's extension directory

### Windows

1. Install [Ruby for Windows](http://rubyinstaller.org/),
  [tup](http://gittup.org/tup/win32/tup-explicit-variant-v0.7.3-45-gcf6a829.zip)
  (explicit-variant branch, see [this
  thread](https://groups.google.com/d/topic/tup-users/UNUSE15PQdA/discussion))
  and [Visual Studio 2017, with C++ support](https://www.visualstudio.com/vs/community/)
2. Prevent Microsoft's C++ compiler from saving telemetry outside of the build directory:
  [instructions here](https://msdn.microsoft.com/en-us/library/ee225238.aspx#Anchor_5)
  or set the `OptIn` registry key to `0` in
  `HKEY_LOCAL_MACHINE\SOFTWARE\WOW6432Node\Microsoft\VSCommon\15.0\SQM`
3. Download the latest [Boost](http://www.boost.org/) and copy the
  `boost` subdirectory into `vendor`
4. Download the latest [curl](http://curl.haxx.se/download.html) source
  code and extract it as `vendor/curl`:

    ```
    reapack> tree vendor
    vendor
    ├── curl/
    │   ├── builds/
    │   │   └── ...
    │   ├── winbuild/
    │   │   ├── Makefile.vc
    │   │   └── ...
    │   └── ...
    └── ...
    ```
5. Download the latest stable amalgamation build of [SQLite](https://www.sqlite.org/download.html).
   Put `sqlite3.h` and `sqlite3.c` in `vendor`.
6. Run `.\build_deps.bat` and `rake` using "Developer Command Prompt for VS 2017"
7. Copy or symlink `x64\bin\reaper_reapack64.dll` or `x86\bin\reaper_reapack32.dll`
   to your REAPER plugin folder
