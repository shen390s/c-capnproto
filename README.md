# capnpc-c

This is a C plugin for [Cap'n Proto](http://kentonv.github.io/capnproto), an
efficient protocol for sharing data and capabilities.

This is a fork of <https://github.com/opensourcerouting/c-capnproto> with support
for CMake and Windows, a build matrix that includes Android and Windows, and
fixes several bugs that start with the [CHANGES for version 0.9.0](./CHANGES.md#090).

The fork is maintained as part of [DkML](https://diskuv.com/dkmlbook/)
and [DkSDK](https://diskuv.com/cmake/help/latest/), but PRs are welcome from
anybody. If you are looking for your first PR, fixing the false positive
memory leaks in the test code would be great!

## Security warning

> The generated code assumes all input to be trusted. Do NOT use with
> untrusted input! There is currently no code in place to check if
> structures/pointers are within bounds.

This is only the code generator plugin, to properly make use of it you
need to download, build and install capnpc and then build and install
this project and then you can utilize it as:

```sh
capnpc compiler/test.capnp -oc
```

## Building on Linux

```sh
git clone https://gitlab.com/dkml/ext/c-capnproto.git
cd c-capnproto
cmake --preset=ci-linux_x86_64
cmake --build --preset=ci-tests
```

## Building on Windows

You will need Visual Studio 2019 installed. Other versions of Visual Studio may work; they simply haven't been tested.

Once you have Visual Studio, run the `x64 Native Tools Command Prompt for VS 2019` and type the following:

```sh
git clone https://gitlab.com/dkml/ext/c-capnproto.git
cd c-capnproto
cmake --preset=ci-windows_x86_64
cmake --build --preset=ci-tests
```

## Usage

### Generating C code from a `.capnp` schema file

The `compiler` directory contains the C language plugin (`capnpc-c`) for use with the `capnp` tool: <https://capnproto.org/capnp-tool.html>.

`capnp` will by default search `$PATH` for `capnpc-c` - if it's on your PATH, you can generate code for your schema as follows:

```sh
capnp compile -o c myschema.capnp
```

Otherwise, you can specify the path to the c plugin:

```sh
capnp compile -o ./capnpc-c myschema.capnp
```

`capnp` generates a C struct that corresponds to each capn proto struct, along with read/write functions that convert to/from capn proto form.

#### fieldgetset

If you want accessor functions for struct members, use the attribute `fieldgetset` in your `.capnp` file as follows:

```capnp
using C = import "${c-capnproto}/compiler/c.capnp";

$C.fieldgetset;

struct MyStruct {}
```

#### extraheader

If you want to add `#include <...>` or any other preprocessor statements in your generated C file, use the attribute `extraheader` in your `.capnp` file as follows:

```capnp
using C = import "${c-capnproto}/compiler/c.capnp";

$C.extraheader("include <stdio.h>");

struct MyStruct {}
```

#### extendedattribute

If you want to add an `__declspec(dllexport)` or some other extended attribute to your generated C functions and structs, use the attribute `extendedattribute` in your `.capnp` file as follows:

```capnp
using C = import "${c-capnproto}/compiler/c.capnp";

$C.extendedattribute("__declspec(dllexport)");

struct MyStruct {}
```

### Example C code

See the unit tests in [`tests/example-test.cpp`](tests/example-test.cpp).
The example schema file is [`tests/addressbook.capnp`](tests/addressbook.capnp).
The tests are written in C++, but only use C features.

You need to compile these runtime library files and link them into your own project's binaries:

* [`lib/capn.c`](lib/capn.c)
* [`lib/capn-malloc.c`](lib/capn-malloc.c)
* [`lib/capn-stream.c`](lib/capn-stream.c)

Your include path must contain the runtime library directory
[`lib`](lib). Header file [`lib/capnp_c.h`](lib/capnp_c.h) contains
the public interfaces of the library.

Using make-based builds, make may try to compile `${x}.capnp` from
`${x}.capnp.c` using its built-in rule for compiling `${y}` from
`${y}.c`. You can either disable make's built-in compile rules or just
this specific case with the no-op rule: `%.capnp: ;`.

For further reference, please see the other unit tests in [`tests`](tests), and header file [`lib/capnp_c.h`](lib/capnp_c.h).

The project [`quagga-capnproto`](https://github.com/opensourcerouting/quagga-capnproto) uses `c-capnproto` and contains some good examples, as found with [this github repository search](https://github.com/opensourcerouting/quagga-capnproto/search?utf8=%E2%9C%93&q=capn&type=):

* Serialization in function [`bgp_notify_send()`](https://github.com/opensourcerouting/quagga-capnproto/blob/27061648f3418fac0d217b16a46add534343e841/bgpd/bgp_zmq.c#L81-L96) in file `quagga-capnproto/bgpd/bgp_zmq.c`
* Deserialization in function [`qzc_callback()`](https://github.com/opensourcerouting/quagga-capnproto/blob/27061648f3418fac0d217b16a46add534343e841/lib/qzc.c#L249-L257) in file `quagga-capnproto/lib/qzc.c`

### Example CMake Usage

The minimum CMake version is 3.22. *The true CMake minimum version could be lower; you are welcome to test and submit a PR to [CMakeLists.txt](./CMakeLists.txt).*

```cmake
FetchContent_Declare(c-capnproto
        GIT_REPOSITORY https://gitlab.com/dkml/ext/c-capnproto.git
        # For reproducibility use a commit id rather than a tag
        GIT_TAG f07596dbb516329d27ea95060a758f5d48d26366)
FetchContent_MakeAvailable(c-capnproto)

# Creating an executable or library that uses c-capnproto?
# --------------------------------------------------------

# filename: yourcode.c
#
#   #include "capnp_c.h"
#   ...

add_executable(YourExecutable ... yourcode.c)
target_link_libraries(YourExecutable PRIVATE CapnC::Runtime)
```

## Status

<https://github.com/opensourcerouting/c-capnproto> was a merge of 3 forks of [James McKaskill's great
work](https://github.com/jmckaskill/c-capnproto), which had been untouched for
a while:

* [liamstask's fork](https://github.com/liamstask/c-capnproto)
* [baruch's fork](https://github.com/baruch/c-capnproto)
* [kylemanna's fork](https://github.com/kylemanna/c-capnproto)
