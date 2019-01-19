#include "clock.h"
#include "hashing.h"
#include "logging.h"
#include "player_ab.h"
#include "prob_cut.h"
#include "referee_util.h"
#include <algorithm>
#include <fstream>

namespace {
  struct ProbCutStats {
    double num_samples;
    double total_offset;
    double total_offset_squared;
  };

  ProbCutStats prob_cut_stats[num_squares][max_probcut_depth+1][max_probcut_depth];
}

int main() {
  verbosity = 0;
  init_hashing();
  init_evaluator();

  constexpr int initial_stones = 8;
  constexpr Duration time_per_move = std::chrono::milliseconds(100);

  const std::vector<Position> starting_positions =
    generate_starting_positions(initial_stones);

  for (size_t i=0; i<starting_positions.size(); ++i) {
    log_always("Game %zu/%zu\n", i, starting_positions.size());
    Position pos = starting_positions[i];

    for(;;) {
      if(pos.finished()) break;

      PlayerAB player;

      Milliscore evals[max_probcut_depth+1] = {};
      for (int depth = 0; depth <= max_probcut_depth; ++depth) {
        evals[depth] = player.evaluate_depth(pos, depth);
      }

      for (int deep = 0; deep <= max_probcut_depth; ++deep) {
        for (int shallow = 0; shallow < deep; ++shallow) {
          double offset = evals[shallow] - evals[deep];
          auto &stats = prob_cut_stats[pos.move_number()][deep][shallow];
          stats.num_samples += 1.0;
          stats.total_offset += offset;
          stats.total_offset_squared += offset * offset;
        }
      }

      PlaySettings settings;
      settings.start_time = current_time();
      settings.time_left = time_per_move;
      settings.use_all_resources = true;

      const Move move = player.choose_move(pos, settings);
      pos.make_move(move, pos);
    }
  }

  std::ofstream f("prob_cut_info_long.tmp");
  f << "#include \"prob_cut.h\"\n";
  f << "extern const ProbCutInfo prob_cut_info_long[num_squares][max_probcut_depth+1][max_probcut_depth] = {\n";
  for (int move_number=0;move_number<num_squares;++move_number) {
    f << "// move " << move_number << "\n";
    f << "{\n";
    for (int deep=0;deep<=max_probcut_depth;++deep) {
      f << "  // deep = " << deep << "\n";
      f << "  {\n";
      for (int shallow=0;shallow<deep;++shallow) {
        const auto &stats = prob_cut_stats[std::max(move_number, initial_stones)][deep][shallow];
        assert(stats.num_samples > 0);
        const double offset = stats.total_offset / stats.num_samples;
        const double stddev = std::sqrt(stats.total_offset_squared / stats.num_samples - offset * offset);
        f << "    {" << shallow << "," << offset << "," << stddev << "},\n";
      }
      f << "  },\n";
    }
    f << "},\n";
  }
  f << "};\n";

}
