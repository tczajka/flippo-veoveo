#include "player_external.h"
#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

PlayerExternal::PlayerExternal(const std::string &path,
                               const std::string &log_path,
                               const Position &initial_position,
                               const Duration game_time) {
  int commanding_pipe[2];
  int moves_pipe[2];

  check_error(pipe2(commanding_pipe, O_CLOEXEC));
  check_error(pipe2(moves_pipe, O_CLOEXEC));
  child_pid = fork();
  check_error(child_pid);
  if (child_pid == 0) {
    // Child.

    // FD 0: read commanding.
    check_error(close(0));
    check_error(dup2(commanding_pipe[0], 0));

    // FD 1: write moves.
    check_error(close(1));
    check_error(dup2(moves_pipe[1], 1));

    // FD 2: log.
    const int log_fd = creat(log_path.c_str(), S_IRUSR | S_IWUSR);
    check_error(log_fd);
    check_error(close(2));
    check_error(dup2(log_fd, 2));
    check_error(close(log_fd));

    // Execute child.
    check_error(execl(path.c_str(),
                      path.c_str(),
                      static_cast<char*>(nullptr)));
  }
  // Parent.
  check_error(close(commanding_pipe[0]));
  check_error(close(moves_pipe[1]));

  commanding_file = fdopen(commanding_pipe[1], "w");
  if (!commanding_file) {
    perror("commanding_file");
    std::exit(1);
  }

  moves_file = fdopen(moves_pipe[0], "r");
  if (!moves_file) {
    perror("moves_file");
    std::exit(1);
  }

  fprintf(commanding_file,
          "Position %s\nTime %d\n",
          initial_position.to_string().c_str(),
          static_cast<int>(
            std::chrono::duration_cast<std::chrono::milliseconds>(
              game_time).count()));
}

PlayerExternal::~PlayerExternal() {
  fprintf(commanding_file, "Quit\n");
  fflush(commanding_file);
  check_error(waitpid(child_pid, nullptr, 0));
  check_error(fclose(commanding_file));
  check_error(fclose(moves_file));
}

Move PlayerExternal::choose_move(const Position &,
                                 const PlaySettings &) {
  const std::string opp_move_str =
    last_opponent_move == invalid_move ?
    std::string("Start") :
    move_to_string(last_opponent_move);

  fprintf(commanding_file, "%s\n", opp_move_str.c_str());
  fflush(commanding_file);
  char buf[100];
  if (!fgets(buf, sizeof(buf), moves_file)) {
    fprintf(stderr, "fgets error");
    std::exit(1);
  }
  if (buf[2]=='\n') buf[2]=0;
  const char my_move = string_to_move(buf);
  if (my_move == invalid_move) {
    fprintf(stderr, "invalid move");
    std::exit(1);
  }
  return my_move;
}

void PlayerExternal::opponent_move(const Position &, const Move move) {
  last_opponent_move = move;
}


void PlayerExternal::check_error(const int ret) {
  if (ret == -1) {
    perror("PlayerExternal");
    std::exit(1);
  }
}
