#include <dynstr.h>
#include <libweb/handler.h>
#include <libweb/html.h>
#include <libweb/http.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

static const size_t max_headers = 5;

static int hello(const struct http_payload *const pl,
    struct http_response *const r, void *const user)
{
    printf("Got %zu headers from the client (max %zu).\n",
        pl->n_headers, max_headers);

    for (size_t i = 0; i < pl->n_headers; i++)
    {
        const struct http_header *const h = &pl->headers[i];

        printf("%s: %s\n", h->header, h->value);
    }

    *r = (const struct http_response)
    {
        .status = HTTP_STATUS_OK
    };

    return 0;
}

static int on_length(const unsigned long long len,
    const struct http_cookie *const c, struct http_response *const r,
    void *const user)
{
    *r = (const struct http_response)
    {
        .status = HTTP_STATUS_FORBIDDEN
    };

    return 1;
}

int main(int argc, char *argv[])
{
    int ret = EXIT_FAILURE;
    const struct handler_cfg cfg =
    {
        .length = on_length,
        .max_headers = max_headers
    };

    struct handler *const h = handler_alloc(&cfg);
    static const char *const urls[] = {"/", "/index.html"};

    if (!h)
    {
        fprintf(stderr, "%s: handler_alloc failed\n", __func__);
        goto end;
    }

    for (size_t i = 0; i < sizeof urls / sizeof *urls; i++)
        if (handler_add(h, urls[i], HTTP_OP_GET, hello, NULL))
        {
            fprintf(stderr, "%s: handler_add failed\n", __func__);
            goto end;
        }

    unsigned short port;

    if (handler_listen(h, 0, &port))
    {
        fprintf(stderr, "%s: handler_listen failed\n", __func__);
        goto end;
    }

    printf("Listening on port %hu\n", port);

    if (handler_loop(h))
    {
        fprintf(stderr, "%s: handler_loop failed\n", __func__);
        goto end;
    }

    ret = EXIT_SUCCESS;

end:
    handler_free(h);
    return ret;
}
