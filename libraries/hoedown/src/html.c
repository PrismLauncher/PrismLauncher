#include "hoedown/html.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#include "hoedown/escape.h"

#define USE_XHTML(opt) (opt->flags & HOEDOWN_HTML_USE_XHTML)

hoedown_html_tag
hoedown_html_is_tag(const uint8_t *data, size_t size, const char *tagname)
{
    size_t i;
    int closed = 0;

    if (size < 3 || data[0] != '<')
        return HOEDOWN_HTML_TAG_NONE;

    i = 1;

    if (data[i] == '/') {
        closed = 1;
        i++;
    }

    for (; i < size; ++i, ++tagname) {
        if (*tagname == 0)
            break;

        if (data[i] != *tagname)
            return HOEDOWN_HTML_TAG_NONE;
    }

    if (i == size)
        return HOEDOWN_HTML_TAG_NONE;

    if (isspace(data[i]) || data[i] == '>')
        return closed ? HOEDOWN_HTML_TAG_CLOSE : HOEDOWN_HTML_TAG_OPEN;

    return HOEDOWN_HTML_TAG_NONE;
}

static void escape_html(hoedown_buffer *ob, const uint8_t *source, size_t length)
{
    hoedown_escape_html(ob, source, length, 0);
}

static void escape_href(hoedown_buffer *ob, const uint8_t *source, size_t length)
{
    hoedown_escape_href(ob, source, length);
}

/********************
 * GENERIC RENDERER *
 ********************/
static int
rndr_autolink(hoedown_buffer *ob, const hoedown_buffer *link, hoedown_autolink_type type, const hoedown_renderer_data *data)
{
    hoedown_html_renderer_state *state = data->opaque;

    if (!link || !link->size)
        return 0;

    HOEDOWN_BUFPUTSL(ob, "<a href=\"");
    if (type == HOEDOWN_AUTOLINK_EMAIL)
        HOEDOWN_BUFPUTSL(ob, "mailto:");
    escape_href(ob, link->data, link->size);

    if (state->link_attributes) {
        hoedown_buffer_putc(ob, '\"');
        state->link_attributes(ob, link, data);
        hoedown_buffer_putc(ob, '>');
    } else {
        HOEDOWN_BUFPUTSL(ob, "\">");
    }

    /*
     * Pretty printing: if we get an email address as
     * an actual URI, e.g. `mailto:foo@bar.com`, we don't
     * want to print the `mailto:` prefix
     */
    if (hoedown_buffer_prefix(link, "mailto:") == 0) {
        escape_html(ob, link->data + 7, link->size - 7);
    } else {
        escape_html(ob, link->data, link->size);
    }

    HOEDOWN_BUFPUTSL(ob, "</a>");

    return 1;
}

static void
rndr_blockcode(hoedown_buffer *ob, const hoedown_buffer *text, const hoedown_buffer *lang, const hoedown_renderer_data *data)
{
    if (ob->size) hoedown_buffer_putc(ob, '\n');

    if (lang) {
        HOEDOWN_BUFPUTSL(ob, "<pre><code class=\"language-");
        escape_html(ob, lang->data, lang->size);
        HOEDOWN_BUFPUTSL(ob, "\">");
    } else {
        HOEDOWN_BUFPUTSL(ob, "<pre><code>");
    }

    if (text)
        escape_html(ob, text->data, text->size);

    HOEDOWN_BUFPUTSL(ob, "</code></pre>\n");
}

static void
rndr_blockquote(hoedown_buffer *ob, const hoedown_buffer *content, const hoedown_renderer_data *data)
{
    if (ob->size) hoedown_buffer_putc(ob, '\n');
    HOEDOWN_BUFPUTSL(ob, "<blockquote>\n");
    if (content) hoedown_buffer_put(ob, content->data, content->size);
    HOEDOWN_BUFPUTSL(ob, "</blockquote>\n");
}

static int
rndr_codespan(hoedown_buffer *ob, const hoedown_buffer *text, const hoedown_renderer_data *data)
{
    HOEDOWN_BUFPUTSL(ob, "<code>");
    if (text) escape_html(ob, text->data, text->size);
    HOEDOWN_BUFPUTSL(ob, "</code>");
    return 1;
}

