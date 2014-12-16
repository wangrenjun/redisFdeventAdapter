#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include "hiredis.h"
#include "async.h"
#include "redisFdeventAdapter.h"
#include "fdevent.h"

void getCallback(redisAsyncContext *c, void *r, void *privdata) {
    redisReply *reply = r;
    if (reply == NULL) return;
    printf("argv[%s]: %s\n", (char*)privdata, reply->str);
}

void getCallback2(redisAsyncContext *c, void *r, void *privdata) {
    unsigned int j;
    redisReply *reply = r;
    if (reply == NULL) return;
    if (reply->type == REDIS_REPLY_ARRAY) {
        for (j = 0; j < reply->elements; j++) {
            printf("%u) %s\n", j, reply->element[j]->str);
        }
    }
    redisAsyncDisconnect(c);
}

void connectCallback(const redisAsyncContext *c, int status) {
    if (status != REDIS_OK) {
        printf("Error: %s\n", c->errstr);
        return;
    }
    printf("Connected...\n");
}

void disconnectCallback(const redisAsyncContext *c, int status) {
    if (status != REDIS_OK) {
        printf("Error: %s\n", c->errstr);
        return;
    }
    printf("Disconnected...\n");
}

int main(int argc, char **argv) {
    int n;

    signal(SIGPIPE, SIG_IGN);
    redisAsyncContext *c = redisAsyncConnect("127.0.0.1", 6379);
    if (c->err) {
        /* Let *c leak for now... */
        printf("Error: %s\n", c->errstr);
        return 1;
    }

    fdevents *ev = fdevent_init(NULL, 1024, FDEVENT_HANDLER_POLL);

    redisFdeventAttach(c, ev);
    redisAsyncSetConnectCallback(c, connectCallback);
    redisAsyncSetDisconnectCallback(c, disconnectCallback);
    redisAsyncCommand(c, NULL, NULL, "SET key %b", argv[argc - 1], strlen(argv[argc - 1]));
    redisAsyncCommand(c, getCallback, (char*)"end-1", "GET key");

    for (n = 0; n < 1000; n++) {
        redisAsyncCommand(c, NULL, NULL, "LPUSH mylist hello-%d", n);
    }
    redisAsyncCommand(c, getCallback2, (char*) "LRANGE", "LRANGE mylist 0 -1");

    while ((n = fdevent_poll(ev, 1000)) > 0) {
        int fd_ndx = -1;
        do {
            int revents;
            int fd;
            fdevent_handler handler;
            void *context;
            handler_t r;

            fd_ndx = fdevent_event_next_fdndx(ev, fd_ndx);
            if (fd_ndx == -1)
                break;
            revents = fdevent_event_get_revent(ev, fd_ndx);
            fd = fdevent_event_get_fd(ev, fd_ndx);
            handler = fdevent_get_handler(ev, fd);
            context = fdevent_get_context(ev, fd);

            switch (r = (*handler)(NULL, context, revents)) {
            case HANDLER_FINISHED:
            case HANDLER_GO_ON:
                break;

            default:
                exit(0);
                break;
            }
        } while (--n > 0);
    }
    fdevent_free(ev);

    return 0;
}
