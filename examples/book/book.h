#if !defined(_BOOK_H_)

#define _BOOK_H_ 1

#include <stdint.h>
#include <string.h>

typedef struct {
    char *caption;
    int32_t start;
    int32_t end;
} chapter_t;

typedef struct {
    uint64_t isbn;
    uint32_t year;
} publish_t;
    
typedef struct {
    int null_;
} nulldata_t;

typedef struct {
    char *title;
    int   n_authors;
    char **authors;
    int   n_chapters;
    chapter_t **chapters_;
    publish_t *publish;
    nulldata_t *nulldata;
    int   n_magic1;
    uint32_t *magic_1;
    char *description;
    int   acquire_method;
    union {
	char *buy;
	char *donation;
    } acquire;
} book_t;

#endif
