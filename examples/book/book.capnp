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

struct Nulldata $C.mapname("nulldata_t") {
  null  @0: UInt32 $C.mapname("null_");
}

struct Buy $C.mapname("buy_t") {
  from  @0: Text;
  u :union $C.mapname("u") $C.mapuniontag("with_recipe") {
    norecipe   @1: Void;
    recipeAddr @2: Text $C.mapname("recipe_addr");
  }
}
struct Book $C.mapname("book_t") {
  title   @0: Text;
  authors @1: List(Text) $C.mapname("authors") $C.maplistcount("n_authors");
  chapters @5: List(Chapter) $C.mapname("chapters_") $C.maplistcount("n_chapters");
  publish  @6: Publish;
  nulldata @7: Nulldata;
  magic1  @2: List(UInt32) $C.mapname("magic_1") $C.maplistcount("n_magic1");
  description @8: Text;
  acquire :union $C.mapuniontag("acquire_method") {
    buy   @3: Buy;
    donation @4: Text;
  }
}
