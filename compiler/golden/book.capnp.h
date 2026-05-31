#ifndef CAPN_F9FFB48DDE27C0E6
#define CAPN_F9FFB48DDE27C0E6
/* AUTO GENERATED - DO NOT EDIT */
#include <capnp_c.h>
#include <book.h>

#ifndef STRING_DUP
#define STRING_DUP strdup
#endif

#if CAPN_VERSION != 1
#error "version mismatch between capnp_c.h and generated code"
#endif

#ifndef capnp_nowarn
# ifdef __GNUC__
#  define capnp_nowarn __extension__
# else
#  define capnp_nowarn
# endif
#endif


#ifdef __cplusplus
extern "C" {
#endif

struct Chapter;
struct Publish;
struct Nulldata;
struct Buy;
struct Book;

typedef struct {capn_ptr p;} Chapter_ptr;
typedef struct {capn_ptr p;} Publish_ptr;
typedef struct {capn_ptr p;} Nulldata_ptr;
typedef struct {capn_ptr p;} Buy_ptr;
typedef struct {capn_ptr p;} Book_ptr;

typedef struct {capn_ptr p;} Chapter_list;
typedef struct {capn_ptr p;} Publish_list;
typedef struct {capn_ptr p;} Nulldata_list;
typedef struct {capn_ptr p;} Buy_list;
typedef struct {capn_ptr p;} Book_list;

struct Chapter {
	capn_text caption;
	uint32_t start;
	uint32_t end;
};

static const size_t Chapter_word_count = 1;

static const size_t Chapter_pointer_count = 1;

static const size_t Chapter_struct_bytes_count = 16;


capn_text Chapter_get_caption(Chapter_ptr p);

uint32_t Chapter_get_start(Chapter_ptr p);

uint32_t Chapter_get_end(Chapter_ptr p);

void Chapter_set_caption(Chapter_ptr p, capn_text caption);

void Chapter_set_start(Chapter_ptr p, uint32_t start);

void Chapter_set_end(Chapter_ptr p, uint32_t end);

struct Publish {
	uint64_t isbn;
	uint32_t year;
};

static const size_t Publish_word_count = 2;

static const size_t Publish_pointer_count = 0;

static const size_t Publish_struct_bytes_count = 16;


uint64_t Publish_get_isbn(Publish_ptr p);

uint32_t Publish_get_year(Publish_ptr p);

void Publish_set_isbn(Publish_ptr p, uint64_t isbn);

void Publish_set_year(Publish_ptr p, uint32_t year);

struct Nulldata {
	uint32_t null;
};

static const size_t Nulldata_word_count = 1;

static const size_t Nulldata_pointer_count = 0;

static const size_t Nulldata_struct_bytes_count = 8;


uint32_t Nulldata_get_null(Nulldata_ptr p);

void Nulldata_set_null(Nulldata_ptr p, uint32_t null);
enum Buy_u_which {
	Buy_u_norecipe = 0,
	Buy_u_recipeAddr = 1
};

struct Buy {
	capn_text from;
	enum Buy_u_which u_which;
	capnp_nowarn union {
		capn_text recipeAddr;
	} u;
};

static const size_t Buy_word_count = 1;

static const size_t Buy_pointer_count = 2;

static const size_t Buy_struct_bytes_count = 24;


capn_text Buy_get_from(Buy_ptr p);

void Buy_set_from(Buy_ptr p, capn_text from);
enum Book_acquire_which {
	Book_acquire_buy = 0,
	Book_acquire_donation = 1
};

struct Book {
	capn_text title;
	capn_ptr authors;
	Chapter_list chapters;
	Publish_ptr publish;
	Nulldata_ptr nulldata;
	capn_list32 magic1;
	capn_text description;
	enum Book_acquire_which acquire_which;
	capnp_nowarn union {
		Buy_ptr buy;
		capn_text donation;
	} acquire;
};

static const size_t Book_word_count = 1;

static const size_t Book_pointer_count = 8;

static const size_t Book_struct_bytes_count = 72;


capn_text Book_get_title(Book_ptr p);

capn_ptr Book_get_authors(Book_ptr p);

Chapter_list Book_get_chapters(Book_ptr p);

Publish_ptr Book_get_publish(Book_ptr p);

Nulldata_ptr Book_get_nulldata(Book_ptr p);

capn_list32 Book_get_magic1(Book_ptr p);

capn_text Book_get_description(Book_ptr p);

void Book_set_title(Book_ptr p, capn_text title);

void Book_set_authors(Book_ptr p, capn_ptr authors);

void Book_set_chapters(Book_ptr p, Chapter_list chapters);

void Book_set_publish(Book_ptr p, Publish_ptr publish);

void Book_set_nulldata(Book_ptr p, Nulldata_ptr nulldata);

void Book_set_magic1(Book_ptr p, capn_list32 magic1);

void Book_set_description(Book_ptr p, capn_text description);

Chapter_ptr new_Chapter(struct capn_segment*);
Publish_ptr new_Publish(struct capn_segment*);
Nulldata_ptr new_Nulldata(struct capn_segment*);
Buy_ptr new_Buy(struct capn_segment*);
Book_ptr new_Book(struct capn_segment*);

Chapter_list new_Chapter_list(struct capn_segment*, int len);
Publish_list new_Publish_list(struct capn_segment*, int len);
Nulldata_list new_Nulldata_list(struct capn_segment*, int len);
Buy_list new_Buy_list(struct capn_segment*, int len);
Book_list new_Book_list(struct capn_segment*, int len);

void read_Chapter(struct Chapter*, Chapter_ptr);
void read_Publish(struct Publish*, Publish_ptr);
void read_Nulldata(struct Nulldata*, Nulldata_ptr);
void read_Buy(struct Buy*, Buy_ptr);
void read_Book(struct Book*, Book_ptr);

void write_Chapter(const struct Chapter*, Chapter_ptr);
void write_Publish(const struct Publish*, Publish_ptr);
void write_Nulldata(const struct Nulldata*, Nulldata_ptr);
void write_Buy(const struct Buy*, Buy_ptr);
void write_Book(const struct Book*, Book_ptr);

void get_Chapter(struct Chapter*, Chapter_list, int i);
void get_Publish(struct Publish*, Publish_list, int i);
void get_Nulldata(struct Nulldata*, Nulldata_list, int i);
void get_Buy(struct Buy*, Buy_list, int i);
void get_Book(struct Book*, Book_list, int i);

void set_Chapter(const struct Chapter*, Chapter_list, int i);
void set_Publish(const struct Publish*, Publish_list, int i);
void set_Nulldata(const struct Nulldata*, Nulldata_list, int i);
void set_Buy(const struct Buy*, Buy_list, int i);
void set_Book(const struct Book*, Book_list, int i);

void encode_Chapter(struct capn_segment *,struct Chapter *, chapter_t *);
void decode_Chapter(chapter_t *, struct Chapter *);
void free_Chapter(chapter_t *);
void encode_Chapter_list(struct capn_segment *,Chapter_list *, int, chapter_t **);
void decode_Chapter_list(int *, chapter_t ***, Chapter_list);
void free_Chapter_list(int, chapter_t **);
void encode_Chapter_ptr(struct capn_segment*, Chapter_ptr *, chapter_t *);
void decode_Chapter_ptr(chapter_t **, Chapter_ptr);
void free_Chapter_ptr(chapter_t **);

void encode_Publish(struct capn_segment *,struct Publish *, publish_t *);
void decode_Publish(publish_t *, struct Publish *);
void free_Publish(publish_t *);
void encode_Publish_list(struct capn_segment *,Publish_list *, int, publish_t **);
void decode_Publish_list(int *, publish_t ***, Publish_list);
void free_Publish_list(int, publish_t **);
void encode_Publish_ptr(struct capn_segment*, Publish_ptr *, publish_t *);
void decode_Publish_ptr(publish_t **, Publish_ptr);
void free_Publish_ptr(publish_t **);

void encode_Nulldata(struct capn_segment *,struct Nulldata *, nulldata_t *);
void decode_Nulldata(nulldata_t *, struct Nulldata *);
void free_Nulldata(nulldata_t *);
void encode_Nulldata_list(struct capn_segment *,Nulldata_list *, int, nulldata_t **);
void decode_Nulldata_list(int *, nulldata_t ***, Nulldata_list);
void free_Nulldata_list(int, nulldata_t **);
void encode_Nulldata_ptr(struct capn_segment*, Nulldata_ptr *, nulldata_t *);
void decode_Nulldata_ptr(nulldata_t **, Nulldata_ptr);
void free_Nulldata_ptr(nulldata_t **);

void encode_Buy(struct capn_segment *,struct Buy *, buy_t *);
void decode_Buy(buy_t *, struct Buy *);
void free_Buy(buy_t *);
void encode_Buy_list(struct capn_segment *,Buy_list *, int, buy_t **);
void decode_Buy_list(int *, buy_t ***, Buy_list);
void free_Buy_list(int, buy_t **);
void encode_Buy_ptr(struct capn_segment*, Buy_ptr *, buy_t *);
void decode_Buy_ptr(buy_t **, Buy_ptr);
void free_Buy_ptr(buy_t **);

void encode_Book(struct capn_segment *,struct Book *, book_t *);
void decode_Book(book_t *, struct Book *);
void free_Book(book_t *);
void encode_Book_list(struct capn_segment *,Book_list *, int, book_t **);
void decode_Book_list(int *, book_t ***, Book_list);
void free_Book_list(int, book_t **);
void encode_Book_ptr(struct capn_segment*, Book_ptr *, book_t *);
void decode_Book_ptr(book_t **, Book_ptr);
void free_Book_ptr(book_t **);


#ifdef __cplusplus
}
#endif
#endif
