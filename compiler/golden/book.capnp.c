#include "book.capnp.h"
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

Chapter_ptr new_Chapter(struct capn_segment *s) {
	Chapter_ptr p;
	p.p = capn_new_struct(s, 8, 1);
	return p;
}
Chapter_list new_Chapter_list(struct capn_segment *s, int len) {
	Chapter_list p;
	p.p = capn_new_list(s, len, 8, 1);
	return p;
}
void read_Chapter(struct Chapter *s capnp_unused, Chapter_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	s->caption = capn_get_text(p.p, 0, capn_val0);
	s->start = capn_read32(p.p, 0);
	s->end = capn_read32(p.p, 4);
}
void write_Chapter(const struct Chapter *s capnp_unused, Chapter_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	capn_set_text(p.p, 0, s->caption);
	capn_write32(p.p, 0, s->start);
	capn_write32(p.p, 4, s->end);
}
void get_Chapter(struct Chapter *s, Chapter_list l, int i) {
	Chapter_ptr p;
	p.p = capn_getp(l.p, i, 0);
	read_Chapter(s, p);
}
void set_Chapter(const struct Chapter *s, Chapter_list l, int i) {
	Chapter_ptr p;
	p.p = capn_getp(l.p, i, 0);
	write_Chapter(s, p);
}

void encode_Chapter(struct capn_segment *cs,struct Chapter *d, chapter_t *s) {
	if (s->caption != NULL) {
		d->caption.str = s->caption;
		d->caption.len = strlen(s->caption);
	}
	else{
		d->caption.str = "";
		d->caption.len = 0;
	}
	d->caption.seg = NULL;
	d->start = s->start;
	d->end = s->end;

}

void decode_Chapter(chapter_t *d, struct Chapter *s) {
	d->caption = STRING_DUP(s->caption.str);
	d->start = s->start;
	d->end = s->end;

}

void free_Chapter(chapter_t *d) {
	if (d->caption != NULL) {
		free(d->caption);
	}

}

capn_text Chapter_get_caption(Chapter_ptr p)
{
	capn_text caption;
	caption = capn_get_text(p.p, 0, capn_val0);
	return caption;
}

uint32_t Chapter_get_start(Chapter_ptr p)
{
	uint32_t start;
	start = capn_read32(p.p, 0);
	return start;
}

uint32_t Chapter_get_end(Chapter_ptr p)
{
	uint32_t end;
	end = capn_read32(p.p, 4);
	return end;
}

void Chapter_set_caption(Chapter_ptr p, capn_text caption)
{
	capn_set_text(p.p, 0, caption);
}

void Chapter_set_start(Chapter_ptr p, uint32_t start)
{
	capn_write32(p.p, 0, start);
}

