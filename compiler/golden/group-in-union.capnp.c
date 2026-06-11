#include "group-in-union.capnp.h"
/* AUTO GENERATED - DO NOT EDIT */
#ifdef __GNUC__
# define capnp_unused __attribute__((unused))
# define capnp_use(x) (void) (x);
#else
# define capnp_unused
# define capnp_use(x)
#endif

#include <stdlib.h>
#include <string.h>
static const capn_text capn_val0 = {0,"",0};
static const capn_ptr capn_null = {CAPN_NULL};

GroupInUnion_ptr new_GroupInUnion(struct capn_segment *s) {
	GroupInUnion_ptr p;
	p.p = capn_new_struct(s, 16, 1);
	return p;
}
GroupInUnion_list new_GroupInUnion_list(struct capn_segment *s, int len) {
	GroupInUnion_list p;
	p.p = capn_new_list(s, len, 16, 1);
	return p;
}
void read_GroupInUnion(struct GroupInUnion *s capnp_unused, GroupInUnion_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	s->data_which = (enum GroupInUnion_data_which)(int) capn_read16(p.p, 4);
	switch (s->data_which) {
	case GroupInUnion_data_baz:
		s->data.baz = capn_get_text(p.p, 0, capn_val0);
		break;
	case GroupInUnion_data_foo:
		s->data.foo.x = (int32_t) ((int32_t)capn_read32(p.p, 0));
		s->data.foo.y = (int64_t) ((int64_t)(capn_read64(p.p, 8)));
		break;
	case GroupInUnion_data_bar:
		s->data.bar.name = capn_get_text(p.p, 0, capn_val0);
		s->data.bar.value = capn_read32(p.p, 0);
		break;
	default:
		break;
	}
}
void write_GroupInUnion(const struct GroupInUnion *s capnp_unused, GroupInUnion_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	capn_write16(p.p, 4, s->data_which);
	switch (s->data_which) {
	case GroupInUnion_data_baz:
		capn_set_text(p.p, 0, s->data.baz);
		break;
	case GroupInUnion_data_foo:
		capn_write32(p.p, 0, (uint32_t) (s->data.foo.x));
		capn_write64(p.p, 8, (uint64_t) (s->data.foo.y));
		break;
	case GroupInUnion_data_bar:
		capn_set_text(p.p, 0, s->data.bar.name);
		capn_write32(p.p, 0, s->data.bar.value);
		break;
	default:
		break;
	}
}
void get_GroupInUnion(struct GroupInUnion *s, GroupInUnion_list l, int i) {
	GroupInUnion_ptr p;
	p.p = capn_getp(l.p, i, 0);
	read_GroupInUnion(s, p);
}
void set_GroupInUnion(const struct GroupInUnion *s, GroupInUnion_list l, int i) {
	GroupInUnion_ptr p;
	p.p = capn_getp(l.p, i, 0);
	write_GroupInUnion(s, p);
}

void encode_GroupInUnion(struct capn_segment *cs,struct GroupInUnion *d, group_in_union_t *s) {
	d->data_which = s->kind;
	switch (d->data_which) {
	case GroupInUnion_data_baz:
		if (s->data.baz != NULL) {
			d->data.baz.str = s->data.baz;
			d->data.baz.len = strlen(s->data.baz);
		}
		else{
			d->data.baz.str = "";
			d->data.baz.len = 0;
		}
		d->data.baz.seg = NULL;
		break;
	case GroupInUnion_data_foo:
		d->data.foo.x = s->data.foo.x;
		d->data.foo.y = s->data.foo.y;
		break;
	case GroupInUnion_data_bar:
		if (s->data.bar.name != NULL) {
			d->data.bar.name.str = s->data.bar.name;
			d->data.bar.name.len = strlen(s->data.bar.name);
		}
		else{
			d->data.bar.name.str = "";
			d->data.bar.name.len = 0;
		}
		d->data.bar.name.seg = NULL;
		d->data.bar.value = s->data.bar.value;
		break;
	default:
		break;
	}

}

