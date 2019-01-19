#include "player_mcts.h"
#include "mathematics.h"
#include <algorithm>
#include <cassert>
#include <cinttypes>
#include <memory>

PlayerMcts::PlayerMcts() :
    allocator{pool_allocator_size},
    mcts_node_lookup{mcts_node_lookup_buckets},
    root_exploration_factor_table(precompute_tables_size),
    exploration_factor_table(precompute_tables_size),
    child_uncertainty_table(precompute_tables_size) {
  log_info("Precompute tables allocated %.2f MB\n",
           static_cast<double>(3 * precompute_tables_size * sizeof(double))/
           (1 << 20));
  for (long i=0; i<precompute_tables_size; ++i) {
    root_exploration_factor_table[i] = raw_root_exploration_factor(i);
    exploration_factor_table[i] = raw_exploration_factor(i);
    child_uncertainty_table[i] = raw_child_uncertainty(i);
  }
}

Move PlayerMcts::choose_move(const Position &position,
                             const PlaySettings &settings) {
  allocate_resources(position, settings);
  const std::size_t initial_allocator_used = allocator.used();
  const std::size_t initial_mcts_node_lookup_size = mcts_node_lookup.size();

  root = find_or_allocate_node(position);
  if (!root) {
    log_info("Could not allocate root! Making random move.\n");
    return random_generator.get_square(position.valid_moves());
  }

  if (settings.quick_if_single_move && root->num_children == 1) {
    log_info("Only one move\n");
    return first_square(position.valid_moves());
  }

  explore_root();

  const Move final_choice = final_move_select();

  log_info("RAM: %.2f / %.2f MB\n",
           static_cast<double>(allocator.used() - initial_allocator_used) / (1<<20),
           static_cast<double>(allocator.limit() - initial_allocator_used) / (1<<20));
  if (allocator.out_of_memory()) {
    log_info("OOM: allocator!!!!!!!!!!\n");
  }
  log_info("Nodes: %zu / %zu\n",
           mcts_node_lookup.size() - initial_mcts_node_lookup_size,
           mcts_node_lookup.limit() - initial_mcts_node_lookup_size);
  if (mcts_node_lookup.out_of_memory()) {
    log_info("OOM: mcts_node_lookup!!!!!\n");
  }

  return final_choice;
}

void PlayerMcts::allocate_resources(const Position &position,
                                    const PlaySettings &settings) {

  int numerator = 1, denominator = 1;
  if (!settings.use_all_resources) {
    int allocation[num_squares];
    for (int i=0;i<think_longer_move_number;++i) {
      allocation[i] = normal_allocation;
    }
    allocation[think_longer_move_number] = think_longer_allocation;
    for (int i=think_longer_move_number+1; i<num_squares; ++i) {
      allocation[i] = (allocation[i-1] * think_longer_decay_percent + 99) / 100;
    }
    const int move_number = position.move_number();
    numerator = allocation[move_number];
    denominator = 0;
    for (int i = move_number; i < num_squares; i += 2) {
      denominator += allocation[i];
    }
  }

  deadline = settings.start_time + settings.time_left * numerator / denominator;

  allocator.set_limit(
      allocator.used() + (allocator.capacity() - allocator.used()) * numerator / denominator);

  mcts_node_lookup.set_limit(
      mcts_node_lookup.size() + (mcts_node_lookup.capacity() - mcts_node_lookup.size()) * numerator / denominator);
}

PlayerMcts::MctsNode *PlayerMcts::find_or_allocate_node(const Position &position) {
  bool inserted;
  CompressedPtr<MctsNode> *const node_ptr_ptr = mcts_node_lookup.insert(position, inserted);
  if (!node_ptr_ptr) {
    // Out of memory in hash table.
    return nullptr;
  }
  if ((*node_ptr_ptr).is_null()) {
    // New node, or allocation previously failed.
    // The following may fail and return nullptr.
    MctsNode *const node = allocator.construct<MctsNode>(position);
    *node_ptr_ptr = allocator.compress(node);
    return node;
  } else {
    return allocator.decompress(*node_ptr_ptr);
  }
}

void PlayerMcts::explore_root() {
  std::int64_t num_simulations = 0;
  while (root->score_lower < root->score_upper && current_time() < deadline) {
    explore(*root, -max_score, max_score);
    ++num_simulations;
  }
  log_info("Simulations %" PRId64 "\n", num_simulations);
}

