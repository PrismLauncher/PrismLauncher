#include "hoedown/html.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#ifdef _MSC_VER
#define snprintf _snprintf
#endif

struct smartypants_data {
	int in_squote;
	int in_dquote;
};

static size_t smartypants_cb__ltag(hoedown_buffer *ob, struct smartypants_data *smrt, uint8_t previous_char, const uint8_t *text, size_t size);
static size_t smartypants_cb__dquote(hoedown_buffer *ob, struct smartypants_data *smrt, uint8_t previous_char, const uint8_t *text, size_t size);
static size_t smartypants_cb__amp(hoedown_buffer *ob, struct smartypants_data *smrt, uint8_t previous_char, const uint8_t *text, size_t size);
static size_t smartypants_cb__period(hoedown_buffer *ob, struct smartypants_data *smrt, uint8_t previous_char, const uint8_t *text, size_t size);
static size_t smartypants_cb__number(hoedown_buffer *ob, struct smartypants_data *smrt, uint8_t previous_char, const uint8_t *text, size_t size);
static size_t smartypants_cb__dash(hoedown_buffer *ob, struct smartypants_data *smrt, uint8_t previous_char, const uint8_t *text, size_t size);
static size_t smartypants_cb__parens(hoedown_buffer *ob, struct smartypants_data *smrt, uint8_t previous_char, const uint8_t *text, size_t size);
static size_t smartypants_cb__squote(hoedown_buffer *ob, struct smartypants_data *smrt, uint8_t previous_char, const uint8_t *text, size_t size);
static size_t smartypants_cb__backtick(hoedown_buffer *ob, struct smartypants_data *smrt, uint8_t previous_char, const uint8_t *text, size_t size);
static size_t smartypants_cb__escape(hoedown_buffer *ob, struct smartypants_data *smrt, uint8_t previous_char, const uint8_t *text, size_t size);

static size_t (*smartypants_cb_ptrs[])
	(hoedown_buffer *, struct smartypants_data *, uint8_t, const uint8_t *, size_t) =
{
	NULL,					/* 0 */
	smartypants_cb__dash,	/* 1 */
	smartypants_cb__parens,	/* 2 */
	smartypants_cb__squote, /* 3 */
	smartypants_cb__dquote, /* 4 */
	smartypants_cb__amp,	/* 5 */
	smartypants_cb__period,	/* 6 */
	smartypants_cb__number,	/* 7 */
	smartypants_cb__ltag,	/* 8 */
	smartypants_cb__backtick, /* 9 */
	smartypants_cb__escape, /* 10 */
};

static const uint8_t smartypants_cb_chars[UINT8_MAX+1] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 4, 0, 0, 0, 5, 3, 2, 0, 0, 0, 0, 1, 6, 0,
	0, 7, 0, 7, 0, 0, 0, 0, 0, 0, 0, 0, 8, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 10, 0, 0, 0,
	9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

static int
word_boundary(uint8_t c)
{
	return c == 0 || isspace(c) || ispunct(c);
}

/*
	If 'text' begins with any kind of single quote (e.g. "'" or "&apos;" etc.),
	returns the length of the sequence of characters that makes up the single-
	quote.  Otherwise, returns zero.
*/
static size_t
squote_len(const uint8_t *text, size_t size)
{
	static char* single_quote_list[] = { "'", "&#39;", "&#x27;", "&apos;", NULL };
	char** p;

	for (p = single_quote_list; *p; ++p) {
		size_t len = strlen(*p);
		if (size >= len && memcmp(text, *p, len) == 0) {
			return len;
		}
	}

	return 0;
}

/* Converts " or ' at very beginning or end of a word to left or right quote */
static int
smartypants_quotes(hoedown_buffer *ob, uint8_t previous_char, uint8_t next_char, uint8_t quote, int *is_open)
{
	char ent[8];

	if (*is_open && !word_boundary(next_char))
		return 0;

	if (!(*is_open) && !word_boundary(previous_char))
		return 0;

	snprintf(ent, sizeof(ent), "&%c%cquo;", (*is_open) ? 'r' : 'l', quote);
	*is_open = !(*is_open);
	hoedown_buffer_puts(ob, ent);
	return 1;
}

