# CS 439 Project 0: The UTCS Shell

**Please see the web-based version of this document for the most up-to-date
instructions for this project**.

## Introduction

Welcome to the first CS 439 project! Like all other projects in this course, you
will be working in C. While your previous classes have taught you the basics
of the C language, they have mostly involved small programs. In this project,
you will write a medium-sized program in C.

The program you will write in this assignment is a simple Unix shell. The shell
is the basic text interface of the Unix world, and you will use shells for most
of the remaining projects in this course (not the one you wrote!).

While building this shell, you should gain a much better understanding of how real
Unix shells work. You will also gain practice working with the process API which
you will implement in later projects.

## Overview of Shells

A _command-line interpreter_ (CLI), or a _shell_, is a program that runs other
programs on the behalf of its user. Your shell will behave much like the ones on
the CS lab machines: it will wait for input from the user, process the input,
and then take the appropriate actions.

The input provided by the user takes the form of a sequence of _commands_.
A command is a sequence of ASCII words separated by whitespace--the first
word is the name of the program to run, and the rest are arguments. Commands
are separated from each other by newlines.

Here are two example commands:

```
echo hello                        world
/bin/ls /bin/ls
```

In the first command, the program is `echo`, and the arguments are `hello` and `world`.
In the second command, the program is `/bin/ls` and the argument is `/bin/ls`.

Shells will usually execute the appropriate actions by using the fork-exec method
to execute another program. However, in some cases, a command cannot be executed
by another process. These commands, known as _built-in commands_, are instead
executed directly by the shell.

Shells also offer other convenience features. For example, the user may want to
capture the output of a program in a file for later analysis. The user may also
want to be able to run several child processes at the same time. You will
implement all of these features in this project.

At the end of this project, you will have a simple, but fully-functioning Unix shell.

## Typographical Conventions

Vocabulary/terminology that you should know will be _italicized_.

Things that need to be emphasized will be in **bold font**.

Filenames, code, and terminal output (generally, anything you might expect
to see in the terminal when working on this project) will be in `teletype`.
Usually, if it's prefaced by `unix>`, this is something you should type in your
regular shell, while things prefaced by `utcsh>` should be given as input
to your project.

🐚 The shell emoji will be used when we wish to point out a difference between
how this project works and how most real-world shells (e.g. bash, zsh) work.

## Getting Started with Your Partner

