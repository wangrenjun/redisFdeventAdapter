#include <stdlib.h>
#include "redisFdeventAdapter.h"

typedef struct redisFdeventEvents
{
    redisAsyncContext *context;
    int reading, writing;
    fdevents *ev;
    int fde_ndx;
} redisFdeventEvents;

static handler_t redisFdeventEvent(struct server *srv, void *context, int revents)
{
    redisFdeventEvents *e = (redisFdeventEvents *)context;

    if (revents & FDEVENT_IN) {
        redisAsyncHandleRead(e->context);
    }

    if (revents & FDEVENT_OUT) {
        redisAsyncHandleWrite(e->context);
    }

    return HANDLER_FINISHED;
}

static void redisFdeventAddRead(void *privdata)
{
    redisFdeventEvents *e = (redisFdeventEvents*)privdata;
    redisContext *c = &(e->context->c);
    if (!e->reading) {
        e->reading = 1;
        fdevent_event_set(e->ev, &e->fde_ndx, c->fd, FDEVENT_IN);
    }
}

static void redisFdeventDelRead(void *privdata)
{
    redisFdeventEvents *e = (redisFdeventEvents*)privdata;
    redisContext *c = &(e->context->c);
    if (e->reading) {
        e->reading = 0;
        fdevent_event_del(e->ev, &e->fde_ndx, c->fd);
    }
}

static void redisFdeventAddWrite(void *privdata)
{
    redisFdeventEvents *e = (redisFdeventEvents*)privdata;
    redisContext *c = &(e->context->c);
    if (!e->writing) {
        e->writing = 1;
        fdevent_event_set(e->ev, &e->fde_ndx, c->fd, FDEVENT_OUT);
    }
}

static void redisFdeventDelWrite(void *privdata)
{
    redisFdeventEvents *e = (redisFdeventEvents*)privdata;
    redisContext *c = &(e->context->c);
    if (e->writing) {
        e->writing = 0;
        fdevent_event_del(e->ev, &e->fde_ndx, c->fd);
    }
}

static void redisFdeventCleanup(void *privdata)
{
    redisFdeventEvents *e = (redisFdeventEvents*)privdata;
    redisContext *c = &(e->context->c);
    redisFdeventDelRead(privdata);
    redisFdeventDelWrite(privdata);
    fdevent_unregister(e->ev, c->fd);
    free(e);
}

int redisFdeventAttach(redisAsyncContext *ac, fdevents *ev)
{
    redisContext *c = &(ac->c);
    redisFdeventEvents *e;

    e = (redisFdeventEvents*)malloc(sizeof(*e));
    e->context = ac;
    e->reading = e->writing = 0;
    e->ev = ev;
    e->fde_ndx = -1;

    ac->ev.addRead = redisFdeventAddRead;
    ac->ev.delRead = redisFdeventDelRead;
    ac->ev.addWrite = redisFdeventAddWrite;
    ac->ev.delWrite = redisFdeventDelWrite;
    ac->ev.cleanup = redisFdeventCleanup;
    ac->ev.data = e;

    fdevent_register(ev, c->fd, redisFdeventEvent, e);

    return REDIS_OK;
}
