#ifndef HANDLER_H
#define HANDLER_H

#include "http.h"
#include <stddef.h>

struct handler;
typedef int (*handler_fn)(const struct http_payload *p,
    struct http_response *r, void *user);

struct handler *handler_alloc(const char *tmpdir);
void handler_free(struct handler *h);
int handler_add(struct handler *h, const char *url, enum http_op op,
    handler_fn f, void *user);
int handler_listen(struct handler *h, short port);

#endif /* HANDLER_H */
