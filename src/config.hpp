#ifndef REAPACK_CONFIG_HPP
#define REAPACK_CONFIG_HPP

#include <string>

#include "remote.hpp"

class Path;

class Config {
public:
  void read(const Path &);
  void write() const;

  const RemoteList &remotes() const { return m_remotes; }

private:
  void fillDefaults();

  std::string m_path;

  void readRemotes();
  void writeRemotes() const;
  RemoteList m_remotes;
};

#endif
