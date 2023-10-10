#ifndef SERVER_H
#define SERVER_H

#include <stdbool.h>
#include <stddef.h>

struct server *server_init(unsigned short port);
struct server_client *server_poll(struct server *s, bool *io, bool *exit);
int server_read(void *buf, size_t n, struct server_client *c);
int server_write(const void *buf, size_t n, struct server_client *c);
int server_close(struct server *s);
int server_client_close(struct server *s, struct server_client *c);
void server_client_write_pending(struct server_client *c, bool write);

#endif /* SERVER_H */
