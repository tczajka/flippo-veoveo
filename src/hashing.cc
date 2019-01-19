#include "hashing.h"
#include "random.h"
#include <limits>

Hash hash_player_row[8][256];
Hash hash_opponent_row[8][256];

void init_hash_row(Hash (&row)[256]) {
  auto &rng = get_random_device();
  row[0] = 0;
  for (int p=1; p<256; p<<=1) {
    static_assert(rng.min() == 0u, "");
    static_assert(rng.max() == std::numeric_limits<Hash>::max(), "");
    const Hash h = rng();
    for (int q=0; q<p; ++q) {
      row[p|q] = row[q] ^ h;
    }
  }
}

void init_hashing() {
  for (int row = 0; row < 8; ++row) {
    init_hash_row(hash_player_row[row]);
    init_hash_row(hash_opponent_row[row]);
  }
}

Hash hash_position(const Position &position) {
  const unsigned char *const p = reinterpret_cast<const unsigned char*>(&position.player);
  const unsigned char *const q = reinterpret_cast<const unsigned char*>(&position.opponent);

  Hash h = 0;
  for (int row=0; row<8; ++row) {
    h ^= hash_player_row[row][p[row]];
  }
  for (int row=0; row<8; ++row) {
    h ^= hash_opponent_row[row][q[row]];
  }

  return h;
}
