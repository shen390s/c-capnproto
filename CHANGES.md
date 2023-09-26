# Changes

## 0.9.1

- (backwards incompatible) Add CMake cache variable `BUILD_SHARED_LIBS`.
  The default is a static library for mobile (Android and iOS), and a
  shared library otherwise; that is the same pattern followed by opencv.
  The variable should be explicitly set to `ON` if a shared library must
  be created, and `OFF` if a static library must be created.
- Use GoogleTest 1.14.0 and enable tests only when C++14 compiler available.
- Support building into shared libraries on Windows:
- `extraheader` attribute: Extra `#include <stdio.h>` or any other
  preprocessor statements in auto-generated header file.
- `extendedattribute` attribute: Text in front of auto-generated functions,
  like `__declspec(dllexport)`

## 0.9.0

- Forked to the DkML repository on GitLab.
- Use binary mode when reading schemas on Windows
- Use SSIZE_T from `BaseTsd.h` to allow MSVC to compile with 32-bits
- Put alignment on segment fields, not just the structure.
- Avoids left shifts of signed integers, which is undefined C behavior. Passes
  ASAN (not a false positive)
- Wrap macro parameters in the `capnp_use(x)` macro. Passes clang-tidy (not
  a false positive)
- Add POSITION_INDEPENDENT_CODE to CapnC::Runtime
- Add C_CAPNPROTO_ENABLE_INSTALL with default ON to turn off and on
  installation of targets and files in `cmake --install`, especially for when
  this project is used within a FetchContent() command.

## 0.3 (632f0d73a1f4a03026b5e4727386b9fe3ec6e00e)

- Fork of `c-capnproto-0.3` plus several untagged commits from https://github.com/opensourcerouting/c-capnproto on Apr 23, 2023. Specifically: https://github.com/opensourcerouting/c-capnproto/commit/632f0d73a1f4a03026b5e4727386b9fe3ec6e00e
