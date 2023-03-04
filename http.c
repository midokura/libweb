#include "http.h"
#include <dynstr.h>
#include <sys/types.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

struct http_ctx
{
    struct ctx
    {
        enum state
        {
            START_LINE,
            HEADER_CR_LINE,
            BODY_LINE
        } state;

        enum
        {
            LINE_CR,
            LINE_LF
        } lstate;

        enum http_op op;
        char *resource, *field, *value, *boundary;
        size_t len;

        struct post
        {
            char *path;
            unsigned long long len, read;
        } post;

        union
        {
            struct start_line
            {
                enum
                {
                    START_LINE_OP,
                    START_LINE_RESOURCE,
                    START_LINE_PROTOCOL
                } state;
            } sl;

            struct multiform
            {
                enum mf_state
                {
                    MF_START_BOUNDARY,
                    MF_HEADER_CR_LINE,
                    MF_BODY_BOUNDARY_LINE,
                    MF_END_BOUNDARY_CR_LINE
                } state;

                enum
                {
                    BODY_CR,
                    BODY_LF,
                    BODY_DATA
                } bstate;

                off_t len, written;
                char *boundary;
                const char *dir;
                size_t blen, nforms, nfiles;
                int fd;
                struct http_post_file *files;

                struct form
                {
                    char *name, *filename, *tmpname, *value;
                } *forms;
            } mf;
        } u;
    } ctx;

    struct write_ctx
    {
        bool pending;
        enum state state;
        struct http_response r;
        off_t n;
        struct dynstr d;
    } wctx;

    enum version
    {
        HTTP_1_0,
        HTTP_1_1
    } version;

    /* From RFC9112, section 3 (Request line):
     * It is RECOMMENDED that all HTTP senders and recipients support,
     * at a minimum, request-line lengths of 8000 octets. */
    char line[8000];
    struct http_cfg cfg;
};

static const char *const versions[] =
{
    [HTTP_1_0] = "HTTP/1.0",
    [HTTP_1_1] = "HTTP/1.1"
};

static int get_version(struct http_ctx *const h, const char *const v)
{
    for (enum version i = 0; i < sizeof versions / sizeof *versions; i++)
        if (!strcmp(versions[i], v))
        {
            h->version = i;
            return 0;
        }

    return -1;
}

static int start_line(struct http_ctx *const h)
{
    const char *const line = (const char *)h->line;

    if (!*line)
    {
        fprintf(stderr, "%s: expected non-empty line\n", __func__);
        return 1;
    }

    const char *const op = strchr(line, ' ');

    if (!op || op == line)
    {
        fprintf(stderr, "%s: expected resource\n", __func__);
        return 1;
    }

    struct ctx *const c = &h->ctx;
    const size_t n = op - line;

    if (!strncmp(line, "GET", n))
        c->op = HTTP_OP_GET;
    else if (!strncmp(line, "POST", n))
        c->op = HTTP_OP_POST;
    else
    {
        fprintf(stderr, "%s: unsupported HTTP op %.*s\n",
            __func__, (int)n, line);
        return 1;
    }

    const char *const resource = op + 1,
        *const res_end = strchr(resource, ' ');

    if (!res_end)
    {
        fprintf(stderr, "%s: expected protocol version\n", __func__);
        return 1;
    }

    const size_t res_n = res_end - resource;

    if (memchr(resource, '*', res_n))
    {
        fprintf(stderr, "%s: illegal character * in resource %.*s\n",
            __func__, (int)res_n, resource);
        return 1;
    }

    const char *const protocol = res_end + 1;

    if (!*protocol)
    {
        fprintf(stderr, "%s: expected protocol version\n", __func__);
        return 1;
    }
    else if (strchr(protocol, ' '))
    {
        fprintf(stderr, "%s: unexpected field after protocol version\n",
            __func__);
        return 1;
    }

    int ret = 1;
    char *enc_res = NULL;

    if (get_version(h, protocol))
    {
        fprintf(stderr, "%s: unsupported protocol %s\n", __func__, protocol);
        goto end;
    }
    else if (!(enc_res = strndup(resource, res_n)))
    {
        fprintf(stderr, "%s: strndup(3): %s\n", __func__, strerror(errno));
        ret = -1;
        goto end;
    }
    else if (!(c->resource = http_decode_url(enc_res)))
    {
        fprintf(stderr, "%s: http_decode_url failed\n", __func__);
        goto end;
    }

    printf("%.*s %s %s\n", (int)n, line, c->resource, protocol);
    ret = 0;
    c->state = HEADER_CR_LINE;

end:
    free(enc_res);
    return ret;
}

