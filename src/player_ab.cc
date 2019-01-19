#include "book.h"
#include "player_ab.h"
#include "logging.h"
#include "mathematics.h"
#include "prepared.h"
#include "prob_cut.h"
#include <algorithm>
#include <cmath>

PlayerAB::PlayerAB():
  transposition_table{transposition_table_buckets}
{
  for (int i=0;i<num_squares;++i) killer_moves[i] = invalid_move;
  for (int i=0;i<num_squares;++i) moves_so_far[i] = invalid_move;
}

Move PlayerAB::choose_move(const Position &position,
                           const PlaySettings &settings) {
  const int move_number = position.move_number();
  log_info("Move %d:\n", move_number);

  if (settings.use_book) {
    const Move book_move = find_book_move(position);
    if (book_move != invalid_move) {
      log_info("Book move=%s\n", move_to_string(book_move).c_str());
      last_move_milliscore = 0;
      moves_so_far[move_number] = book_move;
      return book_move;
    }

    const Move prepared_move = find_prepared_game(move_number, moves_so_far);
    if (prepared_move != invalid_move) {
      log_info("Prepared move=%s\n", move_to_string(prepared_move).c_str());
      last_move_milliscore = 0;
      moves_so_far[move_number] = prepared_move;
      return prepared_move;
    }
  }

  allocate_resources(position, settings);
  nodes_visited = 0;

  // Generate root moves.
  Move moves[max_moves];
  int num_moves=0;
  Milliscore best_milliscore;
  {
    std::pair<Move, Milliscore> move_scores[max_moves];
    Bitboard remaining_moves = position.valid_moves();
    while (remaining_moves) {
      const Move move = first_square(remaining_moves);
      remaining_moves = reset_bit(remaining_moves, move);
      Position next_position;
      position.make_move(move, next_position);
      const Milliscore score =
        next_position.finished() ?
        -next_position.final_score() << milliscore_bits :
        -evaluate(next_position);
      move_scores[num_moves++] = {move, score};
    }
    std::sort(move_scores,
              move_scores + num_moves,
              [](const std::pair<Move, Milliscore> &a,
                 const std::pair<Move, Milliscore> &b) {
                return a.second > b.second;
              });
    for (int i=0; i < num_moves; ++i) {
      moves[i] = move_scores[i].first;
    }
    best_milliscore = move_scores[0].second;
  }

  // Single move?
  if (settings.quick_if_single_move && num_moves == 1) {
    log_info("Only one move\n");
    last_move_milliscore = 0;
    moves_so_far[move_number] = moves[0];
    return moves[0];
  }

  // Iterative deepening.
  for (int depth = 2; depth <= max_eval_move_number - move_number; ++depth) {
    if (current_time() >= deadline_go_deeper) {
      log_info("depth=%d score=%.6f ", depth-1,
               std::ldexp(best_milliscore, -milliscore_bits));
      goto done;
    }

    const Milliscore aspiration_alpha = best_milliscore-aspiration_width;
    const Milliscore aspiration_beta = best_milliscore+aspiration_width;

    try {
      // First move.
      deadline = deadline_drop_work;
      Position next_position;
      position.make_move(moves[0], next_position);

      Milliscore score;
      Milliscore alpha = aspiration_alpha;
      Milliscore beta = aspiration_beta;
      for (;;) {
        score = -alpha_beta(next_position, depth-1, -beta, -alpha, true);

        if (score <= alpha) {
          alpha = -max_milliscore;
        } else if (score >= beta) {
          beta = max_milliscore;
        } else {
          break;
        }
      }
      best_milliscore = score;
    } catch (Timeout) {
      log_info("depth=%d (give up next) score=%.6f ",
               depth-1,
               std::ldexp(best_milliscore, -milliscore_bits));
      goto done;
    }

    try {
      // Other moves - search fully if possible.
      deadline = deadline_drop_work;
      for (int move_index = 1; move_index < num_moves; ++move_index) {
        if (current_time() >= deadline_next_move) throw Timeout{};

        const Move move = moves[move_index];
        Position next_position;
        position.make_move(move, next_position);

        Milliscore beta = best_milliscore + 1;
        Milliscore score;
        for (;;) {
          score = -alpha_beta(next_position, depth-1, -beta, -best_milliscore, true);
          if (score < beta) break;
          beta = score < aspiration_beta ? aspiration_beta : max_milliscore;
        }

        if (score > best_milliscore) {
          best_milliscore = score;
          std::rotate(moves, moves + move_index, moves + (move_index + 1));
        }
      }
      log_verbose("  depth=%d move=%s score=%.6f time=%.3f\n",
                  depth, move_to_string(moves[0]).c_str(),
                  std::ldexp(best_milliscore, -milliscore_bits),
                  to_seconds(current_time() - settings.start_time));
    } catch (Timeout) {
      log_info("depth=%d (partial) score=%.6f ",
               depth,
               std::ldexp(best_milliscore, -milliscore_bits));
      goto done;
    }
  }

  if (current_time() >= deadline_go_deeper) {
    log_info("pre-endgame score=%.6f ",
             std::ldexp(best_milliscore, -milliscore_bits));
    goto done;
  }

  // Endgame.
  {
    Score best_score = rounding_divide(best_milliscore, 1<<milliscore_bits);
    const Score endgame_aspiration_alpha = best_score - endgame_aspiration_width;
    const Score endgame_aspiration_beta = best_score + endgame_aspiration_width;

    // Endgame: first move.
    try {
      deadline = deadline_drop_work;
      Position next_position;
      position.make_move(moves[0], next_position);

      Score score;
      Score alpha = endgame_aspiration_alpha;
      Score beta = endgame_aspiration_beta;
      for (;;) {
        score = -endgame_alpha_beta(next_position, -beta, -alpha);

        if (score <= alpha) {
          alpha = -max_score;
        } else if (score >= beta) {
          beta = max_score;
        } else {
          break;
        }
      }
      best_score = score;
      best_milliscore = best_score << milliscore_bits;
    } catch (Timeout) {
      log_info("pre-endgame (give up) score=%.6f ",
               std::ldexp(best_milliscore, -milliscore_bits));
      goto done;
    }

    // Endgame: other moves.
    try {
      deadline = deadline_drop_work;
      for (int move_index = 1; move_index < num_moves; ++move_index) {
        if (current_time() >= deadline_next_move) throw Timeout{};

        const Move move = moves[move_index];
        Position next_position;
        position.make_move(move, next_position);

        Score beta = best_score + 1;
        Score score;
        for (;;) {
          score = -endgame_alpha_beta(next_position, -beta, -best_score);
          if (score < beta) break;
          beta = score < endgame_aspiration_beta ? endgame_aspiration_beta : max_score;
        }

        if (score > best_score) {
          best_score = score;
          best_milliscore = best_score << milliscore_bits;
          std::rotate(moves, moves + move_index, moves + (move_index + 1));
        }
      }
      log_verbose("  endgame score=%d time=%.3f\n",
                  static_cast<int>(best_score),
                  to_seconds(current_time() - settings.start_time));
    } catch (Timeout) {
      log_info("endgame (partial) score=%d ", static_cast<int>(best_score));
      goto done;
    }

    if (current_time() >= deadline_go_deeper) {
      log_info("endgame score=%d ", static_cast<int>(best_score));
      goto done;
    }

    // Exploit patzers.
    double best_patzer_score = best_score;
    int num_patzer_scores = 0;
    try {
      deadline = deadline_drop_work;
      // Do best move lazily, only if necessary.
      for (int move_index = 1; move_index < num_moves; ++move_index) {
        if (current_time() >= deadline_next_move) throw Timeout{};

        const Move move = moves[move_index];
        Position next_position;
        position.make_move(move, next_position);

        const Score score =
          -endgame_alpha_beta(next_position, -best_score, -(best_score-1));

        if (score >= best_score) {
          if (num_patzer_scores == 0) {
            // Evaluate best move lazily.
            Position best_position;
            position.make_move(moves[0], best_position);
            best_patzer_score = -endgame_patzer_score(best_position);
            ++num_patzer_scores;
          }
          const double patzer_score = -endgame_patzer_score(next_position);
          ++num_patzer_scores;
          if (patzer_score > best_patzer_score) {
            best_patzer_score = patzer_score;
            std::rotate(moves, moves + move_index, moves + (move_index + 1));
          }
        }
      }
    } catch (Timeout) {
      if (num_patzer_scores >= 1) {
        log_info("endgame score=%d patzer(partial)=%.6f equivalent(partial)=%d ",
                 static_cast<int>(best_score),
                 best_patzer_score,
                 num_patzer_scores);
      } else {
        log_info("endgame score=%d give up patzer ",
                 static_cast<int>(best_score));
      }
      goto done;
    }

    if (num_patzer_scores >= 1) {
      log_info("endgame score=%d patzer=%.6f equivalent=%d ",
               static_cast<int>(best_score),
               best_patzer_score,
               num_patzer_scores);
    } else {
      log_info("endgame score=%d unique ",
               static_cast<int>(best_score));
    }
  }

done:
  const double time_used_seconds = 
    to_seconds(current_time() - settings.start_time);

  log_info("move=%s time=%.3f knps=%.0f tt=%zuk%s/%zuk\n",
           move_to_string(moves[0]).c_str(),
           time_used_seconds,
           0.001 * nodes_visited / time_used_seconds,
           transposition_table.size() >> 10,
           transposition_table.out_of_memory() ? "-OOM!" : "",
           transposition_table.capacity() >> 10);

  last_move_milliscore = best_milliscore;
  moves_so_far[move_number] = moves[0];
  return moves[0];
}

