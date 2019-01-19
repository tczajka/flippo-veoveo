#include "random.h"
#include <algorithm>
#include <functional>

class TrueRandomGenerator {
public:
  template<typename Iter>
  void generate(const Iter first, const Iter second) {
    std::generate(first, second, std::ref(get_random_device()));
  }
private:
  static_assert(std::random_device::min() == 0u, "");
  static_assert(std::random_device::max() == 0xffffffffu, "");
};

std::random_device &get_random_device() {
  static std::random_device rdev;
  return rdev;
}

RandomGenerator::RandomGenerator() {
  TrueRandomGenerator trng;
  urng.seed(trng);
}
