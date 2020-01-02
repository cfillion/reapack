/* ReaPack: Package manager for REAPER
 * Copyright (C) 2015-2020  Christian Fillion
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

class XmlNode;
class XmlString;

struct _xmlDoc;
using xmlDoc = _xmlDoc;
struct _xmlNode;
using xmlNode = _xmlNode;
using xmlChar = unsigned char;

class XmlDocument {
public:
  XmlDocument(std::istream &);
  XmlDocument(const XmlDocument &) = delete;
  XmlDocument(XmlDocument &&);
  ~XmlDocument();

  operator bool() const;
  const char *error() const;

  XmlNode root() const;

private:
  xmlDoc *m_doc;
};

class XmlNode {
public:
  XmlNode(xmlNode *);
  XmlNode(const XmlNode &);
  ~XmlNode();

  operator bool() const;
  XmlNode &operator=(const XmlNode &);

  const char *name() const;
  XmlString attribute(const char *name) const;
  bool attribute(const char *name, int *value) const;
  XmlString text() const;

  XmlNode firstChild(const char *element = nullptr) const;
  XmlNode nextSibling(const char *element = nullptr) const;

private:
  xmlNode *m_node;
};

class XmlString {
public:
  XmlString(xmlChar *);
  XmlString(const XmlString &) = delete;
  XmlString(XmlString &&) = default;
  ~XmlString();

  operator bool() const;
  const char *operator *() const;
  const char *value_or(const char *fallback) const;

private:
  xmlChar *m_str;
};

#endif
