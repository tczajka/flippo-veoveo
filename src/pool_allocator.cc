#include "pool_allocator.h"
#include "logging.h"
#include <memory>

PoolAllocator::PoolAllocator(const std::size_t _capacity) :
    m_capacity{_capacity},
    m_limit{_capacity},
    m_out_of_memory{false} {
  m_mem = reinterpret_cast<std::uintptr_t>(std::malloc(_capacity));
  assert(m_mem);
  log_info("PoolAllocator allocated %.2f MB\n",
      static_cast<double>(m_capacity) / (1<<20));
  m_next = m_mem;
}

PoolAllocator::~PoolAllocator() {
  std::free(reinterpret_cast<void*>(m_mem));
}

char *PoolAllocator::allocate_raw(const std::size_t size, const std::size_t align) {
  const std::uintptr_t next = (m_next + align - 1u) & ~(align-1u);
  const std::uintptr_t end = next + size;
  if (end - m_mem > m_limit) {
    m_out_of_memory = true;
    return nullptr;
  }
  m_next = end;
  return reinterpret_cast<char*>(next);
}
