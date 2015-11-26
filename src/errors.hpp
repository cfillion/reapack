#ifndef REAPACK_ERRORS_HPP
#define REAPACK_ERRORS_HPP

#include <stdexcept>

class database_error : public std::runtime_error {
public:
  database_error(const char *what) : std::runtime_error(what) {}
};

#endif
