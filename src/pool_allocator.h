#ifndef POOL_ALLOCATOR_H
#define POOL_ALLOCATOR_H

#include "arch.h"
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <utility>

template<typename T>
class CompressedPtr {
public:
  CompressedPtr() : p{0} {}
  bool is_null() const { return p == 0u; }
private:
  CompressedPtr(const std::uint32_t _p) : p{_p} {}
  std::uint32_t p;
  friend class PoolAllocator;
};

class PoolAllocator {
public:
  explicit PoolAllocator(std::size_t _capacity);
  ~PoolAllocator();

  std::size_t capacity() const { return m_capacity; }
  std::size_t used() const { return m_next - m_mem; }
  std::size_t limit() const { return m_limit; }

  void set_limit(const std::size_t _limit) {
    assert(_limit <= m_capacity);
    m_limit = _limit;
    m_out_of_memory = false;
  }

  bool out_of_memory() const { return m_out_of_memory; }

  template<typename T>
  T *allocate(std::size_t n = 1) {
    return reinterpret_cast<T*>(allocate_raw(n * sizeof(T), alignof(T)));
  }

  template<typename T, typename... Args>
  T *construct(Args&&... args) {
    T *const p = allocate<T>();
    if (p) {
      new (p) T(std::forward<Args>(args)...);
    }
    return p;
  }

  template<typename T>
  CompressedPtr<T> compress(T *p) const {
    if (!p) return CompressedPtr<T>{};
    const std::uint32_t offset =
        reinterpret_cast<std::uintptr_t>(p) - m_mem;
    return CompressedPtr<T>{offset + 1u};
  }

  template<typename T>
  T *decompress(const CompressedPtr<T> cp) const {
    if (cp.is_null()) return nullptr;
    else return reinterpret_cast<T*>(m_mem + cp.p - 1u);
  }

private:
  char *allocate_raw(std::size_t size, std::size_t align);

  std::uintptr_t m_mem;
  std::uintptr_t m_next;
  std::size_t m_capacity;
  std::size_t m_limit;

  bool m_out_of_memory;
};

#endif
