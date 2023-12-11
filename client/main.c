#include <err.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <sys/select.h>
#include <sysexits.h>
#include <unistd.h>

int
main(void)
{
        struct addrinfo hints;
        struct addrinfo *infolist;
        struct addrinfo *p;
        char *host = "localhost";
        char *service = "8080";
        char buf[BUFSIZ + 1];
        fd_set set;
        ssize_t n;
        int maxfd;
        int connfd;
        int error;
        int stdineof;

        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        error = getaddrinfo("localhost", "8080", &hints, &infolist);
        if (error)
                errx(EX_SOFTWARE, "getaddrinfo: %s", gai_strerror(error));

        for (p = infolist; p; p = p->ai_next) {
                connfd = socket(p->ai_family,
                                p->ai_socktype,
                                p->ai_protocol);
                if (connfd < 0)
                        continue;

                if (connect(connfd, p->ai_addr, p->ai_addrlen) == 0)
                        break;

                if (close(connfd) < 0)
                        err(EX_OSERR, "close()");
        }

        if (!p)
                errx(EX_USAGE, "could not connect to %s:%s", host, service);

        freeaddrinfo(infolist);

        printf("[cmd]> ");
        if (!fgets(buf, BUFSIZ, stdin))
                err(EX_SOFTWARE, "no command given");

        n = write(connfd, buf, strlen(buf));
        if (n != (ssize_t)strlen(buf))
                err(EX_OSERR, "could not write command");

        stdineof = 0;
        for (;;) {
                FD_ZERO(&set);
                if (!stdineof)
                        FD_SET(0, &set);
                FD_SET(connfd, &set);
                maxfd = connfd;
                if (select(maxfd + 1, &set, NULL, NULL, NULL) < 0)
                        err(EX_OSERR, "select()");

                if (FD_ISSET(0, &set)) {
                        n = read(0, buf, BUFSIZ);
                        if (n < 0)
                                err(EX_OSERR, "read from stdin");
                        if (n == 0) {
                                stdineof = 1;
                                if (shutdown(connfd, SHUT_WR) < 0)
                                        err(EX_OSERR, "shutdown()");
                        } else {
                                if (write(connfd, buf, n) != n)
                                        err(EX_OSERR, "write()");
                        }
                }

                if (FD_ISSET(connfd, &set)) {
                        n = read(connfd, buf, BUFSIZ);
                        if (n < 0)
                                err(EX_OSERR, "read from socket");
                        if (n == 0) {
                                printf("server terminated prematurely\n");
                                break;
                        }
                        buf[n] = 0;
                        if (fputs(buf, stdout) == EOF)
                                err(EX_OSERR, "fputs()");
                }
        }
}