void Chapter_set_end(Chapter_ptr p, uint32_t end)
{
	capn_write32(p.p, 4, end);
}
void encode_Chapter_list(struct capn_segment *cs, Chapter_list *l,int count,chapter_t **s) {
	Chapter_list lst;
	int i;
	lst = new_Chapter_list(cs, count);
	for(i = 0; i < count; i ++) {
		struct Chapter d;
		encode_Chapter(cs, &d, s[i]);
		set_Chapter(&d, lst, i);
	}
	(*l) = lst;
}
void encode_Chapter_ptr(struct capn_segment *cs, Chapter_ptr *p,chapter_t *s) {
	Chapter_ptr ptr;
	struct Chapter d;
	ptr = new_Chapter(cs);
	if (s == NULL) {
		ptr.p = capn_null;
	}
	else{
		encode_Chapter(cs, &d, s);
		write_Chapter(&d, ptr);
	}
	(*p) = ptr;
}
void decode_Chapter_list(int *pcount, chapter_t ***d, Chapter_list list) {
	int i;
	int nc;
	chapter_t **ptr;
	capn_resolve(&(list.p));
	nc = list.p.len;
	if (nc == 0) {
		(*d) = NULL;
		(*pcount) = 0;
		return;
	}
	ptr = (chapter_t **)calloc(nc, sizeof(chapter_t *));
	for(i = 0; i < nc; i ++) {
		struct Chapter s;
		get_Chapter(&s, list, i);
		ptr[i] = (chapter_t *)calloc(1, sizeof(chapter_t));
		decode_Chapter(ptr[i], &s);
	}
	(*d) = ptr;
	(*pcount) = nc;
}
void decode_Chapter_ptr(chapter_t **d,Chapter_ptr p) {
	struct Chapter s;
	capn_resolve(&(p.p));
	if (p.p.type == CAPN_NULL) {
		(*d) = NULL;
		return;
	}
	*d = (chapter_t *)calloc(1, sizeof(chapter_t));
	read_Chapter(&s, p);
	decode_Chapter(*d, &s);
}
void free_Chapter_list(int pcount, chapter_t **d) {
	int i;
	int nc = pcount;
	chapter_t **ptr = d;
	if (ptr == NULL) return;
	for(i = 0; i < nc; i ++) {
		if(ptr[i] == NULL) continue;
		free_Chapter(ptr[i]);
		free(ptr[i]);
	}
	free(ptr);
}
void free_Chapter_ptr(chapter_t **d){
	if((*d) == NULL) return;
	free_Chapter(*d);
	free(*d);
	(*d) = NULL;
}

Publish_ptr new_Publish(struct capn_segment *s) {
	Publish_ptr p;
	p.p = capn_new_struct(s, 16, 0);
	return p;
}
Publish_list new_Publish_list(struct capn_segment *s, int len) {
	Publish_list p;
	p.p = capn_new_list(s, len, 16, 0);
	return p;
}
void read_Publish(struct Publish *s capnp_unused, Publish_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	s->isbn = capn_read64(p.p, 0);
	s->year = capn_read32(p.p, 8);
}
void write_Publish(const struct Publish *s capnp_unused, Publish_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	capn_write64(p.p, 0, s->isbn);
	capn_write32(p.p, 8, s->year);
}
void get_Publish(struct Publish *s, Publish_list l, int i) {
	Publish_ptr p;
	p.p = capn_getp(l.p, i, 0);
	read_Publish(s, p);
}
void set_Publish(const struct Publish *s, Publish_list l, int i) {
	Publish_ptr p;
	p.p = capn_getp(l.p, i, 0);
	write_Publish(s, p);
}

void encode_Publish(struct capn_segment *cs,struct Publish *d, publish_t *s) {
	d->isbn = s->isbn;
	d->year = s->year;

}

void decode_Publish(publish_t *d, struct Publish *s) {
	d->isbn = s->isbn;
	d->year = s->year;

}

void free_Publish(publish_t *d) {

}

uint64_t Publish_get_isbn(Publish_ptr p)
{
	uint64_t isbn;
	isbn = capn_read64(p.p, 0);
	return isbn;
}

uint32_t Publish_get_year(Publish_ptr p)
{
	uint32_t year;
	year = capn_read32(p.p, 8);
	return year;
}

void Publish_set_isbn(Publish_ptr p, uint64_t isbn)
{
	capn_write64(p.p, 0, isbn);
}

