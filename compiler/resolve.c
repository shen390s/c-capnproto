/* resolve.c - name resolution for capnpc-c */
#define _POSIX_C_SOURCE 200809L

#include "resolve.h"
#include <string.h>
#include <stdlib.h>

void resolve_names(capnp_ctx_t *ctx, struct str *b, struct node *n,
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
