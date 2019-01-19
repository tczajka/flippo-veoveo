# flippo-veoveo

This is my (Tomek Czajka's) entry for [CodeCup 2019](https://www.codecup.nl/), an annual AI programming competition.

The game for this year's tournament was [Flippo](https://www.codecup.nl/flippo/rules.php).

A summary of my entry's algorithm:
* The main search algorithm is [Principal Variation Search](https://en.wikipedia.org/wiki/Principal_variation_search).
* Does forward prunning using a version of [Multi-Prob Cut](https://en.wikipedia.org/wiki/Multi-Prob_Cut).
* Tries to entice opponents to make mistakes once the endgame is solved.
* The evaluation function is interesting.  For each pattern for each line on the board (horizontal, vertical and diagonal), I pre-compute probabilities that each stone will get flipped an odd number of times. This is based on the 1D version of the game with completely random play. Then I assume that flips along different lines are independent, to compute total odd-flips probabilities. This gives me the expected score on each square, and I just add these up to get total expected score.
* Corner moves are particularly good, so there is a bonus in the evaluation function for the availability of corner moves. Also, I try corner moves first in the search.
* [Killer move heuristic](https://en.wikipedia.org/wiki/Killer_heuristic) for move ordering.
* An opening book was automatically built, with some lines going 10 ply deep.
* To score more points against weak opponents, full games against certain simple deterministic and greedy strategies were added to the opening book. These games were generated using [Beam Search](https://en.wikipedia.org/wiki/Beam_search). These sometimes led to awesome results, such as [this 64-0 wipeout](https://www.codecup.nl/showgame.php?ga=137769).
* SSE was used for the evaluation function.
* [Kogge-Stone](https://en.wikipedia.org/wiki/Kogge%E2%80%93Stone_adder) algorithm was used to generate all valid moves in parallel.
* Crazy template metaprogramming was used to have the compiler generate efficient "make move" functions, separately for each square on the board.
* This resulted in processing about 7 million positions / second on the competition computer.
