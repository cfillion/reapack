#ifndef REAPACK_DATABASE_HPP
#define REAPACK_DATABASE_HPP

#include <memory>
#include <string>
#include <vector>

#include "package.hpp"

class TiXmlElement;

class Category;

class Database {
public:
  static Database *load(const char *);

  ~Database();

  void setName(const std::string &name) { m_name = name; }
  const std::string &name() const { return m_name; }

  void addCategory(Category *cat);
  const std::vector<Category *> &categories() const { return m_categories; }
  Category *category(const int i) const { return m_categories[i]; }

  const std::vector<Package *> &packages() const { return m_packages; }

private:
  static Database *loadV1(TiXmlElement *);

  std::string m_name;
  std::vector<Category *> m_categories;
  std::vector<Package *> m_packages;
};

class Category {
public:
  Category(const std::string &name);
  ~Category();

  const std::string &name() const { return m_name; }

  void setDatabase(Database *db) { m_database = db; }
  Database *database() const { return m_database; }

  void addPackage(Package *pack);
  const std::vector<Package *> &packages() const { return m_packages; }
  Package *package(const int i) const { return m_packages[i]; }

private:
  Database *m_database;

  std::string m_name;
  std::vector<Package *> m_packages;
};

#endif
