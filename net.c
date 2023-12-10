#include "net.h"

static int try_addr(struct addrinfo *p, int qsize);

int
tcp_listen(char *host, char *service, int qsize)
{
        struct addrinfo hints;
        struct addrinfo *infolist;
        struct addrinfo *p;
        int listenfd;
        int error;

        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        error = getaddrinfo(host, service, &hints, &infolist);
        if (error)
                errx(EX_SOFTWARE,
                    "tcp_listen(): getaddrinfo(): %s", gai_strerror(error));

        for (p = infolist; p; p = p->ai_next) {
                listenfd = try_addr(p, qsize);
                if (listenfd >= 0)
                        break;
        }

        if (!p)
                errx(EX_SOFTWARE, "could not bind to %s:%s", host, service);

        freeaddrinfo(infolist);
        return listenfd;
}

static int
try_addr(struct addrinfo *p, int qsize)
{
        int on;
        int fd;

        fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (fd < 0)
                goto ret;

        on = 1;
        if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)))
                goto close_fd;

        if (bind(fd, p->ai_addr, p->ai_addrlen))
                goto close_fd;

        if (listen(fd, qsize))
                goto close_fd;

        goto ret;
close_fd:
        if (close(fd))
                err(EX_OSERR, "try_add(): close()");
ret:
        return fd;
}
