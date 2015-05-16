/* buffer.h - simple, fast buffers */

#ifndef HOEDOWN_BUFFER_H
#define HOEDOWN_BUFFER_H

#include <stdio.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_MSC_VER)
#define __attribute__(x)
#define inline __inline
#define __builtin_expect(x,n) x
#endif


/*********
 * TYPES *
 *********/

typedef void *(*hoedown_realloc_callback)(void *, size_t);
typedef void (*hoedown_free_callback)(void *);

struct hoedown_buffer {
	uint8_t *data;	/* actual character data */
	size_t size;	/* size of the string */
	size_t asize;	/* allocated size (0 = volatile buffer) */
	size_t unit;	/* reallocation unit size (0 = read-only buffer) */

	hoedown_realloc_callback data_realloc;
	hoedown_free_callback data_free;
	hoedown_free_callback buffer_free;
};

typedef struct hoedown_buffer hoedown_buffer;


/*************
 * FUNCTIONS *
 *************/

/* allocation wrappers */
void *hoedown_malloc(size_t size) __attribute__ ((malloc));
void *hoedown_calloc(size_t nmemb, size_t size) __attribute__ ((malloc));
void *hoedown_realloc(void *ptr, size_t size) __attribute__ ((malloc));

/* hoedown_buffer_init: initialize a buffer with custom allocators */
void hoedown_buffer_init(
	hoedown_buffer *buffer,
	size_t unit,
	hoedown_realloc_callback data_realloc,
	hoedown_free_callback data_free,
	hoedown_free_callback buffer_free
);

/* hoedown_buffer_uninit: uninitialize an existing buffer */
void hoedown_buffer_uninit(hoedown_buffer *buf);

/* hoedown_buffer_new: allocate a new buffer */
hoedown_buffer *hoedown_buffer_new(size_t unit) __attribute__ ((malloc));

/* hoedown_buffer_reset: free internal data of the buffer */
void hoedown_buffer_reset(hoedown_buffer *buf);

/* hoedown_buffer_grow: increase the allocated size to the given value */
void hoedown_buffer_grow(hoedown_buffer *buf, size_t neosz);

/* hoedown_buffer_put: append raw data to a buffer */
void hoedown_buffer_put(hoedown_buffer *buf, const uint8_t *data, size_t size);

/* hoedown_buffer_puts: append a NUL-terminated string to a buffer */
void hoedown_buffer_puts(hoedown_buffer *buf, const char *str);

/* hoedown_buffer_putc: append a single char to a buffer */
void hoedown_buffer_putc(hoedown_buffer *buf, uint8_t c);

/* hoedown_buffer_putf: read from a file and append to a buffer, until EOF or error */
int hoedown_buffer_putf(hoedown_buffer *buf, FILE* file);

/* hoedown_buffer_set: replace the buffer's contents with raw data */
void hoedown_buffer_set(hoedown_buffer *buf, const uint8_t *data, size_t size);

/* hoedown_buffer_sets: replace the buffer's contents with a NUL-terminated string */
void hoedown_buffer_sets(hoedown_buffer *buf, const char *str);

/* hoedown_buffer_eq: compare a buffer's data with other data for equality */
int hoedown_buffer_eq(const hoedown_buffer *buf, const uint8_t *data, size_t size);

/* hoedown_buffer_eq: compare a buffer's data with NUL-terminated string for equality */
int hoedown_buffer_eqs(const hoedown_buffer *buf, const char *str);

/* hoedown_buffer_prefix: compare the beginning of a buffer with a string */
int hoedown_buffer_prefix(const hoedown_buffer *buf, const char *prefix);

/* hoedown_buffer_slurp: remove a given number of bytes from the head of the buffer */
void hoedown_buffer_slurp(hoedown_buffer *buf, size_t size);

/* hoedown_buffer_cstr: NUL-termination of the string array (making a C-string) */
const char *hoedown_buffer_cstr(hoedown_buffer *buf);

/* hoedown_buffer_printf: formatted printing to a buffer */
void hoedown_buffer_printf(hoedown_buffer *buf, const char *fmt, ...) __attribute__ ((format (printf, 2, 3)));

/* hoedown_buffer_put_utf8: put a Unicode character encoded as UTF-8 */
void hoedown_buffer_put_utf8(hoedown_buffer *buf, unsigned int codepoint);

/* hoedown_buffer_free: free the buffer */
void hoedown_buffer_free(hoedown_buffer *buf);


/* HOEDOWN_BUFPUTSL: optimized hoedown_buffer_puts of a string literal */
#define HOEDOWN_BUFPUTSL(output, literal) \
	hoedown_buffer_put(output, (const uint8_t *)literal, sizeof(literal) - 1)

/* HOEDOWN_BUFSETSL: optimized hoedown_buffer_sets of a string literal */
#define HOEDOWN_BUFSETSL(output, literal) \
	hoedown_buffer_set(output, (const uint8_t *)literal, sizeof(literal) - 1)

/* HOEDOWN_BUFEQSL: optimized hoedown_buffer_eqs of a string literal */
#define HOEDOWN_BUFEQSL(output, literal) \
	hoedown_buffer_eq(output, (const uint8_t *)literal, sizeof(literal) - 1)


#ifdef __cplusplus
}
#endif

#endif /** HOEDOWN_BUFFER_H **/