void decode_GroupInUnion(group_in_union_t *d, struct GroupInUnion *s) {
	d->kind = s->data_which;
	switch (s->data_which) {
	case GroupInUnion_data_baz:
		d->data.baz = STRING_DUP(s->data.baz.str);
		break;
	case GroupInUnion_data_foo:
		d->data.foo.x = s->data.foo.x;
		d->data.foo.y = s->data.foo.y;
		break;
	case GroupInUnion_data_bar:
		d->data.bar.name = STRING_DUP(s->data.bar.name.str);
		d->data.bar.value = s->data.bar.value;
		break;
	default:
		break;
	}

}

void free_GroupInUnion(group_in_union_t *d) {
	switch (d->kind) {
	case GroupInUnion_data_baz:
		if (d->data.baz != NULL) {
			free(d->data.baz);
		}
		break;
	case GroupInUnion_data_foo:
		break;
	case GroupInUnion_data_bar:
		if (d->data.bar.name != NULL) {
			free(d->data.bar.name);
		}
		break;
	default:
		break;
	}

}
void encode_GroupInUnion_list(struct capn_segment *cs, GroupInUnion_list *l,int count,group_in_union_t **s) {
	GroupInUnion_list lst;
	int i;
	lst = new_GroupInUnion_list(cs, count);
	for(i = 0; i < count; i ++) {
		struct GroupInUnion d;
		encode_GroupInUnion(cs, &d, s[i]);
		set_GroupInUnion(&d, lst, i);
	}
	(*l) = lst;
}
void encode_GroupInUnion_ptr(struct capn_segment *cs, GroupInUnion_ptr *p,group_in_union_t *s) {
	GroupInUnion_ptr ptr;
	struct GroupInUnion d;
	ptr = new_GroupInUnion(cs);
	if (s == NULL) {
		ptr.p = capn_null;
	}
	else{
		encode_GroupInUnion(cs, &d, s);
		write_GroupInUnion(&d, ptr);
	}
	(*p) = ptr;
}
void decode_GroupInUnion_list(int *pcount, group_in_union_t ***d, GroupInUnion_list list) {
	int i;
	int nc;
	group_in_union_t **ptr;
	capn_resolve(&(list.p));
	nc = list.p.len;
	if (nc == 0) {
		(*d) = NULL;
		(*pcount) = 0;
		return;
	}
	ptr = (group_in_union_t **)calloc(nc, sizeof(group_in_union_t *));
	for(i = 0; i < nc; i ++) {
		struct GroupInUnion s;
		get_GroupInUnion(&s, list, i);
		ptr[i] = (group_in_union_t *)calloc(1, sizeof(group_in_union_t));
		decode_GroupInUnion(ptr[i], &s);
	}
	(*d) = ptr;
	(*pcount) = nc;
}
void decode_GroupInUnion_ptr(group_in_union_t **d,GroupInUnion_ptr p) {
	struct GroupInUnion s;
	capn_resolve(&(p.p));
	if (p.p.type == CAPN_NULL) {
		(*d) = NULL;
		return;
	}
	*d = (group_in_union_t *)calloc(1, sizeof(group_in_union_t));
	read_GroupInUnion(&s, p);
	decode_GroupInUnion(*d, &s);
}
void free_GroupInUnion_list(int pcount, group_in_union_t **d) {
	int i;
	int nc = pcount;
	group_in_union_t **ptr = d;
	if (ptr == NULL) return;
	for(i = 0; i < nc; i ++) {
		if(ptr[i] == NULL) continue;
		free_GroupInUnion(ptr[i]);
		free(ptr[i]);
	}
	free(ptr);
}
void free_GroupInUnion_ptr(group_in_union_t **d){
	if((*d) == NULL) return;
	free_GroupInUnion(*d);
	free(*d);
	(*d) = NULL;
}
