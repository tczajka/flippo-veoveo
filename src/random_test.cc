#include "random.h"
#include "tests.h"
#include <cmath>

TEST(test_random_get_int) {
  const int n = 6;
  const int iter = 1000000;

  RandomGenerator rng;

  int cnt[n] = {};
  for (int i=0; i<iter; ++i) {
    ++cnt[rng.get_int(n)];
  }

  const double p = 1.0 / n;
  const double mean = iter * p;
  const double variance = iter * p * (1.0 - p);
  const double stddev = std::sqrt(variance);
  assert(stddev < 0.1 * mean);

  for (int i=0;i<n;++i) {
    assert(cnt[i] > mean - 4.0 * stddev);
    assert(cnt[i] < mean + 4.0 * stddev);
  }
}

TEST(test_random_get_square) {
  RandomGenerator rng;

  const int iter = 100000;

  int total = 0;

  for (int i = 0; i < iter; ++i) {
    const Bitboard b = rng.get_bitboard();
    assert(b);

    const int sq = rng.get_square(b);
    assert(get_bit(b, sq));
    total += sq;
  }

  const double mean = iter * 63.0 / 2;
  const double variance = iter * (64.0 * 64.0 - 1) / 12.0;
  const double stddev = std::sqrt(variance);
  assert(stddev < 0.1 * mean);
  assert(total > mean - 4.0 * stddev);
  assert(total < mean + 4.0 * stddev);
}

TEST(test_random_get_bitboard) {
  RandomGenerator rng;
  int cnt[num_squares] = {};
  const int iter = 100000;
  for(int i=0;i<iter;++i) {
    const Bitboard b = rng.get_bitboard();
    for (int sq=0; sq<num_squares; ++sq) {
      cnt[sq] += get_bit(b, sq);
    }
  }
  const double mean = iter * 0.5;
  const double variance = iter * 0.25;
  const double stddev = std::sqrt(variance);
  for (int sq=0; sq<num_squares; ++sq) {
    assert(cnt[sq] > mean - 4.0 * stddev);
    assert(cnt[sq] < mean + 4.0 * stddev);
  }
}
