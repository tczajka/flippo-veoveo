#ifndef LOGGING_H
#define LOGGING_H

#include "arch.h"
#include <cstdio>

extern int verbosity;

#define log_verbosity(v, ...) \
  if (verbosity >= v) std::fprintf(stderr, __VA_ARGS__)

#define log_always(...) log_verbosity(0, __VA_ARGS__)
#define log_info(...) log_verbosity(1, __VA_ARGS__)
#define log_verbose(...) log_verbosity(2, __VA_ARGS__)

#endif
