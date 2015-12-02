#ifndef REAPACK_DATABASE_HPP
#define REAPACK_DATABASE_HPP

#include <memory>
#include <string>
#include <vector>

#include "package.hpp"

class TiXmlElement;

class Database;
typedef std::shared_ptr<Database> DatabasePtr;

class Category;
typedef std::shared_ptr<Category> CategoryPtr;

class Database {
public:
  static DatabasePtr load(const char *);

  void addCategory(CategoryPtr cat);
  const std::vector<CategoryPtr> &categories() const { return m_categories; }
  CategoryPtr category(const int i) const { return m_categories[i]; }

  const std::vector<PackagePtr> &packages() const { return m_packages; }

private:
  static DatabasePtr loadV1(TiXmlElement *);

  std::vector<CategoryPtr> m_categories;
  std::vector<PackagePtr> m_packages;
};

class Category : public std::enable_shared_from_this<Category> {
public:
  Category(const std::string &name);

  const std::string &name() const { return m_name; }

  void addPackage(PackagePtr pack);
  const std::vector<PackagePtr> &packages() const { return m_packages; }
  PackagePtr package(const int i) const { return m_packages[i]; }

private:
  std::string m_name;
  std::vector<PackagePtr> m_packages;
};

#endif
