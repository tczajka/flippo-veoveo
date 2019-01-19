#include "hashing.h"
#include "tests.h"

TEST(test_hash_position) {
  assert(hash_position(Position::initial()) != 0);
}

TEST(test_position_hash_table) {
  PositionHashTable<int> table(1<<4);

  Position pos1(
    "........"
    "........"
    "........"
    "...OO..."
    "...XXX.."
    "........"
    "........"
    "........");

  Position pos2 = Position::initial();

  bool inserted = false;
  int *p = table.insert(pos1, inserted, 1);
  assert(inserted);
  assert(*p == 1);

  assert(!table.find(pos2));
  p = table.insert(pos2, inserted, 2);
  assert(inserted);
  assert(*p == 2);

  p = table.insert(pos2, inserted, 3);
  assert(!inserted);
  assert(*p == 2);

  p = table.insert(pos1, inserted, 4);
  assert(!inserted);
  assert(*p == 1);

  int *f1 = table.find(pos1);
  assert(f1 && *f1 == 1);
  int *f2 = table.find(pos2);
  assert(f2 && *f2 == 2);
  assert(table.size() == 2);
  assert(table.capacity() == 15);
}

namespace {
  struct A {
    static long constructor_count;
    static long destructor_count;

    A() { ++constructor_count; }
    ~A() { ++destructor_count; }
  };

  long A::constructor_count = 0;
  long A::destructor_count = 0;
}

TEST(test_position_hash_destructor) {
  {
    PositionHashTable<A> table(1<<4);
    bool inserted = false;
    table.insert(Position::initial(), inserted);
    assert(inserted);
    table.insert(Position::initial(), inserted);
    assert(!inserted);
  }
  assert(A::constructor_count == 1);
  assert(A::destructor_count == 1);
}

TEST(test_position_hash_limit) {
  const Position pos1(
    "........"
    "........"
    "........"
    "...OO..."
    "...XXX.."
    "........"
    "........"
    "........");

  const Position pos2(
    "........"
    "........"
    "........"
    "...OOO.."
    "...XXX.."
    "........"
    "........"
    "........");

  PositionHashTable<int> table(16);
  table.set_limit(1);
  bool inserted;
  int *p = table.insert(Position::initial(), inserted, 1);
  assert(inserted && *p == 1);
  p = table.insert(pos1, inserted, 2);
  assert(!inserted && !p);
  assert(table.out_of_memory());
  table.set_limit(2);
  p = table.insert(pos1, inserted, 2);
  assert(inserted && *p == 2);
  assert(!table.out_of_memory());
}
