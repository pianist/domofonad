#include "evcurl.h"
#include <stdlib.h>

typedef struct evcurl_conn_info_s
{
    CURL *easy;
    char *url;
    evcurl_processor_t *mp;
    char error[CURL_ERROR_SIZE];
} evcurl_conn_info_t;

typedef struct evcurl_sock_info_s
{
    curl_socket_t sockfd;
    CURL *easy;
    int action;
    long timeout;
    struct ev_io ev;
    int evset;
    evcurl_processor_t *mp;
} evcurl_sock_info_t;



//////////////////////////////////////////////////////////////////////////////////

/* Die if we get a bad CURLMcode somewhere */
static void mcode_or_die(const char *where, CURLMcode code)
{
  if(CURLM_OK != code) {
    const char *s;
    switch(code) {
    case CURLM_BAD_HANDLE:
      s = "CURLM_BAD_HANDLE";
      break;
    case CURLM_BAD_EASY_HANDLE:
      s = "CURLM_BAD_EASY_HANDLE";
      break;
    case CURLM_OUT_OF_MEMORY:
      s = "CURLM_OUT_OF_MEMORY";
      break;
    case CURLM_INTERNAL_ERROR:
      s = "CURLM_INTERNAL_ERROR";
      break;
    case CURLM_UNKNOWN_OPTION:
      s = "CURLM_UNKNOWN_OPTION";
      break;
    case CURLM_LAST:
      s = "CURLM_LAST";
      break;
    default:
      s = "CURLM_unknown";
      break;
    case CURLM_BAD_SOCKET:
      s = "CURLM_BAD_SOCKET";
      fprintf(stderr, "ERROR: %s returns %s\n", where, s);
      /* ignore this error */
      return;
    }
    fprintf(stderr, "ERROR: %s returns %s\n", where, s);
    exit(code);
  }
}
















////////////////////////////////////////////////////////////////////////////////


static void check_multi_info(evcurl_processor_t *mp)
{
    char *eff_url;
    CURLMsg *msg;
    int msgs_left;
    evcurl_conn_info_t *conn;
    CURL *easy;
    CURLcode res;

    while((msg = curl_multi_info_read(mp->multi, &msgs_left)))
    {
        if(msg->msg == CURLMSG_DONE)
        {
            easy = msg->easy_handle;
            res = msg->data.result;
            curl_easy_getinfo(easy, CURLINFO_PRIVATE, &conn);
            curl_easy_getinfo(easy, CURLINFO_EFFECTIVE_URL, &eff_url);
            fprintf(stderr, "DONE: %s => (%d) %s\n", eff_url, res, conn->error);
            curl_multi_remove_handle(mp->multi, easy);
            free(conn->url);
            curl_easy_cleanup(easy);
            free(conn);
        }
    }
}

static void __evcurl_event_cb(EV_P_ struct ev_io *w, int revents)
{
    fprintf(stderr, "%s  w %p revents %i\n", __PRETTY_FUNCTION__, w, revents);

    evcurl_processor_t *mp = (evcurl_processor_t*)w->data;
    CURLMcode rc;

    int action = ((revents & EV_READ)  ? CURL_POLL_IN  : 0) |
                 ((revents & EV_WRITE) ? CURL_POLL_OUT : 0);

    rc = curl_multi_socket_action(mp->multi, w->fd, action, &mp->still_running);
    mcode_or_die("event_cb: curl_multi_socket_action", rc);
    check_multi_info(mp);
    if(mp->still_running <= 0)
    {
        fprintf(stderr, "last transfer done, kill timeout\n");
        ev_timer_stop(mp->loop, &mp->timer_event);
    }
}

static void __evcurl_timer_cb(EV_P_ struct ev_timer *w, int revents)
{
    evcurl_processor_t *g = (evcurl_processor_t*)w->data;

    CURLMcode rc;
    rc = curl_multi_socket_action(g->multi, CURL_SOCKET_TIMEOUT, 0, &g->still_running);

    mcode_or_die("timer_cb: curl_multi_socket_action", rc);
    check_multi_info(g);
}

static int __evcurl_multi_timer_cb(CURLM *multi, long timeout_ms, evcurl_processor_t *mp)
{
    printf("%s %li\n", __PRETTY_FUNCTION__,  timeout_ms);
    ev_timer_stop(mp->loop, &(mp->timer_event));
    if (timeout_ms >= 0)
    {
        /* -1 means delete, other values are timeout times in milliseconds */
        double t = timeout_ms / 1000;
        ev_timer_init(&mp->timer_event, __evcurl_timer_cb, t, 0.);
        ev_timer_start(mp->loop, &mp->timer_event);
    }
    return 0;
}

static void setsock(evcurl_sock_info_t *f, curl_socket_t s, CURL *e, int act, evcurl_processor_t *mp)
{
    printf("%s  \n", __PRETTY_FUNCTION__);
  
    int kind = ((act & CURL_POLL_IN) ? EV_READ : 0) |
               ((act & CURL_POLL_OUT) ? EV_WRITE : 0);

    f->sockfd = s;
    f->action = act;
    f->easy = e;

    if (f->evset) ev_io_stop(mp->loop, &f->ev);
    ev_io_init(&f->ev, __evcurl_event_cb, f->sockfd, kind);

    f->ev.data = mp;
    f->evset = 1;
    ev_io_start(mp->loop, &f->ev);
}

