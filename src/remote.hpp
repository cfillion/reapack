#ifndef REAPACK_REMOTE_HPP
#define REAPACK_REMOTE_HPP

#include <string>
#include <vector>

class Remote;
typedef std::vector<Remote> RemoteList;

class Remote {
public:
  Remote(const std::string &name, const std::string &url)
    : m_name(name), m_url(url)
  {}

  const std::string &name() const { return m_name; }
  const std::string &url() const { return m_url; }

private:
  std::string m_name;
  std::string m_url;
};

#endif
