#include "bitboard.h"
#include "evaluator.h"
#include "logging.h"
#include "random.h"
#include <algorithm>
#include <cassert>
#include <cinttypes>
#include <iostream>

namespace {

bool check_magic(const Bitboard mask, const int result_bits,
                 const Bitboard magic) {
  bool seen[1l << result_bits] = {};
  Bitboard pos = mask;
  for(;;) {
    Bitboard h = (magic * pos) >> (num_squares - result_bits);
    if (seen[h]) return false;
    seen[h] = true;

    if (!pos) break;
    pos = (pos-1u) & mask;
  }

  return true;
}

void print_magic(const Bitboard mask, const int result_bits) {
  RandomGenerator rng;

  Bitboard magic;
  for (;;) {
    magic = rng.get_bitboard() & rng.get_bitboard() & rng.get_bitboard();
    if (check_magic(mask, result_bits, magic)) break;
  }
  log_always("%016" PRIx64 " -> %d %016" PRIx64 "\n",
             mask, result_bits, magic);
}

void verify_simple_magic(int start, int dir, int len) {
  assert(check_magic(compute_line_mask(start, dir, len), len,
                     compute_line_multiplier(start, dir, len)));
}

} // end namespace

int main() {
  log_always("Columns\n");
  for (int i=0;i<8;++i) {
    verify_simple_magic(i, 8, 8);
  }

  log_always("Direction 7\n");
  verify_simple_magic(40, -7, 6);
  verify_simple_magic(48, -7, 7);
  verify_simple_magic(56, -7, 8);
  verify_simple_magic(57, -7, 7);
  verify_simple_magic(58, -7, 6);
  {
    const Bitboard mask = compute_line_mask(16, -7, 3) |
                          compute_line_mask(59, -7, 5);
    print_magic(mask, 8);
  }
  {
    const Bitboard mask = compute_line_mask(24, -7, 4) |
                          compute_line_mask(60, -7, 4);
    print_magic(mask, 8);
  }
  {
    const Bitboard mask = compute_line_mask(32, -7, 5) |
                          compute_line_mask(61, -7, 3);
    print_magic(mask, 8);
  }
  log_always("Direction 9\n");
  verify_simple_magic(2, 9, 6);
  verify_simple_magic(1, 9, 7);
  verify_simple_magic(0, 9, 8);
  verify_simple_magic(8, 9, 7);
  verify_simple_magic(16, 9, 6);
  {
    const Bitboard mask = compute_line_mask(5, 9, 3) |
                          compute_line_mask(24, 9, 5);
    print_magic(mask, 8);
  }
  {
    const Bitboard mask = compute_line_mask(4, 9, 4) |
                          compute_line_mask(32, 9, 4);
    print_magic(mask, 8);
  }
  {
    const Bitboard mask = compute_line_mask(3, 9, 5) |
                          compute_line_mask(40, 9, 3);
    print_magic(mask, 8);
  }

}
