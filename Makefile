CXXFLAGS := -O2 -std=c++17 -Wall -Wextra -pthread

SOURCES := $(wildcard src/*.cc)
TESTS_SRC := $(wildcard src/*_test.cc)
BINARIES_SRC := $(wildcard src/*_main.cc)
REGULAR_SRC := $(filter-out $(TESTS_SRC) $(BINARIES_SRC),$(SOURCES))
TESTS_OBJ := $(TESTS_SRC:src/%.cc=obj/%.o)
REGULAR_OBJ := $(REGULAR_SRC:src/%.cc=obj/%.o)
BINARIES_BIN := $(BINARIES_SRC:src/%_main.cc=bin/%)
DEPENDENCIES := $(SOURCES:src/%.cc=obj/%.d)

SUBMISSION_FILES := \
  src/submission_header.h \
  src/arch.h \
  src/mathematics.h \
  src/bitboard.h \
  src/logging.h \
  src/random.h \
  src/position.h \
  src/hashing.h \
  src/clock.h \
  src/pool_allocator.h \
  src/evaluator.h \
  src/book.h \
  src/prepared.h \
  src/player.h \
  src/prob_cut.h \
  src/player_ab.h \
  src/bitboard.cc \
  src/logging.cc \
  src/random.cc \
  src/position.cc \
  src/hashing.cc \
  src/clock.cc \
  src/pool_allocator.cc \
  src/evaluator.cc \
  src/book.cc \
  src/book_data.cc \
  src/prepared.cc \
  src/prob_cut_info_short.cc \
  src/player_ab.cc \
  src/player_main.cc

BENCHMARK_FILES := \
  src/submission_header.h \
  src/bitboard.h \
  src/clock.h \
  src/logging.h \
  src/random.h \
  src/bitboard.cc \
  src/clock.cc \
  src/logging.cc \
  src/random.cc \
  src/benchmark_main.cc

.PHONY: all
all: $(BINARIES_BIN) bin/submission bin/benchmark_sub

.PHONY: clean
clean:
	rm -fr obj bin submission.cc benchmark_sub.cc

obj:
	mkdir -p obj

bin:
	mkdir -p bin

obj/%.o : src/%.cc | obj
	$(CXX) $(CXXFLAGS) -c -o $@ $<

bin/% : $(REGULAR_OBJ) obj/%_main.o | bin
	$(CXX) $(CXXFLAGS) -o $@ $^

bin/tests : $(TESTS_OBJ)

bin/submission : submission.cc | bin
bin/benchmark_sub : benchmark_sub.cc | bin

bin/submission bin/benchmark_sub :
	$(CXX) $(CXXFLAGS) -o $@ $^

obj/%.d : src/%.cc | obj
	$(CXX) $(CXXFLAGS) -MM -MT '$(@:%.d=%.o) $@' $< > $@

submission.cc : $(SUBMISSION_FILES)
benchmark_sub.cc : $(BENCHMARK_FILES)

submission.cc benchmark_sub.cc:
	{ cat $^ | grep '^#include <' | sort | uniq && cat $^ | grep -v '^#include'; } > $@

-include $(DEPENDENCIES)
