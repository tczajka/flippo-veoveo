#include "evaluator.h"
#include <algorithm>

extern const Milliscore evaluator_to_move_bonus[num_squares+1] = {
300000, -300000, 300000, -300000, 300000, -300000, 300000, -300000, 300715, -274012, 
256020, -270022, 247653, -268498, 240827, -268044, 226796, -258760, 213369, -251181, 
212493, -266549, 222934, -282305, 239812, -296185, 239149, -296478, 232778, -278798, 
223058, -281699, 191180, -281218, 202595, -314669, 223572, -292186, 189998, -277340, 
206835, -270630, 202887, -301031, 221187, -241518, 260672, -231828, 251568, -170309, 
243459, -177243, 359010, -176368, 453418, -132531, 491565, -107849, 494569,   37625, 
423728,  -26931, 294690,   14918,      0, 
};



uint16_t base_2_to_3_table[1<<8];
uint16_t base_2_to_3_table_rev6[1<<6];
uint16_t base_2_to_3_table_rev7[1<<7];
uint16_t base_2_to_3_table_rev8[1<<8];
uint64_t flip_multipliers_1[power_of_3(1)];
uint64_t flip_multipliers_2[power_of_3(2)];
uint64_t flip_multipliers_3[power_of_3(3)];
uint64_t flip_multipliers_4[power_of_3(4)];
uint64_t flip_multipliers_5[power_of_3(5)];
uint64_t flip_multipliers_6[power_of_3(6)];
uint64_t flip_multipliers_7[power_of_3(7)];
uint64_t flip_multipliers_8[power_of_3(8)];
uint64_t flip_multipliers_17[power_of_3(7)];
uint64_t flip_multipliers_26[power_of_3(6)];
uint64_t flip_multipliers_35[power_of_3(8)];
uint64_t flip_multipliers_44[power_of_3(8)];
uint64_t flip_multipliers_53[power_of_3(8)];
uint64_t flip_multipliers_62[power_of_3(6)];
uint64_t flip_multipliers_71[power_of_3(7)];

uint64_t row_expected[power_of_3(8)];

uint16_t magic_power3_table_7_35[1<<8];
uint16_t magic_power3_table_7_44[1<<8];
uint16_t magic_power3_table_7_53[1<<8];
uint16_t magic_power3_table_9_35[1<<8];
uint16_t magic_power3_table_9_44[1<<8];
uint16_t magic_power3_table_9_53[1<<8];

void init_base_2_to_3() {
  base_2_to_3_table[0] = 0;
  for (unsigned i=1;i<(1u<<8);++i) {
    base_2_to_3_table[i] = 3u * base_2_to_3_table[i>>1] + (i&1u);
  }

  base_2_to_3_table_rev6[0] = 0;
  for (unsigned i=1;i<(1u<<6);++i) {
    base_2_to_3_table_rev6[i] =
      base_2_to_3_table_rev6[i>>1] / 3u + (i&1u) * power_of_3(5);
  }

  base_2_to_3_table_rev7[0] = 0;
  for (unsigned i=1;i<(1u<<7);++i) {
    base_2_to_3_table_rev7[i] =
      base_2_to_3_table_rev7[i>>1] / 3u + (i&1u) * power_of_3(6);
  }

  base_2_to_3_table_rev8[0] = 0;
  for (unsigned i=1;i<(1u<<8);++i) {
    base_2_to_3_table_rev8[i] =
      base_2_to_3_table_rev8[i>>1] / 3u + (i&1u) * power_of_3(7);
  }
}