static int
rndr_strikethrough(hoedown_buffer *ob, const hoedown_buffer *content, const hoedown_renderer_data *data)
{
    if (!content || !content->size)
        return 0;

    HOEDOWN_BUFPUTSL(ob, "<del>");
    hoedown_buffer_put(ob, content->data, content->size);
    HOEDOWN_BUFPUTSL(ob, "</del>");
    return 1;
}

static int
rndr_double_emphasis(hoedown_buffer *ob, const hoedown_buffer *content, const hoedown_renderer_data *data)
{
    if (!content || !content->size)
        return 0;

    HOEDOWN_BUFPUTSL(ob, "<strong>");
    hoedown_buffer_put(ob, content->data, content->size);
    HOEDOWN_BUFPUTSL(ob, "</strong>");

    return 1;
}

static int
rndr_emphasis(hoedown_buffer *ob, const hoedown_buffer *content, const hoedown_renderer_data *data)
{
    if (!content || !content->size) return 0;
    HOEDOWN_BUFPUTSL(ob, "<em>");
    if (content) hoedown_buffer_put(ob, content->data, content->size);
    HOEDOWN_BUFPUTSL(ob, "</em>");
    return 1;
}

static int
rndr_underline(hoedown_buffer *ob, const hoedown_buffer *content, const hoedown_renderer_data *data)
{
    if (!content || !content->size)
        return 0;

    HOEDOWN_BUFPUTSL(ob, "<u>");
    hoedown_buffer_put(ob, content->data, content->size);
    HOEDOWN_BUFPUTSL(ob, "</u>");

    return 1;
}

static int
rndr_highlight(hoedown_buffer *ob, const hoedown_buffer *content, const hoedown_renderer_data *data)
{
    if (!content || !content->size)
        return 0;

    HOEDOWN_BUFPUTSL(ob, "<mark>");
    hoedown_buffer_put(ob, content->data, content->size);
    HOEDOWN_BUFPUTSL(ob, "</mark>");

    return 1;
}

static int
rndr_quote(hoedown_buffer *ob, const hoedown_buffer *content, const hoedown_renderer_data *data)
{
    if (!content || !content->size)
        return 0;

    HOEDOWN_BUFPUTSL(ob, "<q>");
    hoedown_buffer_put(ob, content->data, content->size);
    HOEDOWN_BUFPUTSL(ob, "</q>");

    return 1;
}

static int
rndr_linebreak(hoedown_buffer *ob, const hoedown_renderer_data *data)
{
    hoedown_html_renderer_state *state = data->opaque;
    hoedown_buffer_puts(ob, USE_XHTML(state) ? "<br/>\n" : "<br>\n");
    return 1;
}

static void
rndr_header(hoedown_buffer *ob, const hoedown_buffer *content, int level, const hoedown_renderer_data *data)
{
    hoedown_html_renderer_state *state = data->opaque;

    if (ob->size)
        hoedown_buffer_putc(ob, '\n');

    if (level <= state->toc_data.nesting_level)
        hoedown_buffer_printf(ob, "<h%d id=\"toc_%d\">", level, state->toc_data.header_count++);
    else
        hoedown_buffer_printf(ob, "<h%d>", level);

    if (content) hoedown_buffer_put(ob, content->data, content->size);
    hoedown_buffer_printf(ob, "</h%d>\n", level);
}

static int
rndr_link(hoedown_buffer *ob, const hoedown_buffer *content, const hoedown_buffer *link, const hoedown_buffer *title, const hoedown_renderer_data *data)
{
    hoedown_html_renderer_state *state = data->opaque;

    HOEDOWN_BUFPUTSL(ob, "<a href=\"");

    if (link && link->size)
        escape_href(ob, link->data, link->size);

    if (title && title->size) {
        HOEDOWN_BUFPUTSL(ob, "\" title=\"");
        escape_html(ob, title->data, title->size);
    }

    if (state->link_attributes) {
        hoedown_buffer_putc(ob, '\"');
        state->link_attributes(ob, link, data);
        hoedown_buffer_putc(ob, '>');
    } else {
        HOEDOWN_BUFPUTSL(ob, "\">");
    }

    if (content && content->size) hoedown_buffer_put(ob, content->data, content->size);
    HOEDOWN_BUFPUTSL(ob, "</a>");
    return 1;
}

