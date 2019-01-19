#include "player_mc.h"
#include "logging.h"
#include <algorithm>

Move PlayerMc::choose_move(const Position &position,
                           const PlaySettings &settings) {
  const int rounds_left = (num_squares - position.move_number() + 1) / 2;
  const Timestamp end_time = settings.start_time + settings.time_left / rounds_left;

  std::vector<MoveOption> move_options;
  {
    Bitboard valid_moves = position.valid_moves();
    while (valid_moves) {
      const Move move = first_square(valid_moves);
      MoveOption move_option;
      move_option.move = move;
      move_options.push_back(move_option);
      valid_moves = remove_first_square(valid_moves);
    }
  }

  std::int64_t num_simulations = 0;
  std::int64_t num_rounds = 0;
  while (num_rounds == 0 || current_time() < end_time) {
    for (MoveOption &move_option : move_options) {
      Position pos2;
      position.make_move(move_option.move, pos2);

      move_option.total_score -= simulate(pos2);
      ++num_simulations;
    }
    ++num_rounds;
  }

  std::sort(move_options.begin(), move_options.end(),
            [](const MoveOption &a, const MoveOption &b) {
              return a.total_score > b.total_score;
            });

  log_info("Simulations %ld\n", num_simulations);
  log_info("Best moves:");
  for (std::size_t i=0; i < move_options.size() && i < 3; ++i) {
    log_info(" %s (%.3f)", move_to_string(move_options[i].move).c_str(),
             static_cast<double>(move_options[i].total_score) / num_rounds);
  }
  log_info("\n");

  return move_options.begin()->move;
}

int PlayerMc::simulate(const Position &position) {
  Position pos2 = position;
  int rounds = 0;
  while (!pos2.finished()) {
    const Move move = random_generator.get_square(pos2.valid_moves());
    pos2.make_move(move, pos2);
    ++rounds;
  }
  const int score = pos2.final_score();
  return (rounds & 1) ? -score : score;
}