void Publish_set_year(Publish_ptr p, uint32_t year)
{
	capn_write32(p.p, 8, year);
}
void encode_Publish_list(struct capn_segment *cs, Publish_list *l,int count,publish_t **s) {
	Publish_list lst;
	int i;
	lst = new_Publish_list(cs, count);
	for(i = 0; i < count; i ++) {
		struct Publish d;
		encode_Publish(cs, &d, s[i]);
		set_Publish(&d, lst, i);
	}
	(*l) = lst;
}
void encode_Publish_ptr(struct capn_segment *cs, Publish_ptr *p,publish_t *s) {
	Publish_ptr ptr;
	struct Publish d;
	ptr = new_Publish(cs);
	if (s == NULL) {
		ptr.p = capn_null;
	}
	else{
		encode_Publish(cs, &d, s);
		write_Publish(&d, ptr);
	}
	(*p) = ptr;
}
void decode_Publish_list(int *pcount, publish_t ***d, Publish_list list) {
	int i;
	int nc;
	publish_t **ptr;
	capn_resolve(&(list.p));
	nc = list.p.len;
	if (nc == 0) {
		(*d) = NULL;
		(*pcount) = 0;
		return;
	}
	ptr = (publish_t **)calloc(nc, sizeof(publish_t *));
	for(i = 0; i < nc; i ++) {
		struct Publish s;
		get_Publish(&s, list, i);
		ptr[i] = (publish_t *)calloc(1, sizeof(publish_t));
		decode_Publish(ptr[i], &s);
	}
	(*d) = ptr;
	(*pcount) = nc;
}
void decode_Publish_ptr(publish_t **d,Publish_ptr p) {
	struct Publish s;
	capn_resolve(&(p.p));
	if (p.p.type == CAPN_NULL) {
		(*d) = NULL;
		return;
	}
	*d = (publish_t *)calloc(1, sizeof(publish_t));
	read_Publish(&s, p);
	decode_Publish(*d, &s);
}
void free_Publish_list(int pcount, publish_t **d) {
	int i;
	int nc = pcount;
	publish_t **ptr = d;
	if (ptr == NULL) return;
	for(i = 0; i < nc; i ++) {
		if(ptr[i] == NULL) continue;
		free_Publish(ptr[i]);
		free(ptr[i]);
	}
	free(ptr);
}
void free_Publish_ptr(publish_t **d){
	if((*d) == NULL) return;
	free_Publish(*d);
	free(*d);
	(*d) = NULL;
}

Nulldata_ptr new_Nulldata(struct capn_segment *s) {
	Nulldata_ptr p;
	p.p = capn_new_struct(s, 8, 0);
	return p;
}
Nulldata_list new_Nulldata_list(struct capn_segment *s, int len) {
	Nulldata_list p;
	p.p = capn_new_list(s, len, 8, 0);
	return p;
}
void read_Nulldata(struct Nulldata *s capnp_unused, Nulldata_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	s->null = capn_read32(p.p, 0);
}
void write_Nulldata(const struct Nulldata *s capnp_unused, Nulldata_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	capn_write32(p.p, 0, s->null);
}
void get_Nulldata(struct Nulldata *s, Nulldata_list l, int i) {
	Nulldata_ptr p;
	p.p = capn_getp(l.p, i, 0);
	read_Nulldata(s, p);
}
void set_Nulldata(const struct Nulldata *s, Nulldata_list l, int i) {
	Nulldata_ptr p;
	p.p = capn_getp(l.p, i, 0);
	write_Nulldata(s, p);
}

void encode_Nulldata(struct capn_segment *cs,struct Nulldata *d, nulldata_t *s) {
	d->null = s->null_;

}

void decode_Nulldata(nulldata_t *d, struct Nulldata *s) {
	d->null_ = s->null;

}

void free_Nulldata(nulldata_t *d) {

}

uint32_t Nulldata_get_null(Nulldata_ptr p)
{
	uint32_t null;
	null = capn_read32(p.p, 0);
	return null;
}

