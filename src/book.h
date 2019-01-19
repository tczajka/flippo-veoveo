#ifndef BOOK_H
#define BOOK_H

#include "position.h"
#include "evaluator.h"
#include <string>

extern const size_t num_book_entries;
extern const char *const book_entries[];

// invalid_move if not found
Move find_book_move(const Position &);

char encode_square(int sq);
int decode_square(char c);

std::string encode_bitboard(Bitboard b);
std::string encode_position(const Position &);

#endif
