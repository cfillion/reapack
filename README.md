# ReaPack: Package manager for REAPER

[![Build status](https://ci.appveyor.com/api/projects/status/hq0g2nleele3pqrl/branch/master?svg=true)](https://ci.appveyor.com/project/cfillion/reapack/branch/master)
[![Donate](https://img.shields.io/badge/donate-paypal-orange.svg)](https://www.paypal.com/cgi-bin/webscr?business=T3DEWBQJAV7WL&cmd=_donations&currency_code=CAD&item_name=ReaPack%3A+Package+manager+for+REAPER)

Visit the [ReaPack website](https://reapack.com/) for ready-to-use binaries,
the user guide or the package upload tool.

## Building from source

Clone the repository and submodules:

    git clone --recursive --shallow-submodules https://github.com/cfillion/reapack.git

### Prerequisites

Software requirements:

- [CMake](https://cmake.org/) 3.15 or newer
- C++17 compiler (MSVC on Windows)
- PHP (Linux and macOS only)

#### Linux

Install the following libraries (and development headers if your system provides
them separately):

- [Boost](https://www.boost.org/) (1.56 or later)
- [Catch2](https://github.com/catchorg/Catch2) (3.0 or later)
- [libcurl](https://curl.haxx.se/libcurl/)
- [libxml2](http://www.xmlsoft.org/)
- [OpenSSL](https://www.openssl.org/) or compatible
- [SQLite](https://www.sqlite.org/)
- [zlib](https://www.zlib.net/)

#### macOS

Install Boost and Catch2 using [Homebrew](https://brew.sh) (recommended).
The build tools can be installed using `xcode-select --install` or the Xcode IDE.

#### Windows

MSVC can be installed with the [Build Tools for Visual Studio](
https://visualstudio.microsoft.com/thank-you-downloading-visual-studio/?sku=BuildTools)
or the Visual Studio IDE.

Use the x64 or x86 Native Tools Command Prompt for VS 20XX matching the target
architecture when configuring or building ReaPack.

Install [vcpkg](https://docs.microsoft.com/cpp/build/vcpkg) in any directory:

    git clone https://github.com/Microsoft/vcpkg.git C:\path\to\vcpkg

Set `VCPKG_TARGET_TRIPLET` and `CMAKE_TOOLCHAIN_FILE` when creating the build
tree:

    -DVCPKG_TARGET_TRIPLET=%PLATFORM%-windows-static
    -DCMAKE_TOOLCHAIN_FILE=C:\path\to\vcpkg\scripts\buildsystems\vcpkg.cmake

### Build configuration

Create and configure a new build tree inside of the `build` directory.

    cmake -B build -DCMAKE_BUILD_TYPE=Debug

Using the [Ninja](https://ninja-build.org/) generator is recommended for
best performance:

    cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug

Alternatively, multiple build trees can be created if desired:

    cmake -B build/debug    -DCMAKE_BUILD_TYPE=Debug
    cmake -B build/release  -DCMAKE_BUILD_TYPE=Release
    cmake -B build/portable -DCMAKE_BUILD_TYPE=RelWithDebInfo \
      -DCMAKE_INSTALL_PREFIX=/path/to/reaper/portable/install

### Compile and install

To compile a build tree:

    cmake --build build

To install ReaPack into your REAPER installation after building:

    cmake --build build --target install

The following targets are available:

- **`all`**: Build ReaPack (default target)
- **`clean`**: Delete all generated files
  (can be run before building another target using `--clean-first`)
- **`install`**: Build and install ReaPack into REAPER's resource directory
  (as specified in `CMAKE_INSTALL_PREFIX`)
- **`test`**: Build and run the test suite

### Cross-compilation

#### Linux

`g++-$TOOLCHAIN_PREFIX` will be used when compiling for architectures other than
i686. Libraries for the target architecture are expected to be in
`/usr/{include,lib}/$TOOLCHAIN_PREFIX`, `/usr/$TOOLCHAIN_PREFIX/{include,lib}`
or `/usr/lib32`.

    ARCH=i686 TOOLCHAIN_PREFIX=i386-linux-gnu \
      cmake -B build/i686 -DCMAKE_TOOLCHAIN_FILE=cmake/linux-cross.cmake

    ARCH=armv7l TOOLCHAIN_PREFIX=arm-linux-gnueabihf \
      cmake -B build/arm32 -DCMAKE_TOOLCHAIN_FILE=cmake/linux-cross.cmake

    ARCH=aarch64 TOOLCHAIN_PREFIX=aarch64-linux-gnu \
      cmake -B build/arm64 -DCMAKE_TOOLCHAIN_FILE=cmake/linux-cross.cmake

#### macOS

macOS 10.14 or older, Xcode 9 and the latest Clang (`brew install llvm`) are
required for producing 32-bit builds.

    cmake -B build \
      -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_OSX_ARCHITECTURES=i386 \
      -DCMAKE_OSX_DEPLOYMENT_TARGET=10.9 \
      -DCMAKE_TOOLCHAIN_FILE=cmake/brew-llvm.cmake

## Support and feedback

Ask any ReaPack-releated questions in the
[ReaPack forum thread](https://forum.cockos.com/showthread.php?t=177978).
Questions related to development or packaging can be asked in the
[development thread](https://forum.cockos.com/showthread.php?t=169127).

Report bugs or request features in the
[issue tracker](https://github.com/cfillion/reapack/issues).
Send code contributions as pull requests.
