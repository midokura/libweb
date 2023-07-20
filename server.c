#define _POSIX_C_SOURCE 200809L

#include "server.h"
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <poll.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct server
{
    int fd;

    struct server_client
    {
        int fd;
        bool write;
    } *c;

    size_t n;
};

int server_close(struct server *const s)
{
    int ret = 0;

    if (!s)
        return 0;
    else if (s->fd >= 0)
        ret = close(s->fd);

    free(s);
    return ret;
}

int server_client_close(struct server *const s, struct server_client *const c)
{
    int ret = 0;

    for (size_t i = 0; i < s->n; i++)
    {
        struct server_client *const ref = &s->c[i];

        if (c == ref)
        {
            const size_t n = s->n - 1;

            if ((ret = close(c->fd)))
            {
                fprintf(stderr, "%s: close(2): %s\n",
                    __func__, strerror(errno));
                return -1;
            }
            else if (n)
            {
                memcpy(ref, ref + 1, (s->n - i - 1) * sizeof *ref);

                struct server_client *const c = realloc(s->c, n * sizeof *s->c);

                if (!c)
                {
                    fprintf(stderr, "%s: realloc(3): %s\n",
                        __func__, strerror(errno));
                    return -1;
                }

                s->c = c;
            }
            else
            {
                free(s->c);
                s->c = NULL;
            }

            s->n = n;
            break;
        }
    }

    return ret;
}

int server_read(void *const buf, const size_t n, struct server_client *const c)
{
    const ssize_t r = read(c->fd, buf, n);

    if (r < 0)
        fprintf(stderr, "%s: read(2): %s\n", __func__, strerror(errno));

    return r;
}

int server_write(const void *const buf, const size_t n,
    struct server_client *const c)
{
    const ssize_t w = write(c->fd, buf, n);

    if (w < 0)
        fprintf(stderr, "%s: write(2): %s\n", __func__, strerror(errno));

    return w;
}

static struct server_client *alloc_client(struct server *const s)
{
    struct sockaddr_in addr;
    socklen_t sz = sizeof addr;
    const int fd = accept(s->fd, (struct sockaddr *)&addr, &sz);

    if (fd < 0)
    {
        fprintf(stderr, "%s: accept(2): %s\n",
            __func__, strerror(errno));
        return NULL;
    }

    const int flags = fcntl(fd, F_GETFL);

    if (flags < 0)
    {
        fprintf(stderr, "%s: fcntl(2) F_GETFL: %s\n",
            __func__, strerror(errno));
        return NULL;
    }
    else if (fcntl(fd, F_SETFL, flags | O_NONBLOCK))
    {
        fprintf(stderr, "%s: fcntl(2) F_SETFL: %s\n",
            __func__, strerror(errno));
        return NULL;
    }

    const size_t n = s->n + 1;
    struct server_client *const clients = realloc(s->c, n * sizeof *s->c);

    if (!clients)
    {
        fprintf(stderr, "%s: realloc(3): %s\n", __func__, strerror(errno));
        return NULL;
    }

    struct server_client *const c = &clients[s->n];

    *c = (const struct server_client)
    {
        .fd = fd
    };

    s->c = clients;
    s->n = n;
    return c;
}

void server_client_write_pending(struct server_client *const c,
    const bool write)
{
    c->write = write;
}

static volatile sig_atomic_t do_exit;

static void handle_signal(const int signum)
{
    switch (signum)
    {
        case SIGINT:
            /* Fall through. */
        case SIGTERM:
            do_exit = 1;
            break;

        default:
            break;
    }
}

