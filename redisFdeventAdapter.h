#ifndef __REDIS_FDEVENT_ADAPTER__
#define __REDIS_FDEVENT_ADAPTER__

#include "fdevent.h"
#include "hiredis.h"
#include "async.h"

#ifdef __cplusplus
extern "C" {
#endif

    int redisFdeventAttach(redisAsyncContext *ac, fdevents *ev);

#ifdef __cplusplus
}
#endif

#endif /* __REDIS_FDEVENT_ADAPTER__ */
