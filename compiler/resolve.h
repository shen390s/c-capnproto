/* resolve.h - name resolution for capnpc-c */
#ifndef RESOLVE_H
#define RESOLVE_H

#include "ctx.h"

void resolve_names(capnp_ctx_t *ctx, struct str *b, struct node *n,
                   capn_text name, struct node *file, const char *namespace);
int ctx_resolve_names(capnp_ctx_t *ctx);
int ctx_mark_used_import(capnp_ctx_t *ctx);

#endif /* RESOLVE_H */
