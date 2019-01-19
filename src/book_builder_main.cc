#include "book.h"
#include "clock.h"
#include "evaluator.h"
#include "hashing.h"
#include "logging.h"
#include "player_ab.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

namespace {

struct BookNode;

struct BookEdge {
  BookNode *destination;
  Move move;
};

struct BookNode {
  Position position;
  bool expanded;
  bool initialized;
  bool visited = false;
  Milliscore value;
  Move best_move;
  Milliscore expand_badness_player;
  Milliscore expand_badness_opponent;
  std::vector<BookEdge> children;
  std::vector<BookEdge> parents;
};

class BookBuilder {
public:
  BookBuilder(int argc, char **argv);
  void go();
private:
  BookNode *find_or_create_node(const Position &position);
  void initialize(BookNode *);
  BookNode *find_node_to_expand();
  void expand(BookNode *);
  void expand_thread(BookNode *);
  void update(BookNode *);
  void update_node(BookNode *);
  void print_book();
  void collect_entries(BookNode *node,
                       bool player,
                       std::vector<std::string> &entries,
                       std::vector<BookNode*> &all_visited);

  Duration think_time{};
  Duration total_time{};
  int num_threads=1;
  static constexpr Milliscore depth_badness = 200000;

  BookNode *root;
  PositionHashTable<BookNode*> lookup_table{1<<20};
  std::mutex expand_lock;
  size_t num_nodes = 0;
  int max_move_number = 0;
};

BookBuilder::BookBuilder(int argc, char **argv) {
  int next = 1;
  while (next < argc) {
    const std::string arg(argv[next++]);
    if (arg == "-think") {
      assert(next < argc);
      think_time = std::chrono::milliseconds{std::stol(argv[next++])};
    } else if (arg == "-total") {
      assert(next < argc);
      total_time = std::chrono::seconds{std::stol(argv[next++])};
    } else if (arg == "-threads") {
      assert(next < argc);
      num_threads = std::stoi(argv[next++]);
    } else {
      log_always("Invalid argument: %s\n", arg.c_str());
      std::exit(1);
    }
  }
  assert(think_time > Duration{0});
  assert(total_time > think_time);
  assert(num_threads > 0);
}

void BookBuilder::go() {
  Timestamp deadline = current_time() + total_time;
  root = find_or_create_node(Position::initial());
  initialize(root);
  while (current_time() < deadline) {
    BookNode *const node = find_node_to_expand();
    expand(node);
    update(node);
  }
  print_book();
}

BookNode *BookBuilder::find_or_create_node(const Position &position) {
  Position normalized_position;
  position.normalize(normalized_position);
  {
    BookNode **p = lookup_table.find(normalized_position);
    if (p) return *p;
  }
  BookNode *node = new BookNode;
  node->position = normalized_position;
  node->expanded = false;
  node->expand_badness_player = 0;
  node->expand_badness_opponent = 0;
  node->initialized = false;
  ++num_nodes;
  max_move_number = std::max(max_move_number, node->position.move_number());
  bool inserted;
  lookup_table.insert(normalized_position, inserted, node);
  assert(inserted);

  return node;
}

void BookBuilder::initialize(BookNode *const node) {
  PlayerAB player;
  PlaySettings settings;
  settings.start_time = current_time();
  settings.time_left = think_time;
  settings.use_all_resources = true;
  settings.quick_if_single_move = false;
  settings.use_book = false;
  assert(!node->position.finished());
  node->best_move = player.choose_move(node->position, settings);
  node->value = player.get_last_move_milliscore();
  node->initialized = true;
}

BookNode *BookBuilder::find_node_to_expand() {
  BookNode *node = root;
  bool is_player = node->expand_badness_player < node->expand_badness_opponent;
  while (node->expanded) {
    BookNode *best_move_child = nullptr;
    for (const BookEdge &edge : node->children) {
      if (edge.move == node->best_move) {
        best_move_child = edge.destination;
        break;
      }
    }
    assert(best_move_child);
    if (is_player) {
      node = best_move_child;
    } else {
      BookNode *best_child = nullptr;
      Milliscore best_badness = 0;
      for (const BookEdge &edge : node->children) {
        BookNode *child = edge.destination;
        const Milliscore badness =
          child->expand_badness_player +
          (child->value - best_move_child->value);
        if (!best_child || badness < best_badness) {
          best_child = child;
          best_badness = badness;
        }
      }
      node = best_child;
    }
    is_player = !is_player;
  }
  return node;
}

void BookBuilder::expand(BookNode *const node) {
  assert(!node->expanded);
  Bitboard remaining_moves = node->position.valid_moves();
  while (remaining_moves) {
    const Move move = first_square(remaining_moves);
    remaining_moves = reset_bit(remaining_moves, move);
    Position next_position;
    node->position.make_move(move, next_position);
    BookNode *child = find_or_create_node(next_position);
    node->children.push_back({child, move});
    child->parents.push_back({node, move});
  }

  std::vector<std::thread> threads;
  for (int i=0;i<num_threads;++i) {
    threads.emplace_back(&BookBuilder::expand_thread, this, node);
  }

  for (auto &th : threads) {
    th.join();
  }

  node->expanded = true;
}

void BookBuilder::expand_thread(BookNode *const node) {
  for (const auto &edge : node->children) {
    BookNode *child = edge.destination;
    bool initialized;
    {
      std::lock_guard<std::mutex> lg(expand_lock);
      initialized = child->initialized;
      if (!initialized) child->initialized = true;
    }
    if (!initialized) {
      initialize(child);
    }
  }
}

void BookBuilder::update(BookNode *const start) {
  std::vector<BookNode*> all_visited;
  std::queue<BookNode*> q;
  q.push(start);
  start->visited = true;
  all_visited.push_back(start);
  while (!q.empty()) {
    BookNode *const node = q.front(); q.pop();
    update_node(node);
    for (const BookEdge &edge : node->parents) {
      BookNode *parent = edge.destination;
      if (!parent->visited) {
        q.push(parent);
        parent->visited = true;
        all_visited.push_back(parent);
      }
    }
  }

  for (BookNode *node : all_visited) {
    node->visited = false;
  }
}

void BookBuilder::update_node(BookNode *const node) {
  node->best_move = invalid_move;
  node->value = -max_milliscore;
  BookNode *best_child = nullptr;

  for (const BookEdge &edge : node->children) {
    const Milliscore value = -edge.destination->value;
    if (value > node->value) {
      node->best_move = edge.move;
      node->value = value;
      best_child = edge.destination;
    }
  }
  assert(best_child && node->best_move != invalid_move);

  node->expand_badness_player =
    best_child->expand_badness_opponent + depth_badness;

  node->expand_badness_opponent = max_milliscore;
  for (const BookEdge &edge : node->children) {
    node->expand_badness_opponent =
      std::min(node->expand_badness_opponent,
               edge.destination->expand_badness_player +
               edge.destination->value - best_child->value +
               depth_badness);
  }
  assert(node->expand_badness_opponent < max_milliscore);
}

void BookBuilder::print_book() {
  log_always("num_nodes=%zu max_move_number=%d root_value=%.6f\n",
             num_nodes,
             max_move_number,
             std::ldexp(root->value, -milliscore_bits));

  std::vector<std::string> book_entries;
  std::vector<BookNode*> all_visited;
  for (bool player : {true, false}) {
    collect_entries(root, player, book_entries, all_visited);
    for (BookNode *p : all_visited) p->visited = false;
  }
  std::sort(book_entries.begin(), book_entries.end());

  std::ofstream f("book_data.tmp");
  f << "#include \"book.h\"\n";
  f << "extern const size_t num_book_entries = " << book_entries.size()
    << ";\n";
  f << "extern const char *const book_entries[] = {\n";
  for (const std::string &entry : book_entries) {
    f << '"' << entry << "\",";
  }
  f << "};\n";
}

void BookBuilder::collect_entries(BookNode *node,
                                  bool player,
                                  std::vector<std::string> &entries,
                                  std::vector<BookNode*> &all_visited) {
  if (node->visited) return;
  node->visited = true;
  all_visited.push_back(node);
  if (player) {
    entries.push_back(encode_position(node->position) +
                      encode_square(node->best_move));
  }
  if (node->expanded) {
    for (const BookEdge &edge : node->children) {
      collect_entries(edge.destination, !player, entries, all_visited);
    }
  }
}

} // namespace

int main(int argc, char **argv) {
  verbosity = 0;
  init_hashing();
  init_evaluator();
  BookBuilder book_builder{argc, argv};
  book_builder.go();
}
