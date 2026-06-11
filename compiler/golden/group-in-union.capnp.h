#ifndef CAPN_D4A4F1B8E2C6A390
#define CAPN_D4A4F1B8E2C6A390
/* AUTO GENERATED - DO NOT EDIT */
#include <capnp_c.h>

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

struct GroupInUnion;

typedef struct {capn_ptr p;} GroupInUnion_ptr;

typedef struct {capn_ptr p;} GroupInUnion_list;
enum GroupInUnion_data_which {
	GroupInUnion_data_foo = 0,
	GroupInUnion_data_bar = 1,
	GroupInUnion_data_baz = 2
};

struct GroupInUnion {
	enum GroupInUnion_data_which data_which;
	capnp_nowarn union {
		capnp_nowarn struct {
			int32_t x;
			int64_t y;
		} foo;
		capnp_nowarn struct {
			capn_text name;
			uint32_t value;
		} bar;
		capn_text baz;
	} data;
};

static const size_t GroupInUnion_word_count = 2;

static const size_t GroupInUnion_pointer_count = 1;

static const size_t GroupInUnion_struct_bytes_count = 24;


GroupInUnion_ptr new_GroupInUnion(struct capn_segment*);

GroupInUnion_list new_GroupInUnion_list(struct capn_segment*, int len);

void read_GroupInUnion(struct GroupInUnion*, GroupInUnion_ptr);

void write_GroupInUnion(const struct GroupInUnion*, GroupInUnion_ptr);

void get_GroupInUnion(struct GroupInUnion*, GroupInUnion_list, int i);

void set_GroupInUnion(const struct GroupInUnion*, GroupInUnion_list, int i);

void encode_GroupInUnion(struct capn_segment *,struct GroupInUnion *, group_in_union_t *);
void decode_GroupInUnion(group_in_union_t *, struct GroupInUnion *);
void free_GroupInUnion(group_in_union_t *);
void encode_GroupInUnion_list(struct capn_segment *,GroupInUnion_list *, int, group_in_union_t **);
void decode_GroupInUnion_list(int *, group_in_union_t ***, GroupInUnion_list);
void free_GroupInUnion_list(int, group_in_union_t **);
void encode_GroupInUnion_ptr(struct capn_segment*, GroupInUnion_ptr *, group_in_union_t *);
void decode_GroupInUnion_ptr(group_in_union_t **, GroupInUnion_ptr);
void free_GroupInUnion_ptr(group_in_union_t **);


#ifdef __cplusplus
}
#endif
#endif
