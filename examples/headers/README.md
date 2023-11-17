# HTTP headers example

This example shows a HTTP/1.1 server that listens to port `8080` and prints
the headers received from the client (up to a maximum of `max_headers`) to
standard output.

## How to build

If using `make(1)`, just run `make` from this directory.

If using CMake, examples are built by default when configuring the project
from [the top-level `CMakeLists.txt`](../../CMakeLists.txt).

## How to run

Run the executable without any command line arguments.
