#include "bitboard.h"
#include "clock.h"
#include "logging.h"
#include "random.h"
#include <cassert>
#include <limits>

namespace {

void benchmark(const char *fname, void (*const f)(long), const Duration desired_time) {
  long iterations = 1;
  for (;;) {
    if(iterations==0) break;
    const Timestamp start_time = current_time();
    f(iterations);
    const Duration duration = current_time() - start_time;
    const double result = to_seconds(duration) / iterations;
    if (duration < desired_time / 10) {
      iterations *= 2;
    } else if (duration < desired_time / 2) {
      iterations = static_cast<long>(to_seconds(desired_time) / result);
    } else {
      log_always("%s: ", fname);
      if (result < 1e-6) {
        log_always("%.3f ns\n", result * 1e9);
      } else if (result < 1e-3) {
        log_always("%.3f us\n", result * 1e6);
      } else if (result < 1.0) {
        log_always("%.3f ms\n", result * 1e3);
      } else {
        log_always("%.3f s\n", result);
      }
      break;
    }
  }
}

#define BENCHMARK(f, desired_time) benchmark(#f, f, desired_time)

void benchmark_first_square(const long iterations) {
  long res = 0;
  for (long i = 0; i < iterations; ++i) {
    res += first_square(i);
  }
  assert(res < std::numeric_limits<long>::max());
}

void benchmark_last_square(const long iterations) {
  long res = 0;
  for (long i = 0; i < iterations; ++i) {
    res += last_square(i);
  }
  assert(res < std::numeric_limits<long>::max());
}

void benchmark_count_squares(const long iterations) {
  long res = 0;
  const Bitboard b = 0x5555555555555555u;
  for (long i = 0; i < iterations; ++i) {
    res += count_squares(b ^ i);
  }
  assert(res < std::numeric_limits<long>::max());
}

void benchmark_parity(const long iterations) {
  long res = 0;
  for (long i = 0; i < iterations; ++i) {
    res += parity(i);
  }
  assert(res < std::numeric_limits<long>::max());
}

void benchmark_nth_square(const long iterations) {
  long res = 0;
  const Bitboard b = 0x5555555555555555u;
  for (long i = 0; i < iterations; ++i) {
    res += nth_square(b, (i & 31));
  }
  assert(res < std::numeric_limits<long>::max());
}

void benchmark_current_time(const long iterations) {
  for (long i = 0; i < iterations; ++i) {
    current_time();
  }
}

void benchmark_random_int(const long iterations) {
  RandomGenerator rng;
  long res = 0;
  for (long i = 0; i < iterations; ++i) {
    res += rng.get_int(17);
  }
  assert(res < std::numeric_limits<long>::max());
}

void benchmark_random_square(const long iterations) {
  RandomGenerator rng;
  long res = 0;
  const Bitboard b = 0x5555555555555555u;
  for (long i = 0; i < iterations; ++i) {
    res += rng.get_square(b);
  }
  assert(res < std::numeric_limits<long>::max());
}

void benchmark_mult_32(const long iterations) {
  std::uint64_t res = 0;
  for (std::uint64_t i=0;i<std::uint64_t(iterations);++i) {
    res += std::uint32_t(i) * std::uint32_t(i);
  }
  assert(res < std::numeric_limits<std::uint64_t>::max());
}

void benchmark_mult_64(const long iterations) {
  std::uint64_t res = 0;
  for (std::uint64_t i=0;i<std::uint64_t(iterations);++i) {
    res += i * i;
  }
  assert(res < std::numeric_limits<std::uint64_t>::max());
}

}

int main() {
  const Duration t = std::chrono::milliseconds(400);
  BENCHMARK(benchmark_first_square, t);
  BENCHMARK(benchmark_last_square, t);
  BENCHMARK(benchmark_count_squares, t);
  BENCHMARK(benchmark_parity, t);
  BENCHMARK(benchmark_nth_square, t);
  BENCHMARK(benchmark_current_time, t);
  BENCHMARK(benchmark_random_int, t);
  BENCHMARK(benchmark_random_square, t);
  BENCHMARK(benchmark_mult_32, t);
  BENCHMARK(benchmark_mult_64, t);
}
