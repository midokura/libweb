#define _POSIX_C_SOURCE 200809L

#include "slweb/html.h"
#include <dynstr.h>
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct html_node
{
    struct html_attribute
    {
        char *attr, *value;
    } *attrs;

    char *element, *value;
    size_t n;
    struct html_node *child, *sibling;
};

static char *html_encode(const char *s)
{
    struct dynstr d;

    dynstr_init(&d);

    if (!*s && dynstr_append(&d, ""))
    {
        fprintf(stderr, "%s: dynstr_append empty failed\n", __func__);
        goto failure;
    }

    while (*s)
    {
        static const struct esc
        {
            char c;
            const char *str;
        } esc[] =
        {
            {.c = '<', .str = "&gt;"},
            {.c = '>', .str = "&lt;"},
            {.c = '&', .str = "&amp;"},
            {.c = '\"', .str = "&quot;"},
            {.c = '\'', .str = "&apos;"}
        };

        char buf[sizeof "a"] = {0};
        const char *str = NULL;

        for (size_t i = 0; i < sizeof esc / sizeof *esc; i++)
        {
            const struct esc *const e = &esc[i];

            if (*s == e->c)
            {
                str = e->str;
                break;
            }
        }

        if (!str)
        {
            *buf = *s;
            str = buf;
        }

        if (dynstr_append(&d, "%s", str))
        {
            fprintf(stderr, "%s: dynstr_append failed\n", __func__);
            goto failure;
        }

        s++;
    }

    return d.str;

failure:
    dynstr_free(&d);
    return NULL;
}

int html_node_set_value(struct html_node *const n, const char *const val)
{
    if (!(n->value = html_encode(val)))
    {
        fprintf(stderr, "%s: html_encode failed\n", __func__);
        return -1;
    }

    return 0;
}

int html_node_set_value_unescaped(struct html_node *const n,
    const char *const val)
{
    if (!(n->value = strdup(val)))
    {
        fprintf(stderr, "%s: strdup(3): %s\n", __func__, strerror(errno));
        return -1;
    }

    return 0;
}

int html_node_add_attr(struct html_node *const n, const char *const attr,
    const char *const val)
{
    const size_t el = n->n + 1;
    struct html_attribute *const attrs = realloc(n->attrs,
        el * sizeof *n->attrs), *a = NULL;

    if (!attrs)
    {
        fprintf(stderr, "%s: realloc(3): %s\n", __func__, strerror(errno));
        return -1;
    }

    a = &attrs[n->n];
    *a = (const struct html_attribute){0};

    if (!(a->attr = strdup(attr))
        || (val && !(a->value = strdup(val))))
    {
        fprintf(stderr, "%s: strdup(3): %s\n", __func__, strerror(errno));
        free(a->attr);
        free(a->value);
        return -1;
    }

    n->attrs = attrs;
    n->n = el;
    return 0;
}

void html_node_add_sibling(struct html_node *const n,
    struct html_node *const sibling)
{
    for (struct html_node *c = n; c; c = c->sibling)
        if (!c->sibling)
        {
            c->sibling = sibling;
            break;
        }
}

struct html_node *html_node_add_child(struct html_node *const n,
    const char *const element)
{
    struct html_node *const child = html_node_alloc(element);

    if (!child)
        return NULL;
    else if (n->child)
        html_node_add_sibling(n->child, child);
    else
        n->child = child;

    return child;
}

int serialize_node(struct dynstr *const d, const struct html_node *const n,
    const unsigned level)
{
    for (unsigned i = 0; i < level; i++)
        dynstr_append(d, "\t");

    dynstr_append_or_ret_nonzero(d, "<%s", n->element);

    if (n->n)
        dynstr_append_or_ret_nonzero(d, " ");

    for (size_t i = 0; i < n->n; i++)
    {
        const struct html_attribute *const a = &n->attrs[i];

        if (a->value)
            dynstr_append_or_ret_nonzero(d, "%s=\"%s\"", a->attr, a->value);
        else
            dynstr_append_or_ret_nonzero(d, "%s", a->attr);

        if (i + 1 < n->n)
            dynstr_append_or_ret_nonzero(d, " ");
    }

    if (!n->value && !n->child)
        dynstr_append_or_ret_nonzero(d, "/>");
    else
    {
        dynstr_append_or_ret_nonzero(d, ">");

        if (n->value)
            dynstr_append_or_ret_nonzero(d, "%s", n->value);

        if (n->child)
        {
            dynstr_append_or_ret_nonzero(d, "\n");

            if (serialize_node(d, n->child, level + 1))
            {
                fprintf(stderr, "%s: serialize_node failed\n", __func__);
                return -1;
            }

            for (unsigned i = 0; i < level; i++)
                dynstr_append(d, "\t");
        }

        dynstr_append_or_ret_nonzero(d, "</%s>", n->element);
    }

    /* TODO: print siblings */

    dynstr_append_or_ret_nonzero(d, "\n");

    if (n->sibling)
        return serialize_node(d, n->sibling, level);

    return 0;
}

int html_serialize(const struct html_node *const n, struct dynstr *const d)
{
    return serialize_node(d, n, 0);
}

static void html_attribute_free(struct html_attribute *const a)
{
    if (a)
    {
        free(a->attr);
        free(a->value);
    }
}

void html_node_free(struct html_node *const n)
{
    if (n)
    {
        struct html_node *s = n->sibling;

        html_node_free(n->child);

        while (s)
        {
            struct html_node *const next = s->sibling;

            html_node_free(s->child);
            free(s->element);
            free(s->value);

            for (size_t i = 0 ; i < s->n; i++)
                html_attribute_free(&s->attrs[i]);

            free(s->attrs);
            free(s);
            s = next;
        }

        free(n->element);
        free(n->value);

        for (size_t i = 0 ; i < n->n; i++)
            html_attribute_free(&n->attrs[i]);

        free(n->attrs);
    }

    free(n);
}

struct html_node *html_node_alloc(const char *const element)
{
    struct html_node *const n = malloc(sizeof *n);

    if (!n)
    {
        fprintf(stderr, "%s: malloc(3): %s\n", __func__, strerror(errno));
        goto failure;
    }

    *n = (const struct html_node)
    {
        .element = strdup(element)
    };

    if (!n->element)
    {
        fprintf(stderr, "%s: malloc(3): %s\n", __func__, strerror(errno));
        goto failure;
    }

    return n;

failure:
    html_node_free(n);
    return NULL;
}
