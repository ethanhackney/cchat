#include "../chat.h"
#include "../net.h"
#include "../workq.h"
#include <err.h>
#include <stdio.h>
#include <string.h>
#include <sys/select.h>
#include <sysexits.h>
#include <unistd.h>

static void cmd_interp(struct work *w);
static void *chatfn(void *arg);

int
main(void)
{
        struct workq *q;
        int listenfd;
        int connfd;

        listenfd = tcp_listen("localhost", "8080", 10);

        q = workq_new(10, cmd_interp);

        for (;;) {
                connfd = accept(listenfd, NULL, NULL);
                if (connfd < 0)
                        err(EX_OSERR, "main(): accept()");
                workq_add(q, connfd);
        }

        workq_free(&q);
}

static void fd_puts(int fd, char *s);
static void do_create(int connfd, char *cname, char *uname);
static void do_join(int connfd, char *cname, char *uname);

static void
cmd_interp(struct work *w)
{
        char cmdline[BUFSIZ];
        char *cmdp;
        char *cmd;
        char *cname;
        char *uname;
        ssize_t n;
        int connfd;

        connfd = w->w_connfd;
        n = read(connfd, cmdline, sizeof(cmdline));
        if (n <= 0) {
                fd_puts(connfd, "no command line given");
                return;
        }

        cmdp = cmdline;
        cmd = strsep(&cmdp, ":");
        cname = strsep(&cmdp, ":");
        uname = strsep(&cmdp, "\n");

        printf("cmd=%s, cname=%s, uname=%s\n", cmd, cname, uname);

        if (!strcmp(cmd, "create"))
                do_create(connfd, cname, uname);
        if (!strcmp(cmd, "join"))
                do_join(connfd, cname, uname);
}

static void
fd_puts(int fd, char *s)
{
        ssize_t n;

        n = write(fd, s, strlen(s));
        if (n != (ssize_t)strlen(s))
                err(EX_OSERR, "fd_puts(): write()");
}

static void
do_create(int connfd, char *cname, char *uname)
{
        struct chat *c;
        pthread_t tid;

        c = chat_find(cname);
        if (c) {
                fd_puts(connfd, "chatroom already exists");
                return;
        }

        c = chat_new(cname);
        chat_add_user(c, uname, connfd);

        errno = pthread_create(&tid, NULL, chatfn, c);
        if (errno)
                err(EX_SOFTWARE, "do_create(): pthread_create()");
}

static void *
chatfn(void *arg)
{
        struct chat *c;
        struct user *u;
        struct user *v;
        struct user *next;
        char msg[BUFSIZ + 1];
        ssize_t n;
        fd_set set;
        int maxfd;

        c = arg;
        for (;;) {
                errno = pthread_mutex_lock(&c->c_lock);
                if (errno)
                        err(EX_SOFTWARE, "chatfn(): pthread_mutex_lock()");

                FD_ZERO(&set);
                maxfd = -1;
                for (u = c->c_users; u; u = u->u_next) {
                        if (u->u_connfd > maxfd)
                                maxfd = u->u_connfd;
                        FD_SET(u->u_connfd, &set);
                }

                errno = pthread_mutex_unlock(&c->c_lock);
                if (errno)
                        err(EX_SOFTWARE, "chatfn(): pthread_mutex_unlock()");

                if (select(maxfd + 1, &set, NULL, NULL, NULL) < 0)
                        err(EX_OSERR, "chatfn(): select()");

                errno = pthread_mutex_lock(&c->c_lock);
                if (errno)
                        err(EX_SOFTWARE, "chatfn(): pthread_mutex_lock()");

                for (u = c->c_users; u; u = next) {
                        next = u->u_next;

                        if (!FD_ISSET(u->u_connfd, &set))
                                continue;

                        n = read(u->u_connfd, msg, BUFSIZ);
                        if (n <= 0) {
                                chat_rm_user(c, u->u_name);
                                continue;
                        }

                        msg[n] = 0;
                        printf("user %s sent msg=%s\n", u->u_name, msg);
                        for (v = c->c_users; v; v = v->u_next) {
                                if (write(v->u_connfd, msg, n) != n)
                                        err(EX_OSERR, "chatfn(): write()");
                        }
                }

                if (!c->c_users)
                        break;

                errno = pthread_mutex_unlock(&c->c_lock);
                if (errno)
                        err(EX_SOFTWARE, "chatfn(): pthread_mutex_unlock()");
        }

        return NULL;
}

static void
do_join(int connfd, char *cname, char *uname)
{
        struct chat *c;
        struct user *u;

        c = chat_find(cname);
        if (!c) {
                fd_puts(connfd, "chatroom does not exist");
                return;
        }

        u = chat_find_user(c, uname);
        if (u) {
                fd_puts(connfd, "user already in chatroom");
                return;
        }

        chat_add_user(c, uname, connfd);
}
