/* autolink.h - versatile autolinker */

#ifndef HOEDOWN_AUTOLINK_H
#define HOEDOWN_AUTOLINK_H

#include "buffer.h"

#ifdef __cplusplus
extern "C" {
#endif


/*************
 * CONSTANTS *
 *************/

typedef enum hoedown_autolink_flags {
	HOEDOWN_AUTOLINK_SHORT_DOMAINS = (1 << 0)
} hoedown_autolink_flags;


/*************
 * FUNCTIONS *
 *************/

/* hoedown_autolink_is_safe: verify that a URL has a safe protocol */
int hoedown_autolink_is_safe(const uint8_t *data, size_t size);

/* hoedown_autolink__www: search for the next www link in data */
size_t hoedown_autolink__www(size_t *rewind_p, hoedown_buffer *link,
	uint8_t *data, size_t offset, size_t size, hoedown_autolink_flags flags);

/* hoedown_autolink__email: search for the next email in data */
size_t hoedown_autolink__email(size_t *rewind_p, hoedown_buffer *link,
	uint8_t *data, size_t offset, size_t size, hoedown_autolink_flags flags);

/* hoedown_autolink__url: search for the next URL in data */
size_t hoedown_autolink__url(size_t *rewind_p, hoedown_buffer *link,
	uint8_t *data, size_t offset, size_t size, hoedown_autolink_flags flags);


#ifdef __cplusplus
}
#endif

#endif /** HOEDOWN_AUTOLINK_H **/
