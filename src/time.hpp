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

#ifndef REAPACK_TIME_HPP
#define REAPACK_TIME_HPP

#include <ctime>
#include <string>

class Time {
public:
  Time(const char *iso8601);
  Time(int year, int month, int day, int hour = 0, int minute = 0, int second = 0);
  Time(const std::tm &tm = {}) : m_tm(tm) {}

  bool isValid() const { return m_tm.tm_year > 0; }
  operator bool() const { return isValid(); }

  int year() const { return m_tm.tm_year + 1900; }
  int month() const { return m_tm.tm_mon + 1; }
  int day() const { return m_tm.tm_mday; }
  int hour() const { return m_tm.tm_hour; }
  int minute() const { return m_tm.tm_min; }
  int second() const { return m_tm.tm_sec; }

  std::string toString() const;

  int compare(const Time &) const;
  bool operator<(const Time &o) const { return compare(o) < 0; }
  bool operator<=(const Time &o) const { return compare(o) <= 0; }
  bool operator>(const Time &o) const { return compare(o) > 0; }
  bool operator>=(const Time &o) const { return compare(o) >= 0; }
  bool operator==(const Time &o) const { return compare(o) == 0; }
  bool operator!=(const Time &o) const { return compare(o) != 0; }

private:
  std::tm m_tm;
};

std::ostream &operator<<(std::ostream &os, const Time &time);

#endif
