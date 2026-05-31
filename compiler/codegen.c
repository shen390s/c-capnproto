/* codegen.c
 *
 * Copyright (C) 2013 James McKaskill
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include "codegen.h"
#include "codegen_codec.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

void define_enum(capnp_ctx_t *ctx, struct node *n) {
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
        v->intval = ++(ctx->g_valc);
        symbol = strf(&ctx->scratch[3], "capn_val%d", (int)v->intval);
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
        v->intval = ++(ctx->g_valc);
        symbol = strf(&ctx->scratch[3], "capn_val%d", (int)v->intval);
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

void define_const(capnp_ctx_t *ctx, struct node *n) {
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

static const char *xor_member(struct str *buf, struct field *f) {

  if (f->v.intval) {
    switch (f->v.v.which) {
    case Value_int8:
    case Value_int16:
    case Value_int32:
      return strf(buf, " ^ %d", (int32_t)f->v.intval);

    case Value_uint8:
      return strf(buf, " ^ %uu", (uint8_t)f->v.intval);

    case Value_uint16:
    case Value__enum:
      return strf(buf, " ^ %uu", (uint16_t)f->v.intval);

    case Value_uint32:
      return strf(buf, " ^ %uu", (uint32_t)f->v.intval);

    case Value_float32:
      return strf(buf, " ^ %#xu", (uint32_t)f->v.intval);

    case Value_int64:
      return strf(buf, " ^ ((int64_t)((uint64_t) %#xu << 32) ^ %#xu)",
                  (uint32_t)(f->v.intval >> 32), (uint32_t)f->v.intval);
    case Value_uint64:
    case Value_float64:
      return strf(buf, " ^ ((uint64_t) %#xu << 32) ^ %#xu",
                  (uint32_t)(f->v.intval >> 32), (uint32_t)f->v.intval);

    default:
      return "";
    }
  } else {
    return "";
  }
}

static const char *ptr_member(struct str *buf, struct field *f, const char *var) {
  if (!strcmp(f->v.tname, "capn_ptr")) {
    return var;
  } else if (var[0] == '*') {
    return strf(buf, "%s->p", var + 1);
  } else {
    return strf(buf, "%s.p", var);
  }
}

static void set_member(capnp_ctx_t *ctx, struct str *func, struct field *f,
                       const char *ptr, const char *tab, const char *var) {
  const char *xor = xor_member(&ctx->scratch[0], f);
  const char *pvar = ptr_member(&ctx->scratch[1], f, var);

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
  const char *xor = xor_member(&ctx->scratch[0], f);
  const char *pvar = ptr_member(&ctx->scratch[1], f, var);

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































static const char *tabs(int level) {
  static const char t[] = "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t";
  if (level > 16) level = 16;
  if (level < 0) level = 0;
  return t + (16 - level);
}

struct strings {
  int findent;
  int dindent;
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

static const char *field_name(struct str *buf, struct field *f) {
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
      return strf(buf, "_%s", s);
    }
  }

  return s;
}

static void union_block(capnp_ctx_t *ctx, struct strings *s, struct field *f,
                        const char *u1, const char *u2) {
  s->findent++;
  set_member(ctx, &s->set, f, "p.p", tabs(s->findent),
             strf(&ctx->scratch[3], "%s%s", s->var.str, field_name(&ctx->scratch[2], f)));
  get_member(ctx, &s->get, f, "p.p", tabs(s->findent),
             strf(&ctx->scratch[3], "%s%s", s->var.str, field_name(&ctx->scratch[2], f)));
  str_addf(&s->set, "%sbreak;\n", tabs(s->findent));
  str_addf(&s->get, "%sbreak;\n", tabs(s->findent));
  if (ctx->g_codecgen) {
    struct str var1 = STR_INIT;
    struct str var2 = STR_INIT;
    char *mapname = (char *)get_mapname(f->f.annotations);

    if (u2 == NULL) {
      u2 = u1;
    }

    if (mapname == NULL) {
      mapname = (char *)field_name(&ctx->scratch[2], f);
    }

    if (u1 != NULL) {
      strf(&var1, "%s.%s", u1, field_name(&ctx->scratch[2], f));
      strf(&var2, "%s.%s", u2, mapname);
    } else {
      str_add(&var1, field_name(&ctx->scratch[2], f), -1);
      str_add(&var2, mapname, -1);
    }

    encode_member(ctx, &s->encoder, f, tabs(s->findent), var1.str, var2.str);
    str_addf(&s->encoder, "%sbreak;\n", tabs(s->findent));
    decode_member(ctx, &s->decoder, f, tabs(s->findent), var1.str, var2.str);
    str_addf(&s->decoder, "%sbreak;\n", tabs(s->findent));
    free_member(ctx, &s->freeup, f, tabs(s->findent), var1.str, var2.str);
    str_addf(&s->freeup, "%sbreak;\n", tabs(s->findent));
    str_release(&var1);
    str_release(&var2);
  }
  s->findent--;
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
    str_addf(&s->set, "%scase %s_%s:\n", tabs(s->findent), n->name.str,
             field_name(&ctx->scratch[2], f));
    str_addf(&s->get, "%scase %s_%s:\n", tabs(s->findent), n->name.str,
             field_name(&ctx->scratch[2], f));
    if (ctx->g_codecgen) {
      str_addf(&s->encoder, "%scase %s_%s:\n", tabs(s->findent), n->name.str,
               field_name(&ctx->scratch[2], f));
      str_addf(&s->decoder, "%scase %s_%s:\n", tabs(s->findent), n->name.str,
               field_name(&ctx->scratch[2], f));
      str_addf(&s->freeup, "%scase %s_%s:\n", tabs(s->findent), n->name.str,
               field_name(&ctx->scratch[2], f));
    }

    if (u) {
      union_block(ctx, s, u,
                  &(n->n.displayName.str[n->n.displayNamePrefixLength]), NULL);
    }
  }
}

static void declare_slot(capnp_ctx_t *ctx, struct strings *s, struct field *f) {
  switch (f->v.t.which) {
  case Type__void:
    break;
  case Type__bool:
    str_addf(&s->decl, "%s%s %s : 1;\n", tabs(s->dindent), f->v.tname,
             field_name(&ctx->scratch[2], f));
    break;
  default:
    str_addf(&s->decl, "%s%s %s;\n", tabs(s->dindent), f->v.tname, field_name(&ctx->scratch[2], f));
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
  struct str tag = STR_INIT;
  struct str enums = STR_INIT;

  str_reset(&tag);

  if (union_name) {
    str_addf(&tag, "%.*s_which", s->var.len - 1, s->var.str);
    str_addf(&enums, "enum %s_which {", n->name.str);
    str_addf(&s->decl, "%senum %s_which %s_which;\n", tabs(s->dindent), n->name.str,
             union_name);
    str_addf(&s->get, "%s%s = (enum %s_which)(int) capn_read16(p.p, %d);\n",
             tabs(s->findent), tag.str, n->name.str, tagoff);
  } else {
    str_addf(&tag, "%swhich", s->var.str);
    str_addf(&enums, "enum %s_which {", n->name.str);
    str_addf(&s->decl, "%senum %s_which which;\n", tabs(s->dindent), n->name.str);
    str_addf(&s->get, "%s%s = (enum %s_which)(int) capn_read16(p.p, %d);\n",
             tabs(s->findent), tag.str, n->name.str, tagoff);
  }

  str_addf(&s->set, "%scapn_write16(p.p, %d, %s);\n", tabs(s->findent), tagoff,
           tag.str);
  str_addf(&s->set, "%sswitch (%s) {\n", tabs(s->findent), tag.str);
  str_addf(&s->get, "%sswitch (%s) {\n", tabs(s->findent), tag.str);

  if (ctx->g_codecgen) {
    struct str var = STR_INIT;
    char *p = strstr(tag.str, "->");

    if (p == NULL) {
      fail(2, "bad variable");
    }

    strf(&var, "d%s", p);

    str_addf(&s->encoder, "%sswitch (%s) {\n", tabs(s->findent), var.str);
    str_addf(&s->decoder, "%sswitch (%s) {\n", tabs(s->findent), tag.str);
    str_addf(&s->freeup, "%sswitch (%s) {\n", tabs(s->findent), uniontag);
    str_release(&var);
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

  str_addf(&s->decl, "%scapnp_nowarn union {\n", tabs(s->dindent));
  s->dindent++;

  /* when we have defaults or groups we have to emit each case seperately */
  for (f = first_field;
       f < n->fields + capn_len(n->n._struct.fields) && in_union(f); f++) {
    if (f > first_field) {
      str_addf(&enums, ",");
    }

    str_addf(&enums, "\n\t%s_%s = %d", n->name.str, field_name(&ctx->scratch[2], f),
             f->f.discriminantValue);

    switch (f->f.which) {
    case Field_group:
      str_addf(&s->get, "%scase %s_%s:\n", tabs(s->findent), n->name.str,
               field_name(&ctx->scratch[2], f));
      str_addf(&s->set, "%scase %s_%s:\n", tabs(s->findent), n->name.str,
               field_name(&ctx->scratch[2], f));
      s->findent++;
      // When we add a union inside a union, we need to enclose it in its
      // own struct so that its members do not overwrite its own
      // discriminant.
      define_group(ctx, s, f->group, field_name(&ctx->scratch[2], f), true, extattr,
                   extattr_space, uniontag);
      str_addf(&s->get, "%sbreak;\n", tabs(s->findent));
      str_addf(&s->set, "%sbreak;\n", tabs(s->findent));
      if (ctx->g_codecgen) {
        str_addf(&s->encoder, "%sbreak;\n", tabs(s->findent));
        str_addf(&s->decoder, "%sbreak;\n", tabs(s->findent));
        str_addf(&s->freeup, "%sbreak;\n", tabs(s->findent));
      }
      s->findent--;
      break;

    case Field_slot:
      declare_slot(ctx, s, f);
      if (f->v.ptrval.type || f->v.intval) {
        str_addf(&s->get, "%scase %s_%s:\n", tabs(s->findent), n->name.str,
                 field_name(&ctx->scratch[2], f));
        str_addf(&s->set, "%scase %s_%s:\n", tabs(s->findent), n->name.str,
                 field_name(&ctx->scratch[2], f));
        if (ctx->g_codecgen) {
          str_addf(&s->encoder, "%scase %s_%s:\n", tabs(s->findent), n->name.str,
                   field_name(&ctx->scratch[2], f));
          str_addf(&s->decoder, "%scase %s_%s:\n", tabs(s->findent), n->name.str,
                   field_name(&ctx->scratch[2], f));
          str_addf(&s->freeup, "%scase %s_%s:\n", tabs(s->findent), n->name.str,
                   field_name(&ctx->scratch[2], f));
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

  s->dindent--;

  if (union_name) {
    str_addf(&s->decl, "%s} %s;\n", tabs(s->dindent), union_name);
  } else {
    str_addf(&s->decl, "%s};\n", tabs(s->dindent));
  }

  str_addf(&s->get, "%sdefault:\n%s\tbreak;\n%s}\n", tabs(s->findent), tabs(s->findent),
           tabs(s->findent));
  str_addf(&s->set, "%sdefault:\n%s\tbreak;\n%s}\n", tabs(s->findent), tabs(s->findent),
           tabs(s->findent));

  if (ctx->g_codecgen) {
    str_addf(&s->encoder, "%sdefault:\n%s\tbreak;\n%s}\n", tabs(s->findent),
             tabs(s->findent), tabs(s->findent));
    str_addf(&s->decoder, "%sdefault:\n%s\tbreak;\n%s}\n", tabs(s->findent),
             tabs(s->findent), tabs(s->findent));
    str_addf(&s->freeup, "%sdefault:\n%s\tbreak;\n%s}\n", tabs(s->findent),
             tabs(s->findent), tabs(s->findent));
  }

  str_addf(&enums, "\n};\n");
  str_add(&s->enums, enums.str, enums.len);
  str_release(&enums);
  str_release(&tag);
}

static void define_field(capnp_ctx_t *ctx, struct strings *s, struct field *f,
                         const char *extattr, const char *extattr_space) {

  switch (f->f.which) {
  case Field_slot:
    declare_slot(ctx, s, f);
    set_member(ctx, &s->set, f, "p.p", tabs(s->findent),
               strf(&ctx->scratch[3], "%s%s", s->var.str, field_name(&ctx->scratch[2], f)));
    get_member(ctx, &s->get, f, "p.p", tabs(s->findent),
               strf(&ctx->scratch[3], "%s%s", s->var.str, field_name(&ctx->scratch[2], f)));
    if (ctx->g_codecgen) {
      encode_member(ctx, &s->encoder, f, tabs(s->findent), field_name(&ctx->scratch[2], f),
                    get_mapname(f->f.annotations));
      decode_member(ctx, &s->decoder, f, tabs(s->findent), field_name(&ctx->scratch[2], f),
                    get_mapname(f->f.annotations));
      free_member(ctx, &s->freeup, f, tabs(s->findent), field_name(&ctx->scratch[2], f),
                  get_mapname(f->f.annotations));
    }
    break;

  case Field_group:
    if (ctx->g_codecgen) {
      struct str uniontagvar = STR_INIT;

      if (f->group != NULL) {
        int flen = capn_len(f->group->n._struct.fields);
        int ulen = f->group->n._struct.discriminantCount;

        if ((ulen == flen) && (ulen > 0)) {
          if (field_name(&ctx->scratch[2], f) != NULL) {
            char *uniontag = (char *)get_mapuniontag(f->f.annotations);
            struct str buf = STR_INIT;

            if (uniontag != NULL) {
              str_add(&buf, uniontag, -1);
            } else {
              strf(&buf, "%s_which", field_name(&ctx->scratch[2], f));
            }

            str_addf(&s->encoder, "\td->%s_which = s->%s;\n", field_name(&ctx->scratch[2], f),
                     buf.str);
            str_addf(&s->decoder, "\td->%s = s->%s_which;\n", buf.str,
                     field_name(&ctx->scratch[2], f));
            strf(&uniontagvar, "d->%s", buf.str);
            str_release(&buf);
          }
        }
      }
      define_group(ctx, s, f->group, field_name(&ctx->scratch[2], f), false, extattr,
                   extattr_space, uniontagvar.str);
      str_release(&uniontagvar);
    } else {
      define_group(ctx, s, f->group, field_name(&ctx->scratch[2], f), false, extattr,
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
           extattr_space, field->v.tname, node->name.str, field_name(&ctx->scratch[2], field),
           node->name.str);
  str_addf(&s->pub_get, "\n%s%s%s %s_get_%s(%s_ptr p)\n", extattr,
           extattr_space, field->v.tname, node->name.str, field_name(&ctx->scratch[2], field),
           node->name.str);
  struct str getter_body = STR_INIT;
  get_member(ctx, &getter_body, field, "p.p", "", field_name(&ctx->scratch[2], field));
  str_addf(&s->pub_get, "{\n");
  str_addf(&s->pub_get, "%s%s %s;\n", tabs(s->findent), field->v.tname,
           field_name(&ctx->scratch[2], field));
  str_addf(&s->pub_get, "%s%s", tabs(s->findent), getter_body.str);
  str_release(&getter_body);
  str_addf(&s->pub_get, "%sreturn %s;\n}\n", tabs(s->findent), field_name(&ctx->scratch[2], field));
}

static void define_setter_functions(capnp_ctx_t *ctx, struct node *node,
                                    struct field *field, struct strings *s,
                                    const char *extattr,
                                    const char *extattr_space) {
  str_addf(&s->pub_set_header, "\n%s%svoid %s_set_%s(%s_ptr p, %s %s);\n",
           extattr, extattr_space, node->name.str, field_name(&ctx->scratch[2], field),
           node->name.str, field->v.tname, field_name(&ctx->scratch[2], field));
  str_addf(&s->pub_set, "\n%s%svoid %s_set_%s(%s_ptr p, %s %s)\n", extattr,
           extattr_space, node->name.str, field_name(&ctx->scratch[2], field), node->name.str,
           field->v.tname, field_name(&ctx->scratch[2], field));
  struct str setter_body = STR_INIT;
  set_member(ctx, &setter_body, field, "p.p", tabs(s->findent), field_name(&ctx->scratch[2], field));
  str_addf(&s->pub_set, "{\n%s}\n", setter_body.str);
  str_release(&setter_body);
}

static void strings_init(struct strings *s) {
    memset(s, 0, sizeof(*s));
    s->findent = 0; s->dindent = 0; str_init(&s->get, 0);
    str_init(&s->set, 0); str_init(&s->encoder, 0); str_init(&s->decoder, 0);
    str_init(&s->freeup, 0); str_init(&s->enums, 0); str_init(&s->decl, 0);
    str_init(&s->var, 0); str_init(&s->pub_get, 0); str_init(&s->pub_set, 0);
    str_init(&s->pub_get_header, 0); str_init(&s->pub_set_header, 0);
}

static void strings_release(struct strings *s) {
    str_release(&s->get);
    str_release(&s->set); str_release(&s->encoder); str_release(&s->decoder);
    str_release(&s->freeup); str_release(&s->enums); str_release(&s->decl);
    str_release(&s->var); str_release(&s->pub_get); str_release(&s->pub_set);
    str_release(&s->pub_get_header); str_release(&s->pub_set_header);
}

static void emit_struct_accessors(capnp_ctx_t *ctx, struct node *n,
                                  struct strings *s, const char *extattr,
                                  const char *extattr_space) {
  str_addf(&(ctx->SRC), "\n%s%s%s_ptr new_%s(struct capn_segment *s) {\n",
           extattr, extattr_space, n->name.str, n->name.str);
  str_addf(&(ctx->SRC), "\t%s_ptr p;\n", n->name.str);
  str_addf(&(ctx->SRC), "\tp.p = capn_new_struct(s, %d, %d);\n",
           8 * n->n._struct.dataWordCount, n->n._struct.pointerCount);
  str_addf(&(ctx->SRC), "\treturn p;\n");
  str_addf(&(ctx->SRC), "}\n");

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
  str_add(&(ctx->SRC), s->get.str, s->get.len);
  str_addf(&(ctx->SRC), "}\n");

  str_addf(&(ctx->SRC),
           "%s%svoid write_%s(const struct %s *s capnp_unused, %s_ptr p) {\n",
           extattr, extattr_space, n->name.str, n->name.str, n->name.str);
  str_addf(&(ctx->SRC), "\tcapn_resolve(&p.p);\n\tcapnp_use(s);\n");
  str_add(&(ctx->SRC), s->set.str, s->set.len);
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
}

static void emit_struct_codec(capnp_ctx_t *ctx, struct node *n,
                              struct strings *s) {
  if (ctx->g_codecgen) {
    const char *mapname = get_mapname(n->n.annotations);
    struct str buf = STR_INIT;

    if (mapname == NULL) {
      strf(&buf, "struct %s_", n->name.str);
    } else {
      str_add(&buf, mapname, -1);
    }
    str_addf(
        &(ctx->SRC),
        "\nvoid encode_%s(struct capn_segment *cs,struct %s *d, %s *s) {\n",
        n->name.str, n->name.str, buf.str);
    str_addf(&(ctx->SRC), "%s\n", s->encoder.str);
    str_addf(&(ctx->SRC), "}\n");
    str_addf(&(ctx->SRC), "\nvoid decode_%s(%s *d, struct %s *s) {\n",
             n->name.str, buf.str, n->name.str);
    str_addf(&(ctx->SRC), "%s\n", s->decoder.str);
    str_addf(&(ctx->SRC), "}\n");
    str_addf(&(ctx->SRC), "\nvoid free_%s(%s *d) {\n", n->name.str, buf.str);
    str_addf(&(ctx->SRC), "%s\n", s->freeup.str);
    str_addf(&(ctx->SRC), "}\n");
    str_release(&buf);
  }
}
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
    str_addf(&s->decl, "%s/* struct { -empty- } %s; */\n", tabs(s->dindent),
             group_name);
    return;
  }

  if (named_struct) {
    str_addf(&s->decl, "%scapnp_nowarn struct {\n", tabs(s->dindent));
    s->dindent++;
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
      str_addf(&s->decl, "%scapnp_nowarn struct {\n", tabs(s->dindent));
      s->dindent++;
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
      s->dindent--;
      str_addf(&s->decl, "%s} %s;\n", tabs(s->dindent), group_name);
    }
  }

  if (named_struct) {
    s->dindent--;
    str_addf(&s->decl, "%s} %s;\n", tabs(s->dindent), group_name);
  }

  if (group_name) {
    str_setlen(&s->var, s->var.len - strlen(group_name) - 1);
  }
}

void define_struct(capnp_ctx_t *ctx, struct node *n, const char *extattr,
                          const char *extattr_space) {
  struct strings s;
  int i;

  strings_init(&s);

  s.dindent = 1;
  s.findent = 1;
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

  emit_struct_accessors(ctx, n, &s, extattr, extattr_space);
  emit_struct_codec(ctx, n, &s);

  str_add(&(ctx->SRC), s.pub_get.str, s.pub_get.len);
  str_add(&(ctx->SRC), s.pub_set.str, s.pub_set.len);

  str_add(&(ctx->HDR), s.pub_get_header.str, s.pub_get_header.len);
  str_add(&(ctx->HDR), s.pub_set_header.str, s.pub_set_header.len);

  strings_release(&s);
}

void declare(capnp_ctx_t *ctx, struct node *file_node,
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

void declare_ext(capnp_ctx_t *ctx, struct node *file_node,
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


