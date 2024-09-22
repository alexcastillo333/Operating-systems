/*
  utcsh - The UTCS Shell

  Names: Nicholas Hoang, Alex Castillo
  EIDs: nhh355, alc5938
  CS Logins: nhh355, acastill
  Emails: nicholash489@gmail.com, alc5938@utexas.edu
  Unique: 52795
*/

/* Read the additional functions from util.h. They may be beneficial to you
in the future */
#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>

/* Global variables */
/* The array for holding shell paths. Can be edited by the functions in util.c*/
char shell_paths[MAX_ENTRIES_IN_SHELLPATH][MAX_CHARS_PER_CMDLINE];
static char prompt[] = "utcsh> "; /* Command line prompt */
static char *default_shell_path[2] = {"/bin", NULL};
int debug = 0;
/* End Global Variables */

/* Convenience struct for describing a command. Modify this struct as you see
 * fit--add extra members to help you write your code. */
struct Command {
  char **args;      /* Argument array for the command */
  int size;
  char *outputFile; /* Redirect target for file (NULL means no redirect) */
  struct Command *next; /* points to next command (NULL if no next command) */
};

/* Here are the functions we recommend you implement */

void check_file_empty(FILE *f);
struct Command* make_command_list(char *line);
char **tokenize_command_line(char *cmdline, int *size);
struct Command* parse_command(char **tokens, int size);
void eval(struct Command *cmd);
int try_exec_builtin(struct Command *cmd);
void exec_external_cmd(struct Command *cmd);
void open_file(char *file);
void write_error();

/* Main REPL: read, evaluate, and print. This function should remain relatively
   short: if it grows beyond 60 lines, you're doing too much in main() and
   should try to move some of that work into other functions. */
int main(int argc, char **argv) {
  set_shell_path(default_shell_path);

  // Nicholas drove here
  FILE * f = NULL;
  if (argc > 2) {
    write_error();
    exit(EXIT_FAILURE);
  } else if (argc == 2) {
    char * file = argv[1];
    f = fopen(file, "r");

    // check if file exist
    if (f == NULL) {
      write_error();
      exit(EXIT_FAILURE);
    }

    //check if file is empty
    check_file_empty(f);
  }
  // End of Nicholas Drive

  while (1) {
    /* Read */
    // Alex drove here
    size_t buffer = MAX_CHARS_PER_CMDLINE;
    char * line = (char *)malloc(sizeof(char) * buffer); 
    ssize_t cmdSize;
    // check if reading from a file
    if (f != NULL) {
      cmdSize = getline(&line, &buffer, f);
      if (cmdSize == -1) {
        fclose(f);
        free(line);
        exit(EXIT_SUCCESS);
      }
    } else {
      // prompt user
      printf("%s", prompt);
      cmdSize = getline(&line, &buffer, stdin);
    }
    // End of Alex drive

    // make command struct
    struct Command *cmd = make_command_list(line);

    // Start of Nicholas drive
    /* Evaluate */
    // check if there is a command to execute
    if (cmd) {
      eval(cmd);
    }

    /* Print (optional) */
    if (debug) {
      printf("You typed: %s\n", line);
    }
    free(line);
    // End of Nicholas drive
  }  

  return 0;
}

// Check if given file, f, is empty.
// Exit with error if file is empty.
void check_file_empty(FILE *f) {
  // start of Nicholas drive
  size_t buffer = MAX_CHARS_PER_CMDLINE;
  char *line = malloc(sizeof(char) * buffer);
  ssize_t size = getline(&line, &buffer, f);
  free(line);
  if (size == -1) {
    write_error();
    exit(EXIT_FAILURE);
  }

  // return to beginning of file
  fseek(f, 0, SEEK_SET);
  // End of Nicholas drive
}

