#ifndef HTTP_H
#define HTTP_H

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

struct http_header
{
    char *header, *value;
};

struct http_payload
{
    enum http_op
    {
        HTTP_OP_GET,
        HTTP_OP_POST,
        HTTP_OP_HEAD,
        HTTP_OP_PUT
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
            const char *data;
            size_t nfiles, npairs;

            const struct http_post_pair
            {
                const char *name, *value;
            } *pairs;

            const struct http_post_file
            {
                const char *name, *tmpname, *filename;
            } *files;
        } post;

        struct http_put
        {
            const char *tmpname;
        } put;
    } u;

    const struct http_arg
    {
        char *key, *value;
    } *args;

    size_t n_args, n_headers;
    const struct http_header *headers;
    bool expect_continue;
};

#define HTTP_STATUSES \
    X(CONTINUE, "Continue", 100) \
    X(OK, "OK", 200) \
    X(SEE_OTHER, "See other", 303) \
    X(BAD_REQUEST, "Bad Request", 400) \
    X(UNAUTHORIZED, "Unauthorized", 401) \
    X(FORBIDDEN, "Forbidden", 403) \
    X(NOT_FOUND, "Not found", 404) \
    X(PAYLOAD_TOO_LARGE, "Payload too large", 413) \
    X(INTERNAL_ERROR, "Internal Server Error", 500)

struct http_response
{
    enum http_status
    {
#define X(x, y, z) HTTP_STATUS_##x,
        HTTP_STATUSES
#undef X
    } status;

    struct http_header *headers;

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
        struct http_response *r, void *user);
    const char *tmpdir;
    void *user;
    size_t max_headers;
};

struct http_ctx *http_alloc(const struct http_cfg *cfg);
void http_free(struct http_ctx *h);
int http_update(struct http_ctx *h, bool *write, bool *close);
int http_response_add_header(struct http_response *r, const char *header,
    const char *value);
char *http_cookie_create(const char *key, const char *value);
char *http_encode_url(const char *url);
int http_decode_url(const char *url, bool spaces, char **out);

#endif /* HTTP_H */