void Nulldata_set_null(Nulldata_ptr p, uint32_t null)
{
	capn_write32(p.p, 0, null);
}
void encode_Nulldata_list(struct capn_segment *cs, Nulldata_list *l,int count,nulldata_t **s) {
	Nulldata_list lst;
	int i;
	lst = new_Nulldata_list(cs, count);
	for(i = 0; i < count; i ++) {
		struct Nulldata d;
		encode_Nulldata(cs, &d, s[i]);
		set_Nulldata(&d, lst, i);
	}
	(*l) = lst;
}
void encode_Nulldata_ptr(struct capn_segment *cs, Nulldata_ptr *p,nulldata_t *s) {
	Nulldata_ptr ptr;
	struct Nulldata d;
	ptr = new_Nulldata(cs);
	if (s == NULL) {
		ptr.p = capn_null;
	}
	else{
		encode_Nulldata(cs, &d, s);
		write_Nulldata(&d, ptr);
	}
	(*p) = ptr;
}
void decode_Nulldata_list(int *pcount, nulldata_t ***d, Nulldata_list list) {
	int i;
	int nc;
	nulldata_t **ptr;
	capn_resolve(&(list.p));
	nc = list.p.len;
	if (nc == 0) {
		(*d) = NULL;
		(*pcount) = 0;
		return;
	}
	ptr = (nulldata_t **)calloc(nc, sizeof(nulldata_t *));
	for(i = 0; i < nc; i ++) {
		struct Nulldata s;
		get_Nulldata(&s, list, i);
		ptr[i] = (nulldata_t *)calloc(1, sizeof(nulldata_t));
		decode_Nulldata(ptr[i], &s);
	}
	(*d) = ptr;
	(*pcount) = nc;
}
void decode_Nulldata_ptr(nulldata_t **d,Nulldata_ptr p) {
	struct Nulldata s;
	capn_resolve(&(p.p));
	if (p.p.type == CAPN_NULL) {
		(*d) = NULL;
		return;
	}
	*d = (nulldata_t *)calloc(1, sizeof(nulldata_t));
	read_Nulldata(&s, p);
	decode_Nulldata(*d, &s);
}
void free_Nulldata_list(int pcount, nulldata_t **d) {
	int i;
	int nc = pcount;
	nulldata_t **ptr = d;
	if (ptr == NULL) return;
	for(i = 0; i < nc; i ++) {
		if(ptr[i] == NULL) continue;
		free_Nulldata(ptr[i]);
		free(ptr[i]);
	}
	free(ptr);
}
void free_Nulldata_ptr(nulldata_t **d){
	if((*d) == NULL) return;
	free_Nulldata(*d);
	free(*d);
	(*d) = NULL;
}

Buy_ptr new_Buy(struct capn_segment *s) {
	Buy_ptr p;
	p.p = capn_new_struct(s, 8, 2);
	return p;
}
Buy_list new_Buy_list(struct capn_segment *s, int len) {
	Buy_list p;
	p.p = capn_new_list(s, len, 8, 2);
	return p;
}
void read_Buy(struct Buy *s capnp_unused, Buy_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	s->from = capn_get_text(p.p, 0, capn_val0);
	s->u_which = (enum Buy_u_which)(int) capn_read16(p.p, 0);
	switch (s->u_which) {
	case Buy_u_recipeAddr:
		s->u.recipeAddr = capn_get_text(p.p, 1, capn_val0);
		break;
	default:
		break;
	}
}
void write_Buy(const struct Buy *s capnp_unused, Buy_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	capn_set_text(p.p, 0, s->from);
	capn_write16(p.p, 0, s->u_which);
	switch (s->u_which) {
	case Buy_u_recipeAddr:
		capn_set_text(p.p, 1, s->u.recipeAddr);
		break;
	default:
		break;
	}
}
void get_Buy(struct Buy *s, Buy_list l, int i) {
	Buy_ptr p;
	p.p = capn_getp(l.p, i, 0);
	read_Buy(s, p);
}
void set_Buy(const struct Buy *s, Buy_list l, int i) {
	Buy_ptr p;
	p.p = capn_getp(l.p, i, 0);
	write_Buy(s, p);
}

