#ifndef REAPACK_VERSION_HPP
#define REAPACK_VERSION_HPP

#include <cstdint>
#include <set>
#include <string>

#include "source.hpp"

class Package;

class Version {
public:
  Version(const std::string &);
  ~Version();

  const std::string &name() const { return m_name; }
  std::string fullName() const;
  uint64_t code() const { return m_code; }

  void setPackage(Package *pkg) { m_package = pkg; }
  Package *package() const { return m_package; }

  void setChangelog(const std::string &);
  const std::string &changelog() const { return m_changelog; }

  void addSource(Source *source);
  const SourceList &sources() const { return m_sources; }
  Source *source(const size_t i) const { return m_sources[i]; }

  std::vector<Path> files() const;

  bool operator<(const Version &) const;

private:
  std::string m_name;
  uint64_t m_code;

  Package *m_package;

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

#endif
