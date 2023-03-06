#ifndef HTTP_H
#define HTTP_H

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

struct http_payload
{
    enum http_op
    {
        HTTP_OP_GET,
        HTTP_OP_POST
    } op;

    const char *resource;

    struct http_cookie
    {
        const char *field, *value;
    } cookie;

    union
    {
        struct http_post
        {
            bool expect_continue;
            const void *data;
            size_t n;
            const char *dir;

            const struct http_post_file
            {
                const char *tmpname, *filename;
            } *files;
        } post;
    } u;
};

#define HTTP_STATUSES \
    X(CONTINUE, "Continue", 100) \
    X(OK, "OK", 200) \
    X(SEE_OTHER, "See other", 303) \
    X(BAD_REQUEST, "Bad Request", 400) \
    X(UNAUTHORIZED, "Unauthorized", 401) \
    X(FORBIDDEN, "Forbidden", 403) \
    X(NOT_FOUND, "Not found", 404) \
    X(INTERNAL_ERROR, "Internal Server Error", 500)

struct http_response
{
    enum http_status
    {
#define X(x, y, z) HTTP_STATUS_##x,
        HTTP_STATUSES
#undef X
    } status;

    struct http_header
    {
        char *header, *value;
    } *headers;

    union
    {
        const void *ro;
        void *rw;
    } buf;

    FILE *f;
    unsigned long long n;
    size_t n_headers;
    void (*free)(void *);
};

struct http_cfg
{
    int (*read)(void *buf , size_t n, void *user);
    int (*write)(const void *buf, size_t n, void *user);
    int (*payload)(const struct http_payload *p, struct http_response *r,
        void *user);
    int (*length)(unsigned long long len, const struct http_cookie *c,
        void *user);
    const char *tmpdir;
    void *user;
};

struct http_ctx *http_alloc(const struct http_cfg *cfg);
void http_free(struct http_ctx *h);
/* Positive return value: user input error, negative: fatal error. */
int http_update(struct http_ctx *h, bool *write, bool *close);
int http_response_add_header(struct http_response *r, const char *header,
    const char *value);
char *http_cookie_create(const char *key, const char *value);
char *http_encode_url(const char *url);
char *http_decode_url(const char *url);

#endif /* HTTP_H */
