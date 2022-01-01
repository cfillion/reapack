/* ReaPack: Package manager for REAPER
 * Copyright (C) 2015-2022  Christian Fillion
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

#include <iterator>

#include <tinyxml2.h>

struct XmlDocument::Impl { tinyxml2::XMLDocument doc;  };
struct XmlNode::Impl     { tinyxml2::XMLElement *node; };

XmlDocument::XmlDocument(std::istream &stream)
  : m_impl{std::make_unique<Impl>()}
{
  std::string everything(std::istreambuf_iterator<char>(stream), {});
  m_impl->doc.Parse(everything.c_str(), everything.size());
}

XmlDocument::XmlDocument(XmlDocument &&) = default;
XmlDocument::~XmlDocument() = default;

XmlDocument::operator bool() const
{
  return m_impl->doc.ErrorID() == tinyxml2::XML_SUCCESS;
}

const char *XmlDocument::error() const
{
  return !*this ? m_impl->doc.ErrorStr() : nullptr;
}

XmlNode XmlDocument::root() const
{
  return m_impl->doc.RootElement();
}


XmlNode::XmlNode(void *node)
  : m_impl(new Impl{static_cast<tinyxml2::XMLElement *>(node)}) {}
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
  return m_impl->node->Value();
}

XmlString XmlNode::attribute(const char *name) const
{
  return m_impl->node->Attribute(name);
}

bool XmlNode::attribute(const char *name, int *output) const
{
  return m_impl->node->QueryIntAttribute(name, output) == tinyxml2::XML_SUCCESS;
}

XmlString XmlNode::text() const
{
  return m_impl->node->GetText();
}

XmlNode XmlNode::firstChild(const char *element) const
{
  return m_impl->node->FirstChildElement(element);
}

XmlNode XmlNode::nextSibling(const char *element) const
{
  return m_impl->node->NextSiblingElement(element);
}

XmlString::XmlString(const void *str) : m_str(str) {}
XmlString::~XmlString() = default;
