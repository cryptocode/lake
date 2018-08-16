# Lake

This repository contains the reference implementation of the Lake Virtual Machine.

Lake is a "functional assembly language" for Linux, macOS and Windows.

Features include:
* higher order functions
* exception handling
* garbage collection
* arbitrary precision numeric data types
* foreign function call
* maps and array
* modules and macros
* unified loop- and branching construct

The primary utility is as a target for experimenting with language design. Frontends can either generate Lake assembly, or use the Lake API.

Lake is interpreted, and scripts can be bundled into a stand-alone executable. Bundling the VM in a host process is also possible.

## Status

Lake is in an early stage of development, but a good number of advanced tests pass. It should be usable for programming language research projects.

Currently the tests are the best documentation.

## Build

Clone this repository and build with cmake.

Dependencies (boost, mpir, libffi) must currently be installed under ../libs (alternatively, adjust CMakeLists.txt)

```
cmake -G "Unix Makefiles"
make
```

## Run tests

Any .lake file under the test directory with #AUTOTEST as the first line will be executed by the test script.

```
test/runtests.pl
```

There's also a test binary for testing the lake API:

```
out/tests-basic
```
