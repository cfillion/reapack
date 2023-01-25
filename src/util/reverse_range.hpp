#pragma once

template <class R> constexpr auto reverse_range(R &&r) {
  using It = decltype(r.rbegin());

  struct range {
    constexpr It begin() { return rbegin; }
    constexpr It end() { return rend; }
    It rbegin, rend;
  };

  return range{r.rbegin(), r.rend()};
}
