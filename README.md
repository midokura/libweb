# slweb, a suckless web framework

`slweb` is a simple and fast implementation of a web server, written in C99,
that can be integrated into applications.

## Disclaimer

While `slweb` might not share some of the philosophical views from the
[suckless project](https://suckless.org), it still strives towards minimalism,
simplicity and efficiency.

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
- [A library](include/slweb/html.h) to write HTML programmatically.

### TLS

In order to maintain simplicity and reduce the risk for security bugs, `slweb`
does **not** implement TLS support. Instead, this should be provided by a
reverse proxy, such as [`caddy`](https://caddyserver.com/).

### Root permissions

`slweb` does not require root permissions. So, in order to avoid the
risk for security bugs, **please do not run `slweb` as `root`**.

## Requirements

- A POSIX environment.
- [`dynstr`](https://gitea.privatedns.org/xavi92/dynstr)
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

Two build environments are provided for `slweb` - feel free to choose any of
them:

- A mostly POSIX-compliant [`Makefile`](Makefile).
- A [`CMakeLists.txt`](CMakeLists.txt).

`slweb` can be built using the standard build process:

#### Make

```sh
$ make
```

This would generate a static library, namely `libslweb.a`, on the project
top-level directory. Applications can then call the top-level `Makefile` by
the use of recursive `make`. For example, assuming `slweb` is contained on a
subdirectory:

```make
slweb/libslweb.a:
    +cd slweb && $(MAKE)
```

#### CMake

```sh
$ mkdir build/
$ cmake ..
$ cmake --build .
```

A CMake target, also called `slweb`, is created. This makes it possible
to integrate `slweb` into CMake projects via `add_subdirectory` and
`target_link_libraries`. For example:

```cmake
project(example)
add_executable(${PROJECT_NAME} main.c)
add_subdirectory(slweb)
target_link_libraries(${PROJECT_NAME} PRIVATE slweb)
```

## Why this project?

Originally, `slweb` was part of the
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
`slcl`, but has a larger scope than `slweb`, and again simplicity was
essential for `slcl`.
- And, after all, it was a good excuse to learn about HTTP/1.1.

## License

```
slweb, a suckless web framework.
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
