#include "prepared.h"

const Prepared prepared_games[] = {
  // white vs first0 (x2, 50M/56)
  { 0,28, {-1,-1,-1,-1,20,19,10,11,1,3,34,12,4,2,0,5,6,9,26,8,21,13,33,14,32,7,40,15,48,23,56,24,16,25,17,29,43,18,50,22,45,30,44,31,39,37,52,38,54,41,63,42,46,47,57,49,55,51,59,53,58,60,61,62,}},
  // white vs first3 (50M/56)
  { 0,28, {-1,-1,-1,-1,20,37,43,50,38,57,45,54,63,29,22,15,44,53,62,61,39,55,21,52,23,60,59,58,56,51,31,49,19,48,12,47,13,42,7,46,18,41,9,40,26,34,33,32,10,30,8,25,0,24,2,17,11,16,4,14,1,6,5,3,}},
  // white vs first4 (x1, 50M/56)
  { 0,27, {-1,-1,-1,-1,20,19,10,11,1,9,2,3,0,8,34,17,24,25,42,33,32,41,21,49,57,40,56,16,48,50,58,26,43,18,14,51,4,59,60,12,45,44,46,52,7,5,13,29,6,37,61,53,55,22,31,30,15,38,23,54,62,39,47,63,}},
  // white vs first7 (50M/56)
  { 0,27, {-1,-1,-1,-1,20,37,43,38,45,52,61,31,53,59,13,54,63,55,39,47,19,23,12,15,7,46,60,62,11,30,3,22,58,14,57,6,4,29,2,21,1,5,26,44,51,50,0,42,33,34,56,18,10,49,32,41,48,25,17,9,40,24,16,8,}},
  // white vs greedy0 (x1, 50M/56)
  { 0,30, {-1,-1,-1,-1,20,19,10,13,43,44,6,1,52,51,26,21,29,22,23,5,4,33,3,53,7,2,31,34,61,59,37,60,15,30,0,39,47,55,63,50,58,49,48,57,25,62,40,16,56,38,24,32,42,41,8,54,11,46,12,17,45,14,9,18,}},
  // white vs greedy3 (x1, 50M/56)
  { 0,31, {-1,-1,-1,-1,20,37,43,12,38,46,54,34,25,50,13,16,45,6,63,30,4,21,39,31,55,47,57,3,2,51,1,49,23,15,0,5,59,29,42,58,56,60,53,33,7,48,44,62,61,18,24,41,32,40,52,19,17,9,10,26,11,14,8,22,}},
  // white vs greedy4 (50M/56 overlapping)
  { 0,31, {-1,-1,-1,-1,20,19,10,13,43,44,6,1,52,51,26,33,42,21,61,34,58,60,59,5,4,49,17,8,56,24,53,62,50,48,3,57,7,2,25,16,0,40,54,46,55,47,32,39,63,38,37,18,31,23,12,11,41,22,9,14,45,29,15,30,}},
  // white vs greedy7 (50M/56 overlapping)
  { 0,31, {-1,-1,-1,-1,20,37,43,12,38,46,54,34,25,50,13,16,45,6,63,30,4,21,39,31,55,47,57,3,2,44,1,42,29,51,58,60,33,32,56,11,23,15,24,59,0,5,22,14,7,62,40,48,61,49,18,41,53,9,52,19,8,26,17,10,}},

  // black vs first0 (x2, 50M/55)
  { 1,28, {-1,-1,-1,-1,18,37,45,53,9,0,29,19,20,46,10,47,1,2,11,55,4,26,12,16,8,39,13,44,3,5,6,7,14,21,15,43,24,63,31,33,17,40,22,23,25,32,34,48,30,38,41,52,42,49,50,61,51,60,54,56,57,62,58,59,}},
  // black vs first3 (50M/55)
  { 1,28, {-1,-1,-1,-1,45,26,18,10,54,63,34,44,43,17,53,16,62,61,52,8,59,37,51,47,55,24,50,19,60,58,57,56,49,42,48,20,39,0,32,30,46,23,41,40,38,31,29,15,33,25,22,11,21,14,13,2,12,3,9,7,6,1,5,4,}},
  // black vs first4 (x1, 50M/55)
  { 1,28, {-1,-1,-1,-1,18,37,45,53,9,0,43,46,34,55,25,47,16,19,32,17,1,11,33,10,41,8,24,40,48,56,49,51,57,3,59,50,4,39,2,5,26,6,42,61,58,7,12,60,20,62,44,13,52,30,21,38,29,14,22,15,54,23,31,63,}},
  // black vs first7 (50M/55)
  { 1,29, {-1,-1,-1,-1,45,26,18,10,54,63,20,13,44,21,46,5,47,53,62,60,38,55,31,52,39,23,15,7,30,59,22,25,14,58,6,57,61,42,37,2,29,56,12,19,4,3,51,17,43,34,11,1,50,0,49,48,41,33,9,24,40,32,16,8,}},
  // black vs greedy0 (x1, 50M/55)
  { 1,32, {-1,-1,-1,-1,20,37,42,12,38,49,4,56,43,21,19,51,59,44,30,11,26,58,2,17,50,48,41,53,60,40,46,32,24,16,8,61,14,45,62,0,57,3,18,7,47,5,6,63,31,23,55,10,15,39,54,1,13,52,22,25,9,29,34,33,}},
  // black vs greedy3 (x1, 50M/55)
  { 1,32, {-1,-1,-1,-1,43,26,21,51,25,14,59,7,20,42,44,12,4,19,33,52,37,5,61,46,13,15,22,10,3,23,17,31,39,47,55,2,49,18,1,63,6,60,45,56,16,58,57,0,32,40,8,53,48,24,9,62,50,11,41,38,54,34,29,30,}},
  // black vs greedy4 (50M/55)
  { 1,32, {-1,-1,-1,-1,34,44,21,33,52,14,32,7,29,42,26,30,31,37,51,25,19,23,16,10,22,6,13,46,39,5,53,4,3,2,1,47,49,45,55,0,15,24,18,56,61,40,48,63,59,58,62,17,57,60,54,8,41,38,50,11,9,43,20,12,}},
  // black vs greedy7 (50M/55)
  { 1,32, {-1,-1,-1,-1,29,19,42,30,11,49,31,56,34,21,37,33,32,26,12,38,44,40,47,53,41,57,50,17,24,58,10,59,60,61,62,16,14,18,8,63,48,39,45,7,2,23,15,0,4,5,1,46,6,3,9,55,22,25,13,52,54,20,43,51,}},
};

Move find_prepared_game(int move_number,
                        const Move (&moves_so_far)[num_squares]) {
  for (const Prepared &game : prepared_games) {
    if (game.color != move_number % 2) continue;
    bool match = true;
    for (int i=4; i<move_number; ++i) {
      if(moves_so_far[i] != game.moves[i]) {
        match = false;
        break;
      }
    }
    if (match) return game.moves[move_number];
  }
  return invalid_move;
}