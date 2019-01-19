#ifndef TESTS_H
#define TESTS_H

#include "arch.h"
#include <cassert>
#include <iostream>

struct TestCaseRegisterer {
  TestCaseRegisterer(const char *name, void (*f)());
};

#define TEST(name) \
  void name(); \
  TestCaseRegisterer test_case_registerer_ ## name(#name, name); \
  void name()

#endif