static void ctx_free(struct ctx *const c)
{
    if (c->boundary)
    {
        struct multiform *const m = &c->u.mf;

        free(m->files);
        free(m->boundary);

        for (size_t i = 0; i < m->nforms; i++)
        {
            struct form *const f = &m->forms[i];

            free(f->name);
            free(f->filename);
            free(f->tmpname);
            free(f->value);
        }

        free(m->forms);

        if (m->fd >= 0 && close(m->fd))
            fprintf(stderr, "%s: close(2) m->fd: %s\n",
                __func__, strerror(errno));
    }

    free(c->field);
    free(c->value);
    free(c->resource);
    free(c->boundary);

    *c = (const struct ctx){0};
}

static int prepare_headers(struct http_ctx *const h)
{
    struct write_ctx *const w = &h->wctx;
    struct dynstr *const d = &w->d;

    dynstr_init(d);

    for (size_t i = 0; i < w->r.n_headers; i++)
    {
        const struct http_header *const hdr = &w->r.headers[i];

        dynstr_append_or_ret_nonzero(d, "%s: %s\r\n", hdr->header, hdr->value);
        free(hdr->header);
        free(hdr->value);
    }

    free(w->r.headers);
    dynstr_append_or_ret_nonzero(d, "\r\n");
    return 0;
}

static int rw_error(const int r, bool *const close)
{
    if (r < 0)
    {
        switch (errno)
        {
            case ECONNRESET:
                *close = true;
                return 1;

            case EAGAIN:
                return 0;

            default:
                break;
        }

        fprintf(stderr, "%s: %s\n", __func__, strerror(errno));
        return -1;
    }
    else if (!r)
    {
        *close = true;
        return 0;
    }

    fprintf(stderr, "%s: unexpected value %d\n", __func__, r);
    return -1;
}

static int write_start_line(struct http_ctx *const h, bool *const close)
{
    struct write_ctx *const w = &h->wctx;
    struct dynstr *const d = &w->d;
    const size_t rem = d->len - w->n;
    const int res = h->cfg.write(d->str + w->n, rem,
        h->cfg.user);

    if (res <= 0)
        return rw_error(res, close);
    else if ((w->n += res) >= d->len)
    {
        char len[sizeof "18446744073709551615"];
        const int res = snprintf(len, sizeof len, "%llu", w->r.n);

        dynstr_free(d);

        if (res < 0 || res >= sizeof len)
        {
            fprintf(stderr, "%s: snprintf(3) failed\n", __func__);
            return -1;
        }
        else if (http_response_add_header(&w->r, "Content-Length", len))
        {
            fprintf(stderr, "%s: http_response_add_header failed\n", __func__);
            return -1;
        }
        else if (prepare_headers(h))
        {
            fprintf(stderr, "%s: prepare_headers failed\n", __func__);
            return -1;
        }

        w->state = HEADER_CR_LINE;
        w->n = 0;
    }

    return 0;
}

static int write_ctx_free(struct write_ctx *const w)
{
    int ret = 0;
    const struct http_response *const r = &w->r;

    if (r->free)
        r->free(r->buf.rw);

    if (r->f && (ret = fclose(r->f)))
        fprintf(stderr, "%s: fclose(3): %s\n", __func__, strerror(errno));

    dynstr_free(&w->d);
    *w = (const struct write_ctx){0};
    return ret;
}

static int write_header_cr_line(struct http_ctx *const h, bool *const close)
{
    struct write_ctx *const w = &h->wctx;
    struct dynstr *const d = &w->d;
    const size_t rem = d->len - w->n;
    const int res = h->cfg.write(d->str + w->n, rem, h->cfg.user);

    if (res <= 0)
        return rw_error(res, close);
    else if ((w->n += res) >= d->len)
    {
        dynstr_free(d);

        if (w->r.n)
        {
            w->state = BODY_LINE;
            w->n = 0;
        }
        else if (write_ctx_free(w))
        {
            fprintf(stderr, "%s: write_ctx_free failed\n", __func__);
            return -1;
        }
    }

    return 0;
}

static int write_body_mem(struct http_ctx *const h, bool *const close)
{
    struct write_ctx *const w = &h->wctx;
    const struct http_response *const r = &w->r;
    const size_t rem = r->n - w->n;
    const int res = h->cfg.write((const char *)r->buf.ro + w->n, rem,
        h->cfg.user);

    if (res <= 0)
        return rw_error(res, close);
    else if ((w->n += res) >= r->n)
    {
        if (h->version == HTTP_1_0)
            *close = true;

        return write_ctx_free(w);
    }

    return 0;
}

