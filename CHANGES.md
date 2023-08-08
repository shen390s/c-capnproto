# Changes

## 0.9.0

- Forked to the DkML repository on GitLab.
- Use binary mode when reading schemas on Windows
- Use SSIZE_T from `BaseTsd.h` to allow MSVC to compile with 32-bits
- Put alignment on segment fields, not just the structure.
- Avoids left shifts of signed integers, which is undefined C behavior. Passes
  ASAN (not a false positive)
- Wrap macro parameters in the `capnp_use(x)` macro. Passes clang-tidy (not
  a false positive)


## 0.3 (632f0d73a1f4a03026b5e4727386b9fe3ec6e00e)

- Fork of `c-capnproto-0.3` plus several untagged commits from https://github.com/opensourcerouting/c-capnproto on Apr 23, 2023. Specifically: https://github.com/opensourcerouting/c-capnproto/commit/632f0d73a1f4a03026b5e4727386b9fe3ec6e00e
