#ifndef EVALUATOR_H
#define EVALUATOR_H

#include "position.h"
#include <cassert>
#include <cstring>
#include <tmmintrin.h>

using Milliscore = int32_t;
constexpr int milliscore_bits = 20;
constexpr Milliscore max_milliscore = max_score << milliscore_bits;

constexpr Milliscore corner_move_bonus = 384 << 10;
constexpr Milliscore opponent_corner_move_bonus = 256 << 10;
constexpr Milliscore mobility_bonus = 3000;
constexpr Milliscore opponent_mobility_bonus = 2000;
extern const Milliscore evaluator_to_move_bonus[num_squares+1];

constexpr uint32_t power_of_3(const int n) {
  return n==0 ? 1u : 3u * power_of_3(n-1);
}

extern uint16_t base_2_to_3_table[1<<8];
extern uint16_t base_2_to_3_table_rev6[1<<6];
extern uint16_t base_2_to_3_table_rev7[1<<7];
extern uint16_t base_2_to_3_table_rev8[1<<8];
extern uint64_t flip_multipliers_1[power_of_3(1)];
extern uint64_t flip_multipliers_2[power_of_3(2)];
extern uint64_t flip_multipliers_3[power_of_3(3)];
extern uint64_t flip_multipliers_4[power_of_3(4)];
extern uint64_t flip_multipliers_5[power_of_3(5)];
extern uint64_t flip_multipliers_6[power_of_3(6)];
extern uint64_t flip_multipliers_7[power_of_3(7)];
extern uint64_t flip_multipliers_8[power_of_3(8)];

// For player, opponent line, at most 8 squares.
inline uint32_t encode_base_3(const uint32_t player, const uint32_t opponent) {
  return (base_2_to_3_table[player] << 1) + base_2_to_3_table[opponent];
}

inline uint32_t encode_base_3_rev6(const uint32_t player, const uint32_t opponent) {
  return (base_2_to_3_table_rev6[player] << 1) + base_2_to_3_table_rev6[opponent];
}
inline uint32_t encode_base_3_rev7(const uint32_t player, const uint32_t opponent) {
  return (base_2_to_3_table_rev7[player] << 1) + base_2_to_3_table_rev7[opponent];
}
inline uint32_t encode_base_3_rev8(const uint32_t player, const uint32_t opponent) {
  return (base_2_to_3_table_rev8[player] << 1) + base_2_to_3_table_rev8[opponent];
}

constexpr Bitboard magic_7_35 = 0x0801800280201046u;
constexpr Bitboard magic_7_44 = 0x01004004003c4104u;
constexpr Bitboard magic_7_53 = 0x1100444004001101u;
constexpr Bitboard magic_9_35 = 0x00100c0840040802u;
constexpr Bitboard magic_9_44 = 0x0024005020008228u;
constexpr Bitboard magic_9_53 = 0x0040408a01400802u;

void init_evaluator();

constexpr Bitboard compute_line_mask(int start, int dir, int len) {
  return len==0 ?
    Bitboard{0} :
    single_square(start) | compute_line_mask(start + dir, dir, len-1);
}

constexpr Bitboard compute_line_multiplier(int start, int dir, int len) {
  // If dir==1: all bits are same.
  // If dir>=8: inputs are separated by 8+
  // If dir==-7: multiplier bits are separated by 8
  return 
    len==0 ?
    Bitboard{0} :
    single_square(64-len-start) |
        compute_line_multiplier(start + dir, dir, len-1);
}

template <int start, int dir, int len>
inline Bitboard compress_line(const Bitboard board) {
  constexpr Bitboard mask = compute_line_mask(start, dir, len);
  constexpr Bitboard multiplier = compute_line_multiplier(start, dir, len);

  return ((board & mask) * multiplier) >> (64 - len);
}

inline void expand8to16(const __m128i (&doublerows)[4], __m128i (&rows)[8]) {
  const __m128i zero = _mm_setzero_si128();

  rows[0] = _mm_unpacklo_epi8(zero, doublerows[0]);
  rows[1] = _mm_unpackhi_epi8(zero, doublerows[0]);
  rows[2] = _mm_unpacklo_epi8(zero, doublerows[1]);
  rows[3] = _mm_unpackhi_epi8(zero, doublerows[1]);
  rows[4] = _mm_unpacklo_epi8(zero, doublerows[2]);
  rows[5] = _mm_unpackhi_epi8(zero, doublerows[2]);
  rows[6] = _mm_unpacklo_epi8(zero, doublerows[3]);
  rows[7] = _mm_unpackhi_epi8(zero, doublerows[3]);
}

