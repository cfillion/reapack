#ifndef REAPACK_PACKAGE_HPP
#define REAPACK_PACKAGE_HPP

#include "path.hpp"
#include "version.hpp"

class Package;
typedef std::shared_ptr<Package> PackagePtr;

class Category;

class Package {
public:
  enum Type {
    UnknownType,
    ScriptType,
  };

  static Type convertType(const char *);

  Package(const Type, const std::string &name);

  void setCategory(Category *cat) { m_category = cat; }
  Category *category() const { return m_category; }

  Type type() const { return m_type; }
  const std::string &name() const { return m_name; }

  void addVersion(VersionPtr ver);
  const VersionSet &versions() const { return m_versions; }
  VersionPtr version(const int i) const;
  VersionPtr lastVersion() const;

  Path targetLocation() const;

private:
  Path scriptLocation() const;

  Category *m_category;
  Type m_type;
  std::string m_name;
  VersionSet m_versions;
};

#endif
