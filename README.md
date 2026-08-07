# capnpc-c

A C code generator plugin for [Cap'n Proto](https://capnproto.org/), an efficient protocol for sharing data and capabilities.

Given a `.capnp` schema file, `capnpc-c` generates C structs, read/write functions, and optionally higher-level codec (encode/decode/free) functions that convert between wire-format structs and your own application-defined C types.

> **Security warning:** The generated code assumes all input to be trusted.
> Do NOT use with untrusted input. There is currently no bounds-checking on
> structures or pointers.

## Prerequisites

- [Cap'n Proto](https://capnproto.org/install.html) compiler (`capnp`) installed and on your PATH
- A C99 compiler (gcc, clang, MSVC)
- [Meson](https://mesonbuild.com/) build system (>= 1.0.0) and [Ninja](https://ninja-build.org/)

## Building

```sh
git clone <this-repo>
cd c-capnproto
meson setup build
meson compile -C build
```

This builds:
- `libcapnp` — the runtime library
- `capnpc-c` — the code generator plugin

### Running tests

```sh
meson test -C build
```

### Build options

| Option | Default | Description |
|--------|---------|-------------|
| `enable_tests` | `true` | Build unit tests |
| `b_sanitize` | (none) | Enable sanitizers, e.g. `address`, `undefined` |

Example with address sanitizer:

```sh
meson setup build -Db_sanitize=address
meson compile -C build
meson test -C build
```

### Installing

```sh
meson install -C build
```

This installs:
- `capnpc-c` to `$prefix/bin/`
- `libcapnp` to `$prefix/lib/`
- `capnp_c.h` to `$prefix/include/`

## Usage

### Generating C code from a schema

If `capnpc-c` is on your PATH:

```sh
capnp compile -o c myschema.capnp
```

Otherwise, specify the path explicitly:

```sh
capnp compile -o /path/to/capnpc-c myschema.capnp
```

This produces `myschema.capnp.c` and `myschema.capnp.h` in the current directory.

To specify an output directory:

```sh
capnp compile -o /path/to/capnpc-c:/output/dir myschema.capnp
```

If your schema imports `c.capnp`, pass the compiler's source directory as an include path:

```sh
capnp compile -o c -I /path/to/c-capnproto/compiler myschema.capnp
```

### Linking the runtime library

Your project must compile and link these runtime source files:

- `lib/capn.c`
- `lib/capn-malloc.c`
- `lib/capn-stream.c`

And include `lib/capnp_c.h` (add `lib/` to your include path).

### Using as a meson subproject

Add a wrap file at `subprojects/c-capnproto.wrap`, then in your `meson.build`:

```meson
capnpc_proj = subproject('c-capnproto')
libcapnp_dep = capnpc_proj.get_variable('libcapnp_dep')

executable('myapp', 'main.c', 'myschema.capnp.c',
    dependencies: [libcapnp_dep])
```

## Generated Code Layers

The code generator produces two layers of functionality:

### Layer 1: Wire-format struct functions (always generated)

For each struct `Foo` in your schema, the generator produces:

| Function | Purpose |
|----------|---------|
| `new_Foo(seg)` | Allocate a new Foo in a segment |
| `new_Foo_list(seg, len)` | Allocate a list of Foo |
| `read_Foo(s, ptr)` | Read fields from wire format into `struct Foo` |
| `write_Foo(s, ptr)` | Write fields from `struct Foo` to wire format |
| `get_Foo(s, list, i)` | Read the i-th element from a Foo list |
| `set_Foo(s, list, i)` | Write the i-th element to a Foo list |

The generated `struct Foo` uses capnp runtime types (`capn_text`, `capn_data`, `capn_ptr`, etc.) for pointer fields.

### Layer 2: Codec functions (requires `$C.codecgen`)

When you annotate your schema file with `$C.codecgen`, the generator additionally produces codec functions that convert between the wire-format struct and your own application-defined C struct:

| Function | Purpose |
|----------|---------|
| `encode_Foo(cs, d, s)` | Copy from your struct `s` into wire struct `d` |
| `decode_Foo(d, s)` | Copy from wire struct `s` into your struct `d` (allocates memory) |
| `free_Foo(d)` | Free memory allocated by `decode_Foo` |
| `encode_Foo_ptr(cs, p, s)` | Encode + write to a new capnp allocation |
| `decode_Foo_ptr(d, p)` | Read + decode from a capnp pointer |
| `free_Foo_ptr(d)` | Free decoded pointer |
| `encode_Foo_list(cs, l, count, s)` | Encode an array of structs into a list |
| `decode_Foo_list(pcount, d, list)` | Decode a list into an array of struct pointers |
| `free_Foo_list(count, d)` | Free a decoded list |

### Layer 3: Getter/setter functions (requires `$C.fieldgetset`)

When you annotate with `$C.fieldgetset`, the generator produces per-field accessor functions:

```c
capn_text Foo_get_name(Foo_ptr p);
void Foo_set_name(Foo_ptr p, capn_text name);
uint32_t Foo_get_id(Foo_ptr p);
void Foo_set_id(Foo_ptr p, uint32_t id);
```

These allow accessing individual fields without reading/writing the entire struct.

## Annotations Reference

All annotations are defined in `compiler/c.capnp`. Import them in your schema:

```capnp
using C = import "/c.capnp";
```

### File-level annotations

| Annotation | Type | Description |
|------------|------|-------------|
| `$C.fieldgetset` | Void | Generate per-field getter/setter functions |
| `$C.codecgen` | Void | Generate encode/decode/free codec functions |
| `$C.extraheader("...")` | Text | Add preprocessor directives to the generated header |
| `$C.namespace("...")` | Text | Prefix all struct names with a namespace string |
| `$C.nameinfix("...")` | Text | Insert text before `.c`/`.h` in output filenames |
| `$C.extendedattribute("...")` | Text | Add an attribute (e.g. `__declspec(dllexport)`) to all generated functions |
| `$C.donotinclude(id)` | UInt64 | Suppress `#include` for an imported file by its ID |

### Type-level annotations

| Annotation | Applies to | Description |
|------------|-----------|-------------|
| `$C.mapname("type_name")` | struct, enum, field, union | Map to a custom C type/field name in codec |
| `$C.typedefto("type_name")` | struct, enum | Generate a typedef declaration |
| `$C.mapuniontag("field_name")` | union, struct | Map the union discriminant to a custom field name |

### Field-level annotations

| Annotation | Applies to | Description |
|------------|-----------|-------------|
| `$C.mapname("field_name")` | field | Map to a custom field name in the user struct |
| `$C.maplistcount("count_field")` | field | Specify the name of the count field for lists/Data |

## Type Mapping

### Scalar types

| Cap'n Proto | Generated C type (wire struct) | User struct (codec) |
|-------------|-------------------------------|---------------------|
| Bool | `uint8_t` (bit) | `uint8_t` |
| Int8/UInt8 | `int8_t`/`uint8_t` | same |
| Int16/UInt16 | `int16_t`/`uint16_t` | same |
| Int32/UInt32 | `int32_t`/`uint32_t` | same |
| Int64/UInt64 | `int64_t`/`uint64_t` | same |
| Float32 | `float` | `float` |
| Float64 | `double` | `double` |
| Enum | `enum FooEnum` | same |

### Pointer types

| Cap'n Proto | Generated C type (wire struct) | User struct (codec) |
|-------------|-------------------------------|---------------------|
| Text | `capn_text` | `char *` |
| Data | `capn_data` | `uint8_t *` + length field |
| struct Foo | `Foo_ptr` | `foo_t *` (pointer, heap-allocated) |
| List(scalar) | `capn_listN` | `scalar_type *` + count field |
| List(Text) | `capn_ptr` | `char **` + count field |
| List(Data) | `capn_ptr` | `capnp_data_t *` + count field |
| List(struct) | `Foo_list` | `foo_t **` + count field |

### The `capnp_data_t` type

For `Data` fields decoded via codec, the runtime provides:

```c
typedef struct {
    uint8_t *data;
    int len;
} capnp_data_t;
```

This is used for individual elements when decoding `List(Data)`.

## Complete Example: Codec with Data fields

### Schema (`message.capnp`)

```capnp
@0xabcdef1234567890;

using C = import "/c.capnp";
$C.fieldgetset;
$C.codecgen;
$C.extraheader("#include \"message.h\"");

struct Message $C.mapname("message_t") {
  name     @0 :Text;
  payload  @1 :Data    $C.mapname("payload") $C.maplistcount("payload_len");
  chunks   @2 :List(Data) $C.mapname("chunks") $C.maplistcount("n_chunks");
  tag      @3 :UInt32;
}
```

### User struct header (`message.h`)

```c
#ifndef MESSAGE_H
#define MESSAGE_H

#include <stdint.h>
#include "capnp_c.h"

typedef struct {
    char *name;              /* Text -> char* */
    uint8_t *payload;        /* Data -> uint8_t* */
    int payload_len;         /* Data length */
    int n_chunks;            /* List(Data) count */
    capnp_data_t *chunks;   /* List(Data) -> array of {data, len} */
    uint32_t tag;            /* UInt32 -> uint32_t */
} message_t;

#endif
```

### Generating code

```sh
capnp compile -o capnpc-c -I /path/to/c-capnproto/compiler message.capnp
```

Produces `message.capnp.c` and `message.capnp.h`.

### Encoding (serialize to bytes)

```c
#include "message.capnp.h"
#include "message.h"

void send_message(void) {
    uint8_t payload[] = {0xDE, 0xAD, 0xBE, 0xEF};
    capnp_data_t chunks[2] = {
        {.data = (uint8_t*)"hello", .len = 5},
        {.data = (uint8_t*)"world", .len = 5},
    };

    message_t msg = {
        .name = "example",
        .payload = payload,
        .payload_len = sizeof(payload),
        .n_chunks = 2,
        .chunks = chunks,
        .tag = 42,
    };

    struct capn c;
    capn_init_malloc(&c);
    struct capn_segment *cs = capn_root(&c).seg;

    Message_ptr ptr;
    encode_Message_ptr(cs, &ptr, &msg);
    capn_setp(capn_root(&c), 0, ptr.p);

    /* Write to file descriptor */
    capn_write_fd(&c, write, fd, 0 /* not packed */);

    capn_free(&c);
}
```

### Decoding (deserialize from bytes)

```c
#include "message.capnp.h"
#include "message.h"

void receive_message(const uint8_t *buf, size_t len) {
    struct capn c;
    capn_init_mem(&c, buf, len, 0 /* not packed */);

    Message_ptr ptr;
    ptr.p = capn_getp(capn_root(&c), 0, 1);

    message_t *msg = NULL;
    decode_Message_ptr(&msg, ptr);

    if (msg) {
        printf("name: %s\n", msg->name);
        printf("payload (%d bytes):", msg->payload_len);
        for (int i = 0; i < msg->payload_len; i++)
            printf(" %02x", msg->payload[i]);
        printf("\n");

        printf("chunks (%d):\n", msg->n_chunks);
        for (int i = 0; i < msg->n_chunks; i++) {
            printf("  [%d] %d bytes\n", i, msg->chunks[i].len);
        }
        printf("tag: %u\n", msg->tag);

        free_Message_ptr(&msg);  /* frees all allocated memory */
    }

    capn_free(&c);
}
```

## Complete Example: Low-level API (without codec)

For simpler use cases without codec, you work directly with the wire-format structs:

### Schema (`addressbook.capnp`)

```capnp
@0x9eb32e19f86ee174;

using C = import "/c.capnp";
$C.fieldgetset;

struct Person {
  id    @0 :UInt32;
  name  @1 :Text;
  email @2 :Text;
}
```

### Encoding

```c
#include "addressbook.capnp.h"

void write_person(void) {
    struct capn c;
    capn_init_malloc(&c);
    struct capn_segment *cs = capn_root(&c).seg;

    Person_ptr person = new_Person(cs);
    struct Person p;
    memset(&p, 0, sizeof(p));
    p.id = 123;
    p.name.str = "Alice";
    p.name.len = strlen(p.name.str);
    p.name.seg = NULL;
    p.email.str = "alice@example.com";
    p.email.len = strlen(p.email.str);
    p.email.seg = NULL;

    write_Person(&p, person);
    capn_setp(capn_root(&c), 0, person.p);

    /* serialize... */
    capn_write_fd(&c, write, 1, 0);
    capn_free(&c);
}
```

### Decoding

```c
#include "addressbook.capnp.h"

void read_person(const uint8_t *buf, size_t len) {
    struct capn c;
    capn_init_mem(&c, buf, len, 0);

    Person_ptr person;
    person.p = capn_getp(capn_root(&c), 0, 1);

    struct Person p;
    read_Person(&p, person);

    printf("id: %u, name: %s, email: %s\n", p.id, p.name.str, p.email.str);

    capn_free(&c);
}
```

## Runtime API Summary

### Initialization

| Function | Description |
|----------|-------------|
| `capn_init_malloc(&c)` | Initialize for writing (allocates segments on heap) |
| `capn_init_mem(&c, buf, sz, packed)` | Initialize by reading from memory buffer |
| `capn_init_fp(&c, fp, packed)` | Initialize by reading from a FILE* |
| `capn_free(&c)` | Free all segments |

### Serialization

| Function | Description |
|----------|-------------|
| `capn_root(&c)` | Get the root pointer of a message |
| `capn_setp(p, off, tgt)` | Set a pointer field |
| `capn_getp(p, off, resolve)` | Get a pointer field |
| `capn_size(&c)` | Calculate serialized size (unpacked) |
| `capn_write_mem(&c, buf, sz, packed)` | Serialize to memory buffer |
| `capn_write_fd(&c, write_fn, fd, packed)` | Serialize to file descriptor |

### Text and Data

| Function | Description |
|----------|-------------|
| `capn_get_text(p, off, def)` | Read a Text field |
| `capn_set_text(p, off, text)` | Write a Text field |
| `capn_get_data(p, off)` | Read a Data field |
| `capn_new_list8(seg, sz)` | Create a byte list (for writing Data) |
| `capn_setv8(list, off, data, sz)` | Write bytes into a list |
| `capn_getv8(list, off, data, sz)` | Read bytes from a list |

Note: There is no `capn_set_data()`. To write Data, create a list8 and set `data.p = list.p`.

## Project Structure

```
c-capnproto/
├── lib/                    # Runtime library
│   ├── capnp_c.h          # Public header
│   ├── capn.c             # Core implementation
│   ├── capn-malloc.c      # Malloc-based segment allocator
│   └── capn-stream.c      # Serialization/deserialization
├── compiler/               # Code generator plugin
│   ├── capnpc-c.c         # Plugin entry point
│   ├── codegen.c          # Wire struct code generation
│   ├── codegen_codec.c    # Codec (encode/decode/free) code generation
│   ├── c.capnp            # Annotation definitions
│   └── schema.capnp       # Cap'n Proto schema meta-schema
├── tests/                  # Test files
├── examples/book/          # Full codec example
├── meson.build             # Build system
├── meson_options.txt       # Build options
└── subprojects/gtest.wrap  # Google Test dependency
```

## License

MIT License. See [COPYING](COPYING) for details.