struct server_client *server_poll(struct server *const s, bool *const io,
    bool *const exit)
{
    struct server_client *ret = NULL;
    const nfds_t n = s->n + 1;
    struct pollfd *const fds = malloc(n * sizeof *fds);

    if (!fds)
    {
        fprintf(stderr, "%s: malloc(3): %s\n", __func__, strerror(errno));
        goto end;
    }

    struct pollfd *const sfd = &fds[0];

    *io = *exit = false;
    *sfd = (const struct pollfd)
    {
        .fd = s->fd,
        .events = POLLIN
    };

    for (size_t i = 0, j = 1; i < s->n; i++, j++)
    {
        struct pollfd *const p = &fds[j];
        const struct server_client *const c = &s->c[i];
        const int fd = c->fd;

        *p = (const struct pollfd)
        {
            .fd = fd,
            .events = POLLIN
        };

        if (c->write)
            p->events |= POLLOUT;
    }

    int res;

again:

    res = poll(fds, n, -1);

    if (res < 0)
    {
        if (do_exit)
        {
            *exit = true;
            goto end;
        }

        switch (errno)
        {
            case EAGAIN:
                /* Fall through. */
            case EINTR:
                goto again;

            default:
                fprintf(stderr, "%s: poll(2): %s\n", __func__, strerror(errno));
                break;
        }

        goto end;
    }
    else if (!res)
    {
        fprintf(stderr, "%s: poll(2) returned zero\n", __func__);
        goto end;
    }

    if (sfd->revents)
    {
        ret = alloc_client(s);
        goto end;
    }

    for (size_t i = 0, j = 1; i < s->n; i++, j++)
    {
        const struct pollfd *const p = &fds[j];
        struct server_client *const c = &s->c[i];

        if (p->revents)
        {
            *io = true;
            ret = c;
            goto end;
        }
    }

    fprintf(stderr, "%s: unlisted fd\n", __func__);

end:
    free(fds);
    return ret;
}

static int init_signals(void)
{
    struct sigaction sa =
    {
        .sa_handler = handle_signal,
        .sa_flags = SA_RESTART
    };

    sigemptyset(&sa.sa_mask);

    if (sigaction(SIGINT, &sa, NULL))
    {
        fprintf(stderr, "%s: sigaction(2) SIGINT: %s\n",
            __func__, strerror(errno));
        return -1;
    }
    else if (sigaction(SIGTERM, &sa, NULL))
    {
        fprintf(stderr, "%s: sigaction(2) SIGTERM: %s\n",
            __func__, strerror(errno));
        return -1;
    }
    else if (sigaction(SIGPIPE, &sa, NULL))
    {
        fprintf(stderr, "%s: sigaction(2) SIGPIPE: %s\n",
            __func__, strerror(errno));
        return -1;
    }

    return 0;
}

struct server *server_init(const unsigned short port)
{
    struct server *const s = malloc(sizeof *s);

    if (!s)
    {
        fprintf(stderr, "%s: malloc(3): %s\n", __func__, strerror(errno));
        goto failure;
    }

    *s = (const struct server)
    {
        .fd = socket(AF_INET, SOCK_STREAM, 0)
    };

    if (s->fd < 0)
    {
        fprintf(stderr, "%s: socket(2): %s\n", __func__, strerror(errno));
        goto failure;
    }
    else if (init_signals())
    {
        fprintf(stderr, "%s: init_signals failed\n", __func__);
        goto failure;
    }

    const struct sockaddr_in addr =
    {
        .sin_family = AF_INET,
        .sin_port = htons(port)
    };

    enum {QUEUE_LEN = 10};

    if (bind(s->fd, (const struct sockaddr *)&addr, sizeof addr))
    {
        fprintf(stderr, "%s: bind(2): %s\n", __func__, strerror(errno));
        goto failure;
    }
    else if (listen(s->fd, QUEUE_LEN))
    {
        fprintf(stderr, "%s: listen(2): %s\n", __func__, strerror(errno));
        goto failure;
    }

    struct sockaddr_in in;
    socklen_t sz = sizeof in;

    if (getsockname(s->fd, (struct sockaddr *)&in, &sz))
    {
        fprintf(stderr, "%s: getsockname(2): %s\n", __func__, strerror(errno));
        goto failure;
    }

    printf("Listening on port %hu\n", ntohs(in.sin_port));
    return s;

failure:
    server_close(s);
    return NULL;
}
