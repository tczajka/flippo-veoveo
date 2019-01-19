#include "referee_util.h"
#include "hashing.h"

void generate_starting_positions_rec(const Position &position,
                                     const int move_number,
                                     std::vector<Position> &starting_positions,
                                     PositionHashTable<bool> &seen) {
  Position normalized_position;
  position.normalize(normalized_position);
  bool seen_inserted;
  seen.insert(normalized_position, seen_inserted, true);
  assert(!seen.out_of_memory());
  if (!seen_inserted) return;

  if (position.move_number() == move_number) {
    starting_positions.push_back(position);
  } else {
    Bitboard valid_moves = position.valid_moves();
    while (valid_moves) {
      const Move move = first_square(valid_moves);
      valid_moves = reset_bit(valid_moves, move);
      Position pos2;
      position.make_move(move, pos2);
      generate_starting_positions_rec(pos2, move_number,
                                      starting_positions, seen);
    }
  }
}

std::vector<Position> generate_starting_positions(const int move_number) {
  PositionHashTable<bool> seen(1<<22);
  std::vector<Position> starting_positions;
  generate_starting_positions_rec(Position::initial(), move_number,
                                  starting_positions, seen);
  log_always("Starting positions: %zu\n", starting_positions.size());

  return starting_positions;
}