// create Command struct(s) from given line. If the line has concurrent
// commands, they will be linked together
struct Command *make_command_list(char *line) {
  // Nicholas drove here
  const char *delim = "&";
  char *copy = malloc(strlen(line) + 1);
  strcpy(copy, line);
  char *saveptr;
  char *next = strtok_r(copy, delim, &saveptr);
  struct Command *cmd = NULL;
  struct Command *cur = NULL;
  // if concurrent commands, tokenize and parse each one seperately, and
  // link them together
  while (next) {
    int size = 0;
    char **tokens = tokenize_command_line(next, &size);
    struct Command *temp = parse_command(tokens, size);
    // connect next command to end of list.
    if (temp) {
      if (!cmd) {
        cmd = temp;
        cur = cmd;
      } else {
        cur->next = temp;
        cur = cur->next;
      }
    }
    next = strtok_r(NULL, delim, &saveptr);
  }
  return cmd;
  // End of Nicholas Drive
}

/* NOTE: In the skeleton code, all function bodies below this line are dummy
implementations made to avoid warnings. You should delete them and replace them
with your own implementation. */

/** Turn a command line into tokens with strtok
 *
 * This function turns a command line into an array of arguments, making it
 * much easier to process. First, you should figure out how many arguments you
 * have, then allocate a char** of sufficient size and fill it using strtok()
 */
char **tokenize_command_line(char *cmdline, int *size) {
  // Alex drove here
  // count how many tokens there are
  int count = 0;
  const char * delim = " \n\t";
  char *copy = malloc(strlen(cmdline) + 1);
  strcpy(copy, cmdline);
  char * token = strtok(copy, delim);
  while (token) {
    count++;
    token = strtok(NULL, delim);
  }

  // begin filling tokens array
  char **tokens = malloc(sizeof(char *) * count);
  strcpy(copy, cmdline);
  token = strtok(copy, delim);
  for (int i = 0; i < count; i++) {
    tokens[i] = malloc(strlen(token) + 1);
    strncpy(tokens[i], token, strlen(token) + 1);
    token = strtok(NULL, delim);
  }
  *size = count;
  return tokens;
  // End of Alex drive
}

/** Turn tokens into a command.
 *
 * The `struct Command` represents a command to execute. This is the preferred
 * format for storing information about a command, though you are free to change
 * it. This function takes a sequence of tokens and turns them into a struct
 * Command.
 */
struct Command* parse_command(char **tokens, int size) {
  // Nicholas drove here
  if (size == 0) {
    return NULL;
  }
  // End of Nicholas drive
  // Start of Alex drive
  struct Command *cmd = malloc(sizeof(struct Command));
  // check for redirect file and update cmd struct to reflect whether or not
  // it exists
  if (size > 1 && strcmp(tokens[size - 2], ">") == 0) {
    int arraySize = sizeof(char *) * (size - 2);
    char **newArgs = malloc(arraySize);
    memcpy(newArgs, tokens, arraySize);
    cmd->args = newArgs;
    cmd->outputFile = tokens[size - 1];
    cmd->size = size - 2;
  } else {
    cmd->args = tokens;
    cmd->outputFile = NULL;
    cmd->size = size;
  }
  
  return cmd;
  // End of Alex drive
}

/** Evaluate a single command
 *
 * Both built-ins and external commands can be passed to this function--it
 * should work out what the correct type is and take the appropriate action.
 */
void eval(struct Command *cmd) {
  // Start of Nicholas drive
  // check if there is an incorrect redirect request in cmd.
  if (cmd->size) {
    struct Command * temp = cmd;
    while(temp) {
      for (int i = 0; i < temp->size; i++) {
          if (strcmp(">", temp->args[i]) == 0) {
            write_error();
            return;
          }
      }
      temp = temp->next;
    }

    // look through built-ins, execute an external command if not built-in
    if (!try_exec_builtin(cmd)) {
      exec_external_cmd(cmd);
    }
  } else if (cmd->size == 0 && cmd->outputFile) {
    write_error();
  }

  return;
  // End of drive
}

/** Execute built-in commands
 *
 * If the command is a built-in command, execute it and return 1 if appropriate
 * If the command is not a built-in command, do nothing and return 0
 */

 // executes built in command(s) exit, path, or cd. 
 // if concurrent commands, execute them sequentially. Also checks if built in
 // commands have the correct number of  arguments. returns 1 on success
 // (and if exit isnt called). returns 0 if commands are not built in commands. 