static int write_body_file(struct http_ctx *const h, bool *const close)
{
    struct write_ctx *const w = &h->wctx;
    const struct http_response *const r = &w->r;
    const unsigned long long left = r->n - w->n;
    char buf[1024];
    const size_t rem = left > sizeof buf ? sizeof buf : left;

    if (!fread(buf, 1, rem, r->f))
    {
        fprintf(stderr, "%s: fread(3) failed, ferror=%d, feof=%d\n",
            __func__, ferror(r->f), feof(r->f));
        return -1;
    }

    const int res = h->cfg.write(buf, rem, h->cfg.user);

    if (res <= 0)
        return rw_error(res, close);
    else if ((w->n += res) >= r->n)
    {
        if (h->version == HTTP_1_0)
            *close = true;

        return write_ctx_free(w);
    }

    return 0;
}

static int write_body_line(struct http_ctx *const h, bool *const close)
{
    const struct http_response *const r = &h->wctx.r;

    if (r->buf.ro)
        return write_body_mem(h, close);
    else if (r->f)
        return write_body_file(h, close);

    fprintf(stderr, "%s: expected either buffer or file path\n", __func__);
    return -1;
}

static int http_write(struct http_ctx *const h, bool *const close)
{
    static int (*const fn[])(struct http_ctx *, bool *) =
    {
        [START_LINE] = write_start_line,
        [HEADER_CR_LINE] = write_header_cr_line,
        [BODY_LINE] = write_body_line,
    };

    struct write_ctx *const w = &h->wctx;

    const int ret = fn[w->state](h, close);

    if (ret)
        write_ctx_free(w);

    return ret;
}

int http_response_add_header(struct http_response *const r,
    const char *const header, const char *const value)
{
    const size_t n = r->n_headers + 1;
    struct http_header *const headers = realloc(r->headers,
        n * sizeof *r->headers), *h = NULL;

    if (!headers)
    {
        fprintf(stderr, "%s: realloc(3): %s\n", __func__, strerror(errno));
        return -1;
    }

    h = &headers[r->n_headers];

    *h = (const struct http_header)
    {
        .header = strdup(header),
        .value = strdup(value)
    };

    if (!h->header || !h->value)
    {
        fprintf(stderr, "%s: strdup(3): %s\n", __func__, strerror(errno));
        free(h->header);
        free(h->value);
        return -1;
    }

    r->headers = headers;
    r->n_headers = n;
    return 0;
}

static int start_response(struct http_ctx *const h)
{
    static const struct code
    {
        const char *descr;
        int code;
    } codes[] =
    {
#define X(x, y, z) [HTTP_STATUS_##x] = {.descr = y, .code = z},
        HTTP_STATUSES
#undef X
    };

    struct write_ctx *const w = &h->wctx;
    const struct code *const c = &codes[w->r.status];

    w->pending = true;
    dynstr_init(&w->d);
    dynstr_append_or_ret_nonzero(&w->d, "%s %d %s\r\n",
        versions[h->version], c->code, c->descr);
    return 0;
}

static int set_cookie(struct http_ctx *const h, const char *const cookie)
{
    struct ctx *const c = &h->ctx;
    const char *const value = strchr(cookie, '=');

    if (!value)
    {
        fprintf(stderr, "%s: expected field=value for cookie %s\n",
            __func__, cookie);
        goto failure;
    }
    else if (!*(value + 1))
    {
        fprintf(stderr, "%s: expected non-empty value for cookie %s\n",
            __func__, cookie);
        goto failure;
    }
    else if (!(c->field = strndup(cookie, value - cookie)))
    {
        fprintf(stderr, "%s: malloc(3) field: %s\n", __func__, strerror(errno));
        goto failure;
    }
    else if (!(c->value = strdup(value + 1)))
    {
        fprintf(stderr, "%s: malloc(3) value: %s\n", __func__, strerror(errno));
        goto failure;
    }

    return 0;

failure:
    free(c->field);
    free(c->value);
    return -1;
}

static int set_length(struct http_ctx *const h, const char *const len)
{
    h->ctx.post.len = strtoull(len, NULL, 10);
    return 0;
}

