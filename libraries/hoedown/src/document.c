#include "hoedown/document.h"

#include <assert.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>

#include "hoedown/stack.h"

#ifndef _MSC_VER
#include <strings.h>
#else
#define strncasecmp    _strnicmp
#endif

#define REF_TABLE_SIZE 8

#define BUFFER_BLOCK 0
#define BUFFER_SPAN 1

#define HOEDOWN_LI_END 8    /* internal list flag */

const char *hoedown_find_block_tag(const char *str, unsigned int len);

/***************
 * LOCAL TYPES *
 ***************/

/* link_ref: reference to a link */
struct link_ref {
    unsigned int id;

    hoedown_buffer *link;
    hoedown_buffer *title;

    struct link_ref *next;
};

/* footnote_ref: reference to a footnote */
struct footnote_ref {
    unsigned int id;

    int is_used;
    unsigned int num;

    hoedown_buffer *contents;
};

/* footnote_item: an item in a footnote_list */
struct footnote_item {
    struct footnote_ref *ref;
    struct footnote_item *next;
};

/* footnote_list: linked list of footnote_item */
struct footnote_list {
    unsigned int count;
    struct footnote_item *head;
    struct footnote_item *tail;
};

/* char_trigger: function pointer to render active chars */
/*   returns the number of chars taken care of */
/*   data is the pointer of the beginning of the span */
/*   offset is the number of valid chars before data */
typedef size_t
(*char_trigger)(hoedown_buffer *ob, hoedown_document *doc, uint8_t *data, size_t offset, size_t size);

static size_t char_emphasis(hoedown_buffer *ob, hoedown_document *doc, uint8_t *data, size_t offset, size_t size);
static size_t char_quote(hoedown_buffer *ob, hoedown_document *doc, uint8_t *data, size_t offset, size_t size);
static size_t char_linebreak(hoedown_buffer *ob, hoedown_document *doc, uint8_t *data, size_t offset, size_t size);
static size_t char_codespan(hoedown_buffer *ob, hoedown_document *doc, uint8_t *data, size_t offset, size_t size);
static size_t char_escape(hoedown_buffer *ob, hoedown_document *doc, uint8_t *data, size_t offset, size_t size);
static size_t char_entity(hoedown_buffer *ob, hoedown_document *doc, uint8_t *data, size_t offset, size_t size);
static size_t char_langle_tag(hoedown_buffer *ob, hoedown_document *doc, uint8_t *data, size_t offset, size_t size);
static size_t char_autolink_url(hoedown_buffer *ob, hoedown_document *doc, uint8_t *data, size_t offset, size_t size);
static size_t char_autolink_email(hoedown_buffer *ob, hoedown_document *doc, uint8_t *data, size_t offset, size_t size);
static size_t char_autolink_www(hoedown_buffer *ob, hoedown_document *doc, uint8_t *data, size_t offset, size_t size);
static size_t char_link(hoedown_buffer *ob, hoedown_document *doc, uint8_t *data, size_t offset, size_t size);
static size_t char_superscript(hoedown_buffer *ob, hoedown_document *doc, uint8_t *data, size_t offset, size_t size);
static size_t char_math(hoedown_buffer *ob, hoedown_document *doc, uint8_t *data, size_t offset, size_t size);

enum markdown_char_t {
    MD_CHAR_NONE = 0,
    MD_CHAR_EMPHASIS,
    MD_CHAR_CODESPAN,
    MD_CHAR_LINEBREAK,
    MD_CHAR_LINK,
    MD_CHAR_LANGLE,
    MD_CHAR_ESCAPE,
    MD_CHAR_ENTITY,
    MD_CHAR_AUTOLINK_URL,
    MD_CHAR_AUTOLINK_EMAIL,
    MD_CHAR_AUTOLINK_WWW,
    MD_CHAR_SUPERSCRIPT,
    MD_CHAR_QUOTE,
    MD_CHAR_MATH
};

static char_trigger markdown_char_ptrs[] = {
    NULL,
    &char_emphasis,
    &char_codespan,
    &char_linebreak,
    &char_link,
    &char_langle_tag,
    &char_escape,
    &char_entity,
    &char_autolink_url,
    &char_autolink_email,
    &char_autolink_www,
    &char_superscript,
    &char_quote,
    &char_math
};

struct hoedown_document {
    hoedown_renderer md;
    hoedown_renderer_data data;

    struct link_ref *refs[REF_TABLE_SIZE];
    struct footnote_list footnotes_found;
    struct footnote_list footnotes_used;
    uint8_t active_char[256];
    hoedown_stack work_bufs[2];
    hoedown_extensions ext_flags;
    size_t max_nesting;
    int in_link_body;
};

/***************************
 * HELPER FUNCTIONS *
 ***************************/

static hoedown_buffer *
newbuf(hoedown_document *doc, int type)
{
    static const size_t buf_size[2] = {256, 64};
    hoedown_buffer *work = NULL;
    hoedown_stack *pool = &doc->work_bufs[type];

    if (pool->size < pool->asize &&
        pool->item[pool->size] != NULL) {
        work = pool->item[pool->size++];
        work->size = 0;
    } else {
        work = hoedown_buffer_new(buf_size[type]);
        hoedown_stack_push(pool, work);
    }

    return work;
}

static void
popbuf(hoedown_document *doc, int type)
{
    doc->work_bufs[type].size--;
}

static void
unscape_text(hoedown_buffer *ob, hoedown_buffer *src)
{
    size_t i = 0, org;
    while (i < src->size) {
        org = i;
        while (i < src->size && src->data[i] != '\\')
            i++;

        if (i > org)
            hoedown_buffer_put(ob, src->data + org, i - org);

        if (i + 1 >= src->size)
            break;

        hoedown_buffer_putc(ob, src->data[i + 1]);
        i += 2;
    }
}

static unsigned int
hash_link_ref(const uint8_t *link_ref, size_t length)
{
    size_t i;
    unsigned int hash = 0;

    for (i = 0; i < length; ++i)
        hash = tolower(link_ref[i]) + (hash << 6) + (hash << 16) - hash;

    return hash;
}

static struct link_ref *
add_link_ref(
    struct link_ref **references,
    const uint8_t *name, size_t name_size)
{
    struct link_ref *ref = hoedown_calloc(1, sizeof(struct link_ref));

    ref->id = hash_link_ref(name, name_size);
    ref->next = references[ref->id % REF_TABLE_SIZE];

    references[ref->id % REF_TABLE_SIZE] = ref;
    return ref;
}

static struct link_ref *
find_link_ref(struct link_ref **references, uint8_t *name, size_t length)
{
    unsigned int hash = hash_link_ref(name, length);
    struct link_ref *ref = NULL;

    ref = references[hash % REF_TABLE_SIZE];

    while (ref != NULL) {
        if (ref->id == hash)
            return ref;

        ref = ref->next;
    }

    return NULL;
}

static void
free_link_refs(struct link_ref **references)
{
    size_t i;

    for (i = 0; i < REF_TABLE_SIZE; ++i) {
        struct link_ref *r = references[i];
        struct link_ref *next;

        while (r) {
            next = r->next;
            hoedown_buffer_free(r->link);
            hoedown_buffer_free(r->title);
            free(r);
            r = next;
        }
    }
}

static struct footnote_ref *
create_footnote_ref(struct footnote_list *list, const uint8_t *name, size_t name_size)
{
    struct footnote_ref *ref = hoedown_calloc(1, sizeof(struct footnote_ref));

    ref->id = hash_link_ref(name, name_size);

    return ref;
}

static int
add_footnote_ref(struct footnote_list *list, struct footnote_ref *ref)
{
    struct footnote_item *item = hoedown_calloc(1, sizeof(struct footnote_item));
    if (!item)
        return 0;
    item->ref = ref;

    if (list->head == NULL) {
        list->head = list->tail = item;
    } else {
        list->tail->next = item;
        list->tail = item;
    }
    list->count++;

    return 1;
}

static struct footnote_ref *
find_footnote_ref(struct footnote_list *list, uint8_t *name, size_t length)
{
    unsigned int hash = hash_link_ref(name, length);
    struct footnote_item *item = NULL;

    item = list->head;

    while (item != NULL) {
        if (item->ref->id == hash)
            return item->ref;
        item = item->next;
    }

    return NULL;
}

static void
free_footnote_ref(struct footnote_ref *ref)
{
    hoedown_buffer_free(ref->contents);
    free(ref);
}

static void
free_footnote_list(struct footnote_list *list, int free_refs)
{
    struct footnote_item *item = list->head;
    struct footnote_item *next;

    while (item) {
        next = item->next;
        if (free_refs)
            free_footnote_ref(item->ref);
        free(item);
        item = next;
    }
}


/*
 * Check whether a char is a Markdown spacing char.

 * Right now we only consider spaces the actual
 * space and a newline: tabs and carriage returns
 * are filtered out during the preprocessing phase.
 *
 * If we wanted to actually be UTF-8 compliant, we
 * should instead extract an Unicode codepoint from
 * this character and check for space properties.
 */
static int
_isspace(int c)
{
    return c == ' ' || c == '\n';
}

/* is_empty_all: verify that all the data is spacing */
static int
is_empty_all(const uint8_t *data, size_t size)
{
    size_t i = 0;
    while (i < size && _isspace(data[i])) i++;
    return i == size;
}

/*
 * Replace all spacing characters in data with spaces. As a special
 * case, this collapses a newline with the previous space, if possible.
 */
static void
replace_spacing(hoedown_buffer *ob, const uint8_t *data, size_t size)
{
    size_t i = 0, mark;
    hoedown_buffer_grow(ob, size);
    while (1) {
        mark = i;
        while (i < size && data[i] != '\n') i++;
        hoedown_buffer_put(ob, data + mark, i - mark);

        if (i >= size) break;

        if (!(i > 0 && data[i-1] == ' '))
            hoedown_buffer_putc(ob, ' ');
        i++;
    }
}

/****************************
 * INLINE PARSING FUNCTIONS *
 ****************************/

/* is_mail_autolink • looks for the address part of a mail autolink and '>' */
/* this is less strict than the original markdown e-mail address matching */
static size_t
is_mail_autolink(uint8_t *data, size_t size)
{
    size_t i = 0, nb = 0;

    /* address is assumed to be: [-@._a-zA-Z0-9]+ with exactly one '@' */
    for (i = 0; i < size; ++i) {
        if (isalnum(data[i]))
            continue;

        switch (data[i]) {
            case '@':
                nb++;

            case '-':
            case '.':
            case '_':
                break;

            case '>':
                return (nb == 1) ? i + 1 : 0;

            default:
                return 0;
        }
    }

    return 0;
}

