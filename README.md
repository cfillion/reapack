# ReaPack: Package Manager for REAPER

## Build Setup

Download [reaper_plugin.h](http://www.reaper.fm/sdk/plugin/reaper_plugin.h), [catch.hpp](https://raw.githubusercontent.com/philsquared/Catch/master/single_include/catch.hpp), save reaper_plugin_functions.h ([developer] Write C++ API functions header) and clone [WDL](http://www-dev.cockos.com/wdl/WDL.git). All in the `vendor` directory.

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
