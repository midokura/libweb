#include <dynstr.h>
#include <slweb/handler.h>
#include <slweb/html.h>
#include <slweb/http.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

static int hello(const struct http_payload *const pl,
    struct http_response *const r, void *const user)
{
    int ret = -1;
    struct html_node *html = html_node_alloc("html"), *body, *p;
    struct dynstr d;

    dynstr_init(&d);

    if (!html)
    {
        fprintf(stderr, "%s: html_node_alloc failed\n", __func__);
        goto end;
    }
    else if (!(body = html_node_add_child(html, "body")))
    {
        fprintf(stderr, "%s: html_node_add_child body failed\n", __func__);
        goto end;
    }
    else if (!(p = html_node_add_child(html, "p")))
    {
        fprintf(stderr, "%s: html_node_add_child p failed\n", __func__);
        goto end;
    }
    else if (html_node_set_value(p, "Hello from slweb!"))
    {
        fprintf(stderr, "%s: html_node_set_value p failed\n", __func__);
        goto end;
    }
    else if (html_serialize(html, &d))
    {
        fprintf(stderr, "%s: html_serialize failed\n", __func__);
        goto end;
    }

    *r = (const struct http_response)
    {
        .status = HTTP_STATUS_OK,
        .buf.rw = d.str,
        .n = d.len,
        .free = free
    };

    if (http_response_add_header(r, "Content-Type", "text/html"))
    {
        fprintf(stderr, "%s: http_response_add_header failed\n", __func__);
        goto end;
    }

    ret = 0;

end:
    html_node_free(html);

    if (ret)
        dynstr_free(&d);

    return ret;
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
    const short port = 8080;
    const struct handler_cfg cfg =
    {
        .length = on_length
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

    if (handler_listen(h, port))
    {
        fprintf(stderr, "%s: handler_listen failed\n", __func__);
        goto end;
    }

    ret = EXIT_SUCCESS;

end:
    handler_free(h);
    return ret;
}
