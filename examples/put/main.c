#include <dynstr.h>
#include <libweb/handler.h>
#include <libweb/html.h>
#include <libweb/http.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

static int on_put(const struct http_payload *const pl,
    struct http_response *const r, void *const user)
{
    if (pl->expect_continue)
    {
        *r = (const struct http_response)
        {
            .status = HTTP_STATUS_CONTINUE
        };

        return 0;
    }

    printf("File uploaded to %s\n", pl->u.put.tmpname);

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
        .status = HTTP_STATUS_OK
    };

    return 0;
}

int main(int argc, char *argv[])
{
    int ret = EXIT_FAILURE;
    const struct handler_cfg cfg =
    {
        .tmpdir = "/tmp",
        .length = on_length
    };

    struct handler *const h = handler_alloc(&cfg);
    static const char *const urls[] = {"/*"};

    if (!h)
    {
        fprintf(stderr, "%s: handler_alloc failed\n", __func__);
        goto end;
    }

    for (size_t i = 0; i < sizeof urls / sizeof *urls; i++)
        if (handler_add(h, urls[i], HTTP_OP_PUT, on_put, NULL))
        {
            fprintf(stderr, "%s: handler_add failed\n", __func__);
            goto end;
        }

    unsigned short outport;

    if (handler_listen(h, 0, &outport))
    {
        fprintf(stderr, "%s: handler_listen failed\n", __func__);
        goto end;
    }

    printf("Listening on port %hu\n", outport);

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