template <int len>
void init_flip_multipliers(uint64_t (&multipliers)[power_of_3(len)]) {
  double multipliers_double[power_of_3(len)][len];

  constexpr uint32_t mask = (1u << len) - 1u;

  for (uint32_t empty=0; empty <= mask; ++empty) {
    uint32_t player = mask ^ empty;
    for(;;) {
      const uint32_t opponent = mask ^ empty ^ player;

      const auto encoded = encode_base_3(player, opponent);
      double (&cur_multipliers)[len] = multipliers_double[encoded];

      if (empty) {
        double sum_prob_multiplier[len] = {};
        for (int move = 0; move < len; ++move) {
          if (empty & (1u << move)) {
            for (int who=0; who<2; ++who) {
              Position pos(player, opponent);
              if (who==1) std::swap(pos.player, pos.opponent);
              pos.make_move(move, pos);
              if (who==0) std::swap(pos.player, pos.opponent);

              const uint32_t flipped =
                (player ^ pos.player) & (player | opponent);

              const double (&next_multipliers)[len] =
                multipliers_double[encode_base_3(pos.player & mask,
                                                 pos.opponent & mask)];

              for (int i=0;i<len;++i) {
                double m = next_multipliers[i];
                if (flipped & (1u << i)) m = -m;
                sum_prob_multiplier[i] += m;
              }
            } // for who
          } // if (empty & (1<<move))
        } // for (move)

        for (int i=0;i<len;++i) {
          if ((player|opponent) & (1u << i)) {
            cur_multipliers[i] =
              sum_prob_multiplier[i] / (2 * count_squares(empty));
          } else {
            cur_multipliers[i] = 0.0;
          }
        }
      } // if (empty)
      else {
        for (int i=0;i<len;++i) cur_multipliers[i] = 1.0;
      }

      uint64_t res = 0;
      for (int i=0;i<len;++i) {
        const double a = round(cur_multipliers[i] * 64.0);
        assert(a >= -64.0 && a <= 64.0);
        const int8_t b = static_cast<int8_t>(a);
        const uint8_t c = static_cast<uint8_t>(b);
        res |= uint64_t{c} << (8 * i);
      }
      multipliers[encoded] = res;

      if (!player) break;
      player = (player - 1u) & ~empty;
    }
  }
}

void init_row_expected() {
  constexpr uint32_t mask = (1u << 8) - 1u;
  for (uint32_t empty=0; empty <= mask; ++empty) {
    uint32_t player = mask ^ empty;
    for(;;) {
      const uint32_t opponent = mask ^ empty ^ player;
      const auto encoded = encode_base_3(player, opponent);
      const uint64_t multipliers = flip_multipliers_8[encoded];

      uint64_t expected = 0;
      for (int i=0;i<8;++i) {
        const uint8_t m = multipliers >> (8 * i);
        uint8_t e = 0;
        if (player & (1u << i)) {
          e = m;
        } else if (opponent & (1u << i)) {
          e = -m;
        }
        expected |= uint64_t{e} << (8 * i);
      }

      row_expected[encoded] = expected;

      if (!player) break;
      player = (player - 1u) & ~empty;
    }
  }
}

template <int len1, int len2>
void init_combined_flip_multipliers(
    const uint64_t (&multipliers1)[power_of_3(len1)],
    const uint64_t (&multipliers2)[power_of_3(len2)],
    uint64_t (&multipliers_combined)[power_of_3((len1>2?len1:0)+(len2>2?len2:0))]) {
  constexpr int len1a = len1>2 ? len1 : 0;
  constexpr int len2a = len2>2 ? len2 : 0;
  for (uint32_t p1 = 0; p1 < power_of_3(len1a); ++p1) {
    const uint64_t m1 =
      len1 == 1 ? 0x40u :
      len1 == 2 ? 0x4040u :
      multipliers1[p1];

    for (uint32_t p2 = 0; p2 < power_of_3(len2a); ++p2) {
      const uint64_t m2 =
        len2 == 1 ? 0x40u :
        len2 == 2 ? 0x4040u :
        multipliers2[p2];

      const uint32_t p = p1 + p2 * power_of_3(len1a);

      multipliers_combined[p] = m1 | (m2 << (8 * len1));
    }
  }
}

template <int start1, int dir1, int len1,
          int start2, int dir2, int len2,
          Bitboard magic>
void init_magic_power3_table(uint16_t (&magic_power3_table)[1<<(len1+len2)]) {
  for (uint32_t m1 = 0; m1 < (1u << len1); ++m1) {
    for (uint32_t m2 = 0; m2 < (1u << len2); ++m2) {
      Bitboard pos = 0;
      for (int i=0;i<len1;++i) {
        if(m1 & (1u << i)) pos |= single_square(start1 + i * dir1);
      }
      for (int i=0;i<len2;++i) {
        if(m2 & (1u << i)) pos |= single_square(start2 + i * dir2);
      }
      const Bitboard h = (pos * magic) >> 56;
      assert(magic_power3_table[h] == 0);
      magic_power3_table[h] =
        base_2_to_3_table[m1] + power_of_3(len1) * base_2_to_3_table[m2];
    }
  }
}