void encode_Buy(struct capn_segment *cs,struct Buy *d, buy_t *s) {
	if (s->from != NULL) {
		d->from.str = s->from;
		d->from.len = strlen(s->from);
	}
	else{
		d->from.str = "";
		d->from.len = 0;
	}
	d->from.seg = NULL;
	d->u_which = s->with_recipe;
	switch (d->u_which) {
	case Buy_u_recipeAddr:
		if (s->u.recipe_addr != NULL) {
			d->u.recipeAddr.str = s->u.recipe_addr;
			d->u.recipeAddr.len = strlen(s->u.recipe_addr);
		}
		else{
			d->u.recipeAddr.str = "";
			d->u.recipeAddr.len = 0;
		}
		d->u.recipeAddr.seg = NULL;
		break;
	default:
		break;
	}

}

void decode_Buy(buy_t *d, struct Buy *s) {
	d->from = STRING_DUP(s->from.str);
	d->with_recipe = s->u_which;
	switch (s->u_which) {
	case Buy_u_recipeAddr:
		d->u.recipe_addr = STRING_DUP(s->u.recipeAddr.str);
		break;
	default:
		break;
	}

}

void free_Buy(buy_t *d) {
	if (d->from != NULL) {
		free(d->from);
	}
	switch (d->with_recipe) {
	case Buy_u_recipeAddr:
		if (d->u.recipe_addr != NULL) {
			free(d->u.recipe_addr);
		}
		break;
	default:
		break;
	}

}

capn_text Buy_get_from(Buy_ptr p)
{
	capn_text from;
	from = capn_get_text(p.p, 0, capn_val0);
	return from;
}

void Buy_set_from(Buy_ptr p, capn_text from)
{
	capn_set_text(p.p, 0, from);
}
void encode_Buy_list(struct capn_segment *cs, Buy_list *l,int count,buy_t **s) {
	Buy_list lst;
	int i;
	lst = new_Buy_list(cs, count);
	for(i = 0; i < count; i ++) {
		struct Buy d;
		encode_Buy(cs, &d, s[i]);
		set_Buy(&d, lst, i);
	}
	(*l) = lst;
}
void encode_Buy_ptr(struct capn_segment *cs, Buy_ptr *p,buy_t *s) {
	Buy_ptr ptr;
	struct Buy d;
	ptr = new_Buy(cs);
	if (s == NULL) {
		ptr.p = capn_null;
	}
	else{
		encode_Buy(cs, &d, s);
		write_Buy(&d, ptr);
	}
	(*p) = ptr;
}
void decode_Buy_list(int *pcount, buy_t ***d, Buy_list list) {
	int i;
	int nc;
	buy_t **ptr;
	capn_resolve(&(list.p));
	nc = list.p.len;
	if (nc == 0) {
		(*d) = NULL;
		(*pcount) = 0;
		return;
	}
	ptr = (buy_t **)calloc(nc, sizeof(buy_t *));
	for(i = 0; i < nc; i ++) {
		struct Buy s;
		get_Buy(&s, list, i);
		ptr[i] = (buy_t *)calloc(1, sizeof(buy_t));
		decode_Buy(ptr[i], &s);
	}
	(*d) = ptr;
	(*pcount) = nc;
}
void decode_Buy_ptr(buy_t **d,Buy_ptr p) {
	struct Buy s;
	capn_resolve(&(p.p));
	if (p.p.type == CAPN_NULL) {
		(*d) = NULL;
		return;
	}
	*d = (buy_t *)calloc(1, sizeof(buy_t));
	read_Buy(&s, p);
	decode_Buy(*d, &s);
}
void free_Buy_list(int pcount, buy_t **d) {
	int i;
	int nc = pcount;
	buy_t **ptr = d;
	if (ptr == NULL) return;
	for(i = 0; i < nc; i ++) {
		if(ptr[i] == NULL) continue;
		free_Buy(ptr[i]);
		free(ptr[i]);
	}
	free(ptr);
}
void free_Buy_ptr(buy_t **d){
	if((*d) == NULL) return;
	free_Buy(*d);
	free(*d);
	(*d) = NULL;
}

