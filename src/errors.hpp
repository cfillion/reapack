#ifndef REAPACK_ERRORS_HPP
#define REAPACK_ERRORS_HPP

#include <stdexcept>

class reapack_error : public std::runtime_error {
public:
  reapack_error(const char *what) : std::runtime_error(what) {}
};

#endif