void init_evaluator() {
  init_base_2_to_3();

  init_flip_multipliers<1>(flip_multipliers_1);
  init_flip_multipliers<2>(flip_multipliers_2);
  init_flip_multipliers<3>(flip_multipliers_3);
  init_flip_multipliers<4>(flip_multipliers_4);
  init_flip_multipliers<5>(flip_multipliers_5);
  init_flip_multipliers<6>(flip_multipliers_6);
  init_flip_multipliers<7>(flip_multipliers_7);
  init_flip_multipliers<8>(flip_multipliers_8);

  init_row_expected();

  init_combined_flip_multipliers<1,7>(flip_multipliers_1,
                                      flip_multipliers_7,
                                      flip_multipliers_17);
  init_combined_flip_multipliers<2,6>(flip_multipliers_2,
                                      flip_multipliers_6,
                                      flip_multipliers_26);
  init_combined_flip_multipliers<3,5>(flip_multipliers_3,
                                      flip_multipliers_5,
                                      flip_multipliers_35);
  init_combined_flip_multipliers<4,4>(flip_multipliers_4,
                                      flip_multipliers_4,
                                      flip_multipliers_44);
  init_combined_flip_multipliers<5,3>(flip_multipliers_5,
                                      flip_multipliers_3,
                                      flip_multipliers_53);
  init_combined_flip_multipliers<6,2>(flip_multipliers_6,
                                      flip_multipliers_2,
                                      flip_multipliers_62);
  init_combined_flip_multipliers<7,1>(flip_multipliers_7,
                                      flip_multipliers_1,
                                      flip_multipliers_71);

  init_magic_power3_table<2, 7, 3, 31, 7, 5, magic_7_35>(magic_power3_table_7_35);
  init_magic_power3_table<3, 7, 4, 39, 7, 4, magic_7_44>(magic_power3_table_7_44);
  init_magic_power3_table<4, 7, 5, 47, 7, 3, magic_7_53>(magic_power3_table_7_53);

  init_magic_power3_table<5, 9, 3, 24, 9, 5, magic_9_35>(magic_power3_table_9_35);
  init_magic_power3_table<4, 9, 4, 32, 9, 4, magic_9_44>(magic_power3_table_9_44);
  init_magic_power3_table<3, 9, 5, 40, 9, 3, magic_9_53>(magic_power3_table_9_53);
}

template <int row>
inline uint64_t evaluate_row(const Position &position) {
  const unsigned char player = reinterpret_cast<const unsigned char*>(&position.player)[row];
  const unsigned char opponent = reinterpret_cast<const unsigned char*>(&position.opponent)[row];
  return row_expected[encode_base_3(player, opponent)];
}

inline void multiply_expected(__m128i (&a)[8], const __m128i (&doubleb)[4]) {
  __m128i b[8];
  expand8to16(doubleb, b);

  a[0] = _mm_mulhrs_epi16(a[0], b[0]);
  a[1] = _mm_mulhrs_epi16(a[1], b[1]);
  a[2] = _mm_mulhrs_epi16(a[2], b[2]);
  a[3] = _mm_mulhrs_epi16(a[3], b[3]);
  a[4] = _mm_mulhrs_epi16(a[4], b[4]);
  a[5] = _mm_mulhrs_epi16(a[5], b[5]);
  a[6] = _mm_mulhrs_epi16(a[6], b[6]);
  a[7] = _mm_mulhrs_epi16(a[7], b[7]);
}

inline void evaluate_rows(const Position &position, __m128i (&expected)[8]) {
  __m128i doublerows[4];
  doublerows[0] = _mm_set_epi64x(evaluate_row<1>(position), evaluate_row<0>(position));
  doublerows[1] = _mm_set_epi64x(evaluate_row<3>(position), evaluate_row<2>(position));
  doublerows[2] = _mm_set_epi64x(evaluate_row<5>(position), evaluate_row<4>(position));
  doublerows[3] = _mm_set_epi64x(evaluate_row<7>(position), evaluate_row<6>(position));

  expand8to16(doublerows, expected);
}

