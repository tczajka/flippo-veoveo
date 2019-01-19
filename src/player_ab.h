#ifndef PLAYER_AB_H
#define PLAYER_AB_H

#include "evaluator.h"
#include "hashing.h"
#include "player.h"

class PlayerAB : public Player {
public:
  PlayerAB();

  Move choose_move(const Position &position,
                   const PlaySettings &settings) override;

  void opponent_move(const Position &position, Move move) override;

  Milliscore get_last_move_milliscore() const {
    return last_move_milliscore;
  }

  Milliscore evaluate_depth(const Position &position, int depth);

private:
  static constexpr std::size_t transposition_table_buckets = 1<<22;

  static constexpr int allocation_move0  = 1000;
  static constexpr int allocation_move50 = 4000;
  static constexpr int endgame_solve_allocation = 4000; // tweak
  static constexpr int after_solved_allocation = 1000;

  // Computer speed compared to my laptop speed.
  // Overestimating this by 2x results leaving less time for endgame
  //   Test results:  0.02 +- 0.02
  //   (hmm this is good?? is endgame_solve_allocation too large?)
  // Underestimating this by 2x results in leaving more time endgame
  //   Test results: -0.01 +- 0.02
  static constexpr double computer_speed =
#ifdef SUBMISSION
    0.9;
#else
    1.0;
#endif

  static constexpr double expected_eps = 8e8 * computer_speed;

  static constexpr double rough_endgame_branching_factor = 4.0;

  static constexpr int max_eval_move_number = 58;
  static constexpr int min_tt_depth = 2;
  static constexpr int endgame_min_tt_depth = 4;
  // In units of score / 1024:
  static constexpr Milliscore aspiration_width = 200 << (milliscore_bits - 10);
  static constexpr Score endgame_aspiration_width = 1;

  static constexpr int min_pv_depth = 3;
  static constexpr int endgame_min_pv_depth = 4;

  static constexpr int deadline_go_deeper_percentage = 50;
  static constexpr int deadline_next_move_percentage = 75;
  static constexpr int deadline_drop_work_percentage = 100;

  // Probability of worst move = probability of best move * exp(-patzer_skill).
  static constexpr double patzer_skill = 1.0;

  struct Timeout {};

  enum class EntryType : std::int8_t {
    exact,
    lower_bound,
    upper_bound
  };

  struct TranspositionTableEntry {
    int depth=0;
    Milliscore score=0;
    EntryType type=EntryType::exact;
    Move move=invalid_move;
    bool probcut_allowed=false;
  };
  static_assert(PositionHashTable<TranspositionTableEntry>::sizeof_entry == 32,
                "");

  void allocate_resources(const Position &position,
                          const PlaySettings &settings);
  double rough_time_to_solve(const int depth);

  Milliscore alpha_beta(const Position &position, const int depth,
                        const Milliscore alpha, const Milliscore beta,
                        bool probcut_allowed);
  Score endgame_alpha_beta(const Position &position,
                           const Score alpha, const Score beta);

  Score endgame_0(const Position &position);
  Score endgame_1(const Position &position);
  Score endgame_1(const Position &position, const Move move);
  Score endgame_2(const Position &position, const Score beta);
  Score endgame_2(const Position &position, const Score beta,
                  const Move move0, const Move move1);
  Score endgame_3(const Position &position, const Score alpha, const Score beta);
  Score endgame_3(const Position &position,
                  const Score alpha, const Score beta,
                  const Move move0, const Move move1, const Move move2);

  double endgame_patzer_score(const Position &position);

  Move choose_move_statically(const Position &position,
                              Bitboard move_options);

  PositionHashTable<TranspositionTableEntry> transposition_table;
  Move killer_moves[num_squares];
  Timestamp deadline;
  Timestamp deadline_go_deeper;
  Timestamp deadline_next_move;
  Timestamp deadline_drop_work;
  std::int64_t nodes_visited;
  Milliscore last_move_milliscore = 0;

  Move moves_so_far[num_squares];
};

#endif
