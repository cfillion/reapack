#ifndef REAPACK_PATH_HPP
#define REAPACK_PATH_HPP

#include <string>
#include <list>

class Path {
public:
  void prepend(const std::string &part);
  void append(const std::string &part);
  void clear();

  bool empty() const { return m_parts.empty(); }
  size_t size() const { return m_parts.size(); }

  std::string basename() const;
  std::string dirname() const { return join(true); }
  std::string join() const { return join(false); }

  bool operator==(const Path &) const;
  bool operator!=(const Path &) const;
  Path operator+(const std::string &) const;
  Path operator+(const Path &) const;
  std::string &operator[](const size_t index);

private:
  std::string join(const bool) const;

  std::list<std::string> m_parts;
};

#endif
