#ifndef REAPACK_TEMPFILE_HPP
#define REAPACK_TEMPFILE_HPP

#include "path.hpp"

#include <fstream>

class TempFile {
public:
  TempFile(const Path &target);
  TempFile(const TempFile &) = delete;
  ~TempFile();

  const Path &targetPath() const { return m_target; }
  const Path &tempPath() const { return m_temp; }

  bool open();
  void close();
  bool save();

  operator std::ofstream &() { return m_stream; }

private:
  std::ofstream m_stream;

  Path m_target;
  Path m_temp;

  bool m_created;
  bool m_saved;
};

#endif
