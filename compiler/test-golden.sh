#!/bin/sh
# Golden file test: verifies capnpc-c output hasn't changed.
# Usage: ./compiler/test-golden.sh [path-to-capnpc-c]
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
CAPNPC_C="${1:-$ROOT_DIR/builddir/capnpc-c}"
TMPDIR=$(mktemp -d)
trap 'rm -rf "$TMPDIR"' EXIT

fail=0

# Test 1: book.capnp (has $C.codecgen + $C.mapname - full codec output)
capnp compile -o "$CAPNPC_C:$TMPDIR" --src-prefix="$ROOT_DIR/examples/book" \
  -I "$ROOT_DIR/compiler" "$ROOT_DIR/examples/book/book.capnp"
if ! diff -u "$ROOT_DIR/compiler/golden/book.capnp.h" "$TMPDIR/book.capnp.h"; then
  echo "FAIL: book.capnp.h differs"
  fail=1
fi
if ! diff -u "$ROOT_DIR/compiler/golden/book.capnp.c" "$TMPDIR/book.capnp.c"; then
  echo "FAIL: book.capnp.c differs"
  fail=1
fi

# Test 2: addressbook.capnp (no codecgen)
capnp compile -o "$CAPNPC_C:$TMPDIR" --src-prefix="$ROOT_DIR/tests" \
  -I "$ROOT_DIR/compiler" "$ROOT_DIR/tests/addressbook.capnp"
if ! diff -u "$ROOT_DIR/compiler/golden/addressbook.capnp.h" "$TMPDIR/addressbook.capnp.h"; then
  echo "FAIL: addressbook.capnp.h differs"
  fail=1
fi
if ! diff -u "$ROOT_DIR/compiler/golden/addressbook.capnp.c" "$TMPDIR/addressbook.capnp.c"; then
  echo "FAIL: addressbook.capnp.c differs"
  fail=1
fi

# Test 3: group-in-union.capnp (codecgen with :group inside union)
capnp compile -o "$CAPNPC_C:$TMPDIR" --src-prefix="$ROOT_DIR/tests" \
  -I "$ROOT_DIR/compiler" "$ROOT_DIR/tests/group-in-union.capnp"
if ! diff -u "$ROOT_DIR/compiler/golden/group-in-union.capnp.h" "$TMPDIR/group-in-union.capnp.h"; then
  echo "FAIL: group-in-union.capnp.h differs"
  fail=1
fi
if ! diff -u "$ROOT_DIR/compiler/golden/group-in-union.capnp.c" "$TMPDIR/group-in-union.capnp.c"; then
  echo "FAIL: group-in-union.capnp.c differs"
  fail=1
fi

if [ $fail -eq 0 ]; then
  echo "OK: all golden files match"
fi
exit $fail