/*
	Converts ' to left or right single quote; but the initial ' might be in
	different forms, e.g. &apos; or &#39; or &#x27;.
	'squote_text' points to the original single quote, and 'squote_size' is its length.
	'text' points at the last character of the single-quote, e.g. ' or ;
*/
static size_t
smartypants_squote(hoedown_buffer *ob, struct smartypants_data *smrt, uint8_t previous_char, const uint8_t *text, size_t size,
				   const uint8_t *squote_text, size_t squote_size)
{
	if (size >= 2) {
		uint8_t t1 = tolower(text[1]);
		size_t next_squote_len = squote_len(text+1, size-1);

		/* convert '' to &ldquo; or &rdquo; */
		if (next_squote_len > 0) {
			uint8_t next_char = (size > 1+next_squote_len) ? text[1+next_squote_len] : 0;
			if (smartypants_quotes(ob, previous_char, next_char, 'd', &smrt->in_dquote))
				return next_squote_len;
		}

		/* Tom's, isn't, I'm, I'd */
		if ((t1 == 's' || t1 == 't' || t1 == 'm' || t1 == 'd') &&
			(size == 3 || word_boundary(text[2]))) {
			HOEDOWN_BUFPUTSL(ob, "&rsquo;");
			return 0;
		}

		/* you're, you'll, you've */
		if (size >= 3) {
			uint8_t t2 = tolower(text[2]);

			if (((t1 == 'r' && t2 == 'e') ||
				(t1 == 'l' && t2 == 'l') ||
				(t1 == 'v' && t2 == 'e')) &&
				(size == 4 || word_boundary(text[3]))) {
				HOEDOWN_BUFPUTSL(ob, "&rsquo;");
				return 0;
			}
		}
	}

	if (smartypants_quotes(ob, previous_char, size > 0 ? text[1] : 0, 's', &smrt->in_squote))
		return 0;

	hoedown_buffer_put(ob, squote_text, squote_size);
	return 0;
}

/* Converts ' to left or right single quote. */
static size_t
smartypants_cb__squote(hoedown_buffer *ob, struct smartypants_data *smrt, uint8_t previous_char, const uint8_t *text, size_t size)
{
	return smartypants_squote(ob, smrt, previous_char, text, size, text, 1);
}

/* Converts (c), (r), (tm) */
static size_t
smartypants_cb__parens(hoedown_buffer *ob, struct smartypants_data *smrt, uint8_t previous_char, const uint8_t *text, size_t size)
{
	if (size >= 3) {
		uint8_t t1 = tolower(text[1]);
		uint8_t t2 = tolower(text[2]);

		if (t1 == 'c' && t2 == ')') {
			HOEDOWN_BUFPUTSL(ob, "&copy;");
			return 2;
		}

		if (t1 == 'r' && t2 == ')') {
			HOEDOWN_BUFPUTSL(ob, "&reg;");
			return 2;
		}

		if (size >= 4 && t1 == 't' && t2 == 'm' && text[3] == ')') {
			HOEDOWN_BUFPUTSL(ob, "&trade;");
			return 3;
		}
	}

	hoedown_buffer_putc(ob, text[0]);
	return 0;
}

/* Converts "--" to em-dash, etc. */
static size_t
smartypants_cb__dash(hoedown_buffer *ob, struct smartypants_data *smrt, uint8_t previous_char, const uint8_t *text, size_t size)
{
	if (size >= 3 && text[1] == '-' && text[2] == '-') {
		HOEDOWN_BUFPUTSL(ob, "&mdash;");
		return 2;
	}

	if (size >= 2 && text[1] == '-') {
		HOEDOWN_BUFPUTSL(ob, "&ndash;");
		return 1;
	}

	hoedown_buffer_putc(ob, text[0]);
	return 0;
}

