#include "addressbook.capnp.h"
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

Person_ptr new_Person(struct capn_segment *s) {
	Person_ptr p;
	p.p = capn_new_struct(s, 8, 4);
	return p;
}
Person_list new_Person_list(struct capn_segment *s, int len) {
	Person_list p;
	p.p = capn_new_list(s, len, 8, 4);
	return p;
}
void read_Person(struct Person *s capnp_unused, Person_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	s->id = capn_read32(p.p, 0);
	s->name = capn_get_text(p.p, 0, capn_val0);
	s->email = capn_get_text(p.p, 1, capn_val0);
	s->phones.p = capn_getp(p.p, 2, 0);
	s->employment_which = (enum Person_employment_which)(int) capn_read16(p.p, 4);
	switch (s->employment_which) {
	case Person_employment_employer:
		s->employment.employer = capn_get_text(p.p, 3, capn_val0);
		break;
	case Person_employment_school:
		s->employment.school = capn_get_text(p.p, 3, capn_val0);
		break;
	default:
		break;
	}
}
void write_Person(const struct Person *s capnp_unused, Person_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	capn_write32(p.p, 0, s->id);
	capn_set_text(p.p, 0, s->name);
	capn_set_text(p.p, 1, s->email);
	capn_setp(p.p, 2, s->phones.p);
	capn_write16(p.p, 4, s->employment_which);
	switch (s->employment_which) {
	case Person_employment_employer:
		capn_set_text(p.p, 3, s->employment.employer);
		break;
	case Person_employment_school:
		capn_set_text(p.p, 3, s->employment.school);
		break;
	default:
		break;
	}
}
void get_Person(struct Person *s, Person_list l, int i) {
	Person_ptr p;
	p.p = capn_getp(l.p, i, 0);
	read_Person(s, p);
}
void set_Person(const struct Person *s, Person_list l, int i) {
	Person_ptr p;
	p.p = capn_getp(l.p, i, 0);
	write_Person(s, p);
}

uint32_t Person_get_id(Person_ptr p)
{
	uint32_t id;
	id = capn_read32(p.p, 0);
	return id;
}

capn_text Person_get_name(Person_ptr p)
{
	capn_text name;
	name = capn_get_text(p.p, 0, capn_val0);
	return name;
}

capn_text Person_get_email(Person_ptr p)
{
	capn_text email;
	email = capn_get_text(p.p, 1, capn_val0);
	return email;
}

Person_PhoneNumber_list Person_get_phones(Person_ptr p)
{
	Person_PhoneNumber_list phones;
	phones.p = capn_getp(p.p, 2, 0);
	return phones;
}

void Person_set_id(Person_ptr p, uint32_t id)
{
	capn_write32(p.p, 0, id);
}

void Person_set_name(Person_ptr p, capn_text name)
{
	capn_set_text(p.p, 0, name);
}

void Person_set_email(Person_ptr p, capn_text email)
{
	capn_set_text(p.p, 1, email);
}

void Person_set_phones(Person_ptr p, Person_PhoneNumber_list phones)
{
	capn_setp(p.p, 2, phones.p);
}
void encode_Person_list(struct capn_segment *cs, Person_list *l,int count,struct Person_ **s) {
	Person_list lst;
	int i;
	lst = new_Person_list(cs, count);
	for(i = 0; i < count; i ++) {
		struct Person d;
		encode_Person(cs, &d, s[i]);
		set_Person(&d, lst, i);
	}
	(*l) = lst;
}
void encode_Person_ptr(struct capn_segment *cs, Person_ptr *p,struct Person_ *s) {
	Person_ptr ptr;
	struct Person d;
	ptr = new_Person(cs);
	if (s == NULL) {
		ptr.p = capn_null;
	}
	else{
		encode_Person(cs, &d, s);
		write_Person(&d, ptr);
	}
	(*p) = ptr;
}
void decode_Person_list(int *pcount, struct Person_ ***d, Person_list list) {
	int i;
	int nc;
	struct Person_ **ptr;
	capn_resolve(&(list.p));
	nc = list.p.len;
	if (nc == 0) {
		(*d) = NULL;
		(*pcount) = 0;
		return;
	}
	ptr = (struct Person_ **)calloc(nc, sizeof(struct Person_ *));
	for(i = 0; i < nc; i ++) {
		struct Person s;
		get_Person(&s, list, i);
		ptr[i] = (struct Person_ *)calloc(1, sizeof(struct Person_));
		decode_Person(ptr[i], &s);
	}
	(*d) = ptr;
	(*pcount) = nc;
}
void decode_Person_ptr(struct Person_ **d,Person_ptr p) {
	struct Person s;
	capn_resolve(&(p.p));
	if (p.p.type == CAPN_NULL) {
		(*d) = NULL;
		return;
	}
	*d = (struct Person_ *)calloc(1, sizeof(struct Person_));
	read_Person(&s, p);
	decode_Person(*d, &s);
}
void free_Person_list(int pcount, struct Person_ **d) {
	int i;
	int nc = pcount;
	struct Person_ **ptr = d;
	if (ptr == NULL) return;
	for(i = 0; i < nc; i ++) {
		if(ptr[i] == NULL) continue;
		free_Person(ptr[i]);
		free(ptr[i]);
	}
	free(ptr);
}
void free_Person_ptr(struct Person_ **d){
	if((*d) == NULL) return;
	free_Person(*d);
	free(*d);
	(*d) = NULL;
}