/* tag_length • returns the length of the given tag, or 0 is it's not valid */
static size_t
tag_length(uint8_t *data, size_t size, hoedown_autolink_type *autolink)
{
    size_t i, j;

    /* a valid tag can't be shorter than 3 chars */
    if (size < 3) return 0;

    /* begins with a '<' optionally followed by '/', followed by letter or number */
    if (data[0] != '<') return 0;
    i = (data[1] == '/') ? 2 : 1;

    if (!isalnum(data[i]))
        return 0;

    /* scheme test */
    *autolink = HOEDOWN_AUTOLINK_NONE;

    /* try to find the beginning of an URI */
    while (i < size && (isalnum(data[i]) || data[i] == '.' || data[i] == '+' || data[i] == '-'))
        i++;

    if (i > 1 && data[i] == '@') {
        if ((j = is_mail_autolink(data + i, size - i)) != 0) {
            *autolink = HOEDOWN_AUTOLINK_EMAIL;
            return i + j;
        }
    }

    if (i > 2 && data[i] == ':') {
        *autolink = HOEDOWN_AUTOLINK_NORMAL;
        i++;
    }

    /* completing autolink test: no spacing or ' or " */
    if (i >= size)
        *autolink = HOEDOWN_AUTOLINK_NONE;

    else if (*autolink) {
        j = i;

        while (i < size) {
            if (data[i] == '\\') i += 2;
            else if (data[i] == '>' || data[i] == '\'' ||
                    data[i] == '"' || data[i] == ' ' || data[i] == '\n')
                    break;
            else i++;
        }

        if (i >= size) return 0;
        if (i > j && data[i] == '>') return i + 1;
        /* one of the forbidden chars has been found */
        *autolink = HOEDOWN_AUTOLINK_NONE;
    }

    /* looking for something looking like a tag end */
    while (i < size && data[i] != '>') i++;
    if (i >= size) return 0;
    return i + 1;
}

/* parse_inline • parses inline markdown elements */
static void
parse_inline(hoedown_buffer *ob, hoedown_document *doc, uint8_t *data, size_t size)
{
    size_t i = 0, end = 0, consumed = 0;
    hoedown_buffer work = { 0, 0, 0, 0, NULL, NULL, NULL };
    uint8_t *active_char = doc->active_char;

    if (doc->work_bufs[BUFFER_SPAN].size +
        doc->work_bufs[BUFFER_BLOCK].size > doc->max_nesting)
        return;

    while (i < size) {
        /* copying inactive chars into the output */
        while (end < size && active_char[data[end]] == 0)
            end++;

        if (doc->md.normal_text) {
            work.data = data + i;
            work.size = end - i;
            doc->md.normal_text(ob, &work, &doc->data);
        }
        else
            hoedown_buffer_put(ob, data + i, end - i);

        if (end >= size) break;
        i = end;

        end = markdown_char_ptrs[ (int)active_char[data[end]] ](ob, doc, data + i, i - consumed, size - i);
        if (!end) /* no action from the callback */
            end = i + 1;
        else {
            i += end;
            end = i;
            consumed = i;
        }
    }
}

/* is_escaped • returns whether special char at data[loc] is escaped by '\\' */
static int
is_escaped(uint8_t *data, size_t loc)
{
    size_t i = loc;
    while (i >= 1 && data[i - 1] == '\\')
        i--;

    /* odd numbers of backslashes escapes data[loc] */
    return (loc - i) % 2;
}

/* find_emph_char • looks for the next emph uint8_t, skipping other constructs */
static size_t
find_emph_char(uint8_t *data, size_t size, uint8_t c)
{
    size_t i = 0;

    while (i < size) {
        while (i < size && data[i] != c && data[i] != '[' && data[i] != '`')
            i++;

        if (i == size)
            return 0;

        /* not counting escaped chars */
        if (is_escaped(data, i)) {
            i++; continue;
        }

        if (data[i] == c)
            return i;

        /* skipping a codespan */
        if (data[i] == '`') {
            size_t span_nb = 0, bt;
            size_t tmp_i = 0;

            /* counting the number of opening backticks */
            while (i < size && data[i] == '`') {
                i++; span_nb++;
            }

            if (i >= size) return 0;

            /* finding the matching closing sequence */
            bt = 0;
            while (i < size && bt < span_nb) {
                if (!tmp_i && data[i] == c) tmp_i = i;
                if (data[i] == '`') bt++;
                else bt = 0;
                i++;
            }

            /* not a well-formed codespan; use found matching emph char */
            if (i >= size) return tmp_i;
        }
        /* skipping a link */
        else if (data[i] == '[') {
            size_t tmp_i = 0;
            uint8_t cc;

            i++;
            while (i < size && data[i] != ']') {
                if (!tmp_i && data[i] == c) tmp_i = i;
                i++;
            }

            i++;
            while (i < size && _isspace(data[i]))
                i++;

            if (i >= size)
                return tmp_i;

            switch (data[i]) {
            case '[':
                cc = ']'; break;

            case '(':
                cc = ')'; break;

            default:
                if (tmp_i)
                    return tmp_i;
                else
                    continue;
            }

            i++;
            while (i < size && data[i] != cc) {
                if (!tmp_i && data[i] == c) tmp_i = i;
                i++;
            }

            if (i >= size)
                return tmp_i;

            i++;
        }
    }

    return 0;
}

/* parse_emph1 • parsing single emphase */
/* closed by a symbol not preceded by spacing and not followed by symbol */
static size_t
parse_emph1(hoedown_buffer *ob, hoedown_document *doc, uint8_t *data, size_t size, uint8_t c)
{
    size_t i = 0, len;
    hoedown_buffer *work = 0;
    int r;

    /* skipping one symbol if coming from emph3 */
    if (size > 1 && data[0] == c && data[1] == c) i = 1;

    while (i < size) {
        len = find_emph_char(data + i, size - i, c);
        if (!len) return 0;
        i += len;
        if (i >= size) return 0;

        if (data[i] == c && !_isspace(data[i - 1])) {

            if (doc->ext_flags & HOEDOWN_EXT_NO_INTRA_EMPHASIS) {
                if (i + 1 < size && isalnum(data[i + 1]))
                    continue;
            }

            work = newbuf(doc, BUFFER_SPAN);
            parse_inline(work, doc, data, i);

            if (doc->ext_flags & HOEDOWN_EXT_UNDERLINE && c == '_')
                r = doc->md.underline(ob, work, &doc->data);
            else
                r = doc->md.emphasis(ob, work, &doc->data);

            popbuf(doc, BUFFER_SPAN);
            return r ? i + 1 : 0;
        }
    }

    return 0;
}

/* parse_emph2 • parsing single emphase */
static size_t
parse_emph2(hoedown_buffer *ob, hoedown_document *doc, uint8_t *data, size_t size, uint8_t c)
{
    size_t i = 0, len;
    hoedown_buffer *work = 0;
    int r;

    while (i < size) {
        len = find_emph_char(data + i, size - i, c);
        if (!len) return 0;
        i += len;

        if (i + 1 < size && data[i] == c && data[i + 1] == c && i && !_isspace(data[i - 1])) {
            work = newbuf(doc, BUFFER_SPAN);
            parse_inline(work, doc, data, i);

            if (c == '~')
                r = doc->md.strikethrough(ob, work, &doc->data);
            else if (c == '=')
                r = doc->md.highlight(ob, work, &doc->data);
            else
                r = doc->md.double_emphasis(ob, work, &doc->data);

            popbuf(doc, BUFFER_SPAN);
            return r ? i + 2 : 0;
        }
        i++;
    }
    return 0;
}

/* parse_emph3 • parsing single emphase */
/* finds the first closing tag, and delegates to the other emph */
static size_t
parse_emph3(hoedown_buffer *ob, hoedown_document *doc, uint8_t *data, size_t size, uint8_t c)
{
    size_t i = 0, len;
    int r;

    while (i < size) {
        len = find_emph_char(data + i, size - i, c);
        if (!len) return 0;
        i += len;

        /* skip spacing preceded symbols */
        if (data[i] != c || _isspace(data[i - 1]))
            continue;

        if (i + 2 < size && data[i + 1] == c && data[i + 2] == c && doc->md.triple_emphasis) {
            /* triple symbol found */
            hoedown_buffer *work = newbuf(doc, BUFFER_SPAN);

            parse_inline(work, doc, data, i);
            r = doc->md.triple_emphasis(ob, work, &doc->data);
            popbuf(doc, BUFFER_SPAN);
            return r ? i + 3 : 0;

        } else if (i + 1 < size && data[i + 1] == c) {
            /* double symbol found, handing over to emph1 */
            len = parse_emph1(ob, doc, data - 2, size + 2, c);
            if (!len) return 0;
            else return len - 2;

        } else {
            /* single symbol found, handing over to emph2 */
            len = parse_emph2(ob, doc, data - 1, size + 1, c);
            if (!len) return 0;
            else return len - 1;
        }
    }
    return 0;
}

/* parse_math • parses a math span until the given ending delimiter */
static size_t
parse_math(hoedown_buffer *ob, hoedown_document *doc, uint8_t *data, size_t offset, size_t size, const char *end, size_t delimsz, int displaymode)
{
    hoedown_buffer text = { NULL, 0, 0, 0, NULL, NULL, NULL };
    size_t i = delimsz;

    if (!doc->md.math)
        return 0;

    /* find ending delimiter */
    while (1) {
        while (i < size && data[i] != (uint8_t)end[0])
            i++;

        if (i >= size)
            return 0;

        if (!is_escaped(data, i) && !(i + delimsz > size)
            && memcmp(data + i, end, delimsz) == 0)
            break;

        i++;
    }

    /* prepare buffers */
    text.data = data + delimsz;
    text.size = i - delimsz;

    /* if this is a $$ and MATH_EXPLICIT is not active,
     * guess whether displaymode should be enabled from the context */
    i += delimsz;
    if (delimsz == 2 && !(doc->ext_flags & HOEDOWN_EXT_MATH_EXPLICIT))
        displaymode = is_empty_all(data - offset, offset) && is_empty_all(data + i, size - i);

    /* call callback */
    if (doc->md.math(ob, &text, displaymode, &doc->data))
        return i;

    return 0;
}

/* char_emphasis • single and double emphasis parsing */
static size_t
char_emphasis(hoedown_buffer *ob, hoedown_document *doc, uint8_t *data, size_t offset, size_t size)
{
    uint8_t c = data[0];
    size_t ret;

    if (doc->ext_flags & HOEDOWN_EXT_NO_INTRA_EMPHASIS) {
        if (offset > 0 && !_isspace(data[-1]) && data[-1] != '>' && data[-1] != '(')
            return 0;
    }

    if (size > 2 && data[1] != c) {
        /* spacing cannot follow an opening emphasis;
         * strikethrough and highlight only takes two characters '~~' */
        if (c == '~' || c == '=' || _isspace(data[1]) || (ret = parse_emph1(ob, doc, data + 1, size - 1, c)) == 0)
            return 0;

        return ret + 1;
    }

    if (size > 3 && data[1] == c && data[2] != c) {
        if (_isspace(data[2]) || (ret = parse_emph2(ob, doc, data + 2, size - 2, c)) == 0)
            return 0;

        return ret + 2;
    }

    if (size > 4 && data[1] == c && data[2] == c && data[3] != c) {
        if (c == '~' || c == '=' || _isspace(data[3]) || (ret = parse_emph3(ob, doc, data + 3, size - 3, c)) == 0)
            return 0;

        return ret + 3;
    }

    return 0;
}