void PlayerMcts::explore(MctsNode &node, Score alpha, const Score beta) {
  alpha = std::max(alpha, node.score_lower);
  if (node.score_lower >= beta ||
      node.score_upper <= alpha) {
    return;
  }

  MiniMctsNode *const child = tree_move_select(node, alpha);

  Position next_position;

  int played = 0; // 1 = additional game, 2 = full score
  int64_t play_milliscore = 0;

  MctsNode *child_node = nullptr;
  if (child->full_node.is_null()) {
    node.position.make_move(child->move, next_position);

    if (child->num_games >= min_games_to_expand) {
      if (next_position.move_number() >= alpha_beta_move_number) {
        const Score score = -alpha_beta(next_position, -beta, -alpha);
        if (score > alpha) {
          child->score_lower = std::max(child->score_lower, score);
        }
        if (score < beta) {
          child->score_upper = std::min(child->score_upper, score);
        }
        played = 2;
        play_milliscore = int64_t{score} << milliscore_bits;
      } else {
        // Expand child.
        child_node = find_or_allocate_node(next_position);
        if (child_node) {
          child->full_node = allocator.compress(child_node);
          // Add simulations we already have.
          const std::int64_t prev_games = child_node->num_games;

          child_node->num_games += child->num_games;
          child_node->total_milliscore -= child->total_milliscore;

          if (prev_games < evaluate_after_visits &&
              child_node->num_games >= evaluate_after_visits) {
            evaluate_node(*child_node);
          }
        }
      }
    }
  } else {
    child_node = allocator.decompress(child->full_node);
  }

  if (child_node) {
    // Explore recursively.
    explore(*child_node, -beta, -alpha);
    // Update info.
    child->score_lower = -child_node->score_upper;
    child->score_upper = -child_node->score_lower;
    played = 2;
    play_milliscore =
      -rounding_divide(child_node->total_milliscore, child_node->num_games);
    play_milliscore = std::max(
        play_milliscore,
        std::int64_t{child->score_lower} << milliscore_bits);
    play_milliscore = std::min(
        play_milliscore,
        std::int64_t{child->score_upper} << milliscore_bits);
  }

  if (played) {
    node.score_lower = std::max(node.score_lower, child->score_lower);
    if (child->score_upper < node.score_upper &&
        node.visited_children == node.num_children) {
      node.score_upper = -max_score;
      const MiniMctsNode *const children = allocator.decompress(node.children);
      for (int ch = 0; ch < node.num_children; ++ch) {
        node.score_upper = std::max(node.score_upper, children[ch].score_upper);
      }
    }
  } else {
    played = 1;
    play_milliscore = -(random_rollout(next_position) << milliscore_bits);
  }

  node.total_milliscore -= child->total_milliscore;
  // Only increase child count by 1 to avoid skewing statistics for shared children.
  ++child->num_games;

  if (played == 2) {
    child->total_milliscore = child->num_games * play_milliscore;
  } else {
    child->total_milliscore += play_milliscore;
  }

  ++node.num_games;
  node.total_milliscore += child->total_milliscore;

  if (node.num_games == evaluate_after_visits) {
    evaluate_node(node);
  }
}

Score PlayerMcts::random_rollout(const Position &position) {
  Position pos2 = position;
  while (!pos2.finished()) {
    const Bitboard valid_moves = pos2.valid_moves();
    const Move move = rollout_move_select(position, valid_moves);
    pos2.make_move(move, pos2);
  }
  const Score score = pos2.final_score();
  return position.to_move() == 0 ? score : -score;
}

Score PlayerMcts::alpha_beta(const Position &position,
                             const Score alpha,
                             const Score beta) {
  // TODO: Memoize?
  if (position.finished()) {
    return position.final_score();
  }
  Bitboard remaining_moves = position.valid_moves();
  Score best_score = -max_score;
  while (best_score < beta && remaining_moves) {
    const Move move = first_square(remaining_moves);
    remaining_moves = reset_bit(remaining_moves, move);

    Position pos2;
    position.make_move(move, pos2);

    const Score score = -alpha_beta(pos2, -beta, -std::max(alpha, best_score));
    best_score = std::max(best_score, score);
  }

  return best_score;
}

