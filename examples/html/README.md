# HTML serializer example

This example shows a minimal application that serializes a HTML file
and writes it to standard output.

## How to build

If using `make(1)`, just run `make` from this directory.

If using CMake, examples are built by default when configuring the project
from [the top-level `CMakeLists.txt`](../../CMakeLists.txt).

## How to run

Run the executable without any command line arguments. The following text
should be written to the standard output:

```
<html>
        <head>
                <meta charset="UTF-8"/>
        </head>
        <body>testing slweb</body>
</html>
```