static int set_content_type(struct http_ctx *const h, const char *const type)
{
    const char *const sep = strchr(type, ';');

    if (!sep)
        /* No multipart/form-data expected. */
        return 0;

    const size_t n = sep - type;

    if (strncmp(type, "multipart/form-data", n))
    {
        fprintf(stderr, "%s: unsupported Content-Type %.*s\n",
            __func__, (int)n, type);
        return 1;
    }

    const char *boundary = sep + 1;

    while (*boundary == ' ')
        boundary++;

    if (!*boundary)
    {
        fprintf(stderr, "%s: expected boundary\n", __func__);
        return 1;
    }

    const char *const eq = strchr(boundary, '=');

    if (!eq)
    {
        fprintf(stderr, "%s: expected = after boundary\n", __func__);
        return 1;
    }

    const size_t bn = eq - boundary;

    if (strncmp(boundary, "boundary", bn))
    {
        fprintf(stderr, "%s: expected boundary, got %.*s\n",
            __func__, (int)bn, boundary);
        return 1;
    }

    const char *val = eq + 1;

    while (*val == ' ')
        val++;

    if (!*val)
    {
        fprintf(stderr, "%s: expected value after boundary\n", __func__);
        return 1;
    }

    struct ctx *const c = &h->ctx;
    struct dynstr b;

    dynstr_init(&b);

    if (dynstr_append(&b, "\r\n--%s", val))
    {
        fprintf(stderr, "%s: dynstr_append failed\n", __func__);
        return -1;
    }

    c->boundary = b.str;
    c->u.mf = (const struct multiform){.fd = -1};
    return 0;
}

static int payload_get(struct http_ctx *const h, const char *const line)
{
    struct ctx *const c = &h->ctx;
    const struct http_payload p =
    {
        .cookie =
        {
            .field = c->field,
            .value = c->value
        },

        .op = c->op,
        .resource = c->resource
    };

    const int ret = h->cfg.payload(&p, &h->wctx.r, h->cfg.user);

    ctx_free(c);

    if (ret)
        return ret;

    return start_response(h);
}

static int payload_post(struct http_ctx *const h, const char *const line)
{
    struct ctx *const c = &h->ctx;
    const struct http_payload pl =
    {
        .cookie =
        {
            .field = c->field,
            .value = c->value
        },

        .op = c->op,
        .resource = c->resource
    };

    const int ret = h->cfg.payload(&pl, &h->wctx.r, h->cfg.user);

    ctx_free(c);

    if (ret)
        return ret;

    return start_response(h);
}

static int get_field_value(const char *const line, size_t *const n,
    const char **const value)
{
    const char *const field = strchr(line, ':');

    if (!field || line == field)
    {
        fprintf(stderr, "%s: expected field:value\n", __func__);
        return 1;
    }

    *n = field - line;
    *value = field + 1;

    if (!**value)
    {
        fprintf(stderr, "%s: expected value\n", __func__);
        return 1;
    }

    while (**value == ' ')
        (*value)++;

    return 0;
}

static int expect(struct http_ctx *const h, const char *const value)
{
    if (!strcmp(value, "100-continue"))
    {
        struct ctx *const c = &h->ctx;
        const struct http_payload p =
        {
            .u.post.expect_continue = true,
            .cookie =
            {
                .field = c->field,
                .value = c->value
            },

            .op = c->op,
            .resource = c->resource
        };

        const int ret = h->cfg.payload(&p, &h->wctx.r, h->cfg.user);

        if (ret)
            return ret;

        return start_response(h);
    }

    return 0;
}

static int process_header(struct http_ctx *const h, const char *const line,
    const size_t n, const char *const value)
{
    static const struct header
    {
        const char *header;
        int (*f)(struct http_ctx *, const char *);
    } headers[] =
    {
        {
            .header = "Cookie",
            .f = set_cookie
        },

        {
            .header = "Content-Length",
            .f = set_length
        },

        {
            .header = "Expect",
            .f = expect
        },

        {
            .header = "Content-Type",
            .f = set_content_type
        }
    };

    for (size_t i = 0; i < sizeof headers / sizeof *headers; i++)
    {
        const struct header *const hdr = &headers[i];
        int ret;

        if (!strncmp(line, hdr->header, n) && (ret = hdr->f(h, value)))
            return ret;
    }

    return 0;
}

static int header_cr_line(struct http_ctx *const h)
{
    const char *const line = (const char *)h->line;
    struct ctx *const c = &h->ctx;

    if (!*line)
    {
        switch (c->op)
        {
            case HTTP_OP_GET:
                return payload_get(h, line);

            case HTTP_OP_POST:
                if (!c->post.len)
                    return payload_post(h, line);

                c->state = BODY_LINE;
                return 0;
        }
    }

    const char *value;
    size_t n;
    const int ret = get_field_value(line, &n, &value);

    if (ret)
        return ret;

    return process_header(h, line, n, value);
}

static int send_payload(struct http_ctx *const h,
    const struct http_payload *const p)
{
    struct ctx *const c = &h->ctx;
    const int ret = h->cfg.payload(p, &h->wctx.r, h->cfg.user);

    ctx_free(c);

    if (ret)
        return ret;

    return start_response(h);
}

