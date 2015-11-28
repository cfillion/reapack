#ifndef REAPACK_PACKAGE_HPP
#define REAPACK_PACKAGE_HPP

#include "version.hpp"

class Package;
typedef std::shared_ptr<Package> PackagePtr;

class Category;

class InstallLocation {
public:
  InstallLocation(const std::string &d, const std::string &f)
    : m_directory(d), m_filename(f)
  {}

  void prependDir(const std::string &p) { m_directory.insert(0, p); }
  void appendDir(const std::string &p) { m_directory.append(p); }

  const std::string directory() const { return m_directory; }
  const std::string filename() const { return m_filename; }

  std::string fullPath() const { return m_directory + "/" + m_filename; }

  bool operator==(const InstallLocation &o) const
  {
    return m_directory == o.directory() && m_filename == o.filename();
  }

  bool operator!=(const InstallLocation &o) const
  {
    return !(*this == o);
  }

private:
  std::string m_directory;
  std::string m_filename;
};

class Package {
public:
  enum Type {
    UnknownType,
    ScriptType,
  };

  static Type convertType(const char *);

  Package(const Type, const std::string &name, const std::string &author);

  void setCategory(Category *cat) { m_category = cat; }
  Category *category() const { return m_category; }

  Type type() const { return m_type; }
  const std::string &name() const { return m_name; }
  const std::string &author() const { return m_author; }

  void addVersion(VersionPtr ver);
  const VersionSet &versions() const { return m_versions; }
  VersionPtr version(const int i) const;
  VersionPtr lastVersion() const;

  InstallLocation targetLocation() const;

private:
  InstallLocation scriptLocation() const;

  Category *m_category;
  Type m_type;
  std::string m_name;
  std::string m_author;
  VersionSet m_versions;
};

#endif
