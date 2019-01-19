#ifndef PLAYER_H
#define PLAYER_H

#include "clock.h"
#include "position.h"

struct PlaySettings {
  Timestamp start_time{};
  Duration time_left{};
  bool use_all_resources = false;
  bool quick_if_single_move = true;
  bool use_book = true;
};

class Player {
public:
  virtual ~Player() = default;

  virtual Move choose_move(const Position &position,
                           const PlaySettings &settings) = 0;

  virtual void opponent_move(const Position &, Move) {}
};

#endif