/* char_linebreak • '\n' preceded by two spaces (assuming linebreak != 0) */
static size_t
char_linebreak(hoedown_buffer *ob, hoedown_document *doc, uint8_t *data, size_t offset, size_t size)
{
    if (offset < 2 || data[-1] != ' ' || data[-2] != ' ')
        return 0;

    /* removing the last space from ob and rendering */
    while (ob->size && ob->data[ob->size - 1] == ' ')
        ob->size--;

    return doc->md.linebreak(ob, &doc->data) ? 1 : 0;
}


/* char_codespan • '`' parsing a code span (assuming codespan != 0) */
static size_t
char_codespan(hoedown_buffer *ob, hoedown_document *doc, uint8_t *data, size_t offset, size_t size)
{
    hoedown_buffer work = { NULL, 0, 0, 0, NULL, NULL, NULL };
    size_t end, nb = 0, i, f_begin, f_end;

    /* counting the number of backticks in the delimiter */
    while (nb < size && data[nb] == '`')
        nb++;

    /* finding the next delimiter */
    i = 0;
    for (end = nb; end < size && i < nb; end++) {
        if (data[end] == '`') i++;
        else i = 0;
    }

    if (i < nb && end >= size)
        return 0; /* no matching delimiter */

    /* trimming outside spaces */
    f_begin = nb;
    while (f_begin < end && data[f_begin] == ' ')
        f_begin++;

    f_end = end - nb;
    while (f_end > nb && data[f_end-1] == ' ')
        f_end--;

    /* real code span */
    if (f_begin < f_end) {
        work.data = data + f_begin;
        work.size = f_end - f_begin;

        if (!doc->md.codespan(ob, &work, &doc->data))
            end = 0;
    } else {
        if (!doc->md.codespan(ob, 0, &doc->data))
            end = 0;
    }

    return end;
}

/* char_quote • '"' parsing a quote */
static size_t
char_quote(hoedown_buffer *ob, hoedown_document *doc, uint8_t *data, size_t offset, size_t size)
{
    size_t end, nq = 0, i, f_begin, f_end;

    /* counting the number of quotes in the delimiter */
    while (nq < size && data[nq] == '"')
        nq++;

    /* finding the next delimiter */
    end = nq;
    while (1) {
        i = end;
        end += find_emph_char(data + end, size - end, '"');
        if (end == i) return 0;        /* no matching delimiter */
        i = end;
        while (end < size && data[end] == '"' && end - i < nq) end++;
        if (end - i >= nq) break;
    }

    /* trimming outside spaces */
    f_begin = nq;
    while (f_begin < end && data[f_begin] == ' ')
        f_begin++;

    f_end = end - nq;
    while (f_end > nq && data[f_end-1] == ' ')
        f_end--;

    /* real quote */
    if (f_begin < f_end) {
        hoedown_buffer *work = newbuf(doc, BUFFER_SPAN);
        parse_inline(work, doc, data + f_begin, f_end - f_begin);

        if (!doc->md.quote(ob, work, &doc->data))
            end = 0;
        popbuf(doc, BUFFER_SPAN);
    } else {
        if (!doc->md.quote(ob, 0, &doc->data))
            end = 0;
    }

    return end;
}


/* char_escape • '\\' backslash escape */
static size_t
char_escape(hoedown_buffer *ob, hoedown_document *doc, uint8_t *data, size_t offset, size_t size)
{
    static const char *escape_chars = "\\`*_{}[]()#+-.!:|&<>^~=\"$";
    hoedown_buffer work = { 0, 0, 0, 0, NULL, NULL, NULL };
    size_t w;

    if (size > 1) {
        if (data[1] == '\\' && (doc->ext_flags & HOEDOWN_EXT_MATH) &&
            size > 2 && (data[2] == '(' || data[2] == '[')) {
            const char *end = (data[2] == '[') ? "\\\\]" : "\\\\)";
            w = parse_math(ob, doc, data, offset, size, end, 3, data[2] == '[');
            if (w) return w;
        }

        if (strchr(escape_chars, data[1]) == NULL)
            return 0;

        if (doc->md.normal_text) {
            work.data = data + 1;
            work.size = 1;
            doc->md.normal_text(ob, &work, &doc->data);
        }
        else hoedown_buffer_putc(ob, data[1]);
    } else if (size == 1) {
        hoedown_buffer_putc(ob, data[0]);
    }

    return 2;
}

/* char_entity • '&' escaped when it doesn't belong to an entity */
/* valid entities are assumed to be anything matching &#?[A-Za-z0-9]+; */
static size_t
char_entity(hoedown_buffer *ob, hoedown_document *doc, uint8_t *data, size_t offset, size_t size)
{
    size_t end = 1;
    hoedown_buffer work = { 0, 0, 0, 0, NULL, NULL, NULL };

    if (end < size && data[end] == '#')
        end++;

    while (end < size && isalnum(data[end]))
        end++;

    if (end < size && data[end] == ';')
        end++; /* real entity */
    else
        return 0; /* lone '&' */

    if (doc->md.entity) {
        work.data = data;
        work.size = end;
        doc->md.entity(ob, &work, &doc->data);
    }
    else hoedown_buffer_put(ob, data, end);

    return end;
}

/* char_langle_tag • '<' when tags or autolinks are allowed */
static size_t
char_langle_tag(hoedown_buffer *ob, hoedown_document *doc, uint8_t *data, size_t offset, size_t size)
{
    hoedown_buffer work = { NULL, 0, 0, 0, NULL, NULL, NULL };
    hoedown_autolink_type altype = HOEDOWN_AUTOLINK_NONE;
    size_t end = tag_length(data, size, &altype);
    int ret = 0;

    work.data = data;
    work.size = end;

    if (end > 2) {
        if (doc->md.autolink && altype != HOEDOWN_AUTOLINK_NONE) {
            hoedown_buffer *u_link = newbuf(doc, BUFFER_SPAN);
            work.data = data + 1;
            work.size = end - 2;
            unscape_text(u_link, &work);
            ret = doc->md.autolink(ob, u_link, altype, &doc->data);
            popbuf(doc, BUFFER_SPAN);
        }
        else if (doc->md.raw_html)
            ret = doc->md.raw_html(ob, &work, &doc->data);
    }

    if (!ret) return 0;
    else return end;
}

static size_t
char_autolink_www(hoedown_buffer *ob, hoedown_document *doc, uint8_t *data, size_t offset, size_t size)
{
    hoedown_buffer *link, *link_url, *link_text;
    size_t link_len, rewind;

    if (!doc->md.link || doc->in_link_body)
        return 0;

    link = newbuf(doc, BUFFER_SPAN);

    if ((link_len = hoedown_autolink__www(&rewind, link, data, offset, size, HOEDOWN_AUTOLINK_SHORT_DOMAINS)) > 0) {
        link_url = newbuf(doc, BUFFER_SPAN);
        HOEDOWN_BUFPUTSL(link_url, "http://");
        hoedown_buffer_put(link_url, link->data, link->size);

        ob->size -= rewind;
        if (doc->md.normal_text) {
            link_text = newbuf(doc, BUFFER_SPAN);
            doc->md.normal_text(link_text, link, &doc->data);
            doc->md.link(ob, link_text, link_url, NULL, &doc->data);
            popbuf(doc, BUFFER_SPAN);
        } else {
            doc->md.link(ob, link, link_url, NULL, &doc->data);
        }
        popbuf(doc, BUFFER_SPAN);
    }

    popbuf(doc, BUFFER_SPAN);
    return link_len;
}

static size_t
char_autolink_email(hoedown_buffer *ob, hoedown_document *doc, uint8_t *data, size_t offset, size_t size)
{
    hoedown_buffer *link;
    size_t link_len, rewind;

    if (!doc->md.autolink || doc->in_link_body)
        return 0;

    link = newbuf(doc, BUFFER_SPAN);

    if ((link_len = hoedown_autolink__email(&rewind, link, data, offset, size, 0)) > 0) {
        ob->size -= rewind;
        doc->md.autolink(ob, link, HOEDOWN_AUTOLINK_EMAIL, &doc->data);
    }

    popbuf(doc, BUFFER_SPAN);
    return link_len;
}

static size_t
char_autolink_url(hoedown_buffer *ob, hoedown_document *doc, uint8_t *data, size_t offset, size_t size)
{
    hoedown_buffer *link;
    size_t link_len, rewind;

    if (!doc->md.autolink || doc->in_link_body)
        return 0;

    link = newbuf(doc, BUFFER_SPAN);

    if ((link_len = hoedown_autolink__url(&rewind, link, data, offset, size, 0)) > 0) {
        ob->size -= rewind;
        doc->md.autolink(ob, link, HOEDOWN_AUTOLINK_NORMAL, &doc->data);
    }

    popbuf(doc, BUFFER_SPAN);
    return link_len;
}