Book_ptr new_Book(struct capn_segment *s) {
	Book_ptr p;
	p.p = capn_new_struct(s, 8, 8);
	return p;
}
Book_list new_Book_list(struct capn_segment *s, int len) {
	Book_list p;
	p.p = capn_new_list(s, len, 8, 8);
	return p;
}
void read_Book(struct Book *s capnp_unused, Book_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	s->title = capn_get_text(p.p, 0, capn_val0);
	s->authors = capn_getp(p.p, 1, 0);
	s->chapters.p = capn_getp(p.p, 4, 0);
	s->publish.p = capn_getp(p.p, 5, 0);
	s->nulldata.p = capn_getp(p.p, 6, 0);
	s->magic1.p = capn_getp(p.p, 2, 0);
	s->description = capn_get_text(p.p, 7, capn_val0);
	s->acquire_which = (enum Book_acquire_which)(int) capn_read16(p.p, 0);
	switch (s->acquire_which) {
	case Book_acquire_donation:
		s->acquire.donation = capn_get_text(p.p, 3, capn_val0);
		break;
	case Book_acquire_buy:
		s->acquire.buy.p = capn_getp(p.p, 3, 0);
		break;
	default:
		break;
	}
}
void write_Book(const struct Book *s capnp_unused, Book_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	capn_set_text(p.p, 0, s->title);
	capn_setp(p.p, 1, s->authors);
	capn_setp(p.p, 4, s->chapters.p);
	capn_setp(p.p, 5, s->publish.p);
	capn_setp(p.p, 6, s->nulldata.p);
	capn_setp(p.p, 2, s->magic1.p);
	capn_set_text(p.p, 7, s->description);
	capn_write16(p.p, 0, s->acquire_which);
	switch (s->acquire_which) {
	case Book_acquire_donation:
		capn_set_text(p.p, 3, s->acquire.donation);
		break;
	case Book_acquire_buy:
		capn_setp(p.p, 3, s->acquire.buy.p);
		break;
	default:
		break;
	}
}
void get_Book(struct Book *s, Book_list l, int i) {
	Book_ptr p;
	p.p = capn_getp(l.p, i, 0);
	read_Book(s, p);
}
void set_Book(const struct Book *s, Book_list l, int i) {
	Book_ptr p;
	p.p = capn_getp(l.p, i, 0);
	write_Book(s, p);
}

void encode_Book(struct capn_segment *cs,struct Book *d, book_t *s) {
	if (s->title != NULL) {
		d->title.str = s->title;
		d->title.len = strlen(s->title);
	}
	else{
		d->title.str = "";
		d->title.len = 0;
	}
	d->title.seg = NULL;
		if (1) {
		int i_;
		d->authors = capn_new_ptr_list(cs, s->n_authors);
		for(i_ = 0; i_ < s->n_authors; i_ ++) {
			capn_text text_ = {.str = s->authors[i_], .len = strlen(s->authors[i_]),.seg = NULL};
			capn_set_text(d->authors, i_, text_);
		}
	}
	encode_Chapter_list(cs, &(d->chapters), s->n_chapters, s->chapters_);
	encode_Publish_ptr(cs, &(d->publish), s->publish);
	encode_Nulldata_ptr(cs, &(d->nulldata), s->nulldata);
		if (1) {
		int i_;
		d->magic1 = capn_new_list32(cs, s->n_magic1);
		for(i_ = 0; i_ < s->n_magic1; i_ ++) {
			capn_set32(d->magic1, i_, s->magic_1[i_]);
		}
	}
	if (s->description != NULL) {
		d->description.str = s->description;
		d->description.len = strlen(s->description);
	}
	else{
		d->description.str = "";
		d->description.len = 0;
	}
	d->description.seg = NULL;
	d->acquire_which = s->acquire_method;
	switch (d->acquire_which) {
	case Book_acquire_donation:
		if (s->acquire.donation != NULL) {
			d->acquire.donation.str = s->acquire.donation;
			d->acquire.donation.len = strlen(s->acquire.donation);
		}
		else{
			d->acquire.donation.str = "";
			d->acquire.donation.len = 0;
		}
		d->acquire.donation.seg = NULL;
		break;
	case Book_acquire_buy:
		encode_Buy_ptr(cs, &(d->acquire.buy), s->acquire.buy);
		break;
	default:
		break;
	}

}

