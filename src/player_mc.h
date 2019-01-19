#ifndef PLAYER_MC_H
#define PLAYER_MC_H

#include "player.h"
#include "position.h"
#include "random.h"

class PlayerMc : public Player {
public:
  Move choose_move(const Position &position,
                   const PlaySettings &settings) override;

private:
  struct MoveOption {
    Move move = invalid_move;
    std::int64_t total_score = 0;
  };

  int simulate(const Position &position);

  RandomGenerator random_generator;
};

#endif