/* char_link • '[': parsing a link, a footnote or an image */
static size_t
char_link(hoedown_buffer *ob, hoedown_document *doc, uint8_t *data, size_t offset, size_t size)
{
    int is_img = (offset && data[-1] == '!' && !is_escaped(data - offset, offset - 1));
    int is_footnote = (doc->ext_flags & HOEDOWN_EXT_FOOTNOTES && data[1] == '^');
    size_t i = 1, txt_e, link_b = 0, link_e = 0, title_b = 0, title_e = 0;
    hoedown_buffer *content = NULL;
    hoedown_buffer *link = NULL;
    hoedown_buffer *title = NULL;
    hoedown_buffer *u_link = NULL;
    size_t org_work_size = doc->work_bufs[BUFFER_SPAN].size;
    int ret = 0, in_title = 0, qtype = 0;

    /* checking whether the correct renderer exists */
    if ((is_footnote && !doc->md.footnote_ref) || (is_img && !doc->md.image)
        || (!is_img && !is_footnote && !doc->md.link))
        goto cleanup;

    /* looking for the matching closing bracket */
    i += find_emph_char(data + i, size - i, ']');
    txt_e = i;

    if (i < size && data[i] == ']') i++;
    else goto cleanup;

    /* footnote link */
    if (is_footnote) {
        hoedown_buffer id = { NULL, 0, 0, 0, NULL, NULL, NULL };
        struct footnote_ref *fr;

        if (txt_e < 3)
            goto cleanup;

        id.data = data + 2;
        id.size = txt_e - 2;

        fr = find_footnote_ref(&doc->footnotes_found, id.data, id.size);

        /* mark footnote used */
        if (fr && !fr->is_used) {
            if(!add_footnote_ref(&doc->footnotes_used, fr))
                goto cleanup;
            fr->is_used = 1;
            fr->num = doc->footnotes_used.count;

            /* render */
            if (doc->md.footnote_ref)
                ret = doc->md.footnote_ref(ob, fr->num, &doc->data);
        }

        goto cleanup;
    }

    /* skip any amount of spacing */
    /* (this is much more laxist than original markdown syntax) */
    while (i < size && _isspace(data[i]))
        i++;

    /* inline style link */
    if (i < size && data[i] == '(') {
        size_t nb_p;

        /* skipping initial spacing */
        i++;

        while (i < size && _isspace(data[i]))
            i++;

        link_b = i;

        /* looking for link end: ' " ) */
        /* Count the number of open parenthesis */
        nb_p = 0;

        while (i < size) {
            if (data[i] == '\\') i += 2;
            else if (data[i] == '(' && i != 0) {
                nb_p++; i++;
            }
            else if (data[i] == ')') {
                if (nb_p == 0) break;
                else nb_p--; i++;
            } else if (i >= 1 && _isspace(data[i-1]) && (data[i] == '\'' || data[i] == '"')) break;
            else i++;
        }

        if (i >= size) goto cleanup;
        link_e = i;

        /* looking for title end if present */
        if (data[i] == '\'' || data[i] == '"') {
            qtype = data[i];
            in_title = 1;
            i++;
            title_b = i;

            while (i < size) {
                if (data[i] == '\\') i += 2;
                else if (data[i] == qtype) {in_title = 0; i++;}
                else if ((data[i] == ')') && !in_title) break;
                else i++;
            }

            if (i >= size) goto cleanup;

            /* skipping spacing after title */
            title_e = i - 1;
            while (title_e > title_b && _isspace(data[title_e]))
                title_e--;

            /* checking for closing quote presence */
            if (data[title_e] != '\'' &&  data[title_e] != '"') {
                title_b = title_e = 0;
                link_e = i;
            }
        }

        /* remove spacing at the end of the link */
        while (link_e > link_b && _isspace(data[link_e - 1]))
            link_e--;

        /* remove optional angle brackets around the link */
        if (data[link_b] == '<') link_b++;
        if (data[link_e - 1] == '>') link_e--;

        /* building escaped link and title */
        if (link_e > link_b) {
            link = newbuf(doc, BUFFER_SPAN);
            hoedown_buffer_put(link, data + link_b, link_e - link_b);
        }

        if (title_e > title_b) {
            title = newbuf(doc, BUFFER_SPAN);
            hoedown_buffer_put(title, data + title_b, title_e - title_b);
        }

        i++;
    }

    /* reference style link */
    else if (i < size && data[i] == '[') {
        hoedown_buffer *id = newbuf(doc, BUFFER_SPAN);
        struct link_ref *lr;

        /* looking for the id */
        i++;
        link_b = i;
        while (i < size && data[i] != ']') i++;
        if (i >= size) goto cleanup;
        link_e = i;

        /* finding the link_ref */
        if (link_b == link_e)
            replace_spacing(id, data + 1, txt_e - 1);
        else
            hoedown_buffer_put(id, data + link_b, link_e - link_b);

        lr = find_link_ref(doc->refs, id->data, id->size);
        if (!lr)
            goto cleanup;

        /* keeping link and title from link_ref */
        link = lr->link;
        title = lr->title;
        i++;
    }

    /* shortcut reference style link */
    else {
        hoedown_buffer *id = newbuf(doc, BUFFER_SPAN);
        struct link_ref *lr;

        /* crafting the id */
        replace_spacing(id, data + 1, txt_e - 1);

        /* finding the link_ref */
        lr = find_link_ref(doc->refs, id->data, id->size);
        if (!lr)
            goto cleanup;

        /* keeping link and title from link_ref */
        link = lr->link;
        title = lr->title;

        /* rewinding the spacing */
        i = txt_e + 1;
    }

    /* building content: img alt is kept, only link content is parsed */
    if (txt_e > 1) {
        content = newbuf(doc, BUFFER_SPAN);
        if (is_img) {
            hoedown_buffer_put(content, data + 1, txt_e - 1);
        } else {
            /* disable autolinking when parsing inline the
             * content of a link */
            doc->in_link_body = 1;
            parse_inline(content, doc, data + 1, txt_e - 1);
            doc->in_link_body = 0;
        }
    }

    if (link) {
        u_link = newbuf(doc, BUFFER_SPAN);
        unscape_text(u_link, link);
    }

    /* calling the relevant rendering function */
    if (is_img) {
        if (ob->size && ob->data[ob->size - 1] == '!')
            ob->size -= 1;

        ret = doc->md.image(ob, u_link, title, content, &doc->data);
    } else {
        ret = doc->md.link(ob, content, u_link, title, &doc->data);
    }

    /* cleanup */
cleanup:
    doc->work_bufs[BUFFER_SPAN].size = (int)org_work_size;
    return ret ? i : 0;
}

static size_t
char_superscript(hoedown_buffer *ob, hoedown_document *doc, uint8_t *data, size_t offset, size_t size)
{
    size_t sup_start, sup_len;
    hoedown_buffer *sup;

    if (!doc->md.superscript)
        return 0;

    if (size < 2)
        return 0;

    if (data[1] == '(') {
        sup_start = 2;
        sup_len = find_emph_char(data + 2, size - 2, ')') + 2;

        if (sup_len == size)
            return 0;
    } else {
        sup_start = sup_len = 1;

        while (sup_len < size && !_isspace(data[sup_len]))
            sup_len++;
    }

    if (sup_len - sup_start == 0)
        return (sup_start == 2) ? 3 : 0;

    sup = newbuf(doc, BUFFER_SPAN);
    parse_inline(sup, doc, data + sup_start, sup_len - sup_start);
    doc->md.superscript(ob, sup, &doc->data);
    popbuf(doc, BUFFER_SPAN);

    return (sup_start == 2) ? sup_len + 1 : sup_len;
}

static size_t
char_math(hoedown_buffer *ob, hoedown_document *doc, uint8_t *data, size_t offset, size_t size)
{
    /* double dollar */
    if (size > 1 && data[1] == '$')
        return parse_math(ob, doc, data, offset, size, "$$", 2, 1);

    /* single dollar allowed only with MATH_EXPLICIT flag */
    if (doc->ext_flags & HOEDOWN_EXT_MATH_EXPLICIT)
        return parse_math(ob, doc, data, offset, size, "$", 1, 0);

    return 0;
}

/*********************************
 * BLOCK-LEVEL PARSING FUNCTIONS *
 *********************************/

/* is_empty • returns the line length when it is empty, 0 otherwise */
static size_t
is_empty(const uint8_t *data, size_t size)
{
    size_t i;

    for (i = 0; i < size && data[i] != '\n'; i++)
        if (data[i] != ' ')
            return 0;

    return i + 1;
}

/* is_hrule • returns whether a line is a horizontal rule */
static int
is_hrule(uint8_t *data, size_t size)
{
    size_t i = 0, n = 0;
    uint8_t c;

    /* skipping initial spaces */
    if (size < 3) return 0;
    if (data[0] == ' ') { i++;
    if (data[1] == ' ') { i++;
    if (data[2] == ' ') { i++; } } }

    /* looking at the hrule uint8_t */
    if (i + 2 >= size
    || (data[i] != '*' && data[i] != '-' && data[i] != '_'))
        return 0;
    c = data[i];

    /* the whole line must be the char or space */
    while (i < size && data[i] != '\n') {
        if (data[i] == c) n++;
        else if (data[i] != ' ')
            return 0;

        i++;
    }

    return n >= 3;
}

/* check if a line is a code fence; return the
 * end of the code fence. if passed, width of
 * the fence rule and character will be returned */
static size_t
is_codefence(uint8_t *data, size_t size, size_t *width, uint8_t *chr)
{
    size_t i = 0, n = 1;
    uint8_t c;

    /* skipping initial spaces */
    if (size < 3)
        return 0;

    if (data[0] == ' ') { i++;
    if (data[1] == ' ') { i++;
    if (data[2] == ' ') { i++; } } }

    /* looking at the hrule uint8_t */
    c = data[i];
    if (i + 2 >= size || !(c=='~' || c=='`'))
        return 0;

    /* the fence must be that same character */
    while (++i < size && data[i] == c)
        ++n;

    if (n < 3)
        return 0;

    if (width) *width = n;
    if (chr) *chr = c;
    return i;
}

/* expects single line, checks if it's a codefence and extracts language */
static size_t
parse_codefence(uint8_t *data, size_t size, hoedown_buffer *lang, size_t *width, uint8_t *chr)
{
    size_t i, w, lang_start;

    i = w = is_codefence(data, size, width, chr);
    if (i == 0)
        return 0;

    while (i < size && _isspace(data[i]))
        i++;

    lang_start = i;

    while (i < size && !_isspace(data[i]))
        i++;

    lang->data = data + lang_start;
    lang->size = i - lang_start;

    /* Avoid parsing a codespan as a fence */
    i = lang_start + 2;
    while (i < size && !(data[i] == *chr && data[i-1] == *chr && data[i-2] == *chr)) i++;
    if (i < size) return 0;

    return w;
}

/* is_atxheader • returns whether the line is a hash-prefixed header */
static int
is_atxheader(hoedown_document *doc, uint8_t *data, size_t size)
{
    if (data[0] != '#')
        return 0;

    if (doc->ext_flags & HOEDOWN_EXT_SPACE_HEADERS) {
        size_t level = 0;

        while (level < size && level < 6 && data[level] == '#')
            level++;

        if (level < size && data[level] != ' ')
            return 0;
    }

    return 1;
}

/* is_headerline • returns whether the line is a setext-style hdr underline */
static int
is_headerline(uint8_t *data, size_t size)
{
    size_t i = 0;

    /* test of level 1 header */
    if (data[i] == '=') {
        for (i = 1; i < size && data[i] == '='; i++);
        while (i < size && data[i] == ' ') i++;
        return (i >= size || data[i] == '\n') ? 1 : 0; }

    /* test of level 2 header */
    if (data[i] == '-') {
        for (i = 1; i < size && data[i] == '-'; i++);
        while (i < size && data[i] == ' ') i++;
        return (i >= size || data[i] == '\n') ? 2 : 0; }

    return 0;
}

static int
is_next_headerline(uint8_t *data, size_t size)
{
    size_t i = 0;

    while (i < size && data[i] != '\n')
        i++;

    if (++i >= size)
        return 0;

    return is_headerline(data + i, size - i);
}

