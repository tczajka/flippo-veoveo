#ifndef PLAYER_RANDOM_H
#define PLAYER_RANDOM_H

#include "player.h"
#include "position.h"
#include "random.h"

class PlayerRandom : public Player {
public:
  Move choose_move(const Position &position,
                   const PlaySettings &settings) override;

private:
  RandomGenerator random_generator;
};

#endif