PlayerMcts::MiniMctsNode *PlayerMcts::tree_move_select(MctsNode &node,
                                                       const Score alpha) {
  MiniMctsNode *children = allocator.decompress(node.children);

  if (node.visited_children < node.num_children) {
    if (node.visited_children == node.children_capacity) {
      // Resize children.
      const int new_capacity =
        std::min<int>(2 * node.children_capacity + 1, node.num_children);
      MiniMctsNode *const new_children = allocator.allocate<MiniMctsNode>(new_capacity);
      if (!new_children) {
        // OOM!
        goto failed_oom;
      }
      for (int i=0; i<node.visited_children; ++i) {
        new (new_children+i) MiniMctsNode(std::move(children[i]));
      }
      node.children_capacity = new_capacity;
      children = new_children;
      node.children = allocator.compress(children);
    }
    // Pick an unexplored child and add.
    const Move move = rollout_move_select(node.position, node.unvisited_moves);

    MiniMctsNode *const child = children + node.visited_children;
    new (child) MiniMctsNode(move);

    node.unvisited_moves = reset_bit(node.unvisited_moves, move);
    ++node.visited_children;

    return child;
  }

failed_oom:

  MiniMctsNode *child = nullptr;
  double best_bound = -1e100;

  const double E =
    &node == root ?
    root_exploration_factor(node.num_games) :
    exploration_factor(node.num_games);

  for (int child_idx=0; child_idx != node.visited_children; ++child_idx) {
    MiniMctsNode *const p = children + child_idx;
    if (p->score_upper <= alpha) {
      // This move can't help.
      continue;
    }
    const double bound = p->average_score() + E * child_uncertainty(p->num_games);
    if (bound > best_bound) {
      child = p;
      best_bound = bound;
    }
  }
  assert(child);

  return child;
}

inline Move PlayerMcts::rollout_move_select(const Position &, Bitboard moves) {
  const Bitboard corner_moves = moves & corners;
  if (corner_moves) moves = corner_moves;
  return random_generator.get_square(moves);
}

Move PlayerMcts::final_move_select() {
  if (root->visited_children == 0) {
    log_info("Root has no visited children!\n");
    return random_generator.get_square(root->position.valid_moves());
  }

  std::pair<double, MiniMctsNode*> sorted_children[max_moves];
  MiniMctsNode *const children = allocator.decompress(root->children);
  for (int i = 0; i < root->visited_children; ++i) {
    MiniMctsNode *const child = children + i;
    // Secure child score.
    double score;
    if (child->num_games == 0) {
      score = -1e100;
    } else {
      score = child->average_score();
      score = std::fmin(score, child->score_upper);
      score -= secure_child_coefficient / sqrt(child->num_games);
      score = std::fmax(score, child->score_lower);
    }
    sorted_children[i] = std::pair<double, MiniMctsNode*>(score, child);
  }

  std::sort(sorted_children, sorted_children + root->visited_children,
            [](const std::pair<double, MiniMctsNode*> &a,
               const std::pair<double, MiniMctsNode*> &b) {
              return a.first > b.first;
            });

  log_info("Best moves:");
  for (int i=0; i < root->visited_children && i < 3; ++i) {
    const MiniMctsNode *const child = sorted_children[i].second;
    if (child->num_games == 0) continue;
    log_info(" %s", move_to_string(child->move).c_str());
    if (child->score_lower >= child->score_upper) {
      log_info(" [%d]", int{child->score_lower});
    } else {
      if (child->score_lower > -max_score || child->score_upper < max_score) {
        log_info(" [%d - %d]", int{child->score_lower}, int{child->score_upper});
      }
      double score = child->average_score();
      score = fmax(score, child->score_lower);
      score = fmin(score, child->score_upper);
      log_info(" (%.3f)", score);
    }
  }
  log_info("\n");

  return sorted_children[0].second->move;
}

void PlayerMcts::evaluate_node(MctsNode &node) {
  const std::int64_t milliscore = evaluate(node.position);
  node.num_games += evaluation_weight;
  node.total_milliscore += evaluation_weight * milliscore;
}

inline PlayerMcts::MctsNode::MctsNode(const Position &position) :
    position{position},
    num_games{0},
    total_milliscore{0},
    unvisited_moves{position.valid_moves()},
    children{},
    score_lower{-max_score},
    score_upper{max_score},
    num_children(count_squares(unvisited_moves)),
    visited_children{0},
    children_capacity{0}
{
  if (num_games >= evaluate_after_visits) {
    evaluate_node(*this);
  }
}