/* prefix_quote • returns blockquote prefix length */
static size_t
prefix_quote(uint8_t *data, size_t size)
{
    size_t i = 0;
    if (i < size && data[i] == ' ') i++;
    if (i < size && data[i] == ' ') i++;
    if (i < size && data[i] == ' ') i++;

    if (i < size && data[i] == '>') {
        if (i + 1 < size && data[i + 1] == ' ')
            return i + 2;

        return i + 1;
    }

    return 0;
}

/* prefix_code • returns prefix length for block code*/
static size_t
prefix_code(uint8_t *data, size_t size)
{
    if (size > 3 && data[0] == ' ' && data[1] == ' '
        && data[2] == ' ' && data[3] == ' ') return 4;

    return 0;
}

/* prefix_oli • returns ordered list item prefix */
static size_t
prefix_oli(uint8_t *data, size_t size)
{
    size_t i = 0;

    if (i < size && data[i] == ' ') i++;
    if (i < size && data[i] == ' ') i++;
    if (i < size && data[i] == ' ') i++;

    if (i >= size || data[i] < '0' || data[i] > '9')
        return 0;

    while (i < size && data[i] >= '0' && data[i] <= '9')
        i++;

    if (i + 1 >= size || data[i] != '.' || data[i + 1] != ' ')
        return 0;

    if (is_next_headerline(data + i, size - i))
        return 0;

    return i + 2;
}

/* prefix_uli • returns ordered list item prefix */
static size_t
prefix_uli(uint8_t *data, size_t size)
{
    size_t i = 0;

    if (i < size && data[i] == ' ') i++;
    if (i < size && data[i] == ' ') i++;
    if (i < size && data[i] == ' ') i++;

    if (i + 1 >= size ||
        (data[i] != '*' && data[i] != '+' && data[i] != '-') ||
        data[i + 1] != ' ')
        return 0;

    if (is_next_headerline(data + i, size - i))
        return 0;

    return i + 2;
}


/* parse_block • parsing of one block, returning next uint8_t to parse */
static void parse_block(hoedown_buffer *ob, hoedown_document *doc,
            uint8_t *data, size_t size);


/* parse_blockquote • handles parsing of a blockquote fragment */
static size_t
parse_blockquote(hoedown_buffer *ob, hoedown_document *doc, uint8_t *data, size_t size)
{
    size_t beg, end = 0, pre, work_size = 0;
    uint8_t *work_data = 0;
    hoedown_buffer *out = 0;

    out = newbuf(doc, BUFFER_BLOCK);
    beg = 0;
    while (beg < size) {
        for (end = beg + 1; end < size && data[end - 1] != '\n'; end++);

        pre = prefix_quote(data + beg, end - beg);

        if (pre)
            beg += pre; /* skipping prefix */

        /* empty line followed by non-quote line */
        else if (is_empty(data + beg, end - beg) &&
                (end >= size || (prefix_quote(data + end, size - end) == 0 &&
                !is_empty(data + end, size - end))))
            break;

        if (beg < end) { /* copy into the in-place working buffer */
            /* hoedown_buffer_put(work, data + beg, end - beg); */
            if (!work_data)
                work_data = data + beg;
            else if (data + beg != work_data + work_size)
                memmove(work_data + work_size, data + beg, end - beg);
            work_size += end - beg;
        }
        beg = end;
    }

    parse_block(out, doc, work_data, work_size);
    if (doc->md.blockquote)
        doc->md.blockquote(ob, out, &doc->data);
    popbuf(doc, BUFFER_BLOCK);
    return end;
}

static size_t
parse_htmlblock(hoedown_buffer *ob, hoedown_document *doc, uint8_t *data, size_t size, int do_render);

/* parse_blockquote • handles parsing of a regular paragraph */
static size_t
parse_paragraph(hoedown_buffer *ob, hoedown_document *doc, uint8_t *data, size_t size)
{
    hoedown_buffer work = { NULL, 0, 0, 0, NULL, NULL, NULL };
    size_t i = 0, end = 0;
    int level = 0;

    work.data = data;

    while (i < size) {
        for (end = i + 1; end < size && data[end - 1] != '\n'; end++) /* empty */;

        if (is_empty(data + i, size - i))
            break;

        if ((level = is_headerline(data + i, size - i)) != 0)
            break;

        if (is_atxheader(doc, data + i, size - i) ||
            is_hrule(data + i, size - i) ||
            prefix_quote(data + i, size - i)) {
            end = i;
            break;
        }

        i = end;
    }

    work.size = i;
    while (work.size && data[work.size - 1] == '\n')
        work.size--;

    if (!level) {
        hoedown_buffer *tmp = newbuf(doc, BUFFER_BLOCK);
        parse_inline(tmp, doc, work.data, work.size);
        if (doc->md.paragraph)
            doc->md.paragraph(ob, tmp, &doc->data);
        popbuf(doc, BUFFER_BLOCK);
    } else {
        hoedown_buffer *header_work;

        if (work.size) {
            size_t beg;
            i = work.size;
            work.size -= 1;

            while (work.size && data[work.size] != '\n')
                work.size -= 1;

            beg = work.size + 1;
            while (work.size && data[work.size - 1] == '\n')
                work.size -= 1;

            if (work.size > 0) {
                hoedown_buffer *tmp = newbuf(doc, BUFFER_BLOCK);
                parse_inline(tmp, doc, work.data, work.size);

                if (doc->md.paragraph)
                    doc->md.paragraph(ob, tmp, &doc->data);

                popbuf(doc, BUFFER_BLOCK);
                work.data += beg;
                work.size = i - beg;
            }
            else work.size = i;
        }

        header_work = newbuf(doc, BUFFER_SPAN);
        parse_inline(header_work, doc, work.data, work.size);

        if (doc->md.header)
            doc->md.header(ob, header_work, (int)level, &doc->data);

        popbuf(doc, BUFFER_SPAN);
    }

    return end;
}

/* parse_fencedcode • handles parsing of a block-level code fragment */
static size_t
parse_fencedcode(hoedown_buffer *ob, hoedown_document *doc, uint8_t *data, size_t size)
{
    hoedown_buffer text = { 0, 0, 0, 0, NULL, NULL, NULL };
    hoedown_buffer lang = { 0, 0, 0, 0, NULL, NULL, NULL };
    size_t i = 0, text_start, line_start;
    size_t w, w2;
    size_t width, width2;
    uint8_t chr, chr2;

    /* parse codefence line */
    while (i < size && data[i] != '\n')
        i++;

    w = parse_codefence(data, i, &lang, &width, &chr);
    if (!w)
        return 0;

    /* search for end */
    i++;
    text_start = i;
    while ((line_start = i) < size) {
        while (i < size && data[i] != '\n')
            i++;

        w2 = is_codefence(data + line_start, i - line_start, &width2, &chr2);
        if (w == w2 && width == width2 && chr == chr2 &&
            is_empty(data + (line_start+w), i - (line_start+w)))
            break;

        i++;
    }

    text.data = data + text_start;
    text.size = line_start - text_start;

    if (doc->md.blockcode)
        doc->md.blockcode(ob, text.size ? &text : NULL, lang.size ? &lang : NULL, &doc->data);

    return i;
}

static size_t
parse_blockcode(hoedown_buffer *ob, hoedown_document *doc, uint8_t *data, size_t size)
{
    size_t beg, end, pre;
    hoedown_buffer *work = 0;

    work = newbuf(doc, BUFFER_BLOCK);

    beg = 0;
    while (beg < size) {
        for (end = beg + 1; end < size && data[end - 1] != '\n'; end++) {};
        pre = prefix_code(data + beg, end - beg);

        if (pre)
            beg += pre; /* skipping prefix */
        else if (!is_empty(data + beg, end - beg))
            /* non-empty non-prefixed line breaks the pre */
            break;

        if (beg < end) {
            /* verbatim copy to the working buffer,
                escaping entities */
            if (is_empty(data + beg, end - beg))
                hoedown_buffer_putc(work, '\n');
            else hoedown_buffer_put(work, data + beg, end - beg);
        }
        beg = end;
    }

    while (work->size && work->data[work->size - 1] == '\n')
        work->size -= 1;

    hoedown_buffer_putc(work, '\n');

    if (doc->md.blockcode)
        doc->md.blockcode(ob, work, NULL, &doc->data);

    popbuf(doc, BUFFER_BLOCK);
    return beg;
}

/* parse_listitem • parsing of a single list item */
/*    assuming initial prefix is already removed */
static size_t
parse_listitem(hoedown_buffer *ob, hoedown_document *doc, uint8_t *data, size_t size, hoedown_list_flags *flags)
{
    hoedown_buffer *work = 0, *inter = 0;
    size_t beg = 0, end, pre, sublist = 0, orgpre = 0, i;
    int in_empty = 0, has_inside_empty = 0, in_fence = 0;

    /* keeping track of the first indentation prefix */
    while (orgpre < 3 && orgpre < size && data[orgpre] == ' ')
        orgpre++;

    beg = prefix_uli(data, size);
    if (!beg)
        beg = prefix_oli(data, size);

    if (!beg)
        return 0;

    /* skipping to the beginning of the following line */
    end = beg;
    while (end < size && data[end - 1] != '\n')
        end++;

    /* getting working buffers */
    work = newbuf(doc, BUFFER_SPAN);
    inter = newbuf(doc, BUFFER_SPAN);

    /* putting the first line into the working buffer */
    hoedown_buffer_put(work, data + beg, end - beg);
    beg = end;

    /* process the following lines */
    while (beg < size) {
        size_t has_next_uli = 0, has_next_oli = 0;

        end++;

        while (end < size && data[end - 1] != '\n')
            end++;

        /* process an empty line */
        if (is_empty(data + beg, end - beg)) {
            in_empty = 1;
            beg = end;
            continue;
        }

        /* calculating the indentation */
        i = 0;
        while (i < 4 && beg + i < end && data[beg + i] == ' ')
            i++;

        pre = i;

        if (doc->ext_flags & HOEDOWN_EXT_FENCED_CODE) {
            if (is_codefence(data + beg + i, end - beg - i, NULL, NULL))
                in_fence = !in_fence;
        }

        /* Only check for new list items if we are **not** inside
         * a fenced code block */
        if (!in_fence) {
            has_next_uli = prefix_uli(data + beg + i, end - beg - i);
            has_next_oli = prefix_oli(data + beg + i, end - beg - i);
        }

        /* checking for a new item */
        if ((has_next_uli && !is_hrule(data + beg + i, end - beg - i)) || has_next_oli) {
            if (in_empty)
                has_inside_empty = 1;

            /* the following item must have the same (or less) indentation */
            if (pre <= orgpre) {
                /* if the following item has different list type, we end this list */
                if (in_empty && (
                    ((*flags & HOEDOWN_LIST_ORDERED) && has_next_uli) ||
                    (!(*flags & HOEDOWN_LIST_ORDERED) && has_next_oli)))
                    *flags |= HOEDOWN_LI_END;

                break;
            }

            if (!sublist)
                sublist = work->size;
        }
        /* joining only indented stuff after empty lines;
         * note that now we only require 1 space of indentation
         * to continue a list */
        else if (in_empty && pre == 0) {
            *flags |= HOEDOWN_LI_END;
            break;
        }

        if (in_empty) {
            hoedown_buffer_putc(work, '\n');
            has_inside_empty = 1;
            in_empty = 0;
        }

        /* adding the line without prefix into the working buffer */
        hoedown_buffer_put(work, data + beg + i, end - beg - i);
        beg = end;
    }

    /* render of li contents */
    if (has_inside_empty)
        *flags |= HOEDOWN_LI_BLOCK;

    if (*flags & HOEDOWN_LI_BLOCK) {
        /* intermediate render of block li */
        if (sublist && sublist < work->size) {
            parse_block(inter, doc, work->data, sublist);
            parse_block(inter, doc, work->data + sublist, work->size - sublist);
        }
        else
            parse_block(inter, doc, work->data, work->size);
    } else {
        /* intermediate render of inline li */
        if (sublist && sublist < work->size) {
            parse_inline(inter, doc, work->data, sublist);
            parse_block(inter, doc, work->data + sublist, work->size - sublist);
        }
        else
            parse_inline(inter, doc, work->data, work->size);
    }

    /* render of li itself */
    if (doc->md.listitem)
        doc->md.listitem(ob, inter, *flags, &doc->data);

    popbuf(doc, BUFFER_SPAN);
    popbuf(doc, BUFFER_SPAN);
    return beg;
}


