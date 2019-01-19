#include "player_random.h"

Move PlayerRandom::choose_move(const Position &position, const PlaySettings &) {
  return random_generator.get_square(position.valid_moves());
}