static int update_lstate(struct http_ctx *const h, bool *const close,
    int (*const f)(struct http_ctx *), const char b)
{
    int ret = 1;
    struct ctx *const c = &h->ctx;

    switch (c->lstate)
    {
        case LINE_CR:
            if (b == '\r')
                c->lstate = LINE_LF;
            else if (c->len < sizeof h->line - 2)
                h->line[c->len++] = b;
            else
            {
                fprintf(stderr, "%s: line too long\n", __func__);
                goto failure;
            }

            break;

        case LINE_LF:
            if (b == '\n')
            {
                h->line[c->len] = '\0';

                if ((ret = f(h)))
                    goto failure;

                c->len = 0;
            }
            else if (c->len < sizeof h->line - 3)
            {
                h->line[c->len++] = '\r';
                h->line[c->len++] = b;
            }
            else
            {
                fprintf(stderr, "%s: line too long\n", __func__);
                goto failure;
            }

            c->lstate = LINE_CR;
            break;
    }

    return 0;

failure:
    ctx_free(c);
    return ret;
}

static int start_boundary_line(struct http_ctx *const h)
{
    struct ctx *const c = &h->ctx;
    struct multiform *const m = &c->u.mf;
    const char *const line = h->line;

    if (strcmp(line, c->boundary + strlen("\r\n")))
    {
        fprintf(stderr, "%s: expected boundary %s, got %s\n",
            __func__, c->boundary, line);
        return 1;
    }

    m->state = MF_HEADER_CR_LINE;
    m->len = strlen(line) + strlen("\r\n");
    return 0;
}

static int cd_fields(struct http_ctx *const h, struct form *const f,
    const char *sep)
{
    do
    {
        while (*++sep == ' ')
            ;

        if (!*sep)
            break;

        const char *const op = strchr(sep, '=');

        if (!op)
        {
            fprintf(stderr, "%s: expected attr=value\n", __func__);
            return 1;
        }

        const char *const value = op + 1, *const end = strchr(value, ';');
        const size_t vlen = end ? end - value : strlen(value);

        if (*value != '\"' || value[vlen - 1] != '\"')
        {
            fprintf(stderr, "%s: expected \"-enclosed value, got %.*s\n",
                __func__, (int)vlen, value);
            return 1;
        }

        const char *const evalue = value + 1;
        const size_t evlen = vlen - 2;

        if (!strncmp(sep, "name", op - sep))
        {
            if (!evlen)
            {
                fprintf(stderr, "%s: expected non-empty name\n", __func__);
                return 1;
            }
            else if (!(f->name = strndup(evalue, evlen)))
            {
                fprintf(stderr, "%s: strndup(3): %s\n",
                    __func__, strerror(errno));
                return -1;
            }
        }
        else if (!strncmp(sep, "filename", op - sep))
        {
            if (!evlen)
            {
                fprintf(stderr, "%s: expected non-empty filename\n", __func__);
                return 1;
            }
            else if (!(f->filename = strndup(evalue, evlen)))
            {
                fprintf(stderr, "%s: strndup(3): %s\n", __func__, strerror(errno));
                return -1;
            }
        }
    } while ((sep = strchr(sep, ';')));

    if (!f->name)
    {
        fprintf(stderr, "%s: expected name\n", __func__);
        return 1;
    }

    return 0;
}

static int set_content_disposition(struct http_ctx *const h,
    const char *const c)
{
    const char *const sep = strchr(c, ';');
    struct multiform *const m = &h->ctx.u.mf;

    if (!sep)
    {
        fprintf(stderr, "%s: no fields found\n", __func__);
        return 1;
    }
    else if (strncmp(c, "form-data", sep - c))
    {
        fprintf(stderr, "%s: expected form-data, got %.*s\n",
            __func__, (int)(sep - c), c);
        return 1;
    }

    const size_t n = m->nforms + 1;
    struct form *const forms = realloc(m->forms, n * sizeof *m->forms);

    if (!forms)
    {
        fprintf(stderr, "%s: realloc(3): %s\n", __func__, strerror(errno));
        return -1;
    }

    struct form *const f = &forms[m->nforms];

    *f = (const struct form){0};
    m->nforms = n;
    m->forms = forms;
    return cd_fields(h, f, sep);
}

static int mf_header_cr_line(struct http_ctx *const h)
{
    struct ctx *const c = &h->ctx;
    struct multiform *const m = &c->u.mf;
    const char *const line = h->line;

    m->len += strlen(line) + strlen("\r\n");

    if (!*line)
    {
        const size_t n = strlen("\r\n") + strlen(c->boundary) + 1;

        if (!m->boundary && !(m->boundary = calloc(1, n)))
        {
            fprintf(stderr, "%s: malloc(3): %s\n", __func__, strerror(errno));
            return -1;
        }

        m->state = MF_BODY_BOUNDARY_LINE;
        return 0;
    }

    const char *value;
    size_t n;
    int ret = get_field_value(line, &n, &value);

    if (ret)
        return ret;
    else if (!strncmp(line, "Content-Disposition", n)
        && (ret = set_content_disposition(h, value)))
        return ret;

    return 0;
}

