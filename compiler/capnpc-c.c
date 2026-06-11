/* capnpc-c.c
 *
 * Copyright (C) 2013 James McKaskill
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */
#define _POSIX_C_SOURCE 200809L

#include "codegen.h"
#include "codegen_codec.h"
#include "ctx.h"
#include "resolve.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif

#if defined(__linux)
#define _fileno fileno
#endif

static void process_file_annotations(capnp_ctx_t *ctx, struct node *file_node,
                                     const char **nameinfix,
                                     struct id_bst **donotinclude_ids,
                                     struct string_list **extraheader_strings,
                                     const char **extattr,
                                     const char **extattr_space) {
  int j;
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
      if (*nameinfix) {
        fail(2, "$C::nameinfix annotation appears more than once\n");
      }
      *nameinfix = v.text.str ? v.text.str : "";
      break;
    case ANNOTATION_FIELDGETSET: /* $C::fieldgetset */
      ctx->g_fieldgetset = 1;
      break;
    case ANNOTATION_DONOTINCLUDE: /* $C::donotinclude */
      if (v.which != Value_uint64) {
        fail(2, "schema breakage on $C::donotinclude annotation\n");
      }
      *donotinclude_ids = insert_id(*donotinclude_ids, v.uint64);
      break;
    case ANNOTATION_EXTRAHEADER: /* $C::extraheader("...") */
      if (v.which != Value_text) {
        fail(2, "schema breakage on $C::extraheader annotation\n");
      }
      *extraheader_strings =
          insert_file(*extraheader_strings, v.text.str ? v.text.str : "");
      break;
    case ANNOTATION_EXTENDEDATTRIBUTE: /* $C::extendedattribute("...") */
      if (v.which != Value_text) {
        fail(2, "schema breakage on $C::extendedattribute annotation\n");
      }
      if (*extattr) {
        fail(2, "$C::extendedattribute annotation appears more than once\n");
      }
      if (v.text.str && strlen(v.text.str)) {
        *extattr = v.text.str;
        *extattr_space = " ";
      }
      break;
    case ANNOTATION_CODECGEN:
      ctx->g_codecgen = 1;
      break;
    }
  }
  if (!*nameinfix)
    *nameinfix = "";
  if (!*extattr)
    *extattr = "";
}

static void emit_header_preamble(capnp_ctx_t *ctx, struct node *file_node,
                                 struct CodeGeneratorRequest_RequestedFile *file_req,
                                 const char *nameinfix,
                                 struct id_bst *donotinclude_ids,
                                 struct string_list *extraheader_strings) {
  int j;

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

  str_addf(&(ctx->HDR),
           "#ifndef STRING_DUP\n"
           "#define STRING_DUP strdup\n"
           "#endif\n\n");

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

  for (j = 0; j < capn_len(file_req->imports); j++) {
    struct CodeGeneratorRequest_RequestedFile_Import im;
    get_CodeGeneratorRequest_RequestedFile_Import(&im, file_req->imports, j);

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
}

static void write_source_file(capnp_ctx_t *ctx, struct node *file_node,
                              const char *nameinfix) {
  struct str b = STR_INIT;
  char *p;
  FILE *srcf;

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
  str_release(&b);
}

int ctx_gen(capnp_ctx_t *ctx) {
  int i;
  struct node *file_node;
  struct node *n;

  for (i = 0; i < capn_len(ctx->req.requestedFiles); i++) {
    struct CodeGeneratorRequest_RequestedFile file_req;
    struct str b = STR_INIT;
    const char *nameinfix = NULL;
    FILE *hdrf;
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

    process_file_annotations(ctx, file_node, &nameinfix, &donotinclude_ids,
                             &extraheader_strings, &extattr, &extattr_space);

    str_reset(&(ctx->HDR));
    str_reset(&(ctx->SRC));

    emit_header_preamble(ctx, file_node, &file_req, nameinfix,
                         donotinclude_ids, extraheader_strings);

    /* generate types: enums, consts, structs */
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
        if (ctx->g_codecgen) {
          mk_struct_list_encoder(ctx, n);
          mk_struct_ptr_encoder(ctx, n);
          mk_struct_list_decoder(ctx, n);
          mk_struct_ptr_decoder(ctx, n);
          mk_struct_list_free(ctx, n);
          mk_struct_ptr_free(ctx, n);
        }
      }
    }

    /* emit header declarations */
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

    /* write header file */
    hdrf =
        fopen(strf(&b, "%s%s.h", file_node->n.displayName.str, nameinfix), "w");
    if (!hdrf) {
      fail(2, "failed to open %s: %s\n", b.str, strerror(errno));
    }
    fwrite((ctx->HDR).str, 1, (ctx->HDR).len, hdrf);
    fclose(hdrf);

    /* write source file */
    write_source_file(ctx, file_node, nameinfix);

    capn_free(&(ctx->g_valcapn));
    str_release(&b);
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
