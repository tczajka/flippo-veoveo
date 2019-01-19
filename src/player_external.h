#ifndef PLAYER_EXTERNAL_H
#define PLAYER_EXTERNAL_H

#include "player.h"

class PlayerExternal : public Player {
public:
  PlayerExternal(const std::string &path,
                 const std::string &log_path,
                 const Position &initial_position,
                 Duration game_time);

  ~PlayerExternal() override;

  Move choose_move(const Position &position,
                   const PlaySettings &settings) override;

  void opponent_move(const Position &, Move move) override;

private:
  void check_error(int ret);

  pid_t child_pid;
  FILE *commanding_file;
  FILE *moves_file;
  Move last_opponent_move = invalid_move;
};

#endif