static void
rndr_list(hoedown_buffer *ob, const hoedown_buffer *content, hoedown_list_flags flags, const hoedown_renderer_data *data)
{
    if (ob->size) hoedown_buffer_putc(ob, '\n');
    hoedown_buffer_put(ob, (const uint8_t *)(flags & HOEDOWN_LIST_ORDERED ? "<ol>\n" : "<ul>\n"), 5);
    if (content) hoedown_buffer_put(ob, content->data, content->size);
    hoedown_buffer_put(ob, (const uint8_t *)(flags & HOEDOWN_LIST_ORDERED ? "</ol>\n" : "</ul>\n"), 6);
}

static void
rndr_listitem(hoedown_buffer *ob, const hoedown_buffer *content, hoedown_list_flags flags, const hoedown_renderer_data *data)
{
    HOEDOWN_BUFPUTSL(ob, "<li>");
    if (content) {
        size_t size = content->size;
        while (size && content->data[size - 1] == '\n')
            size--;

        hoedown_buffer_put(ob, content->data, size);
    }
    HOEDOWN_BUFPUTSL(ob, "</li>\n");
}

static void
rndr_paragraph(hoedown_buffer *ob, const hoedown_buffer *content, const hoedown_renderer_data *data)
{
    hoedown_html_renderer_state *state = data->opaque;
    size_t i = 0;

    if (ob->size) hoedown_buffer_putc(ob, '\n');

    if (!content || !content->size)
        return;

    while (i < content->size && isspace(content->data[i])) i++;

    if (i == content->size)
        return;

    HOEDOWN_BUFPUTSL(ob, "<p>");
    if (state->flags & HOEDOWN_HTML_HARD_WRAP) {
        size_t org;
        while (i < content->size) {
            org = i;
            while (i < content->size && content->data[i] != '\n')
                i++;

            if (i > org)
                hoedown_buffer_put(ob, content->data + org, i - org);

            /*
             * do not insert a line break if this newline
             * is the last character on the paragraph
             */
            if (i >= content->size - 1)
                break;

            rndr_linebreak(ob, data);
            i++;
        }
    } else {
        hoedown_buffer_put(ob, content->data + i, content->size - i);
    }
    HOEDOWN_BUFPUTSL(ob, "</p>\n");
}

static void
rndr_raw_block(hoedown_buffer *ob, const hoedown_buffer *text, const hoedown_renderer_data *data)
{
    size_t org, sz;

    if (!text)
        return;

    /* FIXME: Do we *really* need to trim the HTML? How does that make a difference? */
    sz = text->size;
    while (sz > 0 && text->data[sz - 1] == '\n')
        sz--;

    org = 0;
    while (org < sz && text->data[org] == '\n')
        org++;

    if (org >= sz)
        return;

    if (ob->size)
        hoedown_buffer_putc(ob, '\n');

    hoedown_buffer_put(ob, text->data + org, sz - org);
    hoedown_buffer_putc(ob, '\n');
}

static int
rndr_triple_emphasis(hoedown_buffer *ob, const hoedown_buffer *content, const hoedown_renderer_data *data)
{
    if (!content || !content->size) return 0;
    HOEDOWN_BUFPUTSL(ob, "<strong><em>");
    hoedown_buffer_put(ob, content->data, content->size);
    HOEDOWN_BUFPUTSL(ob, "</em></strong>");
    return 1;
}

static void
rndr_hrule(hoedown_buffer *ob, const hoedown_renderer_data *data)
{
    hoedown_html_renderer_state *state = data->opaque;
    if (ob->size) hoedown_buffer_putc(ob, '\n');
    hoedown_buffer_puts(ob, USE_XHTML(state) ? "<hr/>\n" : "<hr>\n");
}