inline void transpose(const uint64_t (&columns)[8],
                      __m128i (&doublerows)[4]) {
  const __m128i col01 = _mm_set_epi64x(columns[1], columns[0]);
  const __m128i col23 = _mm_set_epi64x(columns[3], columns[2]);
  const __m128i col45 = _mm_set_epi64x(columns[5], columns[4]);
  const __m128i col67 = _mm_set_epi64x(columns[7], columns[6]);

  const __m128i col44_00 = _mm_castps_si128(
      _mm_shuffle_ps(_mm_castsi128_ps(col01),
                     _mm_castsi128_ps(col23),
                     0b10001000u));

  const __m128i col44_10 = _mm_castps_si128(
      _mm_shuffle_ps(_mm_castsi128_ps(col01),
                     _mm_castsi128_ps(col23),
                     0b11011101u));

  const __m128i col44_01 = _mm_castps_si128(
      _mm_shuffle_ps(_mm_castsi128_ps(col45),
                     _mm_castsi128_ps(col67),
                     0b10001000u));

  const __m128i col44_11 = _mm_castps_si128(
      _mm_shuffle_ps(_mm_castsi128_ps(col45),
                     _mm_castsi128_ps(col67),
                     0b11011101u));

  const __m128i transpose44 = _mm_setr_epi8(
       0,  4,  8, 12,
       1,  5,  9, 13,
       2,  6, 10, 14,
       3,  7, 11, 15);

  // need SSSE3
  const __m128i row44_00 = _mm_shuffle_epi8(col44_00, transpose44);
  const __m128i row44_10 = _mm_shuffle_epi8(col44_10, transpose44);
  const __m128i row44_01 = _mm_shuffle_epi8(col44_01, transpose44);
  const __m128i row44_11 = _mm_shuffle_epi8(col44_11, transpose44);

  doublerows[0] = _mm_unpacklo_epi32(row44_00, row44_01);
  doublerows[1] = _mm_unpackhi_epi32(row44_00, row44_01);
  doublerows[2] = _mm_unpacklo_epi32(row44_10, row44_11);
  doublerows[3] = _mm_unpackhi_epi32(row44_10, row44_11);
}

inline void transpose_diag7(const uint64_t (&diag7)[8],
                            __m128i (&doublerows)[4]) {
  __m128i transposed[4];
  transpose(diag7, transposed);

  const __m128i perm0 = _mm_setr_epi8(
       0,  1,  2,  3,  4,  5,  6,  7,
       9, 10, 11, 12, 13, 14, 15,  8);  

  const __m128i perm1 = _mm_setr_epi8(
      18, 19, 20, 21, 22, 23, 16, 17,
      27, 28, 29, 30, 31, 24, 25, 26);

  const __m128i perm2 = _mm_setr_epi8(
      36, 37, 38, 39, 32, 33, 34, 35,
      45, 46, 47, 40, 41, 42, 43, 44);

  const __m128i perm3 = _mm_setr_epi8(
      54, 55, 48, 49, 50, 51, 52, 53,
      63, 56, 57, 58, 59, 60, 61, 62);

  doublerows[0] = _mm_shuffle_epi8(transposed[0], perm0);
  doublerows[1] = _mm_shuffle_epi8(transposed[1], perm1);
  doublerows[2] = _mm_shuffle_epi8(transposed[2], perm2);
  doublerows[3] = _mm_shuffle_epi8(transposed[3], perm3);
}

inline void transpose_diag9(const uint64_t (&diag9)[8],
                            __m128i (&doublerows)[4]) {
  __m128i transposed[4];
  transpose(diag9, transposed);

  const __m128i perm0 = _mm_setr_epi8(
       0,  1,  2,  3,  4,  5,  6,  7,
       15, 8,  9, 10, 11, 12, 13, 14);

  const __m128i perm1 = _mm_setr_epi8(
      22, 23, 16, 17, 18, 19, 20, 21,
      29, 30, 31, 24, 25, 26, 27, 28);

  const __m128i perm2 = _mm_setr_epi8(
      36, 37, 38, 39, 32, 33, 34, 35,
      43, 44, 45, 46, 47, 40, 41, 42);

  const __m128i perm3 = _mm_setr_epi8(
      50, 51, 52, 53, 54, 55, 48, 49,
      57, 58, 59, 60, 61, 62, 63, 56);

  doublerows[0] = _mm_shuffle_epi8(transposed[0], perm0);
  doublerows[1] = _mm_shuffle_epi8(transposed[1], perm1);
  doublerows[2] = _mm_shuffle_epi8(transposed[2], perm2);
  doublerows[3] = _mm_shuffle_epi8(transposed[3], perm3);
}

inline Milliscore sum_expected(const __m128i (&expected)[8]) {
  const __m128i sum01 = _mm_add_epi16(expected[0], expected[1]);
  const __m128i sum23 = _mm_add_epi16(expected[2], expected[3]);
  const __m128i sum45 = _mm_add_epi16(expected[4], expected[5]);
  const __m128i sum67 = _mm_add_epi16(expected[6], expected[7]);

  const __m128i sum03 = _mm_add_epi16(sum01, sum23);
  const __m128i sum47 = _mm_add_epi16(sum45, sum67);

  // 8 * 11-bit = 14-bit. No overflow!
  const __m128i sum_rows = _mm_add_epi16(sum03, sum47);

  int16_t p[8];
  memcpy(p, &sum_rows, 16);
  return ((p[0]+p[1])+(p[2]+p[3])) + ((p[4]+p[5])+(p[6]+p[7]));
}

Milliscore evaluate_expected(const Position &position);
Milliscore evaluate(const Position &position);

#endif
