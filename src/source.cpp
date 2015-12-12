#include "source.hpp"

#include "errors.hpp"
#include "package.hpp"
#include "version.hpp"

using namespace std;

Source::Platform Source::ConvertPlatform(const char *platform)
{
  if(!strcmp(platform, "all"))
    return GenericPlatform;
  else if(!strcmp(platform, "windows"))
    return WindowsPlatform;
  else if(!strcmp(platform, "win32"))
    return Win32Platform;
  else if(!strcmp(platform, "win64"))
    return Win64Platform;
  else if(!strcmp(platform, "darwin"))
    return DarwinPlatform;
  else if(!strcmp(platform, "darwin32"))
    return Darwin32Platform;
  else if(!strcmp(platform, "darwin64"))
    return Darwin64Platform;
  else
    return UnknownPlatform;
}

Source::Source(const Platform platform, const std::string &url)
  : m_platform(platform), m_url(url), m_version(nullptr)
{
  if(m_url.empty())
    throw reapack_error("empty source url");
}

Package *Source::package() const
{
  return m_version ? m_version->package() : nullptr;
}

string Source::fullName() const
{
  // TODO
  return m_version->fullName();
}

Path Source::targetPath() const
{
  // TODO
  return package()->targetPath();
}
