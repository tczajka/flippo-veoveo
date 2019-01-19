#include "clock.h"
#include "evaluator.h"
#include "hashing.h"
#include "logging.h"
#include "mathematics.h"
#include "player_ab.h"
#include "referee_util.h"
#include <iostream>

int main() {
  verbosity = 0;
  init_hashing();
  init_evaluator();

  constexpr int initial_stones = 8;
  constexpr Duration time_per_move = std::chrono::milliseconds(100);

  std::vector<std::int64_t> to_move_bonus(num_squares+1, 0);
  const std::vector<Position> starting_positions =
    generate_starting_positions(initial_stones);

  for (size_t i=0; i<starting_positions.size(); ++i) {
    log_always("Game %zu/%zu\n", i, starting_positions.size());
    Position pos = starting_positions[i];
    for (;;) {
      to_move_bonus[pos.move_number()] -= evaluate(pos);

      if(pos.finished()) break;

      PlayerAB player;
      PlaySettings settings;
      settings.start_time = current_time();
      settings.time_left = time_per_move;
      settings.use_all_resources = true;

      const Move move = player.choose_move(pos, settings);
      pos.make_move(move, pos);
    }

    const Milliscore final_score = pos.final_score() << milliscore_bits;

    for(int m=initial_stones; m<=num_squares; ++m) {
      if(m%2==0) to_move_bonus[m] += final_score;
      else to_move_bonus[m] -= final_score;
    }
  }

  for (auto &b : to_move_bonus) {
    b = rounding_divide(b, starting_positions.size());
  }

  for (int m=0;m<=num_squares;++m) {
    std::cout << (evaluator_to_move_bonus[m] + to_move_bonus[m]) << ", ";
    if (m%10==9) std::cout << "\n";
  }
  std::cout << "\n";
}
