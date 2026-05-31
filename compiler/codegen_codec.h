/* codegen_codec.h - codec generation functions for capnpc-c */
#ifndef CODEGEN_CODEC_H
#define CODEGEN_CODEC_H

#include "ctx.h"

void encode_member(capnp_ctx_t *ctx, struct str *func, struct field *f,
                   const char *tab, const char *var, const char *var2);
void decode_member(capnp_ctx_t *ctx, struct str *func, struct field *f,
                   const char *tab, const char *var, const char *var2);
void free_member(capnp_ctx_t *ctx, struct str *func, struct field *f,
                 const char *tab, const char *var, const char *var2);
void mk_struct_list_encoder(capnp_ctx_t *ctx, struct node *n);
void mk_struct_ptr_encoder(capnp_ctx_t *ctx, struct node *n);
void mk_struct_list_decoder(capnp_ctx_t *ctx, struct node *n);
void mk_struct_ptr_decoder(capnp_ctx_t *ctx, struct node *n);
void mk_struct_list_free(capnp_ctx_t *ctx, struct node *n);
void mk_struct_ptr_free(capnp_ctx_t *ctx, struct node *n);
void declare_codec(capnp_ctx_t *ctx, struct node *file_node);

#endif /* CODEGEN_CODEC_H */
