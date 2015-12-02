#ifndef REAPACK_CONFIG_HPP
#define REAPACK_CONFIG_HPP

#include <string>
#include <vector>

class Repository {
public:
  Repository(const std::string &name, const std::string &url)
    : m_name(name), m_url(url)
  {}

  const std::string &name() const { return m_name; }
  const std::string &url() const { return m_url; }

private:
  std::string m_name;
  std::string m_url;
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
  void fillDefaults();

  std::string m_path;

  void readRepositories();
  void writeRepositories() const;
  RepositoryList m_repositories;
};

#endif
