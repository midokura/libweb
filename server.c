/* As of FreeBSD 13.2, sigaction(2) still conforms to IEEE Std
 * 1003.1-1990 (POSIX.1), which did not define SA_RESTART.
 * FreeBSD supports it as an extension, but then _POSIX_C_SOURCE must
 * not be defined. */
#ifndef __FreeBSD__
#define _POSIX_C_SOURCE 200809L
#endif

#include "libweb/server.h"
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
        struct server_client *prev, *next;
    } *c;
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

    for (struct server_client *ref = s->c; ref; ref = ref->next)
    {
        if (c == ref)
        {
            struct server_client *const next = ref->next;

            if ((ret = close(c->fd)))
            {
                fprintf(stderr, "%s: close(2): %s\n",
                    __func__, strerror(errno));
            }
            else if (ref->prev)
                ref->prev->next = next;
            else
                s->c = next;

            if (next)
                next->prev = ref->prev;

            free(ref);
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

    struct server_client *const c = malloc(sizeof *c);

    if (!c)
    {
        fprintf(stderr, "%s: malloc(3): %s\n", __func__, strerror(errno));
        return NULL;
    }

    *c = (const struct server_client)
    {
        .fd = fd
    };

    if (!s->c)
        s->c = c;
    else
        for (struct server_client *ref = s->c; ref; ref = ref->next)
            if (!ref->next)
            {
                ref->next = c;
                c->prev = ref;
                break;
            }

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

static size_t get_clients(const struct server *const s)
{
    size_t ret = 0;

    for (const struct server_client *c = s->c; c; c = c->next)
        ret++;

    return ret;
}

struct server_client *server_poll(struct server *const s, bool *const io,
    bool *const exit)
{
    struct server_client *ret = NULL;
    const size_t n_clients = get_clients(s);
    const nfds_t n = n_clients + 1;
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

    for (struct {const struct server_client *c; size_t j;}
        _ = {.c = s->c, .j = 1}; _.c; _.c = _.c->next, _.j++)
    {
        struct pollfd *const p = &fds[_.j];
        const int fd = _.c->fd;

        *p = (const struct pollfd)
        {
            .fd = fd,
            .events = POLLIN
        };

        if (_.c->write)
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
    else if (sfd->revents)
    {
        ret = alloc_client(s);
        goto end;
    }

    for (struct {struct server_client *c; size_t j;}
        _ = {.c = s->c, .j = 1}; _.c; _.c = _.c->next, _.j++)
    {
        const struct pollfd *const p = &fds[_.j];

        if (p->revents)
        {
            *io = true;
            ret = _.c;
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

    static const struct signal
    {
        int signal;
        const char *name;
    } signals[] =
    {
        {.signal = SIGINT, .name = "SIGINT"},
        {.signal = SIGTERM, .name = "SIGTERM"},
        {.signal = SIGPIPE, .name = "SIGPIPE"}
    };

    for (size_t i = 0; i < sizeof signals / sizeof *signals; i++)
    {
        const struct signal *const s = &signals [i];

        if (sigaction(s->signal, &sa, NULL))
        {
            fprintf(stderr, "%s: sigaction(2) %s: %s\n",
                __func__, s->name, strerror(errno));
            return -1;
        }
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