int try_exec_builtin(struct Command *cmd) {
  // Start of Alex drive
  while (cmd) {
    if (strcmp(cmd->args[0], "exit") == 0) {
      // check if exit was given any args
      if (cmd->size > 1) {
        write_error();
      } else {
        exit(0);
      }
    } else if (strcmp(cmd->args[0], "path") == 0) {
      // create new array of paths to set.
      char* arr[cmd->size];
      for (int i = 0; i < cmd->size - 1; i++) {
        arr[i] = cmd->args[i + 1];
      }
      arr[cmd->size - 1] = NULL;
      // set array in shell_paths
      set_shell_path(arr);
      
    } else if (strcmp(cmd->args[0], "cd") == 0) {
      int success = 0;
      // check if too many args was given
      if (cmd->size != 2) {
        write_error();
      } else {
        success = chdir(cmd->args[1]);
        // make sure directory was entered correctly
        if (success == -1) {
          write_error();
        }
      }
    } else {
      return 0;
    }
    cmd = cmd->next;
  }
  
  return 1;
  // End of Alex drive
}

/** Execute an external command
 *
 * Execute an external command by fork-and-exec. Should also take care of
 * output redirection, if any is requested
 */
void exec_external_cmd(struct Command *cmd) {
  // Nicholas started driving here
  int children[MAX_WORDS_PER_CMDLINE / MAX_WORDS_PER_CMD];
  int child = 0;
  int size = 0;
  // fork for each concurrent command
  while (cmd) {
    child = fork();
    if (child < 0) {
      write_error();
      exit(EXIT_FAILURE);
    } else if (child == 0) {
      break;
    } else {
      //save child pid in array and move to next cmd
      children[size] = child;
      cmd = cmd->next;
      size++;
    }
  }

  if (child < 0) {
    write_error();
    exit(EXIT_FAILURE);
  } else if (child == 0) {
    open_file(cmd->outputFile);
    // create array of args, and terminate with null
    char *args[cmd->size + 1];
    for (int i = 0; i < cmd->size; i++) {
      args[i] = cmd->args[i];
    }
    args[cmd->size] = NULL;
    
    //create full file path to pass into exec
    char *filedir = NULL;
    if (is_absolute_path(args[0])) {
      filedir = args[0];
    } else {
      // find first instance of executable
      for (int i = 0; i < MAX_ENTRIES_IN_SHELLPATH && filedir == NULL; i++) {
        filedir = exe_exists_in_dir(shell_paths[i], args[0], false);
      }
    }
    if (filedir == NULL) {
      write_error();
      exit(1);
    }
    int status = execv(filedir, args);
    // failed to execute, exit child
    write_error();
    exit(status);
  } else {
    int status;
    // wait for all children to exit
    for (int i = 0; i < size; i++) {
      waitpid(children[i], &status, 0);
    }
    
  }
  return;
  // End of Nicholas drive
}


// opens an output file, file, for a command to redirect its output to.
// do nothing if the output file is NULL
void open_file(char *file) {
  // Nicholas drove here
  int fd = 0;
  if (file) {
    // open file and truncate, or create a new file
    fd = open(file, O_WRONLY | O_CREAT | O_TRUNC, 0666);
  }
  if (fd < 0) {
    write_error();
    close(fd);
    exit(EXIT_FAILURE);
  } else if (fd != 0 && (dup2(fd, STDOUT_FILENO) == -1 || 
              dup2(fd, STDERR_FILENO) == -1)) {
    // check if file descriptors were set correctly
    write_error();
    close(fd);
    exit(EXIT_FAILURE);
  } else {
    close(fd);
  }
  // end of Nicholas drive
}


// writes an error to stderr.
void write_error() {
  // code copied from assignment notes
  char emsg[30] = "An error has occurred\n";
  int nbytes_written = write(STDERR_FILENO, emsg, strlen(emsg));
  if(nbytes_written != strlen(emsg)){
    exit(2);  // Shouldn't really happen -- if it does, error is unrecoverable
  }
}