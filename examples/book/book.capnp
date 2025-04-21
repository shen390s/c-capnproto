@0xf9ffb48dde27c0e6;

using C = import "/c.capnp";
$C.fieldgetset;
$C.codecgen;
$C.extraheader("#include <book.h>");

struct Chapter $C.mapname("chapter_t") {
  caption @0: Text;
  start   @1: UInt32;
  end     @2: UInt32;
}

struct Publish $C.mapname("publish_t") {
  isbn  @0: UInt64;
  year  @1: UInt32;
}
struct Book $C.mapname("book_t") {
  title   @0: Text;
  authors @1: List(Text) $C.mapname("authors") $C.maplistcount("n_authors");
  chapters @5: List(Chapter) $C.maplistcount("n_chapters");
  publish  @6: Publish;
  magic1  @2: List(UInt32) $C.maplistcount("n_magic1");
  acquire :union $C.mapuniontag("acquire_method") {
    buy   @3: Text;
    donation @4: Text;
  }
}
