/* capnpc-c.c
 *
 * Copyright (C) 2013 James McKaskill
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */
#define _POSIX_C_SOURCE 200809L

#include "schema.capnp.h"
#include "str.h"
#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif

#if defined(__linux)
#define _fileno fileno
#endif

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
} capnp_ctx_t;

static void fail(int code, char *fmt, ...) {
  va_list ap;

  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  va_end(ap);

  exit(code);
}

static struct node *find_node_mayfail(capnp_ctx_t *ctx, uint64_t id) {
  struct node *s = (struct node *)ctx->g_node_tree;
  while (s && s->n.id != id) {
    s = (struct node *)s->hdr.link[s->n.id < id];
  }
  return s;
}

static struct node *find_node(capnp_ctx_t *ctx, uint64_t id) {
  struct node *s = find_node_mayfail(ctx, id);
  if (s == NULL) {
    fail(2, "cant find node with id 0x%x%x\n", (uint32_t)(id >> 32),
         (uint32_t)id);
  }
  return s;
}

static void insert_node(capnp_ctx_t *ctx, struct node *s) {
  struct capn_tree **x = &(ctx->g_node_tree);
  while (*x) {
    s->hdr.parent = *x;
    x = &(*x)->link[((struct node *)*x)->n.id < s->n.id];
  }
  *x = &s->hdr;
  ctx->g_node_tree = capn_tree_insert(ctx->g_node_tree, &s->hdr);
}

/* id_bst implementation */