/* Converts &quot; etc. */
static size_t
smartypants_cb__amp(hoedown_buffer *ob, struct smartypants_data *smrt, uint8_t previous_char, const uint8_t *text, size_t size)
{
	size_t len;
	if (size >= 6 && memcmp(text, "&quot;", 6) == 0) {
		if (smartypants_quotes(ob, previous_char, size >= 7 ? text[6] : 0, 'd', &smrt->in_dquote))
			return 5;
	}

	len = squote_len(text, size);
	if (len > 0) {
		return (len-1) + smartypants_squote(ob, smrt, previous_char, text+(len-1), size-(len-1), text, len);
	}

	if (size >= 4 && memcmp(text, "&#0;", 4) == 0)
		return 3;

	hoedown_buffer_putc(ob, '&');
	return 0;
}

/* Converts "..." to ellipsis */
static size_t
smartypants_cb__period(hoedown_buffer *ob, struct smartypants_data *smrt, uint8_t previous_char, const uint8_t *text, size_t size)
{
	if (size >= 3 && text[1] == '.' && text[2] == '.') {
		HOEDOWN_BUFPUTSL(ob, "&hellip;");
		return 2;
	}

	if (size >= 5 && text[1] == ' ' && text[2] == '.' && text[3] == ' ' && text[4] == '.') {
		HOEDOWN_BUFPUTSL(ob, "&hellip;");
		return 4;
	}

	hoedown_buffer_putc(ob, text[0]);
	return 0;
}

/* Converts `` to opening double quote */
static size_t
smartypants_cb__backtick(hoedown_buffer *ob, struct smartypants_data *smrt, uint8_t previous_char, const uint8_t *text, size_t size)
{
	if (size >= 2 && text[1] == '`') {
		if (smartypants_quotes(ob, previous_char, size >= 3 ? text[2] : 0, 'd', &smrt->in_dquote))
			return 1;
	}

	hoedown_buffer_putc(ob, text[0]);
	return 0;
}

/* Converts 1/2, 1/4, 3/4 */
static size_t
smartypants_cb__number(hoedown_buffer *ob, struct smartypants_data *smrt, uint8_t previous_char, const uint8_t *text, size_t size)
{
	if (word_boundary(previous_char) && size >= 3) {
		if (text[0] == '1' && text[1] == '/' && text[2] == '2') {
			if (size == 3 || word_boundary(text[3])) {
				HOEDOWN_BUFPUTSL(ob, "&frac12;");
				return 2;
			}
		}

		if (text[0] == '1' && text[1] == '/' && text[2] == '4') {
			if (size == 3 || word_boundary(text[3]) ||
				(size >= 5 && tolower(text[3]) == 't' && tolower(text[4]) == 'h')) {
				HOEDOWN_BUFPUTSL(ob, "&frac14;");
				return 2;
			}
		}

		if (text[0] == '3' && text[1] == '/' && text[2] == '4') {
			if (size == 3 || word_boundary(text[3]) ||
				(size >= 6 && tolower(text[3]) == 't' && tolower(text[4]) == 'h' && tolower(text[5]) == 's')) {
				HOEDOWN_BUFPUTSL(ob, "&frac34;");
				return 2;
			}
		}
	}

	hoedown_buffer_putc(ob, text[0]);
	return 0;
}

/* Converts " to left or right double quote */
static size_t
smartypants_cb__dquote(hoedown_buffer *ob, struct smartypants_data *smrt, uint8_t previous_char, const uint8_t *text, size_t size)
{
	if (!smartypants_quotes(ob, previous_char, size > 0 ? text[1] : 0, 'd', &smrt->in_dquote))
		HOEDOWN_BUFPUTSL(ob, "&quot;");

	return 0;
}

