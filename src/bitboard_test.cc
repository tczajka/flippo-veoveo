#include "bitboard.h"
#include "tests.h"

TEST(test_first_last_count_squares) {
  for (int sq = 0; sq < num_squares; ++sq) {
    const Bitboard b = single_square(sq);
    assert(first_square(b) == sq);
    assert(last_square(b) == sq);
    assert(count_squares(b) == 1);
  }

  for (int sq2 = 0; sq2 < num_squares; ++sq2) {
    for (int sq1 = 0; sq1 < sq2; ++sq1) {
      const Bitboard b = single_square(sq1) | single_square(sq2);
      assert(first_square(b) == sq1);
      assert(last_square(b) == sq2);
      assert(count_squares(b) == 2);
    }
  }

  assert(first_square(0xffffffffffffffffu) == 0);
  assert(first_square(0xfffffffffffffff0u) == 4);

  assert(last_square(0xffffffffffffffffu) == 63);
  assert(last_square(0x0fffffffffffffffu) == 59);

  assert(count_squares(0u) == 0);
  assert(count_squares(0xffffffffffffffffu) == 64);
  assert(count_squares(0x1010101010101010u) == 8);
}

TEST(test_parity) {
  assert(parity(0x1000000000000001u) == 0);
  assert(parity(0x1000010000000001u) == 1);
  assert(parity(0x1000030000000001u) == 0);
  assert(parity(0x9000030000000001u) == 1);
}

TEST(test_remove_first_square) {
  assert(remove_first_square(0xffffffffffffffffu) == 0xfffffffffffffffeu);
  assert(remove_first_square(0x0000030030300030u) == 0x0000030030300020u);
  assert(remove_first_square(0x8000000000000000u) == 0x0000000000000000u);
}

TEST(test_nth_square) {
  assert(nth_square(0x0100010101008000u, 0) == 15);
  assert(nth_square(0x0100010101008000u, 1) == 24);
  assert(nth_square(0x0100010101008000u, 2) == 32);
  assert(nth_square(0x0100010101008000u, 3) == 40);
  assert(nth_square(0x0100010101008000u, 4) == 56);
  for(int i=0;i<64;++i) {
    assert(nth_square(0xffffffffffffffffu, i) == i);
  }
}

TEST(test_bitboard_from_string) {
  assert(bitboard_from_string(
    "X......."
    "X.....X."
    "X......."
    "X......."
    ".X......"
    "..X....."
    "...XX..."
    "......XX")
    == 0xc018040201014101u);
}

TEST(test_bitboard_to_string_multiline) {
  assert(bitboard_to_string_multiline(0xc018040201014101u) ==
    "X.......\n"
    "X.....X.\n"
    "X.......\n"
    "X.......\n"
    ".X......\n"
    "..X.....\n"
    "...XX...\n"
    "......XX\n");
}

TEST(test_constants) {
  assert(top_edge == bitboard_from_string(
    "XXXXXXXX"
    "........"
    "........"
    "........"
    "........"
    "........"
    "........"
    "........"));

  assert(bottom_edge == bitboard_from_string(
    "........"
    "........"
    "........"
    "........"
    "........"
    "........"
    "........"
    "XXXXXXXX"));

  assert(left_edge == bitboard_from_string(
    "X......."
    "X......."
    "X......."
    "X......."
    "X......."
    "X......."
    "X......."
    "X......."));

  assert(right_edge == bitboard_from_string(
    ".......X"
    ".......X"
    ".......X"
    ".......X"
    ".......X"
    ".......X"
    ".......X"
    ".......X"));

  assert(corners == bitboard_from_string(
    "X......X"
    "........"
    "........"
    "........"
    "........"
    "........"
    "........"
    "X......X"));
}

TEST(test_neighbors) {
  assert(neighbors(bitboard_from_string(
    ".......X"
    "....X..."
    "....X..."
    "....XXX."
    "..XXX..."
    "....X..."
    "X......."
    ".......X")) ==
    bitboard_from_string(
    "...XXXX."
    "...X.XXX"
    "...X.XXX"
    ".XXX...X"
    ".X...XXX"
    "XXXX.X.."
    ".X.XXXXX"
    "XX....X."));
}

TEST(test_transform_square) {
  for (int symmetry=0;symmetry < num_symmetries; ++symmetry) {
    for (int sq = 0; sq < num_squares; ++sq) {
      const int sq2 = transform_square(sq, symmetry);
      assert(sq2>=0 && sq2<num_squares);
      assert(untransform_square(sq2, symmetry) == sq);
    }
  }
}

TEST(test_transform_bitboard) {
  const Bitboard a = bitboard_from_string(
    "........"
    "....X..."
    "...X.X.."
    "..X..X.."
    "..X..X.."
    "..XXXX.."
    "..X..X.."
    "........");

  const Bitboard b = bitboard_from_string(
    "........"
    "........"
    "..XXXXX."
    ".X...X.."
    "..X..X.."
    "...XXXX."
    "........"
    "........");

  assert(transform_bitboard(a, symmetry_x | symmetry_swap) == b);
  assert(transform_bitboard(b, symmetry_y | symmetry_swap) == a);
}
