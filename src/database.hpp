#ifndef REAPACK_DATABASE_HPP
#define REAPACK_DATABASE_HPP

#include <memory>
#include <string>
#include <vector>

class TiXmlElement;

class Database;
typedef std::shared_ptr<Database> DatabasePtr;

class Category;
typedef std::shared_ptr<Category> CategoryPtr;

class Package;
typedef std::shared_ptr<Package> PackagePtr;

class Database {
public:
  static DatabasePtr load(const char *);

  void addCategory(CategoryPtr cat);
  const std::vector<CategoryPtr> &categories() const { return m_categories; }
  CategoryPtr category(const int i) const { return m_categories[i]; }

private:
  friend Category;
  static DatabasePtr loadV1(TiXmlElement *);

  std::vector<CategoryPtr> m_categories;
};

class Category {
public:
  Category(const std::string &name);

  const std::string &name() const { return m_name; }

  void addPackage(PackagePtr pack);
  const std::vector<PackagePtr> &packages() const { return m_packages; }
  PackagePtr package(const int i) const { return m_packages[i]; }

private:
  friend Package;
  friend DatabasePtr Database::loadV1(TiXmlElement *);
  static CategoryPtr loadV1(TiXmlElement *);

  std::string m_name;
  std::vector<PackagePtr> m_packages;
};

class Package {
public:
  enum Type {
    UnknownType,
    ScriptType,
  };

  static Type convertType(const char *);

  Package();

  void setCategory(Category *cat) { m_category = cat; }
  Category *category() const { return m_category; }

private:
  friend CategoryPtr Category::loadV1(TiXmlElement *);
  static PackagePtr loadV1(TiXmlElement *);

  Category *m_category;
};

#endif