void decode_Book(book_t *d, struct Book *s) {
	d->title = STRING_DUP(s->title.str);
		if (1) {
		int i_, nc_;
		capn_resolve(&(s->authors));
		nc_ = s->authors.len;
		if (nc_ == 0) {
			d->authors = NULL;
		}
		else {
			d->authors = (char **)calloc(nc_, sizeof(char *));
			for(i_ = 0; i_ < nc_; i_ ++) {
				capn_text text_ = capn_get_text(s->authors, i_, capn_val0);
				d->authors[i_] = STRING_DUP(text_.str);
			}
		}
	d->n_authors = nc_;
	}
	decode_Chapter_list(&(d->n_chapters), &(d->chapters_), s->chapters);
	decode_Publish_ptr(&(d->publish), s->publish);
	decode_Nulldata_ptr(&(d->nulldata), s->nulldata);
		if (1) {
		int i_, nc_;
		capn_resolve(&(s->magic1.p));
		nc_ = s->magic1.p.len;
		if (nc_ == 0) {
			d->magic_1 = NULL;
		}
		else {
			d->magic_1 = (uint32_t *)calloc(nc_, sizeof(uint32_t));
			for(i_ = 0; i_ < nc_; i_ ++) {
				d->magic_1[i_] = capn_get32(s->magic1, i_);
			}
		}
	d->n_magic1 = nc_;
	}
	d->description = STRING_DUP(s->description.str);
	d->acquire_method = s->acquire_which;
	switch (s->acquire_which) {
	case Book_acquire_donation:
		d->acquire.donation = STRING_DUP(s->acquire.donation.str);
		break;
	case Book_acquire_buy:
		decode_Buy_ptr(&(d->acquire.buy), s->acquire.buy);
		break;
	default:
		break;
	}

}

void free_Book(book_t *d) {
	if (d->title != NULL) {
		free(d->title);
	}
		if (1) {
		int i_, nc_ = d->n_authors;
		capnp_use(i_);capnp_use(nc_);
		for(i_ = 0; i_ < nc_; i_ ++) {
			if (d->authors[i_] == NULL) continue;
			free(d->authors[i_]);
		}
		free(d->authors);
	}
	free_Chapter_list(d->n_chapters, d->chapters_);
	free_Publish_ptr(&(d->publish));
	free_Nulldata_ptr(&(d->nulldata));
		if (1) {
		int i_, nc_ = d->n_magic1;
		capnp_use(i_);capnp_use(nc_);
		free(d->magic_1);
	}
	if (d->description != NULL) {
		free(d->description);
	}
	switch (d->acquire_method) {
	case Book_acquire_donation:
		if (d->acquire.donation != NULL) {
			free(d->acquire.donation);
		}
		break;
	case Book_acquire_buy:
		free_Buy_ptr(&(d->acquire.buy));
		break;
	default:
		break;
	}

}

capn_text Book_get_title(Book_ptr p)
{
	capn_text title;
	title = capn_get_text(p.p, 0, capn_val0);
	return title;
}

capn_ptr Book_get_authors(Book_ptr p)
{
	capn_ptr authors;
	authors = capn_getp(p.p, 1, 0);
	return authors;
}

Chapter_list Book_get_chapters(Book_ptr p)
{
	Chapter_list chapters;
	chapters.p = capn_getp(p.p, 4, 0);
	return chapters;
}

