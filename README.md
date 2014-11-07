# NoShell library

## Overview

The NoShell C++11 library allows to easily starts sub-process with a
syntax similar to the shell while not using the shell. It like using
the "`system()`", "`popen()`", or "`spawn()`" calls, but easier to use
and features similar to the shell.

For example, the following code will run the "`grep`" command and
setup the stream `is` to read from its standard out.

```cpp
noshell::istream is;
noshell::Exit e = "grep"_C("-c", "/proc/cpuinfo") | is;

int ncpu;
is >> ncpu;
```

Now `ncpu` contains the number of CPUs on your Linux machine. NoShell
allows to create pipelines of commands as well, and do redirections
(with '<', '>', '>>'), like in the shell. For example, the following shell code:

```sh
grep < /proc/cpuinfo | wc -l > ncpus
```

would be done with NoShell like this:

```cpp
noshell::Exit e = "grep"_C() < "/proc/cpuinfo" | "wc"_C("-l") > "ncpus";
```

Note that in the last example, the shell `/bin/sh` was not used. The
redirection of the standard output of `grep` to the standard input of
`wc` and the standard output of `wc` to the file "`ncpus`" is done by
the NoShell library itself.

For more documentation and examples, see the `doc` directory.

## Why such a library?

Using a shell to interpret the command passed to "`system()`",
"`popen()`", or equivalent, is full of problems. The shell has a very
complex syntax and the parsing of a command from a string brings
[security issues][shell shock] and problems with escaping (e.g. spaces in paths,
special characters, etc.).

Starting sub-process with `fork()/exec()` or `spawn()`, and making the
redirections with `pipe()/dup()` is not so easy and takes a lot of
boilerplate code. It is easy to get it wrong and leak resources (file
descriptors, memory). NoShell makes all of this simple and convenient,
and does not leak resources.

NoShell brings some of the functionality of a shell (running multiple
process in a pipeline with input/output redirections) without its
problems: the strings are not interpreted and passed directly as
arguments to the exec call.

[shell shock]: http://en.wikipedia.org/wiki/Shellshock_(software_bug) "Shellshock"

## Installation

For installation, use autoconf/automake:

```sh
autoreconf -i
./configure
make
sudo make install
```

At this point, this library has only been tested on Linux, with `g++`
version 4.7 or newer and `clang++` version 3.2.

## Using the library

Add the following to your make file:

```make
CXXFLAGS = $(shell pkg-config --cflags noshell)
LDFLAGS = $(shell pkg-config --libs noshell)
```