Begin by talking with your partner and agree on how you will collaborate. If you
need to work remotely, you can find some ideas in the [remote collaboration guide](https://docs.google.com/document/d/1uoBtOX_HzZj9hUFFYbmZMpB6WKA7dwYQevJvAda64bQ/edit?usp=sharing)

Set up a repository on the UTCS GitLab server in accordance with the
[Git Instructions](https://docs.google.com/document/d/1WRGVUwYcXOjTsgp0jXkfaq_9wrkjx_81BXa2t_kw2oI)

As you get started this week, please submit your answers to the questions in
our [Group Planning document](https://www.cs.utexas.edu/%7Eans/classes/cs439/projects/shell_project/shell_group_planning.txt)
this Friday evening. On the second Friday of the project, we will ask you to
submit a Group Reflections document.

## Getting Started with the Shell Project

We provide [starter code](https://www.cs.utexas.edu/%7Eans/classes/cs439/projects/shell_project/shell_project.tar.gz)
for this project. Get it from the class web page either by downloading it in your
browser or by running this command from the command line:

```
unix> wget https://www.cs.utexas.edu/%7Eans/classes/cs439/projects/shell_project/shell_project.tar.gz
```

Put the file `shell_project.tar.gz` in the directory in which you plan to do your
work. Make sure only you and your partner have access to this! Then do the following:

1. Type the command `tar xvzf shell_project.tar.gz` to expand the tar archive.
   Once the command has finished expanding the archive, you should see the
   following files:

```
  Files:

  Makefile         # Compiles your shell program and runs the tests
  README.shell     # Used for submission

  # Files for Part 0
  fib.c            # Implement fibonacci here

  # Files for Part 1/2
  argprinter.c     # A test program which can be used to debug execv issues
  util.c           # Instructor-provided utility functions
  util.h           # The header file for util.c
  utcsh.c          # Implement your shell here
  tests            # A directory of tests for your shell
  examples         # A directory with example shell scripts in it
```

1. Type the command `make` to ensure that your compiler can build the skeleton
   code.

1. Fill out the requested information in `README.shell`, where applicable.

1. Download and read over the questions in the
   [design document](https://www.cs.utexas.edu/%7Eans/classes/cs439/projects/shell_project/shell_design.txt)

1. Every time you work, log your time in the
   [Pair Programming Log](https://www.cs.utexas.edu/%7Eans/classes/cs439/projects/shell_project/pair_programming_log.txt).

**Read this entire handout and consider the overall design of your shell before
writing any code.** To help you consider your design, please look over the
questions in the [design document](https://www.cs.utexas.edu/%7Eans/classes/cs439/projects/shell_project/shell_design.txt).
If you do not do this, you may discover that you need to rewrite major portions
of your code as you progress!

## Part 0: fork()/wait()

In this phase of the project, you will learn about the `fork()` and `wait()`
system calls that you will use in the rest of the project.

#### Part 0.1: Reading

[Sections 5.4 and 5.6 of OSTEP](http://pages.cs.wisc.edu/~remzi/OSTEP/cpu-api.pdf)
may be helpful to read before starting this project. You may also wish to
consult the class resources (e.g. on C programming and shell usage) before
starting.

#### Part 0.2: Fibonacci

Update fib.c so that if invoked on the command line with some integer
argument n, where n is less than or equal to 13, it recursively computes
the nth Fibonacci number.

To ensure that you learn about `fork()`/`wait()`, there are restrictions on how
your `fib` program must be written. The full rules are below, but the general
idea is that the program will be written in a recursive manner, and each
recursive call must be made by a new process (i.e. by calling `fork()` followed
by `doFib()`). The child then returns its result to the parent, which waits
until the child is done.

Example outputs:

```
unix> fib 3
2
unix> fib 10
55
```

Your fib program must conform to the following rules:

- The output when given an argument `n` between 0 and 13 (inclusive) must be the
  `n`-th Fibonacci number.
- Fibonacci numbers must be computed recursively, with each recursive call
  occurring in a new process.
- The final result must be printed by the original process.
- You are allowed to modify the body of `doFib()`.
- You must **not** modify the number of parameters or return values of `doFib()`.
- You are allowed to create helper methods, as long as the computation of
  the Fibonacci number is done through creation of child processes.
- If given invalid arguments, your `fib` program should print the usage
  message and exit.
- Your program must not create a fork bomb when run. Any fork bomb behavior
  will result in a zero for this part of the project.
- Your output must **exactly** match the examples given above!

Note that these restrictions only apply to `fib`. Modification
of existing functions' signatures is allowed in later parts of this project.

## Part 1: Shell Skeleton

In this part of the assignment, you will start building the basic framework of
your shell. At the end of this section, you should have a basic functioning
shell framework, which you will extend and upgrade in future sections.

Your basic shell will be called `utcsh`<sup id="a1"> [1](#utcshname)</sup>.

Note: Parts 1 and 2 will walk you through a recommended implementation order for
the shell. You are not required to implement everything in this order, however,
you must implement all functionality in both parts to receive full credit.

#### Part 1.1: Reading

**Remember to read this entire document before implementing anything!**

No additional external reading is required, though if you have not worked with
command line interfaces before, you may wish to read Ubuntu's
[command line tutorial](https://ubuntu.com/tutorials/command-line-for-beginners)

You may also find it helpful to read the manpages for `strtok`, `strcmp`,
and `execv`, though this will not be required until later.

#### Part 1.2: REPL

The core of any shell is the REPL, or the _read-evaluate-print loop_. This is
a loop that does the following three actions repeatedly:

1. **Read** input from the user (or from _script_ the user specifies).
2. **Evaluate** the input, figuring out what the user wants to do and doing it.
3. **Print** any output associated with the requested action.

<!-- dprint-ignore -->
Implement a REPL in the `main()` function in `utcsh.c`. Print `utcsh> ` 
at the start of the line, then read the user's input.

For now, the only command your REPL respond to is the built-in command `exit`,
which will cause the shell to exit by calling `exit(0)`. You should also call
exit if you fail to read a line of input. For reading lines of input, you
should use `getline()`. Run `man getline` to learn more about this function.

🐚 An interesting point is that, in C, 0 is false and nonzero is true,
while in the world of UNIX exit codes, 0 indicates success and nonzero indicates
failure. This turns out to be very useful, but can be a bit hard to keep track
of when you're initially learning how to work with shells.

#### Part 1.3: Parsing and Built-in Commands

Recall from the introduction that a _command_ consists of ASCII words separated
by space. Implement some way to split a command so that you can recover these
words, e.g. you should be able to tell in O(1) time that the 4th word of
`"path a b      c d   e"` is `"d"`.

We recommend that you use `strtok()` for this. Read `man strtok()` very carefully:
this function can lead to hours of debugging if you're not careful.

Expand your shell's ability to process built-in commands by adding two new built-in commands:

- `cd`: `cd` always takes exactly one argument. `utcsh` should call the `chdir()`
  system call with the user-supplied argument.
- `path`: the `path` command takes zero or more arguments, with each argument
  separated by whitespace from the others. A typical usage might look like
  this:

  ```
  utcsh> path /bin /usr/bin
  ```

This command will be used in Part 2.1--for now, just worry about being able to
separate the arguments of this command without crashing.

#### Part 1.4: Handling Errors

In the previous section, we said that `cd` must have exactly one argument. What
happens if the user enters multiple arguments? Your shell declares that
an error has occurred.

Whenever an error occurs, your shell should print the error message on `stderr`
and continue. The only time your shell should exit in response to an error is
described in Part 1.6. <sup id="a2"> [2](#writecall)</sup>

An example snippet for how to print the error is given below. This snippet may
not meet all requirements for this project--check it carefully:

```
char emsg[30] = "An error has occurred\n";
int nbytes_written = write(STDERR_FILENO, emsg, strlen(emsg));
if(nbytes_written != strlen(emsg)){
  exit(2);  // Shouldn't really happen -- if it does, error is unrecoverable
}
```

The built-in commands you have implemented should error under the following conditions:

- `exit`: if any arguments are supplied.
- `cd`: if the number of arguments is wrong, or the `chdir()` call fails.
- `path`: never errors.

**It is never acceptable to crash, segfault, or otherwise break the shell in
response to bad user input**. Your shell must always exit gracefully, i.e. by
calling `exit()` or returning from `main()`.

🐚 Of course, most real world shells implement a huge variety of error messages
to help the user figure out where something went wrong.

#### Part 1.5: Executing External Commands

If the command given is not one of the three built-in commands, it should be
treated as the path to an external executable program.

For these external commands, execute the program using the fork-and-exec method
discussed in class. Here are some hints to help you out:

**For the child process**: The child process must execute the given command
by using the `execv()` call. **You may not call `system()` to run a
command**. Remember that if `execv()` returns, there was an error (usually
caused by incorrect arguments or the file not existing).

**For the parent process**: The parent should use `wait()` or `waitpid()` to
wait on the child. Note that the parent **does not care** about what happens
to the child. As long as `fork()` succeeds, the parent considers the process
launch to have been a success.

The `execv()` syscall is tricky to get right. Read `man execv` to learn about
some of the common pitfalls. However, if you are still confused,
here are some additional hints:

- The `const char* arg` argument of `execv()` becomes the `const char* argv[]`
  argument to `main()` in the program that was `execv()`ed.
- If there are n words in the command, `argv` should be n+1 elements long.
  Read the manpage to find out what the extra argument is.
- The `argprinter` program can be built with `make argprinter`. This program
  prints its `argv` array and then exits. If you are having trouble getting the
  arguments to `execv` right, try running `argprinter` with various arguments from
  `bash` and `utcsh` to see what the differences are.

🐚 Typical shells will collect the exit code of the child to communicate
information to the programmer. For example, the exit code of the `diff` program
can tell you not just whether two files were the same, but **how** they
differed. `utcsh` does not worry about this.

#### Part 1.6: Reading A Script

It can be tedious to have to type in commands one at a time. One common
solution for this is to create a _script_ by putting a related sequence of
commands into a file and using the shell to run that file.

Implement a _script_ system: if `utcsh` is invoked with one argument, instead
of reading commands from `stdin`, it assumes that its argument is a filename and
attempts to read commands one at a time from that file instead of from `stdin`.

You can find example scripts in the `examples/` directory. `say_hello.utcsh`
is the most basic script and consists of a bunch of external commands. There
is also the more advanced `say_hello_path.utcsh`, which relies on the path
feature (which you will implement in 2.1).

There are two other changes to utcsh when operating in script mode:

- The `utcsh>` prompt should not be printed in script mode.
- If the input file is invalid, or there is more than one argument, `utcsh`
  should print an error message and exit with an error code, i.e. call
  `exit(1)`. This is the only situation in which an error should cause `utcsh`
  to exit.

Note that until you finish this section, you will not be able to run the
automated test suite. Once you have finished this section, you may check Section
4 for details on running the tests.

🐚 To show you what a script looks like for bash, we've included two bash
scripts in the examples directory. One does the same thing as the say_hello
scripts and can be run with `bash examples/say_hello.bash`. The other can be
run with `bash examples/file_exists.bash <filename>` and will tell you whether
`<filename>` exists, and if so, if it is a regular file or a directory.

At this point, you have a basic shell that can run both built-in and external
commands, both from a script and from stdin (keyboard input)--for example,
you should be able to run the `say_hello.utcsh` script in the `examples`
directory.

Now might be a good time to make a git commit, if you haven't done so already!

## Part 2: Advanced Shell Features

#### Part 2.1: Paths

When you implemented external program execution, you assumed that the 0-th
argument was the path to an executable file. Unfortunately, this is annoying for
users, because nobody wants to type `/usr/local/bin/ls` every time they want to
run the `ls` command.

The solution to this is a `PATH`: a set of user-specified directories to search
for external programs. When the shell is given a command it does not recognize,
it looks for this program in its `PATH`.

Note that, for the rest of this document, "path" will refer to a string with
slashes in it which is used to locate a file, while `PATH` will be used to
refer to a list of paths used to search for binary files. <sup id="a3"> [3](#pathpath)</sup>

If the program you're given is not an _absolute path_, i.e. a path which starts
from `/`, you should search for your program in each directory in the `PATH`.
For example, if your `PATH` is `"/bin"` `"/usr/bin"`, you would search for
`/bin/ls` and `/usr/bin/ls`, executing the first one you found (and returning
an error if neither exists). You can check that the file exists and is
executable using the functions we provide in the skeleton code. If the file
does not exist, or it is not executable, this is an error.

The user can set the `PATH` with the `path` command. Each argument to the `path`
corresponds to an entry in the shell's `PATH`. The `path` command completely
overwrites the existing `PATH`--it does not append entries. If the PATH is empty
because the user executed a `path` command with no arguments, `utcsh` cannot
execute **any** external programs unless the full path to the program is provided.

A variable for the `PATH` is already provided for you in the skeleton code,
called `shell_paths`. You can manipulate this variable directly, or by using
the helper functions in `util.c`/`util.h`.

🐚 Real shells also let you specify _relative paths_ to programs, e.g. you can
type `bin/myprog` to run a program relative to your current working directory.
You do not need to worry about this for utcsh: the program name will either be
an absolute path or the name of a program to be searched for in `shell_paths`.

**Reminder: the shell itself does not implement `ls` or any other program--it
simply looks them up in the path and executes them.**

#### Part 2.2: Redirection

Many times, a shell user prefers to send the output of a program to a file
rather than to the screen. Usually, a shell provides this nice feature with
the `>` character. This is called _redirection_ of output.
Your shell should include this feature.

For example, if a user types `ls -al /tmp > output`, nothing should be printed
to the screen. Instead, the standard output and standard error of the program
should be rerouted to the file `output`.

If the output file already exists, you should overwrite and truncate it. Look
through the flags in `man 2 open` to find out how to do this.

Here are some rules about the redirection operator:

- Multiple redirects in a command are an error, e.g. `ls > file1 > file2`.
- A redirect without a corresponding command is an error, e.g. `> file1`.
- A redirect without a corresponding file is an error, e.g. `ls >`
- There will always be spaces around a redirect, e.g. `ls>file1` is requesting
  command execution of a file called `ls>file1`, not a redirection.
- You do not need to worry about redirection for built-in commands, e.g. we will
  not test what happens when you type `path /bin > file`.

🐚 Real shells usually allow multiple redirects and redirect `stdout` and
`stderr` separately, and allow you to redirect them to each other, e.g. you can
direct stdout into stderr.

#### Part 2.3: Concurrent Commands

Your shell will allow the user to launch concurrent commands. Remember: when two
things are _concurrent_, they appear to execute at the same time whether
they actually run simultaneously or not (_logical parallelism_). In UTCSH, this is
accomplished with the ampersand operator:

```
utcsh> cmd1 & cmd2 & cmd3 args1
```

Instead of running `cmd1`, waiting for it to finish, and then running `cmd2`,
your shell should run `cmd1`, `cmd2`, and `cmd3` (with whatever args were passed)
**before** waiting for any of them to complete.

Then, once all processes have been started, you must use `wait()` or `waitpid()`
to make sure that **all** processes have completed before moving on.

Each individual command may optionally have its own redirection, e.g.

```
utcsh> cmd1 > file1 & cmd2 arg1 arg2 > file2 & cmd3 > file3
```

Note that we can now have multiple commands on a single line. For obvious reasons,
we shall hereafter refer to a line of input as a _command line_. Each _command
line_ may have one or more _commands_ in it.

Unlike the redirection operator, the ampersand operator might not have spaces
around it. For example `cmd1 arg1&cmd2 > file2` is a valid command line, and
requests the execution of two commands. In addition, some or all of the commands
on either side of the ampersand may be blank. This means that, for example,
`&&&&&&&` is a valid command line.

As you process these commands, there are a number of special cases to consider.
In doing so, you may assume the following:

- If a command line has multiple concurrent commands that are all external, the current spec applies.
- If a command line has multiple concurrent commands that are all built-in, the shell should execute them sequentially from left-to-right.
- You may assume that we will not test command lines that have mixed concurrent external/internal built-in commands. Your shell should not crash if this happens, but otherwise, there are no requirements on what it must do.

🐚 In most bash-like shells, `&` is actually appended to the end of a command
to instruct it to _run in the background_. You can search for "Bash Job Control"
if you want to learn more, but don't try to use this syntax in your actual
shell to run jobs in parallel, or weird things might happen!

## Part 3: Hints

### General Hints

- Remember that C does not have strings in the same way that Java does--you will
  need to be careful about string handling. You can read more about string
  handling in [these notes](http://www.eskimo.com/~scs/cclass/notes/sx8.html).
  For a refresher on general C principles, please see our [guide to Cbasics](https://docs.google.com/document/d/1c3PF8fE7jBFzrBx-HqvkvlPDqWh6u99bdpmCya3RwNU/edit?usp=sharing)
- Always, **always** check the return codes of all system calls, from the very
  beginning of your work. This will often catch mistakes in how you're using
  these functions.
- USE GIT. Make a commit every time you have working code, or have implemented
  a small part of a milestone, not just when you need to get the current version
  to your partner. Committing more frequently lets you try things out much more
  easily, since it's easy to revert changes if you screw things up.
- For more information on using git, check our our [guide to version controland git](https://docs.google.com/document/d/1WRGVUwYcXOjTsgp0jXkfaq_9wrkjx_81BXa2t_kw2oI/edit?usp=sharing). In this class, we'll be using the UTCS GitLab server. For help
  getting started, check out our [guide to getting started with UTCS
  GitLab](https://utexas.hosted.panopto.com/Panopto/Pages/Viewer.aspx?id=26f07913-3137-497f-a4db-ae29003dabfe).
- You are allowed to modify _any_ file you want in this project. If you wish, you
  can delete all the skeleton code and start from scratch. Note that any changes
  you make in the `tests` directory will be reverted before we grade.
- Errors can be debugged with `printf` and `gdb`. For general debugging help, check out our
  [debugging FAQ for CS 439](https://docs.google.com/document/d/1SzvPr9WHrxH7FEc2FHno5OPSKBnHzVP4nFpyYWZECqY/edit?usp=sharing).

### Hints for Part 0

- The `waitpid()`, `fork()`, and `exit()` functions will come in handy. Use
  `man` to learn about them. Remember, you can use `man man` to learn about `man`.
- The `WEXITSTATUS` macro described in the `waitpid` manpage may be useful.

### Hints for Part 1

- A real concern in any text processing program in C how much memory to allocate
  for text handling. In order to simplify your shell implementation, you are
  allowed to limit the size of your inputs according to the macros defined in
  `util.h`. When interpreting these limits, remember that a command line may
  consist of multiple commands.

  If the input violates these limits, you may print the error message and
  continue processing. **Do not crash the shell** if these limits are violated.

  It is possible to write the shell in a way that these limits are not needed,
  but it is slightly more challenging.

- Think **carefully** about how you design your tokenization routines. Right
  now, you only have to deal with one command. In Part 2, you're going to deal
  with multiple commands, possibly each with their own redirects, and each of
  which can error independently of the others. Make sure your design can grow
  to accommodate this.

  A good basic design is to allocate an array of `char*`, then use `strtok` to
  fill it up one element at a time. At the end of this procedure, `array[0]`
  should be the 0-th argument, `array[1]` should be the 1st, and so on.
  You should then store this information in a way that allows multiple copies
  (i.e. _not_ in a global structure, which tends to be a bad idea anyways).

- Be **extremely careful** about doing `a == b` or `a = b` when `a` and `b` are
  `char*`. This likely does not do what you think it does. In order to do the
  operations, look into `strcmp()`, `strcpy()`, and `strncpy()` in `string.h`.

### Hints for Part 2

- Output redirection can be achieved by using a combination of `open()` and
  `dup2()`. Check the man pages for more details. You should make these calls
  after the child process has forked, but before the call to `execv()`. Check
  https://www.cs.utexas.edu/~theksong/2020/243/Using-dup2-to-redirect-output/
  for an example of `dup2()` usage.
- If you've been using the recommended functions so far, you might need to add
  a new function or data structure to make concurrent commands work easily.
- Don't be afraid to modify the skeleton code. We are **not** checking that your
  skeleton code is unmodified, we're checking that your program works and is
  well-written. If you have to modify, add to, or remove from the skeleton to
  achieve this, do it!

### Line Count Hints

We are providing the rough number of lines of code used in the reference
solution as a rough hint for you, so you can see how much work is needed for
each function. **These numbers have been rounded to the nearest multiple of 10.**

| Function                | Lines of Code |
| ----------------------- | ------------- |
| `tokenize_command_line` | 50 lines      |
| `parse_command`         | 60 lines      |
| `eval`                  | 60 lines      |
| `try_exec_builtin`      | 60 lines      |
| `exec_external_cmd`     | 30 lines      |
| `main`                  | 50 lines      |

## Part 4: Checking Your Work

To help you check your work, we've provided a small test suite, along with some
tools to help you run it.

Each test in the test suite will check three things from your shell:

- The exit code
- The standard output
- The standard error

If any of these differ from the correct values, the test suite will print an
error and tell you what part of the output was wrong, along with commands you
can run to see the difference.

In order to make this easier on you, we've included some helper rules in the
Makefile to let you run tests easily.

- To run the full testsuite, run `make check`.
- To run an individual test, run `make testcase id=#`, e.g. `make testcase id=15`.
- To get a description of a test, run `make describe id=#`. This can be useful
  if you're not sure what a test does or want to get commands to run it yourself
  (e.g. to run it under a debugger)

No test should run for more than 10 seconds without either passing or failing.
If your test runs for longer than this, you likely have an infinite loop in your
code.

In general, you should not look directly at the test files themselves unless you
want to modify the tests. If you want to run the command that the test runs, use
`make describe`.

## Part 5: On Programming and Logistics

### Makefile

Your code will be tested and graded with the output of `make utcsh` or `make`
(the two rules are equivalent in the provided Makefile). To aid you in
debugging, two additional rules have been created in the makefile:

- `make debug` will create a binary which is not heavily-optimized and has more
  debugging information than the default build. If you want to feed your program
  into a debugger like `gdb`, `valgrind`, or `rr`, you should use this rule to
  generate it.
- `make asan` will create a binary with _sanitizers_. Think of these as extra
  error checking code that the compiler adds to the program for you. When you
  run a program that has been compiled with sanitizers, the binary itself will
  warn you about memory leaks, invalid pointer dereferences, and other such
  issues.

  We do not enable the sanitizers by default because they can turn an
  otherwise-correct program into an incorrect one, e.g. if your program is
  correct except for a small memory leak, the sanitized binary will still exit
  with an error.

You may use these rules to quickly generate programs for debugging, but keep in
mind that your grade will be based on the binary generated by `make utcsh`.

### Design Document

As part of this project, you will submit a [design document](https://www.cs.utexas.edu/%7Eans/classes/cs439/projects/shell_project/shell_design.txt),
where you will describe your design to us. Please note that this document is a
set of questions that you will answer and is not free form. Your group will
submit one design document.

### General

1. You must work in two-person teams on this project. Failure to do so will result
   in a 0 for the project. Once you have contacted your assigned partner, do
   the following:

   1. exchange first and last names, EIDs, and CS logins
   2. fill out the README.shell distributed with the project
   3. register in Canvas as a Shell Group. (Add yourselves to an empty group
      of your choosing. Feel free to change the name to something more creative!
      Keep it clean.)
   4. Create a private GitLab repo and invite your partner at at least "maintainer"
      level. See our [git and version control guide](https://docs.google.com/document/d/1WRGVUwYcXOjTsgp0jXkfaq_9wrkjx_81BXa2t_kw2oI/edit#heading=h.3fguf535xdp)
      for details on how to do this.

   You must follow the [pair programming guidelines](https://www.cs.utexas.edu/%7Eans/classes/cs439/projects/pairprogramming.html)
   set forth for this class. If you have not registered a group by the group
   registration deadline, we will assign you to a group.

   **Please see the [Grading Criteria](https://www.cs.utexas.edu/%7Eans/classes/cs439/projects/GradingCriteria.html)
   to understand how failure to follow the pair programming guidelines OR fill
   out the README.shell will affect your grade.**

2. You must follow the guidelines laid out in the [C Style Guide](https://www.cs.utexas.edu/%7Eans/classes/cs439/projects/CStyleGuide.html)
   or you will lose points. This includes selecting reasonable names for your
   files and variables.

3. This project will be graded on the UTCS public linux machines. Although
   you are welcome to do testing and development on any platform you like, we
   cannot assist you in setting up other environments, and you must test and do
   final debugging on the UTCS public linux machines. The statement "It worked
   on my machine" will not be considered in the grading process.

4. The execution of your solution shell will be evaluated using the test
   cases that are included in your project directory. To receive credit for the
   test cases, your shell should pass the provided test case, as determined by
   `make clean && make utcsh && make check`.

5. Your code **must** compile without any additions or adjustments, or you
   will receive a 0 for the test cases portion of your grade.

6. Do not use `_exit()` for this assignment--use `exit()` instead.

7. You are encouraged to not use linux.cs.utexas.edu for development.
   Instead, please find another option using the department's [list of public UNIX hosts](https://apps.cs.utexas.edu/unixlabstatus/).

8. You are encouraged to reuse **your own** code that you might have developed
   in previous courses to handle things such as queues, sorting, etc. You are
   also encouraged to use code provided by a public library such as the GNU
   library.

9. You may not look at the written work of any student other than your
   partner. This includes, for example, looking at another student's screen to
   help them debug or looking at another student's print-out. See the syllabus
   for additional details.

10. If you find that the problem is under specified, please make reasonable
    assumptions and document them in the README.shell file. Any clarifications or
    revisions to the assignment will be posted to EdStem.

### Submitting Your Work

1. After you finish your code, use `make turnin` to submit a compressed tarball
   named `turnin.tar.gz` for submission. It may be a good idea to unpack
   this tarball into a clean directory on a UTCS linux system to make sure it
   still compiles. You should then upload the file to the Project 0 Test Cases
   assignment on Canvas. Make sure you have included the necessary information
   in the README.shell and placed your pair programming log in the project
   directory.

2. Once you have completed your design document, please submit it to the
   Project 0 Design and Documentation assignment in Canvas. Make sure you have
   included your name, CS login, and UT EID in the design document.

   The purpose of the design document is to explain and defend your design to
   us. Its grade will reflect both your answers to the questions and the
   correctness and completeness of the implementation of your design. It is
   possible to receive partial credit for speculating on the design of portions
   you do not implement, but your grade will be reduced due to the lack of
   implementation.

### Grading

Code will be evaluated based on its correctness, clarity, and elegance
according to the [Grading Criteria](https://www.cs.utexas.edu/%7Eans/classes/cs439/projects/GradingCriteria.html).
Strive for simplicity. Think before you code.

The most important factor in grading your code design and documentation will
be code inspection and evaluation of the descriptions in the write-ups.
Remember, if your code does not follow the standards, it is wrong. If your
code is not clear and easy to understand, it is wrong.

## Footnotes

Project adapted from one used in OSTEP. Many thanks to the Drs. Arpaci-Dusseau
for permission to use their work.

<b id="utcshname">[1]:</b> This is both an homage to the UTCS department and a
play on the name of the popular [`tcsh` shell](https://en.wikipedia.org/wiki/Tcsh). [↩](#a1)

<b id="writecall">[2]:</b> Note that we check the return value of the write call
in spite of the fact that all we can do if it's wrong is exit. This is good
programming practice, and you should be sure to always check the return codes
of any system or library call that you make. [↩](#a2)

<b id="pathpath">[3]:</b> Sometimes you hear `PATH` referred to as "**the**
path," but in most real-world contexts, you will need to deduce which one
is meant from context. [↩](#a3)
