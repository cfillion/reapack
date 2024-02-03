/* ReaPack: Package manager for REAPER
 * Copyright (C) 2015-2024  Christian Fillion
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

#include <libxml/parser.h>

constexpr int LIBXML2_OPTIONS =
  XML_PARSE_NOBLANKS | XML_PARSE_NOERROR | XML_PARSE_NOWARNING;

struct XmlDocument::Impl { xmlDoc *doc;   };
struct XmlNode::Impl     { xmlNode *node; };

static int readCallback(void *context, char *buffer, const int size)
{
  auto stream = static_cast<std::istream *>(context);
  const std::streamsize bytes = stream->read(buffer, size).gcount();
  return static_cast<int>(bytes);
}

XmlDocument::XmlDocument(std::istream &stream) : m_impl{new Impl}
{
  xmlResetLastError();

  m_impl->doc =
    xmlReadIO(&readCallback, nullptr, static_cast<void *>(&stream),
      nullptr, nullptr, LIBXML2_OPTIONS);
}

XmlDocument::XmlDocument(XmlDocument &&) = default;

XmlDocument::~XmlDocument()
{
  if(m_impl->doc)
    xmlFreeDoc(m_impl->doc);
}

XmlDocument::operator bool() const
{
  return m_impl->doc && xmlGetLastError() == nullptr;
}

const char *XmlDocument::error() const
{
  if(const xmlError *error = xmlGetLastError()) {
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
  return xmlDocGetRootElement(m_impl->doc);
}

XmlNode::XmlNode(void *node) : m_impl(new Impl{static_cast<xmlNode *>(node)}) {}
XmlNode::XmlNode(const XmlNode &copy) { *this = copy; }
XmlNode::~XmlNode() = default;

XmlNode &XmlNode::operator=(const XmlNode &other)
{
  m_impl.reset(new Impl(*other.m_impl));
  return *this;
}

XmlNode::operator bool() const
{
  return m_impl->node != nullptr;
}

const char *XmlNode::name() const
{
  return reinterpret_cast<const char *>(m_impl->node->name);
}

XmlString XmlNode::attribute(const char *name) const
{
  return xmlGetProp(m_impl->node, reinterpret_cast<const xmlChar *>(name));
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
  return m_impl->node->children ? xmlNodeGetContent(m_impl->node) : nullptr;
}

XmlNode XmlNode::firstChild(const char *element) const
{
  for(xmlNode *node = m_impl->node->children; node; node = node->next) {
    if(node->type == XML_ELEMENT_NODE && (!element ||
        !xmlStrcmp(node->name, reinterpret_cast<const xmlChar *>(element))))
      return node;
  }

  return nullptr;
}

XmlNode XmlNode::nextSibling(const char *element) const
{
  for(xmlNode *node = m_impl->node->next; node; node = node->next) {
    if(node->type == XML_ELEMENT_NODE && (!element ||
        !xmlStrcmp(node->name, reinterpret_cast<const xmlChar *>(element))))
      return node;
  }

  return nullptr;
}

XmlString::XmlString(const void *str) : m_str(str) {}
XmlString::~XmlString() { xmlFree(const_cast<void *>(m_str)); }
