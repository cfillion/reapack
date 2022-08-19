#include <ostream>
#include <set>

class Path;
class Time;

std::ostream &operator<<(std::ostream &, const std::set<Path> &);

// include Catch only after having declared our ostream overloads
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_contains.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
