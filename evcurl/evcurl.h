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


typedef struct evcurl_req_result_s
{
    CURLcode result;
    const char* effective_url;
    long response_code;
    const char* content_type;

    void* body;
    size_t sz_body;
} evcurl_req_result_t;

typedef void (*evcurl_http_req_done_cb)(evcurl_req_result_t* res);

CURLMcode evcurl_new_http_GET(evcurl_processor_t *mp, char *url, evcurl_http_req_done_cb _finish_cb);

#endif //__evcurl_h__

