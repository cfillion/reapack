#ifndef REAPACK_DATABASE_HPP
#define REAPACK_DATABASE_HPP

#include <memory>
#include <string>
#include <vector>

#include "package.hpp"

class TiXmlElement;

class Database;
typedef std::vector<Database *> DatabaseList;

class Category;
typedef std::vector<Category *> CategoryList;

class Database {
public:
  static Database *load(const char *);

  ~Database();

  void setName(const std::string &name) { m_name = name; }
  const std::string &name() const { return m_name; }

  void addCategory(Category *cat);
  const CategoryList &categories() const { return m_categories; }
  Category *category(const size_t i) const { return m_categories[i]; }

  const PackageList &packages() const { return m_packages; }

private:
  static Database *loadV1(TiXmlElement *);

  std::string m_name;
  CategoryList m_categories;
  PackageList m_packages;
};

class Category {
public:
  Category(const std::string &name);
  ~Category();

  const std::string &name() const { return m_name; }

  void setDatabase(Database *db) { m_database = db; }
  Database *database() const { return m_database; }

  void addPackage(Package *pack);
  const PackageList &packages() const { return m_packages; }
  Package *package(const size_t i) const { return m_packages[i]; }

private:
  Database *m_database;

  std::string m_name;
  PackageList m_packages;
};

#endif
