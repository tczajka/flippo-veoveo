#include "clock.h"
#include "logging.h"
#include "player_ab.h"
#include "player_deterministic.h"
#include "player_external.h"
#include "player_mc.h"
#include "player_mcts.h"
#include "player_random.h"
#include "position.h"
#include "referee_util.h"
#include <cassert>
#include <cstdlib>
#include <memory>
#include <mutex>
#include <thread>

namespace {

class Match {
public:
  Match(int argc, char **argv);
  void play();

private:
  void run_thread();

  int initial_stones = 4;
  std::vector<Position> starting_positions;

  using PlayerFactory =
    std::function<std::unique_ptr<Player>(
        const std::string &log_path,
        const Position &initial_position,
        Duration game_time)>;
  std::vector<PlayerFactory> player_factories;

  int num_threads = 1;
  bool both_sides = true;

  bool store_log = false;

  std::mutex data_lock;
  std::vector<Position>::const_iterator next_starting_position;

  static constexpr Duration default_time_limit = std::chrono::seconds{1};
  Duration time_limit[2] = {default_time_limit, default_time_limit};
  Duration max_time[2];

  // For player 0.
  std::int64_t total_score;
  std::int64_t total_square_score;

  std::int64_t total_white;
  std::int64_t total_square_white;
};

Match::Match(int argc, char **argv) {
  int next = 1;
  while (next < argc) {
    const std::string arg(argv[next++]);
    if (arg == "random") {
      player_factories.push_back([](const std::string &,
                                    const Position &,
                                    const Duration) {
        return std::make_unique<PlayerRandom>();
      });
    } else if (arg == "mc") {
      player_factories.push_back([](const std::string &,
                                    const Position &,
                                    const Duration) {
        return std::make_unique<PlayerMc>();
      });
    } else if (arg == "mcts") {
      player_factories.push_back([](const std::string &,
                                    const Position &,
                                    const Duration) {
        return std::make_unique<PlayerMcts>();
      });
    } else if (arg == "ab") {
      player_factories.push_back([](const std::string &,
                                    const Position &,
                                    const Duration) {
        return std::make_unique<PlayerAB>();
      });
    } else if (arg.substr(0,5) == "first") {
      assert(arg.size() == 6 && arg[5] >= '0' && arg[5] <= '7');
      player_factories.push_back([arg](const std::string &,
                                       const Position &,
                                       const Duration) {
        return std::make_unique<PlayerFirst>(arg[5]-'0');
      });
    } else if (arg.substr(0,6) == "greedy") {
      assert(arg.size() == 7 && arg[6] >= '0' && arg[6] <= '7');
      player_factories.push_back([arg](const std::string &,
                                       const Position &,
                                       const Duration) {
        return std::make_unique<PlayerGreedy>(arg[6]-'0');
      });
    } else if (arg.find('.') != std::string::npos ||
               arg.find('/') != std::string::npos) {
      player_factories.push_back([arg](const std::string &log_path,
                                       const Position &initial_position,
                                       const Duration game_time) {
        return std::make_unique<PlayerExternal>(
            arg,
            log_path,
            initial_position,
            game_time);
      });
    } else if (arg == "-stones") {
      assert(next < argc);
      initial_stones = std::stoi(argv[next++]);
    } else if (arg == "-threads") {
      assert(next < argc);
      num_threads = std::stoi(argv[next++]);
    } else if (arg == "-verbosity") {
      assert(next < argc);
      verbosity = std::stoi(argv[next++]);
    } else if (arg == "-noswap") {
      both_sides = false;
    } else if (arg == "-log") {
      store_log = true;
    } else if (arg == "-time") {
      assert(next < argc);
      const std::string time_arg_str{argv[next++]};
      const auto colon = time_arg_str.find(':');
      int time_arg[2] = {};
      if (colon == std::string::npos) {
        time_arg[0] = time_arg[1] = std::stoi(time_arg_str);
      } else {
        time_arg[0] = std::stoi(time_arg_str.substr(0, colon));
        time_arg[1] = std::stoi(time_arg_str.substr(colon+1));
      }
      for (int i=0;i<2;++i) {
        time_limit[i] = std::chrono::milliseconds{time_arg[i]};
      }
    } else {
      log_always("Invalid argument: %s\n", arg.c_str());
      std::exit(1);
    }
  }
  
  assert(player_factories.size() == 2);
  assert(initial_stones >= 4 && initial_stones <= 64);
  assert(num_threads >= 1);
}

void Match::play() {
  starting_positions = generate_starting_positions(initial_stones);
  next_starting_position = starting_positions.begin();
  total_score = 0;
  total_square_score = 0;
  total_white = 0;
  total_square_white = 0;
  max_time[0] = max_time[1] = Duration{0};

  std::vector<std::thread> threads;
  for (int i=0;i<num_threads;++i) {
    threads.emplace_back(&Match::run_thread, this);
  }

  for (std::thread &thread : threads) {
    thread.join();
  }

  const double n = starting_positions.size();
  const double mean_score = total_score / n;
  const double var_score = (total_square_score - mean_score * total_score) / (n-1.0) / n;
  const double stddev_score = std::sqrt(var_score);
  printf("Score: %.6f +- %.6f\n", 0.5 * mean_score, 0.5 * stddev_score);

  const double mean_white = total_white / n;
  const double var_white = (total_square_white - mean_white * total_white) / (n-1.0) / n;
  const double stddev_white = std::sqrt(var_white);
  printf("White: %.6f +- %.6f\n", 0.5 * mean_white, 0.5 * stddev_white);

  printf("Time: %.3f %.3f\n", to_seconds(max_time[0]), to_seconds(max_time[1]));
}

void Match::run_thread() {
  for (;;) {
    Position starting;
    long game_number = 0;
    {
      std::lock_guard<std::mutex> guard(data_lock);
      game_number = 
          static_cast<long>(next_starting_position - starting_positions.begin());
      log_always("Game %ld/%zu\n",
          game_number,
          starting_positions.size());
      if (next_starting_position == starting_positions.end()) break;
      starting = *next_starting_position++;
    }
    int score = 0;
    int white = 0;
    for (int attempt = 0; attempt < 1 + both_sides; ++attempt) {
      std::unique_ptr<Player> player[2];
      
      for (int i = 0; i < 2; ++i) {
        std::string log_path = "/dev/null";
        if (store_log) {
          log_path = std::string("/tmp/player") + std::to_string(i) + "." +
            ((i==attempt) ? 'w' : 'b') +
            std::to_string(game_number);
        }
        player[i] = player_factories[i](log_path,starting,time_limit[i]);
      }

      Position pos = starting;
      Duration time_used[2] = {Duration{0}, Duration{0}};
      while (!pos.finished()) {
        const int to_move = pos.to_move();
        const int who = to_move ^ attempt;
        PlaySettings play_settings;
        const Timestamp started_thinking = current_time();
        play_settings.start_time = started_thinking;
        play_settings.time_left = time_limit[who] - time_used[who];
        const Move move = player[who]->choose_move(pos, play_settings);
        time_used[who] += current_time() - started_thinking;
        player[who^1]->opponent_move(pos, move);
        assert(get_bit(pos.valid_moves(), move));
        pos.make_move(move, pos);
      }
      const int score_white = pos.final_score();
      if (attempt==0) score += score_white; else score -= score_white;
      white += score_white;
      {
        std::lock_guard<std::mutex> guard(data_lock);
        for (int i = 0; i<2; ++i) {
          max_time[i] = std::max(max_time[i], time_used[i]);
        }
      }
    }
    {
      std::lock_guard<std::mutex> guard(data_lock);
      total_score += score;
      total_square_score += score * score;
      total_white += white;
      total_square_white += white * white;
    }
  }
}

} // end namespace

int main(int argc, char **argv) {
  init_hashing();
  init_evaluator();
  Match match(argc, argv);
  match.play();
}