static int
rndr_image(hoedown_buffer *ob, const hoedown_buffer *link, const hoedown_buffer *title, const hoedown_buffer *alt, const hoedown_renderer_data *data)
{
    hoedown_html_renderer_state *state = data->opaque;
    if (!link || !link->size) return 0;

    HOEDOWN_BUFPUTSL(ob, "<img src=\"");
    escape_href(ob, link->data, link->size);
    HOEDOWN_BUFPUTSL(ob, "\" alt=\"");

    if (alt && alt->size)
        escape_html(ob, alt->data, alt->size);

    if (title && title->size) {
        HOEDOWN_BUFPUTSL(ob, "\" title=\"");
        escape_html(ob, title->data, title->size); }

    hoedown_buffer_puts(ob, USE_XHTML(state) ? "\"/>" : "\">");
    return 1;
}

static int
rndr_raw_html(hoedown_buffer *ob, const hoedown_buffer *text, const hoedown_renderer_data *data)
{
    hoedown_html_renderer_state *state = data->opaque;

    /* ESCAPE overrides SKIP_HTML. It doesn't look to see if
     * there are any valid tags, just escapes all of them. */
    if((state->flags & HOEDOWN_HTML_ESCAPE) != 0) {
        escape_html(ob, text->data, text->size);
        return 1;
    }

    if ((state->flags & HOEDOWN_HTML_SKIP_HTML) != 0)
        return 1;

    hoedown_buffer_put(ob, text->data, text->size);
    return 1;
}

static void
rndr_table(hoedown_buffer *ob, const hoedown_buffer *content, const hoedown_renderer_data *data)
{
    if (ob->size) hoedown_buffer_putc(ob, '\n');
    HOEDOWN_BUFPUTSL(ob, "<table>\n");
    hoedown_buffer_put(ob, content->data, content->size);
    HOEDOWN_BUFPUTSL(ob, "</table>\n");
}

static void
rndr_table_header(hoedown_buffer *ob, const hoedown_buffer *content, const hoedown_renderer_data *data)
{
    if (ob->size) hoedown_buffer_putc(ob, '\n');
    HOEDOWN_BUFPUTSL(ob, "<thead>\n");
    hoedown_buffer_put(ob, content->data, content->size);
    HOEDOWN_BUFPUTSL(ob, "</thead>\n");
}

static void
rndr_table_body(hoedown_buffer *ob, const hoedown_buffer *content, const hoedown_renderer_data *data)
{
    if (ob->size) hoedown_buffer_putc(ob, '\n');
    HOEDOWN_BUFPUTSL(ob, "<tbody>\n");
    hoedown_buffer_put(ob, content->data, content->size);
    HOEDOWN_BUFPUTSL(ob, "</tbody>\n");
}

static void
rndr_tablerow(hoedown_buffer *ob, const hoedown_buffer *content, const hoedown_renderer_data *data)
{
    HOEDOWN_BUFPUTSL(ob, "<tr>\n");
    if (content) hoedown_buffer_put(ob, content->data, content->size);
    HOEDOWN_BUFPUTSL(ob, "</tr>\n");
}

static void
rndr_tablecell(hoedown_buffer *ob, const hoedown_buffer *content, hoedown_table_flags flags, const hoedown_renderer_data *data)
{
    if (flags & HOEDOWN_TABLE_HEADER) {
        HOEDOWN_BUFPUTSL(ob, "<th");
    } else {
        HOEDOWN_BUFPUTSL(ob, "<td");
    }

    switch (flags & HOEDOWN_TABLE_ALIGNMASK) {
    case HOEDOWN_TABLE_ALIGN_CENTER:
        HOEDOWN_BUFPUTSL(ob, " style=\"text-align: center\">");
        break;

    case HOEDOWN_TABLE_ALIGN_LEFT:
        HOEDOWN_BUFPUTSL(ob, " style=\"text-align: left\">");
        break;

    case HOEDOWN_TABLE_ALIGN_RIGHT:
        HOEDOWN_BUFPUTSL(ob, " style=\"text-align: right\">");
        break;

    default:
        HOEDOWN_BUFPUTSL(ob, ">");
    }

    if (content)
        hoedown_buffer_put(ob, content->data, content->size);

    if (flags & HOEDOWN_TABLE_HEADER) {
        HOEDOWN_BUFPUTSL(ob, "</th>\n");
    } else {
        HOEDOWN_BUFPUTSL(ob, "</td>\n");
    }
}