static int end_boundary_line(struct http_ctx *const h)
{
    const char *const line = h->line;

    if (!*line)
    {
        h->ctx.u.mf.state = MF_HEADER_CR_LINE;
        return 0;
    }
    else if (!strcmp(line, "--"))
    {
        /* Found end boundary. */
        struct ctx *const c = &h->ctx;
        struct multiform *const m = &c->u.mf;

        const struct http_payload p =
        {
            .cookie =
            {
                .field = c->field,
                .value = c->value
            },

            .op = c->op,
            .resource = c->resource,
            .u.post =
            {
                .dir = m->dir,
                .files = m->files,
                .n = m->nfiles
            }
        };

        return send_payload(h, &p);
    }

    fprintf(stderr, "%s: unexpected line after boundary: %s\n",
        __func__, line);
    return 1;
}

static int process_mf_line(struct http_ctx *const h)
{
    static int (*const state[])(struct http_ctx *) =
    {
        [MF_START_BOUNDARY] = start_boundary_line,
        [MF_HEADER_CR_LINE] = mf_header_cr_line,
        [MF_END_BOUNDARY_CR_LINE] = end_boundary_line
    };

    h->ctx.post.read += strlen(h->line) + strlen("\r\n");
    return state[h->ctx.u.mf.state](h);
}

static char *get_tmp(const char *const tmpdir)
{
    struct dynstr d;

    dynstr_init(&d);

    if (dynstr_append(&d, "%s/tmp.XXXXXX", tmpdir))
    {
        fprintf(stderr, "%s: dynstr_append failed\n", __func__);
        return NULL;
    }

    return d.str;
}

static int generate_mf_file(struct http_ctx *const h)
{
    struct multiform *const m = &h->ctx.u.mf;
    struct form *const f = &m->forms[m->nforms - 1];

    if (!(f->tmpname = get_tmp(h->cfg.tmpdir)))
    {
        fprintf(stderr, "%s: get_tmp failed\n", __func__);
        return -1;
    }
    else if ((m->fd = mkstemp(f->tmpname)) < 0)
    {
        fprintf(stderr, "%s: mkstemp(3): %s\n", __func__, strerror(errno));
        return -1;
    }

    return 0;
}

static int read_mf_body_to_mem(struct http_ctx *const h, const void *const buf,
    const size_t n)
{
    struct ctx *const c = &h->ctx;
    struct multiform *const m = &c->u.mf;

    if (m->written + n > sizeof h->line)
    {
        fprintf(stderr, "%s: maximum length exceeded\n", __func__);
        return 1;
    }

    memcpy(&h->line[m->written], buf, n);
    m->written += n;
    m->len += n;
    c->post.read += n;
    return 0;
}

static int read_mf_body_to_file(struct http_ctx *const h, const void *const buf,
    const size_t n)
{
    struct ctx *const c = &h->ctx;
    struct multiform *const m = &c->u.mf;
    ssize_t res;

    if (m->fd < 0 && generate_mf_file(h))
    {
        fprintf(stderr, "%s: generate_mf_file failed\n", __func__);
        return -1;
    }
    else if ((res = pwrite(m->fd, buf, n, m->written)) < 0)
    {
        fprintf(stderr, "%s: pwrite(2): %s\n", __func__, strerror(errno));
        return -1;
    }

    m->written += res;
    m->len += res;
    c->post.read += res;
    return 0;
}

static int reset_boundary(struct http_ctx *const h, const void *const buf,
    const size_t n)
{
    struct multiform *const m = &h->ctx.u.mf;
    struct form *const f = &m->forms[m->nforms - 1];
    const size_t len = strlen(m->boundary);
    int (*const read_mf)(struct http_ctx *, const void *, size_t) =
        f->filename ? read_mf_body_to_file : read_mf_body_to_mem;
    const int res = read_mf(h, m->boundary, len);

    if (res)
        return res;

    memset(m->boundary, '\0', len);
    m->blen = 0;
    return read_mf(h, buf, n);
}

static int apply_from_file(struct http_ctx *const h, struct form *const f)
{
    struct multiform *const m = &h->ctx.u.mf;

    if (close(m->fd))
    {
        fprintf(stderr, "%s: close(2): %s\n", __func__, strerror(errno));
        return -1;
    }

    m->fd = -1;

    const size_t n = m->nfiles + 1;
    struct http_post_file *const files = realloc(m->files,
        n * sizeof *m->files);

    if (!files)
    {
        fprintf(stderr, "%s: realloc(3): %s\n", __func__, strerror(errno));
        return -1;
    }

    struct http_post_file *const pf = &files[m->nfiles];

    *pf = (const struct http_post_file)
    {
        .tmpname = f->tmpname,
        .filename = f->filename
    };

    m->files = files;
    m->nfiles = n;
    return 0;
}

