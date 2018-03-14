# SSH
Coroutine TS based [libssh][libssh] wrapper written in C++20.

## Requirements
* [Visual Studio 2017][vs2017] and [VCPKG][vcpkg] on Windows.
* [LLVM][llvm] with [libcxx][libcxx] version 5.0.1 or newer on Linux and FreeBSD.

The [solution.cmd](solution.cmd) script expects `cmake` in `PATH`.<br/>
The [makefile](makefile) script expects `cmake`, `clang` and `clang++` in `PATH`.

Set the `VCPKG` environment variable to `…/vcpkg/scripts/buildsystems/vcpkg.cmake`.<br/>
Set the `VCPKG_DEFAULT_TRIPLET` environment variable to `x64-windows-static`.<br/>

## Dependencies
Install dependencies on Windows.

```cmd
vcpkg install gtest libssh
```

Install dependencies on Ubuntu.

```sh
apt install libssh-dev
```

Install dependencies on FreeBSD.

```sh
pkg install libssh
```

## Build
Execute [solution.cmd](solution.cmd) to configure the project with cmake and open it in Visual Studio 2017.<br/>
Execute `make` in the project directory to configure and build the project with cmake.<br/>
More useful targets are provided inside the [makefile](makefile).

[libssh]: https://www.libssh.org/
[vs2017]: https://www.visualstudio.com/downloads/
[llvm]: https://llvm.org/
[libcxx]: https://libcxx.llvm.org/
[vcpkg]: https://github.com/Microsoft/vcpkg
