/* ReaPack: Package manager for REAPER
 * Copyright (C) 2015-2025  Christian Fillion
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

#ifndef REAPACK_XML_HPP
#define REAPACK_XML_HPP

#include <istream>
#include <memory>

class XmlNode;
class XmlString;

class XmlDocument {
  struct Impl;
  using ImplPtr = std::unique_ptr<Impl>;

public:
  XmlDocument(std::istream &);
  XmlDocument(const XmlDocument &) = delete;
  XmlDocument(XmlDocument &&);
  ~XmlDocument();

  operator bool() const;
  const char *error() const;

  XmlNode root() const;

private:
  ImplPtr m_impl;
};

class XmlNode {
  friend XmlDocument;
  struct Impl;
  using ImplPtr = std::unique_ptr<Impl>;

public:
  XmlNode(void *);
  XmlNode(const XmlNode &);
  ~XmlNode();

  XmlNode &operator=(const XmlNode &);
  operator bool() const;

  const char *name() const;
  XmlString attribute(const char *name) const;
  bool attribute(const char *name, int *value) const;
  XmlString text() const;

  XmlNode firstChild(const char *element = nullptr) const;
  XmlNode nextSibling(const char *element = nullptr) const;

private:
  ImplPtr m_impl;
};

class XmlString {
public:
  XmlString(const void *);
  XmlString(const XmlString &) = delete;
  XmlString(XmlString &&) = default;
  ~XmlString();

  operator bool() const { return m_str != nullptr; }
  const char *operator *() const { return reinterpret_cast<const char *>(m_str); }
  const char *value_or(const char *fallback) const { return m_str ? **this : fallback; }

private:
  const void *m_str;
};

#endif
