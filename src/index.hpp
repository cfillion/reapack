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

#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "package.hpp"
#include "source.hpp"

class Category;
typedef std::vector<const Category *> CategoryList;

class Download;
class Index;
class Path;
class Remote;
class TiXmlElement;
struct NetworkOpts;

struct Link { std::string name; std::string url; };

typedef std::shared_ptr<const Index> IndexPtr;

class Index : public std::enable_shared_from_this<const Index> {
public:
  enum LinkType { WebsiteLink, DonationLink };
  typedef std::multimap<LinkType, Link> LinkMap;
  typedef std::vector<const Link *> LinkList;

  static Path pathFor(const std::string &name);
  static LinkType linkTypeFor(const char *rel);
  static IndexPtr load(const std::string &name, const char *data = nullptr);
  static Download *fetch(const Remote &, bool stale, const NetworkOpts &);

  Index(const std::string &name);
  ~Index();

  void setName(const std::string &);
  const std::string &name() const { return m_name; }

  void setAboutText(const std::string &rtf) { m_about = rtf; }
  const std::string &aboutText() const { return m_about; }
  void addLink(const LinkType, const Link &);
  LinkList links(const LinkType type) const;

  void addCategory(const Category *cat);
  const CategoryList &categories() const { return m_categories; }
  const Category *category(size_t i) const { return m_categories[i]; }
  const Category *category(const std::string &name) const;

  const PackageList &packages() const { return m_packages; }

private:
  static void loadV1(TiXmlElement *, Index *);

  std::string m_name;
  std::string m_about;
  LinkMap m_links;
  CategoryList m_categories;
  PackageList m_packages;

  std::unordered_map<std::string, size_t> m_catMap;
};

class Category {
public:
  Category(const std::string &name, const Index * = nullptr);
  ~Category();

  const Index *index() const { return m_index; }
  const std::string &name() const { return m_name; }
  std::string fullName() const;

  void addPackage(const Package *pack);
  const PackageList &packages() const { return m_packages; }
  const Package *package(size_t i) const { return m_packages[i]; }
  const Package *package(const std::string &name) const;

private:
  const Index *m_index;

  std::string m_name;
  PackageList m_packages;

  std::unordered_map<std::string, size_t> m_pkgMap;
};

#endif