static int apply_from_mem(struct http_ctx *const h, struct form *const f)
{
    struct multiform *const m = &h->ctx.u.mf;

    if (!(f->value = strndup(h->line, m->written)))
    {
        fprintf(stderr, "%s: strndup(3): %s\n", __func__, strerror(errno));
        return -1;
    }
    else if (!strcmp(f->name, "dir"))
    {
        if (m->dir)
        {
            fprintf(stderr, "%s: \"dir\" defined more than once\n", __func__);
            return 1;
        }

        m->dir = f->value;
    }

    return 0;
}

static int read_mf_body_boundary_byte(struct http_ctx *const h, const char b,
    const size_t len)
{
    struct ctx *const c = &h->ctx;
    struct multiform *const m = &c->u.mf;

    if (b == c->boundary[m->blen])
    {
        m->boundary[len] = b;

        if (++m->blen >= strlen(c->boundary))
        {
            /* Found intermediate boundary. */
            struct form *const f = &m->forms[m->nforms - 1];
            const int ret = f->filename ? apply_from_file(h, f)
                : apply_from_mem(h, f);

            memset(m->boundary, '\0', len + 1);
            m->blen = 0;
            m->state = MF_END_BOUNDARY_CR_LINE;
            m->written = 0;
            return ret;
        }
    }

    return 0;
}

/* Similar to memmem(3), provided here to avoid the use of GNU extensions. */
static const char *http_memmem(const char *const a, const void *const b,
    const size_t n)
{
    const size_t len = strlen(a);

    if (len > n)
        return NULL;

    const char *s = a, *st = NULL;

    for (size_t i = 0; i < n; i++)
    {
        const char *bc = b;
        const char c = bc[i];

        if (*s == c)
        {
            if (!st)
                st = &bc[i];

            if (!*++s)
                return st;
        }
        else if (*(s = a) != c)
            st = NULL;
        else
        {
            st = &bc[i];

            if (!*++s)
                return st;
        }
    }

    return NULL;
}

static int read_mf_body_boundary(struct http_ctx *const h,
    const char **const buf, size_t *const n)
{
    struct ctx *const c = &h->ctx;
    struct multiform *const m = &c->u.mf;
    const char *const boundary = http_memmem(&c->boundary[m->blen], *buf, *n);
    int res;

    if (!boundary)
    {
        if ((res = reset_boundary(h, *buf, *n)))
            return res;

        *n = 0;
        return 0;
    }

    const size_t prev = boundary - *buf;

    if ((res = reset_boundary(h, *buf, prev)))
        return res;

    *buf += prev;
    *n -= prev;

    const size_t len = strlen(m->boundary),
        rem = strlen(c->boundary) - len,
        r = rem > *n ? *n : rem;

    for (size_t i = 0; i < r; i++)
    {
        const char *const b = *buf;

        if ((res = read_mf_body_boundary_byte(h, b[i], len)))
            return res;
    }

    *buf += r;
    *n -= r;
    return 0;
}

static int read_multiform_n(struct http_ctx *const h, bool *const close,
    const char *buf, size_t n)
{
    struct multiform *const m = &h->ctx.u.mf;

    while (n)
    {
        int res;

        switch (m->state)
        {
            case MF_START_BOUNDARY:
                /* Fall through. */
            case MF_HEADER_CR_LINE:
                /* Fall through. */
            case MF_END_BOUNDARY_CR_LINE:
            {
                if ((res = update_lstate(h, close, process_mf_line, *buf)))
                    return res;

                buf++;
                n--;
            }

                break;

            case MF_BODY_BOUNDARY_LINE:
                if ((res = read_mf_body_boundary(h, &buf, &n)))
                    return res;
        }
    }

    return 0;
}

static int read_multiform(struct http_ctx *const h, bool *const close)
{
    /* Note: the larger the buffer below, the less CPU load. */
    char buf[sizeof h->line];
    struct post *const p = &h->ctx.post;
    const unsigned long long left = p->len - p->read;
    const size_t rem = left > sizeof buf ? sizeof buf : left;
    const int r = h->cfg.read(buf, rem, h->cfg.user);

    if (r <= 0)
        return rw_error(r, close);

    return read_multiform_n(h, close, buf, r);
}