/* parse_list • parsing ordered or unordered list block */
static size_t
parse_list(hoedown_buffer *ob, hoedown_document *doc, uint8_t *data, size_t size, hoedown_list_flags flags)
{
    hoedown_buffer *work = 0;
    size_t i = 0, j;

    work = newbuf(doc, BUFFER_BLOCK);

    while (i < size) {
        j = parse_listitem(work, doc, data + i, size - i, &flags);
        i += j;

        if (!j || (flags & HOEDOWN_LI_END))
            break;
    }

    if (doc->md.list)
        doc->md.list(ob, work, flags, &doc->data);
    popbuf(doc, BUFFER_BLOCK);
    return i;
}

/* parse_atxheader • parsing of atx-style headers */
static size_t
parse_atxheader(hoedown_buffer *ob, hoedown_document *doc, uint8_t *data, size_t size)
{
    size_t level = 0;
    size_t i, end, skip;

    while (level < size && level < 6 && data[level] == '#')
        level++;

    for (i = level; i < size && data[i] == ' '; i++);

    for (end = i; end < size && data[end] != '\n'; end++);
    skip = end;

    while (end && data[end - 1] == '#')
        end--;

    while (end && data[end - 1] == ' ')
        end--;

    if (end > i) {
        hoedown_buffer *work = newbuf(doc, BUFFER_SPAN);

        parse_inline(work, doc, data + i, end - i);

        if (doc->md.header)
            doc->md.header(ob, work, (int)level, &doc->data);

        popbuf(doc, BUFFER_SPAN);
    }

    return skip;
}

/* parse_footnote_def • parse a single footnote definition */
static void
parse_footnote_def(hoedown_buffer *ob, hoedown_document *doc, unsigned int num, uint8_t *data, size_t size)
{
    hoedown_buffer *work = 0;
    work = newbuf(doc, BUFFER_SPAN);

    parse_block(work, doc, data, size);

    if (doc->md.footnote_def)
    doc->md.footnote_def(ob, work, num, &doc->data);
    popbuf(doc, BUFFER_SPAN);
}

/* parse_footnote_list • render the contents of the footnotes */
static void
parse_footnote_list(hoedown_buffer *ob, hoedown_document *doc, struct footnote_list *footnotes)
{
    hoedown_buffer *work = 0;
    struct footnote_item *item;
    struct footnote_ref *ref;

    if (footnotes->count == 0)
        return;

    work = newbuf(doc, BUFFER_BLOCK);

    item = footnotes->head;
    while (item) {
        ref = item->ref;
        parse_footnote_def(work, doc, ref->num, ref->contents->data, ref->contents->size);
        item = item->next;
    }

    if (doc->md.footnotes)
        doc->md.footnotes(ob, work, &doc->data);
    popbuf(doc, BUFFER_BLOCK);
}

/* htmlblock_is_end • check for end of HTML block : </tag>( *)\n */
/*    returns tag length on match, 0 otherwise */
/*    assumes data starts with "<" */
static size_t
htmlblock_is_end(
    const char *tag,
    size_t tag_len,
    hoedown_document *doc,
    uint8_t *data,
    size_t size)
{
    size_t i = tag_len + 3, w;

    /* try to match the end tag */
    /* note: we're not considering tags like "</tag >" which are still valid */
    if (i > size ||
        data[1] != '/' ||
        strncasecmp((char *)data + 2, tag, tag_len) != 0 ||
        data[tag_len + 2] != '>')
        return 0;

    /* rest of the line must be empty */
    if ((w = is_empty(data + i, size - i)) == 0 && i < size)
        return 0;

    return i + w;
}

/* htmlblock_find_end • try to find HTML block ending tag */
/*    returns the length on match, 0 otherwise */
static size_t
htmlblock_find_end(
    const char *tag,
    size_t tag_len,
    hoedown_document *doc,
    uint8_t *data,
    size_t size)
{
    size_t i = 0, w;

    while (1) {
        while (i < size && data[i] != '<') i++;
        if (i >= size) return 0;

        w = htmlblock_is_end(tag, tag_len, doc, data + i, size - i);
        if (w) return i + w;
        i++;
    }
}

/* htmlblock_find_end_strict • try to find end of HTML block in strict mode */
/*    (it must be an unindented line, and have a blank line afterwads) */
/*    returns the length on match, 0 otherwise */
static size_t
htmlblock_find_end_strict(
    const char *tag,
    size_t tag_len,
    hoedown_document *doc,
    uint8_t *data,
    size_t size)
{
    size_t i = 0, mark;

    while (1) {
        mark = i;
        while (i < size && data[i] != '\n') i++;
        if (i < size) i++;
        if (i == mark) return 0;

        if (data[mark] == ' ' && mark > 0) continue;
        mark += htmlblock_find_end(tag, tag_len, doc, data + mark, i - mark);
        if (mark == i && (is_empty(data + i, size - i) || i >= size)) break;
    }

    return i;
}

/* parse_htmlblock • parsing of inline HTML block */
static size_t
parse_htmlblock(hoedown_buffer *ob, hoedown_document *doc, uint8_t *data, size_t size, int do_render)
{
    hoedown_buffer work = { NULL, 0, 0, 0, NULL, NULL, NULL };
    size_t i, j = 0, tag_len, tag_end;
    const char *curtag = NULL;

    work.data = data;

    /* identification of the opening tag */
    if (size < 2 || data[0] != '<')
        return 0;

    i = 1;
    while (i < size && data[i] != '>' && data[i] != ' ')
        i++;

    if (i < size)
        curtag = hoedown_find_block_tag((char *)data + 1, (int)i - 1);

    /* handling of special cases */
    if (!curtag) {

        /* HTML comment, laxist form */
        if (size > 5 && data[1] == '!' && data[2] == '-' && data[3] == '-') {
            i = 5;

            while (i < size && !(data[i - 2] == '-' && data[i - 1] == '-' && data[i] == '>'))
                i++;

            i++;

            if (i < size)
                j = is_empty(data + i, size - i);

            if (j) {
                work.size = i + j;
                if (do_render && doc->md.blockhtml)
                    doc->md.blockhtml(ob, &work, &doc->data);
                return work.size;
            }
        }

        /* HR, which is the only self-closing block tag considered */
        if (size > 4 && (data[1] == 'h' || data[1] == 'H') && (data[2] == 'r' || data[2] == 'R')) {
            i = 3;
            while (i < size && data[i] != '>')
                i++;

            if (i + 1 < size) {
                i++;
                j = is_empty(data + i, size - i);
                if (j) {
                    work.size = i + j;
                    if (do_render && doc->md.blockhtml)
                        doc->md.blockhtml(ob, &work, &doc->data);
                    return work.size;
                }
            }
        }

        /* no special case recognised */
        return 0;
    }

    /* looking for a matching closing tag in strict mode */
    tag_len = strlen(curtag);
    tag_end = htmlblock_find_end_strict(curtag, tag_len, doc, data, size);

    /* if not found, trying a second pass looking for indented match */
    /* but not if tag is "ins" or "del" (following original Markdown.pl) */
    if (!tag_end && strcmp(curtag, "ins") != 0 && strcmp(curtag, "del") != 0)
        tag_end = htmlblock_find_end(curtag, tag_len, doc, data, size);

    if (!tag_end)
        return 0;

    /* the end of the block has been found */
    work.size = tag_end;
    if (do_render && doc->md.blockhtml)
        doc->md.blockhtml(ob, &work, &doc->data);

    return tag_end;
}

static void
parse_table_row(
    hoedown_buffer *ob,
    hoedown_document *doc,
    uint8_t *data,
    size_t size,
    size_t columns,
    hoedown_table_flags *col_data,
    hoedown_table_flags header_flag)
{
    size_t i = 0, col, len;
    hoedown_buffer *row_work = 0;

    if (!doc->md.table_cell || !doc->md.table_row)
        return;

    row_work = newbuf(doc, BUFFER_SPAN);

    if (i < size && data[i] == '|')
        i++;

    for (col = 0; col < columns && i < size; ++col) {
        size_t cell_start, cell_end;
        hoedown_buffer *cell_work;

        cell_work = newbuf(doc, BUFFER_SPAN);

        while (i < size && _isspace(data[i]))
            i++;

        cell_start = i;

        len = find_emph_char(data + i, size - i, '|');
        i += len ? len : size - i;

        cell_end = i - 1;

        while (cell_end > cell_start && _isspace(data[cell_end]))
            cell_end--;

        parse_inline(cell_work, doc, data + cell_start, 1 + cell_end - cell_start);
        doc->md.table_cell(row_work, cell_work, col_data[col] | header_flag, &doc->data);

        popbuf(doc, BUFFER_SPAN);
        i++;
    }

    for (; col < columns; ++col) {
        hoedown_buffer empty_cell = { 0, 0, 0, 0, NULL, NULL, NULL };
        doc->md.table_cell(row_work, &empty_cell, col_data[col] | header_flag, &doc->data);
    }

    doc->md.table_row(ob, row_work, &doc->data);

    popbuf(doc, BUFFER_SPAN);
}

