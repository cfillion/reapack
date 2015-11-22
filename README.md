# ReaPack: Package Manager for REAPER

## Build Setup

Download [reaper_plugin.h](http://www.reaper.fm/sdk/plugin/reaper_plugin.h), save reaper_plugin_functions.h ([developer] Write C++ API functions header) and clone [WDL](http://www-dev.cockos.com/wdl/WDL.git) in the `vendor` directory.

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
├── reaper_plugin.h
└── reaper_plugin_functions.h
```

### OS X

1. Install [tup](http://gittup.org/tup/) and Xcode Command Line Tools
2. Run `tup` from this directory
3. Copy or link bin/reapack.dylib to REAPER's extension directory
