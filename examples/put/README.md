# "Hello world" example

This example shows a minimal setup for an application using `libweb` that only
accepts `PUT` requests. When executed, it starts a HTTP/1.1 server on a random
port that shall be printed to standard output. When a `PUT` request is received,
`libweb` shall store the body into a file in the `/tmp` directory, and print the
path of the temporary file to the standard output.

## How to build

If using `make(1)`, just run `make` from this directory.

If using CMake, examples are built by default when configuring the project
from [the top-level `CMakeLists.txt`](../../CMakeLists.txt).

## How to run

Run the executable without any command line arguments.