void PlayerAB::opponent_move(const Position &position, const Move move) {
  moves_so_far[position.move_number()] = move;
}

Milliscore PlayerAB::evaluate_depth(const Position &position, int depth) {
  deadline = current_time() + std::chrono::seconds(3600);
  if (position.move_number() + depth >= num_squares) {
    return endgame_alpha_beta(position, -max_score, max_score) << milliscore_bits;
  } else {
    return alpha_beta(position, depth, -max_milliscore, max_milliscore, false);
  }
}

void PlayerAB::allocate_resources(const Position &position,
                                  const PlaySettings &settings) {
  if (settings.use_all_resources) {
    deadline_go_deeper = settings.start_time + settings.time_left;
    deadline_next_move = settings.start_time + settings.time_left;
    deadline_drop_work = settings.start_time + settings.time_left;
  } else {
    const int move_number = position.move_number();
    const double time_left = to_seconds(settings.time_left);
    int this_move_allocation =
      allocation_move0 + (allocation_move50-allocation_move0) * move_number / 50;
    int total_allocation = 0;
    for (int i=move_number; i < num_squares; i+=2) {
      // See if we can solve endgame at i.
      if (time_left * endgame_solve_allocation /
            (total_allocation + endgame_solve_allocation + after_solved_allocation)
          > rough_time_to_solve(num_squares - i)) {
        total_allocation += endgame_solve_allocation + after_solved_allocation;
        if (i==move_number) this_move_allocation = endgame_solve_allocation;
        break;
      }
      total_allocation +=
        allocation_move0 + (allocation_move50-allocation_move0) * i / 50;
    }

    const Duration duration_goal =
      settings.time_left * this_move_allocation / total_allocation;
    deadline_go_deeper =
      settings.start_time + duration_goal * deadline_go_deeper_percentage / 100;
    deadline_next_move =
      settings.start_time + duration_goal * deadline_next_move_percentage / 100;
    deadline_drop_work =
      settings.start_time + duration_goal * deadline_drop_work_percentage / 100;
  }
}

