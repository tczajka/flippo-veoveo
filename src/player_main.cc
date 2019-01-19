#include "clock.h"
#include "evaluator.h"
#include "hashing.h"
#include "logging.h"
#include "player_ab.h"
#include <cstdlib>
#include <iostream>

constexpr Duration default_game_time{std::chrono::milliseconds(4850)};

int main() {
#ifdef SUBMISSION
  verbosity = 1;
#endif

  std::string input;
  getline(std::cin, input);

  Timestamp start_time = current_time();
  Duration time_left = default_game_time;
  Position position = Position::initial();

  if (!std::cin) std::exit(EXIT_SUCCESS);

  for (;;) {
    if (input.substr(0,5) == "Time ") {
      time_left = std::chrono::milliseconds{std::stoi(input.substr(5))};
    } else if (input.substr(0, 9) == "Position ") {
      position = Position{input.substr(9)};
    } else {
      break;
    }

    getline(std::cin, input);
    start_time = current_time();
    if (!std::cin) std::exit(EXIT_SUCCESS);
  }

  init_hashing();
  init_evaluator();
  PlayerAB player;

  {
    const Timestamp t = current_time();
    log_info("Init time %.3f\n", to_seconds(t - start_time));
    time_left -= t - start_time;
    start_time = t;
  }

  for (;;) {
    const Move opponent_move = string_to_move(input);

    if (opponent_move != invalid_move) {
      if (!get_bit(position.valid_moves(), opponent_move)) {
        log_always("Invalid move\n");
        goto next_input;
      }
      player.opponent_move(position, opponent_move);
      position.make_move(opponent_move, position);
    } else if (input == "Start") {
      // Do nothing.
    } else if (input == "Quit") {
      std::exit(EXIT_SUCCESS);
    } else {
      log_always("Invalid input\n");
      goto next_input;
    }

    if (position.finished()) {
      log_always("Already_finished\n");
    } else {
      PlaySettings play_settings;
      play_settings.start_time = start_time;
      play_settings.time_left = time_left;
      const Move my_move = player.choose_move(position, play_settings);
      const std::string move_name = move_to_string(my_move);
      position.make_move(my_move, position);
      const Duration time_used = current_time() - start_time;
      time_left -= time_used;
      log_info("time left %.3f\n", to_seconds(time_left));
      std::cout << move_to_string(my_move) << '\n';
      std::cout.flush();
    }

next_input:
    std::fflush(stderr);
    getline(std::cin, input);
    start_time = current_time();
    if (!std::cin) std::exit(EXIT_SUCCESS);
  }
}
