# ReaPack: Package Manager for REAPER

## Build Setup

Download these files into the `vendor` directory:

1. reaper_plugin.h from
  [reaper.fm](http://www.reaper.fm/sdk/plugin/reaper_plugin.h)
2. catch.hpp from
  [github.com](https://github.com/philsquared/Catch/raw/master/single_include/catch.hpp)
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
     `brew install tup boost`
3. Run `tup --quiet && bin/test` from this directory
4. Copy or link `bin/reaper_reapack.dylib` to REAPER's extension directory

### Windows

1. Install [tup](http://gittup.org/tup/) and [Visual Studio Express 2013 for
     Desktop](https://www.microsoft.com/en-us/download/details.aspx?id=48131)
2. Download the latest [boost](http://www.boost.org/) and copy the
     `boost` subdirectory into `<reapack>\vendor`
3. Download the latest [curl](http://curl.haxx.se/download.html) source
     code and extract it on your computer
4. Launch "Developer Command Prompt for VS2013" and run
     `VC\vcvarsall x86_amd64` (or x86 for 32-bit)
5. Navigate to `<curl directory>\winbuild` and run
     `nmake /f Makefile.vc mode=static MACHINE=x64` (or x86)
6. Copy `..\builds\libcurl-vc-x64-release-static-ipv6-sspi-winssl`
     to `<reapack directory>\vendor` as `libcurl`
7. Navigate to `<reapack directory>` and run `tup --quiet`
8. Run `bin\test` and copy `bin\reaper_reapack.dll` to your REAPER plugin folder
