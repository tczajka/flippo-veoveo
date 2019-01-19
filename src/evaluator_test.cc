#include "evaluator.h"
#include "random.h"
#include "tests.h"
#include <cstring>

TEST(test_base_3) {
  assert(power_of_3(0) == 1u);
  assert(power_of_3(8) == 6561u);

  assert(base_2_to_3_table[0] == 0u);
  assert(base_2_to_3_table[9] == 28u);
  assert(base_2_to_3_table[0xff] == 3280u);

  assert(base_2_to_3_table_rev6[0b100111] ==
         base_2_to_3_table[0b111001]);

  assert(base_2_to_3_table_rev7[0b1000111] ==
         base_2_to_3_table[0b1110001]);

  assert(base_2_to_3_table_rev8[0b10000111] ==
         base_2_to_3_table[0b11100001]);
}

TEST(test_flip_multipliers) {
  // ...
  assert(flip_multipliers_3[encode_base_3(0,0)] == 0);
  // .X.
  assert(flip_multipliers_3[encode_base_3(2,0)] == 0);
  // XOX
  // 1, 1, 1
  assert(flip_multipliers_3[encode_base_3(5,2)] == 0x404040);
  // XOX.
  //  ^ will be flipped 50-50 (mult=0)
  //   ^ will definitely be flipped (mult=-1)
  assert(flip_multipliers_4[encode_base_3(5,2)] == 0x00c00040);
  // OXX..
  //  ^ will be flipped 50-50
  //   ^ will be flipped if we get:
  //     left to right: XX XO OX
  //     right to left: OX OO
  //     5/8 probability of flippage, multiplier = 3/8-5/8 = -1/4
  assert(flip_multipliers_5[encode_base_3(6,1)] == 0x00f00040);
}

template <int start, int dir, int len>
void test_compress_line_specific() {
  RandomGenerator rng;
  for (int iter=0;iter<100;++iter) {
    const Bitboard b = rng.get_bitboard();
    const Bitboard line = compress_line<start, dir, len>(b);
    assert(line < single_square(len));
    for (int i=0;i<len;++i) {
      assert(get_bit(line, i) == get_bit(b, start + i * dir));
    }
  }
}

TEST(test_compress_line) {
  // Rows.
  test_compress_line_specific< 0, 1, 8>();
  test_compress_line_specific< 8, 1, 8>();
  test_compress_line_specific<16, 1, 8>();
  test_compress_line_specific<24, 1, 8>();
  test_compress_line_specific<32, 1, 8>();
  test_compress_line_specific<40, 1, 8>();
  test_compress_line_specific<48, 1, 8>();
  test_compress_line_specific<56, 1, 8>();

  // Columns.
  test_compress_line_specific<0, 8, 8>();
  test_compress_line_specific<1, 8, 8>();
  test_compress_line_specific<2, 8, 8>();
  test_compress_line_specific<3, 8, 8>();
  test_compress_line_specific<4, 8, 8>();
  test_compress_line_specific<5, 8, 8>();
  test_compress_line_specific<6, 8, 8>();
  test_compress_line_specific<7, 8, 8>();

  // Direction -7
  test_compress_line_specific<16, -7, 3>();
  test_compress_line_specific<24, -7, 4>();
  test_compress_line_specific<32, -7, 5>();
  test_compress_line_specific<40, -7, 6>();
  test_compress_line_specific<48, -7, 7>();
  test_compress_line_specific<56, -7, 8>();
  test_compress_line_specific<57, -7, 7>();
  test_compress_line_specific<58, -7, 6>();
  test_compress_line_specific<59, -7, 5>();
  test_compress_line_specific<60, -7, 4>();
  test_compress_line_specific<61, -7, 3>();

  // Direction 9
  test_compress_line_specific< 5, 9, 3>();
  test_compress_line_specific< 4, 9, 4>();
  test_compress_line_specific< 3, 9, 5>();
  test_compress_line_specific< 2, 9, 6>();
  test_compress_line_specific< 1, 9, 7>();
  test_compress_line_specific< 0, 9, 8>();
  test_compress_line_specific< 8, 9, 7>();
  test_compress_line_specific<16, 9, 6>();
  test_compress_line_specific<24, 9, 5>();
  test_compress_line_specific<32, 9, 4>();
  test_compress_line_specific<40, 9, 3>();
}

