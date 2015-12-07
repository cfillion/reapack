#ifndef REAPACK_REGISTRY_HPP
#define REAPACK_REGISTRY_HPP

#include <map>
#include <string>

class Package;

class Registry {
public:
  typedef std::map<std::string, std::string> Map;

  enum Status {
    UpToDate,
    UpdateAvailable,
    Uninstalled,
  };

  struct QueryResult {
    Status status;
    size_t versionCode;
  };

  void push(Package *pkg);
  void push(const std::string &key, const std::string &value);

  size_t size() const { return m_map.size(); }
  QueryResult query(Package *pkg) const;

  Map::const_iterator begin() const { return m_map.begin(); }
  Map::const_iterator end() const { return m_map.end(); }

private:
  Map m_map;
};

#endif
