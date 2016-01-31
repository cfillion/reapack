/* ReaPack: Package manager for REAPER
 * Copyright (C) 2015-2016  Christian Fillion
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef REAPACK_INDEX_HPP
#define REAPACK_INDEX_HPP

#include <memory>
#include <string>
#include <vector>

#include "package.hpp"

class Category;
typedef std::vector<Category *> CategoryList;

class Path;
class TiXmlElement;

class RemoteIndex {
public:
  static Path pathFor(const std::string &name);
  static RemoteIndex *load(const std::string &name);

  RemoteIndex(const std::string &name);
  ~RemoteIndex();

  const std::string &name() const { return m_name; }

  void addCategory(Category *cat);
  const CategoryList &categories() const { return m_categories; }
  Category *category(const size_t i) const { return m_categories[i]; }

  const PackageList &packages() const { return m_packages; }

private:
  static RemoteIndex *loadV1(TiXmlElement *, const std::string &);

  std::string m_name;
  CategoryList m_categories;
  PackageList m_packages;
};

class Category {
public:
  Category(const std::string &name, RemoteIndex * = nullptr);
  ~Category();

  RemoteIndex *index() const { return m_index; }
  const std::string &name() const { return m_name; }
  std::string fullName() const;

  void addPackage(Package *pack);
  const PackageList &packages() const { return m_packages; }
  Package *package(const size_t i) const { return m_packages[i]; }

private:
  RemoteIndex *m_index;

  std::string m_name;
  PackageList m_packages;
};

#endif
