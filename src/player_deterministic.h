#ifndef PLAYER_DETERMINISTIC_H
#define PLAYER_DETERMINISTIC_H

#include "player.h"

class PlayerFirst : public Player {
public:
  PlayerFirst(const int _symmetry) : symmetry{_symmetry} {}

  Move choose_move(const Position &position,
                   const PlaySettings &settings) override;

private:
  int symmetry;
};

class PlayerGreedy : public Player {
public:
  PlayerGreedy(const int _symmetry) : symmetry{_symmetry} {}

  Move choose_move(const Position &position,
                   const PlaySettings &settings) override;

private:
  int symmetry;
};

#endif
