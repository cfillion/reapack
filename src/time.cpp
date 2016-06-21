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

#include "time.hpp"

#include <iomanip>
#include <sstream>

using namespace std;

Time::Time(const string &iso8601) : m_tm()
{
  tm time = {};

  istringstream stream(iso8601);
  stream >> std::get_time(&time, "%Y-%m-%dT%H:%M:%S");

  if(stream.good())
    swap(m_tm, time);
}

Time::Time(int year, int month, int day, int hour, int minute, int second)
  : m_tm()
{
  m_tm.tm_year = year - 1900;
  m_tm.tm_mon = month - 1;
  m_tm.tm_mday = day;

  m_tm.tm_hour = hour;
  m_tm.tm_min = minute;
  m_tm.tm_sec = second;
}

string Time::toString() const
{
  if(!isValid())
    return {};

  char buf[32] = {};
  strftime(buf, sizeof(buf), "%B %d %Y", &m_tm);
  return buf;
}

int Time::compare(const Time &o) const
{
  if(year() > o.year())
    return 1;
  else if(year() < o.year())
    return -1;

  if(month() > o.month())
    return 1;
  else if(month() < o.month())
    return -1;

  if(day() > o.day())
    return 1;
  else if(day() < o.day())
    return -1;

  if(hour() > o.hour())
    return 1;
  else if(hour() < o.hour())
    return -1;

  if(minute() > o.minute())
    return 1;
  else if(minute() < o.minute())
    return -1;

  if(second() > o.second())
    return 1;
  else if(second() < o.second())
    return -1;

  return 0;
}
