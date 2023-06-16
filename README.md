![CI](https://github.com/github/docs/actions/workflows/c-cpp.yml/badge.svg)

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

For more documentation and examples, see the [doc][github doc] directory.

[github doc]: https://github.com/gmarcais/NoShell/tree/master/doc

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

It is recommended to install using the release tarball `noshel-x.x.x.tar.gz` available from [Github releases](https://github.com/gmarcais/NoShell/releases) rather than from the git tree.

For development, use the git tree, use the `develop` branch and initialize autotools with `autoreconf -i`.


### Autotools

For installation, use autoconf/automake:

```sh
# autoreconf -i # Only if running from git tree
./configure
make
sudo make install
```

Run the unit tests (requires Google `gtest``) with:

``` shell
make check
```


### CMake

Alternatively, you can use CMake to build the library:

```sh
mkdir build
cmake -S . -B build -DNOSHELL_BUILD_TESTS=OFF
cmake --build build
sudo cmake --install build
```

To compile the unit tests (requires Google `gtest`), do not include `-DNOSHELL_BUILD_TESTS=OFF` in the `cmake` command above and run the tests with:

```sh
cd build && ctest
```

## Using the library

### Pkg-config

Add the following to your `Makefile`:

```make
CXXFLAGS = $(shell pkg-config --cflags noshell)
LDFLAGS = $(shell pkg-config --libs noshell)
```

This method works with autotools and cmake installation.

### CMake

Add the following to your `CMakeLists.txt`:

```cmake
find_package(noshell REQUIRED)

# link shared library
target_link_libraries(mytarget noshell::noshell)

# link static library
target_link_libraries(mytarget noshell::noshell-static)
```