template <int col>
inline uint64_t evaluate_column_flips(const Position &position) {
  const Bitboard player   = compress_line<col, 8, 8>(position.player);
  const Bitboard opponent = compress_line<col, 8, 8>(position.opponent);
  return flip_multipliers_8[encode_base_3(player, opponent)];
}

inline void multiply_columns(const Position &position, __m128i (&expected)[8]) {
  uint64_t columns[8];
  columns[0] = evaluate_column_flips<0>(position);
  columns[1] = evaluate_column_flips<1>(position);
  columns[2] = evaluate_column_flips<2>(position);
  columns[3] = evaluate_column_flips<3>(position);
  columns[4] = evaluate_column_flips<4>(position);
  columns[5] = evaluate_column_flips<5>(position);
  columns[6] = evaluate_column_flips<6>(position);
  columns[7] = evaluate_column_flips<7>(position);

  __m128i flips[4];
  transpose(columns, flips);
  multiply_expected(expected, flips);
}

inline void multiply_diag7(const Position &position, __m128i (&expected)[8]) {
  uint64_t diag7[8];

  {
    const Bitboard player   = compress_line<57, -7, 7>(position.player);
    const Bitboard opponent = compress_line<57, -7, 7>(position.opponent);
    diag7[0] = flip_multipliers_17[encode_base_3_rev7(player, opponent)];
  }
  {
    const Bitboard player   = compress_line<58, -7, 6>(position.player);
    const Bitboard opponent = compress_line<58, -7, 6>(position.opponent);
    diag7[1] = flip_multipliers_26[encode_base_3_rev6(player, opponent)];
  }
  {
    constexpr Bitboard mask = compute_line_mask( 2, 7, 3) |
                              compute_line_mask(31, 7, 5);
    const Bitboard player   = ((position.player   & mask) * magic_7_35) >> 56;
    const Bitboard opponent = ((position.opponent & mask) * magic_7_35) >> 56;
    const uint32_t encoded =
      (magic_power3_table_7_35[player] << 1) +
       magic_power3_table_7_35[opponent];
    diag7[2] = flip_multipliers_35[encoded];
  }
  {
    constexpr Bitboard mask = compute_line_mask( 3, 7, 4) |
                              compute_line_mask(39, 7, 4);
    const Bitboard player   = ((position.player   & mask) * magic_7_44) >> 56;
    const Bitboard opponent = ((position.opponent & mask) * magic_7_44) >> 56;
    const uint32_t encoded =
      (magic_power3_table_7_44[player] << 1) +
       magic_power3_table_7_44[opponent];
    diag7[3] = flip_multipliers_44[encoded];
  }
  {
    constexpr Bitboard mask = compute_line_mask( 4, 7, 5) |
                              compute_line_mask(47, 7, 3);
    const Bitboard player   = ((position.player   & mask) * magic_7_53) >> 56;
    const Bitboard opponent = ((position.opponent & mask) * magic_7_53) >> 56;
    const uint32_t encoded =
      (magic_power3_table_7_53[player] << 1) +
       magic_power3_table_7_53[opponent];
    diag7[4] = flip_multipliers_53[encoded];
  }
  {
    const Bitboard player   = compress_line<40, -7, 6>(position.player);
    const Bitboard opponent = compress_line<40, -7, 6>(position.opponent);
    diag7[5] = flip_multipliers_62[encode_base_3_rev6(player, opponent)];
  }
  {
    const Bitboard player   = compress_line<48, -7, 7>(position.player);
    const Bitboard opponent = compress_line<48, -7, 7>(position.opponent);
    diag7[6] = flip_multipliers_71[encode_base_3_rev7(player, opponent)];
  }
  {
    const Bitboard player   = compress_line<56, -7, 8>(position.player);
    const Bitboard opponent = compress_line<56, -7, 8>(position.opponent);
    diag7[7] = flip_multipliers_8[encode_base_3_rev8(player, opponent)];
  }

  __m128i flips[4];
  transpose_diag7(diag7, flips);
  multiply_expected(expected, flips);
}

