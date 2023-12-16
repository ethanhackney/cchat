#include "workq.h"
#include <stdio.h>

static void *workfn(void *arg);

struct workq *
workq_new(size_t nr_threads, void (*fn)(struct work *))
{
        struct workq *q;
        pthread_t tid;
        size_t i;

        q = malloc(sizeof(*q));
        if (!q)
                err(EX_SOFTWARE, "workq_new(): malloc()");

        q->q_head = NULL;
        q->q_tail = &q->q_head;
        q->q_fn = fn;
        q->q_exited = 0;
        q->q_exiting = 0;
        q->q_nr_threads = nr_threads;

        errno = pthread_mutex_init(&q->q_lock, NULL);
        if (errno)
                err(EX_SOFTWARE, "workq_new(): pthread_mutex_init()");

        errno = pthread_cond_init(&q->q_haswork, NULL);
        if (errno)
                err(EX_SOFTWARE, "workq_new(): pthread_cond_init()");

        errno = pthread_cond_init(&q->q_cond_exit, NULL);
        if (errno)
                err(EX_SOFTWARE, "workq_new(): pthread_cond_init()");

        for (i = 0; i < nr_threads; i++) {
                errno = pthread_create(&tid, NULL, workfn, q);
                if (errno)
                        err(EX_SOFTWARE, "workq_new(): pthread_create()");
        }

        return q;
}

static void *
workfn(void *arg)
{
        struct workq *q;
        struct work *w;

        errno = pthread_detach(pthread_self());
        if (errno)
                err(EX_SOFTWARE, "workfn(): pthread_detach()");

        q = arg;
        for (;;) {
                errno = pthread_mutex_lock(&q->q_lock);
                if (errno)
                        err(EX_SOFTWARE, "workfn(): pthread_mutex_lock()");

                while (!q->q_head && !q->q_exiting) {
                        errno = pthread_cond_wait(&q->q_haswork, &q->q_lock);
                        if (errno)
                                err(EX_SOFTWARE, "workfn(): pthread_cond_wait()");
                }

                if (q->q_exiting) {
                        if (++q->q_exited == q->q_nr_threads) {
                                errno = pthread_cond_signal(&q->q_cond_exit);
                                if (errno)
                                        err(EX_SOFTWARE, "workfn(): pthread_cond_signal()");
                        }
                        errno = pthread_mutex_unlock(&q->q_lock);
                        if (errno)
                                err(EX_SOFTWARE, "workfn(): pthread_mutex_unlock()");
                        break;
                }

                w = q->q_head;
                q->q_head = w->w_next;
                if (!q->q_head)
                        q->q_tail = &q->q_head;

                errno = pthread_mutex_unlock(&q->q_lock);
                if (errno)
                        err(EX_SOFTWARE, "workfn(): pthread_mutex_unlock()");

                q->q_fn(w);
                free(w);
        }

        return NULL;
}

void
workq_free(struct workq **q)
{
        struct work *next;
        struct work *p;

        errno = pthread_mutex_lock(&(*q)->q_lock);
        if (errno)
                err(EX_SOFTWARE, "workq_free(): pthread_mutex_lock()");

        (*q)->q_exiting = 1;

        errno = pthread_cond_broadcast(&(*q)->q_haswork);
        if (errno)
                err(EX_SOFTWARE, "workq_free(): pthread_cond_signal()");

        errno = pthread_cond_wait(&(*q)->q_cond_exit, &(*q)->q_lock);
        if (errno)
                err(EX_SOFTWARE, "workq_free(): pthread_cond_wait()");

        errno = pthread_mutex_unlock(&(*q)->q_lock);
        if (errno)
                err(EX_SOFTWARE, "workq_free(): pthread_mutex_unlock()");

        for (p = (*q)->q_head; p; p = next) {
                next = p->w_next;
                if (close(p->w_connfd))
                        err(EX_OSERR, "workq_free(): close()");
                free(p);
        }

        errno = pthread_mutex_unlock(&(*q)->q_lock);
        if (errno)
                err(EX_SOFTWARE, "workq_free(): pthread_mutex_unlock()");

        errno = pthread_mutex_destroy(&(*q)->q_lock);
        if (errno)
                err(EX_SOFTWARE, "workq_free(): pthread_mutex_destroy()");

        free(*q);
        *q = NULL;
}

void
workq_add(struct workq *q, int connfd)
{
        struct work *w;

        w = malloc(sizeof(*w));
        if (!w)
                err(EX_SOFTWARE, "workq_add(): malloc()");

        w->w_connfd = connfd;
        w->w_next = NULL;

        errno = pthread_mutex_lock(&q->q_lock);
        if (errno)
                err(EX_SOFTWARE, "workq_add(): pthread_mutex_lock()");

        *q->q_tail = w;
        q->q_tail = &w->w_next;

        errno = pthread_mutex_unlock(&q->q_lock);
        if (errno)
                err(EX_SOFTWARE, "workq_add(): pthread_mutex_unlock()");
}
