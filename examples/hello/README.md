# "Hello world" example

This example shows a minimal setup for an application using `slweb`. When
executed, it starts a HTTP/1.1 server on port `8080` and returns an example
website reading "Hello from slweb!" when either `/` or `/index.html` are
accessed by clients.

## How to build

If using `make(1)`, just run `make` from this directory.

If using CMake, examples are built by default when configuring the project
from [the top-level `CMakeLists.txt`](../../CMakeLists.txt).

## How to run

Run the executable without any command line arguments.
