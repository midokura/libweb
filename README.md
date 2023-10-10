# libweb, a simple and lightweight web framework

`libweb` is a simple and lightweight implementation of a web server, written in
C99 plus POSIX.1-2008 extensions, that can be integrated into applications.

## Disclaimer

Intentionally, `libweb` does not share some of the philosophical views from the
[suckless project](https://suckless.org). However, it still strives towards
portability, minimalism, simplicity and efficiency.

## Features

- Small and portable HTTP/1.1 server implementation, with support for
`GET` and `POST`.
- Provides a interface to set up user-defined callbacks depending on
the operation (see example below).
- Transport-agnostic implementation.
    - While a POSIX socket, TCP-based implementation is already
    provided, the HTTP interface can be mapped to any other reliable
    transport layer.
- Supports [`multiform/form-data`](https://developer.mozilla.org/en-US/docs/Web/HTTP/Methods/POST)
, which makes it useful to transfer large amounts of data, such as
binary files.
- [A library](include/libweb/html.h) to write HTML programmatically.

### TLS

In order to maintain simplicity and reduce the risk for security bugs, `libweb`
does **not** implement TLS support. Instead, this should be provided by a
reverse proxy, such as [`caddy`](https://caddyserver.com/).

### Root permissions

`libweb` does not require root permissions. So, in order to avoid the
risk for security bugs, **please do not run `libweb` as `root`**.

## Requirements

- A POSIX environment.
- [`dynstr`](https://gitea.privatedns.org/xavi/dynstr)
(provided as a `git` submodule).
- CMake (optional).

### Ubuntu / Debian

#### Mandatory packages

```sh
sudo apt install build-essential
```

#### Optional packages

```sh
sudo apt install cmake
```

## How to use
### Build

Two build environments are provided for `libweb` - feel free to choose any of
them:

- A mostly POSIX-compliant [`Makefile`](Makefile).
- A [`CMakeLists.txt`](CMakeLists.txt).

`libweb` can be built using the standard build process:

#### Make

```sh
$ make
```

This would generate a static library, namely `libweb.a`, on the project
top-level directory. Applications can then call the top-level `Makefile` by
the use of recursive `make`. For example, assuming `libweb` is contained on a
subdirectory:

```make
libweb/libweb.a:
    +cd libweb && $(MAKE)
```

Additionally, `libweb` can be installed using the `install` target. A
custom prefix can be assigned via the `PREFIX` variable:

```sh
$ make PREFIX=$HOME/libweb-prefix install
```

By default, `PREFIX` is assigned to `/usr/local`.

#### CMake

```sh
$ mkdir build/
$ cd build/
$ cmake ..
$ cmake --build .
```

A CMake target, also called `libweb`, is created. This makes it possible
to integrate `libweb` into CMake projects via `add_subdirectory` and
`target_link_libraries`. For example:

```cmake
project(example)
add_executable(${PROJECT_NAME} main.c)
add_subdirectory(libweb)
target_link_libraries(${PROJECT_NAME} PRIVATE libweb)
```

Additionally, `libweb` can be installed using the standard procedure
in CMake. As usual, a custom prefix can be assigned via the
`CMAKE_INSTALL_PREFIX` variable:

```sh
$ cmake --install build/ -DCMAKE_INSTALL_PREFIX=$HOME/libweb-prefix
```

### Examples

[A directory](examples) with examples shows how `libweb` can be used by
applications. These can be built from the top-level directory with:

```sh
$ make examples
```

In the case of CMake builds, examples are built by default. This can be turned
off by assigning `BUILD_EXAMPLES` to `OFF` or `0`:

```sh
$ mkdir build/
$ cd build/
$ cmake .. -DBUILD_EXAMPLES=OFF
$ cmake --build .
```

## Why this project?

Originally, `libweb` was part of the
[`slcl`](https://gitea.privatedns.org/xavi92/slcl) project, a lightweight
cloud solution also written in C99 plus POSIX extensions. However, there
always was a clear separation between application logic and the underlying
HTTP/1.1 server implementation and other surrounding utilities.

Therefore, it made sense to keep all these components on a separate
repository that `slcl` could depend on. Additionally, this would also
benefit other applications interested in this implementation.

### Seriously, why _yet another_ new HTTP/1.1 implementation?

- Popular web server implementations, such as
[`apache`](https://httpd.apache.org/) or [`nginx`](https://nginx.net) are
standalone applications that can be configured to run other
applications in order to generate dynamic content, via a standard
interface called
[Common Gateway Interface](https://en.wikipedia.org/wiki/Common_Gateway_Interface)
, or CGI for short.
    - However, those are vastly complex tools with many features and
    options, whereas simplicity was one of the key design goals for
    `slcl`.
    - Additionally, tools such as `apache` or `nginx` place
    configuration files into `/etc`, which makes it harder to run
    multiple instances on the same machine. While not a strict
    requirement from `slcl`, it was desirable to keep configuration as
    simple as possible for administrators.
- The [`onion`](https://github.com/davidmoreno/onion) project, which
does follow the HTTP library concept, was initially considered for
`slcl`, but has a larger scope than `libweb`, and again simplicity was
essential for `slcl`.
- And, after all, it was a good excuse to learn about HTTP/1.1.

## License

```
libweb, a simple and lightweight web framework.
Copyright (C) 2023  Xavier Del Campo Romero

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
```

Also, see [`LICENSE`](LICENSE).