double PlayerAB::rough_time_to_solve(const int depth) {
  return std::pow(rough_endgame_branching_factor,  depth) / expected_eps;
}

Milliscore PlayerAB::alpha_beta(const Position &position,
                                const int depth,
                                const Milliscore alpha,
                                const Milliscore beta,
                                const bool probcut_allowed) {
  ++nodes_visited;

  if (depth == 0) {
    return evaluate(position);
  }

  if (current_time() >= deadline) throw Timeout{};

  const int move_number = position.move_number();

  TranspositionTableEntry *tt_entry = nullptr;
  if (depth >= min_tt_depth) {
    tt_entry = transposition_table.find(position);

    if (tt_entry && tt_entry->depth >= depth && tt_entry->probcut_allowed <= probcut_allowed) {
      if (tt_entry->type == EntryType::exact ||
          (tt_entry->type == EntryType::lower_bound &&
           tt_entry->score >= beta) ||
          (tt_entry->type == EntryType::upper_bound &&
           tt_entry->score <= alpha)) {
        return tt_entry->score;
      }
    }
  }

  if (probcut_allowed && depth >= min_probcut_depth && depth <= max_probcut_depth) {
    const ProbCutInfo &probcut_info = prob_cut_info_short[move_number][depth];
    if (probcut_info.shallow_depth != -1) {
      const double probcut_beta_d = beta + probcut_info.offset + probcut_info.stddev * probcut_stddevs;
      if (probcut_beta_d > -max_milliscore+2 && probcut_beta_d < max_milliscore-2) {
        const Milliscore probcut_beta = static_cast<Milliscore>(std::round(probcut_beta_d));
        if (alpha_beta(position, probcut_info.shallow_depth, probcut_beta-1, probcut_beta, false)
            >= probcut_beta) {
          return beta;
        }
      }

      const double probcut_alpha_d = alpha + probcut_info.offset - probcut_info.stddev * probcut_stddevs;
      if (probcut_alpha_d > -max_milliscore+2 && probcut_alpha_d < max_milliscore-2) {
        const Milliscore probcut_alpha = static_cast<Milliscore>(std::round(probcut_alpha_d));
        if (alpha_beta(position, probcut_info.shallow_depth, probcut_alpha, probcut_alpha+1, false)
            <= probcut_alpha) {
          return alpha;
        }
      }
    }
  }

  Milliscore best_score = -max_milliscore;
  Move best_move = invalid_move;
  Bitboard remaining_moves = position.valid_moves();

  while (remaining_moves) {
    Move move;
    if (tt_entry &&
        tt_entry->move != invalid_move &&
        get_bit(remaining_moves, tt_entry->move)) {
      move = tt_entry->move;
    } else if (killer_moves[move_number] != invalid_move &&
               get_bit(remaining_moves, killer_moves[move_number])) {
      move = killer_moves[move_number];
    } else {
      move = choose_move_statically(position, remaining_moves);
    }
    remaining_moves = reset_bit(remaining_moves, move);
    Position next_position;
    position.make_move(move, next_position);

    const Milliscore to_beat = std::max(alpha, best_score);
    const Milliscore limit = depth >= min_pv_depth ? to_beat + 1 : beta;
    Milliscore score =
      -alpha_beta(next_position, depth-1, -limit, -to_beat, probcut_allowed);

    if (score >= limit && score < beta) {
      score = -alpha_beta(next_position, depth-1, -beta, -to_beat, probcut_allowed);
    }

    if (score > best_score) {
      best_score = score;
      best_move = move;
      if (best_score >= beta) {
        killer_moves[move_number] = move;
        break;
      }
    }
  }

  if (depth >= min_tt_depth) {
    if (!tt_entry) {
      bool inserted;
      tt_entry = transposition_table.insert(position, inserted);
    }
    if (tt_entry) {
      if (depth >= tt_entry->depth) {
        tt_entry->depth = depth;
        tt_entry->probcut_allowed = probcut_allowed;
        tt_entry->score = best_score;
        tt_entry->move = best_move;
        tt_entry->type =
          best_score >= beta ? EntryType::lower_bound :
          best_score <= alpha ? EntryType::upper_bound :
          EntryType::exact;
      }
    }
  }

  return best_score;
}

