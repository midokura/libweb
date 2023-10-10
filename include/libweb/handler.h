#ifndef HANDLER_H
#define HANDLER_H

#include "libweb/http.h"
#include <stddef.h>

typedef int (*handler_fn)(const struct http_payload *p,
    struct http_response *r, void *user);

struct handler_cfg
{
    const char *tmpdir;
    int (*length)(unsigned long long len, const struct http_cookie *c,
        struct http_response *r, void *user);
    void *user;
};

struct handler *handler_alloc(const struct handler_cfg *cfg);
void handler_free(struct handler *h);
int handler_add(struct handler *h, const char *url, enum http_op op,
    handler_fn f, void *user);
int handler_listen(struct handler *h, unsigned short port);

#endif /* HANDLER_H */
