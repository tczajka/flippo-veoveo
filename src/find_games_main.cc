#include "book.h"
#include "evaluator.h"
#include "hashing.h"
#include "logging.h"
#include "player_ab.h"
#include "player_deterministic.h"
#include "prepared.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <vector>

namespace {

struct State {
  Position position;
  Position middle;
  Milliscore value;

  explicit State() : position(), middle(), value(-max_milliscore) {}
};

inline bool operator==(const State &a, const State &b) {
  return a.position == b.position;
}

struct StateBetter {
  bool operator()(const State &a, const State &b) const {
    if (a.value != b.value) return a.value > b.value;
    if (a.position.player != b.position.player) {
      return a.position.player < b.position.player;
    }
    return a.position.opponent < b.position.opponent;
  }
};

class FindGames {
public:
  FindGames(int argc, char **argv);
  void go();
private:
  void advance_beam();
  void reduce_next_beam();
  State brute_search_beam();
  void brute_search_beam_rec(const Position &position,
                             Prepared &game);
  void recover_moves(const Position &position1,
                     const Position &position2);
  State get_state(const Position &position);
  Milliscore evaluate_rec(const Position &position, int d);
  void print();

  size_t beam_size = 0;
  int brute_threshold = 0;
  int color = -1;
  int depth = 0;
  std::vector<State> beam;
  std::vector<State> next_beam;
  int symmetry = -1;
  bool use_prepared = false;
  int good_moves = 0;
  std::unique_ptr<Player> opponent;
  std::unique_ptr<Player> full_opponent;
  std::string opponent_name;
  Prepared best_game;
  State beam_thresholds[num_squares];
};

FindGames::FindGames(int argc, char **argv) {
  int next = 1;
  while (next < argc) {
    const std::string arg(argv[next++]);
    if (arg == "-beam") {
      assert(next < argc);
      beam_size = std::stoul(argv[next++]);
    } else if (arg == "-brute") {
      assert(next < argc);
      brute_threshold = std::stoi(argv[next++]);
    } else if (arg == "-depth") {
      assert(next < argc);
      depth = std::stoi(argv[next++]);
    } else if (arg == "-color") {
      assert(next < argc);
      color = std::stoi(argv[next++]);
    } else if (arg == "-prepared") {
      use_prepared = true;
    } else if (arg == "-good_moves") {
      assert(next < argc);
      good_moves = std::stoi(argv[next++]);
    } else if (arg == "-first") {
      assert(next < argc);
      symmetry = std::stoi(argv[next++]);
      opponent = std::make_unique<PlayerFirst>(0);
      full_opponent = std::make_unique<PlayerFirst>(symmetry);
      opponent_name = "first" + std::to_string(symmetry);
    } else if (arg == "-greedy") {
      assert(next < argc);
      symmetry = std::stoi(argv[next++]);
      opponent = std::make_unique<PlayerGreedy>(0);
      full_opponent = std::make_unique<PlayerGreedy>(symmetry);
      opponent_name = "greedy" + std::to_string(symmetry);
    } else {
      log_always("Invalid argument: %s\n", arg.c_str());
      std::exit(1);
    }
  }
  assert(beam_size > 0);
  assert(brute_threshold > 0 && brute_threshold + depth <= num_squares-2);
  assert(brute_threshold % 2 == color);
  assert(depth>=0 && depth%2==0);
  assert(color == 0 || color == 1);
  assert(opponent != nullptr);
  beam.reserve(beam_size);
  next_beam.reserve(2 * beam_size);
}

void FindGames::go() {
  best_game.color = color;
  best_game.score = -max_score;
  for (int i=0; i < num_squares; ++i) best_game.moves[i] = invalid_move;

  const Timestamp t0 = current_time();
  Position start_position = Position::initial();
  for (;;) {
    assert(!start_position.finished());
    Move move;
    if (start_position.to_move() == color) {
      move = find_book_move(start_position);
      if (move == invalid_move && use_prepared) {
        move = find_prepared_game(start_position.move_number(),
                                  best_game.moves);
      }
      if (move == invalid_move && start_position.move_number() < good_moves) {
        PlayerAB player;
        PlaySettings settings;
        settings.start_time = current_time();
        settings.time_left = std::chrono::seconds(2);
        settings.use_all_resources = true;
        move = player.choose_move(start_position, settings);
      }
      if (move == invalid_move) break;
    } else {
      move = full_opponent->choose_move(start_position, PlaySettings{});
    }
    best_game.moves[start_position.move_number()] = move;
    start_position.make_move(move, start_position);
  }
  const Timestamp t1 = current_time();

  start_position = start_position.transform(symmetry);

  beam.clear();
  beam.push_back(get_state(start_position));

  const int middle_move_number = start_position.move_number()
    + 2 * ((brute_threshold-start_position.move_number()) / 4);
  while (beam.front().position.move_number() < brute_threshold) {
    if (beam[0].position.move_number() == middle_move_number) {
      for (State &state : beam) {
        state.middle = state.position;
      }
    }
    advance_beam();
    log_info("move_number=%d value=%.6f\n",
             beam.front().position.move_number(),
             std::ldexp(beam.front().value, -milliscore_bits));
  }
  const Timestamp t2 = current_time();
  const State brute_state = brute_search_beam();
  const Timestamp t3 = current_time();
  log_info("score=%d\n", static_cast<int>(best_game.score));
  recover_moves(start_position, brute_state.middle);
  recover_moves(brute_state.middle, brute_state.position);
  const Timestamp t4 = current_time();

  for (int i = start_position.move_number(); i < num_squares; ++i) {
    best_game.moves[i] = untransform_square(best_game.moves[i], symmetry);
  }

  log_info("time: initial=%.1f beam_search=%.1f brute_search=%.1f recovery=%.1f\n",
           to_seconds(t1-t0), to_seconds(t2-t1), to_seconds(t3-t2),
           to_seconds(t4-t3));
  print();
}

void FindGames::advance_beam() {
  StateBetter state_better;
  next_beam.clear();
  State &threshold = beam_thresholds[beam[0].position.move_number()+2];
  for (const State &state : beam) {
    Bitboard remaining_moves = state.position.valid_moves();
    while (remaining_moves) {
      const Move move = first_square(remaining_moves);
      remaining_moves = reset_bit(remaining_moves, move);
      Position next_position;
      state.position.make_move(move, next_position);
      const Move opp_move =
        opponent->choose_move(next_position, PlaySettings{});
      next_position.make_move(opp_move, next_position);
      State next_state = get_state(next_position);
      next_state.middle = state.middle;
      if (state_better(next_state, threshold)) {
        next_beam.push_back(next_state);
        if (next_beam.size() == next_beam.capacity()) {
          reduce_next_beam();
        }
      }
    }
  }
  reduce_next_beam();
  beam = next_beam;
}

void FindGames::reduce_next_beam() {
  StateBetter state_better;
  State &threshold = beam_thresholds[next_beam[0].position.move_number()];

  std::sort(next_beam.begin(), next_beam.end(), state_better);
  auto p = std::unique(next_beam.begin(), next_beam.end());
  next_beam.erase(p, next_beam.end());
  if (next_beam.size() > beam_size) {
    threshold = next_beam[beam_size];
    next_beam.resize(beam_size);
  }
}

State FindGames::brute_search_beam() {
  best_game.score = -max_score;
  Prepared cur_game = best_game;
  State best_state;
  for (const State &state : beam) {
    const Score old_score = best_game.score;
    brute_search_beam_rec(state.position, cur_game);
    if (best_game.score > old_score) {
      best_state = state;
    }
  }
  assert(best_game.score > -max_score);
  return best_state;
}

void FindGames::brute_search_beam_rec(const Position &position,
                                      Prepared &game) {
  if (position.finished()) {
    game.score = position.final_score();
    if (game.score > best_game.score) {
      best_game = game;
    }
  } else {
    const int move_number = position.move_number();
    Bitboard remaining_moves = position.valid_moves();
    while (remaining_moves) {
      const Move move = first_square(remaining_moves);
      game.moves[move_number] = move;
      remaining_moves = reset_bit(remaining_moves, move);
      Position next_position;
      position.make_move(move, next_position);
      if (next_position.finished()) {
        game.score = -next_position.final_score();
        if (game.score > best_game.score) {
          best_game = game;
        }
      } else {
        const Move opp_move =
          opponent->choose_move(next_position, PlaySettings{});
        game.moves[move_number+1] = opp_move;
        next_position.make_move(opp_move, next_position);
        brute_search_beam_rec(next_position, game);
      }
    }
  }
}

void FindGames::recover_moves(const Position &position1,
                              const Position &position2) {
  const int pos1mn = position1.move_number();
  const int pos2mn = position2.move_number();
  log_info("recovering moves %d-%d\n", pos1mn, pos2mn);
  assert(pos2mn - pos1mn > 0 && (pos2mn-pos1mn) % 2 == 0);
  if (pos2mn - pos1mn == 2) {
    Bitboard remaining_moves = position1.valid_moves();
    while (remaining_moves) {
      const Move move = first_square(remaining_moves);
      remaining_moves = reset_bit(remaining_moves, move);
      Position next_position;
      position1.make_move(move, next_position);
      const Move opp_move =
        opponent->choose_move(next_position, PlaySettings{});
      next_position.make_move(opp_move, next_position);
      if (next_position == position2) {
        best_game.moves[pos1mn] = move;
        best_game.moves[pos1mn + 1] = opp_move;
        break;
      }
    }
    assert(best_game.moves[pos1mn] != invalid_move);
  } else {
    const int middle_move_number = pos1mn + 2 * ((pos2mn-pos1mn) / 4);

    beam.clear();
    beam.push_back(get_state(position1));
    while (beam[0].position.move_number() != pos2mn) {
      if (beam[0].position.move_number() == middle_move_number) {
        for (State &state : beam) {
          state.middle = state.position;
        }
      }
      advance_beam();
    }

    Position middle;
    for (const State &state : beam) {
      if (state.position == position2) {
        middle = state.middle;
        break;
      }
    }
    assert(middle.move_number() == middle_move_number);
    recover_moves(position1, middle);
    recover_moves(middle, position2);
  }
}

State FindGames::get_state(const Position &position) {
  State state;
  state.position = position;
  state.value = evaluate_rec(position, depth);
  return state;
}

Milliscore FindGames::evaluate_rec(const Position &position, int d) {
  if (d<=0) return evaluate(position);
  Milliscore eval = -max_milliscore;
  Bitboard remaining_moves = position.valid_moves();
  while (remaining_moves) {
    const Move move = first_square(remaining_moves);
    remaining_moves = reset_bit(remaining_moves, move);
    Position next_position;
    position.make_move(move, next_position);
    const Move opp_move = opponent->choose_move(next_position, PlaySettings{});
    next_position.make_move(opp_move, next_position);
    eval = std::max(eval, evaluate_rec(next_position, d-2));
  }
  return eval;
}

void FindGames::print() {
  std::ofstream f("prepared.tmp");
  f << "  // " << (color?"black":"white") << " vs " << opponent_name << "\n";
  f << "  { " << best_game.color << ","
    << static_cast<int>(best_game.score) << ", {";
  for (const Move move : best_game.moves) {
    f << static_cast<int>(move) << ",";
  }
  f << "}},\n";
}

} // namespace

int main(int argc, char **argv) {
  init_hashing();
  init_evaluator();
  FindGames find_games{argc, argv};
  find_games.go();
}