Score PlayerAB::endgame_alpha_beta(const Position &position,
                                   const Score alpha,
                                   const Score beta) {
  ++nodes_visited;

  const int move_number = position.move_number();
  const int depth = num_squares - move_number;

  if (depth <= 3) {
    if (depth == 3) return endgame_3(position, alpha, beta);
    if (depth == 2) return endgame_2(position, beta);
    if (depth == 1) return endgame_1(position);
    return endgame_0(position);
  }

  if (current_time() >= deadline) throw Timeout{};

  TranspositionTableEntry *tt_entry = nullptr;
  if (depth >= endgame_min_tt_depth) {
    tt_entry = transposition_table.find(position);

    if (tt_entry && tt_entry->depth >= depth) {
      if (tt_entry->type == EntryType::exact ||
          (tt_entry->type == EntryType::lower_bound &&
           tt_entry->score >= beta << milliscore_bits) ||
          (tt_entry->type == EntryType::upper_bound &&
           tt_entry->score <= alpha << milliscore_bits)) {
        return tt_entry->score >> milliscore_bits;
      }
    }
  }

  Score best_score = -max_score;
  Move best_move = invalid_move;
  Bitboard remaining_moves = position.valid_moves();

  while (remaining_moves) {
    Move move;
    if (tt_entry &&
        tt_entry->move != invalid_move &&
        get_bit(remaining_moves, tt_entry->move)) {
      move = tt_entry->move;
    } else if (killer_moves[move_number] != invalid_move &&
               get_bit(remaining_moves, killer_moves[move_number])) {
      move = killer_moves[move_number];
    } else {
      move = choose_move_statically(position, remaining_moves);
    }
    remaining_moves = reset_bit(remaining_moves, move);
    Position next_position;
    position.make_move(move, next_position);

    const Score to_beat = std::max(alpha, best_score);
    const Score limit = depth >= endgame_min_pv_depth ? to_beat + 1 : beta;
    Score score = -endgame_alpha_beta(next_position, -limit, -to_beat);

    if (score >= limit && score < beta) {
      score = -endgame_alpha_beta(next_position, -beta, -to_beat);
    }

    if (score > best_score) {
      best_score = score;
      best_move = move;
      if (best_score >= beta) {
        killer_moves[move_number] = move;
        break;
      }
    }
  }

  if (depth >= endgame_min_tt_depth) {
    if (!tt_entry) {
      bool inserted;
      tt_entry = transposition_table.insert(position, inserted);
    }
    if (tt_entry) {
      if (depth >= tt_entry->depth) {
        tt_entry->depth = depth;
        tt_entry->probcut_allowed = false;
        tt_entry->score = best_score << milliscore_bits;
        tt_entry->move = best_move;
        tt_entry->type =
          best_score >= beta ? EntryType::lower_bound :
          best_score <= alpha ? EntryType::upper_bound :
          EntryType::exact;
      }
    }
  }

  return best_score;
}

