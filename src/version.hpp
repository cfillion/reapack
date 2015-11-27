#ifndef REAPACK_VERSION_HPP
#define REAPACK_VERSION_HPP

#include <set>
#include <string>
#include <vector>

class Version;
typedef std::shared_ptr<Version> VersionPtr;

class Source;
typedef std::shared_ptr<Source> SourcePtr;

class Version {
public:
  Version(const std::string &);

  const std::string &name() const { return m_name; }
  int code() const { return m_code; }

  void setChangelog(const std::string &);
  const std::string &changelog() const { return m_changelog; }

  void addSource(SourcePtr source);
  const std::vector<SourcePtr> &sources() const { return m_sources; }
  SourcePtr source(const int i) const { return m_sources[i]; }

  bool operator<(const Version &);

private:
  std::string m_name;
  int m_code;

  std::string m_changelog;
  std::vector<SourcePtr> m_sources;
};

class VersionCompare {
public:
  constexpr bool operator() (const VersionPtr &l, const VersionPtr &r) const
  {
    return *l.get() < *r.get();
  }
};

typedef std::set<VersionPtr, VersionCompare> VersionSet;

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
