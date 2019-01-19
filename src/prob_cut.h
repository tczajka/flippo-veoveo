#ifndef PROB_CUT_H
#define PROB_CUT_H

#include "position.h"

constexpr int min_probcut_depth = 2;
constexpr int max_probcut_depth = 8;
constexpr double probcut_stddevs = 1.3;

struct ProbCutInfo {
  int shallow_depth; // -1 for no probcut
  double offset; // shallow - deep
  double stddev;
};

// [move number][deep][shallow]
extern const ProbCutInfo prob_cut_info_long[num_squares][max_probcut_depth+1][max_probcut_depth];

extern const int probcut_depth[max_probcut_depth+1];

// [move number][deep]
extern const ProbCutInfo prob_cut_info_short[num_squares][max_probcut_depth+1];

#endif
