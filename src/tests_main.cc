#include "tests.h"
#include "evaluator.h"
#include "hashing.h"
#include "logging.h"
#include <iostream>
#include <vector>

namespace {
  struct TestCase {
    const char *name;
    void (*f)();
  };

  std::vector<TestCase> &get_all_tests() {
    static std::vector<TestCase> all_tests;
    return all_tests;
  }
}

TestCaseRegisterer::TestCaseRegisterer(const char *name, void (*f)()) {
  get_all_tests().push_back(TestCase{name, f});
}

int main() {
  verbosity = 0;
  init_hashing();
  init_evaluator();
  for (const TestCase &test_case : get_all_tests()) {
    std::cerr << "Running " << test_case.name << "\n";
    test_case.f();
  }
  std::cerr << "All tests passed.\n";
}
