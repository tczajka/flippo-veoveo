#include "bitboard.h"
#include <cassert>

Bitboard bitboard_from_string(const std::string &str) {
  assert(str.size() == num_squares);
  Bitboard b = 0;
  for (int sq = 0; sq < num_squares; ++sq) {
    switch (str[sq]) {
      case '.':
        break;
      case 'X':
        b = set_bit(b, sq);
        break;
      default:
        assert(false);
    }
  }
  return b;
}

std::string bitboard_to_string_multiline(const Bitboard b) {
  std::string str;
  for (int y=0;y<8;++y) {
    for (int x=0;x<8;++x) {
      if (get_bit(b, 8*y+x)) str += 'X';
      else str += '.';
    }
    str += '\n';
  }

  return str;
}

Bitboard transform_bitboard(Bitboard b, const int symmetry) {
  if (symmetry == 0) return b;
  Bitboard b2 = 0;
  while (b) {
    const int sq = first_square(b);
    b = remove_first_square(b);
    b2 = set_bit(b2, transform_square(sq, symmetry));
  }
  return b2;
}
