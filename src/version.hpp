#ifndef REAPACK_VERSION_HPP
#define REAPACK_VERSION_HPP

#include <set>
#include <string>

class Version;
typedef std::shared_ptr<Version> VersionPtr;

class Version {
public:
  Version(const std::string &);

  const std::string &name() const { return m_name; }
  int code() const { return m_code; }

  bool operator<(const Version &);

private:
  std::string m_name;
  int m_code;
};

class VersionCompare {
public:
  constexpr bool operator() (const VersionPtr &l, const VersionPtr &r) const
  {
    return *l.get() < *r.get();
  }
};

typedef std::set<VersionPtr, VersionCompare> VersionSet;

#endif
