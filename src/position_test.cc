#include "position.h"
#include "random.h"
#include "tests.h"

TEST(test_move_names) {
  assert(move_to_string(0) == "A1");
  assert(move_to_string(1) == "A2");
  assert(move_to_string(7) == "A8");
  assert(move_to_string(8) == "B1");
  assert(move_to_string(63) == "H8");

  assert(string_to_move("A1") == 0);
  assert(string_to_move("A2") == 1);
  assert(string_to_move("A8") == 7);
  assert(string_to_move("B1") == 8);
  assert(string_to_move("H8") == 63);

  assert(string_to_move("") == invalid_move);
  assert(string_to_move("I1") == invalid_move);
  assert(string_to_move("B0") == invalid_move);

  for (int sq = 0; sq < num_squares; ++sq) {
    assert(string_to_move(move_to_string(sq)) == sq);
  }
}

TEST(test_initial_position) {
  const Position p = Position::initial();
  assert(p == Position(
    "........"
    "........"
    "........"
    "...OX..."
    "...XO..."
    "........"
    "........"
    "........"));

  assert(p.move_number() == 4);

  assert(p.to_string(true) ==
    "........\n"
    "........\n"
    "........\n"
    "...OX...\n"
    "...XO...\n"
    "........\n"
    "........\n"
    "........\n");

  assert(p.to_string() ==
    "........"
    "........"
    "........"
    "...OX..."
    "...XO..."
    "........"
    "........"
    "........");
}

TEST(test_finished_and_score) {
  assert(!Position(
    "OXXOXOXO"
    "XOOXOXXX"
    "OXXOXOXO"
    "OXXOXOXO"
    "OXXOXOX."
    "OXXOXOXO"
    "OXXOXOXO"
    "OXXOXOXO").finished());

  const Position pos1(
    "OXXOXOXO"
    "XOOXOXXX"
    "OXXOXOXO"
    "OXXOXOXO"
    "OXXOXOXO"
    "OXXOXOXO"
    "OXXOXOXO"
    "OXXOXOXO");

  assert(pos1.finished());
  assert(pos1.final_score() == -1);

  const Position pos2(std::string(64, 'O'));
  assert(pos2.finished());
  assert(pos2.final_score() == 32);
}

TEST(test_make_move) {
  const Position pos1(
    ".X...O.."
    "..X.O..."
    ".XO.O.OX"
    "..OOO..X"
    ".O.X.X.X"
    "O.....OX"
    "...OOOOX"
    "..OX....");

  assert(pos1.to_move() == 1);

  Position pos2;
  assert(pos1.make_move(19, pos2));

  assert(pos2 == Position(
    ".X...O.."
    "..O.O..."
    ".XXXO.OX"
    "..OXX..X"
    ".O.X.O.X"
    "O.....XX"
    "...OOOOX"
    "..OX...."));

  // No flips.
  const Position pos3(
    "........"
    "........"
    "........"
    "...OX..."
    "...OX..."
    "........"
    "........"
    "........");

  Position pos4;
  assert(!pos3.make_move(20, pos4));

  assert(pos4 == Position(
    "........"
    "........"
    "....O..."
    "...OX..."
    "...OX..."
    "........"
    "........"
    "........"));
}

TEST(test_valid_moves) {
  const Position pos1(
    "........"
    "........"
    "...XXO.."
    "..OX.X.."
    "...OOO.."
    "........"
    "........"
    "........");

  assert(pos1.to_move() == 1);

  assert(pos1.valid_moves() == bitboard_from_string(
    "........"
    "...X.X.."
    "..X...X."
    ".X......"
    ".XX...X."
    "...X.X.."
    "........"
    "........"));

  // No flips possible.
  const Position pos2(
    "........"
    "...O...."
    "........"
    "...X...."
    "...XX..."
    "........"
    "........"
    "........");

  assert(pos2.to_move() == 0);

  assert(pos2.valid_moves() == bitboard_from_string(
    "..XXX..."
    "..X.X..."
    "..XXX..."
    "..X.XX.."
    "..X..X.."
    "..XXXX.."
    "........"
    "........"));
}

TEST(test_moves_vs_slow) {
  RandomGenerator rng;
  for (int games = 0; games < 10000; ++games) {
    Position pos = Position::initial();
    while (!pos.finished()) {
      const Bitboard valid_moves = pos.valid_moves();
      const Bitboard valid_moves_slow = pos.valid_moves_slow();
      if (valid_moves != valid_moves_slow) {
        std::cout << pos.to_string(true) << "\n";
        std::cout << pos.to_move() << "\n";
        std::cout << bitboard_to_string_multiline(valid_moves) << "\n";
        std::cout << bitboard_to_string_multiline(valid_moves_slow) << "\n";
      }
      assert(valid_moves == valid_moves_slow);
      const Move move = rng.get_square(valid_moves);
      Position pos2;
      Position pos3;
      const bool a = pos.make_move_slow(move, pos2);
      const bool b = pos.make_move(move, pos3);
      assert(a==b);
      if (pos2 != pos3) {
        std::cout << pos.to_string(true) << "\n";
        std::cout << pos.to_move() << "\n";
        std::cout << move_to_string(move) << "\n";
        std::cout << pos2.to_string(true) << "\n";
        std::cout << pos3.to_string(true) << "\n";
      }
      assert(pos2 == pos3);
      pos = pos2;
    }
  }
}


TEST(test_position_normalize) {
  const Position pos1(
    "........"
    "........"
    "..XXX..."
    "...OO..."
    "....O..."
    "........"
    "........"
    "........");

  const Position pos2(
    "........"
    "........"
    "........"
    "...OOX.."
    "....OX.."
    ".....X.."
    "........"
    "........");


  Position pos1_normalized;
  const int tr1 = pos1.normalize(pos1_normalized);

  Position pos2_normalized;
  const int tr2 = pos2.normalize(pos2_normalized);

  assert(pos1_normalized == pos2_normalized);
  assert(pos1.transform(tr1) == pos2.transform(tr2));
}
