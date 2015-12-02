#ifndef REAPACK_TEST_HELPER_IO_HPP
#define REAPACK_TEST_HELPER_IO_HPP

#include <ostream>

class Path;

std::ostream &operator<<(std::ostream &, const Path &);

#endif
