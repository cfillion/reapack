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

#include "serializer.hpp"

#include <boost/lexical_cast.hpp>
#include <sstream>

static const unsigned short VERSION = 1;
static const char FIELD_END = '\x20';
static const char RECORD_END = ',';

using namespace std;

auto Serializer::read(const string &input, const int userVersion) -> Data
{
  m_userVersion = userVersion;

  bool first = true;
  istringstream stream(input);

  Data out;

  while(!stream.eof()) {
    string line;
    getline(stream, line, RECORD_END);

    istringstream lineStream(line);

    Record rec;
    for(size_t i = 0; i < rec.size(); i++) {
      if(lineStream.eof())
        return out;

      string field;
      getline(lineStream, field, FIELD_END);

      int value;

      try {
        value = boost::lexical_cast<int>(field);
      }
      catch(const boost::bad_lexical_cast &) {
        return out; // data is invalid! aborting.
      }

      rec[i] = value;
    }

    if(first) {
      if(rec[0] != m_userVersion || rec[1] != VERSION)
        return {};

      first = false;
    }
    else
      out.push_back(rec);
  }

  return out;
}

string Serializer::write(const Data &data) const
{
  if(!m_userVersion)
    return {};

  ostringstream stream;

  stream
    << m_userVersion << FIELD_END
    << VERSION << RECORD_END;

  auto it = data.begin();

  while(true) {
    const Record &rec = *it;

    size_t j = 0;
    while(true) {
      stream << rec[j];

      if(++j < rec.size())
        stream << FIELD_END;
      else
        break;
    }

    it++;

    if(it != data.end())
      stream << RECORD_END;
    else
      break;
  }

  return stream.str();
}
