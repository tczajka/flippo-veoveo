#ifndef RANDOM_H
#define RANDOM_H

#include "bitboard.h"
#include <random>

std::random_device &get_random_device();

class RandomGenerator {
public:
  RandomGenerator();

  int get_int(const int n) {
    //return std::uniform_int_distribution<int>{0, n-1}(urng);
    return urng() % n;
  }

  int get_square(const Bitboard b) {
    // TODO: Combine count_squares with nth_square.
    const int n = count_squares(b);
    return nth_square(b, get_int(n));
  }

  Bitboard get_bitboard() {
    static_assert(urng.min() == 0u && urng.max() == 0xffffffffu, "");
    return (Bitboard{urng()} << 32) | urng();
  }

private:
  std::mt19937 urng;
};

#endif