static void addsock(curl_socket_t s, CURL *easy, int action, evcurl_processor_t *mp)
{
    evcurl_sock_info_t *fdp = calloc(1, sizeof(evcurl_sock_info_t));

    fdp->mp = mp;
    setsock(fdp, s, easy, action, mp);
    curl_multi_assign(mp->multi, s, fdp);
}

static void remsock(evcurl_sock_info_t *f, evcurl_processor_t *mp)
{
    printf("%s  \n", __PRETTY_FUNCTION__);
    if (f)
    {
        if (f->evset) ev_io_stop(mp->loop, &f->ev);
        free(f);
    }
}

static int __evcurl_sock_cb(CURL *e, curl_socket_t s, int what, void *cbp, void *sockp)
{
    fprintf(stderr, "%s e %p s %i what %i cbp %p sockp %p\n", __PRETTY_FUNCTION__, e, s, what, cbp, sockp);

    evcurl_processor_t *g = (evcurl_processor_t*)cbp;
    evcurl_sock_info_t *fdp = (evcurl_sock_info_t*)sockp;

    const char *whatstr[]={ "none", "IN", "OUT", "INOUT", "REMOVE"};

    fprintf(stderr, "socket callback: s=%d e=%p what=%s ", s, e, whatstr[what]);

    if (what == CURL_POLL_REMOVE)
    {
        remsock(fdp, g);
    }
    else
    {
        if (!fdp)
        {
            addsock(s, e, what, g);
        }
        else
        {
            setsock(fdp, s, e, what, g);
        }
    }

    return 0;
}



evcurl_processor_t *evcurl_create(struct ev_loop* loop)
{
    evcurl_processor_t* __evcurl_pi = (evcurl_processor_t*)malloc(sizeof(evcurl_processor_t));

    memset(__evcurl_pi,  0, sizeof(evcurl_processor_t));

    __evcurl_pi->loop = loop;
    __evcurl_pi->multi = curl_multi_init();

    ev_timer_init(&(__evcurl_pi->timer_event), __evcurl_timer_cb, 0., 0.);
    __evcurl_pi->timer_event.data = __evcurl_pi;

    curl_multi_setopt(__evcurl_pi->multi, CURLMOPT_SOCKETFUNCTION, __evcurl_sock_cb);
    curl_multi_setopt(__evcurl_pi->multi, CURLMOPT_SOCKETDATA, __evcurl_pi);
    curl_multi_setopt(__evcurl_pi->multi, CURLMOPT_TIMERFUNCTION, __evcurl_multi_timer_cb);
    curl_multi_setopt(__evcurl_pi->multi, CURLMOPT_TIMERDATA, __evcurl_pi);

    return __evcurl_pi;
}

void evcurl_destroy(evcurl_processor_t* __evcurl_pi)
{
    curl_multi_cleanup(__evcurl_pi->multi);
    free(__evcurl_pi);
}

///////////////////////////////////////////////////////////////


/* CURLOPT_WRITEFUNCTION */
static size_t __XXX_write_cb(void *ptr, size_t size, size_t nmemb, void *data)
{
  size_t realsize = size * nmemb;
  evcurl_conn_info_t *conn = (evcurl_conn_info_t*) data;
  (void)ptr;
  (void)conn;
  return realsize;
}


/* CURLOPT_PROGRESSFUNCTION */
static int __XXX_prog_cb(void *p, double dltotal, double dlnow, double ult, double uln)
{
  evcurl_conn_info_t *conn = (evcurl_conn_info_t *)p;
  (void)ult;
  (void)uln;

  fprintf(stderr, "Progress: %s (%g/%g)\n", conn->url, dlnow, dltotal);
  return 0;
}

void new_conn(char *url, evcurl_processor_t *mp)
{
  evcurl_conn_info_t *conn;
  CURLMcode rc;

  conn = calloc(1, sizeof(evcurl_conn_info_t));
  conn->error[0]='\0';

  conn->easy = curl_easy_init();
  if(!conn->easy) {
    fprintf(stderr, "curl_easy_init() failed, exiting!\n");
    exit(2);
  }
  conn->mp = mp;
  conn->url = strdup(url);
  curl_easy_setopt(conn->easy, CURLOPT_URL, conn->url);
  curl_easy_setopt(conn->easy, CURLOPT_WRITEFUNCTION, __XXX_write_cb);
  curl_easy_setopt(conn->easy, CURLOPT_WRITEDATA, conn);
  curl_easy_setopt(conn->easy, CURLOPT_VERBOSE, 1L);
  curl_easy_setopt(conn->easy, CURLOPT_ERRORBUFFER, conn->error);
  curl_easy_setopt(conn->easy, CURLOPT_PRIVATE, conn);
  curl_easy_setopt(conn->easy, CURLOPT_NOPROGRESS, 0L);
  curl_easy_setopt(conn->easy, CURLOPT_PROGRESSFUNCTION, __XXX_prog_cb);
  curl_easy_setopt(conn->easy, CURLOPT_PROGRESSDATA, conn);
  curl_easy_setopt(conn->easy, CURLOPT_LOW_SPEED_TIME, 3L);
  curl_easy_setopt(conn->easy, CURLOPT_LOW_SPEED_LIMIT, 10L);

  fprintf(stderr, "Adding easy %p to multi %p (%s)\n", conn->easy, mp->multi, url);
  rc = curl_multi_add_handle(mp->multi, conn->easy);
  mcode_or_die("new_conn: curl_multi_add_handle", rc);

  /* note that the add_handle() will set a time-out to trigger very soon so
     that the necessary socket_action() call will be called by this app */
}







