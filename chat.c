#include "chat.h"

/* chat hash constants */
enum {
        CHAT_HASH_SIZE = 117,
        CHAT_HASH_MULT = 31,
};

/* chatroom hash table */
static struct chat *chat_hash[CHAT_HASH_SIZE];
/* protects access to chat_hash */
static pthread_mutex_t chat_lock = PTHREAD_MUTEX_INITIALIZER;

struct user *
chat_add_user(struct chat *c, char *name, int connfd)
{
        struct user *u;

        u = malloc(sizeof(*u));
        if (!u)
                err(EX_SOFTWARE, "chat_add_user(): malloc()");

        u->u_name = strdup(name);
        if (!u->u_name)
                err(EX_SOFTWARE, "chat_add_user(): strdup()");

        u->u_connfd = connfd;
        u->u_next = c->c_users;
        c->c_users = u;
        return u;
}

void
chat_rm_user(struct chat *c, char *name)
{
        struct user **u;
        struct user *tmp;

        for (u = &c->c_users; *u; u = &(*u)->u_next) {
                if (!strcmp(name, (*u)->u_name))
                        break;
        }

        if (!*u)
                return;

        tmp = *u;
        *u = tmp->u_next;
        if (close(tmp->u_connfd))
                err(EX_OSERR, "chat_rm_user(): close()");
        free(tmp->u_name);
        free(tmp);
}

struct user *
chat_find_user(struct chat *c, char *name)
{
        struct user *u;

        for (u = c->c_users; u; u = u->u_next) {
                if (!strcmp(name, u->u_name))
                        return u;
        }

        return NULL;
}

static size_t chat_hashfn(char *name);

struct chat *
chat_new(char *name)
{
        struct chat *c;
        size_t bkt;

        c = malloc(sizeof(*c));
        if (!c)
                err(EX_SOFTWARE, "chat_new(): malloc()");

        c->c_name = strdup(name);
        if (!c->c_name)
                err(EX_SOFTWARE, "chat_new(): strdup()");

        errno = pthread_mutex_init(&c->c_lock, NULL);
        if (errno)
                err(EX_SOFTWARE, "chat_new(): pthread_mutex_init()");

        c->c_users = NULL;
        bkt = chat_hashfn(name);

        errno = pthread_mutex_lock(&chat_lock);
        if (errno)
                err(EX_SOFTWARE, "chat_new(): pthread_mutex_lock()");

        c->c_next = chat_hash[bkt];
        chat_hash[bkt] = c;

        errno = pthread_mutex_unlock(&chat_lock);
        if (errno)
                err(EX_SOFTWARE, "chat_new(): pthread_mutex_unlock()");

        return c;
}

static size_t
chat_hashfn(char *name)
{
        char *p;
        size_t hash;

        hash = 0;
        for (p = name; *p; p++)
                hash = hash * CHAT_HASH_MULT + *p;

        return hash % CHAT_HASH_SIZE;
}

void
chat_free(char *name)
{
        struct chat **c;
        struct chat *tmp;
        struct user *p;
        struct user *next;
        size_t bkt;

        bkt = chat_hashfn(name);

        errno = pthread_mutex_lock(&chat_lock);
        if (errno)
                err(EX_SOFTWARE, "chat_new(): pthread_mutex_lock()");

        for (c = &chat_hash[bkt]; *c; c = &(*c)->c_next) {
                if (!strcmp(name, (*c)->c_name))
                        break;
        }

        if (!*c) {
                errno = pthread_mutex_unlock(&chat_lock);
                if (errno)
                        err(EX_SOFTWARE, "chat_new(): pthread_mutex_unlock()");
                return;
        }

        tmp = *c;
        *c = tmp->c_next;
        errno = pthread_mutex_unlock(&chat_lock);
        if (errno)
                err(EX_SOFTWARE, "chat_new(): pthread_mutex_unlock()");

        for (p = tmp->c_users; p; p = next) {
                next = p->u_next;
                if (close(p->u_connfd))
                        err(EX_OSERR, "chat_free(): close()");
                free(p->u_name);
                free(p);
        }

        errno = pthread_mutex_destroy(&tmp->c_lock);
        if (errno)
                err(EX_SOFTWARE, "chat_free(): pthread_mutex_destroy()");

        free(tmp->c_name);
        free(tmp);
}

struct chat *
chat_find(char *name)
{
        struct chat *c;
        size_t bkt;

        bkt = chat_hashfn(name);

        errno = pthread_mutex_lock(&chat_lock);
        if (errno)
                err(EX_SOFTWARE, "chat_new(): pthread_mutex_lock()");

        for (c = chat_hash[bkt]; c; c = c->c_next) {
                if (!strcmp(name, c->c_name))
                        break;
        }

        errno = pthread_mutex_unlock(&chat_lock);
        if (errno)
                err(EX_SOFTWARE, "chat_new(): pthread_mutex_unlock()");

        return c;
}
