#include "position.h"
#include <cassert>

std::string move_to_string(const Move move) {
  return std::string{static_cast<char>('A' + (move >> 3)),
                     static_cast<char>('1' + (move & 7))};
}

Move string_to_move(const std::string &str) {
  if (str.size() == 2 &&
      str[0] >= 'A' && str[0] <= 'H' &&
      str[1] >= '1' && str[1] <= '8') {
    return ((str[0]-'A') << 3) | (str[1]-'1');
  } else {
    return invalid_move;
  }
}

Position Position::initial() {
  return Position(0x0000001008000000u,
                  0x0000000810000000u);
}

Position::Position(const std::string &str) :
    player(0), opponent(0) {
  assert(str.size() == num_squares);
  for (int sq = 0; sq < num_squares; ++sq) {
    switch (str[sq]) {
      case 'O':
        player = set_bit(player, sq);
        break;
      case 'X':
        opponent = set_bit(opponent, sq);
        break;
      case '.':
        break;
      default:
        assert(false);
    }
  }
  if (to_move() == 1) {
    std::swap(player, opponent);
  }
}

std::string Position::to_string(const bool multiline) const {
  char player_char = 'O';
  char opponent_char = 'X';
  if (to_move() == 1) {
    std::swap(player_char, opponent_char);
  }

  std::string str;
  for (int y=0;y<8;++y) {
    for (int x=0;x<8;++x) {
      const Bitboard b = single_square(8*y+x);
      if (player & b) str += player_char;
      else if (opponent & b) str += opponent_char;
      else str += '.';
    }
    if (multiline) str += '\n';
  }

  return str;
}

template<int dir>
inline Bitboard valid_moves_one_dir(const Bitboard player, const Bitboard all) {
  const Bitboard flippable_l1 = all & (player << dir);
  const Bitboard flippable_r1 = all & (player >> dir);
  const Bitboard flippable_l2 = flippable_l1 | (all & (flippable_l1 << dir));
  const Bitboard flippable_r2 = flippable_r1 | (all & (flippable_r1 >> dir));
  const Bitboard all_neighbor_r = all & (all << dir);
  const Bitboard all_neighbor_l = all & (all >> dir);
  const Bitboard flippable_l4 = flippable_l2 | (all_neighbor_r & (flippable_l2 << (2*dir)));
  const Bitboard flippable_r4 = flippable_r2 | (all_neighbor_l & (flippable_r2 >> (2*dir)));
  const Bitboard flippable_l6 = flippable_l4 | (all_neighbor_r & (flippable_l4 << (2*dir)));
  const Bitboard flippable_r6 = flippable_r4 | (all_neighbor_l & (flippable_r4 >> (2*dir)));

  return (flippable_l6 << dir) | (flippable_r6 >> dir);
}

Bitboard Position::valid_moves_capturing() const {
  const Bitboard all = player | opponent;
  const Bitboard all_middle = all & ~(left_edge | right_edge);

  const Bitboard moves1 = valid_moves_one_dir<1>(player, all_middle);
  const Bitboard moves8 = valid_moves_one_dir<8>(player, all);
  const Bitboard moves7 = valid_moves_one_dir<7>(player, all_middle);
  const Bitboard moves9 = valid_moves_one_dir<9>(player, all_middle);

  const Bitboard pseudo_moves = (moves1 | moves8) | (moves7 | moves9);
  const Bitboard moves = pseudo_moves & ~all;

  return moves;
}

Bitboard Position::valid_moves_slow() const {
  const Bitboard n = neighbors(player | opponent);

  Bitboard n_rem = n;
  Bitboard valid = 0;
  while (n_rem) {
    const int sq = first_square(n_rem);
    const Bitboard b_sq = single_square(sq);
    Position pos2;
    if (make_move(sq, pos2)) valid |= b_sq;
    n_rem ^= b_sq;
  }

  if (!valid) valid = n;

  return valid;
}

