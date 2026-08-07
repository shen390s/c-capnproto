/* ctx.h - shared types and context for capnpc-c */
#ifndef CTX_H
#define CTX_H

#include "schema.capnp.h"
#include "str.h"
#include <stdbool.h>
#include <stdio.h>

#define ANNOTATION_NAMESPACE 0xf2c035025fec7c2bUL
#define ANNOTATION_FIELDGETSET 0xf72bc690355d66deUL
#define ANNOTATION_DONOTINCLUDE 0x8c99797357b357e9UL
#define ANNOTATION_TYPEDEFTO 0xcefaf27713042144UL
#define ANNOTATION_EXTRAHEADER 0xbadb496d09cf4612UL
#define ANNOTATION_EXTENDEDATTRIBUTE 0xd187bca5c6844c24UL
#define ANNOTATION_CODECGEN 0xcccaac86283e2609UL
#define ANNOTATION_MAPNAME 0xb9edf6fc2d8972b8UL
#define ANNOTATION_NAMEINFIX 0x85a8d86d736ba637UL
#define ANNOTATION_MAPLISTCOUNT 0xb6ea49eb8a9b0f9eUL
#define ANNOTATION_MAPUNIONTAG 0xdce06d41858f91acUL

struct value {
  struct Type t;
  const char *tname;
  struct str tname_buf;
  struct Value v;
  capn_ptr ptrval;
  int64_t intval;
};

struct field {
  struct Field f;
  struct value v;
  struct node *group;
};

struct node {
  struct capn_tree hdr;
  struct Node n;
  struct node *next;
  struct node *file_nodes, *next_file_node;
  struct str name;
  struct field *fields;
};

struct id_bst {
  uint64_t id;
  struct id_bst *left;
  struct id_bst *right;
};

struct string_list {
  const char *string;
  struct string_list *prev;
  struct string_list *next;
};

typedef struct {
  struct capn capn;
  struct str HDR;
  struct str SRC;
  struct capn_segment g_valseg;
  struct capn g_valcapn;
  int g_valc;
  int g_val0used, g_nullused;
  int g_fieldgetset;
  int g_codecgen;
  struct capn_tree *g_node_tree;
  CodeGeneratorRequest_ptr root;
  struct CodeGeneratorRequest req;
  struct node *file_node;
  struct node *all_files;
  struct node *all_structs;
  struct id_bst *used_import_ids;
  struct str scratch[4];
} capnp_ctx_t;

void fail(int code, char *fmt, ...);
struct node *find_node_mayfail(capnp_ctx_t *ctx, uint64_t id);
struct node *find_node(capnp_ctx_t *ctx, uint64_t id);
void insert_node(capnp_ctx_t *ctx, struct node *s);
struct id_bst *insert_id(struct id_bst *bst, uint64_t id);
bool contains_id(struct id_bst *bst, uint64_t id);
void free_id_bst(struct id_bst *bst);
struct string_list *insert_file(struct string_list *list, const char *string);
void free_string_list(struct string_list *list);
const char *get_text_annotation(Annotation_list l, unsigned long id);
const char *get_mapname(Annotation_list l);
const char *get_maplistcount(Annotation_list l);
const char *get_mapuniontag(Annotation_list l);
void decode_field(capnp_ctx_t *ctx, struct field *fields, Field_list l, int i);
int ctx_init(capnp_ctx_t *ctx, FILE *fp);
void ctx_free(capnp_ctx_t *ctx);

#endif /* CTX_H */
