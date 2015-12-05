#ifndef REAPACK_CONFIG_HPP
#define REAPACK_CONFIG_HPP

#include <string>

#include "registry.hpp"
#include "remote.hpp"

class Path;

class Config {
public:
  void read(const Path &);
  void write() const;

  void addRemote(Remote);
  const RemoteMap &remotes() const { return m_remotes; }
  Registry *registry() { return &m_registry; }

private:
  std::string getString(const char *, const std::string &) const;
  void setString(const char *, const std::string &, const std::string &) const;

  void fillDefaults();

  std::string m_path;

  void readRemotes();
  void writeRemotes() const;
  RemoteMap m_remotes;

  void readRegistry();
  void writeRegistry() const;
  Registry m_registry;
};

#endif
