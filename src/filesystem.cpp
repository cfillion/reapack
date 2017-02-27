/* ReaPack: Package manager for REAPER
 * Copyright (C) 2015-2017  Christian Fillion
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "filesystem.hpp"

#include "encoding.hpp"
#include "path.hpp"

#include <cerrno>
#include <cstdio>
#include <fstream>
#include <sys/stat.h>

#include <reaper_plugin_functions.h>

#ifdef _WIN32
#include <windows.h>
#endif

using namespace std;

FILE *FS::open(const Path &path)
{
  const Path &fullPath = Path::prefixRoot(path);

#ifdef _WIN32
  FILE *file = nullptr;
  _wfopen_s(&file, make_autostring(fullPath.join()).c_str(), L"rb");
  return file;
#else
  return fopen(fullPath.join().c_str(), "rb");
#endif
}

bool FS::open(ifstream &stream, const Path &path)
{
  const Path &fullPath = Path::prefixRoot(path);
  stream.open(make_autostring(fullPath.join()), ios_base::binary);
  return stream.good();
}

bool FS::open(ofstream &stream, const Path &path)
{
  mkdir(path.dirname());

  const Path &fullPath = Path::prefixRoot(path);
  stream.open(make_autostring(fullPath.join()), ios_base::binary);
  return stream.good();
}

bool FS::write(const Path &path, const string &contents)
{
  ofstream file;
  if(!open(file, path))
    return false;

  file << contents;
  file.close();

  return true;
}

bool FS::rename(const TempPath &path)
{
#ifdef _WIN32
  remove(path.target());
#endif

  return rename(path.temp(), path.target());
}

bool FS::rename(const Path &from, const Path &to)
{
  const string &fullFrom = Path::prefixRoot(from).join();
  const string &fullTo = Path::prefixRoot(to).join();

#ifdef _WIN32
  return !_wrename(make_autostring(fullFrom).c_str(),
    make_autostring(fullTo).c_str());
#else
  return !::rename(fullFrom.c_str(), fullTo.c_str());
#endif
}

bool FS::remove(const Path &path)
{
  const auto_string &fullPath =
    make_autostring(Path::prefixRoot(path).join());

#ifdef _WIN32
  if(GetFileAttributes(fullPath.c_str()) == INVALID_FILE_ATTRIBUTES
      && GetLastError() == ERROR_FILE_NOT_FOUND)
    return true;

  if(GetFileAttributes(fullPath.c_str()) & FILE_ATTRIBUTE_DIRECTORY)
    return RemoveDirectory(fullPath.c_str()) != 0;
  else if(!_wremove(fullPath.c_str()))
    return true;

  // So the file cannot be removed (probably because it's in use)?
  // Then let's move it somewhere else. And delete it on next startup.
  // Windows is so great!

  Path workaroundPath = Path::DATA;
  workaroundPath.append("old_" + path.last() + ".tmp");

  return rename(path, workaroundPath);
#else
  return !::remove(fullPath.c_str());
#endif
}

bool FS::removeRecursive(const Path &file)
{
  if(!remove(file))
    return false;

  Path dir = file;

  // remove empty directories, but not top-level ones that were created by REAPER
  while(dir.size() > 2) {
    dir.removeLast();

    if(!remove(dir))
      break;
  }

  return true;
}

bool FS::mtime(const Path &path, time_t *time)
{
  const Path &fullPath = Path::prefixRoot(path);

#ifdef _WIN32
  struct _stat st;

  if(_wstat(make_autostring(fullPath.join()).c_str(), &st))
    return false;
#else
  struct stat st;

  if(stat(fullPath.join().c_str(), &st))
    return false;
#endif

  *time = st.st_mtime;

  return true;
}

bool FS::exists(const Path &path)
{
  const Path &fullPath = Path::prefixRoot(path);
  return file_exists(fullPath.join().c_str());
}

void FS::mkdir(const Path &path)
{
  const Path &fullPath = Path::prefixRoot(path);
  RecursiveCreateDirectory(fullPath.join().c_str(), 0);
}

string FS::lastError()
{
  return strerror(errno);
}
