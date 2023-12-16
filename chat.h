#ifndef CHAT_H
#define CHAT_H

#include <err.h>
#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <unistd.h>

/* client info */
struct user {
        /* next on chat user list */
        struct user     *u_next;
        /* user name */
        char            *u_name;
        /* connected socket descriptor */
        int             u_connfd;
};

/* chat room */
struct chat {
        /* protects access to all fields of chat */
        pthread_mutex_t c_lock;
        /* next on chat hash list */
        struct chat     *c_next;
        /* user list */
        struct user     *c_users;
        /* chat room name */
        char            *c_name;
};

/**
 * Add a new user to chat room:
 *      must be called with c->c_lock held
 *
 * args:
 *      @c:             pointer to chat
 *      @name:          user name
 *      @connfd:        connected socket
 * ret:
 *      pointer to new user
 */
extern struct user *chat_add_user(struct chat *c, char *name, int connfd);

/**
 * Remove user from chat room:
 *      must be called with c->c_lock held
 *
 * args:
 *      @c:     pointer to chat
 *      @name:  user name
 * ret:
 *      nothing
 */
extern void chat_rm_user(struct chat *c, char *name);

/**
 * Search for a user by name:
 *      must be called with c->c_lock held
 *
 * args:
 *      @c:     pointer to chat
 *      @name:  user name
 * ret:
 *      @success:       pointer to user
 *      @failure:       NULL
 */
extern struct user *chat_find_user(struct chat *c, char *name);

/**
 * Create a new chat:
 *
 * args:
 *      @name:  name of chatroom
 * ret:
 *      pointer to new chat
 */
extern struct chat *chat_new(char *name);

/**
 * Free a chat:
 *
 * args:
 *      @name:  name of chatroom
 * ret:
 *      nothing
 */
extern void chat_free(char *name);

/**
 * Search for a chat:
 *
 * args:
 *      @name:  name of chatroom
 * ret:
 *      pointer to chat
 */
extern struct chat *chat_find(char *name);

#endif
