#include <stdio.h>
#include <stdlib.h>

#include "book.capnp.h"
#include "book.h"

static void usage(char *app) {
    fprintf(stderr, "usage: %s encode | %s decode\n",
	    app, app);
}

int encode() {
    struct capn c;
    book_t book;
    char *title = "Book title";
    char *authors[2] = {
	"author1",
	"author2"
    };
    uint32_t magic1[2] = {
	1101,1012
    };
    chapter_t chapters_[3] = {
	{.caption ="Chapter1",
	 .start =1,
	 .end=99},
	{.caption = "Chapter2",
	 .start = 100,
	 .end = 150},
	{.caption = "Chapter3",
	 .start = 151,
	 .end=199}
    };
    chapter_t* chapters[3] = {
	&chapters_[0], &chapters_[1], &chapters_[2]
    };
    publish_t publish = {
	.isbn = 335677,
	.year =2001
    };
    struct capn_segment *cs;
    struct Book b;
    Book_ptr p;
    
    book.title = title;
    book.n_authors = 2;
    book.authors = authors;
    book.n_chapters = 3;
    book.chapters_ = &chapters[0];
    book.publish = &publish ;
    book.nulldata = NULL;
    book.n_magic1 = 2;
    book.magic_1 = &magic1[0];
    book.description = NULL;
    book.acquire_method = Book_acquire_buy;
    book.acquire.buy = "bought from Xinhua book store";

    capn_init_malloc(&c);
    cs = capn_root(&c).seg;

    encode_Book_ptr(cs, &p, &book);

    capn_setp(capn_root(&c), 0, p.p);

    capn_write_fd(&c, write, 1, 0);

    capn_free(&c);    

    return 0;
}

int decode() {
    struct capn c;
    Book_ptr p;
    book_t  *book;
    int i;

    capn_init_fp(&c, stdin, 0);
    p.p = capn_getp(capn_root(&c), 0, 1);

    decode_Book_ptr(&book, p);

    printf("title: %s\n", book->title);

    printf("authors(%d):\n", book->n_authors);
    
    for(i = 0; i < book->n_authors; i ++) {
	printf("\t%s\n", book->authors[i]);
    }
    
    printf("chapters(%d):\n", book->n_chapters);
    for(i = 0; i < book->n_chapters; i ++) {
	printf("\tcaption: %s\n", book->chapters_[i]->caption);
	printf("\tfrom %d to %d\n",
	       book->chapters_[i]->start,
	       book->chapters_[i]->end);
    }

    printf("ISBN: %lu year: %u\n",
	   book->publish->isbn,
	   book->publish->year);
    
    printf("magic1:\n");
    for(i = 0; i < book->n_magic1; i ++) {
	printf("\t%d\n", book->magic_1[i]);
    }

    if (book->acquire_method == Book_acquire_buy) {
	printf("%s\n", book->acquire.buy);
    }
    else {
	printf("%s\n", book->acquire.donation);
    }
    
    free_Book_ptr(&book);
    capn_free(&c);
    
    return 0;
}

int main(int argc,char *argv[]) {

    if (argc != 2) {
	usage(argv[0]);
	return -1;
    }

    if ( strcmp(argv[1],"encode") == 0) {
	encode();
    }
    else {
	decode();
    }
    
    return 0;
}
