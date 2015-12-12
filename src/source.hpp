#ifndef REAPACK_SOURCE_HPP
#define REAPACK_SOURCE_HPP

#include <string>
#include <vector>

#include "path.hpp"

class Source;
typedef std::vector<Source *> SourceList;

class Package;
class Version;

class Source {
public:
  enum Platform {
    UnknownPlatform,
    GenericPlatform,

    // windows
    WindowsPlatform,
    Win32Platform,
    Win64Platform,

    // os x
    DarwinPlatform,
    Darwin32Platform,
    Darwin64Platform,
  };

  static Platform ConvertPlatform(const char *);

  Source(const Platform, const std::string &source);

  Platform platform() const { return m_platform; }
  const std::string &url() const { return m_url; }

  void setVersion(Version *ver) { m_version = ver; }
  Version *version() const { return m_version; }
  Package *package() const;

  std::string fullName() const;
  Path targetPath() const;

private:
  Platform m_platform;
  std::string m_url;
  Version *m_version;
};

#endif