inline Score PlayerAB::endgame_0(const Position &position) {
  return position.final_score();
}

inline Score PlayerAB::endgame_1(const Position &position) {
  const Move move = first_square(position.empty_squares());
  return endgame_1(position, move);
}

inline Score PlayerAB::endgame_1(const Position &position,
                                 const Move move) {
  Position next_position;
  position.make_move(move, next_position);
  return -endgame_0(next_position);
}

inline Score PlayerAB::endgame_2(const Position &position, const Score beta) {
  Bitboard semivalid_moves = position.empty_squares();
  const Move move0 = first_square(semivalid_moves);
  semivalid_moves = reset_bit(semivalid_moves, move0);
  const Move move1 = first_square(semivalid_moves);
  return endgame_2(position, beta, move0, move1);
}

inline Score PlayerAB::endgame_2(const Position &position,
                                 const Score beta,
                                 const Move move0,
                                 const Move move1) {
  Score score = -max_score;

  Position pos0;
  const bool move0_ok = position.make_move(move0, pos0);
  if (move0_ok) {
    score = -endgame_1(pos0, move1);
    if (score >= beta) return score;
  }

  Position pos1;
  const bool move1_ok = position.make_move(move1, pos1);
  if (move1_ok) {
    score = std::max<Score>(score, -endgame_1(pos1, move0));
  }

  if (score == -max_score) {
    score = -endgame_1(pos0, move1);
    if (score >= beta) return score;
    score = std::max<Score>(score, -endgame_1(pos1, move0));
  }

  return score;
}