static int read_body_to_mem(struct http_ctx *const h, bool *const close)
{
    char b;
    const int r = h->cfg.read(&b, sizeof b, h->cfg.user);

    if (r <= 0)
        return rw_error(r, close);

    struct ctx *const c = &h->ctx;
    struct post *const p = &c->post;

    if (p->read >= sizeof h->line)
    {
        fprintf(stderr, "%s: exceeded maximum length\n", __func__);
        return 1;
    }

    h->line[p->read++] = b;

    if (p->read >= p->len)
    {
        const struct http_payload pl =
        {
            .cookie =
            {
                .field = c->field,
                .value = c->value
            },

            .op = c->op,
            .resource = c->resource,
            .u.post =
            {
                .data = h->line,
                .n = p->len
            }
        };

        return send_payload(h, &pl);
    }

    return 0;
}

static int read_body(struct http_ctx *const h, bool *const close)
{
    return h->ctx.boundary ? read_multiform(h, close)
        : read_body_to_mem(h, close);
}

static int process_line(struct http_ctx *const h)
{
    static int (*const state[])(struct http_ctx *) =
    {
        [START_LINE] = start_line,
        [HEADER_CR_LINE] = header_cr_line
    };

    return state[h->ctx.state](h);
}

static int http_read(struct http_ctx *const h, bool *const close)
{
    switch (h->ctx.state)
    {
        case START_LINE:
            /* Fall through. */
        case HEADER_CR_LINE:
        {
            char b;
            const int r = h->cfg.read(&b, sizeof b, h->cfg.user);

            if (r <= 0)
                return rw_error(r, close);

            return update_lstate(h, close, process_line, b);
        }

        case BODY_LINE:
            return read_body(h, close);
    }

    fprintf(stderr, "%s: unexpected state %d\n", __func__, h->ctx.state);
    return -1;
}

char *http_cookie_create(const char *const key, const char *const value)
{
    struct dynstr d;

    dynstr_init(&d);
    dynstr_append_or_ret_null(&d, "%s=%s; HttpOnly", key, value);
    return d.str;
}

int http_update(struct http_ctx *const h, bool *const write, bool *const close)
{
    *close = false;

    struct write_ctx *const w = &h->wctx;
    const int ret = w->pending ? http_write(h, close) : http_read(h, close);

    *write = w->pending;
    return ret;
}

void http_free(struct http_ctx *const h)
{
    if (h)
    {
        ctx_free(&h->ctx);
        write_ctx_free(&h->wctx);
    }

    free(h);
}

struct http_ctx *http_alloc(const struct http_cfg *const cfg)
{
    struct http_ctx *const h = malloc(sizeof *h);

    if (!h)
    {
        fprintf(stderr, "%s: malloc(3): %s\n", __func__, strerror(errno));
        goto failure;
    }

    *h = (const struct http_ctx)
    {
        .cfg = *cfg
    };

    return h;

failure:
    http_free(h);
    return NULL;
}

char *http_encode_url(const char *url)
{
    struct dynstr d;
    char c;

    dynstr_init(&d);

    while ((c = *url++))
    {
        /* Unreserved characters must not be percent-encoded. */
        if ((c >= 'A' && c <= 'Z')
            || (c >= 'a' && c <= 'z')
            || (c == '-')
            || (c == '_')
            || (c == '.')
            || (c == '/')
            || (c == '~'))
        {
            if (dynstr_append(&d, "%c", c))
            {
                fprintf(stderr, "%s: dynstr_append failed\n", __func__);
                goto failure;
            }
        }
        else if (dynstr_append(&d, "%%%hhx", c))
        {
            fprintf(stderr, "%s: dynstr_append failed\n", __func__);
            goto failure;
        }
    }

    return d.str;

failure:
    dynstr_free(&d);
    return NULL;
}

char *http_decode_url(const char *url)
{
    char *ret = NULL;
    size_t n = 0;

    while (*url)
    {
        char *const r = realloc(ret, n + 1);

        if (!r)
        {
            fprintf(stderr, "%s: realloc(3) loop: %s\n",
                __func__, strerror(errno));
            goto failure;
        }

        ret = r;

        if (*url != '%')
            ret[n++] = *url++;
        else if (*(url + 1) && *(url + 2))
        {
            const char buf[sizeof "00"] = {*(url + 1), *(url + 2)};

            ret[n++] = strtoul(buf, NULL, 16);
            url += 3;
        }
        else
        {
            fprintf(stderr, "%s: unterminated %%\n", __func__);
            goto failure;
        }
    }

    char *const r = realloc(ret, n + 1);

    if (!r)
    {
        fprintf(stderr, "%s: realloc(3) end: %s\n", __func__, strerror(errno));
        goto failure;
    }

    ret = r;
    ret[n] = '\0';
    return ret;

failure:
    free(ret);
    return NULL;
}
