#include "config.hpp"

#include "path.hpp"

Config::Config()
{
}

void Config::read(const Path &path)
{
  m_path = path.cjoin();
}

void Config::write() const
{
}
