#include "evaluator.h"
#include "hashing.h"
#include "logging.h"
#include "prob_cut.h"
#include <fstream>

int main() {
  verbosity = 0;
  init_hashing();
  init_evaluator();

  std::ofstream f("prob_cut_info_short.tmp");
  f << "#include \"prob_cut.h\"\n";
  f << "extern const ProbCutInfo prob_cut_info_short[num_squares][max_probcut_depth+1] = {\n";
  for (int move_number=0;move_number<num_squares;++move_number) {
    f << "// move " << move_number << "\n";
    f << "{\n";
    for (int deep=0;deep<=max_probcut_depth;++deep) {
      const int shallow = probcut_depth[deep];
      double offset = 0;
      double stddev = 0;
      if (shallow != -1) {
        const ProbCutInfo &info = prob_cut_info_long[move_number][deep][shallow];
        offset = info.offset;
        stddev = info.stddev;
      }
      f << "  {" << shallow << "," << offset << "," << stddev << "},\n";
    }
    f << "},\n";
  }
  f << "};\n";
}
