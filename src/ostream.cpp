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

#include "ostream.hpp"

#include "version.hpp"

#include <boost/algorithm/string/trim.hpp>
#include <locale>

using namespace std;

OutputStream::OutputStream()
{
  // enable number formatting (ie. "1,234" instead of "1234")
  m_stream.imbue(locale(""));
}

void OutputStream::indented(const String &text)
{
  StringStreamI stream(text);
  String line;

  while(getline(stream, line, L('\n'))) {
    boost::algorithm::trim(line);

    if(!line.empty())
      m_stream << L("\x20\x20") << line;

    m_stream << L("\r\n");
  }
}

OutputStream &OutputStream::operator<<(const Version &ver)
{
  m_stream << L('v') << ver.name().toString();

  if(!ver.author().empty())
    m_stream << L(" by ") << ver.author();

  const String &date = ver.time().toString();
  if(!date.empty())
    m_stream << " – " << date;

  m_stream << L("\r\n");

  const String &changelog = ver.changelog();
  indented(changelog.empty() ? L("No changelog") : changelog);

  return *this;
}

