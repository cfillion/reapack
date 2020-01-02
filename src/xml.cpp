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

#include "xml.hpp"

#include <cstdio>
#include <cstring>
#include <fstream>

#include <libxml/parser.h>

constexpr int LIBXML2_OPTIONS =
  XML_PARSE_NOBLANKS | XML_PARSE_NOERROR | XML_PARSE_NOWARNING;

static int readCallback(void *context, char *buffer, const int size)
{
  auto stream = static_cast<std::istream *>(context);
  const std::streamsize bytes = stream->read(buffer, size).gcount();
  return static_cast<int>(bytes);
}

XmlDocument::XmlDocument(std::istream &stream)
{
  xmlResetLastError();

  m_doc =
    xmlReadIO(&readCallback, nullptr, static_cast<void *>(&stream),
      nullptr, nullptr, LIBXML2_OPTIONS);
}

XmlDocument::XmlDocument(XmlDocument &&) = default;

XmlDocument::~XmlDocument()
{
  if(m_doc)
    xmlFreeDoc(m_doc);
}

XmlDocument::operator bool() const
{
  return m_doc && xmlGetLastError() == nullptr;
}

const char *XmlDocument::error() const
{
  if(xmlError *error = xmlGetLastError()) {
    const size_t length = strlen(error->message);
    if(length > 1) {
      // remove trailing newline
      char &tail = error->message[length - 1];
      if(tail == '\n')
        tail = 0;
    }

    return error->message;
  }

  return nullptr;
}

XmlNode XmlDocument::root() const
{
  return xmlDocGetRootElement(m_doc);
}

XmlNode::XmlNode(xmlNode *node) : m_node(node) {}
XmlNode::XmlNode(const XmlNode &) = default;
XmlNode::~XmlNode() = default;

XmlNode::operator bool() const
{
  return m_node != nullptr;
}

XmlNode &XmlNode::operator=(const XmlNode &) = default;

const char *XmlNode::name() const
{
  return reinterpret_cast<const char *>(m_node->name);
}

XmlString XmlNode::attribute(const char *name) const
{
  return xmlGetProp(m_node, reinterpret_cast<const xmlChar *>(name));
}

bool XmlNode::attribute(const char *name, int *output) const
{
  if(const XmlString &value = attribute(name)) {
    return sscanf(*value, "%d", output) == 1;
  }

  return false;
}

XmlString XmlNode::text() const
{
  return m_node->children ? xmlNodeGetContent(m_node) : nullptr;
}

XmlNode XmlNode::firstChild(const char *element) const
{
  for(xmlNode *node = m_node->children; node; node = node->next) {
    if(node->type == XML_ELEMENT_NODE && (!element ||
        !xmlStrcmp(node->name, reinterpret_cast<const xmlChar *>(element))))
      return node;
  }

  return nullptr;
}

XmlNode XmlNode::nextSibling(const char *element) const
{
  for(xmlNode *node = m_node->next; node; node = node->next) {
    if(node->type == XML_ELEMENT_NODE && (!element ||
        !xmlStrcmp(node->name, reinterpret_cast<const xmlChar *>(element))))
      return node;
  }

  return nullptr;
}

XmlString::XmlString(xmlChar *str) : m_str(str) {}

XmlString::~XmlString()
{
  xmlFree(m_str);
}

XmlString::operator bool() const
{
  return m_str != nullptr;
}

const char *XmlString::operator *() const
{
  return reinterpret_cast<const char *>(m_str);
}

const char *XmlString::value_or(const char *fallback) const
{
  return m_str ? **this : fallback;
}
