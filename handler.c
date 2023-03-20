#include "handler.h"
#include "http.h"
#include "server.h"
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

struct handler
{
    struct handler_cfg cfg;
    struct elem
    {
        char *url;
        enum http_op op;
        handler_fn f;
        void *user;
    } *elem;

    struct server *server;
    struct client
    {
        struct handler *h;
        struct server_client *c;
        struct http_ctx *http;
        struct client *next;
    } *clients;

    size_t n_cfg;
};

static int on_read(void *const buf, const size_t n, void *const user)
{
    struct client *const c = user;

    return server_read(buf, n, c->c);
}

static int on_write(const void *const buf, const size_t n, void *const user)
{
    struct client *const c = user;

    return server_write(buf, n, c->c);
}

static int wildcard_cmp(const char *s, const char *p)
{
    while (*p && *s)
    {
        const char *const wc = strchr(p, '*');

        if (!wc)
            return strcmp(s, p);

        const size_t n = wc - p;

        if (n)
        {
            const int r = strncmp(s, p, n);

            if (r)
                return r;

            p += n;
            s += n;
        }
        else if (*(wc + 1) == *s)
        {
            p = wc + 1;
            s += n;
        }
        else if (*(wc + 1) == '*')
            p++;
        else
        {
            s++;
            p += n;
        }
    }

    while (*p)
        if (*p++ != '*')
            return -1;

    return 0;
}

static int on_payload(const struct http_payload *const p,
    struct http_response *const r, void *const user)
{
    struct client *const c = user;
    struct handler *const h = c->h;

    for (size_t i = 0; i < h->n_cfg; i++)
    {
        const struct elem *const e = &h->elem[i];

        if (e->op == p->op && !wildcard_cmp(p->resource, e->url))
            return e->f(p, r, e->user);
    }

    fprintf(stderr, "Not found: %s\n", p->resource);

    *r = (const struct http_response)
    {
        .status = HTTP_STATUS_NOT_FOUND
    };

    return 0;
}

static int on_length(const unsigned long long len,
    const struct http_cookie *const c, struct http_response *const r,
    void *const user)
{
    struct client *const cl = user;
    struct handler *const h = cl->h;

    if (h->cfg.length)
        return h->cfg.length(len, c, r, h->cfg.user);

    return 0;
}

static struct client *find_or_alloc_client(struct handler *const h,
    struct server_client *const c)
{
    for (struct client *cl = h->clients; cl; cl = cl->next)
    {
        if (cl->c == c)
            return cl;
    }

    struct client *const ret = malloc(sizeof *ret);

    if (!ret)
    {
        fprintf(stderr, "%s: malloc(3): %s\n", __func__, strerror(errno));
        return NULL;
    }

    const struct http_cfg cfg =
    {
        .read = on_read,
        .write = on_write,
        .payload = on_payload,
        .length = on_length,
        .user = ret,
        .tmpdir = h->cfg.tmpdir
    };

    *ret = (const struct client)
    {
        .c = c,
        .h = h,
        .http = http_alloc(&cfg)
    };

    if (!ret->http)
    {
        fprintf(stderr, "%s: http_alloc failed\n", __func__);
        return NULL;
    }

    if (!h->clients)
        h->clients = ret;
    else
    {
        for (struct client *c = h->clients; c; c = c->next)
            if (!c->next)
            {
                c->next = ret;
                break;
            }
    }

    return ret;
}

static void client_free(struct client *const c)
{
    if (c)
        http_free(c->http);

    free(c);
}

static int remove_client_from_list(struct handler *const h,
    struct client *const c)
{
    int ret = -1;

    if (server_client_close(h->server, c->c))
    {
        fprintf(stderr, "%s: server_client_close failed\n",
            __func__);
        goto end;
    }

    for (struct client *cl = h->clients, *prev = NULL; cl;
        prev = cl, cl = cl->next)
    {
        if (cl == c)
        {
            if (!prev)
                h->clients = c->next;
            else
                prev->next = cl->next;

            break;
        }
    }

    ret = 0;

end:
    client_free(c);
    return ret;
}

int handler_listen(struct handler *const h, const short port)
{
    if (!(h->server = server_init(port)))
    {
        fprintf(stderr, "%s: server_init failed\n", __func__);
        return -1;
    }

    for (;;)
    {
        bool exit, io;
        struct server_client *const c = server_select(h->server, &io, &exit);

        if (exit)
        {
            printf("Exiting...\n");
            break;
        }
        else if (!c)
        {
            fprintf(stderr, "%s: server_select failed\n", __func__);
            return -1;
        }

        struct client *const cl = find_or_alloc_client(h, c);

        if (!cl)
        {
            fprintf(stderr, "%s: find_or_alloc_client failed\n", __func__);
            return -1;
        }
        else if (io)
        {
            bool write, close;
            const int res = http_update(cl->http, &write, &close);

            if (res || close)
            {
                if (res < 0)
                {
                    fprintf(stderr, "%s: http_update failed\n", __func__);
                    return -1;
                }
                else if (remove_client_from_list(h, cl))
                {
                    fprintf(stderr, "%s: remove_client_from_list failed\n",
                        __func__);
                    return -1;
                }
            }
            else
                server_client_write_pending(cl->c, write);
        }
    }

    return 0;
}

static void free_clients(struct handler *const h)
{
    for (struct client *c = h->clients; c;)
    {
        struct client *const next = c->next;

        server_client_close(h->server, c->c);
        client_free(c);
        c = next;
    }
}

void handler_free(struct handler *const h)
{
    if (h)
    {
        for (size_t i = 0; i < h->n_cfg; i++)
            free(h->elem[i].url);

        free(h->elem);
        free_clients(h);
        server_close(h->server);
    }

    free(h);
}

struct handler *handler_alloc(const struct handler_cfg *const cfg)
{
    struct handler *const h = malloc(sizeof *h);

    if (!h)
    {
        fprintf(stderr, "%s: malloc(3) handler: %s\n",
            __func__, strerror(errno));
        return NULL;
    }

    *h = (const struct handler){.cfg = *cfg};
    return h;
}

int handler_add(struct handler *const h, const char *url,
    const enum http_op op, const handler_fn f, void *const user)
{
    const size_t n = h->n_cfg + 1;
    struct elem *const elem = realloc(h->elem, n * sizeof *h->elem);

    if (!elem)
    {
        fprintf(stderr, "%s: realloc(3): %s\n", __func__, strerror(errno));
        return -1;
    }

    struct elem *const e = &elem[h->n_cfg];

    *e = (const struct elem)
    {
        .url = strdup(url),
        .op = op,
        .f = f,
        .user = user
    };

    if (!e->url)
    {
        fprintf(stderr, "%s: strdup(3): %s\n", __func__, strerror(errno));
        return -1;
    }

    h->elem = elem;
    h->n_cfg = n;
    return 0;
}
