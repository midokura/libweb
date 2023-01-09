#include "server.h"
#include <fcntl.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
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
        struct server_client *ref = &s->c[i];

        if (c == ref)
        {
            if ((ret = close(c->fd)))
            {
                fprintf(stderr, "%s: close(2): %s\n",
                    __func__, strerror(errno));
                return -1;
            }
            else if (s->n - 1)
            {
                memcpy(ref, ref + 1, s->n - i);

                if (!(s->c = realloc(s->c, (s->n - 1) * sizeof *s->c)))
                {
                    fprintf(stderr, "%s: realloc(3): %s\n",
                        __func__, strerror(errno));
                    return -1;
                }
            }
            else
            {
                free(s->c);
                s->c = NULL;
            }

            s->n--;
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
    else if (!(s->c = realloc(s->c, (s->n + 1) * sizeof *s->c)))
    {
        fprintf(stderr, "%s: realloc(3): %s\n",
            __func__, strerror(errno));
        return NULL;
    }

    s->c[s->n] = (const struct server_client)
    {
        .fd = fd
    };

    return &s->c[s->n++];
}

void server_client_write_pending(struct server_client *const c,
    const bool write)
{
    c->write = write;
}

static volatile sig_atomic_t do_exit;

static void handle_signal(const int signum)
{
    do_exit = 1;
}

struct server_client *server_select(struct server *const s, bool *const io,
    bool *const exit)
{
    int nfds = -1;
    fd_set rfds, wfds;

    *io = *exit = false;

    FD_ZERO(&rfds);
    FD_ZERO(&wfds);

    for (size_t i = 0; i < s->n; i++)
    {
        const struct server_client *const c = &s->c[i];
        const int fd = c->fd;

        FD_SET(fd, &rfds);

        if (c->write)
            FD_SET(fd, &wfds);

        if (fd > nfds)
            nfds = fd;
    }

    FD_SET(s->fd, &rfds);

    if (s->fd > nfds)
        nfds = s->fd;

    const int res = select(nfds + 1, &rfds, &wfds, NULL, NULL);

    if (res < 0)
    {
        if (do_exit)
            *exit = true;
        else
            fprintf(stderr, "%s: select(2): %s\n", __func__, strerror(errno));

        return NULL;
    }
    else if (FD_ISSET(s->fd, &rfds))
        return alloc_client(s);

    for (size_t i = 0; i < s->n; i++)
    {
        struct server_client *const c = &s->c[i];

        if (FD_ISSET(c->fd, &rfds) || FD_ISSET(c->fd, &wfds))
        {
            *io = true;
            return c;
        }
    }

    fprintf(stderr, "%s: unlisted fd\n", __func__);
    return NULL;
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
        fprintf(stderr, "%s: sigaction(2) SIGINT: %s\n",
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