static size_t
smartypants_cb__ltag(hoedown_buffer *ob, struct smartypants_data *smrt, uint8_t previous_char, const uint8_t *text, size_t size)
{
	static const char *skip_tags[] = {
	  "pre", "code", "var", "samp", "kbd", "math", "script", "style"
	};
	static const size_t skip_tags_count = 8;

	size_t tag, i = 0;

	/* This is a comment. Copy everything verbatim until --> or EOF is seen. */
	if (i + 4 < size && memcmp(text, "<!--", 4) == 0) {
		i += 4;
		while (i + 3 < size && memcmp(text + i, "-->",  3) != 0)
			i++;
		i += 3;
		hoedown_buffer_put(ob, text, i + 1);
		return i;
	}

	while (i < size && text[i] != '>')
		i++;

	for (tag = 0; tag < skip_tags_count; ++tag) {
		if (hoedown_html_is_tag(text, size, skip_tags[tag]) == HOEDOWN_HTML_TAG_OPEN)
			break;
	}

	if (tag < skip_tags_count) {
		for (;;) {
			while (i < size && text[i] != '<')
				i++;

			if (i == size)
				break;

			if (hoedown_html_is_tag(text + i, size - i, skip_tags[tag]) == HOEDOWN_HTML_TAG_CLOSE)
				break;

			i++;
		}

		while (i < size && text[i] != '>')
			i++;
	}

	hoedown_buffer_put(ob, text, i + 1);
	return i;
}

static size_t
smartypants_cb__escape(hoedown_buffer *ob, struct smartypants_data *smrt, uint8_t previous_char, const uint8_t *text, size_t size)
{
	if (size < 2)
		return 0;

	switch (text[1]) {
	case '\\':
	case '"':
	case '\'':
	case '.':
	case '-':
	case '`':
		hoedown_buffer_putc(ob, text[1]);
		return 1;

	default:
		hoedown_buffer_putc(ob, '\\');
		return 0;
	}
}

#if 0
static struct {
    uint8_t c0;
    const uint8_t *pattern;
    const uint8_t *entity;
    int skip;
} smartypants_subs[] = {
    { '\'', "'s>",      "&rsquo;",  0 },
    { '\'', "'t>",      "&rsquo;",  0 },
    { '\'', "'re>",     "&rsquo;",  0 },
    { '\'', "'ll>",     "&rsquo;",  0 },
    { '\'', "'ve>",     "&rsquo;",  0 },
    { '\'', "'m>",      "&rsquo;",  0 },
    { '\'', "'d>",      "&rsquo;",  0 },
    { '-',  "--",       "&mdash;",  1 },
    { '-',  "<->",      "&ndash;",  0 },
    { '.',  "...",      "&hellip;", 2 },
    { '.',  ". . .",    "&hellip;", 4 },
    { '(',  "(c)",      "&copy;",   2 },
    { '(',  "(r)",      "&reg;",    2 },
    { '(',  "(tm)",     "&trade;",  3 },
    { '3',  "<3/4>",    "&frac34;", 2 },
    { '3',  "<3/4ths>", "&frac34;", 2 },
    { '1',  "<1/2>",    "&frac12;", 2 },
    { '1',  "<1/4>",    "&frac14;", 2 },
    { '1',  "<1/4th>",  "&frac14;", 2 },
    { '&',  "&#0;",      0,       3 },
};
#endif

void
hoedown_html_smartypants(hoedown_buffer *ob, const uint8_t *text, size_t size)
{
	size_t i;
	struct smartypants_data smrt = {0, 0};

	if (!text)
		return;

	hoedown_buffer_grow(ob, size);

	for (i = 0; i < size; ++i) {
		size_t org;
		uint8_t action = 0;

		org = i;
		while (i < size && (action = smartypants_cb_chars[text[i]]) == 0)
			i++;

		if (i > org)
			hoedown_buffer_put(ob, text + org, i - org);

		if (i < size) {
			i += smartypants_cb_ptrs[(int)action]
				(ob, &smrt, i ? text[i - 1] : 0, text + i, size - i);
		}
	}
}
