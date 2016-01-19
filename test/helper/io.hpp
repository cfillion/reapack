#ifndef REAPACK_TEST_HELPER_IO_HPP
#define REAPACK_TEST_HELPER_IO_HPP

#include <ostream>
#include <set>

class Path;

std::ostream &operator<<(std::ostream &, const Path &);
std::ostream &operator<<(std::ostream &, const std::set<Path> &);

#endif
