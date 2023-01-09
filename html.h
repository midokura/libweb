#ifndef HTML_H
#define HTML_H

#include <dynstr.h>

struct html_node *html_node_alloc(const char *element);
void html_node_free(struct html_node *n);
int html_node_set_value(struct html_node *n, const char *val);
int html_node_set_value_unescaped(struct html_node *n, const char *val);
int html_node_add_attr(struct html_node *n, const char *attr, const char *val);
struct html_node *html_node_add_child(struct html_node *n, const char *elem);
void html_node_add_sibling(struct html_node *n, struct html_node *sibling);
int html_serialize(const struct html_node *n, struct dynstr *d);

#endif /* HTML_H */