Person_PhoneNumber_ptr new_Person_PhoneNumber(struct capn_segment *s) {
	Person_PhoneNumber_ptr p;
	p.p = capn_new_struct(s, 8, 1);
	return p;
}
Person_PhoneNumber_list new_Person_PhoneNumber_list(struct capn_segment *s, int len) {
	Person_PhoneNumber_list p;
	p.p = capn_new_list(s, len, 8, 1);
	return p;
}
void read_Person_PhoneNumber(struct Person_PhoneNumber *s capnp_unused, Person_PhoneNumber_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	s->number = capn_get_text(p.p, 0, capn_val0);
	s->type = (enum Person_PhoneNumber_Type)(int) capn_read16(p.p, 0);
}
void write_Person_PhoneNumber(const struct Person_PhoneNumber *s capnp_unused, Person_PhoneNumber_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	capn_set_text(p.p, 0, s->number);
	capn_write16(p.p, 0, (uint16_t) (s->type));
}
void get_Person_PhoneNumber(struct Person_PhoneNumber *s, Person_PhoneNumber_list l, int i) {
	Person_PhoneNumber_ptr p;
	p.p = capn_getp(l.p, i, 0);
	read_Person_PhoneNumber(s, p);
}
void set_Person_PhoneNumber(const struct Person_PhoneNumber *s, Person_PhoneNumber_list l, int i) {
	Person_PhoneNumber_ptr p;
	p.p = capn_getp(l.p, i, 0);
	write_Person_PhoneNumber(s, p);
}

capn_text Person_PhoneNumber_get_number(Person_PhoneNumber_ptr p)
{
	capn_text number;
	number = capn_get_text(p.p, 0, capn_val0);
	return number;
}

enum Person_PhoneNumber_Type Person_PhoneNumber_get_type(Person_PhoneNumber_ptr p)
{
	enum Person_PhoneNumber_Type type;
	type = (enum Person_PhoneNumber_Type)(int) capn_read16(p.p, 0);
	return type;
}

void Person_PhoneNumber_set_number(Person_PhoneNumber_ptr p, capn_text number)
{
	capn_set_text(p.p, 0, number);
}

void Person_PhoneNumber_set_type(Person_PhoneNumber_ptr p, enum Person_PhoneNumber_Type type)
{
	capn_write16(p.p, 0, (uint16_t) (type));
}
void encode_Person_PhoneNumber_list(struct capn_segment *cs, Person_PhoneNumber_list *l,int count,struct Person_PhoneNumber_ **s) {
	Person_PhoneNumber_list lst;
	int i;
	lst = new_Person_PhoneNumber_list(cs, count);
	for(i = 0; i < count; i ++) {
		struct Person_PhoneNumber d;
		encode_Person_PhoneNumber(cs, &d, s[i]);
		set_Person_PhoneNumber(&d, lst, i);
	}
	(*l) = lst;
}
void encode_Person_PhoneNumber_ptr(struct capn_segment *cs, Person_PhoneNumber_ptr *p,struct Person_PhoneNumber_ *s) {
	Person_PhoneNumber_ptr ptr;
	struct Person_PhoneNumber d;
	ptr = new_Person_PhoneNumber(cs);
	if (s == NULL) {
		ptr.p = capn_null;
	}
	else{
		encode_Person_PhoneNumber(cs, &d, s);
		write_Person_PhoneNumber(&d, ptr);
	}
	(*p) = ptr;
}
void decode_Person_PhoneNumber_list(int *pcount, struct Person_PhoneNumber_ ***d, Person_PhoneNumber_list list) {
	int i;
	int nc;
	struct Person_PhoneNumber_ **ptr;
	capn_resolve(&(list.p));
	nc = list.p.len;
	if (nc == 0) {
		(*d) = NULL;
		(*pcount) = 0;
		return;
	}
	ptr = (struct Person_PhoneNumber_ **)calloc(nc, sizeof(struct Person_PhoneNumber_ *));
	for(i = 0; i < nc; i ++) {
		struct Person_PhoneNumber s;
		get_Person_PhoneNumber(&s, list, i);
		ptr[i] = (struct Person_PhoneNumber_ *)calloc(1, sizeof(struct Person_PhoneNumber_));
		decode_Person_PhoneNumber(ptr[i], &s);
	}
	(*d) = ptr;
	(*pcount) = nc;
}
void decode_Person_PhoneNumber_ptr(struct Person_PhoneNumber_ **d,Person_PhoneNumber_ptr p) {
	struct Person_PhoneNumber s;
	capn_resolve(&(p.p));
	if (p.p.type == CAPN_NULL) {
		(*d) = NULL;
		return;
	}
	*d = (struct Person_PhoneNumber_ *)calloc(1, sizeof(struct Person_PhoneNumber_));
	read_Person_PhoneNumber(&s, p);
	decode_Person_PhoneNumber(*d, &s);
}
void free_Person_PhoneNumber_list(int pcount, struct Person_PhoneNumber_ **d) {
	int i;
	int nc = pcount;
	struct Person_PhoneNumber_ **ptr = d;
	if (ptr == NULL) return;
	for(i = 0; i < nc; i ++) {
		if(ptr[i] == NULL) continue;
		free_Person_PhoneNumber(ptr[i]);
		free(ptr[i]);
	}
	free(ptr);
}
void free_Person_PhoneNumber_ptr(struct Person_PhoneNumber_ **d){
	if((*d) == NULL) return;
	free_Person_PhoneNumber(*d);
	free(*d);
	(*d) = NULL;
}

