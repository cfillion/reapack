#ifndef REAPACK_CONFIG_HPP
#define REAPACK_CONFIG_HPP

#include <string>

#include "registry.hpp"
#include "remote.hpp"

class Path;

class Config {
public:
  Config();

  void read(const Path &);
  void write();

  void addRemote(Remote);
  const RemoteMap &remotes() const { return m_remotes; }
  Registry *registry() { return &m_registry; }

private:
  std::string getString(const char *, const std::string &) const;
  void setString(const char *, const std::string &, const std::string &) const;
  size_t getUInt(const char *, const std::string &) const;
  void setUInt(const char *, const std::string &, const size_t) const;
  void deleteKey(const char *, const std::string &) const;
  void cleanupArray(const char *, const std::string &,
    const size_t begin, const size_t end) const;

  void fillDefaults();

  std::string m_path;

  void readRemotes();
  void writeRemotes();
  RemoteMap m_remotes;
  size_t m_remotesIniSize;

  void readRegistry();
  void writeRegistry();
  Registry m_registry;
  size_t m_registryIniSize;
};

#endif
