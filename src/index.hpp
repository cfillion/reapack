/* ReaPack: Package manager for REAPER
 * Copyright (C) 2015-2017  Christian Fillion
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

#include <map>
#include <memory>
#include <unordered_map>
#include <vector>

#include "string.hpp"
#include "metadata.hpp"
#include "package.hpp"
#include "source.hpp"

class Index;
class Path;
class Remote;
class TiXmlElement;
struct NetworkOpts;

typedef std::shared_ptr<const Index> IndexPtr;

class Index : public std::enable_shared_from_this<const Index> {
public:
  static Path pathFor(const String &name);
  static IndexPtr load(const String &name, const char *data = nullptr);

  Index(const String &name);
  ~Index();

  void setName(const String &);
  const String &name() const { return m_name; }

  Metadata *metadata() { return &m_metadata; }
  const Metadata *metadata() const { return &m_metadata; }

  bool addCategory(const Category *cat);
  const auto &categories() const { return m_categories; }
  const Category *category(size_t i) const { return m_categories[i]; }
  const Category *category(const String &name) const;
  const Package *find(const String &cat, const String &pkg) const;

  const std::vector<const Package *> &packages() const { return m_packages; }

private:
  static void loadV1(TiXmlElement *, Index *);

  String m_name;
  Metadata m_metadata;
  std::vector<const Category *> m_categories;
  std::vector<const Package *> m_packages;

  std::unordered_map<String, size_t> m_catMap;
};

class Category {
public:
  Category(const String &name, const Index *);
  ~Category();

  const Index *index() const { return m_index; }
  const String &name() const { return m_name; }
  String fullName() const;

  bool addPackage(const Package *pack);
  const auto &packages() const { return m_packages; }
  const Package *package(size_t i) const { return m_packages[i]; }
  const Package *package(const String &name) const;

private:
  const Index *m_index;

  String m_name;
  std::vector<const Package *> m_packages;
  std::unordered_map<String, size_t> m_pkgMap;
};

#endif
