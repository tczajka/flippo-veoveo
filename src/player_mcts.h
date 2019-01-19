#ifndef PLAYER_MCTS_H
#define PLAYER_MCTS_H

#include "evaluator.h"
#include "hashing.h"
#include "player.h"
#include "pool_allocator.h"
#include "position.h"
#include "random.h"
#include <cmath>
#include <cstdint>

class PlayerMcts : public Player {
public:
  PlayerMcts();

  Move choose_move(const Position &position,
                   const PlaySettings &settings) override;

private:
  static constexpr std::size_t pool_allocator_size = 200 << 20;
  static constexpr std::size_t mcts_node_lookup_buckets = 1<<20;

  static constexpr long precompute_tables_size = 100000;
  static constexpr double ucb_coefficient = 4.0;
  static constexpr double root_coefficient = 1.0;
  static constexpr double secure_child_coefficient = 10.0;
  static constexpr int min_games_to_expand = 1;
  static constexpr int alpha_beta_move_number = 59;

  static constexpr int normal_allocation = 10;
  static constexpr int think_longer_move_number = 52;
  static constexpr int think_longer_allocation = 75;
  static constexpr int think_longer_decay_percent = 30;

  static constexpr std::int64_t evaluate_after_visits = 0;
  static constexpr std::int64_t evaluation_weight = 1;

  struct MctsNode;

  struct MiniMctsNode {
    MiniMctsNode() {}
    MiniMctsNode(const Move _move):
      num_games{0},
      total_milliscore{0},
      full_node{},
      move{_move},
      score_lower{-max_score},
      score_upper{max_score}
    {}

    double average_score() const {
      return std::ldexp(total_milliscore, -milliscore_bits) / num_games;
    }

    std::int64_t num_games;
    std::int64_t total_milliscore;
    CompressedPtr<MctsNode> full_node;
    Move move;
    Score score_lower;
    Score score_upper;
  };

  struct MctsNode {
    explicit MctsNode(const Position &position);

    Position position;
    std::int64_t num_games;
    std::int64_t total_milliscore;
    Bitboard unvisited_moves;
    CompressedPtr<MiniMctsNode> children;
    Score score_lower;
    Score score_upper;
    std::int8_t num_children;
    std::int8_t visited_children;
    std::int8_t children_capacity;
  };

  void allocate_resources(const Position &position,
                          const PlaySettings &settings);
  MctsNode *find_or_allocate_node(const Position &position);
  void explore_root();
  void explore(MctsNode &node, Score alpha, Score beta);
  Score random_rollout(const Position &position);
  Score alpha_beta(const Position &position, Score alpha, Score beta);
  MiniMctsNode *tree_move_select(MctsNode &node, Score alpha);
  Move rollout_move_select(const Position &position, Bitboard moves);
  Move final_move_select();
  static void evaluate_node(MctsNode &node);

  static double raw_root_exploration_factor(const std::int64_t num_games) {
    return root_coefficient * std::sqrt(std::sqrt(num_games));
  }

  static double raw_exploration_factor(const std::int64_t num_games) {
    return ucb_coefficient * std::sqrt(std::log(num_games));
  }

  static double raw_child_uncertainty(const std::int64_t num_visits) {
    return 1.0 / std::sqrt(num_visits);
  }

  double root_exploration_factor(const std::int64_t num_games) const {
    return
      num_games < precompute_tables_size ?
      root_exploration_factor_table[num_games] :
      raw_root_exploration_factor(num_games);
  }

  double exploration_factor(const std::int64_t num_games) const {
    return
      num_games < precompute_tables_size ?
      exploration_factor_table[num_games] :
      raw_exploration_factor(num_games);
  }

  double child_uncertainty(const std::int64_t num_visits) const {
    return
      num_visits < precompute_tables_size ?
      child_uncertainty_table[num_visits] :
      raw_child_uncertainty(num_visits);
  }

  RandomGenerator random_generator;
  PoolAllocator allocator;
  PositionHashTable<CompressedPtr<MctsNode>> mcts_node_lookup;

  Timestamp deadline;
  MctsNode *root;

  std::vector<double> root_exploration_factor_table;
  std::vector<double> exploration_factor_table;
  std::vector<double> child_uncertainty_table;
};

#endif