inline Score PlayerAB::endgame_3(const Position &position,
                                 const Score alpha,
                                 const Score beta) {
  Bitboard semivalid_moves = position.empty_squares();
  const Move move0 = first_square(semivalid_moves);
  semivalid_moves = reset_bit(semivalid_moves, move0);
  const Move move1 = first_square(semivalid_moves);
  semivalid_moves = reset_bit(semivalid_moves, move1);
  const Move move2 = first_square(semivalid_moves);
  return endgame_3(position, alpha, beta, move0, move1, move2);
}

inline Score PlayerAB::endgame_3(const Position &position,
                                 const Score alpha,
                                 const Score beta,
                                 const Move move0,
                                 const Move move1,
                                 const Move move2) {
  Score score = -max_score;

  Position pos0;
  const bool move0_ok = position.make_move(move0, pos0);
  if (move0_ok) {
    score = -endgame_2(pos0, -alpha, move1, move2);
    if (score >= beta) return score;
  }

  Position pos1;
  const bool move1_ok = position.make_move(move1, pos1);
  if (move1_ok) {
    score = std::max<Score>(score, -endgame_2(pos1, -alpha, move0, move2));
  }

  Position pos2;
  const bool move2_ok = position.make_move(move2, pos2);
  if (move2_ok) {
    score = std::max<Score>(score, -endgame_2(pos2, -alpha, move0, move1));
  }

  if (score == -max_score) {
    score =                        -endgame_2(pos0, -alpha, move1, move2);
    if (score >= beta) return score;
    score = std::max<Score>(score, -endgame_2(pos1, -alpha, move0, move2));
    if (score >= beta) return score;
    score = std::max<Score>(score, -endgame_2(pos2, -alpha, move0, move1));
  }

  return score;
}

double PlayerAB::endgame_patzer_score(const Position &position) {
  ++nodes_visited;

  if (position.finished()) {
    return position.final_score();
  }

  int num_moves = 0;
  Score scores[max_moves];

  Bitboard remaining_moves = position.valid_moves();

  while (remaining_moves) {
    const Move move = first_square(remaining_moves);
    remaining_moves = reset_bit(remaining_moves, move);

    Position next_position;
    position.make_move(move, next_position);

    scores[num_moves++] =
      -endgame_alpha_beta(next_position, -max_score, max_score);
  }

  std::sort(scores, scores + num_moves, std::greater<Score>{});

  double total_weight = 0.0;
  double total_score = 0.0;

  double weight = 1.0;
  const double weight_decrease = std::exp(-patzer_skill / num_moves); 
  for (int choice = 0; choice < num_moves; ++choice) {
    total_weight += weight;
    total_score += weight * scores[choice];
    weight *= weight_decrease;
  }

  return total_score / total_weight;
}


inline Move PlayerAB::choose_move_statically(const Position &,
                                             const Bitboard move_options) {
  const Bitboard corner_options = move_options & corners;
  if (corner_options) return first_square(corner_options);
  return first_square(move_options);
}
