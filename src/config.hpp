#ifndef REAPACK_CONFIG_HPP
#define REAPACK_CONFIG_HPP

#include <string>
#include <vector>

struct Repository {
  Repository(const std::string &name, const char *url)
    : m_name(name), m_url(url)
  {}

  const std::string &name() const { return m_name; }
  const char *url() const { return m_url; }

private:
  const std::string &m_name;
  const char *m_url;
};

typedef std::vector<Repository> RepositoryList;

class Path;

class Config {
public:
  Config();

  void read(const Path &);
  void write() const;

  const RepositoryList &repositories() const { return m_repositories; }

private:
  const char *m_path;

  RepositoryList m_repositories;
};

#endif
