/* ctx.c - shared types and context for capnpc-c */
#include "ctx.h"
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

void fail(int code, char *fmt, ...) {
  va_list ap;

  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  va_end(ap);

  exit(code);
}

struct node *find_node_mayfail(capnp_ctx_t *ctx, uint64_t id) {
  struct node *s = (struct node *)ctx->g_node_tree;
  while (s && s->n.id != id) {
    s = (struct node *)s->hdr.link[s->n.id < id];
  }
  return s;
}

struct node *find_node(capnp_ctx_t *ctx, uint64_t id) {
  struct node *s = find_node_mayfail(ctx, id);
  if (s == NULL) {
    fail(2, "cant find node with id 0x%x%x\n", (uint32_t)(id >> 32),
         (uint32_t)id);
  }
  return s;
}

void insert_node(capnp_ctx_t *ctx, struct node *s) {
  struct capn_tree **x = &(ctx->g_node_tree);
  while (*x) {
    s->hdr.parent = *x;
    x = &(*x)->link[((struct node *)*x)->n.id < s->n.id];
  }
  *x = &s->hdr;
  ctx->g_node_tree = capn_tree_insert(ctx->g_node_tree, &s->hdr);
}

struct id_bst *insert_id(struct id_bst *bst, uint64_t id) {
  struct id_bst **current = &bst;

  while (*current) {
    if (id > (*current)->id) {
      current = &(*current)->right;
    } else if (id < (*current)->id) {
      current = &(*current)->left;
    } else {
      return bst;
    }
  }

  *current = malloc(sizeof **current);
  (*current)->id = id;
  (*current)->left = NULL;
  (*current)->right = NULL;

  return bst;
}

bool contains_id(struct id_bst *bst, uint64_t id) {
  struct id_bst *current = bst;

  while (current) {
    if (id == current->id) {
      return true;
    } else if (id < current->id) {
      current = current->left;
    } else {
      current = current->right;
    }
  }

  return false;
}

void free_id_bst(struct id_bst *bst) {
  if (bst) {
    free_id_bst(bst->left);
    free_id_bst(bst->right);
    free(bst);
  }
}

struct string_list *insert_file(struct string_list *list, const char *string) {
  struct string_list **current = &list;
  struct string_list **prev = NULL;

  while (*current) {
    prev = current;
    current = &(*current)->next;
  }

  *current = malloc(sizeof **current);
  (*current)->string = string;
  (*current)->prev = prev == NULL ? NULL : *prev;
  (*current)->next = NULL;

  return list;
}

void free_string_list(struct string_list *list) {
  if (list) {
    free_string_list(list->next);
    free(list);
  }
}

const char *get_text_annotation(Annotation_list l, unsigned long id) {
  int i;

  for (i = capn_len(l) - 1; i >= 0; i--) {
    struct Annotation a;
    struct Value v;

    get_Annotation(&a, l, i);
    read_Value(&v, a.value);

    if (a.id == id) {
      if (v.which != Value_text) {
        fail(2, "annotation is not string");
      }

      return v.text.str;
    }
  }

  return NULL;
}

const char *get_mapname(Annotation_list l) {
  return get_text_annotation(l, ANNOTATION_MAPNAME);
}

const char *get_maplistcount(Annotation_list l) {
  return get_text_annotation(l, ANNOTATION_MAPLISTCOUNT);
}

const char *get_mapuniontag(Annotation_list l) {
  return get_text_annotation(l, ANNOTATION_MAPUNIONTAG);
}

void decode_field(capnp_ctx_t *ctx, struct field *fields, Field_list l,
                  int i) {
  struct field f;
  memset(&f, 0, sizeof(f));
  get_Field(&f.f, l, i);

  if (f.f.codeOrder >= capn_len(l)) {
    fail(3, "unexpectedly large code order %d >= %d\n", f.f.codeOrder,
         capn_len(l));
  }

  if (f.f.which == Field_group) {
    f.group = find_node(ctx, f.f.group.typeId);
  }

  memcpy(&fields[f.f.codeOrder], &f, sizeof(f));
}

int ctx_init(capnp_ctx_t *ctx, FILE *fp) {
  struct capn_segment *current_seg = NULL;
  int total_len = 0;
  int i;
  struct node *n;

  memset(ctx, 0x0, sizeof(*ctx));
  for (i = 0; i < 4; i++)
    str_init(&ctx->scratch[i], 0);
  if (capn_init_fp(&(ctx->capn), fp, 0) < 0) {
    return -1;
  }

  current_seg = ctx->capn.seglist;
  while (current_seg != NULL) {
    total_len += current_seg->len;
    current_seg = current_seg->next;
  }

  ctx->g_valseg.data = calloc(1, total_len);
  ctx->g_valseg.cap = total_len;

  ctx->root.p = capn_getp(capn_root(&(ctx->capn)), 0, 1);
  read_CodeGeneratorRequest(&(ctx->req), ctx->root);

  for (i = 0; i < capn_len(ctx->req.nodes); i++) {
    n = calloc(1, sizeof(*n));
    get_Node(&n->n, ctx->req.nodes, i);
    insert_node(ctx, n);

    switch (n->n.which) {
    case Node_file:
      n->next = ctx->all_files;
      ctx->all_files = n;
      break;

    case Node__struct:
      n->next = ctx->all_structs;
      ctx->all_structs = n;
      break;

    default:
      break;
    }
  }

  for (n = ctx->all_structs; n != NULL; n = n->next) {
    int j;

    n->fields = calloc(capn_len(n->n._struct.fields), sizeof(n->fields[0]));
    for (j = 0; j < capn_len(n->n._struct.fields); j++) {
      decode_field(ctx, n->fields, n->n._struct.fields, j);
    }
  }
  return 0;
}


static void free_all_nodes(struct capn_tree *t) {
  if (t == NULL)
    return;
  free_all_nodes(t->link[0]);
  free_all_nodes(t->link[1]);
  struct node *n = (struct node *)t;
  str_release(&n->name);
  free(n->fields);
  free(n);
}

void ctx_free(capnp_ctx_t *ctx) {
  int i;

  /* Free node tree (nodes, their names, and field arrays) */
  free_all_nodes(ctx->g_node_tree);

  /* Free capnp session (segments allocated by capn_init_fp) */
  capn_free(&ctx->capn);

  /* Free value segment data */
  free(ctx->g_valseg.data);

  /* Free string buffers */
  str_release(&ctx->HDR);
  str_release(&ctx->SRC);
  for (i = 0; i < 4; i++)
    str_release(&ctx->scratch[i]);
}
