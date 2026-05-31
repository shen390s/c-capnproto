/* codegen.h - code generation functions for capnpc-c */
#ifndef CODEGEN_H
#define CODEGEN_H

#include "ctx.h"

void define_enum(capnp_ctx_t *ctx, struct node *n);
void define_const(capnp_ctx_t *ctx, struct node *n);
void define_struct(capnp_ctx_t *ctx, struct node *n, const char *extattr,
                   const char *extattr_space);
void declare(capnp_ctx_t *ctx, struct node *file_node, const char *format,
             int num);
void declare_ext(capnp_ctx_t *ctx, struct node *file_node, const char *format,
                 int num, const char *extattr, const char *extattr_space);

#endif /* CODEGEN_H */
