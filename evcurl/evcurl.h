#ifndef __evcurl_h__
#define __evcurl_h__

#include <ev.h>
#include <curl/curl.h>


typedef struct evcurl_processor_s
{
    struct ev_loop *loop;
    struct ev_timer timer_event;
    CURLM *multi;
    int still_running;
} evcurl_processor_t;

evcurl_processor_t *evcurl_create(struct ev_loop* loop);
void evcurl_destroy(evcurl_processor_t* __evcurl_pi);

void new_conn(char *url, evcurl_processor_t *mp);

#endif //__evcurl_h__