TEST(test_transpose) {
  uint64_t columns[8] = {};
  for (int x=0;x<8;++x) {
    for (int y=0;y<8;++y) {
      columns[x] |= uint64_t(8*y+x) << (8*y);
    }
  }

  __m128i doublerows[4];
  transpose(columns, doublerows);
  for (int y=0;y<4;++y) {
    for (int x=0;x<16;++x) {
      const unsigned char *const p =
        reinterpret_cast<const unsigned char*>(&doublerows[y]);
      assert(p[x] == 16 * y + x);
    }
  }
}

TEST(test_transpose_diag7) {
  uint64_t diag7[8] = {};
  for (int x=0;x<8;++x) {
    for (int y=0;y<8;++y) {
      diag7[x] |= uint64_t(8*y+(x-y+8)%8) << (8*y);
    }
  }

  __m128i doublerows[4];
  transpose_diag7(diag7, doublerows);
  for (int y=0;y<4;++y) {
    for (int x=0;x<16;++x) {
      const unsigned char *const p = reinterpret_cast<const unsigned char*>(&doublerows[y]);
      assert(p[x] == 16 * y + x);
    }
  }
}

TEST(test_transpose_diag9) {
  uint64_t diag9[8] = {};
  for (int x=0;x<8;++x) {
    for (int y=0;y<8;++y) {
      diag9[x] |= uint64_t(8*y+(x+y)%8) << (8*y);
    }
  }

  __m128i doublerows[4];
  transpose_diag9(diag9, doublerows);
  for (int y=0;y<4;++y) {
    for (int x=0;x<16;++x) {
      const unsigned char *const p = reinterpret_cast<const unsigned char*>(&doublerows[y]);
      assert(p[x] == 16 * y + x);
    }
  }
}

TEST(test_sum_expected) {
  __m128i expected[8];
  int total = 0;
  for (int y=0;y<8;++y) {
    char *row = reinterpret_cast<char*>(&expected[y]);
    for (int x=0;x<8;++x) {
      const int16_t val = (1<<11) - (8*y) - x;
      memcpy(row+2*x, &val, 2);
      total += val;
    }
  }
  assert(sum_expected(expected) == total);
}

TEST(test_sum_expected_negative) {
  __m128i expected[8];
  int total = 0;
  for (int y=0;y<8;++y) {
    char *row = reinterpret_cast<char*>(&expected[y]);
    for (int x=0;x<8;++x) {
      const int16_t val = -(1<<11) + (8*y) + x;
      memcpy(row+2*x, &val, 2);
      total += val;
    }
  }
  assert(sum_expected(expected) == total);
}

TEST(test_expand8to16) {
  __m128i doublerows[4];
  for (int y=0;y<4;++y) {
    char *doublerow = reinterpret_cast<char*>(&doublerows[y]);
    for (int x=0;x<16;++x) {
      doublerow[x] = y*16+x-32;
    }
  }

  __m128i rows[8];
  expand8to16(doublerows, rows);
  for (int y=0;y<8;++y) {
    const char *row = reinterpret_cast<const char*>(&rows[y]);
    for (int x=0;x<8;++x) {
      int16_t rowx;
      memcpy(&rowx, row+2*x, 2);
      assert(rowx == (y*8+x-32)*256);
    }
  }
}

TEST(test_evalute) {
  const Position position(
      "........"
      "........"
      "........"
      "........"
      "........"
      "........"
      "........"
      "XOOXXXOX");
  assert(position.to_move() == 0); // O to move
  assert(evaluate_expected(position) == (-1 << milliscore_bits));

  RandomGenerator rng;
  for (int i=0;i<100;++i) {
    const Bitboard p = rng.get_bitboard();
    const Position position2(p, ~p);
    assert(position2.finished());
    assert(evaluate(position2) == position2.final_score() << milliscore_bits);
  }
}

