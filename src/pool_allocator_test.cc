#include "pool_allocator.h"
#include "tests.h"

TEST(test_pool_allocator) {
  PoolAllocator alloc(16);
  alloc.set_limit(3);
  assert(!alloc.construct<int>(7));
  assert(alloc.out_of_memory());
  alloc.set_limit(16);

  int *a = alloc.construct<int>(7);
  assert(a && *a == 7);
  std::int64_t *b = alloc.construct<std::int64_t>(8);
  assert(b && *b == 8);
  assert(reinterpret_cast<std::uintptr_t>(b) % 8u == 0u);
  assert(alloc.used() == 16);

  assert(!alloc.allocate<char>());
}

TEST(test_compressed_ptr) {
  PoolAllocator alloc(16);
  const CompressedPtr<int> cp = alloc.compress<int>(nullptr);
  assert(cp.is_null());
  assert(alloc.decompress(cp) == nullptr);

  int *a = alloc.construct<int>(7);
  assert(a && *a == 7);
  const CompressedPtr<int> ca = alloc.compress(a);
  assert(!ca.is_null());
  assert(alloc.decompress(ca) == a);
}
