#include "helper.hpp"

#include <platform.hpp>

#include <utility>

static const char *M = "[platform]";

TEST_CASE("default platform", M) {
  const Platform pl;
  REQUIRE(pl.value() == Platform::Generic);
  REQUIRE(pl.test());
}

TEST_CASE("platform from string", M) {
  SECTION("unknown")
    REQUIRE(Platform("hello") == Platform::Unknown);

  SECTION("generic")
    REQUIRE(Platform("all") == Platform::Generic);

  SECTION("windows") {
    REQUIRE(Platform("windows") == Platform::Windows_Any);
    REQUIRE(Platform("win32")   == Platform::Windows_x86);
    REQUIRE(Platform("win64")   == Platform::Windows_x64);
  }

  SECTION("macOS") {
    REQUIRE(Platform("darwin")   == Platform::Darwin_Any);
    REQUIRE(Platform("darwin32") == Platform::Darwin_i386);
    REQUIRE(Platform("darwin64") == Platform::Darwin_x86_64);
  }

  SECTION("linux") {
    REQUIRE(Platform("linux")         == Platform::Linux_Any);
    REQUIRE(Platform("linux32")       == Platform::Linux_i686);
    REQUIRE(Platform("linux64")       == Platform::Linux_x86_64);
    REQUIRE(Platform("linux-armv7l")  == Platform::Linux_armv7l);
    REQUIRE(Platform("linux-aarch64") == Platform::Linux_aarch64);
  }
}

TEST_CASE("test platform", M) {
  const std::pair<Platform, bool> tests[] {
    {Platform::Unknown,       false},
    {Platform::Generic,       true },

#ifdef __APPLE__
    {Platform::Linux_Any,     false},
    {Platform::Linux_x86_64,  false},
    {Platform::Linux_i686,    false},
    {Platform::Linux_armv7l,  false},
    {Platform::Linux_aarch64, false},
    {Platform::Windows_Any,   false},
    {Platform::Windows_x64,   false},
    {Platform::Windows_x86,   false},

    {Platform::Darwin_Any,    true },
#  ifdef __x86_64__
    {Platform::Darwin_i386,   false},
    {Platform::Darwin_x86_64, true },
#  elif __i386__
    {Platform::Darwin_i386,   true },
    {Platform::Darwin_x86_64, false},
#  else
#    error Untested architecture
#  endif

#elif __linux__
    {Platform::Darwin_Any,    false},
    {Platform::Darwin_i386,   false},
    {Platform::Darwin_x86_64, false},
    {Platform::Windows_Any,   false},
    {Platform::Windows_x86,   false},
    {Platform::Windows_x64,   false},

    {Platform::Linux_Any,     true },
#  ifdef __x86_64__
    {Platform::Linux_i686,    false},
    {Platform::Linux_x86_64,  true },
    {Platform::Linux_armv7l,  false},
    {Platform::Linux_aarch64, false},
#  elif __i686__
    {Platform::Linux_i686,    true },
    {Platform::Linux_x86_64,  false},
    {Platform::Linux_armv7l,  false},
    {Platform::Linux_aarch64, false},
#  elif __ARM_ARCH_7A__
    {Platform::Linux_i686,    false},
    {Platform::Linux_x86_64,  false},
    {Platform::Linux_armv7l,  true },
    {Platform::Linux_aarch64, false},
#  elif __aarch64__
    {Platform::Linux_i686,    false},
    {Platform::Linux_x86_64,  false},
    {Platform::Linux_armv7l,  false},
    {Platform::Linux_aarch64, true },
#  else
#    error Untested architecture
#  endif

#elif _WIN32
    {Platform::Darwin_Any,    false},
    {Platform::Darwin_i386,   false},
    {Platform::Darwin_x86_64, false},
    {Platform::Linux_Any,     false},
    {Platform::Linux_x86_64,  false},
    {Platform::Linux_i686,    false},
    {Platform::Linux_armv7l,  false},
    {Platform::Linux_aarch64, false},

    {Platform::Windows_Any,   true },
#  ifdef _WIN64
    {Platform::Windows_x86,   false},
    {Platform::Windows_x64,   true },
#  else
    {Platform::Windows_x86,   true },
    {Platform::Windows_x64,   false},
#  endif

#else
#  error Untested platform
#endif
  };

  for(const auto &[platform, expected] : tests) {
    INFO("Testing platform: " << platform);
    REQUIRE(platform.test() == expected);
  }
}

TEST_CASE("current platform", M) {
  REQUIRE((Platform::Current & Platform::Unknown) == 0);
  REQUIRE((Platform::Current & Platform::Unknown) == Platform::Unknown);

  REQUIRE((Platform::Current & Platform::Generic) != 0);
  REQUIRE((Platform::Current & Platform::Generic) != Platform::Generic);
}