inline void multiply_diag9(const Position &position, __m128i (&expected)[8]) {
  uint64_t diag9[8];

  {
    const Bitboard player   = compress_line<0, 9, 8>(position.player);
    const Bitboard opponent = compress_line<0, 9, 8>(position.opponent);
    diag9[0] = flip_multipliers_8[encode_base_3(player, opponent)];
  }
  {
    const Bitboard player   = compress_line<1, 9, 7>(position.player);
    const Bitboard opponent = compress_line<1, 9, 7>(position.opponent);
    diag9[1] = flip_multipliers_71[encode_base_3(player, opponent)];
  }
  {
    const Bitboard player   = compress_line<2, 9, 6>(position.player);
    const Bitboard opponent = compress_line<2, 9, 6>(position.opponent);
    diag9[2] = flip_multipliers_62[encode_base_3(player, opponent)];
  }
  {
    constexpr Bitboard mask = compute_line_mask( 3, 9, 5) |
                              compute_line_mask(40, 9, 3);
    const Bitboard player   = ((position.player   & mask) * magic_9_53) >> 56;
    const Bitboard opponent = ((position.opponent & mask) * magic_9_53) >> 56;
    const uint32_t encoded =
      (magic_power3_table_9_53[player] << 1) +
       magic_power3_table_9_53[opponent];
    diag9[3] = flip_multipliers_53[encoded];
  }
  {
    constexpr Bitboard mask = compute_line_mask( 4, 9, 4) |
                              compute_line_mask(32, 9, 4);
    const Bitboard player   = ((position.player   & mask) * magic_9_44) >> 56;
    const Bitboard opponent = ((position.opponent & mask) * magic_9_44) >> 56;
    const uint32_t encoded =
      (magic_power3_table_9_44[player] << 1) +
       magic_power3_table_9_44[opponent];
    diag9[4] = flip_multipliers_44[encoded];
  }
  {
    constexpr Bitboard mask = compute_line_mask( 5, 9, 3) |
                              compute_line_mask(24, 9, 5);
    const Bitboard player   = ((position.player   & mask) * magic_9_35) >> 56;
    const Bitboard opponent = ((position.opponent & mask) * magic_9_35) >> 56;
    const uint32_t encoded =
      (magic_power3_table_9_35[player] << 1) +
       magic_power3_table_9_35[opponent];
    diag9[5] = flip_multipliers_35[encoded];
  }
  {
    const Bitboard player   = compress_line<16, 9, 6>(position.player);
    const Bitboard opponent = compress_line<16, 9, 6>(position.opponent);
    diag9[6] = flip_multipliers_26[encode_base_3(player, opponent)];
  }
  {
    const Bitboard player   = compress_line< 8, 9, 7>(position.player);
    const Bitboard opponent = compress_line< 8, 9, 7>(position.opponent);
    diag9[7] = flip_multipliers_17[encode_base_3(player, opponent)];
  }

  __m128i flips[4];
  transpose_diag9(diag9, flips);
  multiply_expected(expected, flips);
}

Milliscore evaluate_expected(const Position &position) {
  // Each row is 8 x 2 bytes
  __m128i expected[8];

  // 14-bit.
  evaluate_rows(position, expected);

  // 13-bit.
  multiply_columns(position, expected);

  // 12-bit.
  multiply_diag7(position, expected);

  // 11-bit
  multiply_diag9(position, expected);

  Milliscore result = sum_expected(expected);

  // Each stone is worth 1/2 score.
  result <<= (milliscore_bits - 11 - 1);

  return result;
}

Milliscore evaluate(const Position &position) {
  Milliscore result = evaluate_expected(position);

  // Add a bonus for the person to move.
  result += evaluator_to_move_bonus[position.move_number()];

  const Bitboard valid_moves = position.valid_moves();
  const Bitboard valid_moves_opponent =
    Position(position.opponent, position.player).valid_moves_capturing();

  if (valid_moves & corners) {
    result += corner_move_bonus;
  }

  if (valid_moves_opponent & corners & ~valid_moves) {
    result -= opponent_corner_move_bonus;
  }

  result += count_squares(valid_moves) * mobility_bonus;

  result -= count_squares(valid_moves_opponent) * opponent_mobility_bonus;

  return result;
}
