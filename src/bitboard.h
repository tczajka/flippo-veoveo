#ifndef BITBOARD_H
#define BITBOARD_H

#include "arch.h"
#include <cstdint>
#include <string>

using Bitboard = std::uint64_t;

constexpr int num_squares = 64;

constexpr Bitboard single_square(const int sq) {
  return Bitboard{1} << sq;
}

constexpr Bitboard set_bit(const Bitboard b, const int sq) {
  return b | single_square(sq);
}

constexpr Bitboard reset_bit(const Bitboard b, const int sq) {
  return b & ~single_square(sq);
}

constexpr bool get_bit(const Bitboard b, const int sq) {
  return b & single_square(sq);
}

// TODO: Benchmark other implementations of first_square, last_square, count_squares,
// nth_square.

constexpr int first_square(const Bitboard b) {
  return __builtin_ctzll(b);
}

constexpr Bitboard remove_first_square(const Bitboard b) {
  return b & (b-1u);
}

constexpr int last_square(const Bitboard b) {
  return 63 - __builtin_clzll(b);
}

constexpr int count_squares(const Bitboard b) {
  return __builtin_popcountll(b);
}

constexpr int parity(const Bitboard b) {
  return __builtin_parityll(b);
}

constexpr int nth_square(Bitboard b, int index) {
  while (index) {
    b = remove_first_square(b);
    --index;
  }
  return first_square(b);
}

Bitboard bitboard_from_string(const std::string &str);
std::string bitboard_to_string_multiline(const Bitboard b);

constexpr Bitboard top_edge =    0x00000000000000ffu;
constexpr Bitboard bottom_edge = 0xff00000000000000u;
constexpr Bitboard left_edge =   0x0101010101010101u;
constexpr Bitboard right_edge =  0x8080808080808080u;
constexpr Bitboard corners =     0x8100000000000081u;

constexpr Bitboard neighbors(const Bitboard b) {
  const Bitboard b2 =
    b |
    ((b & ~left_edge) >> 1) |
    ((b & ~right_edge) << 1);

  const Bitboard b3 = b2 | (b2 >> 8) | (b2 << 8);

  return b3 ^ b;
}

constexpr int symmetry_x = 1;
constexpr int symmetry_y = 2;
constexpr int symmetry_swap = 4;
constexpr int num_symmetries = 8;

inline int transform_square(const int sq, const int symmetry) {
  if (symmetry == 0) return sq;
  int y = sq >> 3;
  int x = sq & 7;
  if (symmetry & symmetry_x) x ^= 7;
  if (symmetry & symmetry_y) y ^= 7;
  if (symmetry & symmetry_swap) std::swap(x,y);
  return (y<<3)|x;
}

inline int untransform_square(const int sq, const int symmetry) {
  if (symmetry == 0) return sq;
  int y = sq >> 3;
  int x = sq & 7;
  if (symmetry & symmetry_swap) std::swap(x,y);
  if (symmetry & symmetry_y) y ^= 7;
  if (symmetry & symmetry_x) x ^= 7;
  return (y<<3)|x;
}

Bitboard transform_bitboard(Bitboard b, int symmetry);

#endif
