#ifndef REAPACK_PATH_HPP
#define REAPACK_PATH_HPP

#include <string>
#include <list>

class Path {
public:
  void prepend(const std::string &part);
  void append(const std::string &part);

  bool empty() const { return m_parts.empty(); }
  int size() const { return m_parts.size(); }

  std::string dirname() const { return join(true); }
  const char *cdirname() const { return dirname().c_str(); }

  const std::string &basename() const { return m_parts.back(); }
  const char *cbasename() const { return basename().c_str(); }

  std::string join() const { return join(false); }
  const char *cjoin() const { return join().c_str(); }

  bool operator==(const Path &) const;
  bool operator!=(const Path &) const;
  Path operator+(const std::string &) const;
  Path operator+(const Path &) const;

private:
  std::string join(const bool) const;

  std::list<std::string> m_parts;
};

#endif