static int
rndr_superscript(hoedown_buffer *ob, const hoedown_buffer *content, const hoedown_renderer_data *data)
{
    if (!content || !content->size) return 0;
    HOEDOWN_BUFPUTSL(ob, "<sup>");
    hoedown_buffer_put(ob, content->data, content->size);
    HOEDOWN_BUFPUTSL(ob, "</sup>");
    return 1;
}

static void
rndr_normal_text(hoedown_buffer *ob, const hoedown_buffer *content, const hoedown_renderer_data *data)
{
    if (content)
        escape_html(ob, content->data, content->size);
}

static void
rndr_footnotes(hoedown_buffer *ob, const hoedown_buffer *content, const hoedown_renderer_data *data)
{
    hoedown_html_renderer_state *state = data->opaque;

    if (ob->size) hoedown_buffer_putc(ob, '\n');
    HOEDOWN_BUFPUTSL(ob, "<div class=\"footnotes\">\n");
    hoedown_buffer_puts(ob, USE_XHTML(state) ? "<hr/>\n" : "<hr>\n");
    HOEDOWN_BUFPUTSL(ob, "<ol>\n");

    if (content) hoedown_buffer_put(ob, content->data, content->size);

    HOEDOWN_BUFPUTSL(ob, "\n</ol>\n</div>\n");
}

static void
rndr_footnote_def(hoedown_buffer *ob, const hoedown_buffer *content, unsigned int num, const hoedown_renderer_data *data)
{
    size_t i = 0;
    int pfound = 0;

    /* insert anchor at the end of first paragraph block */
    if (content) {
        while ((i+3) < content->size) {
            if (content->data[i++] != '<') continue;
            if (content->data[i++] != '/') continue;
            if (content->data[i++] != 'p' && content->data[i] != 'P') continue;
            if (content->data[i] != '>') continue;
            i -= 3;
            pfound = 1;
            break;
        }
    }

    hoedown_buffer_printf(ob, "\n<li id=\"fn%d\">\n", num);
    if (pfound) {
        hoedown_buffer_put(ob, content->data, i);
        hoedown_buffer_printf(ob, "&nbsp;<a href=\"#fnref%d\" rev=\"footnote\">&#8617;</a>", num);
        hoedown_buffer_put(ob, content->data + i, content->size - i);
    } else if (content) {
        hoedown_buffer_put(ob, content->data, content->size);
    }
    HOEDOWN_BUFPUTSL(ob, "</li>\n");
}

static int
rndr_footnote_ref(hoedown_buffer *ob, unsigned int num, const hoedown_renderer_data *data)
{
    hoedown_buffer_printf(ob, "<sup id=\"fnref%d\"><a href=\"#fn%d\" rel=\"footnote\">%d</a></sup>", num, num, num);
    return 1;
}

static int
rndr_math(hoedown_buffer *ob, const hoedown_buffer *text, int displaymode, const hoedown_renderer_data *data)
{
    hoedown_buffer_put(ob, (const uint8_t *)(displaymode ? "\\[" : "\\("), 2);
    escape_html(ob, text->data, text->size);
    hoedown_buffer_put(ob, (const uint8_t *)(displaymode ? "\\]" : "\\)"), 2);
    return 1;
}

static void
toc_header(hoedown_buffer *ob, const hoedown_buffer *content, int level, const hoedown_renderer_data *data)
{
    hoedown_html_renderer_state *state = data->opaque;

    if (level <= state->toc_data.nesting_level) {
        /* set the level offset if this is the first header
         * we're parsing for the document */
        if (state->toc_data.current_level == 0)
            state->toc_data.level_offset = level - 1;

        level -= state->toc_data.level_offset;

        if (level > state->toc_data.current_level) {
            while (level > state->toc_data.current_level) {
                HOEDOWN_BUFPUTSL(ob, "<ul>\n<li>\n");
                state->toc_data.current_level++;
            }
        } else if (level < state->toc_data.current_level) {
            HOEDOWN_BUFPUTSL(ob, "</li>\n");
            while (level < state->toc_data.current_level) {
                HOEDOWN_BUFPUTSL(ob, "</ul>\n</li>\n");
                state->toc_data.current_level--;
            }
            HOEDOWN_BUFPUTSL(ob,"<li>\n");
        } else {
            HOEDOWN_BUFPUTSL(ob,"</li>\n<li>\n");
        }

        hoedown_buffer_printf(ob, "<a href=\"#toc_%d\">", state->toc_data.header_count++);
        if (content) hoedown_buffer_put(ob, content->data, content->size);
        HOEDOWN_BUFPUTSL(ob, "</a>\n");
    }
}

