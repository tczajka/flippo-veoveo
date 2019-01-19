#ifndef MATHEMATICS_H
#define MATHEMATICS_H

#include "arch.h"
#include <cstdint>

constexpr int64_t rounding_divide(const int64_t a, const int64_t b) {
  return (2 * a + (a >= 0 ? b : -b)) / (2 * b);
}

#endif
