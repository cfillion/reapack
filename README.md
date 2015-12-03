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
