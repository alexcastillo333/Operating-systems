#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

const int MAX = 13;



static void doFib(int n, int doPrint);


/*
 * unix_error - unix-style error routine.
 */
inline static void unix_error(char *msg) {
  fprintf(stdout, "%s: %s\n", msg, strerror(errno));
  exit(1);
}


int main(int argc, char **argv) {
  int arg;
  int print = 1;

  if (argc != 2) {
    fprintf(stderr, "Usage: fib <num>\n");
    exit(-1);
  }

  arg = atoi(argv[1]);
  if (arg < 0 || arg > MAX) {
    fprintf(stderr, "number must be between 0 and %d\n", MAX);
    exit(-1);
  }

  doFib(arg, print);

  return 0;
}

/*
 * Recursively compute the specified fibonacci number n. If doPrint is
 * true, print it. Otherwise, provide it to my parent process.
 *
 * NOTE: The solution must be recursive and it must fork
 * a new child for each call. Each process should call
 * doFib() exactly once.
 */
static void doFib(int n, int doPrint) {
  // both took turns driving
  int child;
  int fib = 0;
  if (n == 0) {
    fib = 0;
  } else if (n <= 2) {
    fib = 1;
  } else {
    int nums[2];
    for (int i = 0; i < 2; i++) {
      child = fork();

      if (child < 0) {
        // handle forking errors
        exit(1);
      } else if (child == 0) {
        doFib(n - (1 + i), 0);
        return;
      } else {
        // retrieve values passed from child processes
        int status;
        int dead = waitpid(child, &status, 0);
        if (WIFEXITED(status)) {
          nums[i] = WEXITSTATUS(status);
        }
      }
    }

    fib = nums[0] + nums[1];
  }

  if (doPrint == 1) {
    printf("%d\n", fib);
  } else {
    exit(fib);
  }
}