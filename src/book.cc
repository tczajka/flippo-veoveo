#include "book.h"
#include <cassert>
#include <cmath>

Move find_book_move(const Position &position) {
  Position normalized_position;
  const int symmetry = position.normalize(normalized_position);

  const std::string key = encode_position(normalized_position);
  const auto it =
    std::lower_bound(book_entries, book_entries + num_book_entries, key);
  if (it == book_entries + num_book_entries) {
    return invalid_move;
  }

  const std::string s = *it;
  if (s.size() != key.size() + 1u || s.substr(0, key.size()) != key) {
    return invalid_move;
  }
  return untransform_square(decode_square(s[key.size()]), symmetry);
}

char encode_square(int sq) {
  assert(sq >= 0 && sq < 64);
  if (sq < 26) return 'A' + sq;
  else if (sq < 52) return 'a' + (sq-26);
  else if (sq < 62) return '0' + (sq-52);
  else if (sq==62) return '+';
  else return '-';
}

int decode_square(char c) {
  if (c >= 'A' && c <= 'Z') return c - 'A';
  else if (c >= 'a' && c <= 'z') return 26 + (c-'a');
  else if (c >= '0' && c <= '9') return 52 + (c-'0');
  else if (c=='+') return 62;
  else if (c=='-') return 63;
  else {
    assert(false);
    return -1;
  }
}

std::string encode_bitboard(Bitboard b) {
  std::string res;
  while (b) {
    const int sq = first_square(b);
    b = reset_bit(b, sq);
    res += encode_square(sq);
  }
  res += '/';
  return res;
}

std::string encode_position(const Position &p) {
  return encode_bitboard(p.player) + encode_bitboard(p.opponent);
}
