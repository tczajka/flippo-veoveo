#ifndef POSITION_H
#define POSITION_H

#include "bitboard.h"
#include <cstdint>
#include <string>

using Move = std::int8_t;
using Score = std::int8_t;

constexpr Move invalid_move = -1;

// First 4 squares cover 16. Each new square adds at most 5.
// So needs at least 10 more moves to get the whole board.
// 16 + 9*5 = 61.
// This leaves at most 50 empty squares.
static constexpr int max_moves = 50;

// Way outside bounds.
static constexpr Score max_score = 100;

std::string move_to_string(Move move);

// invalid_move if invalid
Move string_to_move(const std::string &str);

class Position {
public:
  static Position initial();

  Position() : player(0), opponent(0) {}
  Position(const Bitboard _player, const Bitboard _opponent) :
    player(_player),
    opponent(_opponent) {
  }

  Position(const std::string &str);

  std::string to_string(bool multiline=false) const;

  int move_number() const {
    return count_squares(player | opponent);
  }

  int to_move() const {
    return parity(player | opponent);
  }

  Bitboard valid_moves_capturing() const;
  Bitboard valid_moves() const {
    const Bitboard moves = valid_moves_capturing();
    return moves ? moves : neighbors(player|opponent);
  }

  Bitboard valid_moves_slow() const;

  Bitboard empty_squares() const {
    return ~(player | opponent);
  }

  // Returns whether anything flipped.
  bool make_move(Move move, Position &new_position) const;

  bool make_move_slow(Move move, Position &new_position) const;

  bool finished() const {
    return (~(player | opponent)) == Bitboard{0};
  }

  // Score for next to move = white.
  Score final_score() const {
    return static_cast<Score>(count_squares(player) - num_squares / 2);
  }

  bool operator==(const Position &other) const {
    return player == other.player && opponent == other.opponent;
  }

  bool operator!=(const Position &other) const {
    return !(*this == other);
  }

  bool operator<(const Position &other) const {
    if (player != other.player) return player < other.player;
    return opponent < other.opponent;
  }

  Position transform(int symmetry) const;

  // Return symmetry used.
  int normalize(Position &normalized_position) const;

  Bitboard player;
  Bitboard opponent;
};

#endif
