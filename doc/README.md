# Examples

The header for the NoShell library is called `noshell.hpp` and the
line to include it is not repeated in the following examples.

Although many of the examples show an equivalent shell code, in no
case does the NoShell library invokes a shell to evaluate the
commands. NoShell uses directly `fork()` and `exec()`.

# Simple commands and redirections

## Running a command

To create a command, add `_C` after a string (this is a user defined
literals. See [No Literals](#No Literals) for an alternative) and pass any
arguments in parenthesis, like a function call. For example:

```cpp
noshell::Exit e = "git"_C("init");
```

is equivalent to running `git init`.

## Input/Output redirections from files

For redirections, use the operators `<`, `>` and `>>` with similar
meaning that in the shell. If the right hand side of the operator is a
string (`char*` or `std::string`), the the input/output is redirected from
that file. For example:

```cpp
noshell::Exit e = "date"_C() > "now";
```

will write the output of the command `date` to the file `now`, similar
to `date > now`. Similarly

```cpp
noshell::Exit e = "ls"_C("-l") >> "files;
```

appends the content of the current directory to `files`', just like
`ls -l >> files`.

Multiple redirections can be specified at once. The syntax `2_R(path)`
can be used to redirect file descriptor 2 (stderr) rather than the
default stdout. For example:

```cpp
noshell::Exit e = "cat"_C("-n") < "input_file" > "output_file" >> 2_R("log");
```

This will copy `input_file` into `output_file`, with line numbers
(switch `-n` of `cat`) and append any errors to `log`. This can be
used to redirect stdout and stderr to the same file:

```cpp
noshell::Exit e = "big_command"_C() > "output_file" > 2_R(1);
```

which is similar to `big_command > output_file 2>&1`.

## Input/Output redirections from descriptors/streams

Similarly, the operator `<`, `>` and `>>` accept an open file
descriptor (an `int`) or stdio stream (a `FILE*`). For example:

```cpp
int fd          = open("inputfile", O_RDONLY);
FILE* st        = fopen("outputfile", "w");
noshell::Exit e = "grep"_C("-v", "^\\s") < fd > st;
fclose(st);
close(fd);
```

will filter out all lines of `inputfile` that start with a space and
write the result to `outputfile`.

# Pipelines

Similarly to the shell, NoShell can run multiple commands in a
pipeline, where the standard output of one feeds into the standard
input of the next, using the operator `|`.

For example,

```cpp
noshell::Exit e = "find"_C(".", "-type", "d") | "grep"_C("^\\d") | "xargs"_C("rm", "-rf");
```
will happily erase every directory whose name starts with a digit.

## Operator precedence

The operators `<`, `>` and `>>` have a higher precedence than the `|`
operator. The following pipeline:

```cpp
noshell::Exit e = "some_big_command"_C() > 2_R("log") | "grep"_C("^>") > "headers";
```

works as expected. It records the `stderr` of the big command
into the file `log`, pipe the `stdout` to the grep and record the
output of the `grep` in the file `headers`.

## Popen equivalent

Similarly to the `popen()` function, one can get a pipe opened to
the input or output of a command. This works with file descriptors,
stdio streams and C++ streams (see warning below).

```cpp
// File descriptors
int fd;
noshell::Exit e = "grep"_C("-c", "^processor") < "/proc/cpuinfo" | fd;

// Stdio streams
FILE* f;
noshell::Exit e = "grep"_C("-c", "^processor") < "/proc/cpuinfo" | f;

// C++ streams
noshell::istream is;
noshell::Exit e = "grep"_C("-c", "^processor") < "/proc/cpuinfo" | is;
```

Now one can read from `fd`, `f` or `is` to get the number of CPUs on
the system.

One can make Multiple piping redirections at once. For example, to
redirect all standard input and output:

```cpp
noshell::istream out, err;
noshell::ostream in;
noshell::Exit e = in | "big_command"_C() | out | 2_R(err);
```

Beware of buffering when multiple pipes are open this way and use
`select()` (or equivalent).

> C++ streams work with the GNU libstdc++, not with LLVM libc++, as
> libc++ does not support the GNU extension `stdio_filebuf`. Also, the
> types `noshell::istream` and `noshell::ostream` inherit from
> `std::istream` and `std::ostream`. They are similar to stdio streams
> in that they can be constructed from open file descriptors (similar
> to `fdopen` for stdio) and the underlying file descriptor is
> obtained with the `fd()` method (similar to `fileno` for stdio).


## Autorun

In all the previous examples, the commands are run automatically. By
assigning the command to an object of type `noshell::Exit`, the
command is run automatically (the all `fork()/exec()/wait()` cycle). In fact, the
following:

```cpp
noshell::Exit e = "some_command"_C();
```

is equivalent to:

```cpp
noshell::PipeLine p = "some_command"_C();
noshell::Exit e     = p.run_wait();
```

When some input/output is piped to the current process, then one must
read/write to the pipes until the sub-process is done, before waiting
for it. Otherwise one gets to a dead lock (sub-process is stopped on
I/O and the parent is stopped waiting for the child). NoShell knows
about this situation and does not wait for such a sub-process
automatically. It is then the responsibility of the user to explicitly
wait on the sub-process.

For example:

```cpp
noshell::ostream os;
noshell::Exit e = os | "grep"_C("^1") > "result";
// Feed some data to the grep process
os << 1 << '\n' << 2 << '\n' << 15 '\n' << 31;
// Close pipe so grep stop processing
os.close();
// Wait on grep to be done
e.wait();
```

In that case, assigning to the `noshell::Exit` class calls the `run()`
method instead of `run_wait()` method. That, the first two lines are
equivalent to:

```cpp
noshell::ostream os;
noshell::PipeLine p = os | "grep"_C("^1") > "result";
noshell::Exit e     = p.run();
```

## Reusing pipelines

Pipelines need not be run immediately. They can be saved and run later or multiple times:

```cpp
noshell::PipeLine p = "some_command"_C();
// Run it once
noshell::Exit e1    = p.run_wait();
// Run it twice, hoping for a different result?
noshell::Exit e2    = p.run_wait();
```

Similarly, the `run` and `wait` need not be combined:

```cpp
noshell::PipeLine p = "some_command"_C();
noshell::Exit e     = p.run();
e.wait();
```

## Success or not?

An object `e` of type `noshell::Exit` contains all the information
about the running command. Then `e.success()` returns `true` if all the
commands in the pipeline where started without errors and return an
exit status of 0.

Note that this is different than a normal shell behavior which uses
the exit status of the last command as the exit status of the
pipeline. Meaning that NoShell considers that a pipeline ran
successfully if ALL its commands succeeded, while the shell considers
that a pipeline ran successfully if the LAST command succeeded.

The status of each command in the pipeline can be examined. The
following example display an exhaustive status about every command in
a pipeline and the reason why it failed:

```cpp
for(const auto& h : e) {
  std::cout << "Command " << e.id(h) << ": ";
  if(h.have_status()) {
    if(h.status().exited())
      std::cout << "exited normally with status " << h.status().exit_status() << '\n';
    if(h.status.signaled())
      std::cout << "killed by signal " << h.status().term_sig() << "(" << h.status().signal() << ")\n";
  } else if(h.setup_error()) {
    std::cout << "failed to run: " << h.what() << '\n';
  } else {
    std::cout << "was not run\n";
  }
}
```

The `e.id(h)` methods returns the index of the command in the pipeline
(0-based).

The method `h.setup_error()` returns true if an error occurred during
the setup of the command (an error in `fork()`, while setting up the
redirections or during `exec()`). If `h.setup_error()` is false, then
NoShell did not encountered any error and started the command
successfully.

The method `h.have_status()` returns true if the command has run and
has been waited for. The raw status (as returned by `wait()`) is
obtained from `h.status().value` or can be examined like above with
convenience methods.

Instead of looping over all the commands, one can loop over the failed
commands only like so:

```cpp
for(const auto& h : e.failures()) {
  ...
}
```

## No literals

All the examples in this page make use of the user defined literal
`_C` and `_R` for command and redirection. If one does not fancy this
newfangled C++11 feature, it is possible to avoid them:

```cpp
#define NOSHELL_NO_LITERALS
#include <noshell.hpp>

// Equivalent to "grep"_C("-c", "/proc/cpuinfo")
noshell::Exit e = noshell::C("grep", "-c", "/proc/cpuinfo") > "ncpus";

// Equivalent to 2_R("file")
noshell::Exit e = noshell::C("big_command") > noshell::R(2).to("file");
```