template <int move, int dir, Bitboard edge_f, Bitboard edge_b>
Bitboard get_move_flips_one_dir(const Position &position) {
  const Bitboard all = position.player | position.opponent;
  constexpr Bitboard move_board = single_square(move);

  constexpr Bitboard ray_f1 = (move_board & ~edge_f) << dir;
  constexpr Bitboard ray_f2 = (ray_f1 & ~edge_f) << dir;

  Bitboard flips_f = 0;
  if (ray_f2) {
    constexpr Bitboard ray_f3 = (ray_f2 & ~edge_f) << dir;
    constexpr Bitboard ray_f4 = (ray_f3 & ~edge_f) << dir;
    constexpr Bitboard ray_f5 = (ray_f4 & ~edge_f) << dir;
    constexpr Bitboard ray_f6 = (ray_f5 & ~edge_f) << dir;
    constexpr Bitboard ray_f7 = (ray_f6 & ~edge_f) << dir;
    constexpr Bitboard ray_f =
      ray_f1 | ray_f2 | ray_f3 | ray_f4 |
      ray_f5 | ray_f6 | ray_f7;

    const Bitboard all_f_filled =
      dir == 1 ? all : all | ~ray_f;
    const Bitboard consecutive_f =
      (all_f_filled ^ (all_f_filled + ray_f1)) & ray_f;
    const Bitboard player_f = position.player & consecutive_f;
    if (player_f) {
      const Bitboard last_player_f = single_square(last_square(player_f));
      flips_f = (last_player_f - 1u) & ray_f;
    }
  }

  constexpr Bitboard ray_b1 = (move_board & ~edge_b) >> dir;
  constexpr Bitboard ray_b2 = (ray_b1 & ~edge_b) >> dir;

  Bitboard flips_b = 0;
  if (ray_b2) {
    constexpr Bitboard ray_b3 = (ray_b2 & ~edge_b) >> dir;
    constexpr Bitboard ray_b4 = (ray_b3 & ~edge_b) >> dir;
    constexpr Bitboard ray_b5 = (ray_b4 & ~edge_b) >> dir;
    constexpr Bitboard ray_b6 = (ray_b5 & ~edge_b) >> dir;
    constexpr Bitboard ray_b7 = (ray_b6 & ~edge_b) >> dir;
    constexpr Bitboard ray_b =
      ray_b1 | ray_b2 | ray_b3 | ray_b4 |
      ray_b5 | ray_b6 | ray_b7;

    Bitboard consecutive_b = all & ray_b;
    const Bitboard empty_b = ~all & ray_b;
    if (empty_b) {
      const Bitboard first_empty_b = single_square(last_square(empty_b));
      consecutive_b &= ~(first_empty_b - 1u);
    }
    const Bitboard player_b = position.player & consecutive_b;
    if (player_b) {
      const Bitboard behind_player_b = player_b ^ (player_b - 1u);
      flips_b = consecutive_b & ~behind_player_b;
    }
  }

  return flips_f | flips_b;
}

template <int move>
Bitboard get_move_flips(const Position &position) {
  const Bitboard flips1 =
    get_move_flips_one_dir<move, 1, right_edge, left_edge>(position);
  const Bitboard flips8 =
    get_move_flips_one_dir<move, 8, bottom_edge, top_edge>(position);
  const Bitboard flips7 =
    get_move_flips_one_dir<move, 7, bottom_edge | left_edge, top_edge | right_edge>(
        position);
  const Bitboard flips9 =
    get_move_flips_one_dir<move, 9, bottom_edge | right_edge, top_edge | left_edge>(
        position);
  return (flips1 | flips8) | (flips7 | flips9);
}

using GetMoveFlipsFunction = Bitboard (*const)(const Position &);

constexpr GetMoveFlipsFunction get_move_flips_table[64] = {
#define GMFT1(i) get_move_flips<i>, get_move_flips<i+1>, get_move_flips<i+2>, get_move_flips<i+3>,
#define GMFT2(i) GMFT1(i) GMFT1(i+4) GMFT1(i+8) GMFT1(i+12)
  GMFT2(0) GMFT2(16) GMFT2(32) GMFT2(48)
#undef GMFT2
#undef GMFT1
};

bool Position::make_move(const Move move, Position &new_position) const {
  const Bitboard flipped = get_move_flips_table[move](*this);
  new_position = Position(opponent ^ flipped, set_bit(player ^ flipped, move));
  return flipped != 0;
}

bool Position::make_move_slow(const Move move, Position &new_position) const {
  const int move_y = move >> 3;
  const int move_x = move & 7;

  Bitboard flipped = 0;

  for (int dy = -1; dy <= 1; ++dy) {
    for (int dx = -1; dx <= 1; ++dx) {
      if (dy == 0 && dx == 0) continue;
      int y = move_y + dy;
      int x = move_x + dx;
      Bitboard farthest_flipped = 0;
      Bitboard cur_flipped = 0;
      while (y >= 0 && y < 8 && x >= 0 && x < 8) {
        const Bitboard b_sq = single_square(8 * y + x);
        if (player & b_sq) {
          farthest_flipped = cur_flipped;
        } else if (!(opponent & b_sq)) {
          break;
        }
        cur_flipped |= b_sq;
        y += dy;
        x += dx;
      }
      flipped |= farthest_flipped;
    }
  }

  new_position = Position(opponent ^ flipped, (player ^ flipped) | single_square(move));
  return flipped != 0;
}

Position Position::transform(const int symmetry) const {
  return Position(transform_bitboard(player, symmetry),
                  transform_bitboard(opponent, symmetry));
}

int Position::normalize(Position &normalized_position) const {
  int symmetry_used = 0;
  normalized_position = *this;

  for (int symmetry = 1; symmetry < num_symmetries; ++symmetry) {
    const Position p = transform(symmetry);
    if (p < normalized_position) {
      normalized_position = p;
      symmetry_used = symmetry;
    }
  }

  return symmetry_used;
}
