/* ReaPack: Package manager for REAPER
 * Copyright (C) 2015-2021  Christian Fillion
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

#ifndef REAPACK_HASH_HPP
#define REAPACK_HASH_HPP

#include <memory>
#include <string>

class Hash {
public:
  enum Algorithm {
    SHA256 = 0x12,
  };

  static bool getAlgorithm(const std::string &hash, Algorithm *out);

  Hash(Algorithm);
  Hash(const Hash &) = delete;

  void addData(const char *data, size_t len);
  const std::string &digest();

private:
  class Context {
  public:
    virtual ~Context() = default;
    virtual size_t hashSize() const = 0;
    virtual void addData(const char *data, size_t len) = 0;
    virtual void getHash(unsigned char *out) = 0;
  };

  class CNGContext;
  class SHA256Context;

  Algorithm m_algo;
  std::string m_value;
  std::unique_ptr<Context> m_context;
};

#endif