static struct id_bst *insert_id(struct id_bst *bst, uint64_t id) {
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

static bool contains_id(struct id_bst *bst, uint64_t id) {
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

static void free_id_bst(struct id_bst *bst) {
  if (bst) {
    free_id_bst(bst->left);
    free_id_bst(bst->right);
    free(bst);
  }
}

/* string_list implementation */

static struct string_list *insert_file(struct string_list *list,
                                       const char *string) {
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

static void free_string_list(struct string_list *list) {
  if (list) {
    free_string_list(list->next);
    free(list);
  }
}

static const char *get_text_annotation(Annotation_list l, unsigned long id) {
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

static const char *get_mapname(Annotation_list l) {
  return get_text_annotation(l, ANNOTATION_MAPNAME);
}

static const char *get_maplistcount(Annotation_list l) {
  return get_text_annotation(l, ANNOTATION_MAPLISTCOUNT);
}

static const char *get_mapuniontag(Annotation_list l) {
  return get_text_annotation(l, ANNOTATION_MAPUNIONTAG);
}

/* resolve_names recursively follows the nestedNodes tree in order to
 * set node->name.
 * It also builds up the list of nodes within a file (file_nodes and
 * next_file_node). */
static void resolve_names(capnp_ctx_t *ctx, struct str *b, struct node *n,
                          capn_text name, struct node *file,
                          const char *namespace) {
  int i, sz = b->len;
  str_add(b, namespace, -1);
  str_add(b, name.str, name.len);
  str_add(&n->name, b->str, b->len);
  str_add(b, "_", 1);

  for (i = capn_len(n->n.nestedNodes) - 1; i >= 0; i--) {
    struct Node_NestedNode nest;
    get_Node_NestedNode(&nest, n->n.nestedNodes, i);
    struct node *nn = find_node(ctx, nest.id);
    if (nn != NULL) {
      resolve_names(ctx, b, nn, nest.name, file, namespace);
    }
  }

  if (n->n.which == Node__struct) {
    for (i = capn_len(n->n._struct.fields) - 1; i >= 0; i--) {
      if (n->fields[i].group) {
        resolve_names(ctx, b, n->fields[i].group, n->fields[i].f.name, file,
                      namespace);
      }
    }
  }

  if (n->n.which != Node__struct || !n->n._struct.isGroup) {
    n->next_file_node = file->file_nodes;
    file->file_nodes = n;
  }

  str_setlen(b, sz);
}

static void define_enum(capnp_ctx_t *ctx, struct node *n) {
  int i;

  str_addf(&(ctx->HDR), "\nenum %s {", n->name.str);
  for (i = 0; i < capn_len(n->n._enum.enumerants); i++) {
    struct Enumerant e;
    get_Enumerant(&e, n->n._enum.enumerants, i);
    if (i) {
      str_addf(&(ctx->HDR), ",");
    }
    str_addf(&(ctx->HDR), "\n\t%s_%s = %d", n->name.str, e.name.str, i);
  }
  str_addf(&(ctx->HDR), "\n};\n");

  for (i = capn_len(n->n.annotations) - 1; i >= 0; i--) {
    struct Annotation a;
    struct Value v;
    get_Annotation(&a, n->n.annotations, i);
    read_Value(&v, a.value);

    switch (a.id) {
    case ANNOTATION_TYPEDEFTO:
      if (v.which != Value_text) {
        fail(2, "schema breakage on $C::typedefto annotation\n");
      }

      str_addf(&(ctx->HDR), "\ntypedef enum %s %s;\n", n->name.str, v.text.str);
      break;
    }
  }
}

static void decode_value(capnp_ctx_t *ctx, struct value *v, Type_ptr type,
                         Value_ptr value, const char *symbol) {
  struct Type list_type;
  memset(v, 0, sizeof(*v));
  read_Type(&v->t, type);
  read_Value(&v->v, value);

  switch (v->t.which) {
  case Type__void:
    v->tname = "void";
    break;
  case Type__bool:
    v->tname = "unsigned";
    break;
  case Type_int8:
    v->tname = "int8_t";
    break;
  case Type_int16:
    v->tname = "int16_t";
    break;
  case Type_int32:
    v->tname = "int32_t";
    break;
  case Type_int64:
    v->tname = "int64_t";
    break;
  case Type_uint8:
    v->tname = "uint8_t";
    break;
  case Type_uint16:
    v->tname = "uint16_t";
    break;
  case Type_uint32:
    v->tname = "uint32_t";
    break;
  case Type_uint64:
    v->tname = "uint64_t";
    break;
  case Type_float32:
    v->tname = "float";
    break;
  case Type_float64:
    v->tname = "double";
    break;
  case Type_text:
    v->tname = "capn_text";
    break;
  case Type_data:
    v->tname = "capn_data";
    break;
  case Type__enum:
    v->tname = strf(&v->tname_buf, "enum %s",
                    find_node(ctx, v->t._enum.typeId)->name.str);
    break;
  case Type__struct:
  case Type__interface:
    v->tname = strf(&v->tname_buf, "%s_ptr",
                    find_node(ctx, v->t._struct.typeId)->name.str);
    break;
  case Type_anyPointer:
    v->tname = "capn_ptr";
    break;
  case Type__list:
    read_Type(&list_type, v->t._list.elementType);

    switch (list_type.which) {
    case Type__void:
      v->tname = "capn_ptr";
      break;
    case Type__bool:
      v->tname = "capn_list1";
      break;
    case Type_int8:
    case Type_uint8:
      v->tname = "capn_list8";
      break;
    case Type_int16:
    case Type_uint16:
    case Type__enum:
      v->tname = "capn_list16";
      break;
    case Type_int32:
    case Type_uint32:
    case Type_float32:
      v->tname = "capn_list32";
      break;
    case Type_int64:
    case Type_uint64:
    case Type_float64:
      v->tname = "capn_list64";
      break;
    case Type_text:
    case Type_data:
    case Type_anyPointer:
    case Type__list:
      v->tname = "capn_ptr";
      break;
    case Type__struct:
    case Type__interface:
      v->tname = strf(&v->tname_buf, "%s_list",
                      find_node(ctx, list_type._struct.typeId)->name.str);
      break;
    }
  }

  switch (v->v.which) {
  case Value__bool:
    v->intval = v->v._bool;
    break;
  case Value_int8:
  case Value_uint8:
    v->intval = v->v.int8;
    break;
  case Value_int16:
  case Value_uint16:
    v->intval = v->v.int16;
    break;
  case Value__enum:
    v->intval = v->v._enum;
    break;
  case Value_int32:
  case Value_uint32:
  case Value_float32:
    v->intval = v->v.int32;
    break;
  case Value_int64:
  case Value_float64:
  case Value_uint64:
    v->intval = v->v.int64;
    break;
  case Value_text:
    if (v->v.text.len) {
      capn_ptr p = capn_root(&(ctx->g_valcapn));
      if (capn_set_text(p, 0, v->v.text)) {
        fail(2, "fail to copy text\n");
      }
      p = capn_getp(p, 0, 1);
      if (!p.type)
        break;

      v->ptrval = p;

      bool symbol_provided = symbol;
      if (!symbol) {
        static struct str buf = STR_INIT;
        v->intval = ++(ctx->g_valc);
        symbol = strf(&buf, "capn_val%d", (int)v->intval);
      }

      str_addf(&(ctx->SRC),
               "%scapn_text %s = {%d,(char*)&capn_buf[%d],(struct "
               "capn_segment*)&capn_seg};\n",
               symbol_provided ? "" : "static ", symbol, p.len - 1,
               (int)(p.data - p.seg->data - 8));
    }
    break;

  case Value_data:
  case Value__struct:
  case Value_anyPointer:
  case Value__list:
    if (v->v.anyPointer.type) {
      capn_ptr p = capn_root(&(ctx->g_valcapn));
      if (capn_setp(p, 0, v->v.anyPointer)) {
        fail(2, "failed to copy object\n");
      }
      p = capn_getp(p, 0, 1);
      if (!p.type)
        break;

      v->ptrval = p;

      bool symbol_provided = symbol;
      if (!symbol) {
        static struct str buf = STR_INIT;
        v->intval = ++(ctx->g_valc);
        symbol = strf(&buf, "capn_val%d", (int)v->intval);
      }

      str_addf(&(ctx->SRC), "%s%s %s = {", symbol_provided ? "" : "static ",
               v->tname, symbol);
      if (strcmp(v->tname, "capn_ptr"))
        str_addf(&(ctx->SRC), "{");

      str_addf(&(ctx->SRC),
               "%d,%d,%d,%d,%d,%d,%d,(char*)&capn_buf[%d],(struct "
               "capn_segment*)&capn_seg",
               p.type, p.has_ptr_tag, p.is_list_member, p.is_composite_list,
               p.datasz, p.ptrs, p.len, (int)(p.data - p.seg->data - 8));

      if (strcmp(v->tname, "capn_ptr"))
        str_addf(&(ctx->SRC), "}");

      str_addf(&(ctx->SRC), "};\n");
    }
    break;

  case Value__interface:
  case Value__void:
    break;
  }
}

static void define_const(capnp_ctx_t *ctx, struct node *n) {
  struct value v;
  decode_value(ctx, &v, n->n._const.type, n->n._const.value, n->name.str);

  switch (v.v.which) {
  case Value__bool:
  case Value_int8:
  case Value_int16:
  case Value_int32:
    str_addf(&(ctx->HDR), "extern %s %s;\n", v.tname, n->name.str);
    str_addf(&(ctx->SRC), "%s %s = %d;\n", v.tname, n->name.str, (int)v.intval);
    break;

  case Value_uint8:
    str_addf(&(ctx->HDR), "extern %s %s;\n", v.tname, n->name.str);
    str_addf(&(ctx->SRC), "%s %s = %u;\n", v.tname, n->name.str,
             (uint8_t)v.intval);
    break;

  case Value_uint16:
    str_addf(&(ctx->HDR), "extern %s %s;\n", v.tname, n->name.str);
    str_addf(&(ctx->SRC), "%s %s = %u;\n", v.tname, n->name.str,
             (uint16_t)v.intval);
    break;

  case Value_uint32:
    str_addf(&(ctx->HDR), "extern %s %s;\n", v.tname, n->name.str);
    str_addf(&(ctx->SRC), "%s %s = %uu;\n", v.tname, n->name.str,
             (uint32_t)v.intval);
    break;

  case Value__enum:
    str_addf(&(ctx->HDR), "extern %s %s;\n", v.tname, n->name.str);
    str_addf(&(ctx->SRC), "%s %s = (%s) %uu;\n", v.tname, n->name.str, v.tname,
             (uint32_t)v.intval);
    break;

  case Value_int64:
  case Value_uint64:
    str_addf(&(ctx->HDR), "extern %s %s;\n", v.tname, n->name.str);
    str_addf(&(ctx->SRC), "%s %s = ((uint64_t) %#xu << 32) | %#xu;\n", v.tname,
             n->name.str, (uint32_t)(v.intval >> 32), (uint32_t)v.intval);
    break;

  case Value_float32:
    str_addf(&(ctx->HDR), "extern union capn_conv_f32 %s;\n", n->name.str);
    str_addf(&(ctx->SRC), "union capn_conv_f32 %s = {%#xu};\n", n->name.str,
             (uint32_t)v.intval);
    break;

  case Value_float64:
    str_addf(&(ctx->HDR), "extern union capn_conv_f64 %s;\n", n->name.str);
    str_addf(&(ctx->SRC),
             "union capn_conv_f64 %s = {((uint64_t) %#xu << 32) | %#xu};\n",
             n->name.str, (uint32_t)(v.intval >> 32), (uint32_t)v.intval);
    break;

  case Value_text:
  case Value_data:
  case Value__struct:
  case Value_anyPointer:
  case Value__list:
    str_addf(&(ctx->HDR), "extern %s %s;\n", v.tname, n->name.str);
    if (!v.ptrval.type) {
      str_addf(&(ctx->SRC), "%s %s;\n", v.tname, n->name.str);
    }
    break;

  case Value__interface:
  case Value__void:
    break;
  }

  str_release(&v.tname_buf);
}

static void decode_field(capnp_ctx_t *ctx, struct field *fields, Field_list l,
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

static const char *xor_member(struct field *f) {
  static struct str buf = STR_INIT;

  if (f->v.intval) {
    switch (f->v.v.which) {
    case Value_int8:
    case Value_int16:
    case Value_int32:
      return strf(&buf, " ^ %d", (int32_t)f->v.intval);

    case Value_uint8:
      return strf(&buf, " ^ %uu", (uint8_t)f->v.intval);

    case Value_uint16:
    case Value__enum:
      return strf(&buf, " ^ %uu", (uint16_t)f->v.intval);

    case Value_uint32:
      return strf(&buf, " ^ %uu", (uint32_t)f->v.intval);

    case Value_float32:
      return strf(&buf, " ^ %#xu", (uint32_t)f->v.intval);

    case Value_int64:
      return strf(&buf, " ^ ((int64_t)((uint64_t) %#xu << 32) ^ %#xu)",
                  (uint32_t)(f->v.intval >> 32), (uint32_t)f->v.intval);
    case Value_uint64:
    case Value_float64:
      return strf(&buf, " ^ ((uint64_t) %#xu << 32) ^ %#xu",
                  (uint32_t)(f->v.intval >> 32), (uint32_t)f->v.intval);

    default:
      return "";
    }
  } else {
    return "";
  }
}

static const char *ptr_member(struct field *f, const char *var) {
  static struct str buf = STR_INIT;
  if (!strcmp(f->v.tname, "capn_ptr")) {
    return var;
  } else if (var[0] == '*') {
    return strf(&buf, "%s->p", var + 1);
  } else {
    return strf(&buf, "%s.p", var);
  }
}

static void set_member(capnp_ctx_t *ctx, struct str *func, struct field *f,
                       const char *ptr, const char *tab, const char *var) {
  const char *xor = xor_member(f);
  const char *pvar = ptr_member(f, var);

  if (f->v.t.which == Type__void)
    return;

  str_add(func, tab, -1);

  switch (f->v.t.which) {
  case Type__bool:
    str_addf(func, "capn_write1(%s, %d, %s != %d);\n", ptr, f->f.slot.offset,
             var, (int)f->v.intval);
    break;
  case Type_int8:
    str_addf(func, "capn_write8(%s, %d, (uint8_t) (%s%s));\n", ptr,
             f->f.slot.offset, var, xor);
    break;
  case Type_int16:
  case Type__enum:
    str_addf(func, "capn_write16(%s, %d, (uint16_t) (%s%s));\n", ptr,
             2 * f->f.slot.offset, var, xor);
    break;
  case Type_int32:
    str_addf(func, "capn_write32(%s, %d, (uint32_t) (%s%s));\n", ptr,
             4 * f->f.slot.offset, var, xor);
    break;
  case Type_int64:
    str_addf(func, "capn_write64(%s, %d, (uint64_t) (%s%s));\n", ptr,
             8 * f->f.slot.offset, var, xor);
    break;
  case Type_uint8:
    str_addf(func, "capn_write8(%s, %d, %s%s);\n", ptr, f->f.slot.offset, var,
             xor);
    break;
  case Type_uint16:
    str_addf(func, "capn_write16(%s, %d, %s%s);\n", ptr, 2 * f->f.slot.offset,
             var, xor);
    break;
  case Type_uint32:
    str_addf(func, "capn_write32(%s, %d, %s%s);\n", ptr, 4 * f->f.slot.offset,
             var, xor);
    break;
  case Type_float32:
    str_addf(func, "capn_write32(%s, %d, capn_from_f32(%s)%s);\n", ptr,
             4 * f->f.slot.offset, var, xor);
    break;
  case Type_uint64:
    str_addf(func, "capn_write64(%s, %d, %s%s);\n", ptr, 8 * f->f.slot.offset,
             var, xor);
    break;
  case Type_float64:
    str_addf(func, "capn_write64(%s, %d, capn_from_f64(%s)%s);\n", ptr,
             8 * f->f.slot.offset, var, xor);
    break;
  case Type_text:
    if (f->v.ptrval.type) {
      ctx->g_val0used = 1;
      str_addf(func,
               "capn_set_text(%s, %d, (%s.str != capn_val%d.str) ? %s : "
               "capn_val0);\n",
               ptr, f->f.slot.offset, var, (int)f->v.intval, var);
    } else {
      str_addf(func, "capn_set_text(%s, %d, %s);\n", ptr, f->f.slot.offset,
               var);
    }
    break;
  case Type_data:
  case Type__struct:
  case Type__interface:
  case Type__list:
  case Type_anyPointer:
    if (!f->v.intval) {
      str_addf(func, "capn_setp(%s, %d, %s);\n", ptr, f->f.slot.offset, pvar);
    } else if (!strcmp(f->v.tname, "capn_ptr")) {
      ctx->g_nullused = 1;
      str_addf(
          func,
          "capn_setp(%s, %d, (%s.data != capn_val%d.data) ? %s : capn_null);\n",
          ptr, f->f.slot.offset, pvar, (int)f->v.intval, pvar);
    } else {
      ctx->g_nullused = 1;
      str_addf(func,
               "capn_setp(%s, %d, (%s.data != capn_val%d.p.data) ? %s : "
               "capn_null);\n",
               ptr, f->f.slot.offset, pvar, (int)f->v.intval, pvar);
    }
    break;
  default:
    break;
  }
}

static void get_member(capnp_ctx_t *ctx, struct str *func, struct field *f,
                       const char *ptr, const char *tab, const char *var) {
  const char *xor = xor_member(f);
  const char *pvar = ptr_member(f, var);

  if (f->v.t.which == Type__void)
    return;

  str_add(func, tab, -1);

  switch (f->v.t.which) {
  case Type__bool:
    str_addf(func, "%s = (capn_read8(%s, %d) & %d) != %d;\n", var, ptr,
             f->f.slot.offset / 8, 1 << (f->f.slot.offset % 8),
             ((int)f->v.intval) << (f->f.slot.offset % 8));
    return;
  case Type_int8:
    str_addf(func, "%s = (int8_t) ((int8_t)capn_read8(%s, %d))%s;\n", var, ptr,
             f->f.slot.offset, xor);
    return;
  case Type_int16:
    str_addf(func, "%s = (int16_t) ((int16_t)capn_read16(%s, %d))%s;\n", var,
             ptr, 2 * f->f.slot.offset, xor);
    return;
  case Type_int32:
    str_addf(func, "%s = (int32_t) ((int32_t)capn_read32(%s, %d))%s;\n", var,
             ptr, 4 * f->f.slot.offset, xor);
    return;
  case Type_int64:
    str_addf(func, "%s = (int64_t) ((int64_t)(capn_read64(%s, %d))%s);\n", var,
             ptr, 8 * f->f.slot.offset, xor);
    return;
  case Type_uint8:
    str_addf(func, "%s = capn_read8(%s, %d)%s;\n", var, ptr, f->f.slot.offset,
             xor);
    return;
  case Type_uint16:
    str_addf(func, "%s = capn_read16(%s, %d)%s;\n", var, ptr,
             2 * f->f.slot.offset, xor);
    return;
  case Type_uint32:
    str_addf(func, "%s = capn_read32(%s, %d)%s;\n", var, ptr,
             4 * f->f.slot.offset, xor);
    return;
  case Type_uint64:
    str_addf(func, "%s = capn_read64(%s, %d)%s;\n", var, ptr,
             8 * f->f.slot.offset, xor);
    return;
  case Type_float32:
    str_addf(func, "%s = capn_to_f32(capn_read32(%s, %d)%s);\n", var, ptr,
             4 * f->f.slot.offset, xor);
    return;
  case Type_float64:
    str_addf(func, "%s = capn_to_f64(capn_read64(%s, %d)%s);\n", var, ptr,
             8 * f->f.slot.offset, xor);
    return;
  case Type__enum:
    str_addf(func, "%s = (%s)(int) capn_read16(%s, %d)%s;\n", var, f->v.tname,
             ptr, 2 * f->f.slot.offset, xor);
    return;
  case Type_text:
    if (!f->v.intval)
      ctx->g_val0used = 1;
    str_addf(func, "%s = capn_get_text(%s, %d, capn_val%d);\n", var, ptr,
             f->f.slot.offset, (int)f->v.intval);
    return;

  case Type_data:
    str_addf(func, "%s = capn_get_data(%s, %d);\n", var, ptr, f->f.slot.offset);
    break;
  case Type__struct:
  case Type__interface:
  case Type_anyPointer:
  case Type__list:
    str_addf(func, "%s = capn_getp(%s, %d, 0);\n", pvar, ptr, f->f.slot.offset);
    break;
  default:
    return;
  }

  if (f->v.intval) {
    str_addf(func, "%sif (!%s.type) {\n", tab, pvar);
    str_addf(func, "%s\t%s = capn_val%d;\n", tab, var, (int)f->v.intval);
    str_addf(func, "%s}\n", tab);
  }
}

static void mk_simple_list_encoder(struct str *func, const char *tab,
                                   const char *list_type, const char *setf,
                                   const char *dvar, const char *cvar,
                                   const char *svar) {
  str_add(func, tab, -1);
  str_addf(func, "if (1) {\n");
  str_add(func, tab, -1);
  str_addf(func, "\tint i_;\n");
  if (strcmp(list_type, "text") == 0) {
    str_add(func, tab, -1);
    str_addf(func, "\td->%s = capn_new_ptr_list(cs, s->%s);\n", dvar, cvar);
    str_add(func, tab, -1);
    str_addf(func, "\tfor(i_ = 0; i_ < s->%s; i_ ++) {\n", cvar);
    str_add(func, tab, -1);
    str_addf(func,
             "\t\tcapn_text text_ = {.str = s->%s[i_], .len = "
             "strlen(s->%s[i_]),.seg "
             "= NULL};\n",
             svar, svar);
    str_add(func, tab, -1);
    str_addf(func, "\t\tcapn_set_text(d->%s, i_, text_);\n", dvar);
    str_add(func, tab, -1);
    str_addf(func, "\t}\n");
  } else {
    str_add(func, tab, -1);
    str_addf(func, "\td->%s = capn_new_%s(cs, s->%s);\n", dvar, list_type,
             cvar);
    str_add(func, tab, -1);
    str_addf(func, "\tfor(i_ = 0; i_ < s->%s; i_ ++) {\n", cvar);
    str_add(func, tab, -1);
    str_addf(func, "\t\tcapn_%s(d->%s, i_, s->%s[i_]);\n", setf, dvar, svar);
    str_add(func, tab, -1);
    str_addf(func, "\t}\n");
  }
  str_add(func, tab, -1);
  str_addf(func, "}\n");
}

static void mk_simple_list_decoder(struct str *func, const char *tab,
                                   const char *list_type, const char *getf,
                                   const char *dvar, const char *cvar,
                                   const char *svar) {
  str_add(func, tab, -1);
  str_addf(func, "if (1) {\n");
  str_add(func, tab, -1);
  str_addf(func, "\tint i_, nc_;\n");
  if (strcmp(list_type, "text") == 0) {
    str_add(func, tab, -1);
    str_addf(func, "\tcapn_resolve(&(s->%s));\n", svar);
    str_add(func, tab, -1);
    str_addf(func, "\tnc_ = s->%s.len;\n", svar);
    str_add(func, tab, -1);
    str_addf(func, "\td->%s = (char **)calloc(nc_, sizeof(char *));\n", dvar);
    str_add(func, tab, -1);
    str_addf(func, "\tfor(i_ = 0; i_ < nc_; i_ ++) {\n");
    str_add(func, tab, -1);
    str_addf(func,
             "\t\tcapn_text text_ = capn_get_text(s->%s, i_, capn_val0);\n",
             svar);
    str_add(func, tab, -1);
    str_addf(func, "\t\td->%s[i_] = strdup(text_.str);\n", dvar);
    str_add(func, tab, -1);
    str_addf(func, "\t}\n");
  } else {
    str_add(func, tab, -1);
    str_addf(func, "\tcapn_resolve(&(s->%s.p));\n", svar);
    str_add(func, tab, -1);
    str_addf(func, "\tnc_ = s->%s.p.len;\n", svar);

    str_add(func, tab, -1);
    str_addf(func, "\td->%s = (%s *)calloc(nc_, sizeof(%s));\n", dvar,
             list_type, list_type);
    str_add(func, tab, -1);
    str_addf(func, "\tfor(i_ = 0; i_ < nc_; i_ ++) {\n");
    str_add(func, tab, -1);
    str_addf(func, "\t\td->%s[i_] = capn_%s(s->%s, i_);\n", dvar, getf, svar);
    str_add(func, tab, -1);
    str_addf(func, "\t}\n");
  }

  str_add(func, tab, -1);
  str_addf(func, "d->%s = nc_;\n", cvar);
  str_add(func, tab, -1);
  str_addf(func, "}\n");
}

static void mk_simple_list_free(struct str *func, const char *tab,
                                const char *list_type, const char *getf,
                                const char *dvar, const char *cvar,
                                const char *svar) {
  str_add(func, tab, -1);
  str_addf(func, "if (1) {\n");
  str_add(func, tab, -1);
  str_addf(func, "\tint i_, nc_ = d->%s;\n", cvar);
  str_add(func, tab, -1);
  str_addf(func, "\tcapnp_use(i_);capnp_use(nc_);\n");
  if (strcmp(list_type, "text") == 0) {
    str_add(func, tab, -1);
    str_addf(func, "\tfor(i_ = 0; i_ < nc_; i_ ++) {\n");
    str_add(func, tab, -1);
    str_addf(func, "\t\tif (d->%s[i_] == NULL) continue;\n", dvar);
    str_add(func, tab, -1);
    str_addf(func, "\t\tfree(d->%s[i_]);\n", dvar);
    str_add(func, tab, -1);
    str_addf(func, "\t}\n");
  }

  str_add(func, tab, -1);
  str_addf(func, "\tfree(d->%s);\n", dvar);
  str_add(func, tab, -1);
  str_addf(func, "}\n");
}

static void gen_call_list_encoder(capnp_ctx_t *ctx, struct str *func,
                                  struct Type *type, const char *tab,
                                  const char *var, const char *countvar,
                                  const char *var2) {
  struct node *n = NULL;

  str_add(func, tab, -1);

  switch (type->which) {
  case Type__bool:
    mk_simple_list_encoder(func, tab, "list1", "set1", var, countvar, var2);
    break;
  case Type_int8:
  case Type_uint8:
    mk_simple_list_encoder(func, tab, "list8", "set8", var, countvar, var2);
    break;
  case Type_int16:
  case Type_uint16:
    mk_simple_list_encoder(func, tab, "list16", "set16", var, countvar, var2);
    break;
  case Type_int32:
  case Type_uint32:
  case Type_float32:
    mk_simple_list_encoder(func, tab, "list32", "set32", var, countvar, var2);
    break;
  case Type_int64:
  case Type_uint64:
  case Type_float64:
    mk_simple_list_encoder(func, tab, "list64", "set64", var, countvar, var2);
    break;
  case Type_text:
    mk_simple_list_encoder(func, tab, "text", NULL, var, countvar, var2);
    break;
  case Type__struct:
    n = find_node(ctx, type->_struct.typeId);

    if (n != NULL) {
      char *dtypename = n->name.str;

      str_addf(func, "encode_%s_list(cs, &(d->%s), s->%s, s->%s);\n", dtypename,
               var, countvar, var2);
    }
    break;
  }
}

static void gen_call_list_decoder(capnp_ctx_t *ctx, struct str *func,
                                  struct Type *type, const char *tab,
                                  const char *var, const char *countvar,
                                  const char *var2) {
  char *t = NULL;
  struct node *n = NULL;

  str_add(func, tab, -1);

  switch (type->which) {
  case Type__bool:
    t = "uint8_t";
    mk_simple_list_decoder(func, tab, t, "get1", var, countvar, var2);
    break;
  case Type_int8:
  case Type_uint8:
    if (type->which == Type_int8) {
      t = "int8_t";
    } else {
      t = "uint8_t";
    }
    mk_simple_list_decoder(func, tab, t, "get8", var, countvar, var2);
    break;
  case Type_int16:
  case Type_uint16:
    if (type->which == Type_int16) {
      t = "int16_t";
    } else {
      t = "uint16_t";
    }
    mk_simple_list_decoder(func, tab, t, "get16", var, countvar, var2);
    break;
  case Type_int32:
  case Type_uint32:
  case Type_float32:
    if (type->which == Type_int32) {
      t = "int32_t";
    } else if (type->which == Type_uint32) {
      t = "uint32_t";
    } else {
      t = "float";
    }
    mk_simple_list_decoder(func, tab, t, "get32", var, countvar, var2);
    break;
  case Type_int64:
  case Type_uint64:
  case Type_float64:
    if (type->which == Type_int64) {
      t = "int64_t";
    } else if (type->which == Type_uint64) {
      t = "uint64_t";
    } else {
      t = "double";
    }
    mk_simple_list_decoder(func, tab, t, "get64", var, countvar, var2);
    break;

  case Type_text:
    mk_simple_list_decoder(func, tab, "text", NULL, var, countvar, var2);
    break;
  case Type__struct:
    n = find_node(ctx, type->_struct.typeId);
    if (n != NULL) {
      char *dtypename = n->name.str;

      str_addf(func, "decode_%s_list(&(d->%s), &(d->%s), s->%s);\n", dtypename,
               countvar, var, var2);
    }
    break;
  }
}

static void gen_call_list_free(capnp_ctx_t *ctx, struct str *func,
                               struct Type *type, const char *tab,
                               const char *var, const char *countvar,
                               const char *var2) {
  char *t = NULL;
  struct node *n = NULL;

  str_add(func, tab, -1);

  switch (type->which) {
  case Type__bool:
    t = "uint8_t";
    mk_simple_list_free(func, tab, t, "get1", var, countvar, var2);
    break;
  case Type_int8:
  case Type_uint8:
    if (type->which == Type_int8) {
      t = "int8_t";
    } else {
      t = "uint8_t";
    }
    mk_simple_list_free(func, tab, t, "get8", var, countvar, var2);
    break;
  case Type_int16:
  case Type_uint16:
    if (type->which == Type_int16) {
      t = "int16_t";
    } else {
      t = "uint16_t";
    }
    mk_simple_list_free(func, tab, t, "get16", var, countvar, var2);
    break;
  case Type_int32:
  case Type_uint32:
  case Type_float32:
    if (type->which == Type_int32) {
      t = "int32_t";
    } else if (type->which == Type_uint32) {
      t = "uint32_t";
    } else {
      t = "float";
    }
    mk_simple_list_free(func, tab, t, "get32", var, countvar, var2);
    break;
  case Type_int64:
  case Type_uint64:
  case Type_float64:
    if (type->which == Type_int64) {
      t = "int64_t";
    } else if (type->which == Type_uint64) {
      t = "uint64_t";
    } else {
      t = "double";
    }
    mk_simple_list_free(func, tab, t, "get64", var, countvar, var2);
    break;

  case Type_text:
    mk_simple_list_free(func, tab, "text", NULL, var, countvar, var2);
    break;
  case Type__struct:
    n = find_node(ctx, type->_struct.typeId);
    if (n != NULL) {
      char *dtypename = n->name.str;

      str_addf(func, "free_%s_list(d->%s, d->%s);\n", dtypename, countvar, var);
    }
    break;
  }
}

static void encode_member(capnp_ctx_t *ctx, struct str *func, struct field *f,
                          const char *tab, const char *var, const char *var2) {
  struct Type list_type;
  struct node *n = NULL;

  if (f->v.t.which == Type__void) {
    return;
  }

  if (var2 == NULL) {
    var2 = var;
  }

  switch (f->v.t.which) {
  case Type__bool:
  case Type_int8:
  case Type_int16:
  case Type_int32:
  case Type_int64:
  case Type_uint8:
  case Type_uint16:
  case Type_uint32:
  case Type_uint64:
  case Type_float32:
  case Type_float64:
  case Type__enum:
    str_add(func, tab, -1);
    str_addf(func, "d->%s = s->%s;\n", var, var2);
    break;
  case Type_text:
    str_add(func, tab, -1);
    str_addf(func, "if (s->%s != NULL) {\n", var2);
    str_add(func, tab, -1);
    str_addf(func, "\td->%s.str = s->%s;\n", var, var2);
    str_add(func, tab, -1);
    str_addf(func, "\td->%s.len = strlen(s->%s);\n", var, var2);
    str_add(func, tab, -1);
    str_addf(func, "}\n");
    str_add(func, tab, -1);
    str_addf(func, "else{\n");
    str_add(func, tab, -1);
    str_addf(func, "\td->%s.str = \"\";\n", var);
    str_add(func, tab, -1);
    str_addf(func, "\td->%s.len = 0;\n", var);
    str_add(func, tab, -1);
    str_addf(func, "}\n");
    str_add(func, tab, -1);
    str_addf(func, "d->%s.seg = NULL;\n", var);
    break;
  case Type__struct:
    n = find_node(ctx, f->v.t._struct.typeId);

    if (n != NULL) {
      str_add(func, tab, -1);
      str_addf(func, "encode_%s_ptr(cs, &(d->%s), s->%s);\n", n->name.str, var,
               var2);
    }
    break;
  case Type__list:
    read_Type(&list_type, f->v.t._list.elementType);
    if (list_type.which != Type__void) {
      char *name = NULL;
      char *ncount = NULL;
      char buf[256];

      name = (char *)get_mapname(f->f.annotations);
      if (name == NULL) {
        var2 = var;
      } else {
        var2 = name;
      }

      ncount = (char *)get_maplistcount(f->f.annotations);
      if (ncount != NULL) {
        sprintf(buf, "%s", ncount);
      } else {
        sprintf(buf, "n_%s", var2);
      }

      gen_call_list_encoder(ctx, func, &list_type, tab, var, buf, var2);
    }
    break;
  default:
    str_add(func, tab, -1);
    str_addf(func, "\t /* %s %s */\n", var, var2);
    break;
  }
}

static void decode_member(capnp_ctx_t *ctx, struct str *func, struct field *f,
                          const char *tab, const char *var, const char *var2) {
  struct Type list_type;
  struct node *n = NULL;

  if (f->v.t.which == Type__void) {
    return;
  }

  if (var2 == NULL) {
    var2 = var;
  }

  switch (f->v.t.which) {
  case Type__bool:
  case Type_int8:
  case Type_int16:
  case Type_int32:
  case Type_int64:
  case Type_uint8:
  case Type_uint16:
  case Type_uint32:
  case Type_uint64:
  case Type_float32:
  case Type_float64:
  case Type__enum:
    str_add(func, tab, -1);
    str_addf(func, "d->%s = s->%s;\n", var2, var);
    break;
  case Type_text:
    str_add(func, tab, -1);
    str_addf(func, "d->%s = strdup(s->%s.str);\n", var2, var);
    break;
  case Type__struct:
    n = find_node(ctx, f->v.t._struct.typeId);
    if (n != NULL) {
      str_add(func, tab, -1);
      str_addf(func, "decode_%s_ptr(&(d->%s), s->%s);\n", n->name.str, var2,
               var);
    }
    break;
  case Type__list:
    read_Type(&list_type, f->v.t._list.elementType);
    if (list_type.which != Type__void) {
      char *name = NULL;
      char *ncount = NULL;
      char buf[256];

      name = (char *)get_mapname(f->f.annotations);
      if (name == NULL) {
        var2 = var;
      } else {
        var2 = name;
      }

      ncount = (char *)get_maplistcount(f->f.annotations);
      if (ncount != NULL) {
        sprintf(buf, "%s", ncount);
      } else {
        char buf2[256];
        char *p;

        strcpy(buf2, var2);
        p = strchr(buf2, '.');
        if (p != NULL) {
          *p = 0x0;
          p++;
        }

        strcpy(buf, buf2);
        if (p != NULL) {
          strcat(buf, ".");
          sprintf(&buf[strlen(buf)], "n_%s", p);
        }
      }

      gen_call_list_decoder(ctx, func, &list_type, tab, var2, buf, var);
    }
    break;
  default:
    str_add(func, tab, -1);
    str_addf(func, "\t /* %s %s */\n", var2, var);
    break;
  }
}

static void free_member(capnp_ctx_t *ctx, struct str *func, struct field *f,
                        const char *tab, const char *var, const char *var2) {
  struct Type list_type;
  struct node *n = NULL;

  if (f->v.t.which == Type__void) {
    return;
  }

  if (var2 == NULL) {
    var2 = var;
  }

  switch (f->v.t.which) {
  case Type__bool:
  case Type_int8:
  case Type_int16:
  case Type_int32:
  case Type_int64:
  case Type_uint8:
  case Type_uint16:
  case Type_uint32:
  case Type_uint64:
  case Type_float32:
  case Type_float64:
  case Type__enum:
    break;
  case Type_text:
    str_add(func, tab, -1);
    str_addf(func, "if (d->%s != NULL) {\n", var2);
    str_add(func, tab, -1);
    str_addf(func, "\tfree(d->%s);\n", var2);
    str_add(func, tab, -1);
    str_addf(func, "}\n");
    break;
  case Type__struct:
    n = find_node(ctx, f->v.t._struct.typeId);
    if (n != NULL) {
      str_add(func, tab, -1);
      str_addf(func, "free_%s_ptr(&(d->%s));\n", n->name.str, var2);
    }
    break;
  case Type__list:
    read_Type(&list_type, f->v.t._list.elementType);
    if (list_type.which != Type__void) {
      char *name = NULL;
      char *ncount = NULL;
      char buf[256];

      name = (char *)get_mapname(f->f.annotations);
      if (name == NULL) {
        var2 = var;
      } else {
        var2 = name;
      }

      ncount = (char *)get_maplistcount(f->f.annotations);
      if (ncount != NULL) {
        sprintf(buf, "%s", ncount);
      } else {
        char buf2[256];
        char *p;

        strcpy(buf2, var2);
        p = strchr(buf2, '.');
        if (p != NULL) {
          *p = 0x0;
          p++;
        }

        strcpy(buf, buf2);
        if (p != NULL) {
          strcat(buf, ".");
          sprintf(&buf[strlen(buf)], "n_%s", p);
        }
      }

      gen_call_list_free(ctx, func, &list_type, tab, var2, buf, var);
    }
    break;
  default:
    str_add(func, tab, -1);
    str_addf(func, "\t /* %s %s */\n", var2, var);
    break;
  }
}

void mk_struct_list_encoder(capnp_ctx_t *ctx, struct node *n) {
  if (n == NULL) {
    return;
  }

  if (1) {
    char *mapname = (char *)get_mapname(n->n.annotations);
    char buf[256];

    if (mapname == NULL) {
      sprintf(buf, "struct %s_", n->name.str);
    } else {
      strcpy(buf, mapname);
    }

    str_addf(&(ctx->SRC),
             "void encode_%s_list(struct capn_segment *cs, %s_list *l,int "
             "count,%s **s) {\n",
             n->name.str, n->name.str, buf);
    str_addf(&(ctx->SRC), "\t%s_list lst;\n", n->name.str);
    str_addf(&(ctx->SRC), "\tint i;\n");
    str_addf(&(ctx->SRC), "\tlst = new_%s_list(cs, count);\n", n->name.str);
    str_addf(&(ctx->SRC), "\tfor(i = 0; i < count; i ++) {\n");
    str_addf(&(ctx->SRC), "\t\tstruct %s d;\n", n->name.str);
    str_addf(&(ctx->SRC), "\t\tencode_%s(cs, &d, s[i]);\n", n->name.str);
    str_addf(&(ctx->SRC), "\t\tset_%s(&d, lst, i);\n", n->name.str);
    str_addf(&(ctx->SRC), "\t}\n");
    str_addf(&(ctx->SRC), "\t(*l) = lst;\n");
    str_addf(&(ctx->SRC), "}\n");
  }
}

void mk_struct_ptr_encoder(capnp_ctx_t *ctx, struct node *n) {
  char *mapname;
  char buf[256];

  if (n == NULL) {
    return;
  }

  mapname = (char *)get_mapname(n->n.annotations);

  if (mapname == NULL) {
    sprintf(buf, "struct %s_", n->name.str);
  } else {
    strcpy(buf, mapname);
  }

  str_addf(&(ctx->SRC),
           "void encode_%s_ptr(struct capn_segment *cs, %s_ptr *p,"
           "%s *s) {\n",
           n->name.str, n->name.str, buf);
  str_addf(&(ctx->SRC), "\t%s_ptr ptr;\n", n->name.str);
  str_addf(&(ctx->SRC), "\tstruct %s d;\n", n->name.str);
  str_addf(&(ctx->SRC), "\tptr = new_%s(cs);\n", n->name.str);
  str_addf(&(ctx->SRC), "\tif (s == NULL) {\n");
  str_addf(&(ctx->SRC), "\t\tptr.p = capn_null;\n",
	  n->name.str);
  str_addf(&(ctx->SRC), "\t}\n");
  str_addf(&(ctx->SRC), "\telse{\n");
  str_addf(&(ctx->SRC), "\t\tencode_%s(cs, &d, s);\n", n->name.str);
  str_addf(&(ctx->SRC), "\t\twrite_%s(&d, ptr);\n", n->name.str);
  str_addf(&(ctx->SRC), "\t}\n");
  str_addf(&(ctx->SRC), "\t(*p) = ptr;\n");
  str_addf(&(ctx->SRC), "}\n");
  ctx->g_nullused = 1;
}

void mk_struct_list_decoder(capnp_ctx_t *ctx, struct node *n) {
  if (n == NULL) {
    return;
  }

  if (1) {
    char *mapname = (char *)get_mapname(n->n.annotations);
    char buf[256];

    if (mapname == NULL) {
      sprintf(buf, "struct %s_", n->name.str);
    } else {
      strcpy(buf, mapname);
    }

    str_addf(&(ctx->SRC),
             "void decode_%s_list(int *pcount, %s ***d, %s_list list) {\n",
             n->name.str, buf, n->name.str);
    str_addf(&(ctx->SRC), "\tint i;\n");
    str_addf(&(ctx->SRC), "\tint nc;\n");
    str_addf(&(ctx->SRC), "\t%s **ptr;\n", buf);
    str_addf(&(ctx->SRC), "\tcapn_resolve(&(list.p));\n");
    str_addf(&(ctx->SRC), "\tnc = list.p.len;\n");
    str_addf(&(ctx->SRC), "\tptr = (%s **)calloc(nc, sizeof(%s *));\n", buf,
             buf);
    str_addf(&(ctx->SRC), "\tfor(i = 0; i < nc; i ++) {\n");
    str_addf(&(ctx->SRC), "\t\tstruct %s s;\n", n->name.str);
    str_addf(&(ctx->SRC), "\t\tget_%s(&s, list, i);\n", n->name.str);
    str_addf(&(ctx->SRC), "\t\tptr[i] = (%s *)calloc(1, sizeof(%s));\n", buf,
             buf);
    str_addf(&(ctx->SRC), "\t\tdecode_%s(ptr[i], &s);\n", n->name.str);
    str_addf(&(ctx->SRC), "\t}\n");
    str_addf(&(ctx->SRC), "\t(*d) = ptr;\n");
    str_addf(&(ctx->SRC), "\t(*pcount) = nc;\n");
    str_addf(&(ctx->SRC), "}\n");
  }
}

void mk_struct_ptr_decoder(capnp_ctx_t *ctx, struct node *n) {
  char *mapname;
  char buf[256];

  if (n == NULL) {
    return;
  }

  mapname = (char *)get_mapname(n->n.annotations);

  if (mapname == NULL) {
    sprintf(buf, "struct %s_", n->name.str);
  } else {
    strcpy(buf, mapname);
  }

  str_addf(&(ctx->SRC),
           "void decode_%s_ptr(%s **d,"
           "%s_ptr p) {\n",
           n->name.str, buf, n->name.str);
  str_addf(&(ctx->SRC), "\tstruct %s s;\n", n->name.str);
  str_addf(&(ctx->SRC), "\tcapn_resolve(&(p.p));\n");
  str_addf(&(ctx->SRC), "\tif (p.p.type == CAPN_NULL) {\n",
	  n->name.str);
  str_addf(&(ctx->SRC), "\t\t(*d) = NULL;\n");
  str_addf(&(ctx->SRC), "\t\treturn;\n");
  str_addf(&(ctx->SRC), "\t}\n");
  str_addf(&(ctx->SRC), "\t*d = (%s *)calloc(1, sizeof(%s));\n", buf, buf);
  str_addf(&(ctx->SRC), "\tread_%s(&s, p);\n", n->name.str);
  str_addf(&(ctx->SRC), "\tdecode_%s(*d, &s);\n", n->name.str);
  str_addf(&(ctx->SRC), "}\n");
  ctx->g_nullused = 1;
}

void mk_struct_list_free(capnp_ctx_t *ctx, struct node *n) {
  if (n == NULL) {
    return;
  }

  if (1) {
    char *mapname = (char *)get_mapname(n->n.annotations);
    char buf[256];

    if (mapname == NULL) {
      sprintf(buf, "struct %s_", n->name.str);
    } else {
      strcpy(buf, mapname);
    }

    str_addf(&(ctx->SRC), "void free_%s_list(int pcount, %s **d) {\n",
             n->name.str, buf);
    str_addf(&(ctx->SRC), "\tint i;\n");
    str_addf(&(ctx->SRC), "\tint nc = pcount;\n");
    str_addf(&(ctx->SRC), "\t%s **ptr = d;\n", buf);
    str_addf(&(ctx->SRC), "\tif (ptr == NULL) return;\n");
    str_addf(&(ctx->SRC), "\tfor(i = 0; i < nc; i ++) {\n");
    str_addf(&(ctx->SRC), "\t\tif(ptr[i] == NULL) continue;\n");
    str_addf(&(ctx->SRC), "\t\tfree_%s(ptr[i]);\n", n->name.str);
    str_addf(&(ctx->SRC), "\t\tfree(ptr[i]);\n");
    str_addf(&(ctx->SRC), "\t}\n");
    str_addf(&(ctx->SRC), "\tfree(ptr);\n");
    str_addf(&(ctx->SRC), "}\n");
  }
}

void mk_struct_ptr_free(capnp_ctx_t *ctx, struct node *n) {
  char *mapname;
  char buf[256];

  if (n == NULL) {
    return;
  }

  mapname = (char *)get_mapname(n->n.annotations);

  if (mapname == NULL) {
    sprintf(buf, "struct %s_", n->name.str);
  } else {
    strcpy(buf, mapname);
  }

  str_addf(&(ctx->SRC), "void free_%s_ptr(%s **d){\n", n->name.str, buf);
  str_addf(&(ctx->SRC), "\tif((*d) == NULL) return;\n");
  str_addf(&(ctx->SRC), "\tfree_%s(*d);\n", n->name.str);
  str_addf(&(ctx->SRC), "\tfree(*d);\n");
  str_addf(&(ctx->SRC), "\t(*d) = NULL;\n");
  str_addf(&(ctx->SRC), "}\n");
}

struct strings {
  struct str ftab;
  struct str dtab;
  struct str get;
  struct str set;
  struct str encoder;
  struct str decoder;
  struct str freeup;
  struct str enums;
  struct str decl;
  struct str var;
  struct str pub_get;
  struct str pub_get_header;
  struct str pub_set;
  struct str pub_set_header;
};

static const char *field_name(struct field *f) {
  static struct str buf = STR_INIT;
  static const char *reserved[] = {
      /* C++11 reserved words */
      "alignas",
      "alignof",
      "and",
      "and_eq",
      "asm",
      "auto",
      "bitand",
      "bitor",
      "bool",
      "break",
      "case",
      "catch",
      "char",
      "char16_t",
      "char32_t",
      "class",
      "compl",
      "const",
      "constexpr",
      "const_cast",
      "continue",
      "decltype",
      "default",
      "delete",
      "do",
      "double",
      "dynamic_cast",
      "else",
      "enum",
      "explicit",
      "export",
      "extern",
      "false",
      "float",
      "for",
      "friend",
      "goto",
      "if",
      "inline",
      "int",
      "long",
      "mutable",
      "namespace",
      "new",
      "noexcept",
      "not",
      "not_eq",
      "nullptr",
      "operator",
      "or",
      "or_eq",
      "private",
      "protected",
      "public",
      "register",
      "reinterpret_cast",
      "return",
      "short",
      "signed",
      "sizeof",
      "static",
      "static_assert",
      "static_cast",
      "struct",
      "switch",
      "template",
      "this",
      "thread_local",
      "throw",
      "true",
      "try",
      "typedef",
      "typeid",
      "typename",
      "union",
      "unsigned",
      "using",
      "virtual",
      "void",
      "volatile",
      "wchar_t",
      "while",
      "xor",
      "xor_eq",
      /* COM reserved words */
      "interface",
      "module",
      "import",
      /* capn reserved otherwise Value_ptr enum and type collide */
      "ptr",
      "list",
      /* C11 keywords not reserved in C++ */
      "restrict",
      "_Alignas",
      "_Alignof",
      "_Atomic",
      "_Bool",
      "_Complex",
      "_Generic",
      "_Imaginary",
      "_Noreturn",
      "_Static_assert",
      "_Thread_local",
      /* capn reserved for parameter names */
      "p",
  };

  size_t i;
  const char *s = f->f.name.str;
  for (i = 0; i < sizeof(reserved) / sizeof(reserved[0]); i++) {
    if (!strcmp(s, reserved[i])) {
      return strf(&buf, "_%s", s);
    }
  }

  return s;
}

static void union_block(capnp_ctx_t *ctx, struct strings *s, struct field *f,
                        const char *u1, const char *u2) {
  static struct str buf = STR_INIT;
  str_add(&s->ftab, "\t", -1);
  set_member(ctx, &s->set, f, "p.p", s->ftab.str,
             strf(&buf, "%s%s", s->var.str, field_name(f)));
  get_member(ctx, &s->get, f, "p.p", s->ftab.str,
             strf(&buf, "%s%s", s->var.str, field_name(f)));
  str_addf(&s->set, "%sbreak;\n", s->ftab.str);
  str_addf(&s->get, "%sbreak;\n", s->ftab.str);
  if (ctx->g_codecgen) {
    char var1[256];
    char var2[256];
    char *mapname = (char *)get_mapname(f->f.annotations);

    if (u2 == NULL) {
      u2 = u1;
    }

    if (mapname == NULL) {
      mapname = (char *)field_name(f);
    }

    if (u1 != NULL) {
      sprintf(var1, "%s.%s", u1, field_name(f));
      sprintf(var2, "%s.%s", u2, mapname);
    } else {
      strcpy(var1, field_name(f));
      strcpy(var2, mapname);
    }

    encode_member(ctx, &s->encoder, f, s->ftab.str, var1, var2);
    str_addf(&s->encoder, "%sbreak;\n", s->ftab.str);
    decode_member(ctx, &s->decoder, f, s->ftab.str, var1, var2);
    str_addf(&s->decoder, "%sbreak;\n", s->ftab.str);
    free_member(ctx, &s->freeup, f, s->ftab.str, var1, var2);
    str_addf(&s->freeup, "%sbreak;\n", s->ftab.str);
  }
  str_setlen(&s->ftab, s->ftab.len - 1);
}

static int in_union(struct field *f) {
  return f->f.discriminantValue != 0xFFFF;
}

static void union_cases(capnp_ctx_t *ctx, struct strings *s, struct node *n,
                        struct field *first_field, int mask) {
  struct field *f, *u = NULL;

  for (f = first_field;
       f < n->fields + capn_len(n->n._struct.fields) && in_union(f); f++) {

    if (f->f.which != Field_slot)
      continue;
    if (f->v.ptrval.type || f->v.intval)
      continue;
    if ((mask & (1 << f->v.t.which)) == 0)
      continue;

    u = f;
    str_addf(&s->set, "%scase %s_%s:\n", s->ftab.str, n->name.str,
             field_name(f));
    str_addf(&s->get, "%scase %s_%s:\n", s->ftab.str, n->name.str,
             field_name(f));
    if (ctx->g_codecgen) {
      str_addf(&s->encoder, "%scase %s_%s:\n", s->ftab.str, n->name.str,
               field_name(f));
      str_addf(&s->decoder, "%scase %s_%s:\n", s->ftab.str, n->name.str,
               field_name(f));
      str_addf(&s->freeup, "%scase %s_%s:\n", s->ftab.str, n->name.str,
               field_name(f));
    }

    if (u) {
      union_block(ctx, s, u,
                  &(n->n.displayName.str[n->n.displayNamePrefixLength]), NULL);
    }
  }
}

static void declare_slot(struct strings *s, struct field *f) {
  switch (f->v.t.which) {
  case Type__void:
    break;
  case Type__bool:
    str_addf(&s->decl, "%s%s %s : 1;\n", s->dtab.str, f->v.tname,
             field_name(f));
    break;
  default:
    str_addf(&s->decl, "%s%s %s;\n", s->dtab.str, f->v.tname, field_name(f));
    break;
  }
}

static void define_group(capnp_ctx_t *ctx, struct strings *s, struct node *n,
                         const char *group_name, bool enclose_unions,
                         const char *extattr, const char *extattr_space,
                         const char *uniontag);

static void do_union(capnp_ctx_t *ctx, struct strings *s, struct node *n,
                     struct field *first_field, const char *union_name,
                     const char *extattr, const char *extattr_space,
                     const char *uniontag) {
  int tagoff = 2 * n->n._struct.discriminantOffset;
  struct field *f;
  static struct str tag = STR_INIT;
  struct str enums = STR_INIT;

  str_reset(&tag);

  if (union_name) {
    str_addf(&tag, "%.*s_which", s->var.len - 1, s->var.str);
    str_addf(&enums, "enum %s_which {", n->name.str);
    str_addf(&s->decl, "%senum %s_which %s_which;\n", s->dtab.str, n->name.str,
             union_name);
    str_addf(&s->get, "%s%s = (enum %s_which)(int) capn_read16(p.p, %d);\n",
             s->ftab.str, tag.str, n->name.str, tagoff);
  } else {
    str_addf(&tag, "%swhich", s->var.str);
    str_addf(&enums, "enum %s_which {", n->name.str);
    str_addf(&s->decl, "%senum %s_which which;\n", s->dtab.str, n->name.str);
    str_addf(&s->get, "%s%s = (enum %s_which)(int) capn_read16(p.p, %d);\n",
             s->ftab.str, tag.str, n->name.str, tagoff);
  }

  str_addf(&s->set, "%scapn_write16(p.p, %d, %s);\n", s->ftab.str, tagoff,
           tag.str);
  str_addf(&s->set, "%sswitch (%s) {\n", s->ftab.str, tag.str);
  str_addf(&s->get, "%sswitch (%s) {\n", s->ftab.str, tag.str);

  if (ctx->g_codecgen) {
    char var[256];
    char *p = strstr(tag.str, "->");

    if (p == NULL) {
      fail(2, "bad variable");
    }

    sprintf(var, "d%s", p);

    str_addf(&s->encoder, "%sswitch (%s) {\n", s->ftab.str, var);
    str_addf(&s->decoder, "%sswitch (%s) {\n", s->ftab.str, tag.str);
    str_addf(&s->freeup, "%sswitch (%s) {\n", s->ftab.str, uniontag);
  }

  /* if we have a bunch of the same C type with zero defaults, we
   * only need to emit one switch block as the layout will line up
   * in the C union */
  union_cases(ctx, s, n, first_field, (1 << Type__bool));
  union_cases(ctx, s, n, first_field, (1 << Type__enum));
  union_cases(ctx, s, n, first_field, (1 << Type_int8) | (1 << Type_uint8));
  union_cases(ctx, s, n, first_field, (1 << Type_int16) | (1 << Type_uint16));
  union_cases(ctx, s, n, first_field,
              (1 << Type_int32) | (1 << Type_uint32) | (1 << Type_float32));
  union_cases(ctx, s, n, first_field,
              (1 << Type_int64) | (1 << Type_uint64) | (1 << Type_float64));
  union_cases(ctx, s, n, first_field, (1 << Type_text));
  union_cases(ctx, s, n, first_field, (1 << Type_data));
  union_cases(ctx, s, n, first_field,
              (1 << Type__struct) | (1 << Type__interface) |
                  (1 << Type_anyPointer) | (1 << Type__list));

  str_addf(&s->decl, "%scapnp_nowarn union {\n", s->dtab.str);
  str_add(&s->dtab, "\t", -1);

  /* when we have defaults or groups we have to emit each case seperately */
  for (f = first_field;
       f < n->fields + capn_len(n->n._struct.fields) && in_union(f); f++) {
    if (f > first_field) {
      str_addf(&enums, ",");
    }

    str_addf(&enums, "\n\t%s_%s = %d", n->name.str, field_name(f),
             f->f.discriminantValue);

    switch (f->f.which) {
    case Field_group:
      str_addf(&s->get, "%scase %s_%s:\n", s->ftab.str, n->name.str,
               field_name(f));
      str_addf(&s->set, "%scase %s_%s:\n", s->ftab.str, n->name.str,
               field_name(f));
      str_add(&s->ftab, "\t", -1);
      // When we add a union inside a union, we need to enclose it in its
      // own struct so that its members do not overwrite its own
      // discriminant.
      define_group(ctx, s, f->group, field_name(f), true, extattr,
                   extattr_space, uniontag);
      str_addf(&s->get, "%sbreak;\n", s->ftab.str);
      str_addf(&s->set, "%sbreak;\n", s->ftab.str);
      if (ctx->g_codecgen) {
        str_addf(&s->encoder, "%sbreak;\n", s->ftab.str);
        str_addf(&s->decoder, "%sbreak;\n", s->ftab.str);
        str_addf(&s->freeup, "%sbreak;\n", s->ftab.str);
      }
      str_setlen(&s->ftab, s->ftab.len - 1);
      break;

    case Field_slot:
      declare_slot(s, f);
      if (f->v.ptrval.type || f->v.intval) {
        str_addf(&s->get, "%scase %s_%s:\n", s->ftab.str, n->name.str,
                 field_name(f));
        str_addf(&s->set, "%scase %s_%s:\n", s->ftab.str, n->name.str,
                 field_name(f));
        if (ctx->g_codecgen) {
          str_addf(&s->encoder, "%scase %s_%s:\n", s->ftab.str, n->name.str,
                   field_name(f));
          str_addf(&s->decoder, "%scase %s_%s:\n", s->ftab.str, n->name.str,
                   field_name(f));
          str_addf(&s->freeup, "%scase %s_%s:\n", s->ftab.str, n->name.str,
                   field_name(f));
        }
        union_block(
            ctx, s, f,
            NULL /*&(n->n.displayName.str[n->n.displayNamePrefixLength]) */,
            NULL);
      }
      break;

    default:
      break;
    }
  }

  str_setlen(&s->dtab, s->dtab.len - 1);

  if (union_name) {
    str_addf(&s->decl, "%s} %s;\n", s->dtab.str, union_name);
  } else {
    str_addf(&s->decl, "%s};\n", s->dtab.str);
  }

  str_addf(&s->get, "%sdefault:\n%s\tbreak;\n%s}\n", s->ftab.str, s->ftab.str,
           s->ftab.str);
  str_addf(&s->set, "%sdefault:\n%s\tbreak;\n%s}\n", s->ftab.str, s->ftab.str,
           s->ftab.str);

  if (ctx->g_codecgen) {
    str_addf(&s->encoder, "%sdefault:\n%s\tbreak;\n%s}\n", s->ftab.str,
             s->ftab.str, s->ftab.str);
    str_addf(&s->decoder, "%sdefault:\n%s\tbreak;\n%s}\n", s->ftab.str,
             s->ftab.str, s->ftab.str);
    str_addf(&s->freeup, "%sdefault:\n%s\tbreak;\n%s}\n", s->ftab.str,
             s->ftab.str, s->ftab.str);
  }

  str_addf(&enums, "\n};\n");
  str_add(&s->enums, enums.str, enums.len);
  str_release(&enums);
}

static void define_field(capnp_ctx_t *ctx, struct strings *s, struct field *f,
                         const char *extattr, const char *extattr_space) {
  static struct str buf = STR_INIT;

  switch (f->f.which) {
  case Field_slot:
    declare_slot(s, f);
    set_member(ctx, &s->set, f, "p.p", s->ftab.str,
               strf(&buf, "%s%s", s->var.str, field_name(f)));
    get_member(ctx, &s->get, f, "p.p", s->ftab.str,
               strf(&buf, "%s%s", s->var.str, field_name(f)));
    if (ctx->g_codecgen) {
      encode_member(ctx, &s->encoder, f, s->ftab.str, field_name(f),
                    get_mapname(f->f.annotations));
      decode_member(ctx, &s->decoder, f, s->ftab.str, field_name(f),
                    get_mapname(f->f.annotations));
      free_member(ctx, &s->freeup, f, s->ftab.str, field_name(f),
                  get_mapname(f->f.annotations));
    }
    break;

  case Field_group:
    if (ctx->g_codecgen) {
      char uniontagvar[256];

      memset(uniontagvar, 0x0, sizeof(uniontagvar));
      if (f->group != NULL) {
        int flen = capn_len(f->group->n._struct.fields);
        int ulen = f->group->n._struct.discriminantCount;

        if ((ulen == flen) && (ulen > 0)) {
          if (field_name(f) != NULL) {
            char *uniontag = (char *)get_mapuniontag(f->f.annotations);
            char buf[256];

            if (uniontag != NULL) {
              strcpy(buf, uniontag);
            } else {
              sprintf(buf, "%s_which", field_name(f));
            }

            str_addf(&s->encoder, "\td->%s_which = s->%s;\n", field_name(f),
                     buf);
            str_addf(&s->decoder, "\td->%s = s->%s_which;\n", buf,
                     field_name(f));
            sprintf(uniontagvar, "d->%s", buf);
          }
        }
      }
      define_group(ctx, s, f->group, field_name(f), false, extattr,
                   extattr_space, uniontagvar);
    } else {
      define_group(ctx, s, f->group, field_name(f), false, extattr,
                   extattr_space, NULL);
    }
    break;
  }
}

static void define_getter_functions(capnp_ctx_t *ctx, struct node *node,
                                    struct field *field, struct strings *s,
                                    const char *extattr,
                                    const char *extattr_space) {
  /**
   * define getter
   */
  str_addf(&s->pub_get_header, "\n%s%s%s %s_get_%s(%s_ptr p);\n", extattr,
           extattr_space, field->v.tname, node->name.str, field_name(field),
           node->name.str);
  str_addf(&s->pub_get, "\n%s%s%s %s_get_%s(%s_ptr p)\n", extattr,
           extattr_space, field->v.tname, node->name.str, field_name(field),
           node->name.str);
  struct str getter_body = STR_INIT;
  get_member(ctx, &getter_body, field, "p.p", "", field_name(field));
  str_addf(&s->pub_get, "{\n");
  str_addf(&s->pub_get, "%s%s %s;\n", s->ftab.str, field->v.tname,
           field_name(field));
  str_addf(&s->pub_get, "%s%s", s->ftab.str, getter_body.str);
  str_release(&getter_body);
  str_addf(&s->pub_get, "%sreturn %s;\n}\n", s->ftab.str, field_name(field));
}

static void define_setter_functions(capnp_ctx_t *ctx, struct node *node,
                                    struct field *field, struct strings *s,
                                    const char *extattr,
                                    const char *extattr_space) {
  str_addf(&s->pub_set_header, "\n%s%svoid %s_set_%s(%s_ptr p, %s %s);\n",
           extattr, extattr_space, node->name.str, field_name(field),
           node->name.str, field->v.tname, field_name(field));
  str_addf(&s->pub_set, "\n%s%svoid %s_set_%s(%s_ptr p, %s %s)\n", extattr,
           extattr_space, node->name.str, field_name(field), node->name.str,
           field->v.tname, field_name(field));
  struct str setter_body = STR_INIT;
  set_member(ctx, &setter_body, field, "p.p", s->ftab.str, field_name(field));
  str_addf(&s->pub_set, "{\n%s}\n", setter_body.str);
  str_release(&setter_body);
}

static void define_encode_function(capnp_ctx_t *ctx, struct node *node,
                                   struct strings *s, const char *extattr,
                                   const char *extattr_space) {}
static void define_group(capnp_ctx_t *ctx, struct strings *s, struct node *n,
                         const char *group_name, bool enclose_unions,
                         const char *extattr, const char *extattr_space,
                         const char *uniontag) {
  struct field *f;
  int flen = capn_len(n->n._struct.fields);
  int ulen = n->n._struct.discriminantCount;
  /* named union is where all group members are in the union */
  int named_union = (group_name && ulen == flen && ulen > 0);
  int named_struct = (group_name && !named_union);
  int empty = 1;

  for (f = n->fields; f < n->fields + flen; f++) {
    decode_value(ctx, &f->v, f->f.slot.type, f->f.slot.defaultValue, NULL);
    if (f->v.t.which != Type__void)
      empty = 0;
  }

  if (named_struct && empty) {
    str_addf(&s->decl, "%s/* struct { -empty- } %s; */\n", s->dtab.str,
             group_name);
    return;
  }

  if (named_struct) {
    str_addf(&s->decl, "%scapnp_nowarn struct {\n", s->dtab.str);
    str_add(&s->dtab, "\t", 1);
  }

  if (group_name) {
    str_addf(&s->var, "%s.", group_name);
  }

  /* fields before the union members */
  for (f = n->fields; f < n->fields + flen && !in_union(f); f++) {
    define_field(ctx, s, f, extattr, extattr_space);

    if (!ctx->g_fieldgetset) {
      continue;
    }

    if ((n->n.which == Node__struct && n->n._struct.isGroup)) {
      // Don't emit in-place getters and setters for groups because they
      // are defined as anonymous structs inside their parent struct.
      // We could do it, but nested structs shouldn't be accessed
      // in-place anyway.
      continue;
    }

    if (f->v.t.which == Type__void) {
      continue;
    }

    define_getter_functions(ctx, n, f, s, extattr, extattr_space);
    define_setter_functions(ctx, n, f, s, extattr, extattr_space);
  }

  if (ulen > 0) {
    if (enclose_unions) {
      // When we are already inside a union, so we need to enclose the union
      // with its disciminant.
      str_addf(&s->decl, "%scapnp_nowarn struct {\n", s->dtab.str);
      str_add(&s->dtab, "\t", 1);
    }

    const bool keep_union_name = named_union && !enclose_unions;

    do_union(ctx, s, n, f, keep_union_name ? group_name : NULL, extattr,
             extattr_space, uniontag);

    while (f < n->fields + flen && in_union(f))
      f++;

    /* fields after the unnamed union */
    for (; f < n->fields + flen; f++) {
      define_field(ctx, s, f, extattr, extattr_space);
    }

    if (enclose_unions) {
      str_setlen(&s->dtab, s->dtab.len - 1);
      str_addf(&s->decl, "%s} %s;\n", s->dtab.str, group_name);
    }
  }

  if (named_struct) {
    str_setlen(&s->dtab, s->dtab.len - 1);
    str_addf(&s->decl, "%s} %s;\n", s->dtab.str, group_name);
  }

  if (group_name) {
    str_setlen(&s->var, s->var.len - strlen(group_name) - 1);
  }
}

static void define_struct(capnp_ctx_t *ctx, struct node *n, const char *extattr,
                          const char *extattr_space) {
  static struct strings s;
  int i;

  str_reset(&s.dtab);
  str_reset(&s.ftab);
  str_reset(&s.get);
  str_reset(&s.set);
  str_reset(&s.encoder);
  str_reset(&s.decoder);
  str_reset(&s.freeup);
  str_reset(&s.enums);
  str_reset(&s.decl);
  str_reset(&s.var);
  str_reset(&s.pub_get);
  str_reset(&s.pub_set);
  str_reset(&s.pub_get_header);
  str_reset(&s.pub_set_header);

  str_add(&s.dtab, "\t", -1);
  str_add(&s.ftab, "\t", -1);
  str_add(&s.var, "s->", -1);

  if (ctx->g_codecgen) {
    if (n->n._struct.discriminantCount > 0) {
      const char *uniontag = get_mapuniontag(n->n.annotations);
      const char *tagname = "which";

      if (uniontag != NULL) {
        tagname = uniontag;
      }

      str_addf(&s.encoder, "\td->which = s->%s;\n", tagname);
      str_addf(&s.decoder, "\td->%s = s->which;\n", tagname);
    }
  }

  define_group(ctx, &s, n, NULL, false, extattr, extattr_space, NULL);

  str_add(&(ctx->HDR), s.enums.str, s.enums.len);

  str_addf(&(ctx->HDR), "\n%sstruct %s {\n",
           s.decl.len == 0 ? "capnp_nowarn " : "", n->name.str);
  str_add(&(ctx->HDR), s.decl.str, s.decl.len);
  str_addf(&(ctx->HDR), "};\n");

  for (i = capn_len(n->n.annotations) - 1; i >= 0; i--) {
    struct Annotation a;
    struct Value v;
    get_Annotation(&a, n->n.annotations, i);
    read_Value(&v, a.value);

    switch (a.id) {
    case ANNOTATION_TYPEDEFTO:
      if (v.which != Value_text) {
        fail(2, "schema breakage on $C::typedefto annotation\n");
      }

      str_addf(&(ctx->HDR), "\ntypedef struct %s %s;\n", n->name.str,
               v.text.str);
      break;
    }
  }

  str_addf(&(ctx->SRC), "\n%s%s%s_ptr new_%s(struct capn_segment *s) {\n",
           extattr, extattr_space, n->name.str, n->name.str);
  str_addf(&(ctx->SRC), "\t%s_ptr p;\n", n->name.str);
  str_addf(&(ctx->SRC), "\tp.p = capn_new_struct(s, %d, %d);\n",
           8 * n->n._struct.dataWordCount, n->n._struct.pointerCount);
  str_addf(&(ctx->SRC), "\treturn p;\n");
  str_addf(&(ctx->SRC), "}\n");

  // adding the ability to get the structure size
  str_addf(&(ctx->HDR), "\nstatic const size_t %s_word_count = %d;\n",
           n->name.str, n->n._struct.dataWordCount);
  str_addf(&(ctx->HDR), "\nstatic const size_t %s_pointer_count = %d;\n",
           n->name.str, n->n._struct.pointerCount);
  str_addf(&(ctx->HDR), "\nstatic const size_t %s_struct_bytes_count = %d;\n\n",
           n->name.str,
           8 * (n->n._struct.pointerCount + n->n._struct.dataWordCount));

  str_addf(&(ctx->SRC),
           "%s%s%s_list new_%s_list(struct capn_segment *s, int len) {\n",
           extattr, extattr_space, n->name.str, n->name.str);
  str_addf(&(ctx->SRC), "\t%s_list p;\n", n->name.str);
  str_addf(&(ctx->SRC), "\tp.p = capn_new_list(s, len, %d, %d);\n",
           8 * n->n._struct.dataWordCount, n->n._struct.pointerCount);
  str_addf(&(ctx->SRC), "\treturn p;\n");
  str_addf(&(ctx->SRC), "}\n");

  str_addf(&(ctx->SRC),
           "%s%svoid read_%s(struct %s *s capnp_unused, %s_ptr p) {\n", extattr,
           extattr_space, n->name.str, n->name.str, n->name.str);
  str_addf(&(ctx->SRC), "\tcapn_resolve(&p.p);\n\tcapnp_use(s);\n");
  str_add(&(ctx->SRC), s.get.str, s.get.len);
  str_addf(&(ctx->SRC), "}\n");

  str_addf(&(ctx->SRC),
           "%s%svoid write_%s(const struct %s *s capnp_unused, %s_ptr p) {\n",
           extattr, extattr_space, n->name.str, n->name.str, n->name.str);
  str_addf(&(ctx->SRC), "\tcapn_resolve(&p.p);\n\tcapnp_use(s);\n");
  str_add(&(ctx->SRC), s.set.str, s.set.len);
  str_addf(&(ctx->SRC), "}\n");

  str_addf(&(ctx->SRC), "%s%svoid get_%s(struct %s *s, %s_list l, int i) {\n",
           extattr, extattr_space, n->name.str, n->name.str, n->name.str);
  str_addf(&(ctx->SRC), "\t%s_ptr p;\n", n->name.str);
  str_addf(&(ctx->SRC), "\tp.p = capn_getp(l.p, i, 0);\n");
  str_addf(&(ctx->SRC), "\tread_%s(s, p);\n", n->name.str);
  str_addf(&(ctx->SRC), "}\n");

  str_addf(&(ctx->SRC),
           "%s%svoid set_%s(const struct %s *s, %s_list l, int i) {\n", extattr,
           extattr_space, n->name.str, n->name.str, n->name.str);
  str_addf(&(ctx->SRC), "\t%s_ptr p;\n", n->name.str);
  str_addf(&(ctx->SRC), "\tp.p = capn_getp(l.p, i, 0);\n");
  str_addf(&(ctx->SRC), "\twrite_%s(s, p);\n", n->name.str);
  str_addf(&(ctx->SRC), "}\n");

  if (ctx->g_codecgen) {
    const char *mapname = get_mapname(n->n.annotations);
    char buf[256];

    if (mapname == NULL) {
      sprintf(buf, "struct %s_", n->name.str);
    } else {
      strcpy(buf, mapname);
    }
    str_addf(
        &(ctx->SRC),
        "\nvoid encode_%s(struct capn_segment *cs,struct %s *d, %s *s) {\n",
        n->name.str, n->name.str, buf);
    str_addf(&(ctx->SRC), "%s\n", s.encoder.str);
    str_addf(&(ctx->SRC), "}\n");
    str_addf(&(ctx->SRC), "\nvoid decode_%s(%s *d, struct %s *s) {\n",
             n->name.str, buf, n->name.str);
    str_addf(&(ctx->SRC), "%s\n", s.decoder.str);
    str_addf(&(ctx->SRC), "}\n");
    str_addf(&(ctx->SRC), "\nvoid free_%s(%s *d) {\n", n->name.str, buf);
    str_addf(&(ctx->SRC), "%s\n", s.freeup.str);
    str_addf(&(ctx->SRC), "}\n");
  }

  str_add(&(ctx->SRC), s.pub_get.str, s.pub_get.len);
  str_add(&(ctx->SRC), s.pub_set.str, s.pub_set.len);

  str_add(&(ctx->HDR), s.pub_get_header.str, s.pub_get_header.len);
  str_add(&(ctx->HDR), s.pub_set_header.str, s.pub_set_header.len);
}

static void declare(capnp_ctx_t *ctx, struct node *file_node,
                    const char *format, int num) {
  struct node *n;
  str_addf(&(ctx->HDR), "\n");
  for (n = file_node->file_nodes; n != NULL; n = n->next_file_node) {
    if (n->n.which == Node__struct && !n->n._struct.isGroup) {
      switch (num) {
      case 3:
        str_addf(&(ctx->HDR), format, n->name.str, n->name.str, n->name.str);
        break;
      case 2:
        str_addf(&(ctx->HDR), format, n->name.str, n->name.str);
        break;
      case 1:
        str_addf(&(ctx->HDR), format, n->name.str);
        break;
      }
    }
  }
}

static void declare_ext(capnp_ctx_t *ctx, struct node *file_node,
                        const char *format, int num, const char *extattr,
                        const char *extattr_space) {
  struct node *n;
  str_addf(&(ctx->HDR), "\n");
  for (n = file_node->file_nodes; n != NULL; n = n->next_file_node) {
    if (n->n.which == Node__struct && !n->n._struct.isGroup) {
      switch (num) {
      case 3:
        str_addf(&(ctx->HDR), format, extattr, extattr_space, n->name.str,
                 n->name.str, n->name.str);
        break;
      case 2:
        str_addf(&(ctx->HDR), format, extattr, extattr_space, n->name.str,
                 n->name.str);
        break;
      case 1:
        str_addf(&(ctx->HDR), format, extattr, extattr_space, n->name.str);
        break;
      }
    }
  }
}

static void mk_codec_declares(capnp_ctx_t *ctx, const char *n1,
                              const char *n2) {
  str_addf(&(ctx->HDR),
           "void encode_%s(struct capn_segment *,struct %s *, %s *);\n", n1, n1,
           n2);
  str_addf(&(ctx->HDR), "void decode_%s(%s *, struct %s *);\n", n1, n2, n1);
  str_addf(&(ctx->HDR), "void free_%s(%s *);\n", n1, n2);
  str_addf(
      &(ctx->HDR),
      "void encode_%s_list(struct capn_segment *,%s_list *, int, %s **);\n", n1,
      n1, n2);
  str_addf(&(ctx->HDR), "void decode_%s_list(int *, %s ***, %s_list);\n", n1,
           n2, n1);
  str_addf(&(ctx->HDR), "void free_%s_list(int, %s **);\n", n1, n2);
  str_addf(&(ctx->HDR),
           "void encode_%s_ptr(struct capn_segment*, %s_ptr *, %s *);\n", n1,
           n1, n2);
  str_addf(&(ctx->HDR), "void decode_%s_ptr(%s **, %s_ptr);\n", n1, n2, n1);
  str_addf(&(ctx->HDR), "void free_%s_ptr(%s **);\n", n1, n2);
}
static void declare_codec(capnp_ctx_t *ctx, struct node *file_node) {
  struct node *n;
  str_addf(&(ctx->HDR), "\n");
  for (n = file_node->file_nodes; n != NULL; n = n->next_file_node) {
    if (n->n.which == Node__struct && !n->n._struct.isGroup) {
      const char *mapname = get_mapname(n->n.annotations);

      if (mapname == NULL) {
        mk_codec_declares(ctx, n->name.str, n->name.str);
      } else {
        mk_codec_declares(ctx, n->name.str, mapname);
      }
      str_addf(&(ctx->HDR), "\n");
    }
  }
}
int ctx_init(capnp_ctx_t *ctx, FILE *fp) {
  struct capn_segment *current_seg = NULL;
  int total_len = 0;
  int i;
  struct node *n;

  memset(ctx, 0x0, sizeof(*ctx));
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

int ctx_resolve_names(capnp_ctx_t *ctx) {
  struct node *n;
  int i, j;

  for (n = ctx->all_files; n != NULL; n = n->next) {
    struct str b = STR_INIT;
    const char *namespace = NULL;

    /* apply name space if present */
    for (j = capn_len(n->n.annotations) - 1; j >= 0; j--) {
      struct Annotation a;
      struct Value v;
      get_Annotation(&a, n->n.annotations, j);
      read_Value(&v, a.value);

      if (a.id == ANNOTATION_NAMESPACE) {
        if (v.which != Value_text) {
          fail(2, "%s: schema breakage on $C::namespace annotation\n",
               n->n.displayName.str);
        }
        if (namespace) {
          fail(2, "%s: $C::namespace annotation appears more than once.\n",
               n->n.displayName.str);
        }
        namespace = v.text.str ? v.text.str : "";
      }
    }

    if (!namespace)
    namespace = "";

    for (i = capn_len(n->n.nestedNodes) - 1; i >= 0; i--) {
      struct Node_NestedNode nest;
      get_Node_NestedNode(&nest, n->n.nestedNodes, i);
      struct node *nn = find_node_mayfail(ctx, nest.id);
      if (nn) {
        resolve_names(ctx, &b, nn, nest.name, n, namespace);
      }
    }

    str_release(&b);
  }

  return 0;
}

int ctx_mark_used_import(capnp_ctx_t *ctx) {
  struct node *n;
  struct node *f;

  /* find all the used imports */
  for (n = ctx->all_structs; n != NULL; n = n->next) {
    char *display_name = strdup(n->n.displayName.str);
    char *file_name = strtok(display_name, ":");

    if (!file_name) {
      fail(2, "Unable to determine file name for struct node: %s\n",
           n->n.displayName.str);
    }

    /* find the file node corresponding to the file name */
    for (f = ctx->all_files; f != NULL; f = f->next) {
      if (!strcmp(file_name, f->n.displayName.str))
        break;
    }

    if (!f) {
      fail(2, "Unable to find file node with file name: %s\n", file_name);
    }

    /* mark this import as used */
    if (!contains_id(ctx->used_import_ids, f->n.id))
      ctx->used_import_ids = insert_id(ctx->used_import_ids, f->n.id);

    free(display_name);
  }

  return 0;
}

int ctx_gen(capnp_ctx_t *ctx) {
  int i, j;
  struct node *file_node;
  struct node *n;

  for (i = 0; i < capn_len(ctx->req.requestedFiles); i++) {
    struct CodeGeneratorRequest_RequestedFile file_req;
    static struct str b = STR_INIT;
    char *p;
    const char *nameinfix = NULL;
    FILE *srcf, *hdrf;
    struct id_bst *donotinclude_ids = NULL;
    struct string_list *extraheader_strings = NULL;
    const char *extattr = NULL;
    const char *extattr_space = "";

    ctx->g_valc = 0;
    ctx->g_valseg.len = 0;
    ctx->g_val0used = 0;
    ctx->g_nullused = 0;
    capn_init_malloc(&(ctx->g_valcapn));
    capn_append_segment(&(ctx->g_valcapn), &(ctx->g_valseg));

    get_CodeGeneratorRequest_RequestedFile(&file_req, ctx->req.requestedFiles,
                                           i);
    file_node = find_node(ctx, file_req.id);
    if (file_node == NULL) {
      fail(2, "invalid file_node specified\n");
    }

    for (j = capn_len(file_node->n.annotations) - 1; j >= 0; j--) {
      struct Annotation a;
      struct Value v;
      get_Annotation(&a, file_node->n.annotations, j);
      read_Value(&v, a.value);

      switch (a.id) {
      case ANNOTATION_NAMEINFIX: /* $C::nameinfix */
        if (v.which != Value_text) {
          fail(2, "schema breakage on $C::nameinfix annotation\n");
        }
        if (nameinfix) {
          fail(2, "$C::nameinfix annotation appears more than once\n");
        }
        nameinfix = v.text.str ? v.text.str : "";
        break;
      case ANNOTATION_FIELDGETSET: /* $C::fieldgetset */
        ctx->g_fieldgetset = 1;
        break;
      case ANNOTATION_DONOTINCLUDE: /* $C::donotinclude */
        if (v.which != Value_uint64) {
          fail(2, "schema breakage on $C::donotinclude annotation\n");
        }
        donotinclude_ids = insert_id(donotinclude_ids, v.uint64);
        break;
      case ANNOTATION_EXTRAHEADER: /* $C::extraheader("...") */
        if (v.which != Value_text) {
          fail(2, "schema breakage on $C::extraheader annotation\n");
        }
        extraheader_strings =
            insert_file(extraheader_strings, v.text.str ? v.text.str : "");
        break;
      case ANNOTATION_EXTENDEDATTRIBUTE: /* $C::extendedattribute("...") */
        if (v.which != Value_text) {
          fail(2, "schema breakage on $C::extendedattribute annotation\n");
        }
        if (extattr) {
          fail(2, "$C::extendedattribute annotation appears more than once\n");
        }
        if (v.text.str && strlen(v.text.str)) {
          extattr = v.text.str;
          extattr_space = " ";
        }
        break;
      case ANNOTATION_CODECGEN:
        ctx->g_codecgen = 1;
        break;
      }
    }
    if (!nameinfix)
      nameinfix = "";
    if (!extattr)
      extattr = "";

    str_reset(&(ctx->HDR));
    str_reset(&(ctx->SRC));

    str_addf(&(ctx->HDR), "#ifndef CAPN_%X%X\n",
             (uint32_t)(file_node->n.id >> 32), (uint32_t)file_node->n.id);
    str_addf(&(ctx->HDR), "#define CAPN_%X%X\n",
             (uint32_t)(file_node->n.id >> 32), (uint32_t)file_node->n.id);
    str_addf(&(ctx->HDR), "/* AUTO GENERATED - DO NOT EDIT */\n");
    str_addf(&(ctx->HDR), "#include <capnp_c.h>\n");
    /* Do [extraheader] in declaration order. */
    struct string_list **current = &extraheader_strings;
    struct string_list **prev = &extraheader_strings;
    while (*current) {
      prev = current;
      current = &(*current)->next;
    }
    current = prev;
    while (*current) {
      str_addf(&(ctx->HDR), "%s\n", (*current)->string);
      current = &(*current)->prev;
    }
    str_addf(&(ctx->HDR), "\n");

    str_addf(&(ctx->HDR), "#if CAPN_VERSION != 1\n");
    str_addf(
        &(ctx->HDR),
        "#error \"version mismatch between capnp_c.h and generated code\"\n");
    str_addf(&(ctx->HDR), "#endif\n\n");
    str_addf(&(ctx->HDR), "#ifndef capnp_nowarn\n"
                          "# ifdef __GNUC__\n"
                          "#  define capnp_nowarn __extension__\n"
                          "# else\n"
                          "#  define capnp_nowarn\n"
                          "# endif\n"
                          "#endif\n\n");

    for (j = 0; j < capn_len(file_req.imports); j++) {
      struct CodeGeneratorRequest_RequestedFile_Import im;
      get_CodeGeneratorRequest_RequestedFile_Import(&im, file_req.imports, j);

      // Check if this import is in the "do not include" list.
      if (contains_id(donotinclude_ids, im.id)) {
        continue;
      }

      // Check if this import is used at all.
      if (!contains_id(ctx->used_import_ids, im.id)) {
        continue;
      }

      // Ignore leading slashes when generating C file #include's.
      // This signifies an absolute import in a library directory.
      const char *base_path =
          im.name.str[0] == '/' ? &im.name.str[1] : im.name.str;
      str_addf(&(ctx->HDR), "#include \"%s%s.h\"\n", base_path, nameinfix);
    }

    free_id_bst(ctx->used_import_ids);
    free_id_bst(donotinclude_ids);
    free_string_list(extraheader_strings);

    str_addf(&(ctx->HDR), "\n#ifdef __cplusplus\nextern \"C\" {\n#endif\n");

    declare(ctx, file_node, "struct %s;\n", 1);
    declare(ctx, file_node, "typedef struct {capn_ptr p;} %s_ptr;\n", 1);
    declare(ctx, file_node, "typedef struct {capn_ptr p;} %s_list;\n", 1);

    for (n = file_node->file_nodes; n != NULL; n = n->next_file_node) {
      if (n->n.which == Node__enum) {
        define_enum(ctx, n);
      }
    }

    for (n = file_node->file_nodes; n != NULL; n = n->next_file_node) {
      if (n->n.which == Node__const) {
        define_const(ctx, n);
      }
    }

    for (n = file_node->file_nodes; n != NULL; n = n->next_file_node) {
      if (n->n.which == Node__struct && !n->n._struct.isGroup) {
        define_struct(ctx, n, extattr, extattr_space);
        mk_struct_list_encoder(ctx, n);
        mk_struct_ptr_encoder(ctx, n);
        mk_struct_list_decoder(ctx, n);
        mk_struct_ptr_decoder(ctx, n);
        mk_struct_list_free(ctx, n);
        mk_struct_ptr_free(ctx, n);
      }
    }

    declare_ext(ctx, file_node, "%s%s%s_ptr new_%s(struct capn_segment*);\n", 2,
                extattr, extattr_space);
    declare_ext(ctx, file_node,
                "%s%s%s_list new_%s_list(struct capn_segment*, int len);\n", 2,
                extattr, extattr_space);
    declare_ext(ctx, file_node, "%s%svoid read_%s(struct %s*, %s_ptr);\n", 3,
                extattr, extattr_space);
    declare_ext(ctx, file_node,
                "%s%svoid write_%s(const struct %s*, %s_ptr);\n", 3, extattr,
                extattr_space);
    declare_ext(ctx, file_node,
                "%s%svoid get_%s(struct %s*, %s_list, int i);\n", 3, extattr,
                extattr_space);
    declare_ext(ctx, file_node,
                "%s%svoid set_%s(const struct %s*, %s_list, int i);\n", 3,
                extattr, extattr_space);

    if (ctx->g_codecgen) {
      declare_codec(ctx, file_node);
    }

    str_addf(&(ctx->HDR), "\n#ifdef __cplusplus\n}\n#endif\n#endif\n");

    /* write out the header */

    hdrf =
        fopen(strf(&b, "%s%s.h", file_node->n.displayName.str, nameinfix), "w");
    if (!hdrf) {
      fail(2, "failed to open %s: %s\n", b.str, strerror(errno));
    }
    fwrite((ctx->HDR).str, 1, (ctx->HDR).len, hdrf);
    fclose(hdrf);

    /* write out the source */

    srcf =
        fopen(strf(&b, "%s%s.c", file_node->n.displayName.str, nameinfix), "w");
    if (!srcf) {
      fail(2, "failed to open %s: %s\n", b.str, strerror(errno));
    }
    p = strrchr(file_node->n.displayName.str, '/');
    fprintf(srcf, "#include \"%s%s.h\"\n",
            p ? p + 1 : file_node->n.displayName.str, nameinfix);
    fprintf(srcf, "/* AUTO GENERATED - DO NOT EDIT */\n");
    fprintf(srcf, "#ifdef __GNUC__\n"
                  "# define capnp_unused __attribute__((unused))\n"
                  "# define capnp_use(x) (void) (x);\n"
                  "#else\n"
                  "# define capnp_unused\n"
                  "# define capnp_use(x)\n"
                  "#endif\n\n");

    fprintf(srcf, "#include <stdlib.h>\n"
                  "#include <string.h>\n");
    if (ctx->g_val0used)
      fprintf(srcf, "static const capn_text capn_val0 = {0,\"\",0};\n");
    if (ctx->g_nullused)
      fprintf(srcf, "static const capn_ptr capn_null = {CAPN_NULL};\n");

    if (ctx->g_valseg.len > 8) {
      size_t k;
      fprintf(srcf, "static const uint8_t capn_buf[%zu] = {",
              ctx->g_valseg.len - 8);
      for (k = 8; k < ctx->g_valseg.len; k++) {
        if (k > 8)
          fprintf(srcf, ",");
        if ((k % 8) == 0)
          fprintf(srcf, "\n\t");
        fprintf(srcf, "%u", ((uint8_t *)ctx->g_valseg.data)[k]);
      }
      fprintf(srcf, "\n};\n");

      fprintf(srcf,
              "static const struct capn_segment capn_seg = "
              "{{0},0,0,0,(char*)&capn_buf[0],%zu,%zu,0};\n",
              ctx->g_valseg.len - 8, ctx->g_valseg.len - 8);
    }

    fwrite((ctx->SRC).str, 1, (ctx->SRC).len, srcf);
    fclose(srcf);

    capn_free(&(ctx->g_valcapn));
  }

  return 0;
}
int main(int argc, char *argv[]) {
  capnp_ctx_t ctx;
  FILE *fp = NULL;

  if (argc > 2) {
    fail(2, "too many arguments\n");
  } else if (argc == 2) {
    fp = fopen(argv[1], "rb");
    if (fp == NULL) {
      perror("fopen");
      return -1;
    }
  } else {
    fp = stdin;
  }

#if defined(_WIN32)
  if (_setmode(_fileno(fp), _O_BINARY) == -1) {
    fail(-1, "fail to set stdin to binary mode\n");
  }
#endif

  ctx_init(&ctx, fp);

  ctx_resolve_names(&ctx);

  ctx_mark_used_import(&ctx);

  ctx_gen(&ctx);

  return 0;
}
