#ifndef REAPACK_PACKAGE_HPP
#define REAPACK_PACKAGE_HPP

#include "path.hpp"
#include "version.hpp"

class Database;
class Category;

class Package;
typedef std::vector<Package *> PackageList;

class Package {
public:
  enum Type {
    UnknownType,
    ScriptType,
  };

  static Type convertType(const char *);

  Package(const Type, const std::string &name);
  ~Package();

  void setCategory(Category *cat) { m_category = cat; }
  Category *category() const { return m_category; }

  Type type() const { return m_type; }
  const std::string &name() const { return m_name; }

  void addVersion(Version *ver);
  const VersionSet &versions() const { return m_versions; }
  Version *version(const int i) const;
  Version *lastVersion() const;

  Path targetPath() const;

private:
  Category *m_category;

  Type m_type;
  std::string m_name;
  VersionSet m_versions;
};

#endif
