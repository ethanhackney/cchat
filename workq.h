#ifndef WORKQ_H
#define WORKQ_H

#include <err.h>
#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <sysexits.h>
#include <unistd.h>

/* client to service */
struct work {
        /* next on worklist */
        struct work     *w_next;
        /* connected socket descriptor */
        int             w_connfd;
};

/* work queue */
struct workq {
        /* protects access to all fields of workq */
        pthread_mutex_t q_lock;
        /* condition variable for when work is available */
        pthread_cond_t  q_haswork;
        /* head of work queue */
        struct work     *q_head;
        /* tail of work queue */
        struct work     **q_tail;
        /* function to call when more work is available */
        void            (*q_fn)(struct work *);
};

/**
 * Create a new workq:
 *
 * args:
 *      @nr_threads:    number of threads in workq
 *      @fn:            function to call when more work is available
 * ret:
 *      pointer to new workq
 */
extern struct workq *workq_new(size_t nr_threads, void (*fn)(struct work *));

/**
 * Free a workq:
 *
 * args:
 *      @q:     pointer to pointer to q
 * ret:
 *      *q set to NULL
 */
extern void workq_free(struct workq **q);

/**
 * Add work to queue:
 *
 * args:
 *      @q:             pointer to q
 *      @connfd:        connected socket descriptor
 * ret:
 *      nothing
 */
extern void workq_add(struct workq *q, int connfd);

#endif
