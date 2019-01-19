#include "book.h"
#include "tests.h"

TEST(test_encode_square) {
  for (int i=0;i<64;++i) {
    assert(decode_square(encode_square(i)) == i);
  }
}
