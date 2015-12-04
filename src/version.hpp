#ifndef REAPACK_VERSION_HPP
#define REAPACK_VERSION_HPP

#include <set>
#include <string>
#include <vector>

class Source;
typedef std::vector<Source *> SourceList;

class Version {
public:
  Version(const std::string &);
  ~Version();

  const std::string &name() const { return m_name; }
  size_t code() const { return m_code; }

  void setChangelog(const std::string &);
  const std::string &changelog() const { return m_changelog; }

  void addSource(Source *source);
  const SourceList &sources() const { return m_sources; }
  Source *source(const size_t i) const { return m_sources[i]; }

  bool operator<(const Version &) const;

private:
  std::string m_name;
  size_t m_code;

  std::string m_changelog;
  SourceList m_sources;
};

class VersionCompare {
public:
  bool operator()(const Version *l, const Version *r) const
  {
    return *l < *r;
  }
};

typedef std::set<Version *, VersionCompare> VersionSet;

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

  static Platform convertPlatform(const char *);

  Source(const Platform, const std::string &source);

  Platform platform() const { return m_platform; }
  const std::string &url() const { return m_url; }

private:
  Platform m_platform;
  std::string m_url;
};

#endif
