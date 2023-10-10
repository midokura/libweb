#include <libweb/html.h>
#include <dynstr.h>
#include <stdlib.h>
#include <stdio.h>

int main()
{
    int ret = EXIT_FAILURE;
    struct dynstr d;
    struct html_node *const html = html_node_alloc("html"), *head,
        *meta, *body;
    static const char text[] = "testing libweb";

    dynstr_init(&d);

    if (!html)
    {
        fprintf(stderr, "html_node_alloc_failed\n");
        goto end;
    }
    else if (!(head = html_node_add_child(html, "head")))
    {
        fprintf(stderr, "html_node_add_child head failed\n");
        goto end;
    }
    else if (!(meta = html_node_add_child(head, "meta")))
    {
        fprintf(stderr, "html_node_add_child meta failed\n");
        goto end;
    }
    else if (html_node_add_attr(meta, "charset", "UTF-8"))
    {
        fprintf(stderr, "%s: html_node_add_attr charset failed\n", __func__);
        goto end;
    }
    else if (!(body = html_node_add_child(html, "body")))
    {
        fprintf(stderr, "html_node_add_child failed\n");
        goto end;
    }
    else if (html_node_set_value(body, text))
    {
        fprintf(stderr, "html_node_set_value failed\n");
        goto end;
    }
    else if (html_serialize(html, &d))
    {
        fprintf(stderr, "html_serialize failed\n");
        goto end;
    }

    printf("%s", d.str);
    ret = EXIT_SUCCESS;

end:
    dynstr_free(&d);
    html_node_free(html);
    return ret;
}