AddressBook_ptr new_AddressBook(struct capn_segment *s) {
	AddressBook_ptr p;
	p.p = capn_new_struct(s, 0, 1);
	return p;
}
AddressBook_list new_AddressBook_list(struct capn_segment *s, int len) {
	AddressBook_list p;
	p.p = capn_new_list(s, len, 0, 1);
	return p;
}
void read_AddressBook(struct AddressBook *s capnp_unused, AddressBook_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	s->people.p = capn_getp(p.p, 0, 0);
}
void write_AddressBook(const struct AddressBook *s capnp_unused, AddressBook_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	capn_setp(p.p, 0, s->people.p);
}
void get_AddressBook(struct AddressBook *s, AddressBook_list l, int i) {
	AddressBook_ptr p;
	p.p = capn_getp(l.p, i, 0);
	read_AddressBook(s, p);
}
void set_AddressBook(const struct AddressBook *s, AddressBook_list l, int i) {
	AddressBook_ptr p;
	p.p = capn_getp(l.p, i, 0);
	write_AddressBook(s, p);
}

Person_list AddressBook_get_people(AddressBook_ptr p)
{
	Person_list people;
	people.p = capn_getp(p.p, 0, 0);
	return people;
}

void AddressBook_set_people(AddressBook_ptr p, Person_list people)
{
	capn_setp(p.p, 0, people.p);
}
void encode_AddressBook_list(struct capn_segment *cs, AddressBook_list *l,int count,struct AddressBook_ **s) {
	AddressBook_list lst;
	int i;
	lst = new_AddressBook_list(cs, count);
	for(i = 0; i < count; i ++) {
		struct AddressBook d;
		encode_AddressBook(cs, &d, s[i]);
		set_AddressBook(&d, lst, i);
	}
	(*l) = lst;
}
void encode_AddressBook_ptr(struct capn_segment *cs, AddressBook_ptr *p,struct AddressBook_ *s) {
	AddressBook_ptr ptr;
	struct AddressBook d;
	ptr = new_AddressBook(cs);
	if (s == NULL) {
		ptr.p = capn_null;
	}
	else{
		encode_AddressBook(cs, &d, s);
		write_AddressBook(&d, ptr);
	}
	(*p) = ptr;
}
void decode_AddressBook_list(int *pcount, struct AddressBook_ ***d, AddressBook_list list) {
	int i;
	int nc;
	struct AddressBook_ **ptr;
	capn_resolve(&(list.p));
	nc = list.p.len;
	if (nc == 0) {
		(*d) = NULL;
		(*pcount) = 0;
		return;
	}
	ptr = (struct AddressBook_ **)calloc(nc, sizeof(struct AddressBook_ *));
	for(i = 0; i < nc; i ++) {
		struct AddressBook s;
		get_AddressBook(&s, list, i);
		ptr[i] = (struct AddressBook_ *)calloc(1, sizeof(struct AddressBook_));
		decode_AddressBook(ptr[i], &s);
	}
	(*d) = ptr;
	(*pcount) = nc;
}
void decode_AddressBook_ptr(struct AddressBook_ **d,AddressBook_ptr p) {
	struct AddressBook s;
	capn_resolve(&(p.p));
	if (p.p.type == CAPN_NULL) {
		(*d) = NULL;
		return;
	}
	*d = (struct AddressBook_ *)calloc(1, sizeof(struct AddressBook_));
	read_AddressBook(&s, p);
	decode_AddressBook(*d, &s);
}
void free_AddressBook_list(int pcount, struct AddressBook_ **d) {
	int i;
	int nc = pcount;
	struct AddressBook_ **ptr = d;
	if (ptr == NULL) return;
	for(i = 0; i < nc; i ++) {
		if(ptr[i] == NULL) continue;
		free_AddressBook(ptr[i]);
		free(ptr[i]);
	}
	free(ptr);
}
void free_AddressBook_ptr(struct AddressBook_ **d){
	if((*d) == NULL) return;
	free_AddressBook(*d);
	free(*d);
	(*d) = NULL;
}
