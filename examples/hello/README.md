# "Hello world" example

This example shows a minimal setup for an application using `libweb`. When
executed, it starts a HTTP/1.1 server on a random port, which is then printed
to the standard output, and returns an example website when either `/` or
`/index.html` are accessed by clients, reading:

```
Hello from libweb!
```

## How to build

If using `make(1)`, just run `make` from this directory.

If using CMake, examples are built by default when configuring the project
from [the top-level `CMakeLists.txt`](../../CMakeLists.txt).

## How to run

Run the executable without any command line arguments.
