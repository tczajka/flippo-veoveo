#ifndef PREPARED_H
#define PREPARED_H

#include "position.h"

struct Prepared {
  int color;
  Score score;
  Move moves[num_squares];
};

// invalid_move if not found
Move find_prepared_game(int move_number,
                        const Move (&moves_so_far)[num_squares]);

#endif