static size_t
parse_table_header(
    hoedown_buffer *ob,
    hoedown_document *doc,
    uint8_t *data,
    size_t size,
    size_t *columns,
    hoedown_table_flags **column_data)
{
    int pipes;
    size_t i = 0, col, header_end, under_end;

    pipes = 0;
    while (i < size && data[i] != '\n')
        if (data[i++] == '|')
            pipes++;

    if (i == size || pipes == 0)
        return 0;

    header_end = i;

    while (header_end > 0 && _isspace(data[header_end - 1]))
        header_end--;

    if (data[0] == '|')
        pipes--;

    if (header_end && data[header_end - 1] == '|')
        pipes--;

    if (pipes < 0)
        return 0;

    *columns = pipes + 1;
    *column_data = hoedown_calloc(*columns, sizeof(hoedown_table_flags));

    /* Parse the header underline */
    i++;
    if (i < size && data[i] == '|')
        i++;

    under_end = i;
    while (under_end < size && data[under_end] != '\n')
        under_end++;

    for (col = 0; col < *columns && i < under_end; ++col) {
        size_t dashes = 0;

        while (i < under_end && data[i] == ' ')
            i++;

        if (data[i] == ':') {
            i++; (*column_data)[col] |= HOEDOWN_TABLE_ALIGN_LEFT;
            dashes++;
        }

        while (i < under_end && data[i] == '-') {
            i++; dashes++;
        }

        if (i < under_end && data[i] == ':') {
            i++; (*column_data)[col] |= HOEDOWN_TABLE_ALIGN_RIGHT;
            dashes++;
        }

        while (i < under_end && data[i] == ' ')
            i++;

        if (i < under_end && data[i] != '|' && data[i] != '+')
            break;

        if (dashes < 3)
            break;

        i++;
    }

    if (col < *columns)
        return 0;

    parse_table_row(
        ob, doc, data,
        header_end,
        *columns,
        *column_data,
        HOEDOWN_TABLE_HEADER
    );

    return under_end + 1;
}

static size_t
parse_table(
    hoedown_buffer *ob,
    hoedown_document *doc,
    uint8_t *data,
    size_t size)
{
    size_t i;

    hoedown_buffer *work = 0;
    hoedown_buffer *header_work = 0;
    hoedown_buffer *body_work = 0;

    size_t columns;
    hoedown_table_flags *col_data = NULL;

    work = newbuf(doc, BUFFER_BLOCK);
    header_work = newbuf(doc, BUFFER_SPAN);
    body_work = newbuf(doc, BUFFER_BLOCK);

    i = parse_table_header(header_work, doc, data, size, &columns, &col_data);
    if (i > 0) {

        while (i < size) {
            size_t row_start;
            int pipes = 0;

            row_start = i;

            while (i < size && data[i] != '\n')
                if (data[i++] == '|')
                    pipes++;

            if (pipes == 0 || i == size) {
                i = row_start;
                break;
            }

            parse_table_row(
                body_work,
                doc,
                data + row_start,
                i - row_start,
                columns,
                col_data, 0
            );

            i++;
        }

        if (doc->md.table_header)
            doc->md.table_header(work, header_work, &doc->data);

        if (doc->md.table_body)
            doc->md.table_body(work, body_work, &doc->data);

        if (doc->md.table)
            doc->md.table(ob, work, &doc->data);
    }

    free(col_data);
    popbuf(doc, BUFFER_SPAN);
    popbuf(doc, BUFFER_BLOCK);
    popbuf(doc, BUFFER_BLOCK);
    return i;
}

/* parse_block • parsing of one block, returning next uint8_t to parse */
static void
parse_block(hoedown_buffer *ob, hoedown_document *doc, uint8_t *data, size_t size)
{
    size_t beg, end, i;
    uint8_t *txt_data;
    beg = 0;

    if (doc->work_bufs[BUFFER_SPAN].size +
        doc->work_bufs[BUFFER_BLOCK].size > doc->max_nesting)
        return;

    while (beg < size) {
        txt_data = data + beg;
        end = size - beg;

        if (is_atxheader(doc, txt_data, end))
            beg += parse_atxheader(ob, doc, txt_data, end);

        else if (data[beg] == '<' && doc->md.blockhtml &&
                (i = parse_htmlblock(ob, doc, txt_data, end, 1)) != 0)
            beg += i;

        else if ((i = is_empty(txt_data, end)) != 0)
            beg += i;

        else if (is_hrule(txt_data, end)) {
            if (doc->md.hrule)
                doc->md.hrule(ob, &doc->data);

            while (beg < size && data[beg] != '\n')
                beg++;

            beg++;
        }

        else if ((doc->ext_flags & HOEDOWN_EXT_FENCED_CODE) != 0 &&
            (i = parse_fencedcode(ob, doc, txt_data, end)) != 0)
            beg += i;

        else if ((doc->ext_flags & HOEDOWN_EXT_TABLES) != 0 &&
            (i = parse_table(ob, doc, txt_data, end)) != 0)
            beg += i;

        else if (prefix_quote(txt_data, end))
            beg += parse_blockquote(ob, doc, txt_data, end);

        else if (!(doc->ext_flags & HOEDOWN_EXT_DISABLE_INDENTED_CODE) && prefix_code(txt_data, end))
            beg += parse_blockcode(ob, doc, txt_data, end);

        else if (prefix_uli(txt_data, end))
            beg += parse_list(ob, doc, txt_data, end, 0);

        else if (prefix_oli(txt_data, end))
            beg += parse_list(ob, doc, txt_data, end, HOEDOWN_LIST_ORDERED);

        else
            beg += parse_paragraph(ob, doc, txt_data, end);
    }
}



/*********************
 * REFERENCE PARSING *
 *********************/

/* is_footnote • returns whether a line is a footnote definition or not */
static int
is_footnote(const uint8_t *data, size_t beg, size_t end, size_t *last, struct footnote_list *list)
{
    size_t i = 0;
    hoedown_buffer *contents = 0;
    size_t ind = 0;
    int in_empty = 0;
    size_t start = 0;

    size_t id_offset, id_end;

    /* up to 3 optional leading spaces */
    if (beg + 3 >= end) return 0;
    if (data[beg] == ' ') { i = 1;
    if (data[beg + 1] == ' ') { i = 2;
    if (data[beg + 2] == ' ') { i = 3;
    if (data[beg + 3] == ' ') return 0; } } }
    i += beg;

    /* id part: caret followed by anything between brackets */
    if (data[i] != '[') return 0;
    i++;
    if (i >= end || data[i] != '^') return 0;
    i++;
    id_offset = i;
    while (i < end && data[i] != '\n' && data[i] != '\r' && data[i] != ']')
        i++;
    if (i >= end || data[i] != ']') return 0;
    id_end = i;

    /* spacer: colon (space | tab)* newline? (space | tab)* */
    i++;
    if (i >= end || data[i] != ':') return 0;
    i++;

    /* getting content buffer */
    contents = hoedown_buffer_new(64);

    start = i;

    /* process lines similar to a list item */
    while (i < end) {
        while (i < end && data[i] != '\n' && data[i] != '\r') i++;

        /* process an empty line */
        if (is_empty(data + start, i - start)) {
            in_empty = 1;
            if (i < end && (data[i] == '\n' || data[i] == '\r')) {
                i++;
                if (i < end && data[i] == '\n' && data[i - 1] == '\r') i++;
            }
            start = i;
            continue;
        }

        /* calculating the indentation */
        ind = 0;
        while (ind < 4 && start + ind < end && data[start + ind] == ' ')
            ind++;

        /* joining only indented stuff after empty lines;
         * note that now we only require 1 space of indentation
         * to continue, just like lists */
        if (ind == 0) {
            if (start == id_end + 2 && data[start] == '\t') {}
            else break;
        }
        else if (in_empty) {
            hoedown_buffer_putc(contents, '\n');
        }

        in_empty = 0;

        /* adding the line into the content buffer */
        hoedown_buffer_put(contents, data + start + ind, i - start - ind);
        /* add carriage return */
        if (i < end) {
            hoedown_buffer_putc(contents, '\n');
            if (i < end && (data[i] == '\n' || data[i] == '\r')) {
                i++;
                if (i < end && data[i] == '\n' && data[i - 1] == '\r') i++;
            }
        }
        start = i;
    }

    if (last)
        *last = start;

    if (list) {
        struct footnote_ref *ref;
        ref = create_footnote_ref(list, data + id_offset, id_end - id_offset);
        if (!ref)
            return 0;
        if (!add_footnote_ref(list, ref)) {
            free_footnote_ref(ref);
            return 0;
        }
        ref->contents = contents;
    }

    return 1;
}

/* is_ref • returns whether a line is a reference or not */
static int
is_ref(const uint8_t *data, size_t beg, size_t end, size_t *last, struct link_ref **refs)
{
/*    int n; */
    size_t i = 0;
    size_t id_offset, id_end;
    size_t link_offset, link_end;
    size_t title_offset, title_end;
    size_t line_end;

    /* up to 3 optional leading spaces */
    if (beg + 3 >= end) return 0;
    if (data[beg] == ' ') { i = 1;
    if (data[beg + 1] == ' ') { i = 2;
    if (data[beg + 2] == ' ') { i = 3;
    if (data[beg + 3] == ' ') return 0; } } }
    i += beg;

    /* id part: anything but a newline between brackets */
    if (data[i] != '[') return 0;
    i++;
    id_offset = i;
    while (i < end && data[i] != '\n' && data[i] != '\r' && data[i] != ']')
        i++;
    if (i >= end || data[i] != ']') return 0;
    id_end = i;

    /* spacer: colon (space | tab)* newline? (space | tab)* */
    i++;
    if (i >= end || data[i] != ':') return 0;
    i++;
    while (i < end && data[i] == ' ') i++;
    if (i < end && (data[i] == '\n' || data[i] == '\r')) {
        i++;
        if (i < end && data[i] == '\r' && data[i - 1] == '\n') i++; }
    while (i < end && data[i] == ' ') i++;
    if (i >= end) return 0;

    /* link: spacing-free sequence, optionally between angle brackets */
    if (data[i] == '<')
        i++;

    link_offset = i;

    while (i < end && data[i] != ' ' && data[i] != '\n' && data[i] != '\r')
        i++;

    if (data[i - 1] == '>') link_end = i - 1;
    else link_end = i;

    /* optional spacer: (space | tab)* (newline | '\'' | '"' | '(' ) */
    while (i < end && data[i] == ' ') i++;
    if (i < end && data[i] != '\n' && data[i] != '\r'
            && data[i] != '\'' && data[i] != '"' && data[i] != '(')
        return 0;
    line_end = 0;
    /* computing end-of-line */
    if (i >= end || data[i] == '\r' || data[i] == '\n') line_end = i;
    if (i + 1 < end && data[i] == '\n' && data[i + 1] == '\r')
        line_end = i + 1;

    /* optional (space|tab)* spacer after a newline */
    if (line_end) {
        i = line_end + 1;
        while (i < end && data[i] == ' ') i++; }

    /* optional title: any non-newline sequence enclosed in '"()
                    alone on its line */
    title_offset = title_end = 0;
    if (i + 1 < end
    && (data[i] == '\'' || data[i] == '"' || data[i] == '(')) {
        i++;
        title_offset = i;
        /* looking for EOL */
        while (i < end && data[i] != '\n' && data[i] != '\r') i++;
        if (i + 1 < end && data[i] == '\n' && data[i + 1] == '\r')
            title_end = i + 1;
        else    title_end = i;
        /* stepping back */
        i -= 1;
        while (i > title_offset && data[i] == ' ')
            i -= 1;
        if (i > title_offset
        && (data[i] == '\'' || data[i] == '"' || data[i] == ')')) {
            line_end = title_end;
            title_end = i; } }

    if (!line_end || link_end == link_offset)
        return 0; /* garbage after the link empty link */

    /* a valid ref has been found, filling-in return structures */
    if (last)
        *last = line_end;

    if (refs) {
        struct link_ref *ref;

        ref = add_link_ref(refs, data + id_offset, id_end - id_offset);
        if (!ref)
            return 0;

        ref->link = hoedown_buffer_new(link_end - link_offset);
        hoedown_buffer_put(ref->link, data + link_offset, link_end - link_offset);

        if (title_end > title_offset) {
            ref->title = hoedown_buffer_new(title_end - title_offset);
            hoedown_buffer_put(ref->title, data + title_offset, title_end - title_offset);
        }
    }

    return 1;
}

