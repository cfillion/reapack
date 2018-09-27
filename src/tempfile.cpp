#include "tempfile.hpp"

#include "filesystem.hpp"

TempFile::TempFile(const Path &target)
  : m_target(target), m_temp(target), m_created(false), m_saved(false)
{
  m_temp[m_temp.size() - 1] += ".part";
}

TempFile::~TempFile()
{
  if(m_created && !m_saved)
    FS::removeRecursive(m_temp);
}

bool TempFile::open()
{
  return FS::open(m_stream, m_temp) && (m_created = true);
}

void TempFile::close()
{
  m_stream.close();
}

bool TempFile::save()
{
#ifdef _WIN32
  FS::remove(m_target);
#endif

  return FS::rename(m_temp, m_target) && (m_saved = true);
}
