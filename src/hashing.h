#ifndef HASHING_H
#define HASHING_H

#include "arch.h"
#include "logging.h"
#include "position.h"
#include <cassert>
#include <cstdint>
#include <limits>
#include <memory>

void init_hashing();

using Hash = std::uint32_t;

Hash hash_position(const Position &position);

template<typename Value>
class PositionHashTable {
public:
  explicit PositionHashTable(std::size_t buckets);
  ~PositionHashTable();

  Value *find(const Position &position) {
    return const_cast<Value*>(const_cast<const PositionHashTable*>(this)->find(position));
  }

  const Value *find(const Position &position) const;

  // nullptr if out of memory
  template<typename... Args>
  Value *insert(const Position &position, bool &inserted, Args&&... args);

  std::size_t capacity() const { return m_capacity; }
  std::size_t size() const {  return m_size; }
  std::size_t limit() const {  return m_limit; }

  void set_limit(const std::size_t _limit) {
    assert(_limit <= m_capacity);
    m_limit = _limit;
    m_out_of_memory = false;
  }

  bool out_of_memory() const {  return m_out_of_memory; }

private:
  struct Entry {
    Entry() {}
    ~Entry() {
      if (valid) value.~Value();
    }

    Position position;
    union {
      Value value;
    };
    bool valid = false;
  };

  std::size_t m_mask;
  std::size_t m_capacity;
  std::size_t m_size;
  std::size_t m_limit;
  bool m_out_of_memory;
  Entry *entries;

public:
  static constexpr std::size_t sizeof_entry = sizeof(Entry);

};

template<typename Value>
PositionHashTable<Value>::PositionHashTable(const std::size_t buckets) :
    m_mask{buckets-1u},
    m_capacity{buckets / 16u * 15u},
    m_size{0},
    m_limit{m_capacity},
    m_out_of_memory{false}
{
  assert(buckets >= 16 &&
         buckets <= std::numeric_limits<std::uint32_t>::max() &&
         (buckets & (buckets-1u))==0);

  void *memptr = nullptr;
  const size_t bytes = buckets * sizeof(Entry);
  static_assert(64 % alignof(Entry) == 0, "");
  int ret = posix_memalign(&memptr, 64, bytes);
  assert(ret==0);
  entries = reinterpret_cast<Entry*>(memptr);
  assert(entries);
  for (size_t i=0; i<buckets; ++i)
    new (entries+i) Entry{};

  log_info("PositionHashTable allocated %.2f MB\n",
      static_cast<double>(bytes) / (1<<20));
}

template<typename Value>
PositionHashTable<Value>::~PositionHashTable() {
  for (size_t i=0; i<=m_mask; ++i) {
    entries[i].~Entry();
  }
  std::free(entries);
}

template<typename Value>
const Value *PositionHashTable<Value>::find(const Position &position) const {
  std::size_t pos = hash_position(position) & m_mask;
  for (;;) {
    const Entry &entry = entries[pos];
    if (!entry.valid) return nullptr;
    if (entry.position == position) return &entry.value;
    pos = (pos + 1u) & m_mask;
  }
}

template<typename Value>
template<typename... Args>
Value *PositionHashTable<Value>::insert(const Position &position, bool &inserted, Args&&... args) {
  std::size_t pos = hash_position(position) & m_mask;
  for (;;) {
    Entry &entry = entries[pos];
    if (!entry.valid) {
      if (m_size >= m_limit) {
        m_out_of_memory = true;
        inserted = false;
        return nullptr;
      }
      entry.position = position;
      entry.valid = true;
      ++m_size;
      inserted = true;
      return new(&entry.value) Value(std::forward<Args>(args)...);
    }
    if (entry.position == position) {
      inserted = false;
      return &entry.value;
    }
    pos = (pos + 1u) & m_mask;
  }
}

#endif