static void expand_tabs(hoedown_buffer *ob, const uint8_t *line, size_t size)
{
    /* This code makes two assumptions:
     * - Input is valid UTF-8.  (Any byte with top two bits 10 is skipped,
     *   whether or not it is a valid UTF-8 continuation byte.)
     * - Input contains no combining characters.  (Combining characters
     *   should be skipped but are not.)
     */
    size_t  i = 0, tab = 0;

    while (i < size) {
        size_t org = i;

        while (i < size && line[i] != '\t') {
            /* ignore UTF-8 continuation bytes */
            if ((line[i] & 0xc0) != 0x80)
                tab++;
            i++;
        }

        if (i > org)
            hoedown_buffer_put(ob, line + org, i - org);

        if (i >= size)
            break;

        do {
            hoedown_buffer_putc(ob, ' '); tab++;
        } while (tab % 4);

        i++;
    }
}

/**********************
 * EXPORTED FUNCTIONS *
 **********************/

hoedown_document *
hoedown_document_new(
    const hoedown_renderer *renderer,
    hoedown_extensions extensions,
    size_t max_nesting)
{
    hoedown_document *doc = NULL;

    assert(max_nesting > 0 && renderer);

    doc = hoedown_malloc(sizeof(hoedown_document));
    memcpy(&doc->md, renderer, sizeof(hoedown_renderer));

    doc->data.opaque = renderer->opaque;

    hoedown_stack_init(&doc->work_bufs[BUFFER_BLOCK], 4);
    hoedown_stack_init(&doc->work_bufs[BUFFER_SPAN], 8);

    memset(doc->active_char, 0x0, 256);

    if (extensions & HOEDOWN_EXT_UNDERLINE && doc->md.underline) {
        doc->active_char['_'] = MD_CHAR_EMPHASIS;
    }

    if (doc->md.emphasis || doc->md.double_emphasis || doc->md.triple_emphasis) {
        doc->active_char['*'] = MD_CHAR_EMPHASIS;
        doc->active_char['_'] = MD_CHAR_EMPHASIS;
        if (extensions & HOEDOWN_EXT_STRIKETHROUGH)
            doc->active_char['~'] = MD_CHAR_EMPHASIS;
        if (extensions & HOEDOWN_EXT_HIGHLIGHT)
            doc->active_char['='] = MD_CHAR_EMPHASIS;
    }

    if (doc->md.codespan)
        doc->active_char['`'] = MD_CHAR_CODESPAN;

    if (doc->md.linebreak)
        doc->active_char['\n'] = MD_CHAR_LINEBREAK;

    if (doc->md.image || doc->md.link || doc->md.footnotes || doc->md.footnote_ref)
        doc->active_char['['] = MD_CHAR_LINK;

    doc->active_char['<'] = MD_CHAR_LANGLE;
    doc->active_char['\\'] = MD_CHAR_ESCAPE;
    doc->active_char['&'] = MD_CHAR_ENTITY;

    if (extensions & HOEDOWN_EXT_AUTOLINK) {
        doc->active_char[':'] = MD_CHAR_AUTOLINK_URL;
        doc->active_char['@'] = MD_CHAR_AUTOLINK_EMAIL;
        doc->active_char['w'] = MD_CHAR_AUTOLINK_WWW;
    }

    if (extensions & HOEDOWN_EXT_SUPERSCRIPT)
        doc->active_char['^'] = MD_CHAR_SUPERSCRIPT;

    if (extensions & HOEDOWN_EXT_QUOTE)
        doc->active_char['"'] = MD_CHAR_QUOTE;

    if (extensions & HOEDOWN_EXT_MATH)
        doc->active_char['$'] = MD_CHAR_MATH;

    /* Extension data */
    doc->ext_flags = extensions;
    doc->max_nesting = max_nesting;
    doc->in_link_body = 0;

    return doc;
}

void
hoedown_document_render(hoedown_document *doc, hoedown_buffer *ob, const uint8_t *data, size_t size)
{
    static const uint8_t UTF8_BOM[] = {0xEF, 0xBB, 0xBF};

    hoedown_buffer *text;
    size_t beg, end;

    int footnotes_enabled;

    text = hoedown_buffer_new(64);

    /* Preallocate enough space for our buffer to avoid expanding while copying */
    hoedown_buffer_grow(text, size);

    /* reset the references table */
    memset(&doc->refs, 0x0, REF_TABLE_SIZE * sizeof(void *));

    footnotes_enabled = doc->ext_flags & HOEDOWN_EXT_FOOTNOTES;

    /* reset the footnotes lists */
    if (footnotes_enabled) {
        memset(&doc->footnotes_found, 0x0, sizeof(doc->footnotes_found));
        memset(&doc->footnotes_used, 0x0, sizeof(doc->footnotes_used));
    }

    /* first pass: looking for references, copying everything else */
    beg = 0;

    /* Skip a possible UTF-8 BOM, even though the Unicode standard
     * discourages having these in UTF-8 documents */
    if (size >= 3 && memcmp(data, UTF8_BOM, 3) == 0)
        beg += 3;

    while (beg < size) /* iterating over lines */
        if (footnotes_enabled && is_footnote(data, beg, size, &end, &doc->footnotes_found))
            beg = end;
        else if (is_ref(data, beg, size, &end, doc->refs))
            beg = end;
        else { /* skipping to the next line */
            end = beg;
            while (end < size && data[end] != '\n' && data[end] != '\r')
                end++;

            /* adding the line body if present */
            if (end > beg)
                expand_tabs(text, data + beg, end - beg);

            while (end < size && (data[end] == '\n' || data[end] == '\r')) {
                /* add one \n per newline */
                if (data[end] == '\n' || (end + 1 < size && data[end + 1] != '\n'))
                    hoedown_buffer_putc(text, '\n');
                end++;
            }

            beg = end;
        }

    /* pre-grow the output buffer to minimize allocations */
    hoedown_buffer_grow(ob, text->size + (text->size >> 1));

    /* second pass: actual rendering */
    if (doc->md.doc_header)
        doc->md.doc_header(ob, 0, &doc->data);

    if (text->size) {
        /* adding a final newline if not already present */
        if (text->data[text->size - 1] != '\n' &&  text->data[text->size - 1] != '\r')
            hoedown_buffer_putc(text, '\n');

        parse_block(ob, doc, text->data, text->size);
    }

    /* footnotes */
    if (footnotes_enabled)
        parse_footnote_list(ob, doc, &doc->footnotes_used);

    if (doc->md.doc_footer)
        doc->md.doc_footer(ob, 0, &doc->data);

    /* clean-up */
    hoedown_buffer_free(text);
    free_link_refs(doc->refs);
    if (footnotes_enabled) {
        free_footnote_list(&doc->footnotes_found, 1);
        free_footnote_list(&doc->footnotes_used, 0);
    }

    assert(doc->work_bufs[BUFFER_SPAN].size == 0);
    assert(doc->work_bufs[BUFFER_BLOCK].size == 0);
}

void
hoedown_document_render_inline(hoedown_document *doc, hoedown_buffer *ob, const uint8_t *data, size_t size)
{
    size_t i = 0, mark;
    hoedown_buffer *text = hoedown_buffer_new(64);

    /* reset the references table */
    memset(&doc->refs, 0x0, REF_TABLE_SIZE * sizeof(void *));

    /* first pass: expand tabs and process newlines */
    hoedown_buffer_grow(text, size);
    while (1) {
        mark = i;
        while (i < size && data[i] != '\n' && data[i] != '\r')
            i++;

        expand_tabs(text, data + mark, i - mark);

        if (i >= size)
            break;

        while (i < size && (data[i] == '\n' || data[i] == '\r')) {
            /* add one \n per newline */
            if (data[i] == '\n' || (i + 1 < size && data[i + 1] != '\n'))
                hoedown_buffer_putc(text, '\n');
            i++;
        }
    }

    /* second pass: actual rendering */
    hoedown_buffer_grow(ob, text->size + (text->size >> 1));

    if (doc->md.doc_header)
        doc->md.doc_header(ob, 1, &doc->data);

    parse_inline(ob, doc, text->data, text->size);

    if (doc->md.doc_footer)
        doc->md.doc_footer(ob, 1, &doc->data);

    /* clean-up */
    hoedown_buffer_free(text);

    assert(doc->work_bufs[BUFFER_SPAN].size == 0);
    assert(doc->work_bufs[BUFFER_BLOCK].size == 0);
}

void
hoedown_document_free(hoedown_document *doc)
{
    size_t i;

    for (i = 0; i < (size_t)doc->work_bufs[BUFFER_SPAN].asize; ++i)
        hoedown_buffer_free(doc->work_bufs[BUFFER_SPAN].item[i]);

    for (i = 0; i < (size_t)doc->work_bufs[BUFFER_BLOCK].asize; ++i)
        hoedown_buffer_free(doc->work_bufs[BUFFER_BLOCK].item[i]);

    hoedown_stack_uninit(&doc->work_bufs[BUFFER_SPAN]);
    hoedown_stack_uninit(&doc->work_bufs[BUFFER_BLOCK]);

    free(doc);
}