Publish_ptr Book_get_publish(Book_ptr p)
{
	Publish_ptr publish;
	publish.p = capn_getp(p.p, 5, 0);
	return publish;
}

Nulldata_ptr Book_get_nulldata(Book_ptr p)
{
	Nulldata_ptr nulldata;
	nulldata.p = capn_getp(p.p, 6, 0);
	return nulldata;
}

capn_list32 Book_get_magic1(Book_ptr p)
{
	capn_list32 magic1;
	magic1.p = capn_getp(p.p, 2, 0);
	return magic1;
}

capn_text Book_get_description(Book_ptr p)
{
	capn_text description;
	description = capn_get_text(p.p, 7, capn_val0);
	return description;
}

void Book_set_title(Book_ptr p, capn_text title)
{
	capn_set_text(p.p, 0, title);
}

void Book_set_authors(Book_ptr p, capn_ptr authors)
{
	capn_setp(p.p, 1, authors);
}

void Book_set_chapters(Book_ptr p, Chapter_list chapters)
{
	capn_setp(p.p, 4, chapters.p);
}

void Book_set_publish(Book_ptr p, Publish_ptr publish)
{
	capn_setp(p.p, 5, publish.p);
}

void Book_set_nulldata(Book_ptr p, Nulldata_ptr nulldata)
{
	capn_setp(p.p, 6, nulldata.p);
}

void Book_set_magic1(Book_ptr p, capn_list32 magic1)
{
	capn_setp(p.p, 2, magic1.p);
}

void Book_set_description(Book_ptr p, capn_text description)
{
	capn_set_text(p.p, 7, description);
}
void encode_Book_list(struct capn_segment *cs, Book_list *l,int count,book_t **s) {
	Book_list lst;
	int i;
	lst = new_Book_list(cs, count);
	for(i = 0; i < count; i ++) {
		struct Book d;
		encode_Book(cs, &d, s[i]);
		set_Book(&d, lst, i);
	}
	(*l) = lst;
}
void encode_Book_ptr(struct capn_segment *cs, Book_ptr *p,book_t *s) {
	Book_ptr ptr;
	struct Book d;
	ptr = new_Book(cs);
	if (s == NULL) {
		ptr.p = capn_null;
	}
	else{
		encode_Book(cs, &d, s);
		write_Book(&d, ptr);
	}
	(*p) = ptr;
}
void decode_Book_list(int *pcount, book_t ***d, Book_list list) {
	int i;
	int nc;
	book_t **ptr;
	capn_resolve(&(list.p));
	nc = list.p.len;
	if (nc == 0) {
		(*d) = NULL;
		(*pcount) = 0;
		return;
	}
	ptr = (book_t **)calloc(nc, sizeof(book_t *));
	for(i = 0; i < nc; i ++) {
		struct Book s;
		get_Book(&s, list, i);
		ptr[i] = (book_t *)calloc(1, sizeof(book_t));
		decode_Book(ptr[i], &s);
	}
	(*d) = ptr;
	(*pcount) = nc;
}
void decode_Book_ptr(book_t **d,Book_ptr p) {
	struct Book s;
	capn_resolve(&(p.p));
	if (p.p.type == CAPN_NULL) {
		(*d) = NULL;
		return;
	}
	*d = (book_t *)calloc(1, sizeof(book_t));
	read_Book(&s, p);
	decode_Book(*d, &s);
}
void free_Book_list(int pcount, book_t **d) {
	int i;
	int nc = pcount;
	book_t **ptr = d;
	if (ptr == NULL) return;
	for(i = 0; i < nc; i ++) {
		if(ptr[i] == NULL) continue;
		free_Book(ptr[i]);
		free(ptr[i]);
	}
	free(ptr);
}
void free_Book_ptr(book_t **d){
	if((*d) == NULL) return;
	free_Book(*d);
	free(*d);
	(*d) = NULL;
}
