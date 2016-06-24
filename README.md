# ReaPack: Package Manager for REAPER

[![Donate](https://www.paypalobjects.com/webstatic/en_US/btn/btn_donate_74x21.png)](https://www.paypal.com/cgi-bin/webscr?cmd=_donations&business=T3DEWBQJAV7WL&lc=CA&item_name=ReaPack%3a%20Package%20Manager%20for%20REAPER&no_note=0&cn=Custom%20message&no_shipping=1&currency_code=CAD&bn=PP%2dDonationsBF%3abtn_donateCC_LG%2egif%3aNonHosted)

## User Guide

https://github.com/cfillion/reapack/wiki

## Build Setup

Download these files into the `vendor` directory:

1. reaper_plugin.h from
  [Jeff0S/sws](https://github.com/Jeff0S/sws/raw/master/reaper/reaper_plugin.h)
2. catch.hpp from
  [philsquared/Catch](https://github.com/philsquared/Catch/raw/master/single_include/catch.hpp)
3. [WDL](http://www.cockos.com/wdl/) from Cockos:
  `git clone http://www-dev.cockos.com/wdl/WDL.git vendor/WDL`
4. reaper_plugin_functions.h from the REAPER action
  "[developer] Write C++ API functions header"

The `vendor` directory structure should be as follow:

```
reapack> tree vendor
vendor
├── WDL/
│   └── WDL/
│       ├── MersenneTwister.h
│       ├── adpcm_decode.h
│       ├── adpcm_encode.h
│       ├── assocarray.h
│       └── ...
├── catch.hpp
├── reaper_plugin.h
└── reaper_plugin_functions.h
```

### OS X

1. Install [Homebrew](http://brew.sh/) and Xcode Command Line Tools
2. Install [tup](http://gittup.org/tup/) and [boost](http://www.boost.org/):
  `brew tap homebrew/fuse && brew install tup boost`
3. Run `rake` from this directory
4. Copy or link `x64/bin/reaper_reapack64.dylib` or `x64/bin/reaper_reapack32.dylib`
   to REAPER's extension directory

### Windows

1. Install [Ruby for Windows](http://rubyinstaller.org/),
  [tup](http://gittup.org/tup/win32/tup-explicit-variant-v0.7.3-45-gcf6a829.zip)
  (explicit-variant branch, see [this
  thread](https://groups.google.com/d/topic/tup-users/UNUSE15PQdA/discussion))
  and [Visual Studio 2015, with Visual C++](https://www.visualstudio.com/products/visual-studio-community-vs)
2. Prevent Microsoft's C++ compiler from saving telemetry outside of the build directory:
   [instructions here](https://msdn.microsoft.com/en-us/library/ee225238.aspx#Anchor_5)
   or set the `OptIn` registry key to `0` in
   `HKEY_LOCAL_MACHINE\SOFTWARE\WOW6432Node\Microsoft\VSCommon\14.0\SQM`
3. Download the latest [boost](http://www.boost.org/) and copy the
  `boost` subdirectory into `<reapack>\vendor`
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
5. Launch "Developer Command Prompt for VS2015" and run `build_deps.bat`
6. Download the latest stable amalgamation build of [sqlite](https://www.sqlite.org/download.html).
   Put `sqlite3.h` and `sqlite3.c` in `<reapack>\vendor`.
7. Run `rake` from this directory using
  "Developer Command Prompt for VS2015"
8. Copy or symlink `x64\bin\reaper_reapack64.dll` or `x86\bin\reaper_reapack32.dll`
   to your REAPER plugin folder
