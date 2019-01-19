#include "hashing.h"
#include "evaluator.h"
#include "logging.h"
#include "player_deterministic.h"
#include "position.h"
#include <cassert>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace {

struct OpponentModel {
  std::string name;
  std::unique_ptr<Player> player;
};

struct Opponent {
  Opponent(const std::string &_name);
  void made_move(const Position &position, const Move move);

  std::string name;
  std::vector<OpponentModel> models;
};

Opponent::Opponent(const std::string &_name) : name{_name} {
  for (int symmetry=0;symmetry<8;++symmetry) {
    models.push_back(OpponentModel{
      "first" + std::to_string(symmetry),
      std::make_unique<PlayerFirst>(symmetry)});
  }
  for (int symmetry=0;symmetry<8;++symmetry) {
    models.push_back(OpponentModel{
      "greedy" + std::to_string(symmetry),
      std::make_unique<PlayerGreedy>(symmetry)});
  }
}

void Opponent::made_move(const Position &position, const Move move) {
  std::vector<OpponentModel> new_models;
  for (OpponentModel &model : models) {
    PlaySettings settings;
    settings.start_time = Timestamp{};
    settings.time_left = Duration{};
    const Move predicted_move = model.player->choose_move(position, settings);
    if (predicted_move == move) new_models.push_back(std::move(model));
  }
  models = std::move(new_models);
}

void analyze_opponents(const char *const competition_file_name) {
  std::ifstream f{competition_file_name};

  std::vector<Opponent> opponents;

  for (;;) {
    std::string line;
    if(!std::getline(f, line)) break;
    if (line.substr(0, 7) == "Player ") {
      opponents.emplace_back(line.substr(7));
    } else if (line.substr(0, 5) == "Game ") {
      std::istringstream game_stream{line.substr(5)};
      Opponent *opp[2];
      for (int i = 0; i<2; ++i) {
        int opp_idx; game_stream >> opp_idx;
        assert(!!game_stream && opp_idx >= 0 && opp_idx < int(opponents.size()));
        opp[i] = &opponents[opp_idx];
      }
      Position position = Position::initial();
      while (!position.finished()) {
        std::string move_name;
        game_stream >> move_name;
        assert(!!game_stream);
        const Move move = string_to_move(move_name);
        assert(move != invalid_move);
        opp[position.to_move()]->made_move(position, move);
        position.make_move(move, position);
      }
    }
  }

  for (const Opponent &opponent : opponents) {
    assert(opponent.models.size() <= 1u);
    if (opponent.models.size() == 1u) {
      std::cout << opponent.name << " " << opponent.models[0].name << "\n";
    }
  }
}

} // end namespace

int main(int argc, char **argv) {
  init_hashing();
  init_evaluator();
  assert(argc == 2);
  analyze_opponents(argv[1]);
}
