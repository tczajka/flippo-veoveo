#include "player_deterministic.h"

Move PlayerFirst::choose_move(const Position &position, const PlaySettings &) {
  const Bitboard transformed_moves =
    transform_bitboard(position.valid_moves(), symmetry);
  return untransform_square(first_square(transformed_moves), symmetry);
}

Move PlayerGreedy::choose_move(const Position &position, const PlaySettings &) {
  Bitboard remaining_transformed_moves =
    transform_bitboard(position.valid_moves(), symmetry);

  Move best_move = invalid_move;
  int best_score = -1;

  while (remaining_transformed_moves) {
    const Move transformed_move = first_square(remaining_transformed_moves);
    remaining_transformed_moves =
      reset_bit(remaining_transformed_moves, transformed_move);
    const Move move = untransform_square(transformed_move, symmetry);

    Position next_position;
    position.make_move(move, next_position);

    const int score = count_squares(next_position.opponent);
    if (score > best_score) {
      best_score = score;
      best_move = move;
    }
  }

  return best_move;
}