static int
toc_link(hoedown_buffer *ob, const hoedown_buffer *content, const hoedown_buffer *link, const hoedown_buffer *title, const hoedown_renderer_data *data)
{
    if (content && content->size) hoedown_buffer_put(ob, content->data, content->size);
    return 1;
}

static void
toc_finalize(hoedown_buffer *ob, int inline_render, const hoedown_renderer_data *data)
{
    hoedown_html_renderer_state *state;

    if (inline_render)
        return;

    state = data->opaque;

    while (state->toc_data.current_level > 0) {
        HOEDOWN_BUFPUTSL(ob, "</li>\n</ul>\n");
        state->toc_data.current_level--;
    }

    state->toc_data.header_count = 0;
}

hoedown_renderer *
hoedown_html_toc_renderer_new(int nesting_level)
{
    static const hoedown_renderer cb_default = {
        NULL,

        NULL,
        NULL,
        toc_header,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,

        NULL,
        rndr_codespan,
        rndr_double_emphasis,
        rndr_emphasis,
        rndr_underline,
        rndr_highlight,
        rndr_quote,
        NULL,
        NULL,
        toc_link,
        rndr_triple_emphasis,
        rndr_strikethrough,
        rndr_superscript,
        NULL,
        NULL,
        NULL,

        NULL,
        rndr_normal_text,

        NULL,
        toc_finalize
    };

    hoedown_html_renderer_state *state;
    hoedown_renderer *renderer;

    /* Prepare the state pointer */
    state = hoedown_malloc(sizeof(hoedown_html_renderer_state));
    memset(state, 0x0, sizeof(hoedown_html_renderer_state));

    state->toc_data.nesting_level = nesting_level;

    /* Prepare the renderer */
    renderer = hoedown_malloc(sizeof(hoedown_renderer));
    memcpy(renderer, &cb_default, sizeof(hoedown_renderer));

    renderer->opaque = state;
    return renderer;
}

hoedown_renderer *
hoedown_html_renderer_new(hoedown_html_flags render_flags, int nesting_level)
{
    static const hoedown_renderer cb_default = {
        NULL,

        rndr_blockcode,
        rndr_blockquote,
        rndr_header,
        rndr_hrule,
        rndr_list,
        rndr_listitem,
        rndr_paragraph,
        rndr_table,
        rndr_table_header,
        rndr_table_body,
        rndr_tablerow,
        rndr_tablecell,
        rndr_footnotes,
        rndr_footnote_def,
        rndr_raw_block,

        rndr_autolink,
        rndr_codespan,
        rndr_double_emphasis,
        rndr_emphasis,
        rndr_underline,
        rndr_highlight,
        rndr_quote,
        rndr_image,
        rndr_linebreak,
        rndr_link,
        rndr_triple_emphasis,
        rndr_strikethrough,
        rndr_superscript,
        rndr_footnote_ref,
        rndr_math,
        rndr_raw_html,

        NULL,
        rndr_normal_text,

        NULL,
        NULL
    };

    hoedown_html_renderer_state *state;
    hoedown_renderer *renderer;

    /* Prepare the state pointer */
    state = hoedown_malloc(sizeof(hoedown_html_renderer_state));
    memset(state, 0x0, sizeof(hoedown_html_renderer_state));

    state->flags = render_flags;
    state->toc_data.nesting_level = nesting_level;

    /* Prepare the renderer */
    renderer = hoedown_malloc(sizeof(hoedown_renderer));
    memcpy(renderer, &cb_default, sizeof(hoedown_renderer));

    if (render_flags & HOEDOWN_HTML_SKIP_HTML || render_flags & HOEDOWN_HTML_ESCAPE)
        renderer->blockhtml = NULL;

    renderer->opaque = state;
    return renderer;
}

void
hoedown_html_renderer_free(hoedown_renderer *renderer)
{
    free(renderer->opaque);
    free(renderer);
}
